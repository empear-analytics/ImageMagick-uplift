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

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Typedef declarations.
*/
typedef struct _ComparePixels
{
  const Image
    *image,
    *reconstruct_image;

  const Quantum
    *p,
    *q;
} ComparePixels;

typedef struct _PixelMetricInfo
{
  const Image
    *image,
    *reconstruct_image;

  MetricType
    metric;

  double
    fuzz;
} PixelMetricInfo;

typedef struct _PixelMetricSums
{
  double
    area,
    similarity[MaxPixelChannels+1];
} PixelMetricSums;

typedef struct _PixelMetricRow
{
  const Image
    *image,
    *reconstruct_image;

  const Quantum
    *p,
    *q;

  size_t
    columns;

  double
    fuzz;
} PixelMetricRow;

static MagickBooleanType
  GetPixelMetricSimilarity(const PixelMetricInfo *,double *,double *,
    ExceptionInfo *);

static inline void AccumulateAEPixel(const ComparePixels *pixels,const double fuzz,
  double *channel_similarity)
{
  const Image
    *image = pixels->image,
    *reconstruct_image = pixels->reconstruct_image;

  const Quantum
    *p = pixels->p,
    *q = pixels->q;

  double
    Da,
    Sa;

  ssize_t
    i;

  Sa=QuantumScale*(double) GetPixelAlpha(image,p);
  Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      error;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      error=(double) p[i]-(double) GetPixelChannel(reconstruct_image,
        channel,q);
    else
      error=Sa*(double) p[i]-Da*(double) GetPixelChannel(reconstruct_image,channel,
        q);
    if (MagickSafeSignificantError(error*error,fuzz) != MagickFalse)
      {
        double ae = fabs(QuantumScale*error);
        channel_similarity[i]+=ae;
        channel_similarity[CompositePixelChannel]+=ae;
      }
  }
}

static inline void NormalizeChannelSimilarity(const Image *image,
  const Image *reconstruct_image,double area,double *similarity)
{
  ssize_t
    channels = 0,
    k;

  area=MagickSafeReciprocal(area);
  for (k=0; k < (ssize_t) GetPixelChannels(image); k++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,k);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[k]*=area;
    channels++;
  }
  similarity[CompositePixelChannel]*=area;
  if (channels != 0)
    similarity[CompositePixelChannel]/=(double) channels;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e D i s t o r t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageDistortion() compares one or more pixel channels of an image to a
%  reconstructed image and returns the specified distortion metric.
%
%  The format of the GetImageDistortion method is:
%
%      MagickBooleanType GetImageDistortion(const Image *image,
%        const Image *reconstruct_image,const MetricType metric,
%        double *distortion,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruction image.
%
%    o metric: the metric.
%
%    o distortion: the computed distortion between the images.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType GetAESimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  const PixelMetricInfo
    info = { image, reconstruct_image, AbsoluteErrorMetric,
      GetFuzzyColorDistance(image,reconstruct_image) };

  double
    area;

  MagickBooleanType
    status;

  size_t
    columns,
    rows;

  /*
    Compute the absolute error similarity.
  */
  (void) memset(similarity,0,(MaxPixelChannels+1)*sizeof(*similarity));
  status=GetPixelMetricSimilarity(&info,similarity,&area,exception);
  SetImageCompareBounds(image,reconstruct_image,&columns,&rows);
  NormalizeChannelSimilarity(image,reconstruct_image,(double) columns*
    (double) rows,similarity);
  return(status);
}

typedef struct _CorrelationSums
{
  double
    *channel_similarity,
    *channel_norm,
    *channel_reconstruct_norm;
} CorrelationSums;

static inline void AccumulateCorrelationPixel(const ComparePixels *pixels,
  const ChannelStatistics *image_statistics,
  const ChannelStatistics *reconstruct_statistics,const CorrelationSums *sums)
{
  const Image
    *image = pixels->image,
    *reconstruct_image = pixels->reconstruct_image;

  double
    *const channel_similarity = sums->channel_similarity,
    *const channel_norm = sums->channel_norm,
    *const channel_reconstruct_norm = sums->channel_reconstruct_norm;

  const Quantum
    *p = pixels->p,
    *q = pixels->q;

  double
    Da,
    Sa;

  ssize_t
    i;

  Sa=QuantumScale*(double) GetPixelAlpha(image,p);
  Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      alpha,
      beta;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      {
        alpha=QuantumScale*((double) p[i]-image_statistics[channel].mean);
        beta=QuantumScale*((double) GetPixelChannel(reconstruct_image,
          channel,q)-reconstruct_statistics[channel].mean);
      }
    else
      {
        alpha=QuantumScale*(Sa*(double) p[i]-
          image_statistics[channel].mean);
        beta=QuantumScale*(Da*(double) GetPixelChannel(reconstruct_image,channel,
          q)-reconstruct_statistics[channel].mean);
      }
    channel_similarity[i]+=alpha*beta;
    channel_norm[i]+=alpha*alpha;
    channel_reconstruct_norm[i]+=beta*beta;
  }
}

typedef struct _CorrelationRow
{
  const Image
    *image,
    *reconstruct_image;

  size_t
    columns;

  ChannelStatistics
    *image_statistics,
    *reconstruct_statistics;
} CorrelationRow;

static inline void AccumulateCorrelationRow(const CorrelationRow *row,
  const Quantum *magick_restrict p,const Quantum *magick_restrict q,
  const CorrelationSums sums)
{
  const Image
    *const image = row->image,
    *const reconstruct_image = row->reconstruct_image;

  const size_t
    columns = row->columns;

  ChannelStatistics
    *const image_statistics = row->image_statistics,
    *const reconstruct_statistics = row->reconstruct_statistics;

  ssize_t
    x;

  for (x=0; x < (ssize_t) columns; x++)
  {
    const ComparePixels pixels = { image, reconstruct_image, p, q };

    if ((GetPixelReadMask(image,p) <= (QuantumRange/2)) ||
        (GetPixelReadMask(reconstruct_image,q) <= (QuantumRange/2)))
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
        continue;
      }
    AccumulateCorrelationPixel(&pixels,image_statistics,
      reconstruct_statistics,&sums);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
  }
}

typedef struct _CorrelationStatisticsTargets
{
  ChannelStatistics
    **image_statistics,
    **reconstruct_statistics;
} CorrelationStatisticsTargets;

static inline MagickBooleanType AcquireCorrelationStatistics(const Image *image,
  const Image *reconstruct_image,ExceptionInfo *exception,
  const CorrelationStatisticsTargets *targets)
{
  ChannelStatistics
    **const image_statistics = targets->image_statistics,
    **const reconstruct_statistics = targets->reconstruct_statistics;

  *image_statistics=GetImageStatistics(image,exception);
  *reconstruct_statistics=GetImageStatistics(reconstruct_image,exception);
  if ((*image_statistics == (ChannelStatistics *) NULL) ||
      (*reconstruct_statistics == (ChannelStatistics *) NULL))
    {
      if (*image_statistics != (ChannelStatistics *) NULL)
        *image_statistics=(ChannelStatistics *) RelinquishMagickMemory(
          *image_statistics);
      if (*reconstruct_statistics != (ChannelStatistics *) NULL)
        *reconstruct_statistics=(ChannelStatistics *) RelinquishMagickMemory(
          *reconstruct_statistics);
      return(MagickFalse);
    }
  return(MagickTrue);
}

#define SimilarityImageTag  "Similarity/Image"

typedef struct _CorrelationTotals
{
  const Image
    *image,
    *reconstruct_image;

  ChannelStatistics
    *image_statistics,
    *reconstruct_statistics;

  double
    *similarity,
    *norm,
    *reconstruct_norm;
} CorrelationTotals;

static MagickBooleanType GetCorrelationTotals(const CorrelationTotals *totals,
  ExceptionInfo *exception)
{
  const Image
    *const image = totals->image,
    *const reconstruct_image = totals->reconstruct_image;

  ChannelStatistics
    *const image_statistics = totals->image_statistics,
    *const reconstruct_statistics = totals->reconstruct_statistics;

  double
    *const similarity = totals->similarity,
    *const norm = totals->norm,
    *const reconstruct_norm = totals->reconstruct_norm;

  CacheView
    *image_view,
    *reconstruct_view;

  MagickBooleanType
    status = MagickTrue;

  MagickOffsetType
    progress = 0;

  size_t
    columns,
    rows;

  ssize_t
    y;

  SetImageCompareBounds(image,reconstruct_image,&columns,&rows);
  image_view=AcquireVirtualCacheView(image,exception);
  reconstruct_view=AcquireVirtualCacheView(reconstruct_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(norm,reconstruct_norm,similarity,status) \
    magick_number_threads(image,image,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    double
      channel_norm[MaxPixelChannels+1] = { 0.0 },
      channel_reconstruct_norm[MaxPixelChannels+1] = { 0.0 },
      channel_similarity[MaxPixelChannels+1] = { 0.0 };

    const CorrelationSums
      sums = { channel_similarity, channel_norm,
        channel_reconstruct_norm };

    CorrelationRow
      row;

    p=GetCacheViewVirtualPixels(image_view,0,y,columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    row.image=image;
    row.reconstruct_image=reconstruct_image;
    row.columns=columns;
    row.image_statistics=image_statistics;
    row.reconstruct_statistics=reconstruct_statistics;
    AccumulateCorrelationRow(&row,p,q,sums);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetCorrelationTotals)
#endif
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
        similarity[CompositePixelChannel]+=channel_similarity[j];
        norm[j]+=channel_norm[j];
        norm[CompositePixelChannel]+=channel_norm[j];
        reconstruct_norm[j]+=channel_reconstruct_norm[j];
        reconstruct_norm[CompositePixelChannel]+=channel_reconstruct_norm[j];
      }
    }
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SimilarityImageTag,progress,rows);
        if (proceed == MagickFalse)
          {
            status=MagickFalse;
            continue;
          }
      }
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

static inline MagickBooleanType GetDPCSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  ChannelStatistics
    *image_statistics,
    *reconstruct_statistics;

  CorrelationStatisticsTargets
    targets;

  CorrelationTotals
    totals;

  double
    norm[MaxPixelChannels+1] = { 0.0 },
    reconstruct_norm[MaxPixelChannels+1] = { 0.0 };

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    k;

  /*
    Compute the dot product correlation similarity.
  */
  targets.image_statistics=(&image_statistics);
  targets.reconstruct_statistics=(&reconstruct_statistics);
  if (AcquireCorrelationStatistics(image,reconstruct_image,exception,
        &targets) == MagickFalse)
    return(MagickFalse);
  (void) memset(similarity,0,(MaxPixelChannels+1)*sizeof(*similarity));
  totals.image=image;
  totals.reconstruct_image=reconstruct_image;
  totals.image_statistics=image_statistics;
  totals.reconstruct_statistics=reconstruct_statistics;
  totals.similarity=similarity;
  totals.norm=norm;
  totals.reconstruct_norm=reconstruct_norm;
  status=GetCorrelationTotals(&totals,exception);
  /*
    Compute dot product correlation: divide by mean.
  */
  for (k=0; k < (ssize_t) GetPixelChannels(image); k++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,k);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[k]*=MagickSafeReciprocal(sqrt(norm[k]*reconstruct_norm[k]));
  }
  similarity[CompositePixelChannel]*=MagickSafeReciprocal(sqrt(
    norm[CompositePixelChannel]*reconstruct_norm[CompositePixelChannel]));
  /*
    Free resources.
  */
  reconstruct_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    reconstruct_statistics);
  image_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    image_statistics);
  return(status);
}

static inline void AccumulateFUZZPixel(const ComparePixels *pixels,const double fuzz,
  double *channel_similarity,double *channel_area)
{
  const Image
    *image = pixels->image,
    *reconstruct_image = pixels->reconstruct_image;

  const Quantum
    *p = pixels->p,
    *q = pixels->q;

  double
    Da,
    Sa;

  ssize_t
    i;

  Sa=QuantumScale*(double) GetPixelAlpha(image,p);
  Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      error;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      error=(double) p[i]-(double) GetPixelChannel(reconstruct_image,
        channel,q);
    else
      error=Sa*(double) p[i]-Da*(double) GetPixelChannel(reconstruct_image,
        channel,q);
    if (MagickSafeSignificantError(error*error,fuzz) != MagickFalse)
      {
        channel_similarity[i]+=QuantumScale*error*QuantumScale*error;
        channel_similarity[CompositePixelChannel]+=QuantumScale*error*
          QuantumScale*error;
        (*channel_area)++;
      }
  }
}

static inline MagickBooleanType GetFUZZSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  const PixelMetricInfo
    info = { image, reconstruct_image, FuzzErrorMetric,
      GetFuzzyColorDistance(image,reconstruct_image) };

  double
    area;

  MagickBooleanType
    status;

  ssize_t
    k;

  /*
    Compute the MSE similarity within tolerance (fuzz).
  */
  status=GetPixelMetricSimilarity(&info,similarity,&area,exception);
  area=MagickSafeReciprocal(area);
  for (k=0; k < (ssize_t) GetPixelChannels(image); k++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,k);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[k]*=area;
  }
  similarity[CompositePixelChannel]*=area;
  return(status);
}

static inline void AccumulateMAEPixel(const ComparePixels *pixels,
  double *channel_similarity)
{
  const Image
    *image = pixels->image,
    *reconstruct_image = pixels->reconstruct_image;

  const Quantum
    *p = pixels->p,
    *q = pixels->q;

  double
    Da,
    Sa;

  ssize_t
    i;

  Sa=QuantumScale*(double) GetPixelAlpha(image,p);
  Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      error;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      error=QuantumScale*fabs((double) p[i]-(double) GetPixelChannel(
        reconstruct_image,channel,q));
    else
      error=QuantumScale*fabs(Sa*(double) p[i]-Da*(double)
        GetPixelChannel(reconstruct_image,channel,q));
    channel_similarity[i]+=error;
    channel_similarity[CompositePixelChannel]+=error;
  }
}

static inline MagickBooleanType GetMAESimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  const PixelMetricInfo
    info = { image, reconstruct_image, MeanAbsoluteErrorMetric,
      0.0 };

  double
    area;

  MagickBooleanType
    status;

  /*
    Compute the mean absolute error similarity.
  */
  (void) memset(similarity,0,(MaxPixelChannels+1)*sizeof(*similarity));
  status=GetPixelMetricSimilarity(&info,similarity,&area,exception);
  NormalizeChannelSimilarity(image,reconstruct_image,area,similarity);
  return(status);
}

static inline void AccumulateMEPPPixel(const ComparePixels *pixels,
  double *channel_similarity,double *channel_mean_error,
  double *channel_maximum_error)
{
  const Image
    *image = pixels->image,
    *reconstruct_image = pixels->reconstruct_image;

  const Quantum
    *p = pixels->p,
    *q = pixels->q;

  double
    Da,
    Sa;

  ssize_t
    i;

  Sa=QuantumScale*(double) GetPixelAlpha(image,p);
  Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      error;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      error=QuantumScale*fabs((double) p[i]-(double) GetPixelChannel(
        reconstruct_image,channel,q));
    else
      error=QuantumScale*fabs(Sa*(double) p[i]-Da*(double)
        GetPixelChannel(reconstruct_image,channel,q));
    channel_similarity[i]+=error;
    channel_similarity[CompositePixelChannel]+=error;
    *channel_mean_error+=error*error;
    if (error > *channel_maximum_error)
      *channel_maximum_error=error;
  }
}

static inline MagickBooleanType GetMEPPSimilarity(Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *reconstruct_view;

  double
    area = 0.0,
    maximum_error = -MagickMaximumValue,
    mean_error = 0.0;

  MagickBooleanType
    status = MagickTrue;

  size_t
    columns,
    rows;

  ssize_t
    channels = 0,
    k,
    y;

  /*
    Compute the mean error per pixel similarity.
  */
  SetImageCompareBounds(image,reconstruct_image,&columns,&rows);
  image_view=AcquireVirtualCacheView(image,exception);
  reconstruct_view=AcquireVirtualCacheView(reconstruct_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(area,similarity,maximum_error,mean_error,status) \
    magick_number_threads(image,image,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict q;

    double
      channel_area = 0.0,
      channel_similarity[MaxPixelChannels+1] = { 0.0 },
      channel_maximum_error = maximum_error,
      channel_mean_error = 0.0;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) columns; x++)
    {
      const ComparePixels pixels = { image, reconstruct_image, p, q };

      if ((GetPixelReadMask(image,p) <= (QuantumRange/2)) ||
          (GetPixelReadMask(reconstruct_image,q) <= (QuantumRange/2)))
        {
          p+=(ptrdiff_t) GetPixelChannels(image);
          q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
          continue;
        }
      AccumulateMEPPPixel(&pixels,channel_similarity,
        &channel_mean_error,&channel_maximum_error);
      channel_area++;
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetMEPPSimilarity)
#endif
    {
      ssize_t
        j;

      area+=channel_area;
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
      mean_error+=channel_mean_error;
      if (channel_maximum_error > maximum_error)
        maximum_error=channel_maximum_error;
    }
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  area=MagickSafeReciprocal(area);
  for (k=0; k < (ssize_t) GetPixelChannels(image); k++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,k);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[k]*=area;
    channels++;
  }
  similarity[CompositePixelChannel]*=area;
  if (channels != 0)
    similarity[CompositePixelChannel]/=(double) channels;
  image->error.mean_error_per_pixel=(double) QuantumRange*
    similarity[CompositePixelChannel];
  image->error.normalized_mean_error=mean_error*area;
  image->error.normalized_maximum_error=maximum_error;
  return(status);
}

static inline void AccumulateMSEPixel(const ComparePixels *pixels,
  double *channel_similarity)
{
  const Image
    *image = pixels->image,
    *reconstruct_image = pixels->reconstruct_image;

  const Quantum
    *p = pixels->p,
    *q = pixels->q;

  double
    Da,
    Sa;

  ssize_t
    i;

  Sa=QuantumScale*(double) GetPixelAlpha(image,p);
  Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      error;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      error=QuantumScale*((double) p[i]-(double) GetPixelChannel(
        reconstruct_image,channel,q));
    else
      error=QuantumScale*(Sa*(double) p[i]-Da*(double) GetPixelChannel(
        reconstruct_image,channel,q));
    channel_similarity[i]+=error*error;
    channel_similarity[CompositePixelChannel]+=error*error;
  }
}

static inline MagickBooleanType GetMSESimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  const PixelMetricInfo
    info = { image, reconstruct_image, MeanSquaredErrorMetric,
      0.0 };

  double
    area;

  MagickBooleanType
    status;

  /*
    Compute the mean sequared error similarity.
  */
  status=GetPixelMetricSimilarity(&info,similarity,&area,exception);
  NormalizeChannelSimilarity(image,reconstruct_image,area,similarity);
  return(status);
}

static inline MagickBooleanType GetNCCSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  ChannelStatistics
    *image_statistics,
    *reconstruct_statistics;

  CorrelationStatisticsTargets
    targets;

  CorrelationTotals
    totals;

  double
    reconstruct_variance[MaxPixelChannels+1] = { 0.0 },
    variance[MaxPixelChannels+1] = { 0.0 };

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    k;

  /*
    Compute the normalized criss-correlation similarity.
  */
  targets.image_statistics=(&image_statistics);
  targets.reconstruct_statistics=(&reconstruct_statistics);
  if (AcquireCorrelationStatistics(image,reconstruct_image,exception,
        &targets) == MagickFalse)
    return(MagickFalse);
  (void) memset(similarity,0,(MaxPixelChannels+1)*sizeof(*similarity));
  totals.image=image;
  totals.reconstruct_image=reconstruct_image;
  totals.image_statistics=image_statistics;
  totals.reconstruct_statistics=reconstruct_statistics;
  totals.similarity=similarity;
  totals.norm=variance;
  totals.reconstruct_norm=reconstruct_variance;
  status=GetCorrelationTotals(&totals,exception);
  /*
    Compute normalized cross correlation: divide by standard deviation.
  */
  for (k=0; k < (ssize_t) GetPixelChannels(image); k++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,k);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[k]*=MagickSafeReciprocal(sqrt(variance[k])*
      sqrt(reconstruct_variance[k]));
  }
  similarity[CompositePixelChannel]*=MagickSafeReciprocal(sqrt(
    variance[CompositePixelChannel])*sqrt(
    reconstruct_variance[CompositePixelChannel]));
  /*
    Free resources.
  */
  reconstruct_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    reconstruct_statistics);
  image_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    image_statistics);
  return(status);
}

static inline void AccumulatePAPixel(const ComparePixels *pixels,
  double *channel_similarity)
{
  const Image
    *image = pixels->image,
    *reconstruct_image = pixels->reconstruct_image;

  const Quantum
    *p = pixels->p,
    *q = pixels->q;

  double
    Da,
    Sa;

  ssize_t
    i;

  Sa=QuantumScale*(double) GetPixelAlpha(image,p);
  Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      distance;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      distance=QuantumScale*fabs((double) p[i]-(double)
        GetPixelChannel(reconstruct_image,channel,q));
    else
      distance=QuantumScale*fabs(Sa*(double) p[i]-Da*(double) GetPixelChannel(
        reconstruct_image,channel,q));
    if (distance > channel_similarity[i])
      channel_similarity[i]=distance;
    if (distance > channel_similarity[CompositePixelChannel])
      channel_similarity[CompositePixelChannel]=distance;
  }
}

static inline MagickBooleanType GetPASimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  const PixelMetricInfo
    info = { image, reconstruct_image, PeakAbsoluteErrorMetric,
      0.0 };

  double
    area;

  /*
    Compute the peak absolute similarity.
  */
  (void) memset(similarity,0,(MaxPixelChannels+1)*sizeof(*similarity));
  return(GetPixelMetricSimilarity(&info,similarity,&area,exception));
}

static inline void AccumulatePDCPixel(const ComparePixels *pixels,const double fuzz,
  double *channel_similarity)
{
  const Image
    *image = pixels->image,
    *reconstruct_image = pixels->reconstruct_image;

  const Quantum
    *p = pixels->p,
    *q = pixels->q;

  double
    Da,
    Sa;

  size_t
    count = 0;

  ssize_t
    i;

  Sa=QuantumScale*(double) GetPixelAlpha(image,p);
  Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      error;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    if (channel == AlphaPixelChannel)
      error=(double) p[i]-(double) GetPixelChannel(reconstruct_image,
        channel,q);
    else
      error=Sa*(double) p[i]-Da*(double) GetPixelChannel(reconstruct_image,
        channel,q);
    if (MagickSafeSignificantError(error*error,fuzz) != MagickFalse)
      {
        channel_similarity[i]++;
        count++;
      }
  }
  if (count != 0)
    channel_similarity[CompositePixelChannel]++;
}

static magick_attribute((__always_inline__)) inline void
  AccumulateMetricPixel(const MetricType metric,const double fuzz,
  const ComparePixels *pixels,PixelMetricSums *sums)
{
  switch (metric)
  {
    case AbsoluteErrorMetric:
    {
      AccumulateAEPixel(pixels,fuzz,sums->similarity);
      break;
    }
    case FuzzErrorMetric:
    {
      AccumulateFUZZPixel(pixels,fuzz,sums->similarity,
        &sums->area);
      break;
    }
    case MeanAbsoluteErrorMetric:
    {
      AccumulateMAEPixel(pixels,sums->similarity);
      sums->area++;
      break;
    }
    case MeanSquaredErrorMetric:
    {
      AccumulateMSEPixel(pixels,sums->similarity);
      sums->area++;
      break;
    }
    case PeakAbsoluteErrorMetric:
    {
      AccumulatePAPixel(pixels,sums->similarity);
      break;
    }
    case PixelDifferenceCountErrorMetric:
    default:
    {
      AccumulatePDCPixel(pixels,fuzz,sums->similarity);
      break;
    }
  }
}

static magick_attribute((__always_inline__)) inline void
  AccumulateMetricColumns(const MetricType metric,const PixelMetricRow *row,
  PixelMetricSums *sums)
{
  const Image
    *image = row->image,
    *reconstruct_image = row->reconstruct_image;

  const Quantum
    *magick_restrict p = row->p,
    *magick_restrict q = row->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) row->columns; x++)
  {
    const ComparePixels pixels = { image, reconstruct_image, p, q };

    if ((GetPixelReadMask(image,p) <= (QuantumRange/2)) ||
        (GetPixelReadMask(reconstruct_image,q) <= (QuantumRange/2)))
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
        continue;
      }
    AccumulateMetricPixel(metric,row->fuzz,&pixels,sums);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
  }
}

static inline void AccumulateMetricRow(const MetricType metric,
  const PixelMetricRow *row,PixelMetricSums *sums)
{
  switch (metric)
  {
    case AbsoluteErrorMetric:
    {
      AccumulateMetricColumns(AbsoluteErrorMetric,row,sums);
      break;
    }
    case FuzzErrorMetric:
    {
      AccumulateMetricColumns(FuzzErrorMetric,row,sums);
      break;
    }
    case MeanAbsoluteErrorMetric:
    {
      AccumulateMetricColumns(MeanAbsoluteErrorMetric,row,sums);
      break;
    }
    case MeanSquaredErrorMetric:
    {
      AccumulateMetricColumns(MeanSquaredErrorMetric,row,sums);
      break;
    }
    case PeakAbsoluteErrorMetric:
    {
      AccumulateMetricColumns(PeakAbsoluteErrorMetric,row,sums);
      break;
    }
    case PixelDifferenceCountErrorMetric:
    default:
    {
      AccumulateMetricColumns(PixelDifferenceCountErrorMetric,row,sums);
      break;
    }
  }
}

static inline void MergeMetricValue(const MetricType metric,
  const double value,double *total)
{
  if (metric != PeakAbsoluteErrorMetric)
    {
      *total+=value;
      return;
    }
  if (value > *total)
    *total=value;
}

static inline void MergeMetricRow(const PixelMetricInfo *info,
  const PixelMetricSums *sums,double *similarity)
{
  const Image
    *image = info->image,
    *reconstruct_image = info->reconstruct_image;

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
    MergeMetricValue(info->metric,sums->similarity[j],similarity+j);
  }
  MergeMetricValue(info->metric,sums->similarity[CompositePixelChannel],
    similarity+CompositePixelChannel);
}

static inline MagickBooleanType GetPixelMetricSimilarity(const PixelMetricInfo *info,
  double *similarity,double *pixel_area,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *reconstruct_view;

  const Image
    *image = info->image,
    *reconstruct_image = info->reconstruct_image;

  double
    area = 0.0;

  MagickBooleanType
    status = MagickTrue;

  size_t
    columns,
    rows;

  ssize_t
    y;

  SetImageCompareBounds(image,reconstruct_image,&columns,&rows);
  image_view=AcquireVirtualCacheView(image,exception);
  reconstruct_view=AcquireVirtualCacheView(reconstruct_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(area,similarity,status) \
    magick_number_threads(image,image,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    PixelMetricRow
      row = { image, reconstruct_image, (const Quantum *) NULL,
        (const Quantum *) NULL, columns, info->fuzz };

    PixelMetricSums
      sums = { 0.0, { 0.0 } };

    if (status == MagickFalse)
      continue;
    row.p=GetCacheViewVirtualPixels(image_view,0,y,columns,1,exception);
    row.q=GetCacheViewVirtualPixels(reconstruct_view,0,y,columns,1,exception);
    if ((row.p == (const Quantum *) NULL) || (row.q == (const Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    AccumulateMetricRow(info->metric,&row,&sums);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetPixelMetricSimilarity)
#endif
    {
      area+=sums.area;
      MergeMetricRow(info,&sums,similarity);
    }
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  *pixel_area=area;
  return(status);
}

static inline MagickBooleanType GetPDCSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  const PixelMetricInfo
    info = { image, reconstruct_image, PixelDifferenceCountErrorMetric,
      GetFuzzyColorDistance(image,reconstruct_image) };

  double
    area;

  /*
    Compute the pixel difference count similarity.
  */
  (void) memset(similarity,0,(MaxPixelChannels+1)*sizeof(*similarity));
  return(GetPixelMetricSimilarity(&info,similarity,&area,exception));
}

static inline Image *GetEdgeCorrelationSurface(const Image *image,
  ExceptionInfo *exception)
{
  Image
    *surface;

  KernelInfo
    *kernel;

  /*
    Build a spatial-domain edge/correlation surface using a LoG convolution.
  */
  kernel=AcquireKernelInfo("LoG:0,1",exception);
  if (kernel == (KernelInfo *) NULL)
    return((Image *) NULL);
  surface=MorphologyImage(image,ConvolveMorphology,1,kernel,exception);
  kernel=DestroyKernelInfo(kernel);
  return(surface);
}

static inline MagickBooleanType IsPhaseRowMissing(const Quantum *p,
  const Quantum *q,const Quantum *pm,const Quantum *qm)
{
  return(((p == (const Quantum *) NULL) || (q == (const Quantum *) NULL) ||
    (pm == (const Quantum *) NULL) || (qm == (const Quantum *) NULL)) ?
    MagickTrue : MagickFalse);
}

typedef struct _PHASEPixel
{
  Image
    *edge_image,
    *edge_reconstruct;

  const Quantum
    *p,
    *q;
} PHASEPixel;

typedef struct _PHASEPixelTargets
{
  double
    (*channel_image_sum)[MaxPixelChannels+1],
    (*channel_image_sum_squared)[MaxPixelChannels+1],
    (*channel_reconstruct_sum)[MaxPixelChannels+1],
    (*channel_reconstruct_sum_squared)[MaxPixelChannels+1],
    (*channel_correlation)[MaxPixelChannels+1];
} PHASEPixelTargets;

static inline void AccumulatePHASEPixel(const PHASEPixel *phase,
  const PHASEPixelTargets *targets)
{
  Image
    *const edge_image = phase->edge_image,
    *const edge_reconstruct = phase->edge_reconstruct;

  const Quantum
    *const p = phase->p,
    *const q = phase->q;

  double
    (*const channel_image_sum)[MaxPixelChannels+1] =
      targets->channel_image_sum,
    (*const channel_image_sum_squared)[MaxPixelChannels+1] =
      targets->channel_image_sum_squared,
    (*const channel_reconstruct_sum)[MaxPixelChannels+1] =
      targets->channel_reconstruct_sum,
    (*const channel_reconstruct_sum_squared)[MaxPixelChannels+1] =
      targets->channel_reconstruct_sum_squared,
    (*const channel_correlation)[MaxPixelChannels+1] =
      targets->channel_correlation;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(edge_image); i++)
  {
    double
      alpha,
      beta;

    PixelChannel
      channel;

    PixelTrait
      reconstruct_traits,
      traits;

    ssize_t
      offset;

    channel=GetPixelChannelChannel(edge_image,i);
    traits=GetPixelChannelTraits(edge_image,channel);
    reconstruct_traits=GetPixelChannelTraits(edge_reconstruct,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    offset=GetPixelChannelOffset(edge_reconstruct,channel);
    if (offset < 0)
      continue;
    alpha=QuantumScale*(double) p[i];
    beta=QuantumScale*(double) q[offset];
    (*channel_image_sum)[i]+=alpha;
    (*channel_image_sum_squared)[i]+=alpha*alpha;
    (*channel_reconstruct_sum)[i]+=beta;
    (*channel_reconstruct_sum_squared)[i]+=beta*beta;
    (*channel_correlation)[i]+=alpha*beta;
  }
}

typedef struct _PHASECorrelationSums
{
  double
    area,
    *correlation,
    *image_sum,
    *image_sum_squared,
    *reconstruct_sum,
    *reconstruct_sum_squared;
} PHASECorrelationSums;

static inline double GetPHASEPearson(const PHASECorrelationSums *sums,
  const ssize_t j)
{
  const double
    area = sums->area;

  double
    *const correlation = sums->correlation,
    *const image_sum = sums->image_sum,
    *const image_sum_squared = sums->image_sum_squared,
    *const reconstruct_sum = sums->reconstruct_sum,
    *const reconstruct_sum_squared = sums->reconstruct_sum_squared;

  double
    denominator,
    image_variance,
    numerator,
    pearson,
    reconstruct_variance;

  numerator=area*correlation[j]-image_sum[j]*reconstruct_sum[j];
  image_variance=area*image_sum_squared[j]-image_sum[j]*image_sum[j];
  reconstruct_variance=area*reconstruct_sum_squared[j]-reconstruct_sum[j]*
    reconstruct_sum[j];
  if ((image_variance < MagickEpsilon) &&
      (reconstruct_variance < MagickEpsilon))
    pearson=(fabs(image_sum[j]-reconstruct_sum[j]) < MagickEpsilon) ?
      1.0 : 0.0;
  else
    {
      denominator=sqrt(image_variance)*sqrt(reconstruct_variance);
      pearson=denominator < MagickEpsilon ? 0.0 : numerator/denominator;
    }
  if (pearson < -1.0)
    pearson=(-1.0);
  if (pearson > 1.0)
    pearson=1.0;
  return(pearson);
}

typedef struct _PHASEReduction
{
  const Image
    *image,
    *reconstruct_image;

  double
    area,
    *correlation,
    *image_sum,
    *image_sum_squared,
    *reconstruct_sum,
    *reconstruct_sum_squared;
} PHASEReduction;

static inline void ReducePHASESimilarity(const PHASEReduction *reduction,
  double *similarity)
{
  const Image
    *const image = reduction->image,
    *const reconstruct_image = reduction->reconstruct_image;

  const double
    area = reduction->area;

  double
    *const correlation = reduction->correlation,
    *const image_sum = reduction->image_sum,
    *const image_sum_squared = reduction->image_sum_squared,
    *const reconstruct_sum = reduction->reconstruct_sum,
    *const reconstruct_sum_squared = reduction->reconstruct_sum_squared;

  PHASECorrelationSums
    sums;

  ssize_t
    channels = 0,
    j;

  similarity[CompositePixelChannel]=0.0;
  for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
  {
    double
      pearson;

    PixelChannel
      channel;

    PixelTrait
      reconstruct_traits,
      traits;

    channel=GetPixelChannelChannel(image,j);
    traits=GetPixelChannelTraits(image,channel);
    reconstruct_traits=GetPixelChannelTraits(reconstruct_image,channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    sums.area=area;
    sums.correlation=correlation;
    sums.image_sum=image_sum;
    sums.image_sum_squared=image_sum_squared;
    sums.reconstruct_sum=reconstruct_sum;
    sums.reconstruct_sum_squared=reconstruct_sum_squared;
    pearson=GetPHASEPearson(&sums,j);
    similarity[j]=pearson;
    similarity[CompositePixelChannel]+=pearson;
    channels++;
  }
  if (channels != 0)
    similarity[CompositePixelChannel]/=(double) channels;
}

static inline MagickBooleanType GetPHASESimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  CacheView
    *edge_reconstruct_view,
    *edge_view,
    *image_view,
    *reconstruct_view;

  double
    area = 0,
    correlation[MaxPixelChannels+1] = { 0.0 },
    image_sum[MaxPixelChannels+1] = { 0.0 },
    image_sum_squared[MaxPixelChannels+1] = { 0.0 },
    reconstruct_sum[MaxPixelChannels+1] = { 0.0 },
    reconstruct_sum_squared[MaxPixelChannels+1] = { 0.0 };

  Image
    *edge_image,
    *edge_reconstruct;

  MagickBooleanType
    status = MagickTrue;

  size_t
    columns = 0,
    rows = 0;

  ssize_t
    y;

  PHASEReduction
    reduction;

  /*
    Compute a Pearson-correlation similarity between two Laplacian-of-
    Gaussian edge surfaces.
  */
  SetImageCompareBounds(image,reconstruct_image,&columns,&rows);
  edge_image=GetEdgeCorrelationSurface(image,exception);
  edge_reconstruct=GetEdgeCorrelationSurface(reconstruct_image,exception);
  if ((edge_image == (Image *) NULL) || (edge_reconstruct == (Image *) NULL))
    {
      if (edge_image != (Image *) NULL)
        edge_image=DestroyImage(edge_image);
      if (edge_reconstruct != (Image *) NULL)
        edge_reconstruct=DestroyImage(edge_reconstruct);
      return(MagickFalse);
    }
  image_view=AcquireVirtualCacheView(image,exception);
  reconstruct_view=AcquireVirtualCacheView(reconstruct_image,exception);
  edge_view=AcquireVirtualCacheView(edge_image,exception);
  edge_reconstruct_view=AcquireVirtualCacheView(edge_reconstruct,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(edge_image,edge_reconstruct,rows,1)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    const Quantum
      *magick_restrict p,
      *magick_restrict pm,
      *magick_restrict q,
      *magick_restrict qm;

    double
      channel_area = 0,
      channel_correlation[MaxPixelChannels+1] = { 0.0 },
      channel_image_sum[MaxPixelChannels+1] = { 0.0 },
      channel_image_sum_squared[MaxPixelChannels+1] = { 0.0 },
      channel_reconstruct_sum[MaxPixelChannels+1] = { 0.0 },
      channel_reconstruct_sum_squared[MaxPixelChannels+1] = { 0.0 };

    ssize_t
      i,
      x;

    if (status == MagickFalse)
      continue;
    pm=GetCacheViewVirtualPixels(image_view,0,y,columns,1,exception);
    qm=GetCacheViewVirtualPixels(reconstruct_view,0,y,columns,1,exception);
    p=GetCacheViewVirtualPixels(edge_view,0,y,columns,1,exception);
    q=GetCacheViewVirtualPixels(edge_reconstruct_view,0,y,columns,1,exception);
    if (IsPhaseRowMissing(p,q,pm,qm) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) columns; x++)
    {
      PHASEPixel
        phase;

      PHASEPixelTargets
        targets;

      if ((GetPixelReadMask(image,pm) <= (QuantumRange/2)) ||
          (GetPixelReadMask(reconstruct_image,qm) <= (QuantumRange/2)))
        {
          p+=(ptrdiff_t) GetPixelChannels(edge_image);
          q+=(ptrdiff_t) GetPixelChannels(edge_reconstruct);
          pm+=(ptrdiff_t) GetPixelChannels(image);
          qm+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
          continue;
        }
      phase.edge_image=edge_image;
      phase.edge_reconstruct=edge_reconstruct;
      phase.p=p;
      phase.q=q;
      targets.channel_image_sum=(&channel_image_sum);
      targets.channel_image_sum_squared=(&channel_image_sum_squared);
      targets.channel_reconstruct_sum=(&channel_reconstruct_sum);
      targets.channel_reconstruct_sum_squared=
        (&channel_reconstruct_sum_squared);
      targets.channel_correlation=(&channel_correlation);
      AccumulatePHASEPixel(&phase,&targets);
      channel_area++;
      p+=(ptrdiff_t) GetPixelChannels(edge_image);
      q+=(ptrdiff_t) GetPixelChannels(edge_reconstruct);
      pm+=(ptrdiff_t) GetPixelChannels(image);
      qm+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetPHASESimilarity)
#endif
    {
      area+=channel_area;
      for (i=0; i <= (ssize_t) MaxPixelChannels; i++)
      {
        correlation[i]+=channel_correlation[i];
        image_sum[i]+=channel_image_sum[i];
        image_sum_squared[i]+=channel_image_sum_squared[i];
        reconstruct_sum[i]+=channel_reconstruct_sum[i];
        reconstruct_sum_squared[i]+=channel_reconstruct_sum_squared[i];
      }
    }
  }
  edge_reconstruct_view=DestroyCacheView(edge_reconstruct_view);
  edge_view=DestroyCacheView(edge_view);
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  edge_image=DestroyImage(edge_image);
  edge_reconstruct=DestroyImage(edge_reconstruct);
  if (status == MagickFalse)
    return(MagickFalse);
  if (area < 1.0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "InsufficientImageDataInRaster","`%s'",image->filename);
      return(MagickFalse);
    }
  /*
    Reduce to a per-channel Pearson coefficient and average.
  */
  reduction.image=image;
  reduction.reconstruct_image=reconstruct_image;
  reduction.area=area;
  reduction.correlation=correlation;
  reduction.image_sum=image_sum;
  reduction.image_sum_squared=image_sum_squared;
  reduction.reconstruct_sum=reconstruct_sum;
  reduction.reconstruct_sum_squared=reconstruct_sum_squared;
  ReducePHASESimilarity(&reduction,similarity);
  return(MagickTrue);
}

static inline double AddPHashDifferences(
  const ChannelPerceptualHash *channel_phash,
  const ChannelPerceptualHash *reconstruct_phash,const ssize_t i,
  double difference)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) channel_phash[0].number_colorspaces; j++)
  {
    double
      alpha,
      beta;

    ssize_t
      k;

    for (k=0; k < MaximumNumberOfPerceptualHashes; k++)
    {
      double
        error;

      alpha=channel_phash[i].phash[j][k];
      beta=reconstruct_phash[i].phash[j][k];
      error=beta-alpha;
      if (IsNaN(error) != 0)
        error=0.0;
      difference+=error*error;
    }
  }
  return(difference);
}

static inline void NormalizePHashSimilarity(const Image *image,
  const Image *reconstruct_image,const ChannelPerceptualHash *channel_phash,
  double *similarity)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[i]=sqrt(similarity[i]/(double)
      channel_phash[0].number_colorspaces);
  }
  similarity[CompositePixelChannel]=sqrt(similarity[CompositePixelChannel]/
    (double) channel_phash[0].number_colorspaces);
}

static inline MagickBooleanType IsChannelSkipped(const PixelTrait traits,
  const PixelTrait reconstruct_traits)
{
  return((((traits & UpdatePixelTrait) == 0) ||
    ((reconstruct_traits & UpdatePixelTrait) == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType GetPHASHSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  ChannelPerceptualHash
    *channel_phash,
    *reconstruct_phash;

  const char
    *artifact;

  ssize_t
    channels = 0,
    i;

  /*
    Compute the perceptual hash similarity.
  */
  channel_phash=GetImagePerceptualHash(image,exception);
  if (channel_phash == (ChannelPerceptualHash *) NULL)
    return(MagickFalse);
  reconstruct_phash=GetImagePerceptualHash(reconstruct_image,exception);
  if (reconstruct_phash == (ChannelPerceptualHash *) NULL)
    {
      channel_phash=(ChannelPerceptualHash *) RelinquishMagickMemory(
        channel_phash);
      return(MagickFalse);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      difference = 0.0;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (IsChannelSkipped(traits,reconstruct_traits) != MagickFalse)
      continue;
    difference=AddPHashDifferences(channel_phash,reconstruct_phash,i,
      difference);
    similarity[i]+=difference;
    similarity[CompositePixelChannel]+=difference;
    channels++;
  }
  if (channels != 0)
    similarity[CompositePixelChannel]/=(double) channels;
  artifact=GetImageArtifact(image,"phash:normalize");
  if (IsStringTrue(artifact) != MagickFalse)
    NormalizePHashSimilarity(image,reconstruct_image,channel_phash,
      similarity);
  /*
    Free resources.
  */
  reconstruct_phash=(ChannelPerceptualHash *) RelinquishMagickMemory(
    reconstruct_phash);
  channel_phash=(ChannelPerceptualHash *) RelinquishMagickMemory(channel_phash);
  return(MagickTrue);
}

static inline MagickBooleanType GetPSNRSimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  MagickBooleanType
    status = MagickTrue;

  ssize_t
    i;

  /*
    Compute the peak signal-to-noise ratio similarity.
  */
  status=GetMSESimilarity(image,reconstruct_image,similarity,exception);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[i]=10.0*MagickSafeLog10(MagickSafeReciprocal(
      similarity[i]))/MagickSafePSNRRecipicol(10.0);
  }
  similarity[CompositePixelChannel]=10.0*MagickSafeLog10(
    MagickSafeReciprocal(similarity[CompositePixelChannel]))/
    MagickSafePSNRRecipicol(10.0);
  return(status);
}

static inline MagickBooleanType GetRMSESimilarity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
#define RMSESquareRoot(x)  sqrt((x) < 0.0 ? 0.0 : (x))

  MagickBooleanType
    status = MagickTrue;

  ssize_t
    i;

  /*
    Compute the root mean-squared error similarity.
  */
  status=GetMSESimilarity(image,reconstruct_image,similarity,exception);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait reconstruct_traits = GetPixelChannelTraits(reconstruct_image,
      channel);
    if (((traits & UpdatePixelTrait) == 0) ||
        ((reconstruct_traits & UpdatePixelTrait) == 0))
      continue;
    similarity[i]=RMSESquareRoot(similarity[i]);
  }
  similarity[CompositePixelChannel]=RMSESquareRoot(
    similarity[CompositePixelChannel]);
  return(status);
}

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

static inline void AccumulateSSIMChannels(const SSIMChannelInputs *inputs,
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

static inline MagickBooleanType GetSSIMSimularity(const Image *image,
  const Image *reconstruct_image,double *similarity,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *reconstruct_view;

  char
    geometry[MagickPathExtent];

  const char
    *artifact;

  double
    area = 0.0,
    c1,
    c2,
    radius,
    sigma;

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
  kernel_info=AcquireKernelInfo(geometry,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  c1=pow(SSIMK1*SSIML,2.0);
  artifact=GetImageArtifact(image,"compare:ssim-k1");
  if (artifact != (const char *) NULL)
    c1=pow(StringToDouble(artifact,(char **) NULL)*SSIML,2.0);
  c2=pow(SSIMK2*SSIML,2.0);
  artifact=GetImageArtifact(image,"compare:ssim-k2");
  if (artifact != (const char *) NULL)
    c2=pow(StringToDouble(artifact,(char **) NULL)*SSIML,2.0);
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

      SSIMChannelInputs
        channel_inputs;

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
      channel_inputs.image=image;
      channel_inputs.reconstruct_image=reconstruct_image;
      channel_inputs.x_pixel_mu=x_pixel_mu;
      channel_inputs.x_pixel_sigma_squared=x_pixel_sigma_squared;
      channel_inputs.xy_sigma=xy_sigma;
      channel_inputs.y_pixel_mu=y_pixel_mu;
      channel_inputs.y_pixel_sigma_squared=y_pixel_sigma_squared;
      channel_inputs.c1=c1;
      channel_inputs.c2=c2;
      AccumulateSSIMChannels(&channel_inputs,channel_similarity);
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(reconstruct_image);
      channel_area++;
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetSSIMSimularity)
#endif
    {
      ssize_t
        j;

      area+=channel_area;
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

static inline MagickBooleanType GetSimilarityForMetric(Image *image,
  const Image *reconstruct_image,const MetricType metric,
  double *channel_similarity,ExceptionInfo *exception)
{
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
