/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               EEEEE  N   N  H   H   AAA   N   N   CCCC  EEEEE               %
%               E      NN  N  H   H  A   A  NN  N  C      E                   %
%               EEE    N N N  HHHHH  AAAAA  N N N  C      EEE                 %
%               E      N  NN  H   H  A   A  N  NN  C      E                   %
%               EEEEE  N   N  H   H  A   A  N   N   CCCC  EEEEE               %
%                                                                             %
%                       CCCC   OOO   L       OOO   RRRR                       %
%                      C      O   O  L      O   O  R   R                      %
%                      C      O   O  L      O   O  RRRR                       %
%                      C      O   O  L      O   O  R R                        %
%                       CCCC   OOO   LLLLL   OOO   R  R                       %
%                                                                             %
%                                                                             %
%                 MagickCore Image Color Enhancement Methods                  %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
  Typedef declarations.
*/
typedef struct _ModulateInfo
{
  ColorspaceType
    colorspace;

  double
    percent_hue,
    percent_saturation,
    percent_brightness;

  IlluminantType
    illuminant;
} ModulateInfo;

typedef struct _SigmoidalContrastRow
{
  const Image
    *image;

  MagickBooleanType
    sharpen;

  double
    contrast,
    midpoint;
} SigmoidalContrastRow;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A u t o G a m m a I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AutoGammaImage() extract the 'mean' from the image and adjust the image
%  to try make set its gamma appropriately.
%
%  The format of the AutoGammaImage method is:
%
%      MagickBooleanType AutoGammaImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-level
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType AutoGammaImage(Image *image,
  ExceptionInfo *exception)
{
  double
    gamma,
    log_mean,
    mean,
    sans;

  MagickStatusType
    status;

  ssize_t
    i;

  log_mean=log(0.5);
  if (image->channel_mask == AllChannels)
    {
      /*
        Apply gamma correction equally across all given channels.
      */
      (void) GetImageMean(image,&mean,&sans,exception);
      gamma=log(mean*QuantumScale)/log_mean;
      return(LevelImage(image,0.0,(double) QuantumRange,gamma,exception));
    }
  /*
    Auto-gamma each channel separately.
  */
  status=MagickTrue;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ChannelType
      channel_mask;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    channel_mask=SetImageChannelMask(image,(ChannelType) (1UL << i));
    status=GetImageMean(image,&mean,&sans,exception);
    gamma=log(mean*QuantumScale)/log_mean;
    status&=(MagickStatusType) LevelImage(image,0.0,(double) QuantumRange,gamma,
      exception);
    (void) SetImageChannelMask(image,channel_mask);
    if (status == MagickFalse)
      break;
  }
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n t r a s t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ContrastImage() enhances the intensity differences between the lighter and
%  darker elements of the image.  Set sharpen to a MagickTrue to increase the
%  image contrast otherwise the contrast is reduced.
%
%  The format of the ContrastImage method is:
%
%      MagickBooleanType ContrastImage(Image *image,
%        const MagickBooleanType sharpen,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o sharpen: Increase or decrease image contrast.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void Contrast(const int sign,double *red,double *green,
  double *blue)
{
  double
    brightness = 0.0,
    hue = 0.0,
    saturation = 0.0;

  /*
    Enhance contrast: dark color become darker, light color become lighter.
  */
  ConvertRGBToHSB(*red,*green,*blue,&hue,&saturation,&brightness);
  brightness+=0.5*sign*(0.5*(sin((double) (MagickPI*(brightness-0.5)))+1.0)-
    brightness);
  if (brightness > 1.0)
    brightness=1.0;
  else
    if (brightness < 0.0)
      brightness=0.0;
  ConvertHSBToRGB(hue,saturation,brightness,red,green,blue);
}

static void ContrastImageRow(const Image *image,const int sign,Quantum *q)
{
  double
    blue,
    green,
    red;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    red=(double) GetPixelRed(image,q);
    green=(double) GetPixelGreen(image,q);
    blue=(double) GetPixelBlue(image,q);
    Contrast(sign,&red,&green,&blue);
    SetPixelRed(image,ClampToQuantum(red),q);
    SetPixelGreen(image,ClampToQuantum(green),q);
    SetPixelBlue(image,ClampToQuantum(blue),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ContrastColormap(Image *image,const int sign)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    double
      blue,
      green,
      red;

    red=(double) image->colormap[i].red;
    green=(double) image->colormap[i].green;
    blue=(double) image->colormap[i].blue;
    Contrast(sign,&red,&green,&blue);
    image->colormap[i].red=(MagickRealType) red;
    image->colormap[i].green=(MagickRealType) green;
    image->colormap[i].blue=(MagickRealType) blue;
  }
}

typedef struct _EnhanceLine
{
  const Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} EnhanceLine;

static void SyncEnhanceRow(const EnhanceLine *line,MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(line->image_view,line->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType GetEnhanceRow(const EnhanceLine *line,const ssize_t y,
  Quantum *magick_restrict *q,MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*q)=GetCacheViewAuthenticPixels(line->image_view,0,y,line->image->columns,1,
    line->exception);
  if ((*q) == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void SetEnhanceProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _ContrastHead
{
  const Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;

  int
    sign;
} ContrastHead;

static MagickBooleanType ContrastImageHead(const ContrastHead *head,
  const ssize_t y,MagickBooleanType *status)
{
  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(head->image_view,0,y,head->image->columns,1,
    head->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ContrastImageRow(head->image,head->sign,q);
  if (SyncCacheViewAuthenticPixels(head->image_view,head->exception) ==
        MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static inline int ContrastSign(const MagickBooleanType sharpen)
{
  return(sharpen != MagickFalse ? 1 : -1);
}

MagickExport MagickBooleanType ContrastImage(Image *image,
  const MagickBooleanType sharpen,ExceptionInfo *exception)
{
#define ContrastImageTag  "Contrast/Image"

  CacheView
    *image_view;

  int
    sign;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateContrastImage(image,sharpen,exception) != MagickFalse)
    return(MagickTrue);
#endif
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  sign=ContrastSign(sharpen);
  /*
    Contrast enhance colormap.
  */
  if (image->storage_class == PseudoClass)
    ContrastColormap(image,sign);
  /*
    Contrast enhance image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ContrastHead
      head;

    head.image=image;
    head.image_view=image_view;
    head.exception=exception;
    head.sign=sign;
    if (ContrastImageHead(&head,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEnhanceProgress(image,ContrastImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E n h a n c e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EnhanceImage() applies a digital filter that improves the quality of a
%  noisy image.
%
%  The format of the EnhanceImage method is:
%
%      Image *EnhanceImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define EnhanceImageTag  "Enhance/Image"
#define EnhancePixel(weight) \
  mean=QuantumScale*((double) GetPixelRed(image,r)+pixel.red)/2.0; \
  distance=QuantumScale*((double) GetPixelRed(image,r)-pixel.red); \
  distance_squared=(4.0+mean)*distance*distance; \
  mean=QuantumScale*((double) GetPixelGreen(image,r)+pixel.green)/2.0; \
  distance=QuantumScale*((double) GetPixelGreen(image,r)-pixel.green); \
  distance_squared+=(7.0-mean)*distance*distance; \
  mean=QuantumScale*((double) GetPixelBlue(image,r)+pixel.blue)/2.0; \
  distance=QuantumScale*((double) GetPixelBlue(image,r)-pixel.blue); \
  distance_squared+=(5.0-mean)*distance*distance; \
  mean=QuantumScale*((double) GetPixelBlack(image,r)+pixel.black)/2.0; \
  distance=QuantumScale*((double) GetPixelBlack(image,r)-pixel.black); \
  distance_squared+=(5.0-mean)*distance*distance; \
  mean=QuantumScale*((double) GetPixelAlpha(image,r)+pixel.alpha)/2.0; \
  distance=QuantumScale*((double) GetPixelAlpha(image,r)-pixel.alpha); \
  distance_squared+=(5.0-mean)*distance*distance; \
  if (distance_squared < 0.069) \
    { \
      aggregate.red+=(weight)*(double) GetPixelRed(image,r); \
      aggregate.green+=(weight)*(double) GetPixelGreen(image,r); \
      aggregate.blue+=(weight)*(double) GetPixelBlue(image,r); \
      aggregate.black+=(weight)*(double) GetPixelBlack(image,r); \
      aggregate.alpha+=(weight)*(double) GetPixelAlpha(image,r); \
      total_weight+=(weight); \
    } \
  r+=(ptrdiff_t) GetPixelChannels(image);

static PixelInfo EnhancePixelInfo(const Image *image,
  const Quantum *magick_restrict p,const ssize_t center)
{
  double
    distance,
    distance_squared,
    mean,
    total_weight;

  PixelInfo
    aggregate,
    pixel;

  const Quantum
    *magick_restrict r;

  GetPixelInfo(image,&aggregate);
  total_weight=0.0;
  GetPixelInfoPixel(image,p+center,&pixel);
  r=p;
  EnhancePixel(5.0); EnhancePixel(8.0); EnhancePixel(10.0);
    EnhancePixel(8.0); EnhancePixel(5.0);
  r=p+GetPixelChannels(image)*(image->columns+4);
  EnhancePixel(8.0); EnhancePixel(20.0); EnhancePixel(40.0);
    EnhancePixel(20.0); EnhancePixel(8.0);
  r=p+2*GetPixelChannels(image)*(image->columns+4);
  EnhancePixel(10.0); EnhancePixel(40.0); EnhancePixel(80.0);
    EnhancePixel(40.0); EnhancePixel(10.0);
  r=p+3*GetPixelChannels(image)*(image->columns+4);
  EnhancePixel(8.0); EnhancePixel(20.0); EnhancePixel(40.0);
    EnhancePixel(20.0); EnhancePixel(8.0);
  r=p+4*GetPixelChannels(image)*(image->columns+4);
  EnhancePixel(5.0); EnhancePixel(8.0); EnhancePixel(10.0);
    EnhancePixel(8.0); EnhancePixel(5.0);
  if (total_weight > MagickEpsilon)
    {
      pixel.red=((aggregate.red+total_weight/2.0)/total_weight);
      pixel.green=((aggregate.green+total_weight/2.0)/total_weight);
      pixel.blue=((aggregate.blue+total_weight/2.0)/total_weight);
      pixel.black=((aggregate.black+total_weight/2.0)/total_weight);
      pixel.alpha=((aggregate.alpha+total_weight/2.0)/total_weight);
    }
  return(pixel);
}

static void EnhanceImageRow(const Image *image,Image *enhance_image,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  PixelInfo
    pixel;

  ssize_t
    center,
    x;

  center=(ssize_t) GetPixelChannels(image)*(2*((ssize_t) image->columns+4)+2);
  GetPixelInfo(image,&pixel);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    pixel=EnhancePixelInfo(image,p,center);
    SetPixelViaPixelInfo(enhance_image,&pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(enhance_image);
  }
}

typedef struct _EnhanceHead
{
  const Image
    *image;

  Image
    *enhance_image;

  CacheView
    *image_view,
    *enhance_view;

  ExceptionInfo
    *exception;
} EnhanceHead;

static MagickBooleanType EnhanceImageHead(const EnhanceHead *head,
  const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(head->image_view,-2,y-2,head->image->columns+4,
    5,head->exception);
  q=QueueCacheViewAuthenticPixels(head->enhance_view,0,y,
    head->enhance_image->columns,1,head->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  EnhanceImageRow(head->image,head->enhance_image,p,q);
  if (SyncCacheViewAuthenticPixels(head->enhance_view,head->exception) ==
        MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static Image *FinishEnhanceImage(CacheView *enhance_view,CacheView *image_view,
  Image *enhance_image,const MagickBooleanType status)
{
  enhance_view=DestroyCacheView(enhance_view);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    enhance_image=DestroyImage(enhance_image);
  return(enhance_image);
}

MagickExport Image *EnhanceImage(const Image *image,ExceptionInfo *exception)
{
  CacheView
    *enhance_view,
    *image_view;

  Image
    *enhance_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Initialize enhanced image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  enhance_image=CloneImage(image,0,0,MagickTrue,
    exception);
  if (enhance_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(enhance_image,DirectClass,exception) == MagickFalse)
    {
      enhance_image=DestroyImage(enhance_image);
      return((Image *) NULL);
    }
  /*
    Enhance image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  enhance_view=AcquireAuthenticCacheView(enhance_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,enhance_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    EnhanceHead
      head;

    head.image=image;
    head.enhance_image=enhance_image;
    head.image_view=image_view;
    head.enhance_view=enhance_view;
    head.exception=exception;
    if (EnhanceImageHead(&head,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEnhanceProgress(image,EnhanceImageTag,progress,&status);
      }
  }
  return(FinishEnhanceImage(enhance_view,image_view,enhance_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o d u l a t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModulateImage() lets you control the brightness, saturation, and hue
%  of an image.  Modulate represents the brightness, saturation, and hue
%  as one parameter (e.g. 90,150,100).  If the image colorspace is HSL, the
%  modulation is lightness, saturation, and hue.  For HWB, use blackness,
%  whiteness, and hue. And for HCL, use chrome, luma, and hue.
%
%  The format of the ModulateImage method is:
%
%      MagickBooleanType ModulateImage(Image *image,const char *modulate,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o modulate: Define the percent change in brightness, saturation, and hue.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef void
  (*ModulateConvertMethod)(const double,const double,const double,double *,
    double *,double *);

typedef struct _ModulateConversion
{
  const ModulateInfo
    *modulate_info;

  ModulateConvertMethod
    convert_from_rgb,
    convert_to_rgb;
} ModulateConversion;

static inline void ModulateHueSpace(const ModulateConversion *conversion,
  double *red,double *green,double *blue)
{
  const ModulateInfo
    *modulate_info = conversion->modulate_info;

  const ModulateConvertMethod
    convert_from_rgb = conversion->convert_from_rgb,
    convert_to_rgb = conversion->convert_to_rgb;

  const double
    percent_hue = modulate_info->percent_hue,
    percent_saturation = modulate_info->percent_saturation,
    percent_brightness = modulate_info->percent_brightness;

  double
    brightness,
    hue,
    saturation;

  /*
    Increase or decrease color brightness, saturation, or hue.
  */
  convert_from_rgb(*red,*green,*blue,&hue,&saturation,&brightness);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  saturation*=0.01*percent_saturation;
  brightness*=0.01*percent_brightness;
  convert_to_rgb(hue,saturation,brightness,red,green,blue);
}

static inline void ModulateHCL(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const ModulateConversion
    conversion = { modulate_info,ConvertRGBToHCL,ConvertHCLToRGB };

  ModulateHueSpace(&conversion,red,green,blue);
}

static inline void ModulateHCLp(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const ModulateConversion
    conversion = { modulate_info,ConvertRGBToHCLp,ConvertHCLpToRGB };

  ModulateHueSpace(&conversion,red,green,blue);
}

static inline void ModulateHSB(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const ModulateConversion
    conversion = { modulate_info,ConvertRGBToHSB,ConvertHSBToRGB };

  ModulateHueSpace(&conversion,red,green,blue);
}

static inline void ModulateHSI(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const ModulateConversion
    conversion = { modulate_info,ConvertRGBToHSI,ConvertHSIToRGB };

  ModulateHueSpace(&conversion,red,green,blue);
}

static inline void ModulateHSL(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const ModulateConversion
    conversion = { modulate_info,ConvertRGBToHSL,ConvertHSLToRGB };

  ModulateHueSpace(&conversion,red,green,blue);
}

static inline void ModulateHSV(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const ModulateConversion
    conversion = { modulate_info,ConvertRGBToHSV,ConvertHSVToRGB };

  ModulateHueSpace(&conversion,red,green,blue);
}

static inline void ModulateHWB(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_hue = modulate_info->percent_hue,
    percent_whiteness = modulate_info->percent_saturation,
    percent_blackness = modulate_info->percent_brightness;

  double
    blackness,
    hue,
    whiteness;

  /*
    Increase or decrease color blackness, whiteness, or hue.
  */
  ConvertRGBToHWB(*red,*green,*blue,&hue,&whiteness,&blackness);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  blackness*=0.01*percent_blackness;
  whiteness*=0.01*percent_whiteness;
  ConvertHWBToRGB(hue,whiteness,blackness,red,green,blue);
}

typedef void
  (*ModulateLCHMethod)(const double,const double,const double,
    const IlluminantType,double *,double *,double *);

typedef struct _ModulateLCHConversion
{
  const ModulateInfo
    *modulate_info;

  ModulateLCHMethod
    convert_from_rgb,
    convert_to_rgb;
} ModulateLCHConversion;

static inline void ModulateLCHSpace(const ModulateLCHConversion *conversion,
  double *red,double *green,double *blue)
{
  const ModulateInfo
    *modulate_info = conversion->modulate_info;

  const ModulateLCHMethod
    convert_from_rgb = conversion->convert_from_rgb,
    convert_to_rgb = conversion->convert_to_rgb;

  const double
    percent_luma = modulate_info->percent_brightness,
    percent_chroma = modulate_info->percent_saturation,
    percent_hue = modulate_info->percent_hue;

  const IlluminantType
    illuminant = modulate_info->illuminant;

  double
    hue,
    luma,
    chroma;

  /*
    Increase or decrease color luma, chroma, or hue.
  */
  convert_from_rgb(*red,*green,*blue,illuminant,&luma,&chroma,&hue);
  luma*=0.01*percent_luma;
  chroma*=0.01*percent_chroma;
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  convert_to_rgb(luma,chroma,hue,illuminant,red,green,blue);
}

static inline void ModulateLCHab(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const ModulateLCHConversion
    conversion = { modulate_info,ConvertRGBToLCHab,ConvertLCHabToRGB };

  ModulateLCHSpace(&conversion,red,green,blue);
}

static inline void ModulateLCHuv(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const ModulateLCHConversion
    conversion = { modulate_info,ConvertRGBToLCHuv,ConvertLCHuvToRGB };

  ModulateLCHSpace(&conversion,red,green,blue);
}

static void ModulateDefaultColorspaces(const ModulateInfo *modulate_info,double *red,
  double *green,double *blue)
{
  const ColorspaceType
    colorspace = modulate_info->colorspace;

  switch (colorspace)
  {
    case HSLColorspace:
    default:
    {
      ModulateHSL(modulate_info,red,green,blue);
      break;
    }
    case HSVColorspace:
    {
      ModulateHSV(modulate_info,red,green,blue);
      break;
    }
    case LCHColorspace:
    case LCHabColorspace:
    {
      ModulateLCHab(modulate_info,red,green,blue);
      break;
    }
  }
}

static void ModulateColor(const ModulateInfo *modulate_info,double *red,
  double *green,double *blue)
{
  const ColorspaceType
    colorspace = modulate_info->colorspace;

  switch (colorspace)
  {
    case HCLColorspace:
    {
      ModulateHCL(modulate_info,red,green,blue);
      break;
    }
    case HCLpColorspace:
    {
      ModulateHCLp(modulate_info,red,green,blue);
      break;
    }
    case HSBColorspace:
    {
      ModulateHSB(modulate_info,red,green,blue);
      break;
    }
    case HSIColorspace:
    {
      ModulateHSI(modulate_info,red,green,blue);
      break;
    }
    default:
    {
      ModulateDefaultColorspaces(modulate_info,red,green,blue);
      break;
    }
    case HWBColorspace:
    {
      ModulateHWB(modulate_info,red,green,blue);
      break;
    }
    case LCHuvColorspace:
    {
      ModulateLCHuv(modulate_info,red,green,blue);
      break;
    }
  }
}

static void ParseModulatePercents(const char *modulate,
  double *percent_brightness,double *percent_saturation,double *percent_hue)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  flags=ParseGeometry(modulate,&geometry_info);
  if ((flags & RhoValue) != 0)
    *percent_brightness=geometry_info.rho;
  if ((flags & SigmaValue) != 0)
    *percent_saturation=geometry_info.sigma;
  if ((flags & XiValue) != 0)
    *percent_hue=geometry_info.xi;
}

static void GetModulateColorspace(const Image *image,
  ColorspaceType *colorspace,IlluminantType *illuminant)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"modulate:colorspace");
  if (artifact != (const char *) NULL)
    *colorspace=(ColorspaceType) ParseCommandOption(MagickColorspaceOptions,
      MagickFalse,artifact);
  artifact=GetImageArtifact(image,"color:illuminant");
  if (artifact != (const char *) NULL)
    {
      ssize_t
        illuminant_type;

      illuminant_type=ParseCommandOption(MagickIlluminantOptions,MagickFalse,
        artifact);
      if (illuminant_type < 0)
        {
          *illuminant=UndefinedIlluminant;
          *colorspace=UndefinedColorspace;
        }
      else
        *illuminant=(IlluminantType) illuminant_type;
    }
}

typedef struct _ModulateRow
{
  Image
    *image;

  ColorspaceType
    colorspace;

  double
    percent_hue,
    percent_saturation,
    percent_brightness;

  IlluminantType
    illuminant;
} ModulateRow;

static void ModulateImageRow(const ModulateRow *modulate_row,Quantum *q)
{
  Image
    *const image = modulate_row->image;

  const ColorspaceType
    colorspace = modulate_row->colorspace;

  const double
    percent_hue = modulate_row->percent_hue,
    percent_saturation = modulate_row->percent_saturation,
    percent_brightness = modulate_row->percent_brightness;

  const IlluminantType
    illuminant = modulate_row->illuminant;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    const ModulateInfo modulate_info = { colorspace, percent_hue,
      percent_saturation, percent_brightness, illuminant };

    double
      blue,
      green,
      red;

    red=(double) GetPixelRed(image,q);
    green=(double) GetPixelGreen(image,q);
    blue=(double) GetPixelBlue(image,q);
    ModulateColor(&modulate_info,&red,&green,&blue);
    SetPixelRed(image,ClampToQuantum(red),q);
    SetPixelGreen(image,ClampToQuantum(green),q);
    SetPixelBlue(image,ClampToQuantum(blue),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ModulateImageColormap(const ModulateRow *modulate_row)
{
  Image
    *const image = modulate_row->image;

  const ColorspaceType
    colorspace = modulate_row->colorspace;

  const double
    percent_hue = modulate_row->percent_hue,
    percent_saturation = modulate_row->percent_saturation,
    percent_brightness = modulate_row->percent_brightness;

  const IlluminantType
    illuminant = modulate_row->illuminant;

  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    const ModulateInfo modulate_info = { colorspace, percent_hue,
      percent_saturation, percent_brightness, illuminant };

    double
      blue,
      green,
      red;

    /*
      Modulate image colormap.
    */
    red=(double) image->colormap[i].red;
    green=(double) image->colormap[i].green;
    blue=(double) image->colormap[i].blue;
    ModulateColor(&modulate_info,&red,&green,&blue);
    image->colormap[i].red=red;
    image->colormap[i].green=green;
    image->colormap[i].blue=blue;
  }
}

typedef struct _ModulateHead
{
  ModulateRow
    modulate_row;

  EnhanceLine
    line;
} ModulateHead;

static MagickBooleanType ModulateImageHead(const ModulateHead *head,
  const ssize_t y,MagickBooleanType *status)
{
  Quantum
    *magick_restrict q;

  if (GetEnhanceRow(&head->line,y,&q,status) == MagickFalse)
    return(MagickFalse);
  ModulateImageRow(&head->modulate_row,q);
  SyncEnhanceRow(&head->line,status);
  return(MagickTrue);
}

#define ModulateImageTag  "Modulate/Image"

typedef struct _ModulateImageInput
{
  Image
    *image;

  ColorspaceType
    colorspace;

  double
    percent_hue,
    percent_saturation,
    percent_brightness;

  IlluminantType
    illuminant;

  ExceptionInfo
    *exception;
} ModulateImageInput;

static MagickBooleanType ModulateImagePixels(const ModulateImageInput *input)
{
  Image
    *const image = input->image;

  const ColorspaceType
    colorspace = input->colorspace;

  const double
    percent_hue = input->percent_hue,
    percent_saturation = input->percent_saturation,
    percent_brightness = input->percent_brightness;

  const IlluminantType
    illuminant = input->illuminant;

  ExceptionInfo
    *const exception = input->exception;

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const ModulateHead
      head = { { image, colorspace, percent_hue, percent_saturation,
        percent_brightness, illuminant }, { image,image_view,exception } };

    if (ModulateImageHead(&head,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEnhanceProgress(image,ModulateImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

MagickExport MagickBooleanType ModulateImage(Image *image,const char *modulate,
  ExceptionInfo *exception)
{
  ColorspaceType
    colorspace = UndefinedColorspace;

  double
    percent_brightness = 100.0,
    percent_hue = 100.0,
    percent_saturation = 100.0;

  IlluminantType
    illuminant = D65Illuminant;

  ModulateImageInput
    input;

  /*
    Initialize modulate table.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (modulate == (char *) NULL)
    return(MagickFalse);
  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  ParseModulatePercents(modulate,&percent_brightness,&percent_saturation,
    &percent_hue);
  GetModulateColorspace(image,&colorspace,&illuminant);
  if (image->storage_class == PseudoClass)
    {
      const ModulateRow
        modulate_row = { image, colorspace, percent_hue, percent_saturation,
          percent_brightness, illuminant };

      ModulateImageColormap(&modulate_row);
    }
  /*
    Modulate image.
  */
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateModulateImage(image,percent_brightness,percent_hue,
        percent_saturation,colorspace,exception) != MagickFalse)
    return(MagickTrue);
#endif
  input.image=image;
  input.colorspace=colorspace;
  input.percent_hue=percent_hue;
  input.percent_saturation=percent_saturation;
  input.percent_brightness=percent_brightness;
  input.illuminant=illuminant;
  input.exception=exception;
  return(ModulateImagePixels(&input));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S i g m o i d a l C o n t r a s t I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SigmoidalContrastImage() adjusts the contrast of an image with a non-linear
%  sigmoidal contrast algorithm.  Increase the contrast of the image using a
%  sigmoidal transfer function without saturating highlights or shadows.
%  Contrast indicates how much to increase the contrast (0 is none; 3 is
%  typical; 20 is pushing it); mid-point indicates where midtones fall in the
%  resultant image (0 is white; 50% is middle-gray; 100% is black).  Set
%  sharpen to MagickTrue to increase the image contrast otherwise the contrast
%  is reduced.
%
%  The format of the SigmoidalContrastImage method is:
%
%      MagickBooleanType SigmoidalContrastImage(Image *image,
%        const MagickBooleanType sharpen,const char *levels,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o sharpen: Increase or decrease image contrast.
%
%    o contrast: strength of the contrast, the larger the number the more
%      'threshold-like' it becomes.
%
%    o midpoint: midpoint of the function as a color value 0 to QuantumRange.
%
%    o exception: return any errors or warnings in this structure.
%
*/

/*
  ImageMagick 6 has a version of this function which uses LUTs.
*/

/*
  Sigmoidal function Sigmoidal with inflexion point moved to b and "slope
  constant" set to a.

  The first version, based on the hyperbolic tangent tanh, when combined with
  the scaling step, is an exact arithmetic clone of the sigmoid function
  based on the logistic curve. The equivalence is based on the identity

    1/(1+exp(-t)) = (1+tanh(t/2))/2

  (http://de.wikipedia.org/wiki/Sigmoidfunktion) and the fact that the
  scaled sigmoidal derivation is invariant under affine transformations of
  the ordinate.

  The tanh version is almost certainly more accurate and cheaper.  The 0.5
  factor in the argument is to clone the legacy ImageMagick behavior. The
  reason for making the define depend on atanh even though it only uses tanh
  has to do with the construction of the inverse of the scaled sigmoidal.
*/
#if defined(MAGICKCORE_HAVE_ATANH)
#define Sigmoidal(a,b,x) ( tanh((0.5*(a))*((x)-(b))) )
#else
#define Sigmoidal(a,b,x) ( 1.0/(1.0+exp((a)*((b)-(x)))) )
#endif
/*
  Scaled sigmoidal function:

    ( Sigmoidal(a,b,x) - Sigmoidal(a,b,0) ) /
    ( Sigmoidal(a,b,1) - Sigmoidal(a,b,0) )

  See http://osdir.com/ml/video.image-magick.devel/2005-04/msg00006.html and
  http://www.cs.dartmouth.edu/farid/downloads/tutorials/fip.pdf.  The limit
  of ScaledSigmoidal as a->0 is the identity, but a=0 gives a division by
  zero. This is fixed below by exiting immediately when contrast is small,
  leaving the image (or colormap) unmodified. This appears to be safe because
  the series expansion of the logistic sigmoidal function around x=b is

  1/2-a*(b-x)/4+...

  so that the key denominator s(1)-s(0) is about a/4 (a/2 with tanh).
*/
#define ScaledSigmoidal(a,b,x) (                    \
  (Sigmoidal((a),(b),(x))-Sigmoidal((a),(b),0.0)) / \
  (Sigmoidal((a),(b),1.0)-Sigmoidal((a),(b),0.0)) )
/*
  Inverse of ScaledSigmoidal, used for +sigmoidal-contrast.  Because b
  may be 0 or 1, the argument of the hyperbolic tangent (resp. logistic
  sigmoidal) may be outside of the interval (-1,1) (resp. (0,1)), even
  when creating a LUT from in gamut values, hence the branching.  In
  addition, HDRI may have out of gamut values.
  InverseScaledSigmoidal is not a two-sided inverse of ScaledSigmoidal:
  It is only a right inverse. This is unavoidable.
*/
static inline double InverseScaledSigmoidal(const double a,const double b,
  const double x)
{
  const double sig0=Sigmoidal(a,b,0.0);
  const double sig1=Sigmoidal(a,b,1.0);
  const double argument=(sig1-sig0)*x+sig0;
  const double clamped=
    (
#if defined(MAGICKCORE_HAVE_ATANH)
      argument < -1+MagickEpsilon
      ?
      -1+MagickEpsilon
      :
      ( argument > 1-MagickEpsilon ? 1-MagickEpsilon : argument )
    );
  return(b+(2.0/a)*atanh(clamped));
#else
      argument < MagickEpsilon
      ?
      MagickEpsilon
      :
      ( argument > 1-MagickEpsilon ? 1-MagickEpsilon : argument )
    );
  return(b-log(1.0/clamped-1.0)/a);
#endif
}

#define ScaledSig(x) (ClampToQuantum((double) QuantumRange* \
  ScaledSigmoidal(contrast,QuantumScale*midpoint,QuantumScale*((double) x))) )
#define InverseScaledSig(x) (ClampToQuantum((double) QuantumRange* \
  InverseScaledSigmoidal(contrast,QuantumScale*midpoint,QuantumScale* \
  ((double) x))) )

static void InverseSigmoidalColormapEntry(Image *image,const ssize_t i,
  const double contrast,const double midpoint)
{
  if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
    image->colormap[i].red=(MagickRealType) InverseScaledSig(
      image->colormap[i].red);
  if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
    image->colormap[i].green=(MagickRealType) InverseScaledSig(
      image->colormap[i].green);
  if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
    image->colormap[i].blue=(MagickRealType) InverseScaledSig(
      image->colormap[i].blue);
  if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
    image->colormap[i].alpha=(MagickRealType) InverseScaledSig(
      image->colormap[i].alpha);
}

static void ScaledSigmoidalColormap(Image *image,const double contrast,
  const double midpoint)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].red=(MagickRealType) ScaledSig(
        image->colormap[i].red);
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].green=(MagickRealType) ScaledSig(
        image->colormap[i].green);
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].blue=(MagickRealType) ScaledSig(
        image->colormap[i].blue);
    if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].alpha=(MagickRealType) ScaledSig(
        image->colormap[i].alpha);
  }
}

static void SigmoidalContrastColormap(Image *image,
  const MagickBooleanType sharpen,const double contrast,const double midpoint)
{
  ssize_t
    i;

  if( sharpen != MagickFalse )
    ScaledSigmoidalColormap(image,contrast,midpoint);
  else
    for (i=0; i < (ssize_t) image->colors; i++)
      InverseSigmoidalColormapEntry(image,i,contrast,midpoint);
}

static void SigmoidalContrastImageRow(const SigmoidalContrastRow *row,
  Quantum *q)
{
  const Image
    *const image = row->image;

  const MagickBooleanType
    sharpen = row->sharpen;

  const double
    contrast = row->contrast,
    midpoint = row->midpoint;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      if( sharpen != MagickFalse )
        q[i]=ScaledSig(q[i]);
      else
        q[i]=InverseScaledSig(q[i]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType SigmoidalContrastImage(Image *image,
  const MagickBooleanType sharpen,const double contrast,const double midpoint,
  ExceptionInfo *exception)
{
#define SigmoidalContrastImageTag  "SigmoidalContrast/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Convenience macros.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Side effect: may clamp values unless contrast<MagickEpsilon, in which
    case nothing is done.
  */
  if (contrast < MagickEpsilon)
    return(MagickTrue);
  /*
    Sigmoidal-contrast enhance colormap.
  */
  if (image->storage_class == PseudoClass)
    SigmoidalContrastColormap(image,sharpen,contrast,midpoint);
  /*
    Sigmoidal-contrast enhance image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    const EnhanceLine
      line = { image,image_view,exception };

    const SigmoidalContrastRow
      row = { image, sharpen, contrast, midpoint };

    if (GetEnhanceRow(&line,y,&q,&status) == MagickFalse)
      continue;
    SigmoidalContrastImageRow(&row,q);
    SyncEnhanceRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEnhanceProgress(image,SigmoidalContrastImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     W h i t e B a l a n c e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WhiteBalanceImage() applies white balancing to an image according to a
%  grayworld assumption in the LAB colorspace.
%
%  The format of the WhiteBalanceImage method is:
%
%      MagickBooleanType WhiteBalanceImage(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-level
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ShiftChromaRow(const Image *image,const double a_mean,
  const double b_mean,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      a,
      b;

    /*
      Scale the chroma distance shifted according to amount of luminance.
    */
    a=(double) GetPixela(image,q)-1.1*(double) GetPixelL(image,q)*a_mean;
    b=(double) GetPixelb(image,q)-1.1*(double) GetPixelL(image,q)*b_mean;
    SetPixela(image,ClampToQuantum(a),q);
    SetPixelb(image,ClampToQuantum(b),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickStatusType LevelVibrance(Image *image,const char *artifact,
  MagickStatusType status,ExceptionInfo *exception)
{
  ChannelType
    channel_mask;

  double
    black_point = 0.0;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  /*
    Level the a & b channels.
  */
  flags=ParseGeometry(artifact,&geometry_info);
  if ((flags & RhoValue) != 0)
    black_point=geometry_info.rho;
  if ((flags & PercentValue) != 0)
    black_point*=((double) QuantumRange/100.0);
  channel_mask=SetImageChannelMask(image,(ChannelType) (aChannel |
    bChannel));
  status&=(MagickStatusType) LevelImage(image,black_point,(double)
    QuantumRange-black_point,1.0,exception);
  (void) SetImageChannelMask(image,channel_mask);
  return(status);
}

static void AccumulateChromaMeans(const Image *image,
  const Quantum *magick_restrict p,double *a_mean,double *b_mean)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    (*a_mean)+=QuantumScale*(double) GetPixela(image,p)-0.5;
    (*b_mean)+=QuantumScale*(double) GetPixelb(image,p)-0.5;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickStatusType LevelWhiteBalanceVibrance(Image *image,
  MagickStatusType status,ExceptionInfo *exception)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"white-balance:vibrance");
  if (artifact != (const char *) NULL)
    status=LevelVibrance(image,artifact,status,exception);
  return(status);
}

static void SumWhiteBalanceChroma(const EnhanceLine *line,
  MagickStatusType *status,double *a_mean,double *b_mean)
{
  ssize_t
    y;

  for (y=0; y < (ssize_t) line->image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    if ((*status) == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(line->image_view,0,y,line->image->columns,1,
      line->exception);
    if (p == (Quantum *) NULL)
      {
        (*status)=MagickFalse;
        continue;
      }
    AccumulateChromaMeans(line->image,p,a_mean,b_mean);
  }
}

static MagickBooleanType FinishWhiteBalance(Image *image,
  MagickStatusType status,ExceptionInfo *exception)
{
  status=LevelWhiteBalanceVibrance(image,status,exception);
  status&=(MagickStatusType) TransformImageColorspace(image,sRGBColorspace,
    exception);
  return(status != 0 ? MagickTrue : MagickFalse);
}

typedef struct _WhiteBalanceHead
{
  const Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;

  double
    a_mean,
    b_mean;
} WhiteBalanceHead;

static MagickBooleanType ShiftWhiteBalanceHead(const WhiteBalanceHead *head,
  const ssize_t y,MagickStatusType *status)
{
  const Image
    *const image = head->image;

  CacheView
    *const image_view = head->image_view;

  const double
    a_mean = head->a_mean,
    b_mean = head->b_mean;

  ExceptionInfo
    *const exception = head->exception;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ShiftChromaRow(image,a_mean,b_mean,q);
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void ReportWhiteBalanceProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickStatusType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport MagickBooleanType WhiteBalanceImage(Image *image,
  ExceptionInfo *exception)
{
#define WhiteBalanceImageTag  "WhiteBalance/Image"

  CacheView
    *image_view;

  double
    a_mean,
    b_mean;

  EnhanceLine
    line;

  MagickOffsetType
    progress;

  MagickStatusType
    status;

  ssize_t
    y;

  /*
    White balance image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  status=TransformImageColorspace(image,LabColorspace,exception);
  a_mean=0.0;
  b_mean=0.0;
  image_view=AcquireAuthenticCacheView(image,exception);
  line.image=image;
  line.image_view=image_view;
  line.exception=exception;
  SumWhiteBalanceChroma(&line,&status,&a_mean,&b_mean);
  a_mean/=((double) image->columns*image->rows);
  b_mean/=((double) image->columns*image->rows);
  progress=0;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    WhiteBalanceHead
      head;

    head.image=image;
    head.image_view=image_view;
    head.exception=exception;
    head.a_mean=a_mean;
    head.b_mean=b_mean;
    if (ShiftWhiteBalanceHead(&head,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportWhiteBalanceProgress(image,WhiteBalanceImageTag,progress,
          &status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(FinishWhiteBalance(image,status,exception));
}
