/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                CCCC   OOO   M   M  PPPP    AAA   RRRR   EEEEE               %
%               C      O   O  MM MM  P   P  A   A  R   R  E                   %
%               C      O   O  M M M  PPPP   AAAAA  RRRR   EEE                 %
%               C      O   O  M   M  P      A   A  R R    E                   %
%                CCCC   OOO   M   M  P      A   A  R  R   EEEEE               %
%                                                                             %
%    SSSSS  IIIII  M   M  IIIII  L       AAA   RRRR   IIIII  TTTTT  Y   Y     %
%    SS       I    MM MM    I    L      A   A  R   R    I      T     Y Y      %
%     SSS     I    M M M    I    L      AAAAA  RRRR     I      T      Y       %
%       SS    I    M   M    I    L      A   A  R R      I      T      Y       %
%    SSSSS  IIIII  M   M  IIIII  LLLLL  A   A  R  R   IIIII    T      Y       %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                     MagickCore Image Similarity Methods                     %
%                                                                             %
%                              Software Design                                %
%                                  Cristy                                     %
%                               December 2003                                 %
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
#include "MagickCore/compare-similarity-private.h"
#include "MagickCore/compare-similarity-frequency-images-private.h"
#include "MagickCore/compare-similarity-frequency-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S i m i l a r i t y I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SimilarityImage() compares the reconstruction of the image and returns the
%  best match offset.  In addition, it returns a similarity image such that an
%  exact match location is completely white and if none of the pixels match,
%  black, otherwise some gray level in-between.
%
%  Contributed by Fred Weinhaus.
%
%  The format of the SimilarityImageImage method is:
%
%      Image *SimilarityImage(const Image *image,const Image *reconstruct,
%        const MetricType metric,const double similarity_threshold,
%        RectangleInfo *offset,double *similarity,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o reconstruct: find an area of the image that closely resembles this image.
%
%    o metric: the metric.
%
%    o similarity_threshold: minimum similarity for (sub)image match.
%
%    o offset: the best match offset of the reconstruction image within the
%      image.
%
%    o similarity: the computed similarity between the images.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static double GetSimilarityMetric(const Image *image,
  const Image *reconstruct_image,const MetricType metric,
  const ssize_t x_offset,const ssize_t y_offset,ExceptionInfo *exception)
{
  double
    *channel_similarity,
    similarity = 0.0;

  ExceptionInfo
    *sans_exception = AcquireExceptionInfo();

  Image
    *similarity_image;

  MagickBooleanType
    status = MagickTrue;

  RectangleInfo
    geometry;

  SimilarityImages
    images;

  size_t
    length = MaxPixelChannels+1UL;

  SetGeometry(reconstruct_image,&geometry);
  geometry.x=x_offset;
  geometry.y=y_offset;
  similarity_image=CropImage(image,&geometry,sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  if (similarity_image == (Image *) NULL)
    return(NAN);
  /*
    Get image distortion.
  */
  channel_similarity=(double *) AcquireQuantumMemory(length,
    sizeof(*channel_similarity));
  if (channel_similarity == (double *) NULL)
    return(NAN);
  (void) memset(channel_similarity,0,length*sizeof(*channel_similarity));
  images.image=similarity_image;
  images.reconstruct_image=reconstruct_image;
  status=GetSimilarityForMetric(&images,metric,channel_similarity,exception);
  similarity_image=DestroyImage(similarity_image);
  similarity=channel_similarity[CompositePixelChannel];
  channel_similarity=(double *) RelinquishMagickMemory(channel_similarity);
  if (status == MagickFalse)
    return(NAN);
  return(similarity);
}

typedef struct
{
  double
    similarity;

  ssize_t
    x,
    y;
} SimilarityInfo;

typedef struct _SimilarityCandidate
{
  MetricType
    metric;

  double
    similarity;

  ssize_t
    x,
    y;
} SimilarityCandidate;

static void SelectLowerSimilarity(const SimilarityCandidate *tile,
  SimilarityInfo *channel_info)
{
  const double
    similarity = tile->similarity;

  const ssize_t
    x = tile->x,
    y = tile->y;

  if (similarity >= (*channel_info).similarity)
    return;
  (*channel_info).similarity=similarity;
  (*channel_info).x=x;
  (*channel_info).y=y;
}

static void SelectBestSimilarity(const SimilarityCandidate *tile,
  SimilarityInfo *channel_info)
{
  const MetricType
    metric = tile->metric;

  const double
    similarity = tile->similarity;

  const ssize_t
    x = tile->x,
    y = tile->y;

  switch (metric)
  {
    case DotProductCorrelationErrorMetric:
    case NormalizedCrossCorrelationErrorMetric:
    case PeakSignalToNoiseRatioErrorMetric:
    case PhaseCorrelationErrorMetric:
    case StructuralSimilarityErrorMetric:
    {
      if (similarity <= (*channel_info).similarity)
        break;
      (*channel_info).similarity=similarity;
      (*channel_info).x=x;
      (*channel_info).y=y;
      break;
    }
    default:
    {
      SelectLowerSimilarity(tile,channel_info);
      break;
    }
  }
}

typedef struct _SimilarityPixel
{
  Image
    *similarity_image;

  PixelChannel
    channel;

  MetricType
    metric;

  double
    similarity;
} SimilarityPixel;

static void SetSimilarityChannel(const SimilarityPixel *pixel,
  Quantum *magick_restrict q)
{
  Image
    *const similarity_image = pixel->similarity_image;

  const PixelChannel
    channel = pixel->channel;

  const MetricType
    metric = pixel->metric;

  const double
    similarity = pixel->similarity;

  switch (metric)
  {
    case DotProductCorrelationErrorMetric:
    case NormalizedCrossCorrelationErrorMetric:
    case PeakSignalToNoiseRatioErrorMetric:
    case PhaseCorrelationErrorMetric:
    case StructuralSimilarityErrorMetric:
    {
      SetPixelChannel(similarity_image,channel,ClampToQuantum((double)
        QuantumRange*similarity),q);
      break;
    }
    default:
    {
      SetPixelChannel(similarity_image,channel,ClampToQuantum((double)
        QuantumRange*(1.0-similarity)),q);
      break;
    }
  }
}

typedef struct _SimilarityChannels
{
  const Image
    *image;

  Image
    *similarity_image;

  MetricType
    metric;

  double
    similarity;
} SimilarityChannels;

static void SetSimilarityChannels(const SimilarityChannels *channels,
  Quantum *magick_restrict q)
{
  const Image
    *const image = channels->image;

  Image
    *const similarity_image = channels->similarity_image;

  const MetricType
    metric = channels->metric;

  const double
    similarity = channels->similarity;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait similarity_traits = GetPixelChannelTraits(similarity_image,
      channel);
    SimilarityPixel
      pixel;

    if (((traits & UpdatePixelTrait) == 0) ||
        ((similarity_traits & UpdatePixelTrait) == 0))
      continue;
    pixel.similarity_image=similarity_image;
    pixel.channel=channel;
    pixel.metric=metric;
    pixel.similarity=similarity;
    SetSimilarityChannel(&pixel,q);
  }
}

typedef struct _SimilarityRow
{
  const Image
    *image,
    *reconstruct;

  Image
    *similarity_image;

  CacheView
    *similarity_view;

  MetricType
    metric;

  MagickBooleanType
    threshold_trigger;

  ExceptionInfo
    *exception;
} SimilarityRow;

static MagickBooleanType ScanSimilarityRow(const SimilarityRow *row,
  const ssize_t y,SimilarityInfo *channel_info,MagickBooleanType *status)
{
  const Image
    *const image = row->image,
    *const reconstruct = row->reconstruct;

  Image
    *const similarity_image = row->similarity_image;

  CacheView
    *const similarity_view = row->similarity_view;

  const MetricType
    metric = row->metric;

  const MagickBooleanType
    threshold_trigger = row->threshold_trigger;

  ExceptionInfo
    *const exception = row->exception;

  double
    similarity;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  if (threshold_trigger != MagickFalse)
    return(MagickFalse);
  q=QueueCacheViewAuthenticPixels(similarity_view,0,y,
    similarity_image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) similarity_image->columns; x++)
  {
    SimilarityCandidate
      tile;

    SimilarityChannels
      channels;

    similarity=GetSimilarityMetric((Image *) image,reconstruct,metric,x,y,
      exception);
    tile.metric=metric;
    tile.similarity=similarity;
    tile.x=x;
    tile.y=y;
    SelectBestSimilarity(&tile,channel_info);
    channels.image=image;
    channels.similarity_image=similarity_image;
    channels.metric=metric;
    channels.similarity=similarity;
    SetSimilarityChannels(&channels,q);
    q+=(ptrdiff_t) GetPixelChannels(similarity_image);
  }
  return(MagickTrue);
}

typedef struct _SimilarityMerge
{
  MetricType
    metric;

  double
    similarity_threshold;

  SimilarityInfo
    channel_info;
} SimilarityMerge;

typedef enum
{
  HigherSimilaritySide,
  LowerSimilaritySide
} SimilaritySide;

typedef struct _SimilaritySideMerge
{
  SimilaritySide
    side;

  double
    similarity_threshold;

  SimilarityInfo
    channel_info;
} SimilaritySideMerge;

static inline int IsSideSimilarityMet(const SimilaritySideMerge *name,
  const double limit)
{
  const SimilaritySide
    side = name->side;

  const SimilarityInfo
    channel_info = name->channel_info;

  return((side == HigherSimilaritySide) ?
    (channel_info.similarity >= limit) :
    (channel_info.similarity < limit));
}

static inline void MergeSideSimilarityInfo(const SimilaritySideMerge *name,
  SimilarityInfo *similarity_info,MagickBooleanType *threshold_trigger)
{
  const double
    similarity_threshold = name->similarity_threshold;

  const SimilarityInfo
    channel_info = name->channel_info;

  if ((similarity_threshold != DefaultSimilarityThreshold) &&
      IsSideSimilarityMet(name,similarity_threshold))
    (*threshold_trigger)=MagickTrue;
  if (IsSideSimilarityMet(name,(*similarity_info).similarity))
    (*similarity_info)=channel_info;
}

static void MergeSimilarityInfo(const SimilarityMerge *merge,
  SimilarityInfo *similarity_info,MagickBooleanType *threshold_trigger)
{
  const MetricType
    metric = merge->metric;

  const double
    similarity_threshold = merge->similarity_threshold;

  const SimilarityInfo
    channel_info = merge->channel_info;

  switch (metric)
  {
    case DotProductCorrelationErrorMetric:
    case NormalizedCrossCorrelationErrorMetric:
    case PeakSignalToNoiseRatioErrorMetric:
    case PhaseCorrelationErrorMetric:
    case StructuralSimilarityErrorMetric:
    {
      const SimilaritySideMerge
        name = { HigherSimilaritySide, similarity_threshold, channel_info };

      MergeSideSimilarityInfo(&name,similarity_info,threshold_trigger);
      break;
    }
    default:
    {
      const SimilaritySideMerge
        name = { LowerSimilaritySide, similarity_threshold, channel_info };

      MergeSideSimilarityInfo(&name,similarity_info,threshold_trigger);
      break;
    }
  }
}

static Image *AcquireSimilarityImage(const Image *image,
  const Image *reconstruct,ExceptionInfo *exception)
{
  Image
    *similarity_image;

  size_t
    columns,
    rows;

  if ((image->columns < reconstruct->columns) ||
      (image->rows < reconstruct->rows))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "GeometryDoesNotContainImage","`%s'",image->filename);
      return((Image *) NULL);
    }
  SetImageCompareBounds(image,reconstruct,&columns,&rows);
  similarity_image=CloneImage(image,columns,rows,MagickTrue,exception);
  return(similarity_image);
}

static void SetSimilarityProperties(const Image *image,
  const double *similarity_metric,const RectangleInfo *offset)
{
  (void) FormatImageProperty((Image *) image,"similarity","%.*g",
    GetMagickPrecision(),*similarity_metric);
  (void) FormatImageProperty((Image *) image,"similarity.offset.x","%.*g",
    GetMagickPrecision(),(double) offset->x);
  (void) FormatImageProperty((Image *) image,"similarity.offset.y","%.*g",
    GetMagickPrecision(),(double) offset->y);
}

#if defined(MAGICKCORE_HDRI_SUPPORT) && defined(MAGICKCORE_FFTW_DELEGATE)
static const FrequencySimilarity *GetRequestedFrequencySimilarity(
  const Image *image,const MetricType metric)
{
  const char *artifact = GetImageArtifact(image,"compare:frequency-domain");
  if (artifact == (const char *) NULL)
    artifact=GetImageArtifact(image,"compare:accelerate-ncc");
  if (IsFrequencyDomainSimilarity(image,artifact) != MagickFalse)
    return(GetFrequencySimilarity(metric));
  return((const FrequencySimilarity *) NULL);
}
#endif

MagickExport Image *SimilarityImage(const Image *image,const Image *reconstruct,
  const MetricType metric,const double similarity_threshold,
  RectangleInfo *offset,double *similarity_metric,ExceptionInfo *exception)
{
#define SimilarityImageTag  "Similarity/Image"

  CacheView
    *similarity_view;

  Image
    *similarity_image = (Image *) NULL;

  MagickBooleanType
    status = MagickTrue;

  MagickOffsetType
    progress = 0;

  SimilarityInfo
    similarity_info = { 0.0, 0, 0 };

  ssize_t
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  assert(offset != (RectangleInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  SetGeometry(reconstruct,offset);
  *similarity_metric=0.0;
  offset->x=0;
  offset->y=0;
#if defined(MAGICKCORE_HDRI_SUPPORT) && defined(MAGICKCORE_FFTW_DELEGATE)
{
  const FrequencySimilarity
    *frequency = GetRequestedFrequencySimilarity(image,metric);

  if (frequency != (const FrequencySimilarity *) NULL)
    {
      similarity_image=frequency->similarity_image(image,reconstruct,
        offset,similarity_metric,exception);
      return(similarity_image);
    }
}
#endif
  similarity_image=AcquireSimilarityImage(image,reconstruct,exception);
  if (similarity_image == (Image *) NULL)
    return((Image *) NULL);
  similarity_image->depth=32;
  similarity_image->colorspace=GRAYColorspace;
  similarity_image->alpha_trait=UndefinedPixelTrait;
  status=SetImageStorageClass(similarity_image,DirectClass,exception);
  if (status == MagickFalse)
    return(DestroyImage(similarity_image));
  /*
    Measure similarity of reconstruction image against image.
  */
  similarity_info.similarity=GetSimilarityMetric(image,reconstruct,metric,
    similarity_info.x,similarity_info.y,exception);
  similarity_view=AcquireAuthenticCacheView(similarity_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(similarity_info,status) \
    magick_number_threads(image,reconstruct,similarity_image->rows,1)
#endif
  for (y=0; y < (ssize_t) similarity_image->rows; y++)
  {
    MagickBooleanType
      threshold_trigger = MagickFalse;

    SimilarityInfo
      channel_info = similarity_info;

    const SimilarityRow row = { image, reconstruct, similarity_image,
      similarity_view, metric, threshold_trigger, exception };

    SimilarityMerge
      merge;

    if (ScanSimilarityRow(&row,y,&channel_info,&status) == MagickFalse)
      continue;
    merge.metric=metric;
    merge.similarity_threshold=similarity_threshold;
    merge.channel_info=channel_info;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetSimilarityMetric)
#endif
    MergeSimilarityInfo(&merge,&similarity_info,&threshold_trigger);
    if (SyncCacheViewAuthenticPixels(similarity_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

        progress++;
        proceed=SetImageProgress(image,SimilarityImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  similarity_view=DestroyCacheView(similarity_view);
  if (status == MagickFalse)
    similarity_image=DestroyImage(similarity_image);
  *similarity_metric=similarity_info.similarity;
  if (fabs(*similarity_metric) < MagickEpsilon)
    *similarity_metric=0.0;
  offset->x=similarity_info.x;
  offset->y=similarity_info.y;
  SetSimilarityProperties(image,similarity_metric,offset);
  return(similarity_image);
}
