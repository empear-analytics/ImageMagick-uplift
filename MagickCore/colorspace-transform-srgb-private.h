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

  MagickCore private methods of colorspace-transform-image.c: the
  sRGB-to-other direction of TransformImageColorspace, from the
  TransformPacket typedef to sRGBTransformImage; included by that file only.
*/
#ifndef MAGICKCORE_COLORSPACE_TRANSFORM_SRGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_TRANSFORM_SRGB_PRIVATE_H

/*
  Typedef declarations.
*/
typedef struct _TransformPacket
{
  MagickRealType
    x,
    y,
    z;
} TransformPacket;

/*
  Define declarations.
*/
#define MaximumLogarithmicColorspace  1024.0

/*
  Forward declarations.
*/
static MagickBooleanType
  TransformsRGBImage(Image *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     s R G B T r a n s f o r m I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  sRGBTransformImage() converts the reference image from sRGB to an alternate
%  colorspace.  The transformation matrices are not the standard ones: the
%  weights are rescaled to normalized the range of the transformed values to
%  be [0..QuantumRange].
%
%  The format of the sRGBTransformImage method is:
%
%      MagickBooleanType sRGBTransformImage(Image *image,
%        const ColorspaceType colorspace,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o colorspace: the colorspace to transform the image to.
%
%   o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsTransformMapMissing(
  const TransformPacket *x_map,const TransformPacket *y_map,
  const TransformPacket *z_map)
{
  return(((x_map == (TransformPacket *) NULL) ||
    (y_map == (TransformPacket *) NULL) ||
    (z_map == (TransformPacket *) NULL)) ? MagickTrue : MagickFalse);
}

static void InitializeYCCGammaMaps(ssize_t i,TransformPacket *x_map,
  TransformPacket *y_map,TransformPacket *z_map)
{
  for ( ; i <= (ssize_t) MaxMap; i++)
  {
    x_map[i].x=0.298839*(1.099*i-0.099);
    x_map[i].y=(-0.298839)*(1.099*i-0.099);
    x_map[i].z=0.70100*(1.099*i-0.099);
    y_map[i].x=0.586811*(1.099*i-0.099);
    y_map[i].y=(-0.586811)*(1.099*i-0.099);
    y_map[i].z=(-0.586811)*(1.099*i-0.099);
    z_map[i].x=0.114350*(1.099*i-0.099);
    z_map[i].y=0.88600*(1.099*i-0.099);
    z_map[i].z=(-0.114350)*(1.099*i-0.099);
  }
}

static void InitializeYCCTransformMaps(TransformPacket *x_map,
  TransformPacket *y_map,TransformPacket *z_map)
{
  ssize_t
    i;

  for (i=0; i <= (ssize_t) (0.018*MaxMap); i++)
  {
    x_map[i].x=0.005382*i;
    x_map[i].y=(-0.003296)*i;
    x_map[i].z=0.009410*i;
    y_map[i].x=0.010566*i;
    y_map[i].y=(-0.006471)*i;
    y_map[i].z=(-0.007880)*i;
    z_map[i].x=0.002052*i;
    z_map[i].y=0.009768*i;
    z_map[i].z=(-0.001530)*i;
  }
  InitializeYCCGammaMaps(i,x_map,y_map,z_map);
}

static TransformPacket *RelinquishTransformMap(TransformPacket *map)
{
  if (map != (TransformPacket *) NULL)
    map=(TransformPacket *) RelinquishMagickMemory(map);
  return(map);
}

static void ParseIlluminantArtifact(const char **artifact,
  IlluminantType *illuminant)
{
  ssize_t
    illuminant_type;

  illuminant_type=ParseCommandOption(MagickIlluminantOptions,MagickFalse,
    *artifact);
  if (illuminant_type < 0)
    *illuminant=UndefinedIlluminant;
  else
    *illuminant=(IlluminantType) illuminant_type;
}

static void GetIlluminantArtifact(const Image *image,const char **artifact,
  IlluminantType *illuminant)
{
  *artifact=GetImageArtifact(image,"color:illuminant");
  if (*artifact != (const char *) NULL)
    ParseIlluminantArtifact(artifact,illuminant);
}

static MagickBooleanType SyncPseudoClassImage(Image *image,
  ExceptionInfo *exception)
{
  if (SyncImage(image,exception) == MagickFalse)
    return(MagickFalse);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType SyncImageToDirectClass(Image *image,
  ExceptionInfo *exception)
{
  if (image->storage_class == PseudoClass)
    {
      if (SyncPseudoClassImage(image,exception) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _ColorspaceRowInfo
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} ColorspaceRowInfo;

static MagickBooleanType FetchColorspaceRow(const ColorspaceRowInfo *row,
  const ssize_t y,Quantum *magick_restrict *q,MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*q)=GetCacheViewAuthenticPixels(row->image_view,0,y,row->image->columns,1,
    row->exception);
  if ((*q) == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType FinishColorspace(const ColorspaceRowInfo *finish,
  const ColorspaceType colorspace,const MagickBooleanType status)
{
  (void) DestroyCacheView(finish->image_view);
  if (SetImageColorspace(finish->image,colorspace,finish->exception) ==
      MagickFalse)
    return(MagickFalse);
  return(status);
}

static void SetColorspaceProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static void SyncColorspaceRow(CacheView *image_view,
  ExceptionInfo *exception,MagickBooleanType *status)
{
  MagickBooleanType
    sync;

  sync=SyncCacheViewAuthenticPixels(image_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

static void TransformLinearGrayRowFromsRGB(MagickBooleanType *status,
  const ColorspaceRowInfo *row,const ssize_t y)
{
  Image
    *const image = row->image;

  CacheView
    *const image_view = row->image_view;

  ExceptionInfo
    *const exception = row->exception;

  MagickBooleanType
    sync;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    MagickRealType
      gray;

    gray=0.212656*DecodePixelGamma(GetPixelRed(image,q))+0.715158*
      DecodePixelGamma(GetPixelGreen(image,q))+0.072186*
      DecodePixelGamma(GetPixelBlue(image,q));
    SetPixelGray(image,ClampToQuantum(gray),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  sync=SyncCacheViewAuthenticPixels(image_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

static void SetCMYKImageType(Image *image)
{
  image->type=image->alpha_trait == UndefinedPixelTrait ?
    ColorSeparationType : ColorSeparationAlphaType;
}

static MagickBooleanType sRGBToCMYK(Image *image,const ColorspaceType colorspace,
  MagickBooleanType status,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ssize_t
    y;

  PixelInfo
    zero;

  /*
    Convert RGB to CMYK colorspace.
  */
  if (SyncImageToDirectClass(image,exception) == MagickFalse)
    return(MagickFalse);
  if (SetImageColorspace(image,colorspace,exception) == MagickFalse)
    return(MagickFalse);
  GetPixelInfo(image,&zero);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    PixelInfo
      pixel;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    const ColorspaceRowInfo
      row = { image,image_view,exception };

    if (FetchColorspaceRow(&row,y,&q,&status) == MagickFalse)
      continue;
    pixel=zero;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      ConvertRGBToCMYK(&pixel);
      SetPixelViaPixelInfo(image,&pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    SyncColorspaceRow(image_view,exception,&status);
  }
  image_view=DestroyCacheView(image_view);
  SetCMYKImageType(image);
  if (SetImageColorspace(image,colorspace,exception) == MagickFalse)
    return(MagickFalse);
  return(status);
}

static MagickBooleanType sRGBToLinearGray(Image *image,const ColorspaceType colorspace,
  MagickBooleanType status,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ssize_t
    y;

  /*
    Transform image from sRGB to GRAY.
  */
  if (SyncImageToDirectClass(image,exception) == MagickFalse)
    return(MagickFalse);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ColorspaceRowInfo
      row;

    row.image=image;
    row.image_view=image_view;
    row.exception=exception;
    TransformLinearGrayRowFromsRGB(&status,&row,y);
  }
  image_view=DestroyCacheView(image_view);
  if (SetImageColorspace(image,colorspace,exception) == MagickFalse)
    return(MagickFalse);
  image->type=GrayscaleType;
  return(status);
}

static MagickBooleanType sRGBToGray(Image *image,const ColorspaceType colorspace,
  MagickBooleanType status,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ssize_t
    y;

  /*
    Transform image from sRGB to GRAY.
  */
  if (SyncImageToDirectClass(image,exception) == MagickFalse)
    return(MagickFalse);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    const ColorspaceRowInfo
      row = { image,image_view,exception };

    if (FetchColorspaceRow(&row,y,&q,&status) == MagickFalse)
      continue;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      MagickRealType
        gray;

      gray=0.212656*(double) GetPixelRed(image,q)+0.715158*(double)
        GetPixelGreen(image,q)+0.072186*(double) GetPixelBlue(image,q);
      SetPixelGray(image,ClampToQuantum(gray),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    SyncColorspaceRow(image_view,exception,&status);
  }
  image_view=DestroyCacheView(image_view);
  if (SetImageColorspace(image,colorspace,exception) == MagickFalse)
    return(MagickFalse);
  image->type=GrayscaleType;
  return(status);
}

typedef struct _sRGBGenericInput
{
  ColorspaceType
    colorspace;

  IlluminantType
    illuminant;
} sRGBGenericInput;

static void GetWhiteLuminance(Image *image,ExceptionInfo *exception,
  double *white_luminance)
{
  const char
    *value;

  value=GetImageProperty(image,"white-luminance",exception);
  if (value != (const char *) NULL)
    *white_luminance=StringToDouble(value,(char **) NULL);
}

static MagickBooleanType sRGBToGeneric(const sRGBGenericInput *input,
  Image *image,MagickBooleanType status,ExceptionInfo *exception)
{
  const ColorspaceType
    colorspace = input->colorspace;

  const IlluminantType
    illuminant = input->illuminant;

  CacheView
    *image_view;

  ssize_t
    y;

  ColorspaceRowInfo
    finish;

  double
    white_luminance = 10000.0;

  /*
    Transform image from sRGB to target colorspace.
  */
  GetWhiteLuminance(image,exception,&white_luminance);
  if (SyncImageToDirectClass(image,exception) == MagickFalse)
    return(MagickFalse);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    const ColorspaceRowInfo
      row = { image,image_view,exception };

    if (FetchColorspaceRow(&row,y,&q,&status) == MagickFalse)
      continue;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        X,
        Y,
        Z;

      ConvertRGBToGeneric(colorspace,(double) GetPixelRed(image,q),
        (double) GetPixelGreen(image,q),(double) GetPixelBlue(image,q),
        white_luminance,illuminant,&X,&Y,&Z);
      SetPixelRed(image,ClampToQuantum((double) QuantumRange*X),q);
      SetPixelGreen(image,ClampToQuantum((double) QuantumRange*Y),q);
      SetPixelBlue(image,ClampToQuantum((double) QuantumRange*Z),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    SyncColorspaceRow(image_view,exception,&status);
  }
  finish.image=image;
  finish.image_view=image_view;
  finish.exception=exception;
  return(FinishColorspace(&finish,colorspace,status));
}

static MagickBooleanType sRGBToLinearRGB(Image *image,const ColorspaceType colorspace,
  MagickBooleanType status,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ssize_t
    y;

  ColorspaceRowInfo
    finish;

  /*
    Transform image from sRGB to linear RGB.
  */
  if (SyncImageToDirectClass(image,exception) == MagickFalse)
    return(MagickFalse);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    const ColorspaceRowInfo
      row = { image,image_view,exception };

    if (FetchColorspaceRow(&row,y,&q,&status) == MagickFalse)
      continue;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        blue,
        green,
        red;

      red=DecodePixelGamma((MagickRealType) GetPixelRed(image,q));
      green=DecodePixelGamma((MagickRealType) GetPixelGreen(image,q));
      blue=DecodePixelGamma((MagickRealType) GetPixelBlue(image,q));
      SetPixelRed(image,ClampToQuantum(red),q);
      SetPixelGreen(image,ClampToQuantum(green),q);
      SetPixelBlue(image,ClampToQuantum(blue),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    SyncColorspaceRow(image_view,exception,&status);
  }
  finish.image=image;
  finish.image_view=image_view;
  finish.exception=exception;
  return(FinishColorspace(&finish,colorspace,status));
}

typedef struct _LogProperties
{
  double
    *density,
    *gamma,
    *film_gamma,
    *reference_black,
    *reference_white;
} LogProperties;

static void GetLogProperties(const Image *image,ExceptionInfo *exception,
  const LogProperties *targets)
{
#define DisplayGamma  (1.0/1.7)
#define FilmGamma  0.6
#define ReferenceBlack  95.0
#define ReferenceWhite  685.0

  const char
    *value;

  double
    *const density = targets->density,
    *const gamma = targets->gamma,
    *const film_gamma = targets->film_gamma,
    *const reference_black = targets->reference_black,
    *const reference_white = targets->reference_white;

  (*density)=DisplayGamma;
  (*gamma)=DisplayGamma;
  value=GetImageProperty(image,"gamma",exception);
  if (value != (const char *) NULL)
    (*gamma)=MagickSafeReciprocal(StringToDouble(value,(char **) NULL));
  (*film_gamma)=FilmGamma;
  value=GetImageProperty(image,"film-gamma",exception);
  if (value != (const char *) NULL)
    (*film_gamma)=StringToDouble(value,(char **) NULL);
  (*reference_black)=ReferenceBlack;
  value=GetImageProperty(image,"reference-black",exception);
  if (value != (const char *) NULL)
    (*reference_black)=StringToDouble(value,(char **) NULL);
  (*reference_white)=ReferenceWhite;
  value=GetImageProperty(image,"reference-white",exception);
  if (value != (const char *) NULL)
    (*reference_white)=StringToDouble(value,(char **) NULL);
}

typedef struct _sRGBTransformTables
{
  Image
    *image;

  TransformPacket
    *x_map,
    *y_map,
    *z_map;

  PrimaryInfo
    primary_info;

  ExceptionInfo
    *exception;
} sRGBTransformTables;

static void sRGBTransformColormapEntry(const sRGBTransformTables *tables,
  const ssize_t i)
{
  const PrimaryInfo
    primary_info = tables->primary_info;

  Image
    *const image = tables->image;

  TransformPacket
    *const x_map = tables->x_map,
    *const y_map = tables->y_map,
    *const z_map = tables->z_map;

  unsigned int
    blue,
    green,
    red;

  PixelInfo
    pixel;

  red=ScaleQuantumToMap(ClampToQuantum(image->colormap[i].red));
  green=ScaleQuantumToMap(ClampToQuantum(image->colormap[i].green));
  blue=ScaleQuantumToMap(ClampToQuantum(image->colormap[i].blue));
  pixel.red=x_map[red].x+y_map[green].x+z_map[blue].x+primary_info.x;
  pixel.green=x_map[red].y+y_map[green].y+z_map[blue].y+primary_info.y;
  pixel.blue=x_map[red].z+y_map[green].z+z_map[blue].z+primary_info.z;
  image->colormap[i].red=(double) ScaleMapToQuantum(pixel.red);
  image->colormap[i].green=(double) ScaleMapToQuantum(pixel.green);
  image->colormap[i].blue=(double) ScaleMapToQuantum(pixel.blue);
}

static void sRGBTransformColormap(const sRGBTransformTables *tables)
{
  ExceptionInfo
    *const exception = tables->exception;

  Image
    *const image = tables->image;

  ssize_t
    i;

  /*
    Convert PseudoClass image.
  */
  for (i=0; i < (ssize_t) image->colors; i++)
    sRGBTransformColormapEntry(tables,i);
  (void) SyncImage(image,exception);
}

static void sRGBToLogPixels(Image *image,Quantum *logmap,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=(ssize_t) image->columns; x != 0; x--)
  {
    double
      blue,
      green,
      red;

    red=(double) DecodePixelGamma((MagickRealType)
      GetPixelRed(image,q));
    green=(double) DecodePixelGamma((MagickRealType)
      GetPixelGreen(image,q));
    blue=(double) DecodePixelGamma((MagickRealType)
      GetPixelBlue(image,q));
    SetPixelRed(image,logmap[ScaleQuantumToMap(ClampToQuantum(red))],q);
    SetPixelGreen(image,logmap[ScaleQuantumToMap(ClampToQuantum(green))],
      q);
    SetPixelBlue(image,logmap[ScaleQuantumToMap(ClampToQuantum(blue))],q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType sRGBToLog(Image *image,const ColorspaceType colorspace,
  MagickBooleanType status,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ssize_t
    i,
    y;

  double
    black,
    density,
    film_gamma,
    gamma,
    reference_black,
    reference_white;

  const LogProperties
    targets = { &density,&gamma,&film_gamma,&reference_black,
      &reference_white };

  Quantum
    *logmap;

  /*
    Transform RGB to Log colorspace.
  */
  GetLogProperties(image,exception,&targets);
  logmap=(Quantum *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(*logmap));
  if (logmap == (Quantum *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  black=pow(10.0,(reference_black-reference_white)*(gamma/density)*0.002*
    MagickSafeReciprocal(film_gamma));
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static)
#endif
  for (i=0; i <= (ssize_t) MaxMap; i++)
    logmap[i]=ScaleMapToQuantum(((double) MaxMap*(reference_white+
      log10(black+(1.0*i/MaxMap)*(1.0-black))/((gamma/density)*0.002*
      MagickSafeReciprocal(film_gamma)))/MaximumLogarithmicColorspace));
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    const ColorspaceRowInfo
      row = { image,image_view,exception };

    if (FetchColorspaceRow(&row,y,&q,&status) == MagickFalse)
      continue;
    sRGBToLogPixels(image,logmap,q);
    SyncColorspaceRow(image_view,exception,&status);
  }
  image_view=DestroyCacheView(image_view);
  logmap=(Quantum *) RelinquishMagickMemory(logmap);
  if (SetImageColorspace(image,colorspace,exception) == MagickFalse)
    return(MagickFalse);
  return(status);
}

typedef struct _sRGBTransformRowInfo
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;

  TransformPacket
    *x_map,
    *y_map,
    *z_map;

  PrimaryInfo
    primary_info;
} sRGBTransformRowInfo;

static MagickBooleanType sRGBTransformRow(const sRGBTransformRowInfo *info,
  const ssize_t y,MagickBooleanType *status)
{
  Image
    *const image = info->image;

  CacheView
    *const image_view = info->image_view;

  ExceptionInfo
    *const exception = info->exception;

  TransformPacket
    *const x_map = info->x_map,
    *const y_map = info->y_map,
    *const z_map = info->z_map;

  const PrimaryInfo
    primary_info = info->primary_info;

  PixelInfo
    pixel;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  unsigned int
    blue,
    green,
    red;

  const ColorspaceRowInfo
    row = { image,image_view,exception };

  if (FetchColorspaceRow(&row,y,&q,&(*status)) == MagickFalse)
    return(MagickFalse);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    red=ScaleQuantumToMap(ClampToQuantum((MagickRealType)
      GetPixelRed(image,q)));
    green=ScaleQuantumToMap(ClampToQuantum((MagickRealType)
      GetPixelGreen(image,q)));
    blue=ScaleQuantumToMap(ClampToQuantum((MagickRealType)
      GetPixelBlue(image,q)));
    pixel.red=(x_map[red].x+y_map[green].x+z_map[blue].x)+
      primary_info.x;
    pixel.green=(x_map[red].y+y_map[green].y+z_map[blue].y)+
      primary_info.y;
    pixel.blue=(x_map[red].z+y_map[green].z+z_map[blue].z)+
      primary_info.z;
    SetPixelRed(image,ScaleMapToQuantum(pixel.red),q);
    SetPixelGreen(image,ScaleMapToQuantum(pixel.green),q);
    SetPixelBlue(image,ScaleMapToQuantum(pixel.blue),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  SyncColorspaceRow(image_view,exception,&(*status));
  return(MagickTrue);
}

static MagickBooleanType sRGBTransformPixels(
  const sRGBTransformTables *tables,MagickBooleanType status,
  MagickOffsetType progress)
{
#define sRGBTransformImageTag  "RGBTransform/Image"

  const PrimaryInfo
    primary_info = tables->primary_info;

  ExceptionInfo
    *const exception = tables->exception;

  Image
    *const image = tables->image;

  TransformPacket
    *const x_map = tables->x_map,
    *const y_map = tables->y_map,
    *const z_map = tables->z_map;

  CacheView
    *image_view;

  ssize_t
    y;

  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const sRGBTransformRowInfo
      info = { image,image_view,exception,x_map,y_map,z_map,
        primary_info };

    if (sRGBTransformRow(&info,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetColorspaceProgress(image,sRGBTransformImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _sRGBMapsInput
{
  ColorspaceType
    colorspace;

  TransformPacket
    *x_map,
    *y_map,
    *z_map;
} sRGBMapsInput;

static void InitializesRGBRec709ThroughIdentityMaps(const sRGBMapsInput *maps,
  PrimaryInfo *primary_info)
{
  const ColorspaceType
    colorspace = maps->colorspace;

  TransformPacket
    *const x_map = maps->x_map,
    *const y_map = maps->y_map,
    *const z_map = maps->z_map;

  ssize_t
    i;

  switch (colorspace)
  {
    case Rec709YCbCrColorspace:
    {
      /*
        Initialize YCbCr tables (ITU-R BT.709):

          Y =  0.212656*R+0.715158*G+0.072186*B
          Cb= -0.114572*R-0.385428*G+0.500000*B
          Cr=  0.500000*R-0.454153*G-0.045847*B

        Cb and Cr, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      (*primary_info).y=(double) ((MaxMap+1)/2);
      (*primary_info).z=(double) ((MaxMap+1)/2);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static)
#endif
      for (i=0; i <= (ssize_t) MaxMap; i++)
      {
        x_map[i].x=(MagickRealType) (0.212656*(double) i);
        x_map[i].y=(MagickRealType) (-0.114572*(double) i);
        x_map[i].z=(MagickRealType) (0.500000*(double) i);
        y_map[i].x=(MagickRealType) (0.715158*(double) i);
        y_map[i].y=(MagickRealType) (-0.385428*(double) i);
        y_map[i].z=(MagickRealType) (-0.454153*(double) i);
        z_map[i].x=(MagickRealType) (0.072186*(double) i);
        z_map[i].y=(MagickRealType) (0.500000*(double) i);
        z_map[i].z=(MagickRealType) (-0.045847*(double) i);
      }
      break;
    }
    case YCCColorspace:
    {
      /*
        Initialize YCC tables:

          Y =  0.298839*R+0.586811*G+0.114350*B
          C1= -0.298839*R-0.586811*G+0.88600*B
          C2=  0.70100*R-0.586811*G-0.114350*B

        YCC is scaled by 1.3584.  C1 zero is 156 and C2 is at 137.
      */
      (*primary_info).y=(MagickRealType) ScaleQuantumToMap(
        ScaleCharToQuantum(156));
      (*primary_info).z=(MagickRealType) ScaleQuantumToMap(
        ScaleCharToQuantum(137));
      InitializeYCCTransformMaps(x_map,y_map,z_map);
      break;
    }
    default:
    {
      /*
        Linear conversion tables.
      */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static)
#endif
      for (i=0; i <= (ssize_t) MaxMap; i++)
      {
        x_map[i].x=(MagickRealType) (1.0*(double) i);
        x_map[i].y=(MagickRealType) 0.0;
        x_map[i].z=(MagickRealType) 0.0;
        y_map[i].x=(MagickRealType) 0.0;
        y_map[i].y=(MagickRealType) (1.0*(double) i);
        y_map[i].z=(MagickRealType) 0.0;
        z_map[i].x=(MagickRealType) 0.0;
        z_map[i].y=(MagickRealType) 0.0;
        z_map[i].z=(MagickRealType) (1.0*(double) i);
      }
      break;
    }
  }
}

static PrimaryInfo InitializesRGBTransformMaps(
  const ColorspaceType colorspace,TransformPacket *x_map,
  TransformPacket *y_map,TransformPacket *z_map)
{
  PrimaryInfo
    primary_info;

  ssize_t
    i;

  (void) memset(&primary_info,0,sizeof(primary_info));
  switch (colorspace)
  {
    case OHTAColorspace:
    {
      /*
        Initialize OHTA tables:

          I1 = 0.33333*R+0.33334*G+0.33333*B
          I2 = 0.50000*R+0.00000*G-0.50000*B
          I3 =-0.25000*R+0.50000*G-0.25000*B

        I and Q, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(MagickRealType) ((MaxMap+1)/2);
      primary_info.z=(MagickRealType) ((MaxMap+1)/2);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static)
#endif
      for (i=0; i <= (ssize_t) MaxMap; i++)
      {
        x_map[i].x=(MagickRealType) (0.33333*(double) i);
        x_map[i].y=(MagickRealType) (0.50000*(double) i);
        x_map[i].z=(MagickRealType) (-0.25000*(double) i);
        y_map[i].x=(MagickRealType) (0.33334*(double) i);
        y_map[i].y=(MagickRealType) (0.00000*(double) i);
        y_map[i].z=(MagickRealType) (0.50000*(double) i);
        z_map[i].x=(MagickRealType) (0.33333*(double) i);
        z_map[i].y=(MagickRealType) (-0.50000*(double) i);
        z_map[i].z=(MagickRealType) (-0.25000*(double) i);
      }
      break;
    }
    case Rec601YCbCrColorspace:
    {
      /*
        Initialize YCbCr tables (ITU-R BT.601):

          Y =  0.2988390*R+0.5868110*G+0.1143500*B
          Cb= -0.1687367*R-0.3312640*G+0.5000000*B
          Cr=  0.5000000*R-0.4186880*G-0.0813120*B

        Cb and Cr, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(MagickRealType) ((MaxMap+1)/2);
      primary_info.z=(MagickRealType) ((MaxMap+1)/2);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static)
#endif
      for (i=0; i <= (ssize_t) MaxMap; i++)
      {
        x_map[i].x=(MagickRealType) (0.298839*(double) i);
        x_map[i].y=(MagickRealType) (-0.1687367*(double) i);
        x_map[i].z=(MagickRealType) (0.500000*(double) i);
        y_map[i].x=(MagickRealType) (0.586811*(double) i);
        y_map[i].y=(MagickRealType) (-0.331264*(double) i);
        y_map[i].z=(MagickRealType) (-0.418688*(double) i);
        z_map[i].x=(MagickRealType) (0.114350*(double) i);
        z_map[i].y=(MagickRealType) (0.500000*(double) i);
        z_map[i].z=(MagickRealType) (-0.081312*(double) i);
      }
      break;
    }
    default:
    {
      sRGBMapsInput
        maps;

      maps.colorspace=colorspace;
      maps.x_map=x_map;
      maps.y_map=y_map;
      maps.z_map=z_map;
      InitializesRGBRec709ThroughIdentityMaps(&maps,&primary_info);
      break;
    }
  }
  return(primary_info);
}

typedef struct _TransformMapTargets
{
  TransformPacket
    **x_map,
    **y_map,
    **z_map;
} TransformMapTargets;

static MagickBooleanType AcquiresRGBTransformMaps(Image *image,
  ExceptionInfo *exception,const TransformMapTargets *targets)
{
  TransformPacket
    **const x_map = targets->x_map,
    **const y_map = targets->y_map,
    **const z_map = targets->z_map;

  *x_map=(TransformPacket *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(**x_map));
  *y_map=(TransformPacket *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(**y_map));
  *z_map=(TransformPacket *) AcquireQuantumMemory((size_t) MaxMap+1UL,
    sizeof(**z_map));
  if (IsTransformMapMissing(*x_map,*y_map,*z_map) != MagickFalse)
    {
      *x_map=RelinquishTransformMap(*x_map);
      *y_map=RelinquishTransformMap(*y_map);
      *z_map=RelinquishTransformMap(*z_map);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  return(MagickTrue);
}

typedef struct _sRGBTableInput
{
  ColorspaceType
    colorspace;

  MagickOffsetType
    progress;
} sRGBTableInput;

static MagickBooleanType sRGBTransformByTables(const sRGBTableInput *table,
  Image *image,MagickBooleanType status,ExceptionInfo *exception)
{
  const ColorspaceType
    colorspace = table->colorspace;

  const MagickOffsetType
    progress = table->progress;

  PrimaryInfo
    primary_info;

  TransformPacket
    *x_map,
    *y_map,
    *z_map;

  sRGBTransformTables
    tables;

  TransformMapTargets
    targets;

  /*
    Allocate the tables.
  */
  targets.x_map=(&x_map);
  targets.y_map=(&y_map);
  targets.z_map=(&z_map);
  if (AcquiresRGBTransformMaps(image,exception,&targets) == MagickFalse)
    return(MagickFalse);
  primary_info=InitializesRGBTransformMaps(colorspace,x_map,y_map,z_map);
  /*
    Convert from sRGB.
  */
  tables.image=image;
  tables.x_map=x_map;
  tables.y_map=y_map;
  tables.z_map=z_map;
  tables.primary_info=primary_info;
  tables.exception=exception;
  switch (image->storage_class)
  {
    case DirectClass:
    default:
    {
      /*
        Convert DirectClass image.
      */
      status=sRGBTransformPixels(&tables,status,progress);
      break;
    }
    case PseudoClass:
    {
      sRGBTransformColormap(&tables);
      break;
    }
  }
  /*
    Relinquish resources.
  */
  z_map=(TransformPacket *) RelinquishMagickMemory(z_map);
  y_map=(TransformPacket *) RelinquishMagickMemory(y_map);
  x_map=(TransformPacket *) RelinquishMagickMemory(x_map);
  if (SetImageColorspace(image,colorspace,exception) == MagickFalse)
    return(MagickFalse);
  return(status);
}

static MagickBooleanType sRGBTransformLogOrLinear(
  const sRGBTableInput *table,Image *image,MagickBooleanType status,
  ExceptionInfo *exception)
{
  const ColorspaceType
    colorspace = table->colorspace;

  switch (colorspace)
  {
    case LogColorspace:
      return(sRGBToLog(image,colorspace,status,exception));
    case RGBColorspace:
    case scRGBColorspace:
      return(sRGBToLinearRGB(image,colorspace,status,exception));
    default:
      break;
  }
  return(sRGBTransformByTables(table,image,status,exception));
}

static MagickBooleanType sRGBTransformCMYKOrGray(
  const sRGBTableInput *table,Image *image,MagickBooleanType status,
  ExceptionInfo *exception)
{
  const ColorspaceType
    colorspace = table->colorspace;

  switch (colorspace)
  {
    case CMYKColorspace:
      return(sRGBToCMYK(image,colorspace,status,exception));
    case LinearGRAYColorspace:
      return(sRGBToLinearGray(image,colorspace,status,exception));
    case GRAYColorspace:
      return(sRGBToGray(image,colorspace,status,exception));
    default:
      break;
  }
  return(sRGBTransformLogOrLinear(table,image,status,exception));
}

static const ColorspaceType
  GenericColorspaces[] =
  {
    CMYColorspace,
    Adobe98Colorspace,
    CAT02LMSColorspace,
    DisplayP3Colorspace,
    HCLColorspace,
    HCLpColorspace,
    HSBColorspace,
    HSIColorspace,
    HSLColorspace,
    HSVColorspace,
    HWBColorspace,
    JzazbzColorspace,
    LabColorspace,
    LCHColorspace,
    LCHabColorspace,
    LCHuvColorspace,
    LMSColorspace,
    LuvColorspace,
    OklabColorspace,
    OklchColorspace,
    ProPhotoColorspace,
    xyYColorspace,
    XYZColorspace,
    YCbCrColorspace,
    YDbDrColorspace,
    YIQColorspace,
    YPbPrColorspace,
    YUVColorspace
  };

static MagickBooleanType IsGenericColorspace(const ColorspaceType colorspace)
{
  size_t
    i;

  for (i=0; i < (sizeof(GenericColorspaces)/sizeof(*GenericColorspaces)); i++)
    if (GenericColorspaces[i] == colorspace)
      return(MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType sRGBTransformImage(Image *image,
  const ColorspaceType colorspace,ExceptionInfo *exception)
{
  const char
    *artifact;

  sRGBTableInput
    table;

  IlluminantType
    illuminant = D65Illuminant;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(colorspace != sRGBColorspace);
  assert(colorspace != TransparentColorspace);
  assert(colorspace != UndefinedColorspace);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  GetIlluminantArtifact(image,&artifact,&illuminant);
  status=MagickTrue;
  progress=0;
  if (IsGenericColorspace(colorspace) != MagickFalse)
    {
      sRGBGenericInput
        input;

      input.colorspace=colorspace;
      input.illuminant=illuminant;
      return(sRGBToGeneric(&input,image,status,exception));
    }
  table.colorspace=colorspace;
  table.progress=progress;
  return(sRGBTransformCMYKOrGray(&table,image,status,exception));
}

#endif
