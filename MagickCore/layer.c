/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                     L       AAA   Y   Y  EEEEE  RRRR                        %
%                     L      A   A   Y Y   E      R   R                       %
%                     L      AAAAA    Y    EEE    RRRR                        %
%                     L      A   A    Y    E      R R                         %
%                     LLLLL  A   A    Y    EEEEE  R  R                        %
%                                                                             %
%                      MagickCore Image Layering Methods                      %
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


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R e m o v e Z e r o D e l a y L a y e r s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveZeroDelayLayers() removes any image that as a zero delay time. Such
%  images generally represent intermediate or partial updates in GIF
%  animations used for file optimization.  They are not ment to be displayed
%  to users of the animation.  Viewable images in an animation should have a
%  time delay of 3 or more centi-seconds (hundredths of a second).
%
%  However if all the frames have a zero time delay, then either the animation
%  is as yet incomplete, or it is not a GIF animation.  This a non-sensible
%  situation, so no image will be removed and a 'Zero Time Animation' warning
%  (exception) given.
%
%  No warning will be given if no image was removed because all images had an
%  appropriate non-zero time delay set.
%
%  Due to the special requirements of GIF disposal handling, GIF animations
%  should be coalesced first, before calling this function, though that is not
%  a requirement.
%
%  The format of the RemoveZeroDelayLayers method is:
%
%      void RemoveZeroDelayLayers(Image **image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image list
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *GetFirstNonZeroDelayImage(Image **images)
{
  Image
    *i;

  i=GetFirstImageInList(*images);
  for ( ; i != (Image *) NULL; i=GetNextImageInList(i))
    if ( i->delay != 0L ) break;
  return(i);
}

MagickExport void RemoveZeroDelayLayers(Image **images,
     ExceptionInfo *exception)
{
  Image
    *i;

  assert((*images) != (const Image *) NULL);
  assert((*images)->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      (*images)->filename);
  i=GetFirstNonZeroDelayImage(images);
  if ( i == (Image *) NULL ) {
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
       "ZeroTimeAnimation","`%s'",GetFirstImageInList(*images)->filename);
    return;
  }
  i=GetFirstImageInList(*images);
  while ( i != (Image *) NULL )
  {
    if ( i->delay == 0L ) {
      (void) DeleteImageFromList(&i);
      *images=i;
    }
    else
      i=GetNextImageInList(i);
  }
  *images=GetFirstImageInList(*images);
}



