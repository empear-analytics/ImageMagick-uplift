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

static void ConvertSimilarityToDistortionValue(const MetricType metric,
  double *distortion)
{
  switch (metric)
  {
    case DotProductCorrelationErrorMetric:
    case NormalizedCrossCorrelationErrorMetric:
    case PhaseCorrelationErrorMetric:
    case StructuralSimilarityErrorMetric:
    {
      *distortion=(1.0-(*distortion))/2.0;
      break;
    }
    default: break;
  }
}

MagickExport MagickBooleanType GetImageDistortion(Image *image,
  const Image *reconstruct_image,const MetricType metric,double *distortion,
  ExceptionInfo *exception)
{
#define CompareMetricNotSupportedException  "metric not supported"

  double
    *channel_similarity;

  const SimilarityImages
    images = { image, reconstruct_image };

  MagickBooleanType
    status = MagickTrue;

  size_t
    length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickCoreSignature);
  assert(distortion != (double *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Get image distortion.
  */
  *distortion=0.0;
  length=MaxPixelChannels+1UL;
  channel_similarity=(double *) AcquireQuantumMemory(length,
    sizeof(*channel_similarity));
  if (channel_similarity == (double *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) memset(channel_similarity,0,length*sizeof(*channel_similarity));
  status=GetSimilarityForMetric(&images,metric,channel_similarity,exception);
  *distortion=channel_similarity[CompositePixelChannel];
  ConvertSimilarityToDistortionValue(metric,distortion);
  channel_similarity=(double *) RelinquishMagickMemory(channel_similarity);
  if (fabs(*distortion) < MagickEpsilon)
    *distortion=0.0;
  (void) FormatImageProperty(image,"distortion","%.*g",GetMagickPrecision(),
    *distortion);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e D i s t o r t i o n s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageDistortions() compares the pixel channels of an image to a
%  reconstructed image and returns the specified metric for each channel.
%
%  The format of the GetImageDistortions method is:
%
%      double *GetImageDistortions(const Image *image,
%        const Image *reconstruct_image,const MetricType metric,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruction image.
%
%    o metric: the metric.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ConvertSimilarityToDistortion(const MetricType metric,
  double *distortion)
{
  ssize_t
    i;

  switch (metric)
  {
    case DotProductCorrelationErrorMetric:
    case NormalizedCrossCorrelationErrorMetric:
    case PhaseCorrelationErrorMetric:
    case StructuralSimilarityErrorMetric:
    {
      for (i=0; i <= MaxPixelChannels; i++)
        distortion[i]=(1.0-distortion[i])/2.0;
      break;
    }
    default: break;
  }
}

MagickExport double *GetImageDistortions(Image *image,
  const Image *reconstruct_image,const MetricType metric,
  ExceptionInfo *exception)
{
  double
    *distortion,
    *channel_similarity;

  const SimilarityImages
    images = { image, reconstruct_image };

  MagickBooleanType
    status = MagickTrue;

  size_t
    length;

  ssize_t
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Get image distortion.
  */
  length=MaxPixelChannels+1UL;
  channel_similarity=(double *) AcquireQuantumMemory(length,
    sizeof(*channel_similarity));
  if (channel_similarity == (double *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) memset(channel_similarity,0,length*sizeof(*channel_similarity));
  status=GetSimilarityForMetric(&images,metric,channel_similarity,exception);
  if (status == MagickFalse)
    {
      channel_similarity=(double *) RelinquishMagickMemory(channel_similarity);
      return((double *) NULL);
    }
  distortion=channel_similarity;
  ConvertSimilarityToDistortion(metric,distortion);
  for (i=0; i <= MaxPixelChannels; i++)
    if (fabs(distortion[i]) < MagickEpsilon)
      distortion[i]=0.0;
  (void) FormatImageProperty(image,"distortion","%.*g",GetMagickPrecision(),
    distortion[CompositePixelChannel]);
  return(distortion);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S e t I m a g e C o l o r M e t r i c                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageColorMetric() measures the difference between colors at each pixel
%  location of two images.  A value other than 0 means the colors match
%  exactly.  Otherwise an error measure is computed by summing over all
%  pixels in an image the distance squared in RGB space between each image
%  pixel and its corresponding pixel in the reconstruction image.  The error
%  measure is assigned to these image members:
%
%    o mean_error_per_pixel:  The mean error for any single pixel in
%      the image.
%
%    o normalized_mean_error:  The normalized mean quantization error for
%      any single pixel in the image.  This distance measure is normalized to
%      a range between 0 and 1.  It is independent of the range of red, green,
%      and blue values in the image.
%
%    o normalized_maximum_error:  The normalized maximum quantization
%      error for any single pixel in the image.  This distance measure is
%      normalized to a range between 0 and 1.  It is independent of the range
%      of red, green, and blue values in your image.
%
%  A small normalized mean square error, accessed as
%  image->normalized_mean_error, suggests the images are very similar in
%  spatial layout and color.
%
%  The format of the SetImageColorMetric method is:
%
%      MagickBooleanType SetImageColorMetric(Image *image,
%        const Image *reconstruct_image,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruction image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageColorMetric(Image *image,
  const Image *reconstruct_image,ExceptionInfo *exception)
{
  double
    channel_similarity[MaxPixelChannels+1] = { 0.0 };

  MagickBooleanType
    status;

  status=GetMEPPSimilarity(image,reconstruct_image,channel_similarity,
    exception);
  if (status == MagickFalse)
    return(MagickFalse);
  status=fabs(image->error.mean_error_per_pixel) < MagickEpsilon ?
    MagickTrue : MagickFalse;
  return(status);
}

