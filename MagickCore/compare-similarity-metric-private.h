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

  MagickCore private methods of compare-similarity-private.h: the FUZZ, MAE, MEPP, MSE, NCC, PA and PDC similarity measures and the pixel metric row loop, from AccumulateFUZZPixel to GetPDCSimilarity; included by that header only.
*/
#ifndef MAGICKCORE_COMPARE_SIMILARITY_METRIC_PRIVATE_H
#define MAGICKCORE_COMPARE_SIMILARITY_METRIC_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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

typedef struct _MEPPErrors
{
  double
    area,
    mean_error,
    maximum_error;
} MEPPErrors;

static inline void NormalizeMEPPSimilarity(Image *image,
  const Image *reconstruct_image,const MEPPErrors *errors,double *similarity)
{
  double
    area = errors->area;

  const double
    mean_error = errors->mean_error,
    maximum_error = errors->maximum_error;

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
  image->error.mean_error_per_pixel=(double) QuantumRange*
    similarity[CompositePixelChannel];
  image->error.normalized_mean_error=mean_error*area;
  image->error.normalized_maximum_error=maximum_error;
}

typedef struct _MEPPRow
{
  Image
    *image;

  const Image
    *reconstruct_image;

  size_t
    columns;
} MEPPRow;

typedef struct _MEPPRowTargets
{
  const Quantum
    *magick_restrict *p,
    *magick_restrict *q;

  double
    (*channel_similarity)[MaxPixelChannels+1],
    *channel_mean_error,
    *channel_maximum_error,
    *channel_area;
} MEPPRowTargets;

static magick_attribute((__always_inline__)) inline void
  AccumulateMEPPRow(const MEPPRow *row,const MEPPRowTargets *targets)
{
  const Quantum
    *magick_restrict *const p = targets->p,
    *magick_restrict *const q = targets->q;

  double
    (*const channel_similarity)[MaxPixelChannels+1] =
      targets->channel_similarity,
    *const channel_mean_error = targets->channel_mean_error,
    *const channel_maximum_error = targets->channel_maximum_error,
    *const channel_area = targets->channel_area;

  ssize_t
    x;

  for (x=0; x < (ssize_t) row->columns; x++)
  {
    const ComparePixels pixels = { row->image, row->reconstruct_image, (*p),
      (*q) };

    if ((GetPixelReadMask(row->image,(*p)) <= (QuantumRange/2)) ||
        (GetPixelReadMask(row->reconstruct_image,(*q)) <= (QuantumRange/2)))
      {
        (*p)+=(ptrdiff_t) GetPixelChannels(row->image);
        (*q)+=(ptrdiff_t) GetPixelChannels(row->reconstruct_image);
        continue;
      }
    AccumulateMEPPPixel(&pixels,(*channel_similarity),
      channel_mean_error,channel_maximum_error);
    (*channel_area)++;
    (*p)+=(ptrdiff_t) GetPixelChannels(row->image);
    (*q)+=(ptrdiff_t) GetPixelChannels(row->reconstruct_image);
  }
}

typedef struct _MEPPMerge
{
  Image
    *image;

  const Image
    *reconstruct_image;

  const double
    *channel_similarity;

  double
    channel_mean_error,
    channel_maximum_error;
} MEPPMerge;

static void MergeMEPPRow(const MEPPMerge *merge,double *similarity,
  double *mean_error,double *maximum_error)
{
  Image
    *const image = merge->image;

  const Image
    *const reconstruct_image = merge->reconstruct_image;

  const double
    *const channel_similarity = merge->channel_similarity,
    channel_mean_error = merge->channel_mean_error,
    channel_maximum_error = merge->channel_maximum_error;

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
  (*mean_error)+=channel_mean_error;
  if (channel_maximum_error > (*maximum_error))
    (*maximum_error)=channel_maximum_error;
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
    y;

  MEPPErrors
    errors;

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

    MEPPRow
      row;

    MEPPRowTargets
      targets;

    MEPPMerge
      merge;

    if (status == MagickFalse)
      continue;
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
    targets.p=(&p);
    targets.q=(&q);
    targets.channel_similarity=(&channel_similarity);
    targets.channel_mean_error=(&channel_mean_error);
    targets.channel_maximum_error=(&channel_maximum_error);
    targets.channel_area=(&channel_area);
    AccumulateMEPPRow(&row,&targets);
    merge.image=image;
    merge.reconstruct_image=reconstruct_image;
    merge.channel_similarity=channel_similarity;
    merge.channel_mean_error=channel_mean_error;
    merge.channel_maximum_error=channel_maximum_error;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetMEPPSimilarity)
#endif
    {
      area+=channel_area;
      MergeMEPPRow(&merge,similarity,&mean_error,&maximum_error);
    }
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  errors.area=area;
  errors.mean_error=mean_error;
  errors.maximum_error=maximum_error;
  NormalizeMEPPSimilarity(image,reconstruct_image,&errors,similarity);
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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
