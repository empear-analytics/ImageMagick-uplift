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
%                   CCCC   OOO   L       OOO   RRRR   SSSSS                   %
%                  C      O   O  L      O   O  R   R  SS                      %
%                  C      O   O  L      O   O  RRRR    SSS                    %
%                  C      O   O  L      O   O  R R       SS                   %
%                   CCCC   OOO   LLLLL   OOO   R  R   SSSSS                   %
%                                                                             %
%                                                                             %
%                    MagickCore Image Level Colors Methods                    %
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

static inline MagickBooleanType IsGrayImageWithColorLevels(const Image *image,
  const PixelInfo *black_color,const PixelInfo *white_color)
{
  return(((IsGrayColorspace(image->colorspace) != MagickFalse) &&
    ((IsGrayColorspace(black_color->colorspace) == MagickFalse) ||
     (IsGrayColorspace(white_color->colorspace) == MagickFalse))) ?
    MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L e v e l I m a g e C o l o r s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LevelImageColors() maps the given color to "black" and "white" values,
%  linearly spreading out the colors, and level values on a channel by channel
%  bases, as per LevelImage().  The given colors allows you to specify
%  different level ranges for each of the color channels separately.
%
%  If the boolean 'invert' is set true the image values will modified in the
%  reverse direction. That is any existing "black" and "white" colors in the
%  image will become the color values given, with all other values compressed
%  appropriately.  This effectively maps a greyscale gradient into the given
%  color gradient.
%
%  The format of the LevelImageColors method is:
%
%    MagickBooleanType LevelImageColors(Image *image,
%      const PixelInfo *black_color,const PixelInfo *white_color,
%      const MagickBooleanType invert,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_color: The color to map black to/from
%
%    o white_point: The color to map white to/from
%
%    o invert: if true map the colors (levelize), rather than from (level)
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _LevelColors
{
  Image
    *image;

  const PixelInfo
    *black_color,
    *white_color;

  ExceptionInfo
    *exception;
} LevelColors;

static MagickStatusType LevelColorsPass(const LevelColors *levels,
  MagickStatusType status)
{
  const PixelInfo
    *const black_color = levels->black_color,
    *const white_color = levels->white_color;

  ChannelType
    channel_mask;

  Image
    *const image = levels->image;

  ExceptionInfo
    *const exception = levels->exception;

  if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
    {
      channel_mask=SetImageChannelMask(image,RedChannel);
      status&=(MagickStatusType) LevelImage(image,black_color->red,
        white_color->red,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
    {
      channel_mask=SetImageChannelMask(image,GreenChannel);
      status&=(MagickStatusType) LevelImage(image,black_color->green,
        white_color->green,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
    {
      channel_mask=SetImageChannelMask(image,BlueChannel);
      status&=(MagickStatusType) LevelImage(image,black_color->blue,
        white_color->blue,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  if (((GetPixelBlackTraits(image) & UpdatePixelTrait) != 0) &&
      (image->colorspace == CMYKColorspace))
    {
      channel_mask=SetImageChannelMask(image,BlackChannel);
      status&=(MagickStatusType) LevelImage(image,black_color->black,
        white_color->black,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  if (((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0) &&
      (image->alpha_trait != UndefinedPixelTrait))
    {
      channel_mask=SetImageChannelMask(image,AlphaChannel);
      status&=(MagickStatusType) LevelImage(image,black_color->alpha,
        white_color->alpha,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  return(status);
}

static MagickStatusType LevelizeColorsPass(const LevelColors *levels,
  MagickStatusType status)
{
  const PixelInfo
    *const black_color = levels->black_color,
    *const white_color = levels->white_color;

  ChannelType
    channel_mask;

  Image
    *const image = levels->image;

  ExceptionInfo
    *const exception = levels->exception;

  if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
    {
      channel_mask=SetImageChannelMask(image,RedChannel);
      status&=(MagickStatusType) LevelizeImage(image,black_color->red,
        white_color->red,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
    {
      channel_mask=SetImageChannelMask(image,GreenChannel);
      status&=(MagickStatusType) LevelizeImage(image,black_color->green,
        white_color->green,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
    {
      channel_mask=SetImageChannelMask(image,BlueChannel);
      status&=(MagickStatusType) LevelizeImage(image,black_color->blue,
        white_color->blue,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  if (((GetPixelBlackTraits(image) & UpdatePixelTrait) != 0) &&
      (image->colorspace == CMYKColorspace))
    {
      channel_mask=SetImageChannelMask(image,BlackChannel);
      status&=(MagickStatusType) LevelizeImage(image,black_color->black,
        white_color->black,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  if (((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0) &&
      (image->alpha_trait != UndefinedPixelTrait))
    {
      channel_mask=SetImageChannelMask(image,AlphaChannel);
      status&=(MagickStatusType) LevelizeImage(image,black_color->alpha,
        white_color->alpha,1.0,exception);
      (void) SetImageChannelMask(image,channel_mask);
    }
  return(status);
}

MagickExport MagickBooleanType LevelImageColors(Image *image,
  const PixelInfo *black_color,const PixelInfo *white_color,
  const MagickBooleanType invert,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  /*
    Allocate and initialize levels map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsGrayImageWithColorLevels(image,black_color,white_color) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  status=MagickTrue;
  if (invert == MagickFalse)
    {
      LevelColors
        levels;

      levels.image=image;
      levels.black_color=black_color;
      levels.white_color=white_color;
      levels.exception=exception;
      status=LevelColorsPass(&levels,status);
    }
  else
    {
      LevelColors
        levels;

      levels.image=image;
      levels.black_color=black_color;
      levels.white_color=white_color;
      levels.exception=exception;
      status=LevelizeColorsPass(&levels,status);
    }
  return(status != 0 ? MagickTrue : MagickFalse);
}
