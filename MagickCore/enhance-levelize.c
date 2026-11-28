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
%           L      EEEEE  V   V  EEEEE  L      IIIII  ZZZZZ  EEEEE            %
%           L      E      V   V  E      L        I       ZZ  E                %
%           L      EEE    V   V  EEE    L        I     ZZZ   EEE              %
%           L      E       V V   E      L        I    ZZ     E                %
%           LLLLL  EEEEE    V    EEEEE  LLLLL  IIIII  ZZZZZ  EEEEE            %
%                                                                             %
%                                                                             %
%                      MagickCore Image Levelize Method                       %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L e v e l i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LevelizeImage() applies the reversed LevelImage() operation to just
%  the specific channels specified.  It compresses the full range of color
%  values, so that they lie between the given black and white points. Gamma is
%  applied before the values are mapped.
%
%  LevelizeImage() can be called with by using a +level command line
%  API option, or using a '!' on a -level or LevelImage() geometry string.
%
%  It can be used to de-contrast a greyscale image to the exact levels
%  specified.  Or by using specific levels for each channel of an image you
%  can convert a gray-scale image to any linear color gradient, according to
%  those levels.
%
%  The format of the LevelizeImage method is:
%
%      MagickBooleanType LevelizeImage(Image *image,const double black_point,
%        const double white_point,const double gamma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_point: The level to map zero (black) to.
%
%    o white_point: The level to map QuantumRange (white) to.
%
%    o gamma: adjust gamma by this factor before mapping values.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define LevelizeValue(x) ClampToQuantum(((MagickRealType) gamma_pow((double) \
  (QuantumScale*((double) x)),gamma))*(white_point-black_point)+black_point)

static void LevelizeImageRow(const Image *image,const void *arguments,
  Quantum *q)
{
  const LevelPoints
    *level = (const LevelPoints *) arguments;

  const double
    black_point = level->black_point,
    white_point = level->white_point,
    gamma = level->gamma;

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
      q[j]=LevelizeValue(q[j]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType LevelizeImage(Image *image,
  const double black_point,const double white_point,const double gamma,
  ExceptionInfo *exception)
{
#define LevelizeImageTag  "Levelize/Image"

  const LevelPoints
    level = { black_point, white_point, gamma };

  ssize_t
    i;

  const ToneRows
    rows = { LevelizeImageTag,LevelizeImageRow,&level };

  /*
    Allocate and initialize levels map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    for (i=0; i < (ssize_t) image->colors; i++)
    {
      /*
        Level colormap.
      */
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].red=(double) LevelizeValue(image->colormap[i].red);
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].green=(double) LevelizeValue(
          image->colormap[i].green);
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].blue=(double) LevelizeValue(image->colormap[i].blue);
      if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].alpha=(double) LevelizeValue(
          image->colormap[i].alpha);
    }
  /*
    Level image.
  */
  return(ToneImageRows(image,&rows,exception));
}
