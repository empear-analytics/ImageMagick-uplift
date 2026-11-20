/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private composite methods: virtual pixels, compose arguments, the copy composite and the blur and displace passes.
*/
#ifndef MAGICKCORE_COMPOSITE_IMAGE_VIRTUAL_PRIVATE_H
#define MAGICKCORE_COMPOSITE_IMAGE_VIRTUAL_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _VirtualCompositeInputs
{
  const Image
    *image,
    *source_image;

  CompositeOperator
    compose;

  MagickRealType
    canvas_dissolve;

  MagickBooleanType
    clamp,
    canvas_pixel;
} VirtualCompositeInputs;

static void SetVirtualDissolvePixel(const VirtualCompositeInputs *composite,
  const PixelChannel channel,const Quantum *source,MagickRealType *pixel)
{
  switch (composite->compose)
  {
    case BlendCompositeOp:
    case DissolveCompositeOp:
    {
      if (channel == AlphaPixelChannel)
        (*pixel)=composite->canvas_dissolve*(double) GetPixelAlpha(
          composite->source_image,source);
      else
        (*pixel)=(MagickRealType) source[channel];
      break;
    }
    default:
    {
      (*pixel)=(MagickRealType) source[channel];
      break;
    }
  }
}

static void SetVirtualSourcePixel(const VirtualCompositeInputs *composite,
  const PixelChannel channel,const Quantum *source,MagickRealType *pixel)
{
  switch (composite->compose)
  {
    case ClearCompositeOp:
    case CopyCompositeOp:
    case ReplaceCompositeOp:
    {
      if (channel == AlphaPixelChannel)
        (*pixel)=(MagickRealType) TransparentAlpha;
      else
        (*pixel)=0.0;
      break;
    }
    default:
    {
      SetVirtualDissolvePixel(composite,channel,source,&(*pixel));
      break;
    }
  }
}

static MagickRealType GetVirtualCanvasPixel(const PixelChannel channel,
  const Quantum *magick_restrict q,const ssize_t i)
{
  MagickRealType
    pixel;

  if (channel == AlphaPixelChannel)
    pixel=(MagickRealType) TransparentAlpha;
  else
    pixel=(MagickRealType) q[i];
  return(pixel);
}

static const CompositeOperator
  CanvasPixelOperators[] =
  {
    AlphaCompositeOp,
    ChangeMaskCompositeOp,
    CopyAlphaCompositeOp,
    DstAtopCompositeOp,
    DstInCompositeOp,
    InCompositeOp,
    OutCompositeOp,
    SrcInCompositeOp,
    SrcOutCompositeOp
  };

static inline void SetVirtualCompositeChannels(
  const VirtualCompositeInputs *composite,const Quantum *source,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(composite->image); i++)
  {
    MagickRealType
      pixel = 0.0;

    PixelChannel channel = GetPixelChannelChannel(composite->image,i);
    PixelTrait traits = GetPixelChannelTraits(composite->image,channel);
    PixelTrait source_traits = GetPixelChannelTraits(composite->source_image,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (source_traits == UndefinedPixelTrait))
      continue;
    if (composite->canvas_pixel != MagickFalse)
      pixel=GetVirtualCanvasPixel(channel,q,i);
    else
      SetVirtualSourcePixel(composite,channel,source,&pixel);
    q[i]=composite->clamp != MagickFalse ? ClampPixel(pixel) :
      ClampToQuantum(pixel);
  }
}

static inline void CopySourceChannels(const Image *source_image,
  const Image *image,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(source_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(source_image,i);
    PixelTrait source_traits = GetPixelChannelTraits(source_image,
      channel);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((source_traits == UndefinedPixelTrait) ||
        (traits == UndefinedPixelTrait))
      continue;
    SetPixelChannel(image,channel,p[i],q);
  }
}

static IlluminantType ParseComposeIlluminant(const char *artifact)
{
  IlluminantType
    illuminant;

  ssize_t
    illuminant_type;

  illuminant_type=ParseCommandOption(MagickIlluminantOptions,MagickFalse,
    artifact);
  if (illuminant_type < 0)
    illuminant=UndefinedIlluminant;
  else
    illuminant=(IlluminantType) illuminant_type;
  return(illuminant);
}

static ColorspaceType ParseComposeColorspace(const char *artifact)
{
  ColorspaceType
    colorspace;

  ssize_t
    colorspace_type;

  colorspace_type=ParseCommandOption(MagickColorspaceOptions,MagickFalse,
    artifact);
  if (colorspace_type < 0)
    colorspace=UndefinedColorspace;
  else
    colorspace=(ColorspaceType) colorspace_type;
  return(colorspace);
}

static inline double GetCompositeGamma(const CompositeOperator compose,
  const MagickRealType alpha)
{
  double
    gamma;

  switch (compose)
  {
    case DarkenCompositeOp:
    case LightenCompositeOp:
    case ModulusSubtractCompositeOp:
    {
      gamma=MagickSafeReciprocal(1.0-alpha);
      break;
    }
    default:
    {
      gamma=MagickSafeReciprocal(alpha);
      break;
    }
  }
  return(gamma);
}

static MagickRealType GetHueAlpha(const MagickRealType Sa,
  const MagickRealType Da,MagickRealType pixel)
{
  if (fabs((double) QuantumRange*Sa-(double) TransparentAlpha) < MagickEpsilon)
    {
      pixel=(double) QuantumRange*Da;
      return(pixel);
    }
  if (fabs((double) QuantumRange*Da-(double) TransparentAlpha) < MagickEpsilon)
    {
      pixel=(double) QuantumRange*Sa;
      return(pixel);
    }
  if (Sa < Da)
    {
      pixel=(double) QuantumRange*Da;
      return(pixel);
    }
  pixel=(double) QuantumRange*Sa;
  return(pixel);
}

static MagickRealType GetCopyAlpha(const Image *source_image,
  const MagickRealType Si,const MagickRealType Sa)
{
  MagickRealType
    pixel;

  if (source_image->alpha_trait == UndefinedPixelTrait)
    pixel=Si;
  else
    pixel=(double) QuantumRange*Sa;
  return(pixel);
}

static MagickRealType GetFreezeAlpha(const MagickRealType Sa,
  const MagickRealType Da)
{
  MagickRealType
    pixel;

  pixel=(double) QuantumRange*(1.0-(1.0-Sa)*(1.0-Sa)*
    MagickSafeReciprocal(Da));
  if (pixel < 0.0)
    pixel=0.0;
  return(pixel);
}

static MagickRealType GetReflectAlpha(const MagickRealType Sa,
  const MagickRealType Da)
{
  MagickRealType
    pixel;

  pixel=(double) QuantumRange*(Sa*Sa*
    MagickSafeReciprocal(1.0-Da));
  if (pixel > (double) QuantumRange)
    pixel=(double) QuantumRange;
  return(pixel);
}

static void SetDissolveFactors(const GeometryInfo geometry_info,
  const MagickStatusType flags,MagickRealType *source_dissolve,
  MagickRealType *canvas_dissolve)
{
  *source_dissolve=geometry_info.rho/100.0;
  *canvas_dissolve=1.0;
  if ((*source_dissolve-MagickEpsilon) < 0.0)
    *source_dissolve=0.0;
  if ((*source_dissolve+MagickEpsilon) > 1.0)
    {
      *canvas_dissolve=2.0-*source_dissolve;
      *source_dissolve=1.0;
    }
  if ((flags & SigmaValue) != 0)
    *canvas_dissolve=geometry_info.sigma/100.0;
  if ((*canvas_dissolve-MagickEpsilon) < 0.0)
    *canvas_dissolve=0.0;
  if ((*canvas_dissolve+MagickEpsilon) > 1.0)
    *canvas_dissolve=1.0;
}

static void SetBlendFactors(const GeometryInfo geometry_info,
  const MagickStatusType flags,MagickRealType *source_dissolve,
  MagickRealType *canvas_dissolve)
{
  *source_dissolve=geometry_info.rho/100.0;
  *canvas_dissolve=1.0-*source_dissolve;
  if ((flags & SigmaValue) != 0)
    *canvas_dissolve=geometry_info.sigma/100.0;
}

typedef struct _ChangeMaskImages
{
  Image
    *image,
    *source_image;
} ChangeMaskImages;

static MagickRealType GetChangeMaskAlpha(const ChangeMaskImages *images,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  const MagickRealType Da)
{
  MagickRealType
    pixel;

  if (IsFuzzyEquivalencePixel(images->source_image,p,images->image,q) !=
      MagickFalse)
    pixel=(MagickRealType) TransparentAlpha;
  else
    pixel=(double) QuantumRange*Da;
  return(pixel);
}

typedef struct _IntensityAlphaInputs
{
  MagickBooleanType
    compose_sync;

  MagickRealType
    Sa,
    Da,
    Si,
    Di;
} IntensityAlphaInputs;

static MagickRealType GetDarkenIntensityAlpha(
  const IntensityAlphaInputs *intensity,MagickRealType pixel)
{
  if (intensity->compose_sync == MagickFalse)
    {
      pixel=intensity->Si < intensity->Di ? intensity->Sa : intensity->Da;
      return(pixel);
    }
  pixel=intensity->Sa*intensity->Si < intensity->Da*intensity->Di ?
    intensity->Sa : intensity->Da;
  return(pixel);
}

static MagickRealType GetLightenIntensityAlpha(
  const IntensityAlphaInputs *intensity,MagickRealType pixel)
{
  const MagickBooleanType
    compose_sync = intensity->compose_sync;

  const MagickRealType
    Sa = intensity->Sa,
    Da = intensity->Da,
    Si = intensity->Si,
    Di = intensity->Di;

  if (compose_sync == MagickFalse)
    {
      pixel=Si > Di ? Sa : Da;
      return(pixel);
    }
  pixel=Sa*Si > Da*Di ? Sa : Da;
  return(pixel);
}

typedef struct _MultiplyAlphaInputs
{
  MagickBooleanType
    compose_sync;

  MagickRealType
    alpha,
    Sa,
    Da;
} MultiplyAlphaInputs;

static MagickRealType GetMultiplyAlpha(const MultiplyAlphaInputs *multiply,
  MagickRealType pixel)
{
  if (multiply->compose_sync == MagickFalse)
    {
      pixel=(double) QuantumRange*multiply->Sa*multiply->Da;
      return(pixel);
    }
  pixel=(double) QuantumRange*multiply->alpha;
  return(pixel);
}

static void CopyCompositeRow(const Image *source_image,const Image *image,
  const Quantum *p,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    if (GetPixelReadMask(source_image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(source_image);
        q+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    CopySourceChannels(source_image,image,p,q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef struct _IntensityRowInputs
{
  Image
    *image,
    *source_image;

  MagickBooleanType
    clamp;
} IntensityRowInputs;

static void SetIntensityAlphaRow(const IntensityRowInputs *row,
  const Quantum *p,Quantum *q)
{
  Image
    *const image = row->image,
    *const source_image = row->source_image;

  const MagickBooleanType
    clamp = row->clamp;

  ssize_t
    x;

  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    if (GetPixelReadMask(source_image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(source_image);
        q+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    SetPixelAlpha(image,clamp != MagickFalse ?
      ClampPixel(GetPixelIntensity(source_image,p)) :
      ClampToQuantum(GetPixelIntensity(source_image,p)),q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef struct _CompositeSetup
{
  ssize_t
    x_offset,
    y_offset;

  MagickBooleanType
    clamp,
    intensity;
} CompositeSetup;

static void CompositeCopyRow(const IntensityRowInputs *row,
  const MagickBooleanType intensity,const Quantum *p,Quantum *q)
{
  if (intensity == MagickFalse)
    CopyCompositeRow(row->source_image,row->image,p,q);
  else
    SetIntensityAlphaRow(row,p,q);
}

static MagickBooleanType CompositeCopyImage(Image *image,Image *source_image,
  const CompositeSetup *setup,ExceptionInfo *exception)
{
  const ssize_t
    x_offset = setup->x_offset,
    y_offset = setup->y_offset;

  const IntensityRowInputs
    row = { image, source_image, setup->clamp };

  CacheView
    *image_view,
    *source_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  source_view=AcquireVirtualCacheView(source_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(source_image,image,source_image->rows,4)
#endif
  for (y=0; y < (ssize_t) source_image->rows; y++)
  {
    const Quantum
      *p;

    Quantum
      *q;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(source_view,0,y,source_image->columns,1,
      exception);
    q=GetCacheViewAuthenticPixels(image_view,x_offset,y+y_offset,
      source_image->columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    CompositeCopyRow(&row,setup->intensity,p,q);
    SyncCompositeRow(image_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        SetCompositeProgress(image,CompositeImageTag,(MagickOffsetType) y,
          &status);
      }
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  source_image=DestroyImage(source_image);
  return(status);
}

typedef struct _DisplaceSetup
{
  CompositeOperator
    compose;

  MagickStatusType
    flags;

  GeometryInfo
    geometry_info;

  Image
    *image,
    *source_image;

  ssize_t
    x_offset,
    y_offset;
} DisplaceSetup;

static void SetDisplaceScaleArguments(const DisplaceSetup *displace,
  MagickRealType *horizontal_scale,MagickRealType *vertical_scale)
{
  (*horizontal_scale)=displace->geometry_info.rho;
  (*vertical_scale)=displace->geometry_info.sigma;
  if ((displace->flags & PercentValue) != 0)
    {
      if ((displace->flags & AspectValue) == 0)
        {
          (*horizontal_scale)*=(displace->source_image->columns-1)/200.0;
          (*vertical_scale)*=(displace->source_image->rows-1)/200.0;
        }
      else
        {
          (*horizontal_scale)*=(displace->image->columns-1)/200.0;
          (*vertical_scale)*=(displace->image->rows-1)/200.0;
        }
    }
  if ((displace->flags & HeightValue) == 0)
    (*vertical_scale)=(*horizontal_scale);
}

static void SetDisplaceScaleDefaults(const DisplaceSetup *displace,
  MagickRealType *horizontal_scale,MagickRealType *vertical_scale)
{
  if ((displace->flags & AspectValue) == 0)
    {
      (*horizontal_scale)=(MagickRealType) (displace->source_image->columns-1)/2.0;
      (*vertical_scale)=(MagickRealType) (displace->source_image->rows-1)/2.0;
    }
  else
    {
      (*horizontal_scale)=(MagickRealType) (displace->image->columns-1)/2.0;
      (*vertical_scale)=(MagickRealType) (displace->image->rows-1)/2.0;
    }
}

static void SetDistortCenterX(const DisplaceSetup *displace,PointInfo *center)
{
  if ((displace->flags & XValue) == 0)
    if ((displace->flags & AspectValue) != 0)
      (*center).x=(MagickRealType) ((displace->image->columns-1)/2.0);
    else
      (*center).x=(MagickRealType) (displace->x_offset+(displace->source_image->columns-1)/
        2.0);
  else
    if ((displace->flags & AspectValue) != 0)
      (*center).x=displace->geometry_info.xi;
    else
      (*center).x=(MagickRealType) (displace->x_offset+displace->geometry_info.xi);
}

static void SetDistortCenter(const DisplaceSetup *displace,PointInfo *center)
{
  if (displace->compose == DistortCompositeOp)
    {
      SetDistortCenterX(displace,center);
      if ((displace->flags & YValue) == 0)
        if ((displace->flags & AspectValue) != 0)
          (*center).y=(MagickRealType) ((displace->image->rows-1)/2.0);
        else
          (*center).y=(MagickRealType) (displace->y_offset+(displace->source_image->rows-1)/2.0);
      else
        if ((displace->flags & AspectValue) != 0)
          (*center).y=displace->geometry_info.psi;
        else
          (*center).y=(MagickRealType) (displace->y_offset+displace->geometry_info.psi);
    }
}

static void GetBlendIterations(const Image *image,double *iterations,
  double *residual_threshold,size_t *tick)
{
  const char
    *artifact;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,&geometry_info);
      (*iterations)=geometry_info.rho;
      if ((flags & SigmaValue) != 0)
        (*residual_threshold)=geometry_info.sigma;
      if ((flags & XiValue) != 0)
        (*tick)=(size_t) geometry_info.xi;
    }
}

static void GetMathematicsArguments(const Image *image,
  GeometryInfo *geometry_info,ExceptionInfo *exception)
{
  const char
    *artifact;

  MagickStatusType
    flags;

  SetGeometryInfo(geometry_info);
  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,geometry_info);
      if (flags == NoValue)
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidGeometry","`%s'",artifact);
    }
}

static void GetModulateArguments(const Image *image,
  GeometryInfo *geometry_info,MagickRealType *percent_luma,
  MagickRealType *percent_chroma)
{
  const char
    *artifact;

  MagickStatusType
    flags;

  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,geometry_info);
      (*percent_luma)=(*geometry_info).rho;
      if ((flags & SigmaValue) != 0)
        (*percent_chroma)=(*geometry_info).sigma;
    }
}

static void GetThresholdArguments(const Image *image,
  GeometryInfo *geometry_info,MagickRealType *amount,MagickRealType *threshold)
{
  const char
    *artifact;

  MagickStatusType
    flags;

  artifact=GetImageArtifact(image,"compose:args");
  if (artifact != (char *) NULL)
    {
      flags=ParseGeometry(artifact,geometry_info);
      (*amount)=(*geometry_info).rho;
      (*threshold)=(*geometry_info).sigma;
      if ((flags & SigmaValue) == 0)
        (*threshold)=0.05f;
    }
  (*threshold)*=(double) QuantumRange;
}

typedef struct _BlurComposite
{
  double
    angle_range,
    angle_start,
    height,
    width;

  Image
    *canvas_image,
    *image,
    *source_image;

  CacheView
    *canvas_view,
    *source_view;

  ResampleFilter
    *resample_filter;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} BlurComposite;

static void RotateBlurEllipse(const BlurComposite *blur_composite,
  const Quantum *p,SegmentInfo *blur)
{
  const double
    angle_range = blur_composite->angle_range,
    angle_start = blur_composite->angle_start,
    height = blur_composite->height,
    width = blur_composite->width;

  Image
    *const source_image = blur_composite->source_image;

  if (fabs(angle_range) > MagickEpsilon)
    {
      MagickRealType
        angle;

      angle=angle_start+angle_range*QuantumScale*(double) 
        GetPixelBlue(source_image,p);
      (*blur).x1=width*cos(angle);
      (*blur).x2=width*sin(angle);
      (*blur).y1=(-height*sin(angle));
      (*blur).y2=height*cos(angle);
    }
}

typedef struct _BlurRow
{
  ssize_t
    y;

  const Quantum
    *p;

  Quantum
    *q;
} BlurRow;

static void BlurCompositeColumns(const BlurComposite *blur_composite,
  const BlurRow *row,SegmentInfo *blur,PixelInfo *pixel)
{
  Image
    *const canvas_image = blur_composite->canvas_image,
    *const image = blur_composite->image,
    *const source_image = blur_composite->source_image;

  ResampleFilter
    *const resample_filter = blur_composite->resample_filter;

  const ssize_t
    x_offset = blur_composite->x_offset,
    y_offset = blur_composite->y_offset,
    y = row->y;

  ExceptionInfo
    *const exception = blur_composite->exception;

  const Quantum
    *p = row->p;

  Quantum
    *q = row->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    if (((x_offset+x) < 0) || ((x_offset+x) >= (ssize_t) image->columns))
      {
        p+=(ptrdiff_t) GetPixelChannels(source_image);
        continue;
      }
    RotateBlurEllipse(blur_composite,p,blur);
    ScaleResampleFilter(resample_filter,
      (*blur).x1*QuantumScale*(double) GetPixelRed(source_image,p),
      (*blur).y1*QuantumScale*(double) GetPixelGreen(source_image,p),
      (*blur).x2*QuantumScale*(double) GetPixelRed(source_image,p),
      (*blur).y2*QuantumScale*(double) GetPixelGreen(source_image,p) );
    (void) ResamplePixelColor(resample_filter,(double) x_offset+x,
      (double) y_offset+y,pixel,exception);
    SetPixelViaPixelInfo(canvas_image,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(canvas_image);
  }
}

static void BlurCompositeRows(const BlurComposite *blur_composite,
  SegmentInfo *blur,PixelInfo *pixel)
{
  Image
    *const canvas_image = blur_composite->canvas_image,
    *const image = blur_composite->image,
    *const source_image = blur_composite->source_image;

  CacheView
    *const canvas_view = blur_composite->canvas_view,
    *const source_view = blur_composite->source_view;

  const ssize_t
    y_offset = blur_composite->y_offset;

  ExceptionInfo
    *const exception = blur_composite->exception;

  ssize_t
    y;

  for (y=0; y < (ssize_t) source_image->rows; y++)
  {
    MagickBooleanType
      sync;

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    BlurRow
      row;

    if (((y+y_offset) < 0) || ((y+y_offset) >= (ssize_t) image->rows))
      continue;
    p=GetCacheViewVirtualPixels(source_view,0,y,source_image->columns,1,
      exception);
    q=QueueCacheViewAuthenticPixels(canvas_view,0,y,canvas_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      break;
    row.y=y;
    row.p=p;
    row.q=q;
    BlurCompositeColumns(blur_composite,&row,blur,pixel);
    sync=SyncCacheViewAuthenticPixels(canvas_view,exception);
    if (sync == MagickFalse)
      break;
  }
}

typedef struct _DisplaceColumns
{
  Image
    *canvas_image,
    *image,
    *source_image;

  CacheView
    *image_view;

  PointInfo
    center;

  CompositeOperator
    compose;

  MagickRealType
    horizontal_scale,
    vertical_scale;

  ssize_t
    x_offset,
    y;

  ExceptionInfo
    *exception;
} DisplaceColumns;

static MagickBooleanType DisplaceCompositeColumns(
  const DisplaceColumns *columns,const Quantum *magick_restrict p,
  Quantum *magick_restrict q,PixelInfo *pixel)
{
  Image
    *const canvas_image = columns->canvas_image,
    *const image = columns->image,
    *const source_image = columns->source_image;

  CacheView
    *const image_view = columns->image_view;

  const PointInfo
    center = columns->center;

  const CompositeOperator
    compose = columns->compose;

  const MagickRealType
    horizontal_scale = columns->horizontal_scale,
    vertical_scale = columns->vertical_scale;

  const ssize_t
    x_offset = columns->x_offset,
    y = columns->y;

  ExceptionInfo
    *const exception = columns->exception;

  MagickBooleanType
    status;

  PointInfo
    offset;

  ssize_t
    x;

  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    if (((x_offset+x) < 0) || ((x_offset+x) >= (ssize_t) image->columns))
      {
        p+=(ptrdiff_t) GetPixelChannels(source_image);
        continue;
      }
    /*
      Displace the offset.
    */
    offset.x=(double) (horizontal_scale*((double) GetPixelRed(
      source_image,p)-(((MagickRealType) QuantumRange+1.0)/2.0)))/
      (((MagickRealType) QuantumRange+1.0)/2.0)+center.x+
      ((compose == DisplaceCompositeOp) ? x : 0);
    offset.y=(double) (vertical_scale*((double) GetPixelGreen(
      source_image,p)-(((MagickRealType) QuantumRange+1.0)/2.0)))/
      (((MagickRealType) QuantumRange+1.0)/2.0)+center.y+
      ((compose == DisplaceCompositeOp) ? y : 0);
    status=InterpolatePixelInfo(image,image_view,
      UndefinedInterpolatePixel,(double) offset.x,(double) offset.y,
      pixel,exception);
    if (status == MagickFalse)
      break;
    /*
      Mask with the 'invalid (*pixel) mask' in alpha channel.
    */
    (*pixel).alpha=(MagickRealType) QuantumRange*(QuantumScale*(*pixel).alpha)*
      (QuantumScale*(double) GetPixelAlpha(source_image,p));
    SetPixelViaPixelInfo(canvas_image,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(canvas_image);
  }
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
