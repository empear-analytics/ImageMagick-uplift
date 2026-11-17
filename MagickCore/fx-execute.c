/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                                FFFFF  X   X                                 %
%                                F       X X                                  %
%                                FFF      X                                   %
%                                F       X X                                  %
%                                F      X   X                                 %
%                                                                             %
%               EEEEE  X   X  EEEEE   CCCC  U   U  TTTTT  EEEEE               %
%               E       X X   E      C      U   U    T    E                   %
%               EEE      X    EEE    C      U   U    T    EEE                 %
%               E       X X   E      C      U   U    T    E                   %
%               EEEEE  X   X  EEEEE   CCCC   UUU     T    EEEEE               %
%                                                                             %
%                                                                             %
%                 MagickCore Fx Expression Execution Methods                  %
%                                                                             %
%                               Software Design                               %
%                             snibgo (Alan Gibson)                            %
%                                 January 2022                                %
%                                                                             %
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
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/fx-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/policy.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/fx-execute-private.h"

/* Following is substitute for FxEvaluateChannelExpression().
*/
MagickPrivate MagickBooleanType FxEvaluateChannelExpression (
  FxInfo *pfx,
  const PixelChannel channel, const ssize_t x, const ssize_t y,
  double *result, ExceptionInfo *exception)
{
  const int
    id = GetOpenMPThreadId();

  fxFltType ret;

  const fxPixelPosT pos = { channel, x, y };

  assert (pfx != NULL);
  assert (pfx->image != NULL);
  assert (pfx->Images != NULL);
  assert (pfx->Imgs != NULL);
  assert (pfx->fxrts != NULL);

  pfx->fxrts[id].thisPixel = NULL;


  if (!ExecuteRPN (pfx, &pfx->fxrts[id], &ret, &pos)) {
    (void) ThrowMagickException (
      exception, GetMagickModule(), OptionError,
      "ExecuteRPN failed", " ");
    return MagickFalse;
  }

  *result = (double) ret;

  return MagickTrue;
}
