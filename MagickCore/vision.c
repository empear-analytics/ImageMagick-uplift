/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   V   V  IIIII  SSSSS  IIIII   OOO   N   N                  %
%                   V   V    I    SS       I    O   O  NN  N                  %
%                   V   V    I     SSS     I    O   O  N N N                  %
%                    V V     I       SS    I    O   O  N  NN                  %
%                     V    IIIII  SSSSS  IIIII   OOO   N   N                  %
%                                                                             %
%                                                                             %
%                      MagickCore Computer Vision Methods                     %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               September 2014                                %
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

#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/opencl-private.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resource_.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/vision.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I n t e g r a l I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IntegralImage() returns the sum of values (pixel values) in the image.
%
%  The format of the IntegralImage method is:
%
%      Image *IntegralImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _IntegralChannel
{
  Image
    *integral_image;

  const Quantum
    *p;

  ssize_t
    x,
    y,
    i;
} IntegralChannel;

static void IntegratePixelChannel(Quantum *q,
  const IntegralChannel *integral_channel)
{
  Image
    *const integral_image = integral_channel->integral_image;

  const Quantum
    *const p = integral_channel->p;

  const ssize_t
    x = integral_channel->x,
    y = integral_channel->y,
    i = integral_channel->i;

  double
    sum;

  PixelTrait traits = GetPixelChannelTraits(integral_image,
    (PixelChannel) i);
  if (traits == UndefinedPixelTrait)
    return;
  if ((traits & CopyPixelTrait) != 0)
    return;
  sum=(double) q[i];
  if (x > 0)
    sum+=(double) (q-GetPixelChannels(integral_image))[i];
  if (y > 0)
    sum+=(double) p[i];
  if ((x > 0) && (y > 0))
    sum-=(double) (p-GetPixelChannels(integral_image))[i];
  q[i]=ClampToQuantum(sum);
}


static void IntegrateRow(Image *integral_image,const Quantum *p,Quantum *q,
  const ssize_t y)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) integral_image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(integral_image); i++)
    {
      IntegralChannel
        integral_channel;

      integral_channel.integral_image=integral_image;
      integral_channel.p=p;
      integral_channel.x=x;
      integral_channel.y=y;
      integral_channel.i=i;
      IntegratePixelChannel(q,&integral_channel);
    }
    p+=(ptrdiff_t) GetPixelChannels(integral_image);
    q+=(ptrdiff_t) GetPixelChannels(integral_image);
  }
}


static Image *DestroyIntegralViews(CacheView *image_view,
  CacheView *integral_view,Image *integral_image,const MagickBooleanType status)
{
  integral_view=DestroyCacheView(integral_view);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    integral_image=DestroyImage(integral_image);
  return(integral_image);
}


#define IntegralImageTag  "Integral/Image"

typedef struct _CCIntegralRow
{
  const Image
    *image;

  Image
    *integral_image;

  CacheView
    *integral_view;

  ExceptionInfo
    *exception;
} CCIntegralRow;

static void IntegrateImageRow(const CCIntegralRow *integral_row,const ssize_t y,
  MagickOffsetType *progress,MagickBooleanType *status)
{
  const Image
    *const image = integral_row->image;

  Image
    *const integral_image = integral_row->integral_image;

  CacheView
    *const integral_view = integral_row->integral_view;

  ExceptionInfo
    *const exception = integral_row->exception;

  const Quantum
    *magick_restrict p;

  MagickBooleanType
    sync;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(integral_view,0,y-1,integral_image->columns,1,
    exception);
  q=GetCacheViewAuthenticPixels(integral_view,0,y,integral_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (p == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  IntegrateRow(integral_image,p,q,y);
  sync=SyncCacheViewAuthenticPixels(integral_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      (*progress)++;
      proceed=SetImageProgress(integral_image,IntegralImageTag,*progress,
        integral_image->rows);
      if (proceed == MagickFalse)
        (*status)=MagickFalse;
    }
}

MagickExport Image *IntegralImage(const Image *image,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *integral_view;

  CCIntegralRow
    integral_row;

  Image
    *integral_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Initialize integral image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  integral_image=CloneImage(image,0,0,MagickTrue,exception);
  if (integral_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(integral_image,DirectClass,exception) == MagickFalse)
    {
      integral_image=DestroyImage(integral_image);
      return((Image *) NULL);
    }
  /*
    Calculate the sum of values (pixel values) in the image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(integral_image,exception);
  integral_view=AcquireAuthenticCacheView(integral_image,exception);
  integral_row.image=image;
  integral_row.integral_image=integral_image;
  integral_row.integral_view=integral_view;
  integral_row.exception=exception;
  for (y=0; y < (ssize_t) integral_image->rows; y++)
    IntegrateImageRow(&integral_row,y,&progress,&status);
  return(DestroyIntegralViews(image_view,integral_view,integral_image,status));
}
