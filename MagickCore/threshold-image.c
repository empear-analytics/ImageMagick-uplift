/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        TTTTT  H   H  RRRR   EEEEE  SSSSS  H   H   OOO   L      DDDD         %
%          T    H   H  R   R  E      SS     H   H  O   O  L      D   D        %
%          T    HHHHH  RRRR   EEE     SSS   HHHHH  O   O  L      D   D        %
%          T    H   H  R R    E         SS  H   H  O   O  L      D   D        %
%          T    H   H  R  R   EEEEE  SSSSS  H   H   OOO   LLLLL  DDDD         %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                     MagickCore Image Threshold Methods                      %
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
#include "MagickCore/threshold-private.h"

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

    const ThresholdLine
      line = { image,image_view,exception };

    if (GetThresholdRow(&line,y,&q,&status) == MagickFalse)
      continue;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        pixel;

      pixel=GetPixelIntensity(image,q);
      BilevelPixel(image,threshold,pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    SyncThresholdRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetThresholdProgress(image,ThresholdImageTag,progress++,&status);
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

static void ClampPixelRow(const Image *image,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ClampPixelChannels(image,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType ClampImageLine(const ThresholdLine *line,
  const ssize_t y,MagickBooleanType *status)
{
  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(line->image_view,0,y,line->image->columns,1,
    line->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ClampPixelRow(line->image,q);
  if (SyncCacheViewAuthenticPixels(line->image_view,line->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
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
    ThresholdLine
      line;

    line.image=image;
    line.image_view=image_view;
    line.exception=exception;
    if (ClampImageLine(&line,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetThresholdProgress(image,ClampImageTag,progress,&status);
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

static void ScaleColorThresholdLimit(PixelInfo *limit)
{
  (*limit).red*=(double) QuantumRange;
  (*limit).green*=(double) QuantumRange;
  (*limit).blue*=(double) QuantumRange;
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
  ScaleColorThresholdLimit(&start);
  ScaleColorThresholdLimit(&stop);
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

    const ThresholdLine
      line = { image,image_view,exception };

    if (GetThresholdRow(&line,y,&q,&status) == MagickFalse)
      continue;
    ColorThresholdRow(image,&start,&stop,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetThresholdProgress(image,ThresholdImageTag,progress,&status);
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

static void FillDitherLevels(const char *token,double *levels)
{
  ssize_t
    i;

  for (i=0; (i < MaxPixelChannels); i++)
    levels[i]=StringToDouble(token,(char **) NULL);
}

static const char *ParseDitherLevel(const char *p,char *token,double *levels,
  const ssize_t i)
{
  (void) GetNextToken(p,&p,MagickPathExtent,token);
  if (*token == ',')
    (void) GetNextToken(p,&p,MagickPathExtent,token);
  levels[i]=StringToDouble(token,(char **) NULL);
  return(p);
}

static void ParseDitherLevels(const char *p,double *levels)
{
  char
    token[MagickPathExtent];

  ssize_t
    i;

  (void) GetNextToken(p,&p,MagickPathExtent,token);
  FillDitherLevels(token,levels);
  for (i=0; (*p != '\0') && (i < MaxPixelChannels); i++)
    p=ParseDitherLevel(p,token,levels,i);
}

static void DecrementDitherLevels(double *levels)
{
  ssize_t
    i;

  for (i=0; i < MaxPixelChannels; i++)
    if (fabs(levels[i]) >= 1)
      levels[i]-=1.0;
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
  DecrementDitherLevels(levels);
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

static MagickBooleanType CheckOrderedDitherMap(const ThresholdMap *map,
  const char *threshold_map,ExceptionInfo *exception)
{
  if (map == (ThresholdMap *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : '%s'","ordered-dither",threshold_map);
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _OrderedDitherRow
{
  ThresholdMap
    *map;

  ssize_t
    y;
} OrderedDitherRow;

static void OrderedDitherPixels(const Image *image,const double *levels,
  const OrderedDitherRow *row,Quantum *magick_restrict q)
{
  ThresholdMap
    *const map = row->map;

  const ssize_t
    y = row->y;

  OrderedDither
    ordered_dither;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ordered_dither.map=map;
    ordered_dither.x=x;
    ordered_dither.y=y;
    OrderedDitherPixel(image,levels,&ordered_dither,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static const char *SkipThresholdMapSeparators(const char *threshold_map)
{
  const char
    *p;

  p=(char *) threshold_map;
  while (IsThresholdMapSeparator(p) != MagickFalse)
    p++;
  return(p);
}

typedef struct _OrderedDitherMap
{
  ThresholdMap
    *map;

  const char
    *threshold_map;
} OrderedDitherMap;

static MagickBooleanType PrepareOrderedDither(const OrderedDitherMap *input,
  double *levels,Image *image,ExceptionInfo *exception)
{
  ThresholdMap
    *const map = input->map;

  const char
    *const threshold_map = input->threshold_map;

  if (CheckOrderedDitherMap(map,threshold_map,exception) == MagickFalse)
    return(MagickFalse);
  GetDitherLevels(threshold_map,levels);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
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

  OrderedDitherMap
    input;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (threshold_map == (const char *) NULL)
    return(MagickTrue);
  p=SkipThresholdMapSeparators(threshold_map);
  threshold_map=p;
  map=GetNamedThresholdMap(threshold_map,p,exception);
  input.map=map;
  input.threshold_map=threshold_map;
  if (PrepareOrderedDither(&input,levels,image,exception) == MagickFalse)
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
    OrderedDitherRow
      row;

    Quantum
      *magick_restrict q;

    const ThresholdLine
      line = { image,image_view,exception };

    if (GetThresholdRow(&line,y,&q,&status) == MagickFalse)
      continue;
    row.map=map;
    row.y=y;
    OrderedDitherPixels(image,levels,&row,q);
    SyncThresholdRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetThresholdProgress(image,DitherImageTag,progress,&status);
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

static double GetRandomPixelThreshold(
  const RandomThreshold *random_threshold,const Quantum *magick_restrict q,
  const ssize_t i)
{
  double
    threshold;

  if ((double) q[i] < random_threshold->min_threshold)
    threshold=random_threshold->min_threshold;
  else
    if ((double) q[i] > random_threshold->max_threshold)
      threshold=random_threshold->max_threshold;
    else
      threshold=((double) QuantumRange*GetPseudoRandomValue(
        random_threshold->random_info[random_threshold->id]));
  return(threshold);
}

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
    threshold=GetRandomPixelThreshold(random_threshold,q,i);
    q[i]=(double) q[i] <= threshold ? 0 : QuantumRange;
  }
}

typedef struct _RandomThresholdRow
{
  double
    min_threshold,
    max_threshold;

  RandomInfo
    **magick_restrict random_info;

  int
    id;
} RandomThresholdRow;

static void RandomThresholdPixels(const Image *image,
  const RandomThresholdRow *row,Quantum *magick_restrict q)
{
  const double
    min_threshold = row->min_threshold,
    max_threshold = row->max_threshold;

  RandomInfo
    **magick_restrict const random_info = row->random_info;

  const int
    id = row->id;

  RandomThreshold
    random_threshold;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    random_threshold.min_threshold=min_threshold;
    random_threshold.max_threshold=max_threshold;
    random_threshold.random_info=random_info;
    random_threshold.id=id;
    RandomThresholdPixel(image,&random_threshold,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
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

    RandomThresholdRow
      row;

    const ThresholdLine
      line = { image,image_view,exception };

    if (GetThresholdRow(&line,y,&q,&status) == MagickFalse)
      continue;
    row.min_threshold=min_threshold;
    row.max_threshold=max_threshold;
    row.random_info=random_info;
    row.id=id;
    RandomThresholdPixels(image,&row,q);
    SyncThresholdRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetThresholdProgress(image,ThresholdImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  random_info=DestroyRandomInfoTLS(random_info);
  return(status);
}

