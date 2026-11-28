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
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                        MagickCore Image Level Method                        %
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
%     L e v e l I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LevelImage() adjusts the levels of a particular image channel by
%  scaling the colors falling between specified white and black points to
%  the full available quantum range.
%
%  The parameters provided represent the black, and white points.  The black
%  point specifies the darkest color in the image. Colors darker than the
%  black point are set to zero.  White point specifies the lightest color in
%  the image.  Colors brighter than the white point are set to the maximum
%  quantum value.
%
%  If a '!' flag is given, map black and white colors to the given levels
%  rather than mapping those levels to black and white.  See
%  LevelizeImage() below.
%
%  Gamma specifies a gamma correction to apply to the image.
%
%  The format of the LevelImage method is:
%
%      MagickBooleanType LevelImage(Image *image,const double black_point,
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
%    o exception: return any errors or warnings in this structure.
%
*/

static inline double LevelPixel(const double black_point,
  const double white_point,const double gamma,const double pixel)
{
  double
    level_pixel,
    scale;

  scale=MagickSafeReciprocal(white_point-black_point);
  level_pixel=(double) QuantumRange*gamma_pow(scale*((double) pixel-(double)
    black_point),MagickSafeReciprocal(gamma));
  return(level_pixel);
}

static void LevelColormap(Image *image,const double black_point,
  const double white_point,const double gamma)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    /*
      Level colormap.
    */
    if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].red=(double) ClampToQuantum(LevelPixel(black_point,
        white_point,gamma,image->colormap[i].red));
    if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].green=(double) ClampToQuantum(LevelPixel(black_point,
        white_point,gamma,image->colormap[i].green));
    if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].blue=(double) ClampToQuantum(LevelPixel(black_point,
        white_point,gamma,image->colormap[i].blue));
    if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
      image->colormap[i].alpha=(double) ClampToQuantum(LevelPixel(black_point,
        white_point,gamma,image->colormap[i].alpha));
  }
}

static void LevelImageRow(const Image *image,const void *arguments,
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
      q[j]=ClampToQuantum(LevelPixel(black_point,white_point,gamma,
        (double) q[j]));
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType LevelImage(Image *image,const double black_point,
  const double white_point,const double gamma,ExceptionInfo *exception)
{
#define LevelImageTag  "Level/Image"

  const LevelPoints
    level = { black_point, white_point, gamma };

  MagickBooleanType
    status;

  const ToneRows
    rows = { LevelImageTag,LevelImageRow,&level };

  /*
    Allocate and initialize levels map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    LevelColormap(image,black_point,white_point,gamma);
  /*
    Level image.
  */
  status=ToneImageRows(image,&rows,exception);
  (void) ClampImage(image,exception);
  return(status);
}
