/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                      L       AAA   Y   Y  EEEEE  RRRR                       %
%                      L      A   A   Y Y   E      R   R                      %
%                      L      AAAAA    Y    EEE    RRRR                       %
%                      L      A   A    Y    E      R R                        %
%                      LLLLL  A   A    Y    EEEEE  R  R                       %
%                                                                             %
%                  FFFFF  RRRR    AAA   M   M  EEEEE  SSSSS                   %
%                  F      R   R  A   A  MM MM  E      SS                      %
%                  FFF    RRRR   AAAAA  M M M  EEE     SSS                    %
%                  F      R R    A   A  M   M  E         SS                   %
%                  F      R  R   A   A  M   M  EEEEE  SSSSS                   %
%                                                                             %
%                    MagickCore Image Layer Frame Methods                     %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                              Anthony Thyssen                                %
%                               January 2006                                  %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/effect.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/profile.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resize.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/layer-frames-private.h"



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R e m o v e D u p l i c a t e L a y e r s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveDuplicateLayers() removes any image that is exactly the same as the
%  next image in the given image list.  Image size and virtual canvas offset
%  must also match, though not the virtual canvas size itself.
%
%  No check is made with regards to image disposal setting, though it is the
%  dispose setting of later image that is kept.  Also any time delays are also
%  added together. As such coalesced image animations should still produce the
%  same result, though with duplicate frames merged into a single frame.
%
%  The format of the RemoveDuplicateLayers method is:
%
%      void RemoveDuplicateLayers(Image **image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image list
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsDifferentLayerGeometry(const Image *image,
  const Image *next)
{
  return(((image->columns != next->columns) || (image->rows != next->rows) ||
    (image->page.x != next->page.x) ||
    (image->page.y != next->page.y)) ? MagickTrue : MagickFalse);
}

MagickExport void RemoveDuplicateLayers(Image **images,ExceptionInfo *exception)
{
  RectangleInfo
    bounds;

  Image
    *image,
    *next;

  assert((*images) != (const Image *) NULL);
  assert((*images)->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      (*images)->filename);
  image=GetFirstImageInList(*images);
  for ( ; (next=GetNextImageInList(image)) != (Image *) NULL; image=next)
  {
    if (IsDifferentLayerGeometry(image,next) != MagickFalse)
      continue;
    bounds=CompareImagesBounds(image,next,CompareAnyLayer,exception);
    if (bounds.x < 0)
      {
        /*
          Two images are the same, merge time delays and delete one.
        */
        size_t
          time;

        time=(size_t) (1000.0*image->delay*
          MagickSafeReciprocal((double) image->ticks_per_second));
        time+=(size_t) (1000.0*next->delay*
          MagickSafeReciprocal((double) next->ticks_per_second));
        next->ticks_per_second=100L;
        next->delay=time*(size_t) image->ticks_per_second/1000;
        next->iterations=image->iterations;
        *images=image;
        (void) DeleteImageFromList(images);
      }
  }
  *images=GetFirstImageInList(*images);
}
