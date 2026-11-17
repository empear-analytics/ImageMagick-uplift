/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  V   V  IIIII  SSSSS  U   U   AAA   L                       %
%                  V   V    I    SS     U   U  A   A  L                       %
%                  V   V    I     SSS   U   U  AAAAA  L                       %
%                   V V     I       SS  U   U  A   A  L                       %
%                    V    IIIII  SSSSS   UUU   A   A  LLLLL                   %
%                                                                             %
%               EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT  SSSSS               %
%               E      F      F      E      C        T    SS                  %
%               EEE    FFF    FFF    EEE    C        T     SSS                %
%               E      F      F      E      C        T       SS               %
%               EEEEE  F      F      EEEEE   CCCC    T    SSSSS               %
%                                                                             %
%        BBBB   L      U   U  EEEEE  SSSSS  H   H  IIIII  FFFFF  TTTTT        %
%        B   B  L      U   U  E      SS     H   H    I    F        T          %
%        BBBB   L      U   U  EEE     SSS   HHHHH    I    FFF      T          %
%        B   B  L      U   U  E         SS  H   H    I    F        T          %
%        BBBB   LLLLL   UUU   EEEEE  SSSSS  H   H  IIIII  F        T          %
%                                                                             %
%                                                                             %
%                        MagickCore Blue Shift Method                         %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
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
#include "MagickCore/cache-private.h"
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
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/visual-effects.h"
#include "MagickCore/visual-effects-pixel-methods-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B l u e S h i f t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlueShiftImage() mutes the colors of the image to simulate a scene at
%  nighttime in the moonlight.
%
%  The format of the BlueShiftImage method is:
%
%      Image *BlueShiftImage(const Image *image,const double factor,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o factor: the shift factor.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Quantum GetPixelMinimumRGB(const Image *image,
  const Quantum *magick_restrict p)
{
  Quantum
    quantum;

  quantum=GetPixelRed(image,p);
  if (GetPixelGreen(image,p) < quantum)
    quantum=GetPixelGreen(image,p);
  if (GetPixelBlue(image,p) < quantum)
    quantum=GetPixelBlue(image,p);
  return(quantum);
}

static Quantum GetPixelMaximumRGB(const Image *image,
  const Quantum *magick_restrict p)
{
  Quantum
    quantum;

  quantum=GetPixelRed(image,p);
  if (GetPixelGreen(image,p) > quantum)
    quantum=GetPixelGreen(image,p);
  if (GetPixelBlue(image,p) > quantum)
    quantum=GetPixelBlue(image,p);
  return(quantum);
}

typedef struct _BlueShiftRow
{
  const Image
    *image;

  Image
    *shift_image;

  CacheView
    *image_view,
    *shift_view;

  double
    factor;

  ExceptionInfo
    *exception;
} BlueShiftRow;

static MagickBooleanType BlueShiftImageRow(const BlueShiftRow *shift_row,
  const ssize_t y,MagickBooleanType *status)
{
  const Image
    *const image = shift_row->image;

  Image
    *const shift_image = shift_row->shift_image;

  CacheView
    *const image_view = shift_row->image_view,
    *const shift_view = shift_row->shift_view;

  const double
    factor = shift_row->factor;

  ExceptionInfo
    *const exception = shift_row->exception;

  MagickBooleanType
    sync;

  PixelInfo
    pixel;

  Quantum
    quantum;

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=QueueCacheViewAuthenticPixels(shift_view,0,y,shift_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    quantum=GetPixelMinimumRGB(image,p);
    pixel.red=0.5*((double) GetPixelRed(image,p)+factor*(double) quantum);
    pixel.green=0.5*((double) GetPixelGreen(image,p)+factor*(double) quantum);
    pixel.blue=0.5*((double) GetPixelBlue(image,p)+factor*(double) quantum);
    quantum=GetPixelMaximumRGB(image,p);
    pixel.red=0.5*(pixel.red+factor*(double) quantum);
    pixel.green=0.5*(pixel.green+factor*(double) quantum);
    pixel.blue=0.5*(pixel.blue+factor*(double) quantum);
    SetPixelRed(shift_image,ClampToQuantum(pixel.red),q);
    SetPixelGreen(shift_image,ClampToQuantum(pixel.green),q);
    SetPixelBlue(shift_image,ClampToQuantum(pixel.blue),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(shift_image);
  }
  sync=SyncCacheViewAuthenticPixels(shift_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport Image *BlueShiftImage(const Image *image,const double factor,
  ExceptionInfo *exception)
{
#define BlueShiftImageTag  "BlueShift/Image"

  CacheView
    *image_view,
    *shift_view;

  Image
    *shift_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Allocate blue shift image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&shift_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Blue-shift DirectClass image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  shift_view=AcquireAuthenticCacheView(shift_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,shift_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    BlueShiftRow
      shift_row;

    shift_row.image=image;
    shift_row.shift_image=shift_image;
    shift_row.image_view=image_view;
    shift_row.shift_view=shift_view;
    shift_row.factor=factor;
    shift_row.exception=exception;
    if (BlueShiftImageRow(&shift_row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,BlueShiftImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  shift_view=DestroyCacheView(shift_view);
  if (status == MagickFalse)
    shift_image=DestroyImage(shift_image);
  return(shift_image);
}
