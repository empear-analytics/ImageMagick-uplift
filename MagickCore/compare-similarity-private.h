/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private methods shared by the image distortion and the image
  similarity methods: the per-metric similarity measures and their row and
  pixel accumulators.
*/
#ifndef MAGICKCORE_COMPARE_SIMILARITY_PRIVATE_H
#define MAGICKCORE_COMPARE_SIMILARITY_PRIVATE_H

#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/compare.h"
#include "MagickCore/compare-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/distort.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/fourier.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"
#include "MagickCore/compare-similarity-correlation-private.h"
#include "MagickCore/compare-similarity-metric-private.h"
#include "MagickCore/compare-similarity-phase-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _SSIMTap
{
  const Image
    *image,
    *reconstruct_image;

  MagickRealType
    *k;

  const Quantum
    *test,
    *reconstruct;
} SSIMTap;

typedef struct _SSIMTapTargets
{
  double
    (*x_pixel_mu)[MaxPixelChannels+1],
    (*x_pixel_sigma_squared)[MaxPixelChannels+1],
    (*y_pixel_mu)[MaxPixelChannels+1],
    (*y_pixel_sigma_squared)[MaxPixelChannels+1],
    (*xy_sigma)[MaxPixelChannels+1];
} SSIMTapTargets;

static inline void AccumulateSSIMTap(const SSIMTap *tap,
  const SSIMTapTargets *targets)
{
  const Image
    *const image = tap->image,
    *const reconstruct_image = tap->reconstruct_image;

  MagickRealType
    *const k = tap->k;

  const Quantum
    *const test = tap->test,
    *const reconstruct = tap->reconstruct;

  double
    (*const x_pixel_mu)[MaxPixelChannels+1] = targets->x_pixel_mu,
    (*const x_pixel_sigma_squared)[MaxPixelChannels+1] =
      targets->x_pixel_sigma_squared,
    (*const y_pixel_mu)[MaxPixelChannels+1] = targets->y_pixel_mu,
    (*const y_pixel_sigma_squared)[MaxPixelChannels+1] =
      targets->y_pixel_sigma_squared,
    (*const xy_sigma)[MaxPixelChannels+1] = targets->xy_sigma;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      x_pixel,
      y_pixel;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(
      reconstruct_image,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    x_pixel=QuantumScale*(double) test[i];
    (*x_pixel_mu)[i]+=(*k)*x_pixel;
    (*x_pixel_sigma_squared)[i]+=(*k)*x_pixel*x_pixel;
    y_pixel=QuantumScale*(double)
      GetPixelChannel(reconstruct_image,channel,reconstruct);
    (*y_pixel_mu)[i]+=(*k)*y_pixel;
    (*y_pixel_sigma_squared)[i]+=(*k)*y_pixel*y_pixel;
    (*xy_sigma)[i]+=(*k)*x_pixel*y_pixel;
  }
}

typedef struct _SSIMWindow
{
  KernelInfo
    *kernel_info;

  const Image
    *image,
    *reconstruct_image;

  const Quantum
    *p,
    *q;

  size_t
    columns;
} SSIMWindow;

static inline void SumSSIMWindow(const SSIMWindow *window,
  const SSIMTapTargets *sums)
{
  KernelInfo
    *const kernel_info = window->kernel_info;

  const Image
    *const image = window->image,
    *const reconstruct_image = window->reconstruct_image;

  const Quantum
    *const p = window->p,
    *const q = window->q;

  const size_t
    columns = window->columns;

  const Quantum
    *magick_restrict reconstruct,
    *magick_restrict test;

  MagickRealType
    *k;

  ssize_t
    v;

  k=kernel_info->values;
  test=p;
  reconstruct=q;
  for (v=0; v < (ssize_t) kernel_info->height; v++)
  {
    ssize_t
      u;

    for (u=0; u < (ssize_t) kernel_info->width; u++)
    {
      SSIMTap
        tap;

      SSIMTapTargets
        targets;

      tap.image=image;
      tap.reconstruct_image=reconstruct_image;
      tap.k=k;
      tap.test=test;
      tap.reconstruct=reconstruct;
      targets.x_pixel_mu=(sums->x_pixel_mu);
      targets.x_pixel_sigma_squared=(sums->x_pixel_sigma_squared);
      targets.y_pixel_mu=(sums->y_pixel_mu);
      targets.y_pixel_sigma_squared=(sums->y_pixel_sigma_squared);
      targets.xy_sigma=(sums->xy_sigma);
      AccumulateSSIMTap(&tap,&targets);
      k++;
      test+=(ptrdiff_t) GetPixelChannels(image);
      reconstruct+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
    }
    test+=(ptrdiff_t) ((double) GetPixelChannels(image)*(double) columns);
    reconstruct+=(ptrdiff_t) ((double) GetPixelChannels(reconstruct_image)*
      (double) columns);
  }
}

typedef struct _SSIMChannelInputs
{
  const Image
    *image,
    *reconstruct_image;

  double
    *x_pixel_mu,
    *x_pixel_sigma_squared,
    *xy_sigma,
    *y_pixel_mu,
    *y_pixel_sigma_squared,
    c1,
    c2;
} SSIMChannelInputs;

static magick_attribute((__always_inline__)) inline void
  AccumulateSSIMChannels(const SSIMChannelInputs *inputs,
  double *channel_similarity)
{
  const Image
    *const image = inputs->image,
    *const reconstruct_image = inputs->reconstruct_image;

  double
    *const x_pixel_mu = inputs->x_pixel_mu,
    *const x_pixel_sigma_squared = inputs->x_pixel_sigma_squared,
    *const xy_sigma = inputs->xy_sigma,
    *const y_pixel_mu = inputs->y_pixel_mu,
    *const y_pixel_sigma_squared = inputs->y_pixel_sigma_squared;

  const double
    c1 = inputs->c1,
    c2 = inputs->c2;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      ssim,
      x_pixel_mu_squared,
      x_pixel_sigmas_squared,
      xy_mu,
      xy_sigmas,
      y_pixel_mu_squared,
      y_pixel_sigmas_squared;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(
      reconstruct_image,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    x_pixel_mu_squared=x_pixel_mu[i]*x_pixel_mu[i];
    y_pixel_mu_squared=y_pixel_mu[i]*y_pixel_mu[i];
    xy_mu=x_pixel_mu[i]*y_pixel_mu[i];
    xy_sigmas=xy_sigma[i]-xy_mu;
    x_pixel_sigmas_squared=x_pixel_sigma_squared[i]-x_pixel_mu_squared;
    y_pixel_sigmas_squared=y_pixel_sigma_squared[i]-y_pixel_mu_squared;
    ssim=((2.0*xy_mu+c1)*(2.0*xy_sigmas+c2))*
      MagickSafeReciprocal((x_pixel_mu_squared+y_pixel_mu_squared+c1)*
       (x_pixel_sigmas_squared+y_pixel_sigmas_squared+c2));
    channel_similarity[i]+=ssim;
    channel_similarity[CompositePixelChannel]+=ssim;
  }
}

#define SSIMRadius  5.0
#define SSIMSigma  1.5
#define SSIMK1  0.01
#define SSIMK2  0.03
#define SSIML  1.0

static inline KernelInfo *AcquireSSIMKernel(const Image *image,
  ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  const char
    *artifact;

  double
    radius,
    sigma;

  radius=SSIMRadius;
  artifact=GetImageArtifact(image,"compare:ssim-radius");
  if (artifact != (const char *) NULL)
    radius=StringToDouble(artifact,(char **) NULL);
  sigma=SSIMSigma;
  artifact=GetImageArtifact(image,"compare:ssim-sigma");
  if (artifact != (const char *) NULL)
    sigma=StringToDouble(artifact,(char **) NULL);
  (void) FormatLocaleString(geometry,MagickPathExtent,"gaussian:%.17gx%.17g",
    radius,sigma);
  return(AcquireKernelInfo(geometry,exception));
}

static void MergeSSIMSimilarity(const Image *image,
  const Image *reconstruct_image,const double *channel_similarity,
  double *similarity)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,j);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[j]+=channel_similarity[j];
  }
  similarity[CompositePixelChannel]+=
    channel_similarity[CompositePixelChannel];
}

typedef struct _SSIMConstant
{
  double
    k;

  const char
    *option;
} SSIMConstant;

static inline double GetSSIMConstant(const Image *image,
  const SSIMConstant *constant)
{
  const double
    k = constant->k;

  const char
    *const option = constant->option;

  const char
    *artifact;

  double
    c;

  c=pow(k*SSIML,2.0);
  artifact=GetImageArtifact(image,option);
  if (artifact != (const char *) NULL)
    c=pow(StringToDouble(artifact,(char **) NULL)*SSIML,2.0);
  return(c);
}

static inline MagickBooleanType GetSSIMSimularity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *reconstruct_view;

  const SSIMConstant
    ssim_k1 = { SSIMK1, "compare:ssim-k1" },
    ssim_k2 = { SSIMK2, "compare:ssim-k2" };

  double
    area = 0.0,
    c1,
    c2;

  KernelInfo
    *kernel_info;

  MagickBooleanType
    status = MagickTrue;

  size_t
    columns,
    rows;

  ssize_t
    y;

  /*
    Compute the structual similarity index similarity.
  */
  kernel_info=AcquireSSIMKernel(image,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  c1=GetSSIMConstant(image,&ssim_k1);
  c2=GetSSIMConstant(image,&ssim_k2);
  SetImageCompareBounds(image,reconstruct_image,&columns,&rows);
  image_view=AcquireVirtualCacheView(image,exception);
  reconstruct_view=AcquireVirtualCacheView(reconstruct_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(area,similarity,status) \
    magick_number_threads(image,reconstruct_image,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    double
      channel_area = 0.0,
      channel_similarity[MaxPixelChannels+1] = { 0.0 };

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-((ssize_t) kernel_info->width/2L),y-
      ((ssize_t) kernel_info->height/2L),columns+kernel_info->width,
      kernel_info->height,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,-((ssize_t) kernel_info->width/
      2L),y-((ssize_t) kernel_info->height/2L),columns+kernel_info->width,
      kernel_info->height,exception);
    if ((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) columns; x++)
    {
      double
        x_pixel_mu[MaxPixelChannels+1] = { 0.0 },
        x_pixel_sigma_squared[MaxPixelChannels+1] = { 0.0 },
        xy_sigma[MaxPixelChannels+1] = { 0.0 },
        y_pixel_mu[MaxPixelChannels+1] = { 0.0 },
        y_pixel_sigma_squared[MaxPixelChannels+1] = { 0.0 };

      SSIMTapTargets
        sums;

      SSIMWindow
        window;

      if ((GetPixelReadMask(image,p) <= (QuantumRange/2)) ||
          (GetPixelReadMask(reconstruct_image,q) <= (QuantumRange/2)))
        {
          p+=(ptrdiff_t) GetPixelChannels(image);
          q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
          continue;
        }
      window.kernel_info=kernel_info;
      window.image=image;
      window.reconstruct_image=reconstruct_image;
      window.p=p;
      window.q=q;
      window.columns=columns;
      sums.x_pixel_mu=(&x_pixel_mu);
      sums.x_pixel_sigma_squared=(&x_pixel_sigma_squared);
      sums.y_pixel_mu=(&y_pixel_mu);
      sums.y_pixel_sigma_squared=(&y_pixel_sigma_squared);
      sums.xy_sigma=(&xy_sigma);
      SumSSIMWindow(&window,&sums);
      {
        const SSIMChannelInputs channel_inputs = { image, reconstruct_image,
          x_pixel_mu, x_pixel_sigma_squared, xy_sigma, y_pixel_mu,
          y_pixel_sigma_squared, c1, c2 };

        AccumulateSSIMChannels(&channel_inputs,channel_similarity);
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
      channel_area++;
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetSSIMSimularity)
#endif
    {
      area+=channel_area;
      MergeSSIMSimilarity(image,reconstruct_image,channel_similarity,
        similarity);
    }
  }
  image_view=DestroyCacheView(image_view);
  reconstruct_view=DestroyCacheView(reconstruct_view);
  kernel_info=DestroyKernelInfo(kernel_info);
  NormalizeChannelSimilarity(image,reconstruct_image,area,similarity);
  return(status);
}

static inline MagickBooleanType GetDSSIMSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  MagickBooleanType
    status = MagickTrue;

  ssize_t
    i;

  /*
    Compute the structual dissimilarity index similarity.
  */
  status=GetSSIMSimularity(image,reconstruct_image,similarity,exception);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[i]=(1.0-similarity[i])/2.0;
  }
  similarity[CompositePixelChannel]=(1.0-similarity[CompositePixelChannel])/2.0;
  return(status);
}

typedef MagickBooleanType
  (*SimilarityMetricMethod)(const Image *,const Image *,double *,
    ExceptionInfo *);

typedef struct _SimilarityMetricEntry
{
  MetricType
    metric;

  SimilarityMetricMethod
    get_similarity;
} SimilarityMetricEntry;

static const SimilarityMetricEntry
  SimilarityMetricEntries[] =
  {
    { AbsoluteErrorMetric, GetAESimilarity },
    { DotProductCorrelationErrorMetric, GetDPCSimilarity },
    { FuzzErrorMetric, GetFUZZSimilarity },
    { MeanAbsoluteErrorMetric, GetMAESimilarity },
    { MeanSquaredErrorMetric, GetMSESimilarity },
    { NormalizedCrossCorrelationErrorMetric, GetNCCSimilarity },
    { PeakAbsoluteErrorMetric, GetPASimilarity },
    { PeakSignalToNoiseRatioErrorMetric, GetPSNRSimilarity },
    { PerceptualHashErrorMetric, GetPHASHSimilarity },
    { PhaseCorrelationErrorMetric, GetPHASESimilarity },
    { PixelDifferenceCountErrorMetric, GetPDCSimilarity },
    { StructuralDissimilarityErrorMetric, GetDSSIMSimilarity },
    { StructuralSimilarityErrorMetric, GetSSIMSimularity }
  };

static inline const SimilarityMetricEntry *GetSimilarityMetricEntry(
  const MetricType metric)
{
  size_t
    i;

  for (i=0; i < (sizeof(SimilarityMetricEntries)/
       sizeof(*SimilarityMetricEntries)); i++)
    if (SimilarityMetricEntries[i].metric == metric)
      return(SimilarityMetricEntries+i);
  return((const SimilarityMetricEntry *) NULL);
}

typedef struct _SimilarityImages
{
  Image
    *image;

  const Image
    *reconstruct_image;
} SimilarityImages;

static inline MagickBooleanType GetSimilarityForMetric(
  const SimilarityImages *images,const MetricType metric,
  double *channel_similarity,ExceptionInfo *exception)
{
  Image
    *const image = images->image;

  const Image
    *const reconstruct_image = images->reconstruct_image;

  MagickBooleanType
    status = MagickTrue;

  switch (metric)
  {
    case MeanErrorPerPixelErrorMetric:
    {
      status=GetMEPPSimilarity(image,reconstruct_image,channel_similarity,
        exception);
      break;
    }
    default:
    {
      const SimilarityMetricEntry
        *entry = GetSimilarityMetricEntry(metric);

      if (entry == (const SimilarityMetricEntry *) NULL)
        status=GetRMSESimilarity(image,reconstruct_image,channel_similarity,
          exception);
      else
        status=entry->get_similarity(image,reconstruct_image,
          channel_similarity,exception);
      break;
    }
  }
  return(status);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
