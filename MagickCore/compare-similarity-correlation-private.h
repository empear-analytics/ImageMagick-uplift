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

  MagickCore private methods of compare-similarity-private.h: the shared typedefs and the AE, correlation and DPC similarity measures, from ComparePixels to GetDPCSimilarity; included by that header only.
*/
#ifndef MAGICKCORE_COMPARE_SIMILARITY_CORRELATION_PRIVATE_H
#define MAGICKCORE_COMPARE_SIMILARITY_CORRELATION_PRIVATE_H

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

static void MergeCorrelationTotals(const Image *image,
  const Image *reconstruct_image,const CorrelationSums *sums,
  const CorrelationSums *merged)
{
  double
    *const channel_similarity = sums->channel_similarity,
    *const channel_norm = sums->channel_norm,
    *const channel_reconstruct_norm = sums->channel_reconstruct_norm,
    *const similarity = merged->channel_similarity,
    *const norm = merged->channel_norm,
    *const reconstruct_norm = merged->channel_reconstruct_norm;

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

  const CorrelationSums
    merged = { similarity, norm, reconstruct_norm };

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
      MergeCorrelationTotals(image,reconstruct_image,&sums,&merged);
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
          status=MagickFalse;
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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
