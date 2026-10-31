/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%       TTTTT  H   H  RRRR   EEEEE  SSSSS  H   H   OOO   L      DDDD          %
%         T    H   H  R   R  E      SS     H   H  O   O  L      D   D         %
%         T    HHHHH  RRRR   EEE     SSS   HHHHH  O   O  L      D   D         %
%         T    H   H  R R    E         SS  H   H  O   O  L      D   D         %
%         T    H   H  R  R   EEEEE  SSSSS  H   H   OOO   LLLLL  DDDD          %
%                                                                             %
%                                                                             %
%                      MagickCore Image Threshold Methods                     %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
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
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/configure.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/shear.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
  Define declarations.
*/
#define ThresholdsFilename  "thresholds.xml"

/*
  Typedef declarations.
*/
struct _ThresholdMap
{
  char
    *map_id,
    *description;

  size_t
    width,
    height;

  ssize_t
    divisor,
    *levels;
};

/*
  Static declarations.
*/
#if MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
  #include "MagickCore/threshold-map.h"
#else
static const char *const
  BuiltinMap=
    "<?xml version=\"1.0\"?>"
    "<thresholds>"
    "  <threshold map=\"threshold\" alias=\"1x1\">"
    "    <description>Threshold 1x1 (non-dither)</description>"
    "    <levels width=\"1\" height=\"1\" divisor=\"2\">"
    "        1"
    "    </levels>"
    "  </threshold>"
    "  <threshold map=\"checks\" alias=\"2x1\">"
    "    <description>Checkerboard 2x1 (dither)</description>"
    "    <levels width=\"2\" height=\"2\" divisor=\"3\">"
    "       1 2"
    "       2 1"
    "    </levels>"
    "  </threshold>"
    "</thresholds>";
#endif

/*
  Forward declarations.
*/
static ThresholdMap
  *GetThresholdMapFile(const char *,const char *,const char *,ExceptionInfo *);



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B i l e v e l I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BilevelImage() changes the value of individual pixels based on the
%  intensity of each pixel channel.  The result is a high-contrast image.
%
%  More precisely each channel value of the image is 'thresholded' so that if
%  it is equal to or less than the given value it is set to zero, while any
%  value greater than that give is set to it maximum or QuantumRange.
%
%  This function is what is used to implement the "-threshold" operator for
%  the command line API.
%
%  If the default channel setting is given the image is thresholded using just
%  the gray 'intensity' of the image, rather than the individual channels.
%
%  The format of the BilevelImage method is:
%
%      MagickBooleanType BilevelImage(Image *image,const double threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold: define the threshold values.
%
%    o exception: return any errors or warnings in this structure.
%
%  Aside: You can get the same results as operator using LevelImages()
%  with the 'threshold' value for both the black_point and the white_point.
%
*/
static void BilevelPixel(const Image *image,const double threshold,
  double pixel,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (image->channel_mask != AllChannels)
      pixel=(double) q[i];
    q[i]=(Quantum) (pixel <= threshold ? 0 : QuantumRange);
  }
}

static MagickBooleanType SetDirectNonGrayImage(Image *image,
  ExceptionInfo *exception)
{
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if (IsGrayColorspace(image->colorspace) == MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  return(MagickTrue);
}

MagickExport MagickBooleanType BilevelImage(Image *image,const double threshold,
  ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetDirectNonGrayImage(image,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Bilevel threshold image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        pixel;

      pixel=GetPixelIntensity(image,q);
      BilevelPixel(image,threshold,pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ThresholdImageTag,progress++,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
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
%     B l a c k T h r e s h o l d I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlackThresholdImage() is like ThresholdImage() but forces all pixels below
%  the threshold into black while leaving all pixels at or above the threshold
%  unchanged.
%
%  The format of the BlackThresholdImage method is:
%
%      MagickBooleanType BlackThresholdImage(Image *image,
%        const char *threshold,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold: define the threshold value.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static PixelInfo GetThresholdLevels(const Image *image,
  const char *thresholds)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  PixelInfo
    threshold;

  GetPixelInfo(image,&threshold);
  flags=ParseGeometry(thresholds,&geometry_info);
  threshold.red=geometry_info.rho;
  threshold.green=geometry_info.rho;
  threshold.blue=geometry_info.rho;
  threshold.black=geometry_info.rho;
  threshold.alpha=100.0;
  if ((flags & SigmaValue) != 0)
    threshold.green=geometry_info.sigma;
  if ((flags & XiValue) != 0)
    threshold.blue=geometry_info.xi;
  if ((flags & PsiValue) != 0)
    threshold.alpha=geometry_info.psi;
  if (threshold.colorspace == CMYKColorspace)
    {
      if ((flags & PsiValue) != 0)
        threshold.black=geometry_info.psi;
      if ((flags & ChiValue) != 0)
        threshold.alpha=geometry_info.chi;
    }
  if ((flags & PercentValue) != 0)
    {
      threshold.red*=((MagickRealType) QuantumRange/100.0);
      threshold.green*=((MagickRealType) QuantumRange/100.0);
      threshold.blue*=((MagickRealType) QuantumRange/100.0);
      threshold.black*=((MagickRealType) QuantumRange/100.0);
      threshold.alpha*=((MagickRealType) QuantumRange/100.0);
    }
  return(threshold);
}

static void BlackThresholdPixel(const Image *image,const PixelInfo *threshold,
  double pixel,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (image->channel_mask != AllChannels)
      pixel=(double) q[i];
    if (pixel < GetPixelInfoChannel(threshold,channel))
      q[i]=(Quantum) 0;
  }
}

static void BlackThresholdRow(const Image *image,const PixelInfo *threshold,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      pixel;

    pixel=GetPixelIntensity(image,q);
    BlackThresholdPixel(image,threshold,pixel,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType SetDirectColorImage(Image *image,
  ExceptionInfo *exception)
{
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  return(MagickTrue);
}

MagickExport MagickBooleanType BlackThresholdImage(Image *image,
  const char *thresholds,ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    threshold;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (thresholds == (const char *) NULL)
    return(MagickTrue);
  if (SetDirectColorImage(image,exception) == MagickFalse)
    return(MagickFalse);
  threshold=GetThresholdLevels(image,thresholds);
  /*
    White threshold image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    BlackThresholdRow(image,&threshold,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ThresholdImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
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
%     C l a m p I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClampImage() set each pixel whose value is below zero to zero and any the
%  pixel whose value is above the quantum range to the quantum range (e.g.
%  65535) otherwise the pixel value remains unchanged.
%
%  The format of the ClampImage method is:
%
%      MagickBooleanType ClampImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void ClampPixelChannels(const Image *image,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=ClampPixel((MagickRealType) q[i]);
  }
}

static MagickBooleanType ClampImageColormap(Image *image,
  ExceptionInfo *exception)
{
  ssize_t
    i;

  PixelInfo
    *magick_restrict q;

  q=image->colormap;
  for (i=0; i < (ssize_t) image->colors; i++)
  {
    q->red=(double) ClampPixel(q->red);
    q->green=(double) ClampPixel(q->green);
    q->blue=(double) ClampPixel(q->blue);
    q->alpha=(double) ClampPixel(q->alpha);
    q++;
  }
  return(SyncImage(image,exception));
}

MagickExport MagickBooleanType ClampImage(Image *image,ExceptionInfo *exception)
{
#define ClampImageTag  "Clamp/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    return(ClampImageColormap(image,exception));
  /*
    Clamp image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      ClampPixelChannels(image,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ClampImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
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
%     C o l o r T h r e s h o l d I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ColorThresholdImage() forces all pixels in the color range to white
%  otherwise black.
%
%  The format of the ColorThresholdImage method is:
%
%      MagickBooleanType ColorThresholdImage(Image *image,
%        const PixelInfo *start_color,const PixelInfo *stop_color,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o start_color, stop_color: define the start and stop color range.  Any
%      pixel within the range returns white otherwise black.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType IsPixelInColorRange(const Image *image,
  const PixelInfo *start,const PixelInfo *stop,const Quantum *q)
{
  MagickBooleanType
    foreground = MagickTrue;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (((double) q[i] < GetPixelInfoChannel(start,channel)) ||
        ((double) q[i] > GetPixelInfoChannel(stop,channel)))
      foreground=MagickFalse;
  }
  return(foreground);
}

static IlluminantType GetThresholdIlluminant(const char *artifact,
  IlluminantType illuminant)
{
  if (artifact != (const char *) NULL)
    {
      ssize_t
        illuminant_type;

      illuminant_type=ParseCommandOption(MagickIlluminantOptions,MagickFalse,
        artifact);
      if (illuminant_type < 0)
        illuminant=UndefinedIlluminant;
      else
        illuminant=(IlluminantType) illuminant_type;
    }
  return(illuminant);
}

typedef struct _ColorThresholdRange
{
  Image
    *image;

  const PixelInfo
    *start_color,
    *stop_color;

  IlluminantType
    illuminant;
} ColorThresholdRange;

static void ConvertColorThresholdRange(const ColorThresholdRange *range,
  PixelInfo *start,PixelInfo *stop)
{
  switch (range->image->colorspace)
  {
    case HCLColorspace:
    {
      ConvertRGBToHCL(range->start_color->red,range->start_color->green,
        range->start_color->blue,&(*start).red,&(*start).green,&(*start).blue);
      ConvertRGBToHCL(range->stop_color->red,range->stop_color->green,
        range->stop_color->blue,&(*stop).red,&(*stop).green,&(*stop).blue);
      break;
    }
    case HSBColorspace:
    {
      ConvertRGBToHSB(range->start_color->red,range->start_color->green,
        range->start_color->blue,&(*start).red,&(*start).green,&(*start).blue);
      ConvertRGBToHSB(range->stop_color->red,range->stop_color->green,
        range->stop_color->blue,&(*stop).red,&(*stop).green,&(*stop).blue);
      break;
    }
    case HSLColorspace:
    {
      ConvertRGBToHSL(range->start_color->red,range->start_color->green,
        range->start_color->blue,&(*start).red,&(*start).green,&(*start).blue);
      ConvertRGBToHSL(range->stop_color->red,range->stop_color->green,
        range->stop_color->blue,&(*stop).red,&(*stop).green,&(*stop).blue);
      break;
    }
    case HSVColorspace:
    {
      ConvertRGBToHSV(range->start_color->red,range->start_color->green,
        range->start_color->blue,&(*start).red,&(*start).green,&(*start).blue);
      ConvertRGBToHSV(range->stop_color->red,range->stop_color->green,
        range->stop_color->blue,&(*stop).red,&(*stop).green,&(*stop).blue);
      break;
    }
    case HWBColorspace:
    {
      ConvertRGBToHWB(range->start_color->red,range->start_color->green,
        range->start_color->blue,&(*start).red,&(*start).green,&(*start).blue);
      ConvertRGBToHWB(range->stop_color->red,range->stop_color->green,
        range->stop_color->blue,&(*stop).red,&(*stop).green,&(*stop).blue);
      break;
    }
    case LabColorspace:
    {
      ConvertRGBToLab(range->start_color->red,range->start_color->green,
        range->start_color->blue,range->illuminant,&(*start).red,
        &(*start).green,&(*start).blue);
      ConvertRGBToLab(range->stop_color->red,range->stop_color->green,
        range->stop_color->blue,range->illuminant,&(*stop).red,
        &(*stop).green,&(*stop).blue);
      break;
    }
    default:
    {
      (*start).red*=QuantumScale;
      (*start).green*=QuantumScale;
      (*start).blue*=QuantumScale;
      (*stop).red*=QuantumScale;
      (*stop).green*=QuantumScale;
      (*stop).blue*=QuantumScale;
      break;
    }
  }
}

static void ColorThresholdRow(Image *image,const PixelInfo *start,
  const PixelInfo *stop,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    MagickBooleanType
      foreground = MagickTrue;

    foreground=IsPixelInColorRange(image,start,stop,q);
    SetPixelIndex(image,(Quantum) (foreground != MagickFalse ? 1 : 0),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static IlluminantType GetImageThresholdIlluminant(const Image *image,
  IlluminantType illuminant)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"color:illuminant");
  illuminant=GetThresholdIlluminant(artifact,illuminant);
  return(illuminant);
}

MagickExport MagickBooleanType ColorThresholdImage(Image *image,
  const PixelInfo *start_color,const PixelInfo *stop_color,
  ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  CacheView
    *image_view;

  ColorThresholdRange
    range;

  IlluminantType
    illuminant = D65Illuminant;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    start,
    stop;

  ssize_t
    y;

  /*
    Color threshold image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=AcquireImageColormap(image,2,exception);
  if (status == MagickFalse)
    return(status);
  illuminant=GetImageThresholdIlluminant(image,illuminant);
  start=(*start_color);
  stop=(*stop_color);
  range.image=image;
  range.start_color=start_color;
  range.stop_color=stop_color;
  range.illuminant=illuminant;
  ConvertColorThresholdRange(&range,&start,&stop);
  start.red*=(double) QuantumRange;
  start.green*=(double) QuantumRange;
  start.blue*=(double) QuantumRange;
  stop.red*=(double) QuantumRange;
  stop.green*=(double) QuantumRange;
  stop.blue*=(double) QuantumRange;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    ColorThresholdRow(image,&start,&stop,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ThresholdImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  image->colorspace=sRGBColorspace;
  return(SyncImage(image,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  D e s t r o y T h r e s h o l d M a p                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyThresholdMap() de-allocate the given ThresholdMap
%
%  The format of the ListThresholdMaps method is:
%
%      ThresholdMap *DestroyThresholdMap(Threshold *map)
%
%  A description of each parameter follows.
%
%    o map:    Pointer to the Threshold map to destroy
%
*/
MagickExport ThresholdMap *DestroyThresholdMap(ThresholdMap *map)
{
  assert(map != (ThresholdMap *) NULL);
  if (map->map_id != (char *) NULL)
    map->map_id=DestroyString(map->map_id);
  if (map->description != (char *) NULL)
    map->description=DestroyString(map->description);
  if (map->levels != (ssize_t *) NULL)
    map->levels=(ssize_t *) RelinquishMagickMemory(map->levels);
  map=(ThresholdMap *) RelinquishMagickMemory(map);
  return(map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t T h r e s h o l d M a p                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetThresholdMap() loads and searches one or more threshold map files for the
%  map matching the given name or alias.
%
%  The format of the GetThresholdMap method is:
%
%      ThresholdMap *GetThresholdMap(const char *map_id,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o map_id:  ID of the map to look for.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport ThresholdMap *GetThresholdMap(const char *map_id,
  ExceptionInfo *exception)
{
  ThresholdMap
    *map;

  map=GetThresholdMapFile(BuiltinMap,"built-in",map_id,exception);
  if (map != (ThresholdMap *) NULL)
    return(map);
#if !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT
  {
    const StringInfo
      *option;

    LinkedListInfo
      *options;

    options=GetConfigureOptions(ThresholdsFilename,exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
    while (option != (const StringInfo *) NULL)
    {
      map=GetThresholdMapFile((const char *) GetStringInfoDatum(option),
        GetStringInfoPath(option),map_id,exception);
      if (map != (ThresholdMap *) NULL)
        break;
      option=(const StringInfo *) GetNextValueInLinkedList(options);
    }
    options=DestroyConfigureOptions(options);
  }
#endif
  return(map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  G e t T h r e s h o l d M a p F i l e                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetThresholdMapFile() look for a given threshold map name or alias in the
%  given XML file data, and return the allocated the map when found.
%
%  The format of the ListThresholdMaps method is:
%
%      ThresholdMap *GetThresholdMap(const char *xml,const char *filename,
%         const char *map_id,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o xml:  The threshold map list in XML format.
%
%    o filename:  The threshold map XML filename.
%
%    o map_id:  ID of the map to look for in XML list.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ThresholdMap *AcquireThresholdMap(XMLTreeInfo *threshold,
  XMLTreeInfo *description)
{
  const char
    *attribute,
    *content;

  ThresholdMap
    *map;

  map=(ThresholdMap *) AcquireCriticalMemory(sizeof(*map));
  map->map_id=(char *) NULL;
  map->description=(char *) NULL;
  map->levels=(ssize_t *) NULL;
  attribute=GetXMLTreeAttribute(threshold,"map");
  if (attribute != (char *) NULL)
    map->map_id=ConstantString(attribute);
  content=GetXMLTreeContent(description);
  if (content != (char *) NULL)
    map->description=ConstantString(content);
  return(map);
}

static XMLTreeInfo *FindThresholdMap(XMLTreeInfo *thresholds,
  const char *map_id)
{
  const char
    *attribute;

  XMLTreeInfo
    *threshold;

  for (threshold=GetXMLTreeChild(thresholds,"threshold");
       threshold != (XMLTreeInfo *) NULL;
       threshold=GetNextXMLTreeTag(threshold))
  {
    attribute=GetXMLTreeAttribute(threshold,"map");
    if ((attribute != (char *) NULL) && (LocaleCompare(map_id,attribute) == 0))
      break;
    attribute=GetXMLTreeAttribute(threshold,"alias");
    if ((attribute != (char *) NULL) && (LocaleCompare(map_id,attribute) == 0))
      break;
  }
  return(threshold);
}

static MagickBooleanType ReadThresholdLevels(ThresholdMap *map,
  XMLTreeInfo *levels,const char *map_id,ExceptionInfo *exception)
{
  char
    *p;

  const char
    *content;

  double
    value;

  ssize_t
    i;

  content=GetXMLTreeContent(levels);
  if (content == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingContent", "<levels>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  map->levels=(ssize_t *) AcquireQuantumMemory((size_t) map->width,map->height*
    sizeof(*map->levels));
  if (map->levels == (ssize_t *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireThresholdMap");
  for (i=0; i < (ssize_t) (map->width*map->height); i++)
  {
    map->levels[i]=(ssize_t) strtol(content,&p,10);
    if (p == content)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlInvalidContent", "<level> too few values, map \"%s\"",map_id);
        return(MagickFalse);
      }
    if ((map->levels[i] < 0) || (map->levels[i] > map->divisor))
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlInvalidContent", "<level> %.17g out of range, map \"%s\"",
          (double) map->levels[i],map_id);
        return(MagickFalse);
      }
    content=p;
  }
  value=(double) strtol(content,&p,10);
  (void) value;
  if (p != content)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidContent", "<level> too many values, map \"%s\"",map_id);
     return(MagickFalse);
   }
  return(MagickTrue);
}

static MagickBooleanType ReadThresholdLevelsSize(ThresholdMap *map,
  XMLTreeInfo *levels,const char *map_id,ExceptionInfo *exception)
{
  const char
    *attribute;

  attribute=GetXMLTreeAttribute(levels,"width");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute", "<levels width>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  map->width=StringToUnsignedLong(attribute);
  if (map->width == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
       "XmlInvalidAttribute", "<levels width>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  attribute=GetXMLTreeAttribute(levels,"height");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute", "<levels height>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  map->height=StringToUnsignedLong(attribute);
  if (map->height == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidAttribute", "<levels height>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  attribute=GetXMLTreeAttribute(levels,"divisor");
  if (attribute == (char *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingAttribute", "<levels divisor>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  map->divisor=(ssize_t) StringToLong(attribute);
  if (map->divisor < 2)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlInvalidAttribute", "<levels divisor>, map \"%s\"",map_id);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static ThresholdMap *GetThresholdMapFile(const char *xml,const char *filename,
  const char *map_id,ExceptionInfo *exception)
{
  ThresholdMap
    *map;

  XMLTreeInfo
    *description,
    *levels,
    *threshold,
    *thresholds;

  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading threshold map file \"%s\" ...",filename);
  map=(ThresholdMap *) NULL;
  thresholds=NewXMLTree(xml,exception);
  if (thresholds == (XMLTreeInfo *) NULL)
    return(map);
  threshold=FindThresholdMap(thresholds,map_id);
  if (threshold == (XMLTreeInfo *) NULL)
    {
      thresholds=DestroyXMLTree(thresholds);
      return(map);
    }
  description=GetXMLTreeChild(threshold,"description");
  if (description == (XMLTreeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingElement", "<description>, map \"%s\"",map_id);
      thresholds=DestroyXMLTree(thresholds);
      return(map);
    }
  levels=GetXMLTreeChild(threshold,"levels");
  if (levels == (XMLTreeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "XmlMissingElement", "<levels>, map \"%s\"", map_id);
      thresholds=DestroyXMLTree(thresholds);
      return(map);
    }
  map=AcquireThresholdMap(threshold,description);
  if (ReadThresholdLevelsSize(map,levels,map_id,exception) == MagickFalse)
    {
      thresholds=DestroyXMLTree(thresholds);
      map=DestroyThresholdMap(map);
      return(map);
    }
  if (ReadThresholdLevels(map,levels,map_id,exception) == MagickFalse)
    {
      thresholds=DestroyXMLTree(thresholds);
      map=DestroyThresholdMap(map);
      return(map);
    }
  thresholds=DestroyXMLTree(thresholds);
  return(map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  L i s t T h r e s h o l d M a p F i l e                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListThresholdMapFile() lists the threshold maps and their descriptions
%  in the given XML file data.
%
%  The format of the ListThresholdMaps method is:
%
%      MagickBooleanType ListThresholdMaps(FILE *file,const char*xml,
%         const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to the output FILE.
%
%    o xml:  The threshold map list in XML format.
%
%    o filename:  The threshold map XML filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickBooleanType ListThresholdMapFile(FILE *file,const char *xml,
  const char *filename,ExceptionInfo *exception)
{
  const char
    *alias,
    *content,
    *map;

  XMLTreeInfo
    *description,
    *threshold,
    *thresholds;

  assert( xml != (char *) NULL );
  assert( file != (FILE *) NULL );
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading threshold map file \"%s\" ...",filename);
  thresholds=NewXMLTree(xml,exception);
  if ( thresholds == (XMLTreeInfo *) NULL )
    return(MagickFalse);
  (void) FormatLocaleFile(file,"%-16s %-12s %s\n","Map","Alias","Description");
  (void) FormatLocaleFile(file,
    "----------------------------------------------------\n");
  threshold=GetXMLTreeChild(thresholds,"threshold");
  for ( ; threshold != (XMLTreeInfo *) NULL;
          threshold=GetNextXMLTreeTag(threshold))
  {
    map=GetXMLTreeAttribute(threshold,"map");
    if (map == (char *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlMissingAttribute", "<map>");
        thresholds=DestroyXMLTree(thresholds);
        return(MagickFalse);
      }
    alias=GetXMLTreeAttribute(threshold,"alias");
    description=GetXMLTreeChild(threshold,"description");
    if (description == (XMLTreeInfo *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlMissingElement", "<description>, map \"%s\"",map);
        thresholds=DestroyXMLTree(thresholds);
        return(MagickFalse);
      }
    content=GetXMLTreeContent(description);
    if (content == (char *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "XmlMissingContent", "<description>, map \"%s\"", map);
        thresholds=DestroyXMLTree(thresholds);
        return(MagickFalse);
      }
    (void) FormatLocaleFile(file,"%-16s %-12s %s\n",map,alias ? alias : "",
      content);
  }
  thresholds=DestroyXMLTree(thresholds);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t T h r e s h o l d M a p s                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListThresholdMaps() lists the threshold maps and their descriptions
%  as defined by "threshold.xml" to a file.
%
%  The format of the ListThresholdMaps method is:
%
%      MagickBooleanType ListThresholdMaps(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to the output FILE.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListThresholdMaps(FILE *file,
  ExceptionInfo *exception)
{
  const StringInfo
    *option;

  LinkedListInfo
    *options;

  MagickStatusType
    status;

  status=MagickTrue;
  if (file == (FILE *) NULL)
    file=stdout;
  options=GetConfigureOptions(ThresholdsFilename,exception);
  (void) FormatLocaleFile(file,
    "\n   Threshold Maps for Ordered Dither Operations\n");
  option=(const StringInfo *) GetNextValueInLinkedList(options);
  while (option != (const StringInfo *) NULL)
  {
    (void) FormatLocaleFile(file,"\nPath: %s\n\n",GetStringInfoPath(option));
    status&=(MagickStatusType) ListThresholdMapFile(file,(const char *)
      GetStringInfoDatum(option),GetStringInfoPath(option),exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyConfigureOptions(options);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O r d e r e d D i t h e r I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OrderedDitherImage() will perform a ordered dither based on a number
%  of pre-defined dithering threshold maps, but over multiple intensity
%  levels, which can be different for different channels, according to the
%  input argument.
%
%  The format of the OrderedDitherImage method is:
%
%      MagickBooleanType OrderedDitherImage(Image *image,
%        const char *threshold_map,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold_map: A string containing the name of the threshold dither
%      map to use, followed by zero or more numbers representing the number
%      of color levels to dither between.
%
%      Any level number less than 2 will be equivalent to 2, and means only
%      binary dithering will be applied to each color channel.
%
%      No numbers also means a 2 level (bitmap) dither will be applied to all
%      channels, while a single number is the number of levels applied to each
%      channel in sequence.  More numbers will be applied in turn to each of
%      the color channels.
%
%      For example: "o3x3,6" will generate a 6 level posterization of the
%      image with an ordered 3x3 diffused pixel dither being applied between
%      each level. While checker,8,8,4 will produce a 332 colormaped image
%      with only a single checkerboard hash pattern (50% grey) between each
%      color level, to basically double the number of color levels with
%      a bare minimum of dithering.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsThresholdMapSeparator(const char *p)
{
  return((((isspace((int) ((unsigned char) *p)) != 0) || (*p == ',')) &&
    (*p != '\0')) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsThresholdMapNameCharacter(const char *p)
{
  return((((isspace((int) ((unsigned char) *p)) == 0) && (*p != ',')) &&
    (*p != '\0')) ? MagickTrue : MagickFalse);
}

static void ParseDitherLevels(const char *p,double *levels)
{
  char
    token[MagickPathExtent];

  ssize_t
    i;

  (void) GetNextToken(p,&p,MagickPathExtent,token);
  for (i=0; (i < MaxPixelChannels); i++)
    levels[i]=StringToDouble(token,(char **) NULL);
  for (i=0; (*p != '\0') && (i < MaxPixelChannels); i++)
  {
    (void) GetNextToken(p,&p,MagickPathExtent,token);
    if (*token == ',')
      (void) GetNextToken(p,&p,MagickPathExtent,token);
    levels[i]=StringToDouble(token,(char **) NULL);
  }
}

static void GetDitherLevels(const char *threshold_map,double *levels)
{
  const char
    *p;

  ssize_t
    i;

  for (i=0; i < MaxPixelChannels; i++)
    levels[i]=2.0;
  p=strchr((char *) threshold_map,',');
  if ((p != (char *) NULL) && (isdigit((int) ((unsigned char) *(++p))) != 0))
    ParseDitherLevels(p,levels);
  for (i=0; i < MaxPixelChannels; i++)
    if (fabs(levels[i]) >= 1)
      levels[i]-=1.0;
}

typedef struct _OrderedDither
{
  ThresholdMap
    *map;

  ssize_t
    x,
    y;
} OrderedDither;

static void OrderedDitherPixel(const Image *image,const double *levels,
  const OrderedDither *ordered_dither,Quantum *magick_restrict q)
{
  ThresholdMap
    *const map = ordered_dither->map;

  const ssize_t
    x = ordered_dither->x,
    y = ordered_dither->y;

  ssize_t
    j,
    n;

  n=0;
  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
  {
    ssize_t
      level,
      threshold;

    PixelChannel channel = GetPixelChannelChannel(image,j);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (fabs(levels[n]) < MagickEpsilon)
      {
        n++;
        continue;
      }
    threshold=(ssize_t) (QuantumScale*(double) q[j]*(levels[n]*
      (map->divisor-1)+1));
    level=threshold/(map->divisor-1);
    threshold-=level*(map->divisor-1);
    q[j]=ClampToQuantum((double) (level+(threshold >=
      map->levels[(x % (ssize_t) map->width)+(ssize_t) map->width*
      (y % (ssize_t) map->height)]))*(double) QuantumRange/levels[n]);
    n++;
  }
}

static const char *CopyThresholdMapName(const char *threshold_map,
  const char *p,char *token)
{
  while (IsThresholdMapNameCharacter(p) != MagickFalse)
  {
    if ((p-threshold_map) >= (MagickPathExtent-1))
      break;
    token[p-threshold_map]=(*p);
    p++;
  }
  return(p);
}

static ThresholdMap *GetNamedThresholdMap(const char *threshold_map,
  const char *p,ExceptionInfo *exception)
{
  char
    token[MagickPathExtent];

  p=CopyThresholdMapName(threshold_map,p,token);
  token[p-threshold_map]='\0';
  return(GetThresholdMap(token,exception));
}

MagickExport MagickBooleanType OrderedDitherImage(Image *image,
  const char *threshold_map,ExceptionInfo *exception)
{
#define DitherImageTag  "Dither/Image"

  CacheView
    *image_view;

  const char
    *p;

  double
    levels[CompositePixelChannel];

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  ThresholdMap
    *map;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (threshold_map == (const char *) NULL)
    return(MagickTrue);
  p=(char *) threshold_map;
  while (IsThresholdMapSeparator(p) != MagickFalse)
    p++;
  threshold_map=p;
  map=GetNamedThresholdMap(threshold_map,p,exception);
  if (map == (ThresholdMap *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : '%s'","ordered-dither",threshold_map);
      return(MagickFalse);
    }
  GetDitherLevels(threshold_map,levels);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    OrderedDither
      ordered_dither;

    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      ordered_dither.map=map;
      ordered_dither.x=x;
      ordered_dither.y=y;
      OrderedDitherPixel(image,levels,&ordered_dither,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,DitherImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  map=DestroyThresholdMap(map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     P e r c e p t i b l e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PerceptibleImage() set each pixel whose value is less than |epsilon| to
%  epsilon or -epsilon (whichever is closer) otherwise the pixel value remains
%  unchanged.
%
%  The format of the PerceptibleImage method is:
%
%      MagickBooleanType PerceptibleImage(Image *image,const double epsilon,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o epsilon: the epsilon threshold (e.g. 1.0e-9).
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline Quantum PerceptibleThreshold(const Quantum quantum,
  const double epsilon)
{
  double
    sign;

  sign=(double) quantum < 0.0 ? -1.0 : 1.0;
  if ((sign*(double) quantum) >= epsilon)
    return(quantum);
  return((Quantum) (sign*epsilon));
}

static void PerceptiblePixel(const Image *image,const double epsilon,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) != 0)
      q[i]=PerceptibleThreshold(q[i],epsilon);
  }
}

static MagickBooleanType PerceptibleImageColormap(Image *image,
  const double epsilon,ExceptionInfo *exception)
{
  ssize_t
    i;

  PixelInfo
    *magick_restrict q;

  q=image->colormap;
  for (i=0; i < (ssize_t) image->colors; i++)
  {
    if ((GetPixelChannelTraits(image,RedPixelChannel) & UpdatePixelTrait) != 0)
      q->red=(MagickRealType) PerceptibleThreshold(ClampToQuantum(q->red),
        epsilon);
    if ((GetPixelChannelTraits(image,GreenPixelChannel) & UpdatePixelTrait) != 0)
      q->green=(MagickRealType) PerceptibleThreshold(ClampToQuantum(q->green),
        epsilon);
    if ((GetPixelChannelTraits(image,BluePixelChannel) & UpdatePixelTrait) != 0)
      q->blue=(MagickRealType) PerceptibleThreshold(ClampToQuantum(q->blue),
        epsilon);
    if ((GetPixelChannelTraits(image,AlphaPixelChannel) & UpdatePixelTrait) != 0)
      q->alpha=(MagickRealType) PerceptibleThreshold(ClampToQuantum(q->alpha),
        epsilon);
    q++;
  }
  return(SyncImage(image,exception));
}

MagickExport MagickBooleanType PerceptibleImage(Image *image,
  const double epsilon,ExceptionInfo *exception)
{
#define PerceptibleImageTag  "Perceptible/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    return(PerceptibleImageColormap(image,epsilon,exception));
  /*
    Perceptible image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      PerceptiblePixel(image,epsilon,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,PerceptibleImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
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
%     R a n d o m T h r e s h o l d I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RandomThresholdImage() changes the value of individual pixels based on the
%  intensity of each pixel compared to a random threshold.  The result is a
%  low-contrast, two color image.
%
%  The format of the RandomThresholdImage method is:
%
%      MagickBooleanType RandomThresholdImage(Image *image,
%        const char *thresholds,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o low,high: Specify the high and low thresholds. These values range from
%      0 to QuantumRange.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _RandomThreshold
{
  double
    min_threshold,
    max_threshold;

  RandomInfo
    **random_info;

  int
    id;
} RandomThreshold;

static void RandomThresholdPixel(const Image *image,
  const RandomThreshold *random_threshold,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      threshold;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if ((double) q[i] < random_threshold->min_threshold)
      threshold=random_threshold->min_threshold;
    else
      if ((double) q[i] > random_threshold->max_threshold)
        threshold=random_threshold->max_threshold;
      else
        threshold=((double) QuantumRange*GetPseudoRandomValue(
          random_threshold->random_info[random_threshold->id]));
    q[i]=(double) q[i] <= threshold ? 0 : QuantumRange;
  }
}

MagickExport MagickBooleanType RandomThresholdImage(Image *image,
  const double min_threshold, const double max_threshold,ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  RandomInfo
    **magick_restrict random_info;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  unsigned long
    key;
#endif

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Random threshold image.
  */
  status=MagickTrue;
  progress=0;
  random_info=AcquireRandomInfoTLS();
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  key=GetRandomSecretKey(random_info[0]);
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,key == ~0UL ? 0 : 1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    Quantum
      *magick_restrict q;

    RandomThreshold
      random_threshold;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      random_threshold.min_threshold=min_threshold;
      random_threshold.max_threshold=max_threshold;
      random_threshold.random_info=random_info;
      random_threshold.id=id;
      RandomThresholdPixel(image,&random_threshold,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ThresholdImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  random_info=DestroyRandomInfoTLS(random_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R a n g e T h r e s h o l d I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RangeThresholdImage() applies soft and hard thresholding.
%
%  The format of the RangeThresholdImage method is:
%
%      MagickBooleanType RangeThresholdImage(Image *image,
%        const double low_black,const double low_white,const double high_white,
%        const double high_black,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o low_black: Define the minimum black threshold value.
%
%    o low_white: Define the minimum white threshold value.
%
%    o high_white: Define the maximum white threshold value.
%
%    o high_black: Define the maximum black threshold value.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType SetDirectRGBImage(Image *image,
  ExceptionInfo *exception)
{
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) TransformImageColorspace(image,sRGBColorspace,exception);
  return(MagickTrue);
}

static inline MagickBooleanType IsRangeThresholdWhite(const double pixel,
  const double low_white,const double high_white)
{
  return(((pixel >= low_white) && (pixel <= high_white)) ? MagickTrue :
    MagickFalse);
}

static inline Quantum RangeThresholdValue(const double pixel,
  const double low_black,const double low_white,const double high_white,
  const double high_black)
{
  if (pixel < low_black)
    return((Quantum) 0);
  else
    if ((pixel >= low_black) && (pixel < low_white))
      return(ClampToQuantum((double) QuantumRange*
        MagickSafeReciprocal(low_white-low_black)*(pixel-low_black)));
    else
      if (IsRangeThresholdWhite(pixel,low_white,high_white) != MagickFalse)
        return(QuantumRange);
      else
        if ((pixel > high_white) && (pixel <= high_black))
          return(ClampToQuantum((double) QuantumRange*(double)
            MagickSafeReciprocal(high_black-high_white)*
            (high_black-pixel)));
        else
          if (pixel > high_black)
            return((Quantum) 0);
          else
            return((Quantum) 0);
}

static inline void RangeThresholdChannels(const Image *image,
  const double low_black,const double low_white,const double high_white,
  const double high_black,Quantum *magick_restrict q)
{
  double
    pixel;

  ssize_t
    i;

  pixel=GetPixelIntensity(image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (image->channel_mask != AllChannels)
      pixel=(double) q[i];
    q[i]=RangeThresholdValue(pixel,low_black,low_white,high_white,
      high_black);
  }
}

MagickExport MagickBooleanType RangeThresholdImage(Image *image,
  const double low_black,const double low_white,const double high_white,
  const double high_black,ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetDirectRGBImage(image,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Range threshold image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      RangeThresholdChannels(image,low_black,low_white,high_white,high_black,
        q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ThresholdImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
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
%     W h i t e T h r e s h o l d I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WhiteThresholdImage() is like ThresholdImage() but forces all pixels above
%  the threshold into white while leaving all pixels at or below the threshold
%  unchanged.
%
%  The format of the WhiteThresholdImage method is:
%
%      MagickBooleanType WhiteThresholdImage(Image *image,
%        const char *threshold,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold: Define the threshold value.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void WhiteThresholdPixel(const Image *image,const PixelInfo *threshold,
  double pixel,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if (image->channel_mask != AllChannels)
      pixel=(double) q[i];
    if (pixel > GetPixelInfoChannel(threshold,channel))
      q[i]=QuantumRange;
  }
}

static void WhiteThresholdIntensity(const Image *image,
  const PixelInfo *threshold,Quantum *magick_restrict q)
{
  double
    pixel;

  pixel=GetPixelIntensity(image,q);
  WhiteThresholdPixel(image,threshold,pixel,q);
}

MagickExport MagickBooleanType WhiteThresholdImage(Image *image,
  const char *thresholds,ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    threshold;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (thresholds == (const char *) NULL)
    return(MagickTrue);
  if (SetDirectRGBImage(image,exception) == MagickFalse)
    return(MagickFalse);
  threshold=GetThresholdLevels(image,thresholds);
  /*
    White threshold image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      x;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      WhiteThresholdIntensity(image,&threshold,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ThresholdImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
