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
%                      L      EEEEE  V   V  EEEEE  L                          %
%                      L      E      V   V  E      L                          %
%                      L      EEE    V   V  EEE    L                          %
%                      L      E       V V   E      L                          %
%                      LLLLL  EEEEE    V    EEEEE  LLLLL                      %
%                                                                             %
%                                                                             %
%                     MagickCore Image Tone Level Methods                     %
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
#include "MagickCore/enhance-private.h"
#include "MagickCore/enhance-level-private.h"

static inline MagickBooleanType IsStretchMemoryMissing(const Quantum *black,
  const Quantum *white,const Quantum *stretch_map,const double *histogram)
{
  return(((black == (Quantum *) NULL) || (white == (Quantum *) NULL) ||
    (stretch_map == (Quantum *) NULL) || (histogram == (double *) NULL)) ?
    MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n t r a s t S t r e t c h I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ContrastStretchImage() is a simple image enhancement technique that attempts
%  to improve the contrast in an image by 'stretching' the range of intensity
%  values it contains to span a desired range of values. It differs from the
%  more sophisticated histogram equalization in that it can only apply a
%  linear scaling function to the image pixel values.  As a result the
%  'enhancement' is less harsh.
%
%  The format of the ContrastStretchImage method is:
%
%      MagickBooleanType ContrastStretchImage(Image *image,
%        const char *levels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_point: the black point.
%
%    o white_point: the white point.
%
%    o levels: Specify the levels where the black and white points have the
%      range of 0 to number-of-pixels (e.g. 1%, 10x90%, etc.).
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void AccumulateStretchHistogram(const Image *image,
  const Quantum *p,double *histogram)
{
  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      pixel;

    pixel=GetPixelIntensity(image,p);
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      if (image->channel_mask != AllChannels)
        pixel=(double) p[i];
      histogram[GetPixelChannels(image)*ScaleQuantumToMap(
        ClampToQuantum(pixel))+(size_t) i]++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void BuildStretchMap(const Image *image,const Quantum *black,
  const Quantum *white,Quantum *stretch_map)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ssize_t
      j;

    for (j=0; j <= (ssize_t) MaxMap; j++)
    {
      double
        gamma;

      gamma=MagickSafeReciprocal(white[i]-black[i]);
      if (j < (ssize_t) black[i])
        stretch_map[(ssize_t) GetPixelChannels(image)*j+i]=(Quantum) 0;
      else
        if (j > (ssize_t) white[i])
          stretch_map[(ssize_t) GetPixelChannels(image)*j+i]=QuantumRange;
        else
          if (black[i] != white[i])
            stretch_map[(ssize_t) GetPixelChannels(image)*j+i]=
              ScaleMapToQuantum((double) (MaxMap*gamma*(j-(double) black[i])));
    }
  }
}

static void StretchColormap(Image *image,const Quantum *stretch_map)
{
  ssize_t
    i,
    j;

  for (j=0; j < (ssize_t) image->colors; j++)
  {
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      {
        i=GetPixelChannelOffset(image,RedPixelChannel);
        image->colormap[j].red=(MagickRealType) stretch_map[
          GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(
          image->colormap[j].red))+(size_t) i];
      }
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      {
        i=GetPixelChannelOffset(image,GreenPixelChannel);
        image->colormap[j].green=(MagickRealType) stretch_map[
          GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(
          image->colormap[j].green))+(size_t) i];
      }
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      {
        i=GetPixelChannelOffset(image,BluePixelChannel);
        image->colormap[j].blue=(MagickRealType) stretch_map[
          GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(
          image->colormap[j].blue))+(size_t) i];
      }
    if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
      {
        i=GetPixelChannelOffset(image,AlphaPixelChannel);
        image->colormap[j].alpha=(MagickRealType) stretch_map[
          GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(
          image->colormap[j].alpha))+(size_t) i];
      }
  }
}

typedef struct _StretchMaps
{
  const Quantum
    *black,
    *white,
    *stretch_map;
} StretchMaps;

static void StretchImageRow(const Image *image,const void *arguments,
  Quantum *q)
{
  const StretchMaps
    *maps = (const StretchMaps *) arguments;

  const Quantum
    *black = maps->black,
    *white = maps->white,
    *stretch_map = maps->stretch_map;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      j;

    for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,j);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      if (black[j] == white[j])
        continue;
      q[j]=ClampToQuantum(stretch_map[GetPixelChannels(image)*
        ScaleQuantumToMap(q[j])+(size_t) j]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef enum
{
  BlackStretchLevel,
  WhiteStretchLevel
} StretchLevelSide;

typedef struct _StretchLevelSearch
{
  StretchLevelSide
    side;

  double
    point;
} StretchLevelSearch;

static inline ssize_t FindStretchLevel(const Image *image,
  const double *histogram,const ssize_t i,const StretchLevelSearch *search)
{
  const StretchLevelSide
    side = search->side;

  const double
    point = search->point;

  double
    intensity;

  ssize_t
    j;

  intensity=0.0;
  for (j=((side == BlackStretchLevel) ? 0 : (ssize_t) MaxMap);
       ((side == BlackStretchLevel) ? j <= (ssize_t) MaxMap : j != 0);
       ((side == BlackStretchLevel) ? j++ : j--))
  {
    intensity+=histogram[(ssize_t) GetPixelChannels(image)*j+i];
    if (intensity > ((side == BlackStretchLevel) ? point :
        ((double) image->columns*image->rows-point)))
      break;
  }
  return(j);
}

static ssize_t FindStretchBlackLevel(const Image *image,
  const double *histogram,const ssize_t i,const double black_point)
{
  const StretchLevelSearch
    search = { BlackStretchLevel,black_point };

  return(FindStretchLevel(image,histogram,i,&search));
}

static ssize_t FindStretchWhiteLevel(const Image *image,
  const double *histogram,const ssize_t i,const double white_point)
{
  const StretchLevelSearch
    search = { WhiteStretchLevel,white_point };

  return(FindStretchLevel(image,histogram,i,&search));
}

static void RelinquishStretchMemory(double *histogram,Quantum *stretch_map,
  Quantum *white,Quantum *black)
{
  if (histogram != (double *) NULL)
    histogram=(double *) RelinquishMagickMemory(histogram);
  if (stretch_map != (Quantum *) NULL)
    stretch_map=(Quantum *) RelinquishMagickMemory(stretch_map);
  if (white != (Quantum *) NULL)
    white=(Quantum *) RelinquishMagickMemory(white);
  if (black != (Quantum *) NULL)
    black=(Quantum *) RelinquishMagickMemory(black);
}

static void SetStretchGrayColorspace(Image *image,ExceptionInfo *exception)
{
  ImageType
    type;

  type=IdentifyImageType(image,exception);
  if (IsGrayImageType(type) != MagickFalse)
    (void) SetImageColorspace(image,GRAYColorspace,exception);
}

static void SetStretchProperty(Image *image,const Quantum *black,
  const Quantum *white,ExceptionInfo *exception)
{
  char
    property[MagickPathExtent];

  (void) FormatLocaleString(property,MagickPathExtent,"%gx%g%%",100.0*
    QuantumScale*GetPixelIntensity(image,black),100.0*QuantumScale*
    GetPixelIntensity(image,white));
  (void) SetImageProperty(image,"histogram:contrast-stretch",property,
    exception);
}

typedef struct _StretchScan
{
  Image
    *image;

  CacheView
    *image_view;

  double
    *histogram;

  ExceptionInfo
    *exception;
} StretchScan;

static void ScanStretchHistogramRow(const StretchScan *scan,const ssize_t y,
  MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(scan->image_view,0,y,scan->image->columns,1,
    scan->exception);
  if (p == (const Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  AccumulateStretchHistogram(scan->image,p,scan->histogram);
}

static void FormStretchHistogram(Image *image,double *histogram,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  StretchScan
    scan;

  ssize_t
    y;

  status=MagickTrue;
  (void) memset(histogram,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*histogram));
  image_view=AcquireVirtualCacheView(image,exception);
  scan.image=image;
  scan.image_view=image_view;
  scan.histogram=histogram;
  scan.exception=exception;
  for (y=0; y < (ssize_t) image->rows; y++)
    ScanStretchHistogramRow(&scan,y,&status);
  image_view=DestroyCacheView(image_view);
}

typedef struct _StretchLevels
{
  Image
    *image;

  double
    *histogram,
    black_point,
    white_point;
} StretchLevels;

static void FindStretchLevels(const StretchLevels *levels,Quantum *black,
  Quantum *white)
{
  Image
    *const image = levels->image;

  double
    *const histogram = levels->histogram;

  const double
    black_point = levels->black_point,
    white_point = levels->white_point;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ssize_t
      j;

    black[i]=(Quantum) 0;
    white[i]=(Quantum) ScaleQuantumToMap(QuantumRange);
    j=FindStretchBlackLevel(image,histogram,i,black_point);
    black[i]=(Quantum) j;
    j=FindStretchWhiteLevel(image,histogram,i,white_point);
    white[i]=(Quantum) j;
  }
}

#define ContrastStretchImageTag  "ContrastStretch/Image"

typedef struct _StretchPass
{
  Quantum
    *black,
    *white,
    *stretch_map;
} StretchPass;

static MagickBooleanType StretchImagePixels(Image *image,
  const StretchPass *pass,ExceptionInfo *exception)
{
  Quantum
    *const black = pass->black,
    *const white = pass->white,
    *const stretch_map = pass->stretch_map;

  const StretchMaps
    maps = { black, white, stretch_map };

  const ToneRows
    rows = { ContrastStretchImageTag,StretchImageRow,&maps };

  return(ToneImageRows(image,&rows,exception));
}

MagickExport MagickBooleanType ContrastStretchImage(Image *image,
  const double black_point,const double white_point,ExceptionInfo *exception)
{
  double
    *histogram;

  MagickBooleanType
    status;

  Quantum
    *black,
    *stretch_map,
    *white;

  StretchLevels
    levels;

  StretchPass
    pass;

  /*
    Allocate histogram and stretch map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  SetStretchGrayColorspace(image,exception);
  black=(Quantum *) AcquireQuantumMemory(MaxPixelChannels,sizeof(*black));
  white=(Quantum *) AcquireQuantumMemory(MaxPixelChannels,sizeof(*white));
  stretch_map=(Quantum *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*stretch_map));
  histogram=(double *) AcquireQuantumMemory(MaxMap+1UL,MaxPixelChannels*
    sizeof(*histogram));
  if (IsStretchMemoryMissing(black,white,stretch_map,histogram) != MagickFalse)
    {
      RelinquishStretchMemory(histogram,stretch_map,white,black);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  /*
    Form histogram.
  */
  FormStretchHistogram(image,histogram,exception);
  /*
    Find the histogram boundaries by locating the black/white levels.
  */
  levels.image=image;
  levels.histogram=histogram;
  levels.black_point=black_point;
  levels.white_point=white_point;
  FindStretchLevels(&levels,black,white);
  histogram=(double *) RelinquishMagickMemory(histogram);
  /*
    Stretch the histogram to create the stretched image mapping.
  */
  (void) memset(stretch_map,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*stretch_map));
  BuildStretchMap(image,black,white,stretch_map);
  /*
    Stretch-contrast colormap.
  */
  if (image->storage_class == PseudoClass)
    StretchColormap(image,stretch_map);
  /*
    Stretch-contrast image.
  */
  pass.black=black;
  pass.white=white;
  pass.stretch_map=stretch_map;
  status=StretchImagePixels(image,&pass,exception);
  SetStretchProperty(image,black,white,exception);
  white=(Quantum *) RelinquishMagickMemory(white);
  black=(Quantum *) RelinquishMagickMemory(black);
  stretch_map=(Quantum *) RelinquishMagickMemory(stretch_map);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     G a m m a I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GammaImage() gamma-corrects a particular image channel.  The same
%  image viewed on different devices will have perceptual differences in the
%  way the image's intensities are represented on the screen.  Specify
%  individual gamma levels for the red, green, and blue channels, or adjust
%  all three with the gamma parameter.  Values typically range from 0.8 to 2.3.
%
%  You can also reduce the influence of a particular channel with a gamma
%  value of 0.
%
%  The format of the GammaImage method is:
%
%      MagickBooleanType GammaImage(Image *image,const double gamma,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o level: the image gamma as a string (e.g. 1.6,1.2,1.0).
%
%    o gamma: the image gamma.
%
*/

static void GammaImageRow(const Image *image,const void *arguments,
  Quantum *q)
{
  const Quantum
    *gamma_map = (const Quantum *) arguments;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      j;

    for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,j);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      q[j]=gamma_map[ScaleQuantumToMap(ClampToQuantum((MagickRealType)
        q[j]))];
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void BuildGammaMap(Quantum *gamma_map,const double gamma)
{
  ssize_t
    i;

  if (gamma != 0.0)
    for (i=0; i <= (ssize_t) MaxMap; i++)
      gamma_map[i]=ScaleMapToQuantum((double) (MaxMap*pow((double) i/
        MaxMap,MagickSafeReciprocal(gamma))));
}

static void GammaColormap(Image *image,const Quantum *gamma_map)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    /*
      Gamma-correct colormap.
    */
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].red=(double) gamma_map[ScaleQuantumToMap(
        ClampToQuantum(image->colormap[i].red))];
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].green=(double) gamma_map[ScaleQuantumToMap(
        ClampToQuantum(image->colormap[i].green))];
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].blue=(double) gamma_map[ScaleQuantumToMap(
        ClampToQuantum(image->colormap[i].blue))];
    if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].alpha=(double) gamma_map[ScaleQuantumToMap(
        ClampToQuantum(image->colormap[i].alpha))];
  }
}

static void ScaleImageGamma(Image *image,const double gamma)
{
  if (image->gamma != 0.0)
    image->gamma*=gamma;
}

MagickExport MagickBooleanType GammaImage(Image *image,const double gamma,
  ExceptionInfo *exception)
{
#define GammaImageTag  "Gamma/Image"

  MagickBooleanType
    status;

  Quantum
    *gamma_map;

  ToneRows
    rows;

  /*
    Allocate and initialize gamma maps.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (gamma == 1.0)
    return(MagickTrue);
  gamma_map=(Quantum *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*gamma_map));
  if (gamma_map == (Quantum *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) memset(gamma_map,0,(MaxMap+1)*sizeof(*gamma_map));
  BuildGammaMap(gamma_map,gamma);
  if (image->storage_class == PseudoClass)
    GammaColormap(image,gamma_map);
  /*
    Gamma-correct image.
  */
  rows.tag=GammaImageTag;
  rows.method=GammaImageRow;
  rows.arguments=gamma_map;
  status=ToneImageRows(image,&rows,exception);
  gamma_map=(Quantum *) RelinquishMagickMemory(gamma_map);
  ScaleImageGamma(image,gamma);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     G r a y s c a l e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GrayscaleImage() converts the image to grayscale.
%
%  The format of the GrayscaleImage method is:
%
%      MagickBooleanType GrayscaleImage(Image *image,
%        const PixelIntensityMethod method ,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o method: the pixel intensity method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void EncodeGrayscaleGamma(const Image *image,MagickRealType *red,
  MagickRealType *green,MagickRealType *blue)
{
  if (image->colorspace == RGBColorspace)
    {
      (*red)=EncodePixelGamma((*red));
      (*green)=EncodePixelGamma((*green));
      (*blue)=EncodePixelGamma((*blue));
    }
}

typedef struct _GrayscaleSample
{
  const Image
    *image;

  PixelIntensityMethod
    method;

  MagickRealType
    blue,
    green,
    red;
} GrayscaleSample;

static void GetGammaLumaIntensity(const GrayscaleSample *sample,
  MagickRealType *intensity)
{
  const Image
    *const image = sample->image;

  const PixelIntensityMethod
    method = sample->method;

  MagickRealType
    blue = sample->blue,
    green = sample->green,
    red = sample->red;

  switch (method)
  {
    case Rec601LumaPixelIntensityMethod:
    {
      EncodeGrayscaleGamma(image,&red,&green,&blue);
      (*intensity)=0.298839*red+0.586811*green+0.114350*blue;
      break;
    }
    case Rec709LumaPixelIntensityMethod:
    default:
    {
      if (image->colorspace == RGBColorspace)
        {
          red=EncodePixelGamma(red);
          green=EncodePixelGamma(green);
          blue=EncodePixelGamma(blue);
        }
      (*intensity)=0.212656*red+0.715158*green+0.072186*blue;
      break;
    }
  }
}

static void GetLuminanceIntensity(const GrayscaleSample *sample,
  MagickRealType *intensity)
{
  const Image
    *const image = sample->image;

  const PixelIntensityMethod
    method = sample->method;

  MagickRealType
    blue = sample->blue,
    green = sample->green,
    red = sample->red;

  switch (method)
  {
    case Rec601LuminancePixelIntensityMethod:
    {
      if (image->colorspace == sRGBColorspace)
        {
          red=DecodePixelGamma(red);
          green=DecodePixelGamma(green);
          blue=DecodePixelGamma(blue);
        }
      (*intensity)=0.298839*red+0.586811*green+0.114350*blue;
      break;
    }
    default:
    {
      GetGammaLumaIntensity(sample,intensity);
      break;
    }
    case Rec709LuminancePixelIntensityMethod:
    {
      if (image->colorspace == sRGBColorspace)
        {
          red=DecodePixelGamma(red);
          green=DecodePixelGamma(green);
          blue=DecodePixelGamma(blue);
        }
      (*intensity)=0.212656*red+0.715158*green+0.072186*blue;
      break;
    }
  }
}

static MagickRealType GetGrayscaleIntensity(const Image *image,
  const PixelIntensityMethod method,const Quantum *q)
{
  GrayscaleSample
    sample;

  MagickRealType
    blue,
    green,
    red,
    intensity;

  red=(MagickRealType) GetPixelRed(image,q);
  green=(MagickRealType) GetPixelGreen(image,q);
  blue=(MagickRealType) GetPixelBlue(image,q);
  intensity=0.0;
  sample.image=image;
  sample.method=method;
  sample.blue=blue;
  sample.green=green;
  sample.red=red;
  switch (method)
  {
    case AveragePixelIntensityMethod:
    {
      intensity=(red+green+blue)/3.0;
      break;
    }
    case BrightnessPixelIntensityMethod:
    {
      intensity=MagickMax(MagickMax(red,green),blue);
      break;
    }
    case LightnessPixelIntensityMethod:
    {
      intensity=(MagickMin(MagickMin(red,green),blue)+
        MagickMax(MagickMax(red,green),blue))/2.0;
      break;
    }
    case MSPixelIntensityMethod:
    {
      intensity=(MagickRealType) (((double) red*red+green*green+
        blue*blue)/3.0);
      break;
    }
    default:
    {
      GetLuminanceIntensity(&sample,&intensity);
      break;
    }
    case RMSPixelIntensityMethod:
    {
      intensity=(MagickRealType) (sqrt((double) red*red+green*green+
        blue*blue)/sqrt(3.0));
      break;
    }
  }
  return(intensity);
}

static void GrayscaleImageRow(const Image *image,const void *arguments,
  Quantum *q)
{
  const PixelIntensityMethod
    method = (*(const PixelIntensityMethod *) arguments);

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    MagickRealType
      intensity;

    intensity=GetGrayscaleIntensity(image,method,q);
    SetPixelGray(image,ClampToQuantum(intensity),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType SyncGrayscaleImageClass(Image *image,
  ExceptionInfo *exception)
{
  if (image->storage_class == PseudoClass)
    {
      if (SyncImage(image,exception) == MagickFalse)
        return(MagickFalse);
      if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType SetGrayscaleImageColorspace(Image *image,
  const PixelIntensityMethod method,ExceptionInfo *exception)
{
  image->intensity=method;
  image->type=GrayscaleType;
  if ((method == Rec601LuminancePixelIntensityMethod) ||
      (method == Rec709LuminancePixelIntensityMethod))
    return(SetImageColorspace(image,LinearGRAYColorspace,exception));
  return(SetImageColorspace(image,GRAYColorspace,exception));
}

MagickExport MagickBooleanType GrayscaleImage(Image *image,
  const PixelIntensityMethod method,ExceptionInfo *exception)
{
#define GrayscaleImageTag  "Grayscale/Image"

  const ToneRows
    rows = { GrayscaleImageTag,GrayscaleImageRow,&method };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SyncGrayscaleImageClass(image,exception) == MagickFalse)
    return(MagickFalse);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateGrayscaleImage(image,method,exception) != MagickFalse)
    {
      image->intensity=method;
      image->type=GrayscaleType;
      if ((method == Rec601LuminancePixelIntensityMethod) ||
          (method == Rec709LuminancePixelIntensityMethod))
        return(SetImageColorspace(image,LinearGRAYColorspace,exception));
      return(SetImageColorspace(image,GRAYColorspace,exception));
    }
#endif
  /*
    Grayscale image.
  */
  (void) ToneImageRows(image,&rows,exception);
  return(SetGrayscaleImageColorspace(image,method,exception));
}


