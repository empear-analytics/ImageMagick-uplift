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
%                      RRRR    AAA   N   N   GGGG  EEEEE                      %
%                      R   R  A   A  NN  N  G      E                          %
%                      RRRR   AAAAA  N N N  G  GG  EEE                        %
%                      R R    A   A  N  NN  G   G  E                          %
%                      R  R   A   A  N   N   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                  MagickCore Image Range Threshold Methods                   %
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
#include "MagickCore/threshold-level-private.h"

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

typedef struct _RangeThresholdRow
{
  double
    low_black,
    low_white,
    high_white,
    high_black;
} RangeThresholdRow;

static void RangeThresholdPixels(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  const RangeThresholdRow
    *row = (const RangeThresholdRow *) arguments;

  const double
    low_black = row->low_black,
    low_white = row->low_white,
    high_white = row->high_white,
    high_black = row->high_black;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    RangeThresholdChannels(image,low_black,low_white,high_white,high_black,
      q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType RangeThresholdImage(Image *image,
  const double low_black,const double low_white,const double high_white,
  const double high_black,ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  const RangeThresholdRow
    row = { low_black,low_white,high_white,high_black };

  const ThresholdRows
    rows = { ThresholdImageTag,RangeThresholdPixels,&row };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetDirectRGBImage(image,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Range threshold image.
  */
  return(ThresholdImageRows(image,&rows,exception));
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

static void WhiteThresholdIntensity(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  const PixelInfo
    *threshold = (const PixelInfo *) arguments;

  ThresholdIntensity(image,WhiteThresholdPixel,threshold,q);
}

static void WhiteThresholdRow(const Image *image,const void *arguments,
  Quantum *magick_restrict q)
{
  ThresholdPixels(image,WhiteThresholdIntensity,arguments,q);
}

static MagickBooleanType TryWhiteThresholdImage(Image *image,
  const char *thresholds,ExceptionInfo *exception,MagickBooleanType *result)
{
  if (thresholds == (const char *) NULL)
    {
      (*result)=MagickTrue;
      return(MagickTrue);
    }
  if (SetDirectRGBImage(image,exception) == MagickFalse)
    {
      (*result)=MagickFalse;
      return(MagickTrue);
    }
  return(MagickFalse);
}

MagickExport MagickBooleanType WhiteThresholdImage(Image *image,
  const char *thresholds,ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  MagickBooleanType
    returned;

  PixelInfo
    threshold;

  const ThresholdRows
    rows = { ThresholdImageTag,WhiteThresholdRow,&threshold };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (TryWhiteThresholdImage(image,thresholds,exception,&returned) != MagickFalse)
    return(returned);
  threshold=GetThresholdLevels(image,thresholds);
  /*
    White threshold image.
  */
  return(ThresholdImageRows(image,&rows,exception));
}
