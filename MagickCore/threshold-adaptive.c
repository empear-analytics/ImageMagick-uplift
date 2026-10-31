/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        TTTTT  H   H  RRRR   EEEEE  SSSSS  H   H   OOO   L      DDDD         %
%          T    H   H  R   R  E      SS     H   H  O   O  L      D   D        %
%          T    HHHHH  RRRR   EEE     SSS   HHHHH  O   O  L      D   D        %
%          T    H   H  R R    E         SS  H   H  O   O  L      D   D        %
%          T    H   H  R  R   EEEEE  SSSSS  H   H   OOO   LLLLL  DDDD         %
%                                                                             %
%            AAA   DDDD    AAA   PPPP   TTTTT  IIIII  V   V  EEEEE            %
%           A   A  D   D  A   A  P   P    T      I    V   V  E                %
%           AAAAA  D   D  AAAAA  PPPP     T      I    V   V  EEE              %
%           A   A  D   D  A   A  P        T      I     V V   E                %
%           A   A  DDDD   A   A  P        T    IIIII    V    EEEEE            %
%                                                                             %
%                                                                             %
%                    MagickCore Adaptive Threshold Method                     %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
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
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/configure.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/shear.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
  Typedef declarations.
*/
typedef struct _AdaptiveThresholdPixelInfo
{
  const Image
    *image,
    *threshold_image;

  size_t
    width,
    height;

  MagickSizeType
    number_pixels;

  double
    bias,
    *channel_bias,
    *channel_sum;
} AdaptiveThresholdPixelInfo;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A d a p t i v e T h r e s h o l d I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AdaptiveThresholdImage() selects an individual threshold for each pixel
%  based on the range of intensity values in its local neighborhood.  This
%  allows for thresholding of an image whose global intensity histogram
%  doesn't contain distinctive peaks.
%
%  The format of the AdaptiveThresholdImage method is:
%
%      Image *AdaptiveThresholdImage(const Image *image,const size_t width,
%        const size_t height,const double bias,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o width: the width of the local neighborhood.
%
%    o height: the height of the local neighborhood.
%
%    o bias: the mean bias.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SumThresholdWindow(const Image *image,
  const Quantum *magick_restrict p,const ssize_t i,const size_t width,
  const size_t height,double *channel_bias,double *channel_sum)
{
  const Quantum
    *magick_restrict pixels;

  ssize_t
    u,
    v;

  pixels=p;
  *channel_bias=0.0;
  *channel_sum=0.0;
  for (v=0; v < (ssize_t) height; v++)
  {
    for (u=0; u < (ssize_t) width; u++)
    {
      if (u == (ssize_t) (width-1))
        *channel_bias+=(double) pixels[i];
      *channel_sum+=(double) pixels[i];
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(ptrdiff_t) GetPixelChannels(image)*image->columns;
  }
}

static void SlideThresholdWindow(const AdaptiveThresholdPixelInfo *pixel_info,
  const Quantum *magick_restrict p,const ssize_t i,const PixelChannel channel)
{
  const Image
    *const image = pixel_info->image;

  const size_t
    width = pixel_info->width,
    height = pixel_info->height;

  double
    *const channel_bias = pixel_info->channel_bias,
    *const channel_sum = pixel_info->channel_sum;

  const Quantum
    *magick_restrict pixels;

  ssize_t
    v;

  channel_sum[channel]-=channel_bias[channel];
  channel_bias[channel]=0.0;
  pixels=p;
  for (v=0; v < (ssize_t) height; v++)
  {
    channel_bias[channel]+=(double) pixels[i];
    pixels+=(width-1)*GetPixelChannels(image);
    channel_sum[channel]+=(double) pixels[i];
    pixels+=(ptrdiff_t) GetPixelChannels(image)*(image->columns+1);
  }
}

static void AdaptiveThresholdPixel(
  const AdaptiveThresholdPixelInfo *pixel_info,
  const Quantum *magick_restrict p,Quantum *magick_restrict q,
  const ssize_t center)
{
  const Image
    *const image = pixel_info->image,
    *const threshold_image = pixel_info->threshold_image;

  const MagickSizeType
    number_pixels = pixel_info->number_pixels;

  const double
    bias = pixel_info->bias;

  double
    *const channel_sum = pixel_info->channel_sum;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      mean;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait threshold_traits=GetPixelChannelTraits(threshold_image,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (threshold_traits == UndefinedPixelTrait))
      continue;
    if ((threshold_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(threshold_image,channel,p[center+i],q);
        continue;
      }
    SlideThresholdWindow(pixel_info,p,i,channel);
    mean=(double) (channel_sum[channel]/number_pixels+bias);
    SetPixelChannel(threshold_image,channel,(Quantum) ((double)
      p[center+i] <= mean ? 0 : QuantumRange),q);
  }
}

typedef struct _AdaptiveThresholdInfo
{
  const Image
    *image;

  Image
    *threshold_image;

  size_t
    width,
    height;

  MagickSizeType
    number_pixels;

  double
    bias;
} AdaptiveThresholdInfo;

static void AdaptiveThresholdRow(const AdaptiveThresholdInfo *info,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = info->image;

  Image
    *const threshold_image = info->threshold_image;

  const size_t
    width = info->width,
    height = info->height;

  const MagickSizeType
    number_pixels = info->number_pixels;

  const double
    bias = info->bias;

  double
    channel_bias[MaxPixelChannels],
    channel_sum[MaxPixelChannels];

  const AdaptiveThresholdPixelInfo
    pixel_info = { image, threshold_image, width, height, number_pixels,
      bias, channel_bias, channel_sum };

  ssize_t
    center,
    i,
    x;

  center=(ssize_t) GetPixelChannels(image)*((ssize_t) image->columns+
    (ssize_t) width)*((ssize_t) height/2L)+(ssize_t) GetPixelChannels(image)*
    ((ssize_t) width/2);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait threshold_traits=GetPixelChannelTraits(threshold_image,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (threshold_traits == UndefinedPixelTrait))
      continue;
    if ((threshold_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(threshold_image,channel,p[center+i],q);
        continue;
      }
    SumThresholdWindow(image,p,i,width,height,&channel_bias[channel],
      &channel_sum[channel]);
  }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    AdaptiveThresholdPixel(&pixel_info,p,q,center);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(threshold_image);
  }
}

MagickExport Image *AdaptiveThresholdImage(const Image *image,
  const size_t width,const size_t height,const double bias,
  ExceptionInfo *exception)
{
#define AdaptiveThresholdImageTag  "AdaptiveThreshold/Image"

  CacheView
    *image_view,
    *threshold_view;

  Image
    *threshold_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MagickSizeType
    number_pixels;

  ssize_t
    y;

  /*
    Initialize threshold image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  threshold_image=CloneImage(image,0,0,MagickTrue,exception);
  if (threshold_image == (Image *) NULL)
    return((Image *) NULL);
  if ((width == 0) || (height == 0))
    return(threshold_image);
  status=SetImageStorageClass(threshold_image,DirectClass,exception);
  if (status == MagickFalse)
    {
      threshold_image=DestroyImage(threshold_image);
      return((Image *) NULL);
    }
  /*
    Threshold image.
  */
  status=MagickTrue;
  progress=0;
  number_pixels=(MagickSizeType) width*height;
  image_view=AcquireVirtualCacheView(image,exception);
  threshold_view=AcquireAuthenticCacheView(threshold_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,threshold_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    AdaptiveThresholdInfo
      info;

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-((ssize_t) width/2L),y-(ssize_t)
      (height/2L),image->columns+width,height,exception);
    q=QueueCacheViewAuthenticPixels(threshold_view,0,y,threshold_image->columns,
      1,exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    info.image=image;
    info.threshold_image=threshold_image;
    info.width=width;
    info.height=height;
    info.number_pixels=number_pixels;
    info.bias=bias;
    AdaptiveThresholdRow(&info,p,q);
    if (SyncCacheViewAuthenticPixels(threshold_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,AdaptiveThresholdImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  threshold_image->type=image->type;
  threshold_view=DestroyCacheView(threshold_view);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    threshold_image=DestroyImage(threshold_image);
  return(threshold_image);
}
