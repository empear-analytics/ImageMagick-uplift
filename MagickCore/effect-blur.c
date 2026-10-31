/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT                   %
%                  E      F      F      E      C        T                     %
%                  EEE    FFF    FFF    EEE    C        T                     %
%                  E      F      F      E      C        T                     %
%                  EEEEE  F      F      EEEEE   CCCC    T                     %
%                                                                             %
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R   R                          %
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R R                            %
%                         BBBB   LLLLL   UUU   R  R                           %
%                                                                             %
%                                                                             %
%                MagickCore Image Blur and Convolution Methods                %
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
#include "MagickCore/accelerate-private.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
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
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/shear.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/threshold.h"
#include "MagickCore/utility-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A d a p t i v e B l u r I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AdaptiveBlurImage() adaptively blurs the image by blurring less
%  intensely near image edges and more intensely far from edges.  We blur the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and AdaptiveBlurImage() selects a suitable radius for you.
%
%  The format of the AdaptiveBlurImage method is:
%
%      Image *AdaptiveBlurImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Laplacian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ssize_t GetAdaptiveKernelIndex(const Image *edge_image,
  const Quantum *r,const size_t width)
{
  ssize_t
    j;

  j=CastDoubleToSsizeT(ceil((double) width*(1.0-QuantumScale*
    GetPixelIntensity(edge_image,r))-0.5));
  if (j < 0)
    j=0;
  else
    if (j > (ssize_t) width)
      j=(ssize_t) width;
  if ((j & 0x01) != 0)
    j--;
  return(j);
}

typedef struct _AdaptiveSum
{
  const Image
    *image;

  size_t
    width;

  ssize_t
    j,
    i;
} AdaptiveSum;

typedef struct _AdaptiveSumTargets
{
  double
    *pixel,
    *gamma;

  const double
    *magick_restrict *k;

  const Quantum
    *magick_restrict *pixels;
} AdaptiveSumTargets;

static void SumAdaptivePixels(const AdaptiveSum *sum,
  const AdaptiveSumTargets *targets)
{
  const Image
    *const image = sum->image;

  const size_t
    width = sum->width;

  const ssize_t
    j = sum->j,
    i = sum->i;

  double
    *const pixel = targets->pixel,
    *const gamma = targets->gamma;

  const double
    *magick_restrict *const k = targets->k;

  const Quantum
    *magick_restrict *const pixels = targets->pixels;

  ssize_t
    u,
    v;

  for (v=0; v < ((ssize_t) width-j); v++)
  {
    for (u=0; u < ((ssize_t) width-j); u++)
    {
      *pixel+=(**k)*(double) (*pixels)[i];
      *gamma+=(**k);
      (*k)++;
      *pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
}

static void SumAdaptiveBlendedPixels(const AdaptiveSum *sum,
  const AdaptiveSumTargets *targets)
{
  double
    *const pixel = targets->pixel,
    *const gamma = targets->gamma;

  const double
    *magick_restrict *const k = targets->k;

  const Quantum
    *magick_restrict *const pixels = targets->pixels;

  double
    alpha;

  ssize_t
    u,
    v;

  for (v=0; v < ((ssize_t) sum->width-sum->j); v++)
  {
    for (u=0; u < ((ssize_t) sum->width-sum->j); u++)
    {
      alpha=(double) (QuantumScale*(double) GetPixelAlpha(sum->image,*pixels));
      *pixel+=(**k)*alpha*(double) (*pixels)[sum->i];
      *gamma+=(**k)*alpha;
      (*k)++;
      *pixels+=(ptrdiff_t) GetPixelChannels(sum->image);
    }
  }
}

typedef struct _AdaptiveChannel
{
  const Image
    *image;

  Image
    *blur_image;

  double
    **kernel;

  const Quantum
    *p;

  size_t
    width;

  ssize_t
    j,
    center;
} AdaptiveChannel;

static void BlurAdaptiveChannel(const AdaptiveChannel *adaptive,
  const ssize_t i,Quantum *magick_restrict q)
{
  const double
    *magick_restrict k;

  const Quantum
    *magick_restrict pixels;

  double
    gamma,
    pixel;

  AdaptiveSum
    sum;

  AdaptiveSumTargets
    targets;

  PixelChannel
    channel;

  PixelTrait
    blur_traits,
    traits;

  channel=GetPixelChannelChannel(adaptive->image,i);
  traits=GetPixelChannelTraits(adaptive->image,channel);
  blur_traits=GetPixelChannelTraits(adaptive->blur_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (blur_traits == UndefinedPixelTrait))
    return;
  if ((blur_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(adaptive->blur_image,channel,
        adaptive->p[adaptive->center+i],q);
      return;
    }
  k=adaptive->kernel[adaptive->j];
  pixels=adaptive->p;
  pixel=0.0;
  gamma=0.0;
  if ((blur_traits & BlendPixelTrait) == 0)
    {
      /*
        No alpha blending.
      */
      sum.image=adaptive->image;
      sum.width=adaptive->width;
      sum.j=adaptive->j;
      sum.i=i;
      targets.pixel=(&pixel);
      targets.gamma=(&gamma);
      targets.k=(&k);
      targets.pixels=(&pixels);
      SumAdaptivePixels(&sum,&targets);
      gamma=MagickSafeReciprocal(gamma);
      SetPixelChannel(adaptive->blur_image,channel,ClampToQuantum(gamma*pixel),
        q);
      return;
    }
  /*
    Alpha blending.
  */
  sum.image=adaptive->image;
  sum.width=adaptive->width;
  sum.j=adaptive->j;
  sum.i=i;
  targets.pixel=(&pixel);
  targets.gamma=(&gamma);
  targets.k=(&k);
  targets.pixels=(&pixels);
  SumAdaptiveBlendedPixels(&sum,&targets);
  gamma=MagickSafeReciprocal(gamma);
  SetPixelChannel(adaptive->blur_image,channel,ClampToQuantum(gamma*pixel),q);
}

static void RelinquishAdaptiveKernels(double **kernel,ssize_t w)
{
  for (w-=2; w >= 0; w-=2)
    kernel[w]=(double *) RelinquishAlignedMemory(kernel[w]);
}

#define AdaptiveBlurImageTag  "Convolve/Image"
#define MagickSigma  (fabs(sigma) < MagickEpsilon ? MagickEpsilon : sigma)

static MagickBooleanType AcquireAdaptiveBlurKernel(double **kernel,
  const size_t width,const ssize_t w,const double sigma)
{
  double
    normalize;

  ssize_t
    j,
    k,
    u,
    v;

  kernel[w]=(double *) MagickAssumeAligned(AcquireAlignedMemory(
    (width-(size_t) w),(width-(size_t) w)*sizeof(**kernel)));
  if (kernel[w] == (double *) NULL)
    return(MagickFalse);
  normalize=0.0;
  j=((ssize_t) width-w-1)/2;
  k=0;
  for (v=(-j); v <= j; v++)
  {
    for (u=(-j); u <= j; u++)
    {
      kernel[w][k]=(double) (exp(-((double) u*u+v*v)/(2.0*MagickSigma*
        MagickSigma))/(2.0*MagickPI*MagickSigma*MagickSigma));
      normalize+=kernel[w][k];
      k++;
    }
  }
  kernel[w][(k-1)/2]+=(double) (1.0-normalize);
  if (sigma < MagickEpsilon)
    kernel[w][(k-1)/2]=1.0;
  return(MagickTrue);
}

MagickExport Image *AdaptiveBlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  CacheView
    *blur_view,
    *edge_view,
    *image_view;

  double
    **kernel;

  Image
    *blur_image,
    *edge_image,
    *gaussian_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  size_t
    width;

  ssize_t
    w,
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  if (fabs(sigma) < MagickEpsilon)
    return(blur_image);
  if (SetImageStorageClass(blur_image,DirectClass,exception) == MagickFalse)
    {
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  /*
    Edge detect the image brightness channel, level, blur, and level again.
  */
  edge_image=EdgeImage(image,radius,exception);
  if (edge_image == (Image *) NULL)
    {
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  (void) AutoLevelImage(edge_image,exception);
  gaussian_image=BlurImage(edge_image,radius,sigma,exception);
  if (gaussian_image != (Image *) NULL)
    {
      edge_image=DestroyImage(edge_image);
      edge_image=gaussian_image;
    }
  (void) AutoLevelImage(edge_image,exception);
  /*
    Create a set of kernels from maximum (radius,sigma) to minimum.
  */
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double **) MagickAssumeAligned(AcquireAlignedMemory((size_t) width,
    sizeof(*kernel)));
  if (kernel == (double **) NULL)
    {
      edge_image=DestroyImage(edge_image);
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) memset(kernel,0,(size_t) width*sizeof(*kernel));
  for (w=0; w < (ssize_t) width; w+=2)
  {
    if (AcquireAdaptiveBlurKernel(kernel,width,w,sigma) == MagickFalse)
      break;
  }
  if (w < (ssize_t) width)
    {
      RelinquishAdaptiveKernels(kernel,w);
      kernel=(double **) RelinquishAlignedMemory(kernel);
      edge_image=DestroyImage(edge_image);
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Adaptively blur image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  edge_view=AcquireVirtualCacheView(edge_image,exception);
  blur_view=AcquireAuthenticCacheView(blur_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,blur_image,blur_image->rows,1)
#endif
  for (y=0; y < (ssize_t) blur_image->rows; y++)
  {
    const Quantum
      *magick_restrict r;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    r=GetCacheViewVirtualPixels(edge_view,0,y,edge_image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(blur_view,0,y,blur_image->columns,1,
      exception);
    if ((r == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) blur_image->columns; x++)
    {
      const Quantum
        *magick_restrict p;

      ssize_t
        i;

      ssize_t
        center,
        j;

      j=GetAdaptiveKernelIndex(edge_image,r,width);
      p=GetCacheViewVirtualPixels(image_view,x-((ssize_t) width-j)/2L,y-
        ((ssize_t) width-j)/2L,width-(size_t) j,width-(size_t) j,exception);
      if (p == (const Quantum *) NULL)
        break;
      center=(ssize_t) (GetPixelChannels(image)*(width-(size_t) j)*
        ((width-(size_t) j)/2L)+GetPixelChannels(image)*((width-(size_t) j)/2));
      for (i=0; i < (ssize_t) GetPixelChannels(blur_image); i++)
      {
        AdaptiveChannel
          adaptive;

        adaptive.image=image;
        adaptive.blur_image=blur_image;
        adaptive.kernel=kernel;
        adaptive.p=p;
        adaptive.width=width;
        adaptive.j=j;
        adaptive.center=center;
        BlurAdaptiveChannel(&adaptive,i,q);
      }
      q+=(ptrdiff_t) GetPixelChannels(blur_image);
      r+=(ptrdiff_t) GetPixelChannels(edge_image);
    }
    if (SyncCacheViewAuthenticPixels(blur_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,AdaptiveBlurImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  blur_image->type=image->type;
  blur_view=DestroyCacheView(blur_view);
  edge_view=DestroyCacheView(edge_view);
  image_view=DestroyCacheView(image_view);
  edge_image=DestroyImage(edge_image);
  for (w=0; w < (ssize_t) width; w+=2)
    kernel[w]=(double *) RelinquishAlignedMemory(kernel[w]);
  kernel=(double **) RelinquishAlignedMemory(kernel);
  if (status == MagickFalse)
    blur_image=DestroyImage(blur_image);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A d a p t i v e S h a r p e n I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AdaptiveSharpenImage() adaptively sharpens the image by sharpening more
%  intensely near image edges and less intensely far from edges. We sharpen the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and AdaptiveSharpenImage() selects a suitable radius for you.
%
%  The format of the AdaptiveSharpenImage method is:
%
%      Image *AdaptiveSharpenImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Laplacian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType AcquireAdaptiveSharpenKernel(double **kernel,
  const size_t width,const ssize_t w,const double sigma)
{
  double
    normalize;

  ssize_t
    j,
    k,
    u,
    v;

  kernel[w]=(double *) MagickAssumeAligned(AcquireAlignedMemory((size_t)
    (width-(size_t) w),(width-(size_t) w)*sizeof(**kernel)));
  if (kernel[w] == (double *) NULL)
    return(MagickFalse);
  normalize=0.0;
  j=((ssize_t) width-w-1)/2;
  k=0;
  for (v=(-j); v <= j; v++)
  {
    for (u=(-j); u <= j; u++)
    {
      kernel[w][k]=(double) (-exp(-((double) u*u+v*v)/(2.0*MagickSigma*
        MagickSigma))/(2.0*MagickPI*MagickSigma*MagickSigma));
      normalize+=kernel[w][k];
      k++;
    }
  }
  kernel[w][(k-1)/2]=(double) ((-2.0)*normalize);
  if (sigma < MagickEpsilon)
    kernel[w][(k-1)/2]=1.0;
  return(MagickTrue);
}

MagickExport Image *AdaptiveSharpenImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
#define AdaptiveSharpenImageTag  "Convolve/Image"
#define MagickSigma  (fabs(sigma) < MagickEpsilon ? MagickEpsilon : sigma)

  CacheView
    *sharp_view,
    *edge_view,
    *image_view;

  double
    **kernel;

  Image
    *sharp_image,
    *edge_image,
    *gaussian_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  size_t
    width;

  ssize_t
    w,
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  sharp_image=CloneImage(image,0,0,MagickTrue,exception);
  if (sharp_image == (Image *) NULL)
    return((Image *) NULL);
  if (fabs(sigma) < MagickEpsilon)
    return(sharp_image);
  if (SetImageStorageClass(sharp_image,DirectClass,exception) == MagickFalse)
    {
      sharp_image=DestroyImage(sharp_image);
      return((Image *) NULL);
    }
  /*
    Edge detect the image brightness channel, level, sharp, and level again.
  */
  edge_image=EdgeImage(image,radius,exception);
  if (edge_image == (Image *) NULL)
    {
      sharp_image=DestroyImage(sharp_image);
      return((Image *) NULL);
    }
  (void) AutoLevelImage(edge_image,exception);
  gaussian_image=BlurImage(edge_image,radius,sigma,exception);
  if (gaussian_image != (Image *) NULL)
    {
      edge_image=DestroyImage(edge_image);
      edge_image=gaussian_image;
    }
  (void) AutoLevelImage(edge_image,exception);
  /*
    Create a set of kernels from maximum (radius,sigma) to minimum.
  */
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double **) MagickAssumeAligned(AcquireAlignedMemory((size_t)
    width,sizeof(*kernel)));
  if (kernel == (double **) NULL)
    {
      edge_image=DestroyImage(edge_image);
      sharp_image=DestroyImage(sharp_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) memset(kernel,0,(size_t) width*sizeof(*kernel));
  for (w=0; w < (ssize_t) width; w+=2)
  {
    if (AcquireAdaptiveSharpenKernel(kernel,width,w,sigma) == MagickFalse)
      break;
  }
  if (w < (ssize_t) width)
    {
      RelinquishAdaptiveKernels(kernel,w);
      kernel=(double **) RelinquishAlignedMemory(kernel);
      edge_image=DestroyImage(edge_image);
      sharp_image=DestroyImage(sharp_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Adaptively sharpen image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  edge_view=AcquireVirtualCacheView(edge_image,exception);
  sharp_view=AcquireAuthenticCacheView(sharp_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,sharp_image,sharp_image->rows,1)
#endif
  for (y=0; y < (ssize_t) sharp_image->rows; y++)
  {
    const Quantum
      *magick_restrict r;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    r=GetCacheViewVirtualPixels(edge_view,0,y,edge_image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(sharp_view,0,y,sharp_image->columns,1,
      exception);
    if ((r == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) sharp_image->columns; x++)
    {
      const Quantum
        *magick_restrict p;

      ssize_t
        i;

      ssize_t
        center,
        j;

      j=GetAdaptiveKernelIndex(edge_image,r,width);
      p=GetCacheViewVirtualPixels(image_view,x-(((ssize_t) width-j)/2L),y-
        (((ssize_t) width-j)/2L),width-(size_t) j,width-(size_t) j,exception);
      if (p == (const Quantum *) NULL)
        break;
      center=(ssize_t) (GetPixelChannels(image)*(width-(size_t) j)*
       ((width-(size_t) j)/2L)+GetPixelChannels(image)*((width-(size_t) j)/2));
      for (i=0; i < (ssize_t) GetPixelChannels(sharp_image); i++)
      {
        AdaptiveChannel
          adaptive;

        adaptive.image=image;
        adaptive.blur_image=sharp_image;
        adaptive.kernel=kernel;
        adaptive.p=p;
        adaptive.width=width;
        adaptive.j=j;
        adaptive.center=center;
        BlurAdaptiveChannel(&adaptive,i,q);
      }
      q+=(ptrdiff_t) GetPixelChannels(sharp_image);
      r+=(ptrdiff_t) GetPixelChannels(edge_image);
    }
    if (SyncCacheViewAuthenticPixels(sharp_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,AdaptiveSharpenImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  sharp_image->type=image->type;
  sharp_view=DestroyCacheView(sharp_view);
  edge_view=DestroyCacheView(edge_view);
  image_view=DestroyCacheView(image_view);
  edge_image=DestroyImage(edge_image);
  for (w=0; w < (ssize_t) width; w+=2)
    kernel[w]=(double *) RelinquishAlignedMemory(kernel[w]);
  kernel=(double **) RelinquishAlignedMemory(kernel);
  if (status == MagickFalse)
    sharp_image=DestroyImage(sharp_image);
  return(sharp_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B l u r I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlurImage() blurs an image.  We convolve the image with a Gaussian operator
%  of the given radius and standard deviation (sigma).  For reasonable results,
%  the radius should be larger than sigma.  Use a radius of 0 and BlurImage()
%  selects a suitable radius for you.
%
%  The format of the BlurImage method is:
%
%      Image *BlurImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *BlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  KernelInfo
    *kernel_info;

  Image
    *blur_image;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  blur_image=AccelerateBlurImage(image,radius,sigma,exception);
  if (blur_image != (Image *) NULL)
    return(blur_image);
#endif
  (void) FormatLocaleString(geometry,MagickPathExtent,
    "blur:%.17gx%.17g;blur:%.17gx%.17g+90",radius,sigma,radius,sigma);
  kernel_info=AcquireKernelInfo(geometry,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  blur_image=ConvolveImage(image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B i l a t e r a l B l u r I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BilateralBlurImage() is a non-linear, edge-preserving, and noise-reducing
%  smoothing filter for images.  It replaces the intensity of each pixel with
%  a weighted average of intensity values from nearby pixels.  This weight is
%  based on a Gaussian distribution.  The weights depend not only on Euclidean
%  distance of pixels, but also on the radiometric differences (e.g., range
%  differences, such as color intensity, depth distance, etc.). This preserves
%  sharp edges.
%
%  The format of the BilateralBlurImage method is:
%
%      Image *BilateralBlurImage(const Image *image,const size_t width,
%        const size_t height,const double intensity_sigma,
%        const double spatial_sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o width: the width of the neighborhood in pixels.
%
%    o height: the height of the neighborhood in pixels.
%
%    o intensity_sigma: sigma in the intensity space. A larger value means
%      that farther colors within the pixel neighborhood (see spatial_sigma)
%      will be mixed together, resulting in larger areas of semi-equal color.
%
%    o spatial_sigma: sigma in the coordinate space. A larger value means that
%      farther pixels influence each other as long as their colors are close
%      enough (see intensity_sigma ). When the neighborhood diameter is greater
%      than zero, it specifies the neighborhood size regardless of
%      spatial_sigma. Otherwise, the neighborhood diameter is proportional to
%      spatial_sigma.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline double BlurDistance(const ssize_t x,const ssize_t y,
  const ssize_t u,const ssize_t v)
{
  return(sqrt(((double) x-u)*((double) x-u)+((double) y-v)*((double) y-v)));
}

static inline double BlurGaussian(const double x,const double sigma)
{
  return(exp(-((double) x*x)*MagickSafeReciprocal(2.0*sigma*sigma))*
    MagickSafeReciprocal(Magick2PI*sigma*sigma));
}

static double **DestroyBilateralTLS(const size_t number_threads,
  double **weights)
{
  ssize_t
    i;

  assert(weights != (double **) NULL);
  for (i=0; i <= (ssize_t) number_threads; i++)
    if (weights[i] != (double *) NULL)
      weights[i]=(double *) RelinquishMagickMemory(weights[i]);
  weights=(double **) RelinquishMagickMemory(weights);
  return(weights);
}

static double **AcquireBilateralTLS(const size_t number_threads,
  const size_t width,const size_t height)
{
  double
    **weights;

  size_t
    count;

  ssize_t
    i;

  if (HeapOverflowSanityCheckGetSize(height,sizeof(**weights),&count) != MagickFalse)
    return((double **) NULL);
  weights=(double **) AcquireQuantumMemory(number_threads+1,sizeof(*weights));
  if (weights == (double **) NULL)
    return((double **) NULL);
  (void) memset(weights,0,(number_threads+1)*sizeof(*weights));
  for (i=0; i <= (ssize_t) number_threads; i++)
  {
    weights[i]=(double *) AcquireQuantumMemory(width,count);
    if (weights[i] == (double *) NULL)
      return(DestroyBilateralTLS(number_threads,weights));
  }
  return(weights);
}

#define MaxIntensity  (255)
#define BilateralBlurImageTag  "Blur/Image"

typedef struct _BilateralWeights
{
  const Image
    *image;

  const Quantum
    *p;

  size_t
    width,
    height;

  OffsetInfo
    mid;

  double
    *intensity_gaussian,
    *spatial_gaussian,
    intensity_sigma,
    spatial_sigma;

  int
    id;

  ssize_t
    x,
    y;
} BilateralWeights;

static void SetBilateralWeights(const BilateralWeights *bilateral,
  double **weights)
{
  const Image
    *const image = bilateral->image;

  const Quantum
    *const p = bilateral->p;

  const size_t
    width = bilateral->width,
    height = bilateral->height;

  const OffsetInfo
    mid = bilateral->mid;

  double
    *const intensity_gaussian = bilateral->intensity_gaussian,
    *const spatial_gaussian = bilateral->spatial_gaussian;

  const double
    intensity_sigma = bilateral->intensity_sigma,
    spatial_sigma = bilateral->spatial_sigma;

  const int
    id = bilateral->id;

  const ssize_t
    x = bilateral->x,
    y = bilateral->y;

  const Quantum
    *r;

  ssize_t
    n,
    u,
    v;

  n=0;
  for (v=0; v < (ssize_t) MagickMax(height,1); v++)
  {
    for (u=0; u < (ssize_t) MagickMax(width,1); u++)
    {
      double
        intensity;

      r=p-(ssize_t) (GetPixelChannels(image)*MagickMax(width,1)*
        (size_t) (mid.y-v)+GetPixelChannels(image)*(size_t) (mid.x-u));
      intensity=ScaleQuantumToChar((const Quantum)
        GetPixelIntensity(image,r))-(double)
        ScaleQuantumToChar((const Quantum) GetPixelIntensity(image,p));
      if ((intensity >= -MaxIntensity) && (intensity <= MaxIntensity))
        weights[id][n]=intensity_gaussian[(ssize_t) intensity+MaxIntensity]*
          spatial_gaussian[n];
      else
        weights[id][n]=BlurGaussian(intensity,intensity_sigma)*
          BlurGaussian(BlurDistance(x,y,x+u-mid.x,y+v-mid.y),spatial_sigma);
      n++;
    }
  }
}

typedef struct _BilateralSum
{
  const Image
    *image;

  const Quantum
    *p;

  size_t
    width,
    height;

  OffsetInfo
    mid;

  double
    **weights;

  int
    id;

  ssize_t
    i;
} BilateralSum;

static void SumBilateralPixels(const BilateralSum *sum,double *pixel,
  double *gamma,ssize_t *n)
{
  const Image
    *const image = sum->image;

  const Quantum
    *const p = sum->p;

  const size_t
    width = sum->width,
    height = sum->height;

  const OffsetInfo
    mid = sum->mid;

  double
    **const weights = sum->weights;

  const int
    id = sum->id;

  const ssize_t
    i = sum->i;

  const Quantum
    *r;

  ssize_t
    u,
    v;

  for (v=0; v < (ssize_t) MagickMax(height,1); v++)
  {
    for (u=0; u < (ssize_t) MagickMax(width,1); u++)
    {
       r=p-(ssize_t) (GetPixelChannels(image)*MagickMax(width,1)*
        (size_t) (mid.y-v)+GetPixelChannels(image)*(size_t)
        (mid.x-u));
      *pixel+=weights[id][*n]*(double) r[i];
      *gamma+=weights[id][*n];
      (*n)++;
    }
  }
}

static void SumBilateralBlendedPixels(const BilateralSum *sum,double *pixel,
  double *gamma,ssize_t *n)
{
  const Image
    *const image = sum->image;

  const Quantum
    *const p = sum->p;

  const size_t
    width = sum->width,
    height = sum->height;

  const OffsetInfo
    mid = sum->mid;

  double
    **const weights = sum->weights;

  const int
    id = sum->id;

  const ssize_t
    i = sum->i;

  const Quantum
    *r;

  ssize_t
    u,
    v;

  for (v=0; v < (ssize_t) MagickMax(height,1); v++)
  {
    for (u=0; u < (ssize_t) MagickMax(width,1); u++)
    {
      double
        alpha,
        beta;

      r=p-(ssize_t) (GetPixelChannels(image)*MagickMax(width,1)*(size_t)
        (mid.y-v)+GetPixelChannels(image)*(size_t) (mid.x-u));
      alpha=(double) (QuantumScale*(double) GetPixelAlpha(image,p));
      beta=(double) (QuantumScale*(double) GetPixelAlpha(image,r));
      *pixel+=weights[id][*n]*(double) r[i];
      *gamma+=weights[id][*n]*alpha*beta;
      (*n)++;
    }
  }
}

typedef struct _BilateralChannel
{
  const Image
    *image;

  const Quantum
    *p;

  size_t
    width,
    height;

  OffsetInfo
    mid;

  double
    **weights;

  int
    id;
} BilateralChannel;

static void BlurBilateralChannel(const BilateralChannel *bilateral_channel,
  Image *blur_image,const ssize_t i,Quantum *magick_restrict q)
{
  const Image
    *const image = bilateral_channel->image;

  const Quantum
    *const p = bilateral_channel->p;

  const size_t
    width = bilateral_channel->width,
    height = bilateral_channel->height;

  const OffsetInfo
    mid = bilateral_channel->mid;

  double
    **const weights = bilateral_channel->weights;

  const int
    id = bilateral_channel->id;

  BilateralSum
    sum;

  PixelChannel
    channel;

  PixelTrait
    blur_traits,
    traits;

  double
    gamma,
    pixel;

  ssize_t
    n;

  channel=GetPixelChannelChannel(image,i);
  traits=GetPixelChannelTraits(image,channel);
  blur_traits=GetPixelChannelTraits(blur_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (blur_traits == UndefinedPixelTrait))
    return;
  if ((blur_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(blur_image,channel,p[i],q);
      return;
    }
  pixel=0.0;
  gamma=0.0;
  n=0;
  if ((blur_traits & BlendPixelTrait) == 0)
    {
      /*
        No alpha blending.
      */
      sum.image=image;
      sum.p=p;
      sum.width=width;
      sum.height=height;
      sum.mid=mid;
      sum.weights=weights;
      sum.id=id;
      sum.i=i;
      SumBilateralPixels(&sum,&pixel,&gamma,&n);
      SetPixelChannel(blur_image,channel,ClampToQuantum(
        MagickSafeReciprocal(gamma)*pixel),q);
      return;
    }
  /*
    Alpha blending.
  */
  sum.image=image;
  sum.p=p;
  sum.width=width;
  sum.height=height;
  sum.mid=mid;
  sum.weights=weights;
  sum.id=id;
  sum.i=i;
  SumBilateralBlendedPixels(&sum,&pixel,&gamma,&n);
  SetPixelChannel(blur_image,channel,ClampToQuantum(
    MagickSafeReciprocal(gamma)*pixel),q);
}

MagickExport Image *BilateralBlurImage(const Image *image,const size_t width,
  const size_t height,const double intensity_sigma,const double spatial_sigma,
  ExceptionInfo *exception)
{
  CacheView
    *blur_view,
    *image_view;

  double
    intensity_gaussian[2*(MaxIntensity+1)],
    *spatial_gaussian,
    **weights;

  Image
    *blur_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  OffsetInfo
    mid;

  size_t
    number_threads;

  ssize_t
    w,
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(blur_image,DirectClass,exception) == MagickFalse)
    {
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  weights=AcquireBilateralTLS(number_threads,MagickMax(width,1),
    MagickMax(height,1));
  if (weights == (double **) NULL)
    {
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  for (w=(-MaxIntensity); w <= MaxIntensity; w++)
    intensity_gaussian[w+MaxIntensity]=BlurGaussian((double) w,intensity_sigma);
  spatial_gaussian=weights[number_threads];
  {
    ssize_t
      n,
      v;

    n=0;
    mid.x=(ssize_t) (MagickMax(width,1)/2L);
    mid.y=(ssize_t) (MagickMax(height,1)/2L);
    for (v=0; v < (ssize_t) MagickMax(height,1); v++)
    {
      ssize_t
        u;

      for (u=0; u < (ssize_t) MagickMax(width,1); u++)
        spatial_gaussian[n++]=BlurGaussian(BlurDistance(0,0,u-mid.x,v-mid.y),
          spatial_sigma);
    }
  }
  /*
    Bilateral blur image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  blur_view=AcquireAuthenticCacheView(blur_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,blur_image,blur_image->rows,1)
#endif
  for (y=0; y < (ssize_t) blur_image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(blur_view,0,y,blur_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) blur_image->columns; x++)
    {
      BilateralWeights
        bilateral;

      const Quantum
        *magick_restrict p;

      ssize_t
        i;

      /*
        Tonal weighting preserves edges while smoothing in the flat regions.
      */
      p=GetCacheViewVirtualPixels(image_view,x-mid.x,y-mid.y,MagickMax(width,1),
        MagickMax(height,1),exception);
      if (p == (const Quantum *) NULL)
        break;
      p+=(ptrdiff_t) (GetPixelChannels(image)*MagickMax(width,1)*(size_t) mid.y+
        GetPixelChannels(image)*(size_t) mid.x);
      bilateral.image=image;
      bilateral.p=p;
      bilateral.width=width;
      bilateral.height=height;
      bilateral.mid=mid;
      bilateral.intensity_gaussian=intensity_gaussian;
      bilateral.spatial_gaussian=spatial_gaussian;
      bilateral.intensity_sigma=intensity_sigma;
      bilateral.spatial_sigma=spatial_sigma;
      bilateral.id=id;
      bilateral.x=x;
      bilateral.y=y;
      SetBilateralWeights(&bilateral,weights);
      for (i=0; i < (ssize_t) GetPixelChannels(blur_image); i++)
      {
        BilateralChannel
          bilateral_channel;

        bilateral_channel.image=image;
        bilateral_channel.p=p;
        bilateral_channel.width=width;
        bilateral_channel.height=height;
        bilateral_channel.mid=mid;
        bilateral_channel.weights=weights;
        bilateral_channel.id=id;
        BlurBilateralChannel(&bilateral_channel,blur_image,i,q);
      }
      q+=(ptrdiff_t) GetPixelChannels(blur_image);
    }
    if (SyncCacheViewAuthenticPixels(blur_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,BilateralBlurImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  blur_image->type=image->type;
  blur_view=DestroyCacheView(blur_view);
  image_view=DestroyCacheView(image_view);
  weights=DestroyBilateralTLS(number_threads,weights);
  if (status == MagickFalse)
    blur_image=DestroyImage(blur_image);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n v o l v e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvolveImage() applies a custom convolution kernel to the image.
%
%  The format of the ConvolveImage method is:
%
%      Image *ConvolveImage(const Image *image,const KernelInfo *kernel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o kernel: the filtering kernel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *ConvolveImage(const Image *image,
  const KernelInfo *kernel_info,ExceptionInfo *exception)
{
  Image
    *convolve_image;

  convolve_image=MorphologyImage(image,ConvolveMorphology,1,kernel_info,
    exception);
  return(convolve_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E m b o s s I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EmbossImage() returns a grayscale image with a three-dimensional effect.
%  We convolve the image with a Gaussian operator of the given radius and
%  standard deviation (sigma).  For reasonable results, radius should be
%  larger than sigma.  Use a radius of 0 and Emboss() selects a suitable
%  radius for you.
%
%  The format of the EmbossImage method is:
%
%      Image *EmbossImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void NormalizeKernelValues(KernelInfo *kernel_info)
{
  double
    gamma,
    normalize;

  ssize_t
    i;

  normalize=0.0;
  for (i=0; i < (ssize_t) (kernel_info->width*kernel_info->height); i++)
    normalize+=kernel_info->values[i];
  gamma=MagickSafeReciprocal(normalize);
  for (i=0; i < (ssize_t) (kernel_info->width*kernel_info->height); i++)
    kernel_info->values[i]*=gamma;
}

static void SetEmbossKernelValues(KernelInfo *kernel_info,const double sigma)
{
  ssize_t
    i;

  ssize_t
    j,
    k,
    u,
    v;

  j=(ssize_t) (kernel_info->width-1)/2;
  k=j;
  i=0;
  for (v=(-j); v <= j; v++)
  {
    for (u=(-j); u <= j; u++)
    {
      kernel_info->values[i]=(MagickRealType) (((u < 0) || (v < 0) ? -8.0 :
        8.0)*exp(-((double) u*u+v*v)/(2.0*MagickSigma*MagickSigma))/
        (2.0*MagickPI*MagickSigma*MagickSigma));
      if (u != k)
        kernel_info->values[i]=0.0;
      i++;
    }
    k--;
  }
}

MagickExport Image *EmbossImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *emboss_image;

  KernelInfo
    *kernel_info;

  size_t
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel_info=AcquireKernelInfo((const char *) NULL,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  kernel_info->width=width;
  kernel_info->height=width;
  kernel_info->x=(ssize_t) (width-1)/2;
  kernel_info->y=(ssize_t) (width-1)/2;
  kernel_info->values=(MagickRealType *) MagickAssumeAligned(
    AcquireAlignedMemory(kernel_info->width,kernel_info->width*
    sizeof(*kernel_info->values)));
  if (kernel_info->values == (MagickRealType *) NULL)
    {
      kernel_info=DestroyKernelInfo(kernel_info);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  SetEmbossKernelValues(kernel_info,sigma);
  NormalizeKernelValues(kernel_info);
  emboss_image=ConvolveImage(image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  if (emboss_image != (Image *) NULL)
    (void) EqualizeImage(emboss_image,exception);
  return(emboss_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     G a u s s i a n B l u r I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GaussianBlurImage() blurs an image.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, the radius should be larger than sigma.  Use a
%  radius of 0 and GaussianBlurImage() selects a suitable radius for you.
%
%  The format of the GaussianBlurImage method is:
%
%      Image *GaussianBlurImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ConvolveKernelGeometry(const Image *image,const char *geometry,
  ExceptionInfo *exception)
{
  Image
    *blur_image;

  KernelInfo
    *kernel_info;

  kernel_info=AcquireKernelInfo(geometry,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  blur_image=ConvolveImage(image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  return(blur_image);
}

MagickExport Image *GaussianBlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  (void) FormatLocaleString(geometry,MagickPathExtent,"gaussian:%.17gx%.17g",
    radius,sigma);
  return(ConvolveKernelGeometry(image,geometry,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o t i o n B l u r I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MotionBlurImage() simulates motion blur.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and MotionBlurImage() selects a suitable radius for you.
%  Angle gives the angle of the blurring motion.
%
%  Andrew Protano contributed this effect.
%
%  The format of the MotionBlurImage method is:
%
%    Image *MotionBlurImage(const Image *image,const double radius,
%      const double sigma,const double angle,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting
%      the center pixel.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o angle: Apply the effect along this angle.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickRealType *GetMotionBlurKernel(const size_t width,
  const double sigma)
{
  MagickRealType
    *kernel,
    normalize;

  ssize_t
    i;

  /*
   Generate a 1-D convolution kernel.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  kernel=(MagickRealType *) MagickAssumeAligned(AcquireAlignedMemory((size_t)
    width,sizeof(*kernel)));
  if (kernel == (MagickRealType *) NULL)
    return(kernel);
  normalize=0.0;
  for (i=0; i < (ssize_t) width; i++)
  {
    kernel[i]=(MagickRealType) (exp((-((double) i*i)/(double) (2.0*MagickSigma*
      MagickSigma)))/(MagickSQ2PI*MagickSigma));
    normalize+=kernel[i];
  }
  for (i=0; i < (ssize_t) width; i++)
    kernel[i]/=normalize;
  return(kernel);
}

typedef struct _MotionSum
{
  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ssize_t
    x,
    y,
    i;

  ExceptionInfo
    *exception;
} MotionSum;

static MagickBooleanType SumMotionPixels(const MotionSum *sum,double *pixel,
  MagickRealType **k)
{
  CacheView
    *const motion_view = sum->motion_view;

  OffsetInfo
    *const offset = sum->offset;

  const size_t
    width = sum->width;

  const ssize_t
    x = sum->x,
    y = sum->y,
    i = sum->i;

  ExceptionInfo
    *const exception = sum->exception;

  const Quantum
    *magick_restrict r;

  MagickBooleanType
    status;

  ssize_t
    j;

  status=MagickTrue;
  for (j=0; j < (ssize_t) width; j++)
  {
    r=GetCacheViewVirtualPixels(motion_view,x+offset[j].x,y+
      offset[j].y,1,1,exception);
    if (r == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    *pixel+=(**k)*(double) r[i];
    (*k)++;
  }
  return(status);
}

typedef struct _MotionBlendedSum
{
  const Image
    *image;

  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ssize_t
    x,
    y,
    i;

  ExceptionInfo
    *exception;
} MotionBlendedSum;

static MagickBooleanType SumMotionBlendedPixels(const MotionBlendedSum *sum,
  double *pixel,double *gamma,MagickRealType **k)
{
  const Quantum
    *magick_restrict r;

  double
    alpha = 0.0;

  MagickBooleanType
    status;

  ssize_t
    j;

  status=MagickTrue;
  for (j=0; j < (ssize_t) sum->width; j++)
  {
    r=GetCacheViewVirtualPixels(sum->motion_view,sum->x+sum->offset[j].x,
      sum->y+sum->offset[j].y,1,1,sum->exception);
    if (r == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    alpha=QuantumScale*(double) GetPixelAlpha(sum->image,r);
    *pixel+=(**k)*alpha*(double) r[sum->i];
    *gamma+=(**k)*alpha;
    (*k)++;
  }
  return(status);
}

typedef struct _MotionChannel
{
  const Image
    *image;

  Image
    *blur_image;

  const Quantum
    *p;

  MagickRealType
    *kernel;

  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ssize_t
    x,
    y;

  ExceptionInfo
    *exception;
} MotionChannel;

static MagickBooleanType BlurMotionChannel(const MotionChannel *motion,
  const ssize_t i,Quantum *magick_restrict q)
{
  double
    gamma = 0.0,
    pixel;

  MagickBooleanType
    status;

  MotionBlendedSum
    blended_sum;

  MotionSum
    sum;

  PixelChannel
    channel;

  PixelTrait
    blur_traits,
    traits;

  MagickRealType
    *magick_restrict k;

  status=MagickTrue;
  channel=GetPixelChannelChannel(motion->image,i);
  traits=GetPixelChannelTraits(motion->image,channel);
  blur_traits=GetPixelChannelTraits(motion->blur_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (blur_traits == UndefinedPixelTrait))
    return(status);
  if ((blur_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(motion->blur_image,channel,motion->p[i],q);
      return(status);
    }
  k=motion->kernel;
  pixel=0.0;
  if ((blur_traits & BlendPixelTrait) == 0)
    {
      sum.motion_view=motion->motion_view;
      sum.offset=motion->offset;
      sum.width=motion->width;
      sum.x=motion->x;
      sum.y=motion->y;
      sum.i=i;
      sum.exception=motion->exception;
      if (SumMotionPixels(&sum,&pixel,&k) == MagickFalse)
        status=MagickFalse;
      SetPixelChannel(motion->blur_image,channel,ClampToQuantum(pixel),q);
      return(status);
    }
  blended_sum.image=motion->image;
  blended_sum.motion_view=motion->motion_view;
  blended_sum.offset=motion->offset;
  blended_sum.width=motion->width;
  blended_sum.x=motion->x;
  blended_sum.y=motion->y;
  blended_sum.i=i;
  blended_sum.exception=motion->exception;
  if (SumMotionBlendedPixels(&blended_sum,&pixel,&gamma,&k) == MagickFalse)
    status=MagickFalse;
  gamma=MagickSafeReciprocal(gamma);
  SetPixelChannel(motion->blur_image,channel,ClampToQuantum(gamma*pixel),q);
  return(status);
}

static MagickBooleanType BlurMotionPixel(const MotionChannel *pixel_motion,
  Quantum *magick_restrict q)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
  for (i=0; i < (ssize_t) GetPixelChannels(pixel_motion->image); i++)
  {
    MotionChannel
      motion;

    motion.image=pixel_motion->image;
    motion.blur_image=pixel_motion->blur_image;
    motion.p=pixel_motion->p;
    motion.kernel=pixel_motion->kernel;
    motion.motion_view=pixel_motion->motion_view;
    motion.offset=pixel_motion->offset;
    motion.width=pixel_motion->width;
    motion.x=pixel_motion->x;
    motion.y=pixel_motion->y;
    motion.exception=pixel_motion->exception;
    if (BlurMotionChannel(&motion,i,q) == MagickFalse)
      status=MagickFalse;
  }
  return(status);
}

typedef struct _MotionRow
{
  const Image
    *image;

  Image
    *blur_image;

  MagickRealType
    *kernel;

  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} MotionRow;

static MagickBooleanType BlurMotionRow(const MotionRow *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = row->image;

  Image
    *const blur_image = row->blur_image;

  MagickRealType
    *const kernel = row->kernel;

  CacheView
    *const motion_view = row->motion_view;

  OffsetInfo
    *const offset = row->offset;

  const size_t
    width = row->width;

  const ssize_t
    y = row->y;

  ExceptionInfo
    *const exception = row->exception;

  MagickBooleanType
    status;

  ssize_t
    x;

  status=MagickTrue;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    MotionChannel
      pixel_motion;

    pixel_motion.image=image;
    pixel_motion.blur_image=blur_image;
    pixel_motion.p=p;
    pixel_motion.kernel=kernel;
    pixel_motion.motion_view=motion_view;
    pixel_motion.offset=offset;
    pixel_motion.width=width;
    pixel_motion.x=x;
    pixel_motion.y=y;
    pixel_motion.exception=exception;
    if (BlurMotionPixel(&pixel_motion,q) == MagickFalse)
      status=MagickFalse;
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(blur_image);
  }
  return(status);
}

MagickExport Image *MotionBlurImage(const Image *image,const double radius,
  const double sigma,const double angle,ExceptionInfo *exception)
{
#define BlurImageTag  "Blur/Image"

  CacheView
    *blur_view,
    *image_view,
    *motion_view;

  Image
    *blur_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MagickRealType
    *kernel;

  OffsetInfo
    *offset;

  PointInfo
    point;

  size_t
    width;

  ssize_t
    w,
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel=GetMotionBlurKernel(width,sigma);
  if (kernel == (MagickRealType *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  offset=(OffsetInfo *) AcquireQuantumMemory(width,sizeof(*offset));
  if (offset == (OffsetInfo *) NULL)
    {
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  point.x=(double) width*sin(DegreesToRadians(angle));
  point.y=(double) width*cos(DegreesToRadians(angle));
  for (w=0; w < (ssize_t) width; w++)
  {
    offset[w].x=CastDoubleToSsizeT(ceil((double) (w*point.y)/
      hypot(point.x,point.y)-0.5));
    offset[w].y=CastDoubleToSsizeT(ceil((double) (w*point.x)/
      hypot(point.x,point.y)-0.5));
  }
  /*
    Motion blur image.
  */
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  blur_image=AccelerateMotionBlurImage(image,kernel,width,offset,exception);
  if (blur_image != (Image *) NULL)
    {
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      offset=(OffsetInfo *) RelinquishMagickMemory(offset);
      return(blur_image);
    }
#endif
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    {
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      offset=(OffsetInfo *) RelinquishMagickMemory(offset);
      return((Image *) NULL);
    }
  if (SetImageStorageClass(blur_image,DirectClass,exception) == MagickFalse)
    {
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      offset=(OffsetInfo *) RelinquishMagickMemory(offset);
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  motion_view=AcquireVirtualCacheView(image,exception);
  blur_view=AcquireAuthenticCacheView(blur_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,blur_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    MotionRow
      row;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(blur_view,0,y,blur_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    row.image=image;
    row.blur_image=blur_image;
    row.kernel=kernel;
    row.motion_view=motion_view;
    row.offset=offset;
    row.width=width;
    row.y=y;
    row.exception=exception;
    if (BlurMotionRow(&row,p,q) == MagickFalse)
      status=MagickFalse;
    if (SyncCacheViewAuthenticPixels(blur_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,BlurImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  blur_view=DestroyCacheView(blur_view);
  motion_view=DestroyCacheView(motion_view);
  image_view=DestroyCacheView(image_view);
  kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
  offset=(OffsetInfo *) RelinquishMagickMemory(offset);
  if (status == MagickFalse)
    blur_image=DestroyImage(blur_image);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R o t a t i o n a l B l u r I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RotationalBlurImage() applies a radial blur to the image.
%
%  Andrew Protano contributed this effect.
%
%  The format of the RotationalBlurImage method is:
%
%    Image *RotationalBlurImage(const Image *image,const double angle,
%      ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o angle: the angle of the radial blur.
%
%    o blur: the blur.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _RotationalSum
{
  CacheView
    *radial_view;

  PointInfo
    blur_center,
    center;

  double
    *cos_theta,
    *sin_theta;

  size_t
    n,
    step;

  ssize_t
    i;

  ExceptionInfo
    *exception;
} RotationalSum;

static MagickBooleanType SumRotationalPixels(const RotationalSum *sum,
  double *pixel,double *gamma)
{
  CacheView
    *const radial_view = sum->radial_view;

  const PointInfo
    blur_center = sum->blur_center,
    center = sum->center;

  double
    *const cos_theta = sum->cos_theta,
    *const sin_theta = sum->sin_theta;

  const size_t
    n = sum->n,
    step = sum->step;

  const ssize_t
    i = sum->i;

  ExceptionInfo
    *const exception = sum->exception;

  const Quantum
    *magick_restrict r;

  MagickBooleanType
    status;

  ssize_t
    j;

  status=MagickTrue;
  for (j=0; j < (ssize_t) n; j+=(ssize_t) step)
  {
    r=GetCacheViewVirtualPixels(radial_view, (ssize_t) (blur_center.x+
      center.x*cos_theta[j]-center.y*sin_theta[j]+0.5),(ssize_t)
      (blur_center.y+center.x*sin_theta[j]+center.y*cos_theta[j]+0.5),
      1,1,exception);
    if (r == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    *pixel+=(double) r[i];
    (*gamma)++;
  }
  return(status);
}

typedef struct _RotationalBlendedSum
{
  const Image
    *image;

  CacheView
    *radial_view;

  PointInfo
    blur_center,
    center;

  double
    *cos_theta,
    *sin_theta;

  size_t
    n,
    step;

  ssize_t
    i;

  ExceptionInfo
    *exception;
} RotationalBlendedSum;

static MagickBooleanType SumRotationalBlendedPixels(
  const RotationalBlendedSum *sum,double *pixel,double *gamma)
{
  const Quantum
    *magick_restrict r;

  MagickBooleanType
    status;

  ssize_t
    j;

  status=MagickTrue;
  for (j=0; j < (ssize_t) sum->n; j+=(ssize_t) sum->step)
  {
    double
      alpha;

    r=GetCacheViewVirtualPixels(sum->radial_view, (ssize_t) (sum->blur_center.x+
      sum->center.x*sum->cos_theta[j]-sum->center.y*sum->sin_theta[j]+0.5),
      (ssize_t) (sum->blur_center.y+sum->center.x*sum->sin_theta[j]+
      sum->center.y*sum->cos_theta[j]+0.5),1,1,sum->exception);
    if (r == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    alpha=QuantumScale*(double) GetPixelAlpha(sum->image,r);
    *pixel+=alpha*(double) r[sum->i];
    *gamma+=alpha;
  }
  return(status);
}

static size_t GetRotationalBlurStep(const double radius,
  const double blur_radius,const size_t n)
{
  size_t
    step;

  if (radius == 0)
    step=1;
  else
    {
      step=(size_t) (blur_radius/radius);
      if (step == 0)
        step=1;
      else
        if (step >= n)
          step=n-1;
    }
  return(step);
}

typedef struct _RotationalChannel
{
  const Image
    *image;

  Image
    *blur_image;

  const Quantum
    *p;

  CacheView
    *radial_view;

  PointInfo
    blur_center,
    center;

  double
    *cos_theta,
    *sin_theta;

  size_t
    n,
    step;

  ExceptionInfo
    *exception;
} RotationalChannel;

static MagickBooleanType BlurRotationalChannel(
  const RotationalChannel *rotational,const ssize_t i,
  Quantum *magick_restrict q)
{
  double
    gamma,
    pixel;

  MagickBooleanType
    status;

  PixelChannel
    channel;

  PixelTrait
    blur_traits,
    traits;

  RotationalBlendedSum
    blended_sum;

  RotationalSum
    sum;

  status=MagickTrue;
  channel=GetPixelChannelChannel(rotational->image,i);
  traits=GetPixelChannelTraits(rotational->image,channel);
  blur_traits=GetPixelChannelTraits(rotational->blur_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (blur_traits == UndefinedPixelTrait))
    return(status);
  if ((blur_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(rotational->blur_image,channel,rotational->p[i],q);
      return(status);
    }
  gamma=0.0;
  pixel=0.0;
  if ((GetPixelChannelTraits(rotational->image,AlphaPixelChannel) ==
       UndefinedPixelTrait) || (channel == AlphaPixelChannel))
    {
      sum.radial_view=rotational->radial_view;
      sum.blur_center=rotational->blur_center;
      sum.center=rotational->center;
      sum.cos_theta=rotational->cos_theta;
      sum.sin_theta=rotational->sin_theta;
      sum.n=rotational->n;
      sum.step=rotational->step;
      sum.i=i;
      sum.exception=rotational->exception;
      if (SumRotationalPixels(&sum,&pixel,&gamma) == MagickFalse)
        status=MagickFalse;
      gamma=MagickSafeReciprocal(gamma);
      SetPixelChannel(rotational->blur_image,channel,ClampToQuantum(gamma*
        pixel),q);
      return(status);
    }
  blended_sum.image=rotational->image;
  blended_sum.radial_view=rotational->radial_view;
  blended_sum.blur_center=rotational->blur_center;
  blended_sum.center=rotational->center;
  blended_sum.cos_theta=rotational->cos_theta;
  blended_sum.sin_theta=rotational->sin_theta;
  blended_sum.n=rotational->n;
  blended_sum.step=rotational->step;
  blended_sum.i=i;
  blended_sum.exception=rotational->exception;
  if (SumRotationalBlendedPixels(&blended_sum,&pixel,&gamma) == MagickFalse)
    status=MagickFalse;
  gamma=MagickSafeReciprocal(gamma);
  SetPixelChannel(rotational->blur_image,channel,ClampToQuantum(gamma*pixel),
    q);
  return(status);
}

static MagickBooleanType BlurRotationalPixel(
  const RotationalChannel *pixel_rotational,Quantum *magick_restrict q)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
  for (i=0; i < (ssize_t) GetPixelChannels(pixel_rotational->image); i++)
  {
    RotationalChannel
      rotational;

    rotational.image=pixel_rotational->image;
    rotational.blur_image=pixel_rotational->blur_image;
    rotational.p=pixel_rotational->p;
    rotational.radial_view=pixel_rotational->radial_view;
    rotational.blur_center=pixel_rotational->blur_center;
    rotational.center=pixel_rotational->center;
    rotational.cos_theta=pixel_rotational->cos_theta;
    rotational.sin_theta=pixel_rotational->sin_theta;
    rotational.n=pixel_rotational->n;
    rotational.step=pixel_rotational->step;
    rotational.exception=pixel_rotational->exception;
    if (BlurRotationalChannel(&rotational,i,q) == MagickFalse)
      status=MagickFalse;
  }
  return(status);
}

typedef struct _RotationalRow
{
  const Image
    *image;

  Image
    *blur_image;

  CacheView
    *radial_view;

  PointInfo
    blur_center;

  double
    blur_radius,
    *cos_theta,
    *sin_theta;

  size_t
    n;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} RotationalRow;

static MagickBooleanType BlurRotationalRow(const RotationalRow *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = row->image;

  Image
    *const blur_image = row->blur_image;

  CacheView
    *const radial_view = row->radial_view;

  const PointInfo
    blur_center = row->blur_center;

  const double
    blur_radius = row->blur_radius;

  double
    *const cos_theta = row->cos_theta,
    *const sin_theta = row->sin_theta;

  const size_t
    n = row->n;

  const ssize_t
    y = row->y;

  ExceptionInfo
    *const exception = row->exception;

  MagickBooleanType
    status;

  ssize_t
    x;

  status=MagickTrue;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      radius;

    PointInfo
      center;

    RotationalChannel
      pixel_rotational;

    size_t
      step;

    center.x=(double) x-blur_center.x;
    center.y=(double) y-blur_center.y;
    radius=hypot((double) center.x,center.y);
    step=GetRotationalBlurStep(radius,blur_radius,n);
    pixel_rotational.image=image;
    pixel_rotational.blur_image=blur_image;
    pixel_rotational.p=p;
    pixel_rotational.radial_view=radial_view;
    pixel_rotational.blur_center=blur_center;
    pixel_rotational.center=center;
    pixel_rotational.cos_theta=cos_theta;
    pixel_rotational.sin_theta=sin_theta;
    pixel_rotational.n=n;
    pixel_rotational.step=step;
    pixel_rotational.exception=exception;
    if (BlurRotationalPixel(&pixel_rotational,q) == MagickFalse)
      status=MagickFalse;
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(blur_image);
  }
  return(status);
}

MagickExport Image *RotationalBlurImage(const Image *image,const double angle,
  ExceptionInfo *exception)
{
  CacheView
    *blur_view,
    *image_view,
    *radial_view;

  double
    blur_radius,
    *cos_theta,
    offset,
    *sin_theta,
    theta;

  Image
    *blur_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PointInfo
    blur_center;

  size_t
    n;

  ssize_t
    w,
    y;

  /*
    Allocate blur image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  blur_image=AccelerateRotationalBlurImage(image,angle,exception);
  if (blur_image != (Image *) NULL)
    return(blur_image);
#endif
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(blur_image,DirectClass,exception) == MagickFalse)
    {
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  blur_center.x=(double) (image->columns-1)/2.0;
  blur_center.y=(double) (image->rows-1)/2.0;
  blur_radius=hypot(blur_center.x,blur_center.y);
  n=(size_t) fabs(4.0*DegreesToRadians(angle)*sqrt((double) blur_radius)+2UL);
  theta=DegreesToRadians(angle)/(double) (n-1);
  cos_theta=(double *) AcquireQuantumMemory((size_t) n,sizeof(*cos_theta));
  sin_theta=(double *) AcquireQuantumMemory((size_t) n,sizeof(*sin_theta));
  if ((cos_theta == (double *) NULL) || (sin_theta == (double *) NULL))
    {
      if (cos_theta != (double *) NULL)
        cos_theta=(double *) RelinquishMagickMemory(cos_theta);
      if (sin_theta != (double *) NULL)
        sin_theta=(double *) RelinquishMagickMemory(sin_theta);
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  offset=theta*(double) (n-1)/2.0;
  for (w=0; w < (ssize_t) n; w++)
  {
    cos_theta[w]=cos((double) (theta*w-offset));
    sin_theta[w]=sin((double) (theta*w-offset));
  }
  /*
    Radial blur image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  radial_view=AcquireVirtualCacheView(image,exception);
  blur_view=AcquireAuthenticCacheView(blur_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,blur_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    RotationalRow
      row;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(blur_view,0,y,blur_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    row.image=image;
    row.blur_image=blur_image;
    row.radial_view=radial_view;
    row.blur_center=blur_center;
    row.blur_radius=blur_radius;
    row.cos_theta=cos_theta;
    row.sin_theta=sin_theta;
    row.n=n;
    row.y=y;
    row.exception=exception;
    if (BlurRotationalRow(&row,p,q) == MagickFalse)
      status=MagickFalse;
    if (SyncCacheViewAuthenticPixels(blur_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,BlurImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  blur_view=DestroyCacheView(blur_view);
  radial_view=DestroyCacheView(radial_view);
  image_view=DestroyCacheView(image_view);
  cos_theta=(double *) RelinquishMagickMemory(cos_theta);
  sin_theta=(double *) RelinquishMagickMemory(sin_theta);
  if (status == MagickFalse)
    blur_image=DestroyImage(blur_image);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S e l e c t i v e B l u r I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SelectiveBlurImage() selectively blur pixels within a contrast threshold.
%  It is similar to the unsharpen mask that sharpens everything with contrast
%  above a certain threshold.
%
%  The format of the SelectiveBlurImage method is:
%
%      Image *SelectiveBlurImage(const Image *image,const double radius,
%        const double sigma,const double threshold,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o threshold: only pixels within this contrast threshold are included
%      in the blur operation.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsSelectiveRowMissing(const Quantum *p,
  const Quantum *l,const Quantum *q)
{
  return(((p == (const Quantum *) NULL) || (l == (const Quantum *) NULL) ||
    (q == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

typedef struct _SelectiveTap
{
  double
    contrast,
    threshold;

  const MagickRealType
    *k;

  const Quantum
    *pixels;

  ssize_t
    i;
} SelectiveTap;

static void AddSelectiveTap(const SelectiveTap *tap,double *pixel,
  double *gamma)
{
  const double
    contrast = tap->contrast,
    threshold = tap->threshold;

  const MagickRealType
    *magick_restrict const k = tap->k;

  const Quantum
    *magick_restrict const pixels = tap->pixels;

  const ssize_t
    i = tap->i;

  if (fabs(contrast) < threshold)
    {
      *pixel+=(*k)*(double) pixels[i];
      *gamma+=(*k);
    }
}

typedef struct _SelectiveSum
{
  const Image
    *image;

  Image
    *luminance_image;

  size_t
    width;

  double
    intensity,
    threshold;

  ssize_t
    i;
} SelectiveSum;

typedef struct _SelectiveSumTargets
{
  double
    *pixel,
    *gamma;

  const MagickRealType
    *magick_restrict *k;

  const Quantum
    *magick_restrict *pixels,
    *magick_restrict *luminance_pixels;
} SelectiveSumTargets;

static void SumSelectivePixels(const SelectiveSum *sum,
  const SelectiveSumTargets *targets)
{
  double
    *const pixel = targets->pixel,
    *const gamma = targets->gamma;

  const MagickRealType
    *magick_restrict *const k = targets->k;

  const Quantum
    *magick_restrict *const pixels = targets->pixels,
    *magick_restrict *const luminance_pixels = targets->luminance_pixels;

  double
    contrast;

  ssize_t
    u,
    v;

  for (v=0; v < (ssize_t) sum->width; v++)
  {
    for (u=0; u < (ssize_t) sum->width; u++)
    {
      SelectiveTap
        tap;

      contrast=GetPixelIntensity(sum->luminance_image,*luminance_pixels)-
        sum->intensity;
      tap.contrast=contrast;
      tap.threshold=sum->threshold;
      tap.k=*k;
      tap.pixels=*pixels;
      tap.i=sum->i;
      AddSelectiveTap(&tap,pixel,gamma);
      (*k)++;
      *pixels+=(ptrdiff_t) GetPixelChannels(sum->image);
      *luminance_pixels+=(ptrdiff_t) GetPixelChannels(sum->luminance_image);
    }
    *pixels+=(ptrdiff_t) GetPixelChannels(sum->image)*sum->image->columns;
    *luminance_pixels+=(ptrdiff_t) GetPixelChannels(sum->luminance_image)*
      sum->luminance_image->columns;
  }
}

static void SumSelectiveBlendedPixels(const SelectiveSum *sum,
  const SelectiveSumTargets *targets)
{
  double
    alpha,
    contrast;

  ssize_t
    u,
    v;

  for (v=0; v < (ssize_t) sum->width; v++)
  {
    for (u=0; u < (ssize_t) sum->width; u++)
    {
      contrast=GetPixelIntensity(sum->image,*targets->pixels)-sum->intensity;
      if (fabs(contrast) < sum->threshold)
        {
          alpha=QuantumScale*(double) GetPixelAlpha(sum->image,
            *targets->pixels);
          *targets->pixel+=(**targets->k)*alpha*(double)
            (*targets->pixels)[sum->i];
          *targets->gamma+=(**targets->k)*alpha;
        }
      (*targets->k)++;
      *targets->pixels+=(ptrdiff_t) GetPixelChannels(sum->image);
      *targets->luminance_pixels+=(ptrdiff_t) GetPixelChannels(
        sum->luminance_image);
    }
    *targets->pixels+=(ptrdiff_t) GetPixelChannels(sum->image)*
      sum->image->columns;
    *targets->luminance_pixels+=(ptrdiff_t) GetPixelChannels(
      sum->luminance_image)*sum->luminance_image->columns;
  }
}

typedef struct _SelectiveChannelSum
{
  const Image
    *image;

  Image
    *blur_image,
    *luminance_image;

  MagickRealType
    *kernel;

  const Quantum
    *p,
    *l;

  ssize_t
    center;

  double
    intensity,
    threshold;

  size_t
    width;

  PixelChannel
    channel;

  PixelTrait
    blur_traits;
} SelectiveChannelSum;

static void BlurSelectiveChannelSum(const SelectiveChannelSum *blur,
  const ssize_t i,Quantum *magick_restrict q)
{
  double
    gamma,
    pixel;

  const MagickRealType
    *magick_restrict k;

  const Quantum
    *magick_restrict luminance_pixels,
    *magick_restrict pixels;

  SelectiveSum
    sum;

  SelectiveSumTargets
    targets;

  k=blur->kernel;
  pixel=0.0;
  pixels=blur->p;
  luminance_pixels=blur->l;
  gamma=0.0;
  if ((blur->blur_traits & BlendPixelTrait) == 0)
    {
      sum.image=blur->image;
      sum.luminance_image=blur->luminance_image;
      sum.width=blur->width;
      sum.intensity=blur->intensity;
      sum.threshold=blur->threshold;
      sum.i=i;
      targets.pixel=(&pixel);
      targets.gamma=(&gamma);
      targets.k=(&k);
      targets.pixels=(&pixels);
      targets.luminance_pixels=(&luminance_pixels);
      SumSelectivePixels(&sum,&targets);
      if (fabs((double) gamma) < MagickEpsilon)
        {
          SetPixelChannel(blur->blur_image,blur->channel,
            blur->p[blur->center+i],q);
          return;
        }
      gamma=MagickSafeReciprocal(gamma);
      SetPixelChannel(blur->blur_image,blur->channel,ClampToQuantum(
        gamma*pixel),q);
      return;
    }
  sum.image=blur->image;
  sum.luminance_image=blur->luminance_image;
  sum.width=blur->width;
  sum.intensity=blur->intensity;
  sum.threshold=blur->threshold;
  sum.i=i;
  targets.pixel=(&pixel);
  targets.gamma=(&gamma);
  targets.k=(&k);
  targets.pixels=(&pixels);
  targets.luminance_pixels=(&luminance_pixels);
  SumSelectiveBlendedPixels(&sum,&targets);
  if (fabs((double) gamma) < MagickEpsilon)
    {
      SetPixelChannel(blur->blur_image,blur->channel,blur->p[blur->center+i],
        q);
      return;
    }
  gamma=MagickSafeReciprocal(gamma);
  SetPixelChannel(blur->blur_image,blur->channel,ClampToQuantum(gamma*pixel),
    q);
}

typedef struct _SelectiveChannel
{
  const Image
    *image;

  Image
    *blur_image,
    *luminance_image;

  MagickRealType
    *kernel;

  const Quantum
    *p,
    *l;

  ssize_t
    center;

  double
    intensity,
    threshold;

  size_t
    width;
} SelectiveChannel;

static void BlurSelectiveChannel(const SelectiveChannel *selective,
  const ssize_t i,Quantum *magick_restrict q)
{
  PixelChannel
    channel;

  PixelTrait
    blur_traits,
    traits;

  SelectiveChannelSum
    blur;

  channel=GetPixelChannelChannel(selective->image,i);
  traits=GetPixelChannelTraits(selective->image,channel);
  blur_traits=GetPixelChannelTraits(selective->blur_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (blur_traits == UndefinedPixelTrait))
    return;
  if ((blur_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(selective->blur_image,channel,
        selective->p[selective->center+i],q);
      return;
    }
  blur.image=selective->image;
  blur.blur_image=selective->blur_image;
  blur.luminance_image=selective->luminance_image;
  blur.kernel=selective->kernel;
  blur.p=selective->p;
  blur.l=selective->l;
  blur.center=selective->center;
  blur.intensity=selective->intensity;
  blur.threshold=selective->threshold;
  blur.width=selective->width;
  blur.channel=channel;
  blur.blur_traits=blur_traits;
  BlurSelectiveChannelSum(&blur,i,q);
}

static void BuildSelectiveKernel(MagickRealType *kernel,const size_t width,
  const double sigma)
{
  ssize_t
    i,
    j,
    v;

  j=(ssize_t) (width-1)/2;
  i=0;
  for (v=(-j); v <= j; v++)
  {
    ssize_t
      u;

    for (u=(-j); u <= j; u++)
      kernel[i++]=(MagickRealType) (exp(-((double) u*u+v*v)/(2.0*MagickSigma*
        MagickSigma))/(2.0*MagickPI*MagickSigma*MagickSigma));
  }
}

MagickExport Image *SelectiveBlurImage(const Image *image,const double radius,
  const double sigma,const double threshold,ExceptionInfo *exception)
{
#define SelectiveBlurImageTag  "SelectiveBlur/Image"

  CacheView
    *blur_view,
    *image_view,
    *luminance_view;

  Image
    *blur_image,
    *luminance_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MagickRealType
    *kernel;

  size_t
    width;

  ssize_t
    center,
    y;

  /*
    Initialize blur image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel=(MagickRealType *) MagickAssumeAligned(AcquireAlignedMemory((size_t)
    width,width*sizeof(*kernel)));
  if (kernel == (MagickRealType *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  BuildSelectiveKernel(kernel,width,sigma);
  if (image->debug != MagickFalse)
    {
      char
        format[MagickPathExtent],
        *message;

      const MagickRealType
        *k;

      ssize_t
        u,
        v;

      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  SelectiveBlurImage with %.17gx%.17g kernel:",(double) width,(double)
        width);
      message=AcquireString("");
      k=kernel;
      for (v=0; v < (ssize_t) width; v++)
      {
        *message='\0';
        (void) FormatLocaleString(format,MagickPathExtent,"%.17g: ",(double) v);
        (void) ConcatenateString(&message,format);
        for (u=0; u < (ssize_t) width; u++)
        {
          (void) FormatLocaleString(format,MagickPathExtent,"%+f ",(double)
            *k++);
          (void) ConcatenateString(&message,format);
        }
        (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
      }
      message=DestroyString(message);
    }
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(blur_image,DirectClass,exception) == MagickFalse)
    {
      blur_image=DestroyImage(blur_image);
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      return((Image *) NULL);
    }
  luminance_image=CloneImage(image,0,0,MagickTrue,exception);
  if (luminance_image == (Image *) NULL)
    {
      blur_image=DestroyImage(blur_image);
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      return((Image *) NULL);
    }
  status=TransformImageColorspace(luminance_image,GRAYColorspace,exception);
  if (status == MagickFalse)
    {
      luminance_image=DestroyImage(luminance_image);
      blur_image=DestroyImage(blur_image);
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      return((Image *) NULL);
    }
  /*
    Threshold blur image.
  */
  status=MagickTrue;
  progress=0;
  center=(ssize_t) (GetPixelChannels(image)*(image->columns+width)*
    ((width-1)/2L)+GetPixelChannels(image)*((width-1)/2L));
  image_view=AcquireVirtualCacheView(image,exception);
  luminance_view=AcquireVirtualCacheView(luminance_image,exception);
  blur_view=AcquireAuthenticCacheView(blur_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,blur_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    MagickBooleanType
      sync;

    const Quantum
      *magick_restrict l,
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-((ssize_t) (width-1)/2L),y-(ssize_t)
      ((width-1)/2L),image->columns+width,width,exception);
    l=GetCacheViewVirtualPixels(luminance_view,-((ssize_t) (width-1)/2L),y-
      (ssize_t) ((width-1)/2L),luminance_image->columns+width,width,exception);
    q=QueueCacheViewAuthenticPixels(blur_view,0,y,blur_image->columns,1,
      exception);
    if (IsSelectiveRowMissing(p,l,q) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        intensity;

      ssize_t
        i;

      intensity=GetPixelIntensity(image,p+center);
      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        SelectiveChannel
          selective;

        selective.image=image;
        selective.blur_image=blur_image;
        selective.luminance_image=luminance_image;
        selective.kernel=kernel;
        selective.p=p;
        selective.l=l;
        selective.center=center;
        selective.intensity=intensity;
        selective.threshold=threshold;
        selective.width=width;
        BlurSelectiveChannel(&selective,i,q);
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
      l+=(ptrdiff_t) GetPixelChannels(luminance_image);
      q+=(ptrdiff_t) GetPixelChannels(blur_image);
    }
    sync=SyncCacheViewAuthenticPixels(blur_view,exception);
    if (sync == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SelectiveBlurImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  blur_image->type=image->type;
  blur_view=DestroyCacheView(blur_view);
  luminance_view=DestroyCacheView(luminance_view);
  image_view=DestroyCacheView(image_view);
  luminance_image=DestroyImage(luminance_image);
  kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
  if (status == MagickFalse)
    blur_image=DestroyImage(blur_image);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h a r p e n I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SharpenImage() sharpens the image.  We convolve the image with a Gaussian
%  operator of the given radius and standard deviation (sigma).  For
%  reasonable results, radius should be larger than sigma.  Use a radius of 0
%  and SharpenImage() selects a suitable radius for you.
%
%  Using a separable kernel would be faster, but the negative weights cancel
%  out on the corners of the kernel producing often undesirable ringing in the
%  filtered result; this can be avoided by using a 2D gaussian shaped image
%  sharpening kernel instead.
%
%  The format of the SharpenImage method is:
%
%    Image *SharpenImage(const Image *image,const double radius,
%      const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Laplacian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *SharpenImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  double
    normalize;

  Image
    *sharp_image;

  KernelInfo
    *kernel_info;

  ssize_t
    i;

  size_t
    width;

  ssize_t
    j,
    u,
    v;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel_info=AcquireKernelInfo((const char *) NULL,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(kernel_info,0,sizeof(*kernel_info));
  kernel_info->width=width;
  kernel_info->height=width;
  kernel_info->x=(ssize_t) (width-1)/2;
  kernel_info->y=(ssize_t) (width-1)/2;
  kernel_info->signature=MagickCoreSignature;
  kernel_info->values=(MagickRealType *) MagickAssumeAligned(
    AcquireAlignedMemory(kernel_info->width,kernel_info->height*
    sizeof(*kernel_info->values)));
  if (kernel_info->values == (MagickRealType *) NULL)
    {
      kernel_info=DestroyKernelInfo(kernel_info);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  normalize=0.0;
  j=(ssize_t) (kernel_info->width-1)/2;
  i=0;
  for (v=(-j); v <= j; v++)
  {
    for (u=(-j); u <= j; u++)
    {
      kernel_info->values[i]=(MagickRealType) (-exp(-((double) u*u+v*v)/(2.0*
        MagickSigma*MagickSigma))/(2.0*MagickPI*MagickSigma*MagickSigma));
      normalize+=kernel_info->values[i];
      i++;
    }
  }
  kernel_info->values[i/2]=(double) ((-2.0)*normalize);
  NormalizeKernelValues(kernel_info);
  sharp_image=ConvolveImage(image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  return(sharp_image);
}
