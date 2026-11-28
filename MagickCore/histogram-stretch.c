/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        H   H  IIIII  SSSSS  TTTTT   OOO    GGGG  RRRR    AAA   M   M        %
%        H   H    I    SS       T    O   O  G      R   R  A   A  MM MM        %
%        HHHHH    I     SSS     T    O   O  G  GG  RRRR   AAAAA  M M M        %
%        H   H    I       SS    T    O   O  G   G  R R    A   A  M   M        %
%        H   H  IIIII  SSSSS    T     OOO    GGG   R  R   A   A  M   M        %
%                                                                             %
%               SSSSS  TTTTT  RRRR   EEEEE  TTTTT   CCCC  H   H               %
%               SS       T    R   R  E        T    C      H   H               %
%                SSS     T    RRRR   EEE      T    C      HHHHH               %
%                  SS    T    R R    E        T    C      H   H               %
%               SSSSS    T    R  R   EEEEE    T     CCCC  H   H               %
%                                                                             %
%                                                                             %
%                      MagickCore Min-Max Stretch Method                      %
%                                                                             %
%                              Software Design                                %
%                              Anthony Thyssen                                %
%                               Fred Weinhaus                                 %
%                                August 2009                                  %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/prepress.h"
#include "MagickCore/quantize.h"
#include "MagickCore/registry.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M i n M a x S t r e t c h I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MinMaxStretchImage() uses the exact minimum and maximum values found in
%  each of the channels given, as the BlackPoint and WhitePoint to linearly
%  stretch the colors (and histogram) of the image.  The stretch points are
%  also moved further inward by the adjustment values given.
%
%  If the adjustment values are both zero this function is equivalent to a
%  perfect normalization (or autolevel) of the image.
%
%  Each channel is stretched independently of each other (producing color
%  distortion) unless the special 'SyncChannels' flag is also provided in the
%  channels setting. If this flag is present the minimum and maximum point
%  will be extracted from all the given channels, and those channels will be
%  stretched by exactly the same amount (preventing color distortion).
%
%  In the special case that only ONE value is found in a channel of the image
%  that value is not stretched, that value is left as is.
%
%  The 'SyncChannels' is turned on in the 'DefaultChannels' setting by
%  default.
%
%  The format of the MinMaxStretchImage method is:
%
%      MagickBooleanType MinMaxStretchImage(Image *image,const double black,
%        const double white,const double gamma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-level
%
%    o black, white:  move the black / white point inward from the minimum and
%      maximum points by this color value.
%
%    o gamma: the gamma.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _MinMaxStretch
{
  Image
    *image;

  double
    black,
    white,
    gamma;

  ExceptionInfo
    *exception;
} MinMaxStretch;

static MagickBooleanType StretchAllChannels(const MinMaxStretch *stretch,
  MagickStatusType status)
{
  Image
    *const image = stretch->image;

  const double
    black = stretch->black,
    white = stretch->white,
    gamma = stretch->gamma;

  ExceptionInfo
    *const exception = stretch->exception;

  double
    min,
    max;

  /*
    Auto-level all channels equally.
  */
  (void) GetImageRange(image,&min,&max,exception);
  min+=black;
  max-=white;
  if (fabs(min-max) >= MagickEpsilon)
    status&=(MagickStatusType) LevelImage(image,min,max,gamma,exception);
  return(status != 0 ? MagickTrue : MagickFalse);
}

MagickExport MagickBooleanType MinMaxStretchImage(Image *image,
  const double black,const double white,const double gamma,
  ExceptionInfo *exception)
{
  double
    min,
    max;

  ssize_t
    i;

  MagickStatusType
    status;

  status=MagickTrue;
  if (image->channel_mask == AllChannels)
    {
      MinMaxStretch
        stretch;

      stretch.image=image;
      stretch.black=black;
      stretch.white=white;
      stretch.gamma=gamma;
      stretch.exception=exception;
      return(StretchAllChannels(&stretch,status));
    }
  /*
    Auto-level each channel.
  */
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ChannelType
      channel_mask;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    channel_mask=SetImageChannelMask(image,(ChannelType) (1UL << i));
    status&=(MagickStatusType) GetImageRange(image,&min,&max,exception);
    min+=black;
    max-=white;
    if (fabs(min-max) >= MagickEpsilon)
      status&=(MagickStatusType) LevelImage(image,min,max,gamma,exception);
    (void) SetImageChannelMask(image,channel_mask);
  }
  return(status != 0 ? MagickTrue : MagickFalse);
}
