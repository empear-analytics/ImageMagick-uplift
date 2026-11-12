/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%              EEEEE  N   N  H   H   AAA   N   N   CCCC  EEEEE                %
%              E      NN  N  H   H  A   A  NN  N  C      E                    %
%              EEE    N N N  HHHHH  AAAAA  N N N  C      EEE                  %
%              E      N  NN  H   H  A   A  N  NN  C      E                    %
%              EEEEE  N   N  H   H  A   A  N   N   CCCC  EEEEE                %
%                                                                             %
%                                                                             %
%                    MagickCore Image Enhancement Methods                     %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A u t o L e v e l I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AutoLevelImage() adjusts the levels of a particular image channel by
%  scaling the minimum and maximum values to the full quantum range.
%
%  The format of the LevelImage method is:
%
%      MagickBooleanType AutoLevelImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-level
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType AutoLevelImage(Image *image,
  ExceptionInfo *exception)
{
  return(MinMaxStretchImage(image,0.0,0.0,1.0,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B r i g h t n e s s C o n t r a s t I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BrightnessContrastImage() changes the brightness and/or contrast of an
%  image.  It converts the brightness and contrast parameters into slope and
%  intercept and calls a polynomial function to apply to the image.
%
%  The format of the BrightnessContrastImage method is:
%
%      MagickBooleanType BrightnessContrastImage(Image *image,
%        const double brightness,const double contrast,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o brightness: the brightness percent (-100 .. 100).
%
%    o contrast: the contrast percent (-100 .. 100).
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType BrightnessContrastImage(Image *image,
  const double brightness,const double contrast,ExceptionInfo *exception)
{
#define BrightnessContrastImageTag  "BrightnessContrast/Image"

  double
    coefficients[2],
    intercept,
    slope;

  MagickBooleanType
    status;

  /*
    Compute slope and intercept.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  slope=100.0*MagickSafeReciprocal(100.0-contrast);
  if (contrast < 0.0)
    slope=0.01*contrast+1.0;
  intercept=(0.01*brightness-0.5)*slope+0.5;
  coefficients[0]=slope;
  coefficients[1]=intercept;
  status=FunctionImage(image,PolynomialFunction,2,coefficients,exception);
  return(status);
}





/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L i n e a r S t r e t c h I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LinearStretchImage() discards any pixels below the black point and above
%  the white point and levels the remaining pixels.
%
%  The format of the LinearStretchImage method is:
%
%      MagickBooleanType LinearStretchImage(Image *image,
%        const double black_point,const double white_point,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_point: the black point.
%
%    o white_point: the white point.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ssize_t FindLinearBlackLevel(const double *histogram,
  const double black_point)
{
  double
    intensity;

  ssize_t
    black;

  intensity=0.0;
  for (black=0; black < (ssize_t) MaxMap; black++)
  {
    intensity+=histogram[black];
    if (intensity >= black_point)
      break;
  }
  return(black);
}

static ssize_t FindLinearWhiteLevel(const double *histogram,
  const double white_point)
{
  double
    intensity;

  ssize_t
    white;

  intensity=0.0;
  for (white=(ssize_t) MaxMap; white != 0; white--)
  {
    intensity+=histogram[white];
    if (intensity >= white_point)
      break;
  }
  return(white);
}

MagickExport MagickBooleanType LinearStretchImage(Image *image,
  const double black_point,const double white_point,ExceptionInfo *exception)
{
#define LinearStretchImageTag  "LinearStretch/Image"

  CacheView
    *image_view;

  char
    property[MagickPathExtent];

  double
    *histogram,
    intensity;

  MagickBooleanType
    status;

  ssize_t
    black,
    white,
    y;

  /*
    Allocate histogram and linear map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  histogram=(double *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*histogram));
  if (histogram == (double *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Form histogram.
  */
  (void) memset(histogram,0,(MaxMap+1)*sizeof(*histogram));
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      intensity=GetPixelIntensity(image,p);
      histogram[ScaleQuantumToMap(ClampToQuantum(intensity))]++;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  image_view=DestroyCacheView(image_view);
  /*
    Find the histogram boundaries by locating the black and white point levels.
  */
  black=FindLinearBlackLevel(histogram,black_point);
  white=FindLinearWhiteLevel(histogram,white_point);
  histogram=(double *) RelinquishMagickMemory(histogram);
  status=LevelImage(image,(double) ScaleMapToQuantum((MagickRealType) black),
    (double) ScaleMapToQuantum((MagickRealType) white),1.0,exception);
  (void) FormatLocaleString(property,MagickPathExtent,"%gx%g%%",100.0*black/
    MaxMap,100.0*white/MaxMap);
  (void) SetImageProperty(image,"histogram:linear-stretch",property,exception);
  return(status);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     N o r m a l i z e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The NormalizeImage() method enhances the contrast of a color image by
%  mapping the darkest 2 percent of all pixel to black and the brightest
%  1 percent to white.
%
%  The format of the NormalizeImage method is:
%
%      MagickBooleanType NormalizeImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType NormalizeImage(Image *image,
  ExceptionInfo *exception)
{
  double
    black_point,
    white_point;

  black_point=0.02*image->columns*image->rows;
  white_point=0.99*image->columns*image->rows;
  return(ContrastStretchImage(image,black_point,white_point,exception));
}

