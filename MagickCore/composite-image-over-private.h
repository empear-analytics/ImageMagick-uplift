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

  MagickCore private composite methods: the over composite and the blend relaxation of the saliency and seamless composites.
*/
#ifndef MAGICKCORE_COMPOSITE_IMAGE_OVER_PRIVATE_H
#define MAGICKCORE_COMPOSITE_IMAGE_OVER_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsOutsideSourceColumn(const Quantum *pixels,
  const ssize_t x,const ssize_t x_offset,const Image *source_image)
{
  return(((pixels == (Quantum *) NULL) || (x < x_offset) ||
    ((x-x_offset) >= (ssize_t) source_image->columns)) ? MagickTrue :
    MagickFalse);
}

typedef struct _OverChannelInputs
{
  const Image
    *source_image;

  const Quantum
    *p;

  PixelChannel
    channel;

  PixelTrait
    traits;

  MagickRealType
    alpha,
    Da,
    Sa;

  MagickBooleanType
    clamp;
} OverChannelInputs;

static void CompositeOverChannel(const OverChannelInputs *over,
  Quantum *magick_restrict q,const ssize_t i)
{
  double
    gamma;

  MagickRealType
    Dc,
    Dca,
    pixel,
    Sc,
    Sca;

  if (over->channel == AlphaPixelChannel)
    {
      /*
        Set alpha channel.
      */
      pixel=(double) QuantumRange*over->alpha;
      q[i]=over->clamp != MagickFalse ? ClampPixel(pixel) :
        ClampToQuantum(pixel);
      return;
    }
  /*
    Sc: source color.
    Dc: canvas color.
  */
  Sc=(MagickRealType) GetPixelChannel(over->source_image,over->channel,
    over->p);
  Dc=(MagickRealType) q[i];
  if ((over->traits & CopyPixelTrait) != 0)
    {
      /*
        Copy channel.
      */
      q[i]=ClampToQuantum(Sc);
      return;
    }
  /*
    Porter-Duff compositions:
      Sca: source normalized color multiplied by alpha.
      Dca: normalized canvas color multiplied by alpha.
  */
  Sca=QuantumScale*over->Sa*Sc;
  Dca=QuantumScale*over->Da*Dc;
  gamma=MagickSafeReciprocal(over->alpha);
  pixel=(double) QuantumRange*gamma*(Sca+Dca*(1.0-over->Sa));
  q[i]=over->clamp != MagickFalse ? ClampPixel(pixel) :
    ClampToQuantum(pixel);
}

typedef struct _OverPixelInputs
{
  Image
    *image;

  const Image
    *source_image;

  const Quantum
    *p;

  MagickRealType
    alpha,
    Da,
    Sa;

  MagickBooleanType
    clamp;
} OverPixelInputs;

static void CompositeOverPixelChannels(const OverPixelInputs *authentic,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(authentic->image); i++)
  {
    OverChannelInputs
      over;

    PixelChannel channel = GetPixelChannelChannel(authentic->image,i);
    PixelTrait traits = GetPixelChannelTraits(authentic->image,channel);
    PixelTrait source_traits=GetPixelChannelTraits(authentic->source_image,
      channel);
    if (traits == UndefinedPixelTrait)
      continue;
    if ((source_traits == UndefinedPixelTrait) &&
        (channel != AlphaPixelChannel))
        continue;
    over.source_image=authentic->source_image;
    over.p=authentic->p;
    over.channel=channel;
    over.traits=traits;
    over.alpha=authentic->alpha;
    over.Da=authentic->Da;
    over.Sa=authentic->Sa;
    over.clamp=authentic->clamp;
    CompositeOverChannel(&over,q,i);
  }
}

typedef struct _VirtualOverInputs
{
  Image
    *image;

  const Image
    *source_image;

  ssize_t
    x,
    x_offset,
    y,
    y_offset;

  MagickBooleanType
    clamp;

  ExceptionInfo
    *exception;
} VirtualOverInputs;

static void CompositeVirtualOverPixel(const VirtualOverInputs *virtual_pixel,
  Quantum *magick_restrict q)
{
  Quantum
    source[MaxPixelChannels];

  ssize_t
    i;

  (void) GetOneVirtualPixel(virtual_pixel->source_image,
    CastDoubleToSsizeT((double) virtual_pixel->x-virtual_pixel->x_offset),
    CastDoubleToSsizeT((double) virtual_pixel->y-virtual_pixel->y_offset),
    source,virtual_pixel->exception);
  for (i=0; i < (ssize_t) GetPixelChannels(virtual_pixel->image); i++)
  {
    MagickRealType
      pixel;

    PixelChannel channel = GetPixelChannelChannel(virtual_pixel->image,i);
    PixelTrait traits = GetPixelChannelTraits(virtual_pixel->image,channel);
    PixelTrait source_traits=GetPixelChannelTraits(virtual_pixel->source_image,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (source_traits == UndefinedPixelTrait))
      continue;
    if (channel == AlphaPixelChannel)
      pixel=(MagickRealType) TransparentAlpha;
    else
      pixel=(MagickRealType) q[i];
    q[i]=virtual_pixel->clamp != MagickFalse ? ClampPixel(pixel) :
      ClampToQuantum(pixel);
  }
}

static const Quantum *AdvanceOverSourcePixel(const Image *source_image,
  const Quantum *pixels,const Quantum *p)
{
  size_t
    channels;

  p+=(ptrdiff_t) GetPixelChannels(source_image);
  channels=GetPixelChannels(source_image);
  if (p >= (pixels+channels*source_image->columns))
    p=pixels;
  return(p);
}

typedef struct _OverRowInputs
{
  Image
    *image;

  const Image
    *source_image;

  const Quantum
    *pixels;

  MagickBooleanType
    clip_to_self;

  ssize_t
    x_offset,
    y_offset,
    y;

  MagickBooleanType
    clamp;

  ExceptionInfo
    *exception;
} OverRowInputs;

static void CompositeOverRow(const OverRowInputs *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) row->image->columns; x++)
  {
    MagickRealType
      alpha,
      Da,
      Sa;

    OverPixelInputs
      authentic;

    if (row->clip_to_self != MagickFalse)
      {
        if (x < row->x_offset)
          {
            q+=(ptrdiff_t) GetPixelChannels(row->image);
            continue;
          }
        if ((x-row->x_offset) >= (ssize_t) row->source_image->columns)
          break;
      }
    if (IsOutsideSourceColumn(row->pixels,x,row->x_offset,
          row->source_image) != MagickFalse)
      {
        VirtualOverInputs
          virtual_pixel;

        /*
          Virtual composite:
            Sc: source color.
            Dc: canvas color.
        */
        virtual_pixel.image=row->image;
        virtual_pixel.source_image=row->source_image;
        virtual_pixel.x=x;
        virtual_pixel.x_offset=row->x_offset;
        virtual_pixel.y=row->y;
        virtual_pixel.y_offset=row->y_offset;
        virtual_pixel.clamp=row->clamp;
        virtual_pixel.exception=row->exception;
        CompositeVirtualOverPixel(&virtual_pixel,q);
        q+=(ptrdiff_t) GetPixelChannels(row->image);
        continue;
      }
    /*
      Authentic composite:
        Sa:  normalized source alpha.
        Da:  normalized canvas alpha.
    */
    Sa=QuantumScale*(double) GetPixelAlpha(row->source_image,p);
    Da=QuantumScale*(double) GetPixelAlpha(row->image,q);
    alpha=Sa+Da-Sa*Da;
    authentic.image=row->image;
    authentic.source_image=row->source_image;
    authentic.p=p;
    authentic.alpha=alpha;
    authentic.Da=Da;
    authentic.Sa=Sa;
    authentic.clamp=row->clamp;
    CompositeOverPixelChannels(&authentic,q);
    p=AdvanceOverSourcePixel(row->source_image,row->pixels,p);
    q+=(ptrdiff_t) GetPixelChannels(row->image);
  }
}

static void SetCompositeProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _OverSourceRow
{
  CacheView
    *source_view;

  const Image
    *source_image;

  MagickBooleanType
    clip_to_self;

  ssize_t
    x_offset,
    y_offset,
    y;

  ExceptionInfo
    *exception;
} OverSourceRow;

static MagickBooleanType SkipOverRow(const OverSourceRow *source,
  const MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  if (source->clip_to_self != MagickFalse)
    {
      if (source->y < source->y_offset)
        return(MagickFalse);
      if ((source->y-source->y_offset) >= (ssize_t) source->source_image->rows)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType FetchOverSourceRow(const OverSourceRow *source,
  const Quantum **pixels,const Quantum *magick_restrict *p,
  MagickBooleanType *status)
{
  /*
    If pixels is NULL, y is outside overlay region.
  */
  (*pixels)=(Quantum *) NULL;
  (*p)=(Quantum *) NULL;
  if ((source->y >= source->y_offset) &&
      ((source->y-source->y_offset) < (ssize_t) source->source_image->rows))
    {
      (*p)=GetCacheViewVirtualPixels(source->source_view,0,
        CastDoubleToSsizeT((double) source->y-source->y_offset),
        source->source_image->columns,1,source->exception);
      if ((*p) == (const Quantum *) NULL)
        {
          (*status)=MagickFalse;
          return(MagickFalse);
        }
      (*pixels)=(*p);
      if (source->x_offset < 0)
        (*p)-=(ptrdiff_t) CastDoubleToSsizeT((double) source->x_offset*
          GetPixelChannels(source->source_image));
    }
  return(MagickTrue);
}

static MagickBooleanType GetCompositeClamp(const Image *image)
{
  const char
    *value;

  MagickBooleanType
    clamp;

  clamp=MagickTrue;
  value=GetImageArtifact(image,"compose:clamp");
  if (value != (const char *) NULL)
    clamp=IsStringTrue(value);
  return(clamp);
}

typedef struct _OverRowHead
{
  OverSourceRow
    source;

  Image
    *image;

  CacheView
    *image_view;

  MagickBooleanType
    clamp;
} OverRowHead;

static MagickBooleanType CompositeOverRowHead(const OverRowHead *head,
  MagickBooleanType *status)
{
  const OverSourceRow
    source = head->source;

  Image
    *const image = head->image;

  CacheView
    *const image_view = head->image_view;

  const MagickBooleanType
    clamp = head->clamp;

  const Image
    *const source_image = source.source_image;

  const MagickBooleanType
    clip_to_self = source.clip_to_self;

  const ssize_t
    x_offset = source.x_offset,
    y_offset = source.y_offset,
    y = source.y;

  ExceptionInfo
    *const exception = source.exception;

  const Quantum
    *pixels;

  PixelInfo
    canvas_pixel,
    source_pixel;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  OverRowInputs
    row;

  if (SkipOverRow(&source,status) == MagickFalse)
    return(MagickFalse);
  if (FetchOverSourceRow(&source,&pixels,&p,status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  GetPixelInfo(image,&canvas_pixel);
  GetPixelInfo(source_image,&source_pixel);
  row.image=image;
  row.source_image=source_image;
  row.pixels=pixels;
  row.clip_to_self=clip_to_self;
  row.x_offset=x_offset;
  row.y_offset=y_offset;
  row.y=y;
  row.clamp=clamp;
  row.exception=exception;
  CompositeOverRow(&row,p,q);
  SyncCompositeRow(image_view,exception,status);
  return(MagickTrue);
}

static MagickBooleanType CompositeOverImage(Image *image,
  const Image *source_image,const CompositePlacement *placement,
  ExceptionInfo *exception)
{
#define CompositeImageTag  "Composite/Image"

  const MagickBooleanType
    clip_to_self = placement->clip_to_self;

  const ssize_t
    x_offset = placement->x_offset,
    y_offset = placement->y_offset;

  CacheView
    *image_view,
    *source_view;

  MagickBooleanType
    clamp,
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Composite image.
  */
  status=MagickTrue;
  progress=0;
  clamp=GetCompositeClamp(image);
  status=MagickTrue;
  progress=0;
  source_view=AcquireVirtualCacheView(source_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(source_image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const OverSourceRow
      source = { source_view, source_image, clip_to_self, x_offset, y_offset,
        y, exception };

    const OverRowHead
      head = { source, image, image_view, clamp };

    if (CompositeOverRowHead(&head,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetCompositeProgress(image,CompositeImageTag,progress,&status);
      }
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

static void ReportBlendResidual(const MagickBooleanType verbose,
  const ssize_t i,const double residual)
{
  if (verbose != MagickFalse)
    (void) FormatLocaleFile(stderr,"  %g: %g\n",(double) i,(double)
      residual);
}

typedef struct _BlendRelaxation
{
  Image
    *anchor_image,
    *divergent_image,
    **relax_image,
    **residual_image;

  Image
    **const *residual_failure,
    **const *kernel_failure;
} BlendRelaxation;

static void ReleaseBlendImages(Image **const *images)
{
  ssize_t
    k;

  for (k=0; images[k] != (Image **) NULL; k++)
    *images[k]=DestroyImage(*images[k]);
}

static void ReportBlendIteration(const MagickBooleanType verbose,
  const ssize_t i,const size_t tick,const double residual)
{
  if ((verbose != MagickFalse) && ((i % MagickMax(tick,1)) == 0))
    (void) FormatLocaleFile(stderr,"  %g: %g\n",(double) i,(double) residual);
}

static void ReportBlendTitle(const BlendRelaxation *relaxation,
  const MagickBooleanType verbose)
{
  if (verbose == MagickFalse)
    return;
  if (relaxation->divergent_image != (Image *) NULL)
    (void) FormatLocaleFile(stderr,"saliency blending:\n");
  else
    (void) FormatLocaleFile(stderr,"seamless blending:\n");
}

static Image *CloneBlendResidual(Image *residual_image,
  const Image *relax_image,ExceptionInfo *exception)
{
  if (residual_image != (Image *) NULL)
    residual_image=DestroyImage(residual_image);
  return(CloneImage(relax_image,0,0,MagickTrue,exception));
}

static MagickBooleanType RelaxBlendStep(const BlendRelaxation *relaxation,
  const KernelInfo *kernel_info,double *residual,ExceptionInfo *exception)
{
  Image
    *convolve_image,
    *sum_image;

  const BlendSumWeights difference = { 1.0, -1.0 };
  const CompositePlacement origin = { MagickTrue, 0, 0 };

  convolve_image=ConvolveImage(*relaxation->relax_image,kernel_info,exception);
  if (convolve_image == (Image *) NULL)
    return(MagickFalse);
  *relaxation->relax_image=DestroyImage(*relaxation->relax_image);
  *relaxation->relax_image=convolve_image;
  if (relaxation->divergent_image != (Image *) NULL)
    {
      sum_image=BlendSumImage(*relaxation->relax_image,
        relaxation->divergent_image,&difference,exception);
      if (sum_image == (Image *) NULL)
        return(MagickFalse);
      *relaxation->relax_image=DestroyImage(*relaxation->relax_image);
      *relaxation->relax_image=sum_image;
    }
  if (CompositeOverImage(*relaxation->relax_image,relaxation->anchor_image,
      &origin,exception) == MagickFalse)
    return(MagickFalse);
  return(BlendRMSEResidual(*relaxation->relax_image,
    *relaxation->residual_image,residual,exception));
}

static MagickBooleanType RelaxBlendImage(Image *image,
  const BlendRelaxation *relaxation,const BlendRequest *request,
  ExceptionInfo *exception)
{
  KernelInfo
    *kernel_info;

  MagickBooleanType
    verbose;

  ssize_t
    i;

  *relaxation->residual_image=CloneImage(*relaxation->relax_image,0,0,
    MagickTrue,exception);
  if (*relaxation->residual_image == (Image *) NULL)
    {
      ReleaseBlendImages(relaxation->residual_failure);
      return(MagickFalse);
    }
  /*
    Convolve relaxed image and blur area of interest.
  */
  kernel_info=AcquireKernelInfo("3x3:0,0.25,0,0.25,0,0.25,0,0.25,0",exception);
  if (kernel_info == (KernelInfo *) NULL)
    {
      ReleaseBlendImages(relaxation->kernel_failure);
      return(MagickFalse);
    }
  verbose=IsStringTrue(GetImageArtifact(image,"verbose"));
  ReportBlendTitle(relaxation,verbose);
  for (i=0; i < (ssize_t) request->iterations; i++)
  {
    double
      residual = 1.0;

    if (RelaxBlendStep(relaxation,kernel_info,&residual,
        exception) == MagickFalse)
      break;
    ReportBlendIteration(verbose,i,request->tick,residual);
    if (residual < request->residual_threshold)
      {
        ReportBlendResidual(verbose,i,residual);
        break;
      }
    *relaxation->residual_image=CloneBlendResidual(
      *relaxation->residual_image,*relaxation->relax_image,exception);
    if (*relaxation->residual_image == (Image *) NULL)
      break;
  }
  kernel_info=DestroyKernelInfo(kernel_info);
  return(MagickTrue);
}

static void ReleaseSaliencyImages(Image *crop_image,Image *divergent_image,
  Image *residual_image)
{
  crop_image=DestroyImage(crop_image);
  divergent_image=DestroyImage(divergent_image);
  residual_image=DestroyImage(residual_image);
}

static MagickBooleanType SaliencyBlendImage(Image *image,
  const Image *source_image,const BlendRequest *request,
  ExceptionInfo *exception)
{
  Image
    *crop_image,
    *divergent_image,
    *relax_image,
    *residual_image = (Image *) NULL;

  MagickBooleanType
    status = MagickTrue;

  RectangleInfo
    crop_info = {
      source_image->columns,
      source_image->rows,
      request->x_offset,
      request->y_offset
    };

  const CompositePlacement placement = { MagickTrue, request->x_offset, request->y_offset };

  Image
    **const residual_failure[] = { &crop_image, &relax_image, NULL },
    **const kernel_failure[] = { &crop_image, &residual_image, &relax_image,
      NULL };

  /*
    Saliency blend composite operator.
  */
  crop_image=CropImage(image,&crop_info,exception);
  if (crop_image == (Image *) NULL)
    return(MagickFalse);
  DisableCompositeClampUnlessSpecified(crop_image);
  divergent_image=BlendDivergentImage(crop_image,source_image,exception);
  if (divergent_image == (Image *) NULL)
    {
      crop_image=DestroyImage(crop_image);
      return(MagickFalse);
    }
  (void) ResetImagePage(crop_image,"0x0+0+0");
  relax_image=BlendMeanImage(crop_image,source_image,exception);
  if (relax_image == (Image *) NULL)
    {
      crop_image=DestroyImage(crop_image);
      divergent_image=DestroyImage(divergent_image);
      return(MagickFalse);
    }
  status=BlendMaskAlphaChannel(crop_image,source_image,exception);
  if (status == MagickFalse)
    {
      crop_image=DestroyImage(crop_image);
      divergent_image=DestroyImage(divergent_image);
      return(MagickFalse);
    }
  {
    const BlendRelaxation
      relaxation = { crop_image, divergent_image, &relax_image,
        &residual_image, residual_failure, kernel_failure };

    if (RelaxBlendImage(image,&relaxation,request,exception) == MagickFalse)
      return(MagickFalse);
  }
  ReleaseSaliencyImages(crop_image,divergent_image,residual_image);
  /*
    Composite relaxed over the background image.
  */
  status=CompositeOverImage(image,relax_image,&placement,exception);
  relax_image=DestroyImage(relax_image);
  return(status);
}

static Image *BlendSeamlessMeanImage(const Image *image,
  const Image *source_image,const BlendRequest *request,
  ExceptionInfo *exception)
{
  Image
    *crop_image,
    *mean_image,
    *sum_image;

  RectangleInfo
    crop_info = {
      source_image->columns,
      source_image->rows,
      request->x_offset,
      request->y_offset
    };

  const BlendSumWeights difference = { 1.0, -1.0 };

  crop_image=CropImage(image,&crop_info,exception);
  if (crop_image == (Image *) NULL)
    return((Image *) NULL);
  DisableCompositeClampUnlessSpecified(crop_image);
  (void) ResetImagePage(crop_image,"0x0+0+0");
  sum_image=BlendSumImage(crop_image,source_image,&difference,exception);
  crop_image=DestroyImage(crop_image);
  if (sum_image == (Image *) NULL)
    return((Image *) NULL);
  mean_image=BlendMeanImage(sum_image,source_image,exception);
  sum_image=DestroyImage(sum_image);
  return(mean_image);
}

static void ReleaseSeamlessImages(Image *mean_image,Image *residual_image)
{
  mean_image=DestroyImage(mean_image);
  residual_image=DestroyImage(residual_image);
}

static MagickBooleanType SeamlessBlendImage(Image *image,
  const Image *source_image,const BlendRequest *request,
  ExceptionInfo *exception)
{
  Image
    *foreground_image,
    *mean_image,
    *relax_image,
    *residual_image;

  MagickBooleanType
    status = MagickTrue;

  const BlendSumWeights sum = { 1.0, 1.0 };
  const CompositePlacement placement = { MagickTrue, request->x_offset, request->y_offset };

  Image
    **const residual_failure[] = { &relax_image, &mean_image, NULL },
    **const kernel_failure[] = { &residual_image, &relax_image, &mean_image,
      NULL };

  /*
    Seamless blend composite operator.
  */
  mean_image=BlendSeamlessMeanImage(image,source_image,request,exception);
  if (mean_image == (Image *) NULL)
    return(MagickFalse);
  relax_image=CloneImage(mean_image,0,0,MagickTrue,exception);
  if (relax_image == (Image *) NULL)
    {
      mean_image=DestroyImage(mean_image);
      return(MagickFalse);
    }
  status=BlendMaskAlphaChannel(mean_image,source_image,exception);
  if (status == MagickFalse)
    {
      relax_image=DestroyImage(relax_image);
      mean_image=DestroyImage(mean_image);
      return(MagickFalse);
    }
  {
    const BlendRelaxation
      relaxation = { mean_image, (Image *) NULL, &relax_image,
        &residual_image, residual_failure, kernel_failure };

    if (RelaxBlendImage(image,&relaxation,request,exception) == MagickFalse)
      return(MagickFalse);
  }
  ReleaseSeamlessImages(mean_image,residual_image);
  /*
    Composite the foreground image over the background image.
  */
  foreground_image=BlendSumImage(source_image,relax_image,&sum,exception);
  relax_image=DestroyImage(relax_image);
  if (foreground_image == (Image *) NULL)
    return(MagickFalse);
  (void) SetImageMask(foreground_image,ReadPixelMask,(const Image *) NULL,
    exception);
  status=CompositeOverImage(image,foreground_image,&placement,exception);
  foreground_image=DestroyImage(foreground_image);
  return(status);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
