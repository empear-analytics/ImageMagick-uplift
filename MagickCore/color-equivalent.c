/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                       CCCC   OOO   L       OOO   RRRR                       %
%                      C      O   O  L      O   O  R   R                      %
%                      C      O   O  L      O   O  RRRR                       %
%                      C      O   O  L      O   O  R R                        %
%                       CCCC   OOO   LLLLL   OOO   R  R                       %
%                                                                             %
%    EEEEE   QQQ   U   U  IIIII  V   V   AAA   L      EEEEE  N   N  TTTTT     %
%    E      Q   Q  U   U    I    V   V  A   A  L      E      NN  N    T       %
%    EEE    Q   Q  U   U    I    V   V  AAAAA  L      EEE    N N N    T       %
%    E      Q  QQ  U   U    I     V V   A   A  L      E      N  NN    T       %
%    EEEEE   QQQQ   UUU   IIIII    V    A   A  LLLLL  EEEEE  N   N    T       %
%                                                                             %
%                                                                             %
%                           Equivalent Image Method                           %
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
%  We use linked-lists because splay-trees do not currently support duplicate
%  key / value pairs (.e.g X11 green compliance and SVG green compliance).
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/cache.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I s E q u i v a l e n t I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsEquivalentImage() returns true if the target is similar to a region of the
%  image.
%
%  The format of the IsEquivalentImage method is:
%
%      MagickBooleanType IsEquivalentImage(const Image *image,
%        const Image *target_image,ssize_t *x_offset,ssize_t *y_offset,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o target_image: the target image.
%
%    o x_offset: On input the starting x position to search for a match;
%      on output the x position of the first match found.
%
%    o y_offset: On input the starting y position to search for a match;
%      on output the y position of the first match found.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType IsEquivalentImage(const Image *image,
  const Image *target_image,ssize_t *x_offset,ssize_t *y_offset,
  ExceptionInfo *exception)
{
#define SearchImageText  "  Searching image...  "

  CacheView
    *image_view,
    *target_view;

  MagickBooleanType
    status;

  PixelInfo
    target,
    pixel;

  const Quantum
    *p,
    *q;

  ssize_t
    i,
    x;

  ssize_t
    j,
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(target_image != (Image *) NULL);
  assert(target_image->signature == MagickCoreSignature);
  assert(x_offset != (ssize_t *) NULL);
  assert(y_offset != (ssize_t *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  x=0;
  status=MagickTrue;
  GetPixelInfo(image,&pixel);
  GetPixelInfo(image,&target);
  image_view=AcquireVirtualCacheView(image,exception);
  target_view=AcquireVirtualCacheView(target_image,exception);
  for (y=(*y_offset); y < (ssize_t) image->rows; y++)
  {
    for (x=y == 0 ? *x_offset : 0; x < (ssize_t) image->columns; x++)
    {
      for (j=0; j < (ssize_t) target_image->rows; j++)
      {
        for (i=0; i < (ssize_t) target_image->columns; i++)
        {
          p=GetCacheViewVirtualPixels(image_view,x+i,y+j,1,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          GetPixelInfoPixel(image,p,&pixel);
          q=GetCacheViewVirtualPixels(target_view,i,j,1,1,exception);
          if (q == (const Quantum *) NULL)
            break;
          GetPixelInfoPixel(image,q,&target);
          if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
            break;
        }
        if (i < (ssize_t) target_image->columns)
          break;
      }
      if (j == (ssize_t) target_image->rows)
        break;
    }
    if (x < (ssize_t) image->columns)
      break;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

        proceed=SetImageProgress(image,SearchImageText,(MagickOffsetType) y,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  target_view=DestroyCacheView(target_view);
  image_view=DestroyCacheView(image_view);
  *x_offset=x;
  *y_offset=y;
  if (status == MagickFalse)
    return(status);
  return(y < (ssize_t) image->rows ? MagickTrue : MagickFalse);
}
