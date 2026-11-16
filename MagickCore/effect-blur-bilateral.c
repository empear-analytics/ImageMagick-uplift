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
%        BBBB   IIIII  L       AAA   TTTTT  EEEEE  RRRR    AAA   L            %
%        B   B    I    L      A   A    T    E      R   R  A   A  L            %
%        BBBB     I    L      AAAAA    T    EEE    RRRR   AAAAA  L            %
%        B   B    I    L      A   A    T    E      R R    A   A  L            %
%        BBBB   IIIII  LLLLL  A   A    T    EEEEE  R  R   A   A  LLLLL        %
%                                                                             %
%                                                                             %
%              MagickCore Bilateral and Rotational Blur Methods               %
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
  Define declarations.
*/
#define BlurImageTag  "Blur/Image"

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

typedef struct _BilateralRow
{
  const Image
    *image;

  Image
    *blur_image;

  CacheView
    *image_view;

  OffsetInfo
    mid;

  size_t
    width,
    height;

  double
    *intensity_gaussian,
    *spatial_gaussian,
    **weights,
    intensity_sigma,
    spatial_sigma;

  int
    id;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} BilateralRow;

static void BlurBilateralRow(const BilateralRow *row,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) row->blur_image->columns; x++)
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
    p=GetCacheViewVirtualPixels(row->image_view,x-row->mid.x,row->y-row->mid.y,
      MagickMax(row->width,1),MagickMax(row->height,1),row->exception);
    if (p == (const Quantum *) NULL)
      break;
    p+=(ptrdiff_t) (GetPixelChannels(row->image)*MagickMax(row->width,1)*
      (size_t) row->mid.y+GetPixelChannels(row->image)*(size_t) row->mid.x);
    bilateral.image=row->image;
    bilateral.p=p;
    bilateral.width=row->width;
    bilateral.height=row->height;
    bilateral.mid=row->mid;
    bilateral.intensity_gaussian=row->intensity_gaussian;
    bilateral.spatial_gaussian=row->spatial_gaussian;
    bilateral.intensity_sigma=row->intensity_sigma;
    bilateral.spatial_sigma=row->spatial_sigma;
    bilateral.id=row->id;
    bilateral.x=x;
    bilateral.y=row->y;
    SetBilateralWeights(&bilateral,row->weights);
    for (i=0; i < (ssize_t) GetPixelChannels(row->blur_image); i++)
    {
      BilateralChannel
        bilateral_channel;

      bilateral_channel.image=row->image;
      bilateral_channel.p=p;
      bilateral_channel.width=row->width;
      bilateral_channel.height=row->height;
      bilateral_channel.mid=row->mid;
      bilateral_channel.weights=row->weights;
      bilateral_channel.id=row->id;
      BlurBilateralChannel(&bilateral_channel,row->blur_image,i,q);
    }
    q+=(ptrdiff_t) GetPixelChannels(row->blur_image);
  }
}

static MagickBooleanType CloneDirectImage(const Image *image,
  Image **blur_image,ExceptionInfo *exception)
{
  *blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*blur_image == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(*blur_image,DirectClass,exception) == MagickFalse)
    {
      *blur_image=DestroyImage(*blur_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _BilateralSpatial
{
  size_t
    width,
    height;

  OffsetInfo
    mid;

  double
    *spatial_gaussian,
    spatial_sigma;
} BilateralSpatial;

static void SetBilateralSpatial(const BilateralSpatial *spatial,ssize_t n)
{
  ssize_t
    v;

  for (v=0; v < (ssize_t) MagickMax(spatial->height,1); v++)
  {
    ssize_t
      u;

    for (u=0; u < (ssize_t) MagickMax(spatial->width,1); u++)
      spatial->spatial_gaussian[n++]=BlurGaussian(BlurDistance(0,0,
        u-spatial->mid.x,v-spatial->mid.y),spatial->spatial_sigma);
  }
}

static MagickBooleanType BlurBilateralLine(const BilateralRow *row,
  CacheView *blur_view,MagickBooleanType *status)
{
  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=QueueCacheViewAuthenticPixels(blur_view,0,row->y,row->blur_image->columns,1,
    row->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  BlurBilateralRow(row,q);
  if (SyncCacheViewAuthenticPixels(blur_view,row->exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void FillBilateralIntensity(double *intensity_gaussian,
  const double intensity_sigma)
{
  ssize_t
    w;

  for (w=(-MaxIntensity); w <= MaxIntensity; w++)
    intensity_gaussian[w+MaxIntensity]=BlurGaussian((double) w,intensity_sigma);
}

static OffsetInfo FillBilateralSpatial(const size_t width,
  const size_t height,double *spatial_gaussian,const double spatial_sigma)
{
  BilateralSpatial
    spatial;

  OffsetInfo
    mid;

  ssize_t
    n;

  n=0;
  mid.x=(ssize_t) (MagickMax(width,1)/2L);
  mid.y=(ssize_t) (MagickMax(height,1)/2L);
  spatial.width=width;
  spatial.height=height;
  spatial.mid=mid;
  spatial.spatial_gaussian=spatial_gaussian;
  spatial.spatial_sigma=spatial_sigma;
  SetBilateralSpatial(&spatial,n);
  return(mid);
}

typedef struct _BilateralBlur
{
  const Image
    *image;

  Image
    *blur_image;

  OffsetInfo
    mid;

  size_t
    width,
    height;

  double
    *intensity_gaussian,
    *spatial_gaussian,
    **weights,
    intensity_sigma,
    spatial_sigma;

  size_t
    number_threads;

  ExceptionInfo
    *exception;
} BilateralBlur;

static Image *BlurBilateralRows(const BilateralBlur *bilateral)
{
  const Image
    *const image = bilateral->image;

  Image
    *const blur_image = bilateral->blur_image;

  const OffsetInfo
    mid = bilateral->mid;

  const size_t
    width = bilateral->width,
    height = bilateral->height;

  double
    *const intensity_gaussian = bilateral->intensity_gaussian,
    *const spatial_gaussian = bilateral->spatial_gaussian,
    **weights = bilateral->weights;

  const double
    intensity_sigma = bilateral->intensity_sigma,
    spatial_sigma = bilateral->spatial_sigma;

  const size_t
    number_threads = bilateral->number_threads;

  ExceptionInfo
    *const exception = bilateral->exception;

  CacheView
    *blur_view,
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

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

    const BilateralRow
      row = { image, blur_image, image_view, mid, width, height,
        intensity_gaussian, spatial_gaussian, weights, intensity_sigma,
        spatial_sigma, id, y, exception };

    if (BlurBilateralLine(&row,blur_view,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetBlurProgress(image,BilateralBlurImageTag,progress,&status);
      }
  }
  blur_image->type=image->type;
  blur_view=DestroyCacheView(blur_view);
  image_view=DestroyCacheView(image_view);
  weights=DestroyBilateralTLS(number_threads,weights);
  return(ReleaseFailedImage(blur_image,status));
}

MagickExport Image *BilateralBlurImage(const Image *image,const size_t width,
  const size_t height,const double intensity_sigma,const double spatial_sigma,
  ExceptionInfo *exception)
{
  BilateralBlur
    bilateral;

  double
    intensity_gaussian[2*(MaxIntensity+1)],
    *spatial_gaussian,
    **weights;

  Image
    *blur_image;

  OffsetInfo
    mid;

  size_t
    number_threads;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneDirectImage(image,&blur_image,exception) == MagickFalse)
    return((Image *) NULL);
  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  weights=AcquireBilateralTLS(number_threads,MagickMax(width,1),
    MagickMax(height,1));
  if (weights == (double **) NULL)
    {
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  FillBilateralIntensity(intensity_gaussian,intensity_sigma);
  spatial_gaussian=weights[number_threads];
  mid=FillBilateralSpatial(width,height,spatial_gaussian,spatial_sigma);
  /*
    Bilateral blur image.
  */
  bilateral.image=image;
  bilateral.blur_image=blur_image;
  bilateral.mid=mid;
  bilateral.width=width;
  bilateral.height=height;
  bilateral.intensity_gaussian=intensity_gaussian;
  bilateral.spatial_gaussian=spatial_gaussian;
  bilateral.weights=weights;
  bilateral.intensity_sigma=intensity_sigma;
  bilateral.spatial_sigma=spatial_sigma;
  bilateral.number_threads=number_threads;
  bilateral.exception=exception;
  return(BlurBilateralRows(&bilateral));
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

static void RelinquishRotationalTables(double *cos_theta,double *sin_theta)
{
  if (cos_theta != (double *) NULL)
    cos_theta=(double *) RelinquishMagickMemory(cos_theta);
  if (sin_theta != (double *) NULL)
    sin_theta=(double *) RelinquishMagickMemory(sin_theta);
}

static MagickBooleanType BlurRotationalLine(const RotationalRow *row,
  CacheView *image_view,CacheView *blur_view,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,row->y,row->image->columns,1,
    row->exception);
  q=QueueCacheViewAuthenticPixels(blur_view,0,row->y,row->blur_image->columns,
    1,row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  if (BlurRotationalRow(row,p,q) == MagickFalse)
    (*status)=MagickFalse;
  if (SyncCacheViewAuthenticPixels(blur_view,row->exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void FillRotationalTables(double *cos_theta,double *sin_theta,
  const size_t n,const double theta)
{
  double
    offset;

  ssize_t
    w;

  offset=theta*(double) (n-1)/2.0;
  for (w=0; w < (ssize_t) n; w++)
  {
    cos_theta[w]=cos((double) (theta*w-offset));
    sin_theta[w]=sin((double) (theta*w-offset));
  }
}

typedef struct _RotationalBlur
{
  const Image
    *image;

  Image
    *blur_image;

  PointInfo
    blur_center;

  double
    blur_radius,
    *cos_theta,
    *sin_theta;

  size_t
    n;

  ExceptionInfo
    *exception;
} RotationalBlur;

static Image *BlurRotationalRows(const RotationalBlur *rotational)
{
  const Image
    *const image = rotational->image;

  Image
    *const blur_image = rotational->blur_image;

  const PointInfo
    blur_center = rotational->blur_center;

  const double
    blur_radius = rotational->blur_radius;

  double
    *cos_theta = rotational->cos_theta,
    *sin_theta = rotational->sin_theta;

  const size_t
    n = rotational->n;

  ExceptionInfo
    *const exception = rotational->exception;

  CacheView
    *blur_view,
    *image_view,
    *radial_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

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
    const RotationalRow
      row = { image, blur_image, radial_view, blur_center, blur_radius,
        cos_theta, sin_theta, n, y, exception };

    if (BlurRotationalLine(&row,image_view,blur_view,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetBlurProgress(image,BlurImageTag,progress,&status);
      }
  }
  blur_view=DestroyCacheView(blur_view);
  radial_view=DestroyCacheView(radial_view);
  image_view=DestroyCacheView(image_view);
  cos_theta=(double *) RelinquishMagickMemory(cos_theta);
  sin_theta=(double *) RelinquishMagickMemory(sin_theta);
  return(ReleaseFailedImage(blur_image,status));
}

MagickExport Image *RotationalBlurImage(const Image *image,const double angle,
  ExceptionInfo *exception)
{
  double
    blur_radius,
    *cos_theta,
    *sin_theta,
    theta;

  Image
    *blur_image;

  PointInfo
    blur_center;

  RotationalBlur
    rotational;

  size_t
    n;

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
  if (CloneDirectImage(image,&blur_image,exception) == MagickFalse)
    return((Image *) NULL);
  blur_center.x=(double) (image->columns-1)/2.0;
  blur_center.y=(double) (image->rows-1)/2.0;
  blur_radius=hypot(blur_center.x,blur_center.y);
  n=(size_t) fabs(4.0*DegreesToRadians(angle)*sqrt((double) blur_radius)+2UL);
  theta=DegreesToRadians(angle)/(double) (n-1);
  cos_theta=(double *) AcquireQuantumMemory((size_t) n,sizeof(*cos_theta));
  sin_theta=(double *) AcquireQuantumMemory((size_t) n,sizeof(*sin_theta));
  if ((cos_theta == (double *) NULL) || (sin_theta == (double *) NULL))
    {
      RelinquishRotationalTables(cos_theta,sin_theta);
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  FillRotationalTables(cos_theta,sin_theta,n,theta);
  /*
    Radial blur image.
  */
  rotational.image=image;
  rotational.blur_image=blur_image;
  rotational.blur_center=blur_center;
  rotational.blur_radius=blur_radius;
  rotational.cos_theta=cos_theta;
  rotational.sin_theta=sin_theta;
  rotational.n=n;
  rotational.exception=exception;
  return(BlurRotationalRows(&rotational));
}
