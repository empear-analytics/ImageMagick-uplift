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
%               W   W   AAA   V   V  EEEEE  L      EEEEE  TTTTT               %
%               W   W  A   A  V   V  E      L      E        T                 %
%               W W W  AAAAA  V   V  EEE    L      EEE      T                 %
%               WW WW  A   A   V V   E      L      E        T                 %
%               W   W  A   A    V    EEEEE  LLLLL  EEEEE    T                 %
%                                                                             %
%                                                                             %
%                     MagickCore Wavelet Denoise Methods                      %
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

/*
  Typedef declarations.
*/
typedef struct _WaveletLine
{
  size_t
    stride,
    extent,
    scale;
} WaveletLine;

/*
  Define declarations.
*/
#define AddNoiseImageTag  "AddNoise/Image"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     W a v e l e t D e n o i s e I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WaveletDenoiseImage() removes noise from the image using a wavelet
%  transform.  The wavelet transform is a fast hierarchical scheme for
%  processing an image using a set of consecutive lowpass and high_pass filters,
%  followed by a decimation.  This results in a decomposition into different
%  scales which can be regarded as different “frequency bands”, determined by
%  the mother wavelet.  Adapted from dcraw.c by David Coffin.
%
%  The format of the WaveletDenoiseImage method is:
%
%      Image *WaveletDenoiseImage(const Image *image,const double threshold,
%        const double softness,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold: set the threshold for smoothing.
%
%    o softness: attenuate the smoothing threshold.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void HatTransform(const float *magick_restrict pixels,
  const WaveletLine *line,float *kernel)
{
  const size_t
    stride = line->stride,
    extent = line->extent,
    scale = line->scale;

  const float
    *magick_restrict p,
    *magick_restrict q,
    *magick_restrict r;

  ssize_t
    i;

  p=pixels;
  q=pixels+scale*stride;
  r=pixels+scale*stride;
  for (i=0; i < (ssize_t) scale; i++)
  {
    kernel[i]=0.25f*(*p+(*p)+(*q)+(*r));
    p+=(ptrdiff_t) stride;
    q-=stride;
    r+=(ptrdiff_t) stride;
  }
  for ( ; i < (ssize_t) (extent-scale); i++)
  {
    kernel[i]=0.25f*(2.0f*(*p)+*(p-scale*stride)+*(p+scale*stride));
    p+=(ptrdiff_t) stride;
  }
  q=p-scale*stride;
  r=pixels+stride*(extent-2);
  for ( ; i < (ssize_t) extent; i++)
  {
    kernel[i]=0.25f*(*p+(*p)+(*q)+(*r));
    p+=(ptrdiff_t) stride;
    q+=(ptrdiff_t) stride;
    r-=stride;
  }
}

static inline MagickBooleanType IsNonRGBPixelChannel(
  const PixelChannel pixel_channel)
{
  return(((pixel_channel != RedPixelChannel) &&
    (pixel_channel != GreenPixelChannel) &&
    (pixel_channel != BluePixelChannel)) ? MagickTrue : MagickFalse);
}

static void CopyWaveletRow(const Image *image,const float *magick_restrict p,
  float *magick_restrict q)
{
  ssize_t
    c;

  for (c=0; c < (ssize_t) image->columns; c++)
    *q++=(*p++);
}

static void CopyWaveletColumn(const Image *image,
  const float *magick_restrict p,float *magick_restrict q)
{
  ssize_t
    r;

  for (r=0; r < (ssize_t) image->rows; r++)
  {
    *q=(*p++);
    q+=(ptrdiff_t) image->columns;
  }
}

typedef struct _WaveletThreshold
{
  size_t
    high_pass,
    low_pass;

  MagickSizeType
    number_pixels;

  double
    magnitude,
    softness;
} WaveletThreshold;

static void ThresholdWaveletLevel(float *pixels,
  const WaveletThreshold *wavelet)
{
  const size_t
    high_pass = wavelet->high_pass,
    low_pass = wavelet->low_pass;

  const MagickSizeType
    number_pixels = wavelet->number_pixels;

  const double
    magnitude = wavelet->magnitude,
    softness = wavelet->softness;

  ssize_t
    i;

  for (i=0; i < (ssize_t) number_pixels; ++i)
  {
    pixels[(ssize_t) high_pass+i]-=pixels[(ssize_t) low_pass+i];
    if ((double) pixels[(ssize_t) high_pass+i] < -magnitude)
      pixels[(ssize_t) high_pass+i]+=(float) (magnitude-softness*magnitude);
    else
      if ((double) pixels[(ssize_t) high_pass+i] > magnitude)
        pixels[(ssize_t) high_pass+i]-=(float) (magnitude-softness*
          magnitude);
      else
        pixels[(ssize_t) high_pass+i]*=(float) softness;
    if (high_pass != 0)
      pixels[i]+=pixels[(ssize_t) high_pass+i];
  }
}

typedef struct _WaveletChannel
{
  const Image
    *image;

  CacheView
    *image_view;

  ssize_t
    channel;

  ExceptionInfo
    *exception;
} WaveletChannel;

static MagickBooleanType CopyWaveletChannel(
  const WaveletChannel *wavelet_channel,float *pixels,MagickBooleanType status)
{
  const Image
    *const image = wavelet_channel->image;

  CacheView
    *const image_view = wavelet_channel->image_view;

  const ssize_t
    channel = wavelet_channel->channel;

  ExceptionInfo
    *const exception = wavelet_channel->exception;

  ssize_t
    i,
    y;

  i=0;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    p=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      pixels[i++]=(float) p[channel];
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(status);
}

typedef struct _WaveletReconstruct
{
  const Image
    *image;

  Image
    *noise_image;

  CacheView
    *noise_view;

  PixelChannel
    pixel_channel;

  float
    *pixels;

  size_t
    low_pass;

  ExceptionInfo
    *exception;
} WaveletReconstruct;

static MagickBooleanType ReconstructWaveletChannel(
  const WaveletReconstruct *wavelet_reconstruct,MagickBooleanType status)
{
  const Image
    *const image = wavelet_reconstruct->image;

  Image
    *const noise_image = wavelet_reconstruct->noise_image;

  CacheView
    *const noise_view = wavelet_reconstruct->noise_view;

  const PixelChannel
    pixel_channel = wavelet_reconstruct->pixel_channel;

  float
    *const pixels = wavelet_reconstruct->pixels;

  const size_t
    low_pass = wavelet_reconstruct->low_pass;

  ExceptionInfo
    *const exception = wavelet_reconstruct->exception;

  ssize_t
    i,
    y;

  i=0;
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    MagickBooleanType
      sync;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    ssize_t
      offset;

    q=GetCacheViewAuthenticPixels(noise_view,0,y,noise_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    offset=GetPixelChannelOffset(noise_image,pixel_channel);
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      MagickRealType
        pixel;

      pixel=(MagickRealType) pixels[i]+(MagickRealType)
        pixels[(ssize_t) low_pass+i];
      q[offset]=ClampToQuantum(pixel);
      i++;
      q+=(ptrdiff_t) GetPixelChannels(noise_image);
    }
    sync=SyncCacheViewAuthenticPixels(noise_view,exception);
    if (sync == MagickFalse)
      status=MagickFalse;
  }
  return(status);
}

static MagickBooleanType ReportWaveletProgress(const Image *image,
  const ssize_t channel,MagickBooleanType status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,AddNoiseImageTag,(MagickOffsetType)
        channel,GetPixelChannels(image));
      if (proceed == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

typedef struct _WaveletDenoise
{
  const Image
    *image;

  size_t
    number_levels;

  MagickSizeType
    number_pixels;

  double
    threshold,
    softness;

  const float
    *noise_levels;

  float
    *pixels,
    *kernel;

  CacheView
    *image_view;

  Image
    *noise_image;

  CacheView
    *noise_view;

  ExceptionInfo
    *exception;
} WaveletDenoise;

static void ThresholdWaveletCoefficients(const WaveletDenoise *denoise,
  const ssize_t level,const size_t high_pass,const size_t low_pass)
{
  double
    magnitude;

  WaveletThreshold
    wavelet;

  magnitude=denoise->threshold*(double) denoise->noise_levels[level];
  wavelet.high_pass=high_pass;
  wavelet.low_pass=low_pass;
  wavelet.number_pixels=denoise->number_pixels;
  wavelet.magnitude=magnitude;
  wavelet.softness=denoise->softness;
  ThresholdWaveletLevel(denoise->pixels,&wavelet);
}

static size_t DecomposeWaveletLevels(const WaveletDenoise *denoise,
  const Image *image,size_t low_pass)
{
  size_t
    high_pass = 0;

  ssize_t
    level,
    y;

  high_pass=0;
  for (level=0; level < (ssize_t) denoise->number_levels; level++)
  {
    ssize_t
      x;

    low_pass=(size_t) (denoise->number_pixels*((level & 0x01)+1));
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp parallel for schedule(static,1) \
      magick_number_threads(image,image,image->rows,1)
#endif
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      const int
        id = GetOpenMPThreadId();

      float
        *magick_restrict p,
        *magick_restrict q;

      WaveletLine
        line;

      p=denoise->kernel+id*(ssize_t) image->columns;
      q=denoise->pixels+y*(ssize_t) image->columns;
      line.stride=1;
      line.extent=image->columns;
      line.scale=((size_t) 1UL << level);
      HatTransform(q+high_pass,&line,p);
      q+=(ptrdiff_t) low_pass;
      CopyWaveletRow(image,p,q);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp parallel for schedule(static,1) \
      magick_number_threads(image,image,image->columns,1)
#endif
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      const int
        id = GetOpenMPThreadId();

      float
        *magick_restrict p,
        *magick_restrict q;

      WaveletLine
        line;

      p=denoise->kernel+id*(ssize_t) image->rows;
      q=denoise->pixels+x+low_pass;
      line.stride=image->columns;
      line.extent=image->rows;
      line.scale=((size_t) 1UL << level);
      HatTransform(q,&line,p);
      CopyWaveletColumn(image,p,q);
    }
    /*
      To threshold, each coefficient is compared to a threshold value and
      attenuated / shrunk by some factor.
    */
    ThresholdWaveletCoefficients(denoise,level,high_pass,low_pass);
    high_pass=low_pass;
  }
  return(low_pass);
}

static MagickBooleanType DenoiseWaveletChannels(
  const WaveletDenoise *denoise,MagickBooleanType status)
{
  ssize_t
    channel;

  for (channel=0; channel < (ssize_t) GetPixelChannels(denoise->image);
       channel++)
  {
    size_t
      low_pass = 0;

    PixelChannel
      pixel_channel;

    PixelTrait
      traits;

    WaveletChannel
      wavelet_channel;

    WaveletReconstruct
      wavelet_reconstruct;

    if (status == MagickFalse)
      continue;
    traits=GetPixelChannelTraits(denoise->image,(PixelChannel) channel);
    if (traits == UndefinedPixelTrait)
      continue;
    pixel_channel=GetPixelChannelChannel(denoise->image,channel);
    if (IsNonRGBPixelChannel(pixel_channel) != MagickFalse)
      continue;
    /*
      Copy channel from image to wavelet pixel array.
    */
    wavelet_channel.image=denoise->image;
    wavelet_channel.image_view=denoise->image_view;
    wavelet_channel.channel=channel;
    wavelet_channel.exception=denoise->exception;
    status=CopyWaveletChannel(&wavelet_channel,denoise->pixels,status);
    /*
      Low pass filter outputs are called approximation kernel & high pass
      filters are referred to as detail kernel. The detail kernel
      have high values in the noisy parts of the signal.
    */
    low_pass=DecomposeWaveletLevels(denoise,denoise->image,low_pass);
    /*
      Reconstruct image from the thresholded wavelet kernel.
    */
    wavelet_reconstruct.image=denoise->image;
    wavelet_reconstruct.noise_image=denoise->noise_image;
    wavelet_reconstruct.noise_view=denoise->noise_view;
    wavelet_reconstruct.pixel_channel=pixel_channel;
    wavelet_reconstruct.pixels=denoise->pixels;
    wavelet_reconstruct.low_pass=low_pass;
    wavelet_reconstruct.exception=denoise->exception;
    status=ReconstructWaveletChannel(&wavelet_reconstruct,status);
    status=ReportWaveletProgress(denoise->image,channel,status);
  }
  return(status);
}

static size_t CountWaveletLevels(const Image *image,size_t number_levels)
{
  while ((number_levels > 0) &&
         (((size_t) 1 << (number_levels-1)) >= MagickMin(image->columns,image->rows)))
    number_levels--;
  return(number_levels);
}

static MagickBooleanType CloneWaveletImage(const Image *image,
  Image **noise_image,ExceptionInfo *exception)
{
  *noise_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*noise_image == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(*noise_image,DirectClass,exception) == MagickFalse)
    {
      *noise_image=DestroyImage(*noise_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void RelinquishWaveletBuffers(float *kernel,MemoryInfo *pixels_info)
{
  if (kernel != (float *) NULL)
    kernel=(float *) RelinquishMagickMemory(kernel);
  if (pixels_info != (MemoryInfo *) NULL)
    pixels_info=RelinquishVirtualMemory(pixels_info);
}

static inline MagickBooleanType IsWaveletBufferMissing(
  const MemoryInfo *pixels_info,const float *kernel)
{
  return(((pixels_info == (MemoryInfo *) NULL) || (kernel == (float *) NULL)) ?
    MagickTrue : MagickFalse);
}

static void DestroyWaveletResources(CacheView *image_view,
  CacheView *noise_view,float *kernel,MemoryInfo *pixels_info)
{
  noise_view=DestroyCacheView(noise_view);
  image_view=DestroyCacheView(image_view);
  kernel=(float *) RelinquishMagickMemory(kernel);
  pixels_info=RelinquishVirtualMemory(pixels_info);
}

typedef struct _WaveletDenoiseInput
{
  const Image
    *image;

  Image
    *noise_image;

  size_t
    number_levels;

  MagickSizeType
    number_pixels;

  double
    threshold,
    softness;

  const float
    *noise_levels;

  float
    *pixels,
    *kernel;

  MemoryInfo
    *pixels_info;

  ExceptionInfo
    *exception;
} WaveletDenoiseInput;

static Image *DenoiseWaveletImage(const WaveletDenoiseInput *input)
{
  CacheView
    *image_view,
    *noise_view;

  Image
    *noise_image = input->noise_image;

  MagickBooleanType
    status;

  WaveletDenoise
    denoise;

  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(input->image,input->exception);
  noise_view=AcquireAuthenticCacheView(noise_image,input->exception);
  denoise.image=input->image;
  denoise.number_levels=input->number_levels;
  denoise.number_pixels=input->number_pixels;
  denoise.threshold=input->threshold;
  denoise.softness=input->softness;
  denoise.noise_levels=input->noise_levels;
  denoise.pixels=input->pixels;
  denoise.kernel=input->kernel;
  denoise.image_view=image_view;
  denoise.noise_image=noise_image;
  denoise.noise_view=noise_view;
  denoise.exception=input->exception;
  status=DenoiseWaveletChannels(&denoise,status);
  DestroyWaveletResources(image_view,noise_view,input->kernel,
    input->pixels_info);
  if (status == MagickFalse)
    noise_image=DestroyImage(noise_image);
  return(noise_image);
}

MagickExport Image *WaveletDenoiseImage(const Image *image,
  const double threshold,const double softness,ExceptionInfo *exception)
{
  float
    *kernel,
    *pixels;

  Image
    *noise_image;

  MagickSizeType
    number_pixels;

  MemoryInfo
    *pixels_info;

  size_t
    number_levels = 5;

  WaveletDenoiseInput
    input;

  static const float
    noise_levels[] = { 0.8002f, 0.2735f, 0.1202f, 0.0585f, 0.0291f, 0.0152f,
      0.0080f, 0.0044f };

  /*
    Initialize noise image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  number_pixels=(MagickSizeType) image->columns*image->rows;
  number_levels=CountWaveletLevels(image,number_levels);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (number_levels >= 5)
    {
       noise_image=AccelerateWaveletDenoiseImage(image,threshold,exception);
       if (noise_image != (Image *) NULL)
         return(noise_image);
    }
#endif
  if (CloneWaveletImage(image,&noise_image,exception) == MagickFalse)
    return((Image *) NULL);
  if (AcquireMagickResource(WidthResource,4*image->columns) == MagickFalse)
    {
      noise_image=DestroyImage(noise_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  pixels_info=AcquireVirtualMemory(4*image->columns,image->rows*
    sizeof(*pixels));
  kernel=(float *) AcquireQuantumMemory(MagickMax(image->rows,image->columns)+1,
    GetOpenMPMaximumThreads()*sizeof(*kernel));
  if (IsWaveletBufferMissing(pixels_info,kernel) != MagickFalse)
    {
      RelinquishWaveletBuffers(kernel,pixels_info);
      noise_image=DestroyImage(noise_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  pixels=(float *) GetVirtualMemoryBlob(pixels_info);
  input.image=image;
  input.noise_image=noise_image;
  input.number_levels=number_levels;
  input.number_pixels=number_pixels;
  input.threshold=threshold;
  input.softness=softness;
  input.noise_levels=noise_levels;
  input.pixels=pixels;
  input.kernel=kernel;
  input.pixels_info=pixels_info;
  input.exception=exception;
  return(DenoiseWaveletImage(&input));
}
