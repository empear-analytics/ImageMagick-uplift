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
%            AAA   DDDD    AAA   PPPP   TTTTT  IIIII  V   V  EEEEE            %
%           A   A  D   D  A   A  P   P    T      I    V   V  E                %
%           AAAAA  D   D  AAAAA  PPPP     T      I    V   V  EEE              %
%           A   A  D   D  A   A  P        T      I     V V   E                %
%           A   A  DDDD   A   A  P        T    IIIII    V    EEEEE            %
%                                                                             %
%                                                                             %
%                MagickCore Adaptive Blur and Sharpen Methods                 %
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
#include "MagickCore/effect-blur-adaptive-filters-private.h"

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

static MagickBooleanType FinishAdaptiveBlurKernels(double **kernel,
  const size_t width,const ssize_t w)
{
  if (w < (ssize_t) width)
    {
      RelinquishAdaptiveKernels(kernel,w);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AcquireAdaptiveBlurKernels(double **kernel,
  const size_t width,const double sigma)
{
  ssize_t
    w;

  for (w=0; w < (ssize_t) width; w+=2)
  {
    if (AcquireAdaptiveBlurKernel(kernel,width,w,sigma) == MagickFalse)
      break;
  }
  return(FinishAdaptiveBlurKernels(kernel,width,w));
}

static MagickBooleanType FillAdaptiveBlurKernelSet(double ***kernel,
  const size_t width,const double sigma)
{
  if (AcquireAdaptiveBlurKernels(*kernel,width,sigma) == MagickFalse)
    {
      *kernel=(double **) RelinquishAlignedMemory(*kernel);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AcquireAdaptiveBlurKernelSet(double ***kernel,
  const size_t width,const double sigma)
{
  *kernel=(double **) MagickAssumeAligned(AcquireAlignedMemory((size_t) width,
    sizeof(**kernel)));
  if (*kernel == (double **) NULL)
    return(MagickFalse);
  (void) memset(*kernel,0,(size_t) width*sizeof(**kernel));
  return(FillAdaptiveBlurKernelSet(kernel,width,sigma));
}

typedef struct _AdaptiveBlurRow
{
  const Image
    *image;

  Image
    *blur_image;

  Image
    *edge_image;

  CacheView
    *image_view;

  double
    **kernel;

  size_t
    width;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} AdaptiveBlurRow;

static void BlurAdaptiveRow(const AdaptiveBlurRow *row,
  const Quantum *magick_restrict r,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) row->blur_image->columns; x++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      i;

    ssize_t
      center,
      j;

    j=GetAdaptiveKernelIndex(row->edge_image,r,row->width);
    p=GetCacheViewVirtualPixels(row->image_view,x-((ssize_t) row->width-j)/2L,
      row->y-((ssize_t) row->width-j)/2L,row->width-(size_t) j,
      row->width-(size_t) j,row->exception);
    if (p == (const Quantum *) NULL)
      break;
    center=(ssize_t) (GetPixelChannels(row->image)*(row->width-(size_t) j)*
      ((row->width-(size_t) j)/2L)+GetPixelChannels(row->image)*
      ((row->width-(size_t) j)/2));
    for (i=0; i < (ssize_t) GetPixelChannels(row->blur_image); i++)
    {
      AdaptiveChannel
        adaptive;

      adaptive.image=row->image;
      adaptive.blur_image=row->blur_image;
      adaptive.kernel=row->kernel;
      adaptive.p=p;
      adaptive.width=row->width;
      adaptive.j=j;
      adaptive.center=center;
      BlurAdaptiveChannel(&adaptive,i,q);
    }
    q+=(ptrdiff_t) GetPixelChannels(row->blur_image);
    r+=(ptrdiff_t) GetPixelChannels(row->edge_image);
  }
}

typedef struct _AdaptiveEdge
{
  const Image
    *image;

  double
    radius,
    sigma;
} AdaptiveEdge;

static void ReplaceAdaptiveEdgeImage(Image **edge_image,Image *gaussian_image)
{
  if (gaussian_image != (Image *) NULL)
    {
      *edge_image=DestroyImage(*edge_image);
      *edge_image=gaussian_image;
    }
}

static MagickBooleanType PrepareAdaptiveEdge(const AdaptiveEdge *edge,
  Image **edge_image,ExceptionInfo *exception)
{
  Image
    *gaussian_image;

  *edge_image=EdgeImage(edge->image,edge->radius,exception);
  if (*edge_image == (Image *) NULL)
    return(MagickFalse);
  (void) AutoLevelImage(*edge_image,exception);
  gaussian_image=BlurImage(*edge_image,edge->radius,edge->sigma,exception);
  ReplaceAdaptiveEdgeImage(edge_image,gaussian_image);
  (void) AutoLevelImage(*edge_image,exception);
  return(MagickTrue);
}

static void RelinquishAdaptiveKernelSet(double **kernel,const size_t width)
{
  ssize_t
    w;

  for (w=0; w < (ssize_t) width; w+=2)
    kernel[w]=(double *) RelinquishAlignedMemory(kernel[w]);
  kernel=(double **) RelinquishAlignedMemory(kernel);
}

typedef struct _AdaptiveBlurLine
{
  const Image
    *image;

  Image
    *blur_image,
    *edge_image;

  CacheView
    *image_view,
    *edge_view,
    *blur_view;

  double
    **kernel;

  size_t
    width;

  ExceptionInfo
    *exception;
} AdaptiveBlurLine;

static MagickBooleanType BlurAdaptiveLine(const AdaptiveBlurLine *line,
  const ssize_t y,MagickBooleanType *status)
{
  const Image
    *const image = line->image;

  Image
    *const blur_image = line->blur_image,
    *const edge_image = line->edge_image;

  CacheView
    *const image_view = line->image_view,
    *const edge_view = line->edge_view,
    *const blur_view = line->blur_view;

  double
    **const kernel = line->kernel;

  const size_t
    width = line->width;

  ExceptionInfo
    *const exception = line->exception;

  const Quantum
    *magick_restrict r;

  Quantum
    *magick_restrict q;

  AdaptiveBlurRow
    row;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  r=GetCacheViewVirtualPixels(edge_view,0,y,edge_image->columns,1,exception);
  q=QueueCacheViewAuthenticPixels(blur_view,0,y,blur_image->columns,1,
    exception);
  if ((r == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  row.image=image;
  row.blur_image=blur_image;
  row.edge_image=edge_image;
  row.image_view=image_view;
  row.kernel=kernel;
  row.width=width;
  row.y=y;
  row.exception=exception;
  BlurAdaptiveRow(&row,r,q);
  if (SyncCacheViewAuthenticPixels(blur_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

typedef struct _AdaptiveBlur
{
  const Image
    *image;

  Image
    *blur_image,
    *edge_image;

  double
    **kernel;

  size_t
    width;

  ExceptionInfo
    *exception;
} AdaptiveBlur;

static MagickBooleanType BlurAdaptiveRows(const AdaptiveBlur *adaptive,
  MagickBooleanType status,MagickOffsetType progress)
{
  const Image
    *const image = adaptive->image;

  Image
    *const blur_image = adaptive->blur_image,
    *const edge_image = adaptive->edge_image;

  double
    **const kernel = adaptive->kernel;

  const size_t
    width = adaptive->width;

  ExceptionInfo
    *const exception = adaptive->exception;

  CacheView
    *blur_view,
    *edge_view,
    *image_view;

  ssize_t
    y;

  image_view=AcquireVirtualCacheView(image,exception);
  edge_view=AcquireVirtualCacheView(edge_image,exception);
  blur_view=AcquireAuthenticCacheView(blur_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,blur_image,blur_image->rows,1)
#endif
  for (y=0; y < (ssize_t) blur_image->rows; y++)
  {
    const AdaptiveBlurLine
      line = { image, blur_image, edge_image, image_view, edge_view, blur_view,
        kernel, width, exception };

    if (BlurAdaptiveLine(&line,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetBlurProgress(image,AdaptiveBlurImageTag,progress,&status);
      }
  }
  blur_image->type=image->type;
  blur_view=DestroyCacheView(blur_view);
  edge_view=DestroyCacheView(edge_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

MagickExport Image *AdaptiveBlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  AdaptiveBlur
    adaptive;

  double
    **kernel;

  AdaptiveEdge
    edge;

  Image
    *blur_image,
    *edge_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  size_t
    width;

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
  edge.image=image;
  edge.radius=radius;
  edge.sigma=sigma;
  if (PrepareAdaptiveEdge(&edge,&edge_image,exception) == MagickFalse)
    {
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  /*
    Create a set of kernels from maximum (radius,sigma) to minimum.
  */
  width=GetOptimalKernelWidth2D(radius,sigma);
  if (AcquireAdaptiveBlurKernelSet(&kernel,width,sigma) == MagickFalse)
    {
      edge_image=DestroyImage(edge_image);
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Adaptively blur image.
  */
  status=MagickTrue;
  progress=0;
  adaptive.image=image;
  adaptive.blur_image=blur_image;
  adaptive.edge_image=edge_image;
  adaptive.kernel=kernel;
  adaptive.width=width;
  adaptive.exception=exception;
  status=BlurAdaptiveRows(&adaptive,status,progress);
  edge_image=DestroyImage(edge_image);
  RelinquishAdaptiveKernelSet(kernel,width);
  return(ReleaseFailedImage(blur_image,status));
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

static MagickBooleanType SharpenAdaptiveLine(const AdaptiveBlurRow *row,
  CacheView *edge_view,CacheView *sharp_view,MagickBooleanType *status)
{
  const Image
    *const image = row->image;

  Image
    *const sharp_image = row->blur_image;

  Image
    *const edge_image = row->edge_image;

  double
    **const kernel = row->kernel;

  const size_t
    width = row->width;

  const ssize_t
    y = row->y;

  ExceptionInfo
    *const exception = row->exception;

  const Quantum
    *magick_restrict r;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  r=GetCacheViewVirtualPixels(edge_view,0,y,edge_image->columns,1,exception);
  q=QueueCacheViewAuthenticPixels(sharp_view,0,y,sharp_image->columns,1,
    exception);
  if ((r == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
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
    p=GetCacheViewVirtualPixels(row->image_view,x-(((ssize_t) width-j)/2L),y-
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
    (*status)=MagickFalse;
  return(MagickTrue);
}

#define AdaptiveSharpenImageTag  "Convolve/Image"

typedef struct _AdaptiveSharpen
{
  const Image
    *image;

  Image
    *sharp_image,
    *edge_image;

  double
    **kernel;

  size_t
    width;

  ExceptionInfo
    *exception;
} AdaptiveSharpen;

static Image *SharpenAdaptiveRows(const AdaptiveSharpen *adaptive)
{
  const Image
    *const image = adaptive->image;

  Image
    *const sharp_image = adaptive->sharp_image;

  Image
    *edge_image = adaptive->edge_image;

  double
    **kernel = adaptive->kernel;

  const size_t
    width = adaptive->width;

  ExceptionInfo
    *const exception = adaptive->exception;

  CacheView
    *sharp_view,
    *edge_view,
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    w,
    y;

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
    const AdaptiveBlurRow
      row = { image, sharp_image, edge_image, image_view, kernel, width, y,
        exception };

    if (SharpenAdaptiveLine(&row,edge_view,sharp_view,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetBlurProgress(image,AdaptiveSharpenImageTag,progress,&status);
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
  return(ReleaseFailedImage(sharp_image,status));
}

static ssize_t AcquireAdaptiveSharpenKernelRun(double **kernel,
  const size_t width,const double sigma)
{
  ssize_t
    w;

  for (w=0; w < (ssize_t) width; w+=2)
  {
    if (AcquireAdaptiveSharpenKernel(kernel,width,w,sigma) == MagickFalse)
      break;
  }
  return(w);
}

MagickExport Image *AdaptiveSharpenImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  AdaptiveEdge
    edge;

  AdaptiveSharpen
    adaptive;

  double
    **kernel;

  Image
    *sharp_image,
    *edge_image;

  size_t
    width;

  ssize_t
    w;

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
  edge.image=image;
  edge.radius=radius;
  edge.sigma=sigma;
  if (PrepareAdaptiveEdge(&edge,&edge_image,exception) == MagickFalse)
    {
      sharp_image=DestroyImage(sharp_image);
      return((Image *) NULL);
    }
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
  w=AcquireAdaptiveSharpenKernelRun(kernel,width,sigma);
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
  adaptive.image=image;
  adaptive.sharp_image=sharp_image;
  adaptive.edge_image=edge_image;
  adaptive.kernel=kernel;
  adaptive.width=width;
  adaptive.exception=exception;
  return(SharpenAdaptiveRows(&adaptive));
}
