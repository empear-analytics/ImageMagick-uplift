/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        SSSSS  TTTTT   AAA   TTTTT  IIIII  SSSSS  TTTTT  IIIII   CCCC        %
%        SS       T    A   A    T      I    SS       T      I    C            %
%         SSS     T    AAAAA    T      I     SSS     T      I    C            %
%           SS    T    A   A    T      I       SS    T      I    C            %
%        SSSSS    T    A   A    T    IIIII  SSSSS    T    IIIII   CCCC        %
%                                                                             %
%               M   M  EEEEE   AAA   SSSSS  U   U  RRRR   EEEEE               %
%               MM MM  E      A   A  SS     U   U  R   R  E                   %
%               M M M  EEE    AAAAA   SSS   U   U  RRRR   EEE                 %
%               M   M  E      A   A     SS  U   U  R R    E                   %
%               M   M  EEEEE  A   A  SSSSS   UUU   R  R   EEEEE               %
%                                                                             %
%                                                                             %
%               MagickCore Image Statistic Measurement Methods                %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/display.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/utility.h"
#include "MagickCore/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e E n t r o p y                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageEntropy() returns the entropy of one or more image channels.
%
%  The format of the GetImageEntropy method is:
%
%      MagickBooleanType GetImageEntropy(const Image *image,double *entropy,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o entropy: the average entropy of the selected channels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageEntropy(const Image *image,
  double *entropy,ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  channel_statistics=GetImageStatistics(image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    {
      *entropy=NAN;
      return(MagickFalse);
    }
  *entropy=channel_statistics[CompositePixelChannel].entropy;
  channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    channel_statistics);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e E x t r e m a                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageExtrema() returns the extrema of one or more image channels.
%
%  The format of the GetImageExtrema method is:
%
%      MagickBooleanType GetImageExtrema(const Image *image,size_t *minima,
%        size_t *maxima,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o minima: the minimum value in the channel.
%
%    o maxima: the maximum value in the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageExtrema(const Image *image,
  size_t *minima,size_t *maxima,ExceptionInfo *exception)
{
  double
    max,
    min;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=GetImageRange(image,&min,&max,exception);
  *minima=(size_t) ceil(min-0.5);
  *maxima=(size_t) floor(max+0.5);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e K u r t o s i s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageKurtosis() returns the kurtosis and skewness of one or more image
%  channels.
%
%  The format of the GetImageKurtosis method is:
%
%      MagickBooleanType GetImageKurtosis(const Image *image,double *kurtosis,
%        double *skewness,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o kurtosis: the kurtosis of the channel.
%
%    o skewness: the skewness of the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageKurtosis(const Image *image,
  double *kurtosis,double *skewness,ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  channel_statistics=GetImageStatistics(image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    {
      *kurtosis=NAN;
      *skewness=NAN;
      return(MagickFalse);
    }
  *kurtosis=channel_statistics[CompositePixelChannel].kurtosis;
  *skewness=channel_statistics[CompositePixelChannel].skewness;
  channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    channel_statistics);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e M e a n                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageMean() returns the mean and standard deviation of one or more image
%  channels.
%
%  The format of the GetImageMean method is:
%
%      MagickBooleanType GetImageMean(const Image *image,double *mean,
%        double *standard_deviation,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o mean: the average value in the channel.
%
%    o standard_deviation: the standard deviation of the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetImageMean(const Image *image,double *mean,
  double *standard_deviation,ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  channel_statistics=GetImageStatistics(image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    {
      *mean=NAN;
      *standard_deviation=NAN;
      return(MagickFalse);
    }
  *mean=channel_statistics[CompositePixelChannel].mean;
  *standard_deviation=
    channel_statistics[CompositePixelChannel].standard_deviation;
  channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    channel_statistics);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e M e d i a n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageMedian() returns the median pixel of one or more image channels.
%
%  The format of the GetImageMedian method is:
%
%      MagickBooleanType GetImageMedian(const Image *image,double *median,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o median: the average value in the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType CompositeImageMedian(const Image *image,double *median,
  ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  channel_statistics=GetImageStatistics(image,exception);
  if (channel_statistics == (ChannelStatistics *) NULL)
    {
      *median=NAN;
      return(MagickFalse);
    }
  *median=channel_statistics[CompositePixelChannel].median;
  channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
    channel_statistics);
  return(MagickTrue);
}

MagickExport MagickBooleanType GetImageMedian(const Image *image,double *median,
  ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(CompositeImageMedian(image,median,exception));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l P e r c e p t u a l H a s h                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImagePerceptualHash() returns the perceptual hash of one or more
%  image channels.
%
%  The format of the GetImagePerceptualHash method is:
%
%      ChannelPerceptualHash *GetImagePerceptualHash(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetPerceptualHashes(ChannelPerceptualHash *perceptual_hash,
  const ChannelMoments *moments,const ssize_t i)
{
  size_t
    j;

  ssize_t
    channel;

  for (channel=0; channel <= MaxPixelChannels; channel++)
    for (j=0; j < MaximumNumberOfPerceptualHashes; j++)
      perceptual_hash[channel].phash[i][j]=(-MagickSafeLog10(fabs(
        moments[channel].invariant[j])));
}

static char *AcquirePerceptualColorspaces(const char *artifact)
{
  char
    *colorspaces;

  if (artifact != (const char *) NULL)
    colorspaces=AcquireString(artifact);
  else
    colorspaces=AcquireString("xyY,HSB");
  return(colorspaces);
}

MagickExport ChannelPerceptualHash *GetImagePerceptualHash(const Image *image,
  ExceptionInfo *exception)
{
  ChannelPerceptualHash
    *perceptual_hash;

  char
    *colorspaces,
    *p,
    *q;

  const char
    *artifact;

  MagickBooleanType
    status;

  ssize_t
    i;

  perceptual_hash=(ChannelPerceptualHash *) AcquireQuantumMemory(
    MaxPixelChannels+1UL,sizeof(*perceptual_hash));
  if (perceptual_hash == (ChannelPerceptualHash *) NULL)
    return((ChannelPerceptualHash *) NULL);
  (void) memset(perceptual_hash,0,(MaxPixelChannels+1UL)*
    sizeof(*perceptual_hash));
  artifact=GetImageArtifact(image,"phash:colorspaces");
  colorspaces=AcquirePerceptualColorspaces(artifact);
  perceptual_hash[0].number_colorspaces=0;
  perceptual_hash[0].number_channels=0;
  q=colorspaces;
  for (i=0; (p=StringToken(",",&q)) != (char *) NULL; i++)
  {
    ChannelMoments
      *moments;

    Image
      *hash_image;

    ssize_t
      colorspace;

    if (i >= MaximumNumberOfPerceptualColorspaces)
      break;
    colorspace=ParseCommandOption(MagickColorspaceOptions,MagickFalse,p);
    if (colorspace < 0)
      break;
    perceptual_hash[0].colorspace[i]=(ColorspaceType) colorspace;
    hash_image=BlurImage(image,0.0,1.0,exception);
    if (hash_image == (Image *) NULL)
      break;
    status=TransformImageColorspace(hash_image,(ColorspaceType) colorspace,
      exception);
    if (status == MagickFalse)
      {
        hash_image=DestroyImage(hash_image);
        break;
      }
    moments=GetImageMoments(hash_image,exception);
    if (moments == (ChannelMoments *) NULL)
      {
        hash_image=DestroyImage(hash_image);
        break;
      }
    perceptual_hash[0].number_colorspaces++;
    perceptual_hash[0].number_channels+=GetImageChannels(hash_image);
    SetPerceptualHashes(perceptual_hash,moments,i);
    hash_image=DestroyImage(hash_image);
    moments=(ChannelMoments *) RelinquishMagickMemory(moments);
  }
  colorspaces=DestroyString(colorspaces);
  return(perceptual_hash);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e R a n g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageRange() returns the range of one or more image channels.
%
%  The format of the GetImageRange method is:
%
%      MagickBooleanType GetImageRange(const Image *image,double *minima,
%        double *maxima,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o minima: the minimum value in the channel.
%
%    o maxima: the maximum value in the channel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void AccumulateChannelRange(const Image *image,
  const Quantum *magick_restrict p,double *channel_maxima,
  double *channel_minima)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if ((double) p[i] > (*channel_maxima))
      (*channel_maxima)=(double) p[i];
    if ((double) p[i] < (*channel_minima))
      (*channel_minima)=(double) p[i];
  }
}

MagickExport MagickBooleanType GetImageRange(const Image *image,double *minima,
  double *maxima,ExceptionInfo *exception)
{
  typedef struct
  {
    double
      maxima,
      minima;
  } RangeInfo;

  CacheView
    *image_view;

  MagickBooleanType
    status;

  RangeInfo
    range_info = { -MagickMaximumValue, MagickMaximumValue };

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(range_info,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    RangeInfo
      channel_range;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    channel_range=range_info;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      AccumulateChannelRange(image,p,&channel_range.maxima,
        &channel_range.minima);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_GetImageRange)
#endif
    {
      if (channel_range.maxima > range_info.maxima)
        range_info.maxima=channel_range.maxima;
      if (channel_range.minima < range_info.minima)
        range_info.minima=channel_range.minima;
    }
  }
  image_view=DestroyCacheView(image_view);
  *maxima=range_info.maxima;
  *minima=range_info.minima;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e S t a t i s t i c s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageStatistics() returns statistics for each channel in the image.  The
%  statistics include the channel depth, its minima, maxima, mean, standard
%  deviation, kurtosis and skewness.  You can access the red channel mean, for
%  example, like this:
%
%      channel_statistics=GetImageStatistics(image,exception);
%      red_mean=channel_statistics[RedPixelChannel].mean;
%
%  Use MagickRelinquishMemory() to free the statistics buffer.
%
%  The format of the GetImageStatistics method is:
%
%      ChannelStatistics *GetImageStatistics(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#define SwapPixels(alpha,beta) \
{ \
  Quantum gamma=(alpha); \
  (alpha)=(beta);(beta)=gamma; \
}

static void SetMedianPivot(Quantum *pixels,const ssize_t low,
  const ssize_t mid,const ssize_t high)
{
  if (pixels[mid] > pixels[high])
    SwapPixels(pixels[mid],pixels[high]);
  if (pixels[low] > pixels[high])
    SwapPixels(pixels[low], pixels[high]);
  if (pixels[mid] > pixels[low])
    SwapPixels(pixels[mid],pixels[low]);
  SwapPixels(pixels[mid],pixels[low+1]);
}

static ssize_t SortMedianPair(Quantum *pixels,const ssize_t low,
  const ssize_t high,const ssize_t median)
{
  if (pixels[low] > pixels[high])
    SwapPixels(pixels[low],pixels[high]);
  return(median);
}

static void PartitionMedianPixels(Quantum *pixels,const ssize_t low,
  ssize_t *l,ssize_t *h)
{
  for ( ; ; )
  {
    do (*l)++; while (pixels[low] > pixels[*l]);
    do (*h)--; while (pixels[*h] > pixels[low]);
    if (*h < *l)
      break;
    SwapPixels(pixels[*l],pixels[*h]);
  }
}

static ssize_t GetMedianPixel(Quantum *pixels,const size_t n)
{
  ssize_t
    low = 0,
    high = (ssize_t) n-1,
    median = (low+high)/2;

  for ( ; ; )
  {
    ssize_t
      l = low+1,
      h = high,
      mid = (low+high)/2;

    if (high <= low)
      return(median);
    if (high == (low+1))
      return(SortMedianPair(pixels,low,high,median));
    SetMedianPivot(pixels,low,mid,high);
    PartitionMedianPixels(pixels,low,&l,&h);
    SwapPixels(pixels[low],pixels[h]);
    if (h <= median)
      low=l;
    if (h >= median)
      high=h-1;
  }
}

static inline long double MagickSafeReciprocalLD(const long double x)
{
  long double
    sign;

  /*
    Return 1/x where x is perceptible (not unlimited or infinitesimal).
  */
  sign=x < 0.0 ? -1.0 : 1.0;
  if ((sign*x) >= MagickEpsilon)
    return(1.0/x);
  return(sign/MagickEpsilon);
}

static void UpdateWelfordMoments(ChannelStatistics *cs,
  const Quantum *magick_restrict p,const ssize_t i)
{
  /* Calculate running totals for Welford's method.
  */
  double
    n = cs->area,
    n1 = cs->area-1;

  long double
    delta,
    delta_n,
    delta_n2,
    term1;

  delta=(double) p[i]-cs->M1;
  delta_n=delta/n;
  delta_n2=delta_n*delta_n;
  term1=delta*delta_n*n1;
  cs->M4+=term1*delta_n2*(n*n-3.0*n+3.0)+6.0*delta_n2*cs->M2-4.0*
    delta_n*cs->M3;
  cs->M3+=term1*delta_n*(n-2.0)-3.0*delta_n*cs->M2;
  cs->M2+=term1;
  cs->M1+=delta_n;
}

static void ComputeChannelHigherMoments(ChannelStatistics *cs,
  const double adj_area)
{
  if (cs->area > 1.0)
    cs->standard_deviation=(double) sqrtl(cs->M2/((long double)
      cs->area-1.0));
  else
    cs->standard_deviation=(double) sqrtl(cs->M2/((long double)
      cs->area));
  cs->variance=cs->standard_deviation*cs->standard_deviation;
  cs->skewness=(double) (sqrtl(cs->area)*cs->M3/powl(cs->M2*adj_area,
    1.5));
  cs->kurtosis=(double) (cs->area*cs->M4/(cs->M2*cs->M2*adj_area*
    adj_area)-3.0);
}

static void ComputeChannelMoments(ChannelStatistics *cs)
{
  double
    adj_area = 1.0;

  cs->mean=0.0;
  if (cs->area > 0)
    {
      cs->mean=(double) (cs->sumLD/(long double) cs->area);
      if (cs->area > 1.0)
        adj_area=cs->area/(cs->area-1.0);
    }
  cs->sum=(double) cs->sum;
  if (cs->M2 == 0.0)
    {
      cs->standard_deviation=0.0;
      cs->variance=0.0;
      cs->skewness=0.0;
      cs->kurtosis=0.0;
    }
  else
    ComputeChannelHigherMoments(cs,adj_area);
}

static void ResetChannelStatistics(ChannelStatistics *channel_statistics)
{
  ssize_t
    i;

  for (i=0; i <= (ssize_t) MaxPixelChannels; i++)
  {
    ChannelStatistics *cs = channel_statistics+i;
    cs->area=0.0;
    cs->depth=1;
    cs->maxima=(-MagickMaximumValue);
    cs->minima=MagickMaximumValue;
    cs->sum=0.0;
    cs->sumLD=0.0;
    cs->mean=0.0;
    cs->standard_deviation=0.0;
    cs->variance=0.0;
    cs->skewness=0.0;
    cs->kurtosis=0.0;
    cs->entropy=0.0;
  }
}

static double CountHistogramBins(const Image *image,const double *histogram,
  const ssize_t i)
{
  double
    number_bins;

  ssize_t
    j;

  number_bins=0.0;
  for (j=0; j <= (ssize_t) MaxMap; j++)
    if (histogram[(ssize_t) GetPixelChannels(image)*j+i] > 0.0)
      number_bins++;
  return(number_bins);
}

static void NormalizeChannelSums(ChannelStatistics *cs)
{
  if (cs->area > 0.0)
    {
      cs->sum/=cs->area;
      cs->sum_squared/=cs->area;
      cs->sum_cubed/=cs->area;
      cs->sum_fourth_power/=cs->area;
    }
}

static void ComputeCompositeStatistics(const Image *image,
  ChannelStatistics *channel_statistics)
{
  ChannelStatistics
    *cs_comp;

  double
    channels;

  ssize_t
    i;

  cs_comp=channel_statistics+CompositePixelChannel;
  cs_comp->sum=0.0;
  cs_comp->sum_squared=0.0;
  cs_comp->sum_cubed=0.0;
  cs_comp->sum_fourth_power=0.0;
  cs_comp->maxima=(-MagickMaximumValue);
  cs_comp->minima=MagickMaximumValue;
  cs_comp->area=0.0;
  cs_comp->mean=0.0;
  cs_comp->median=0.0;
  cs_comp->variance=0.0;
  cs_comp->standard_deviation=0.0;
  cs_comp->entropy=0.0;
  cs_comp->skewness=0.0;
  cs_comp->kurtosis=0.0;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ChannelStatistics
      *cs;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    cs=channel_statistics+channel;
    if (cs_comp->maxima < cs->maxima)
      cs_comp->maxima=cs->maxima;
    if (cs_comp->minima > cs->minima)
      cs_comp->minima=cs->minima;
    cs_comp->sum+=cs->sum;
    cs_comp->sum_squared+=cs->sum_squared;
    cs_comp->sum_cubed+=cs->sum_cubed;
    cs_comp->sum_fourth_power+=cs->sum_fourth_power;
    cs_comp->median+=cs->median;
    cs_comp->area+=cs->area;
    cs_comp->mean+=cs->mean;
    cs_comp->variance+=cs->variance;
    cs_comp->standard_deviation+=cs->standard_deviation;
    cs_comp->skewness+=cs->skewness;
    cs_comp->kurtosis+=cs->kurtosis;
    cs_comp->entropy+=cs->entropy;
  }
  channels=(double) GetImageChannels(image);
  cs_comp->sum/=channels;
  cs_comp->sum_squared/=channels;
  cs_comp->sum_cubed/=channels;
  cs_comp->sum_fourth_power/=channels;
  cs_comp->median/=channels;
  cs_comp->area/=channels;
  cs_comp->mean/=channels;
  cs_comp->variance/=channels;
  cs_comp->standard_deviation/=channels;
  cs_comp->skewness/=channels;
  cs_comp->kurtosis/=channels;
  cs_comp->entropy/=channels;
}

static void ComputeChannelStatisticsMoments(const Image *image,
  ChannelStatistics *channel_statistics,const ssize_t i)
{
  ChannelStatistics
    *cs;

  PixelChannel channel = GetPixelChannelChannel(image,i);
  PixelTrait traits = GetPixelChannelTraits(image,channel);
  if ((traits & UpdatePixelTrait) == 0)
    return;
  cs=channel_statistics+(ssize_t) channel;
  ComputeChannelMoments(cs);
}

static void ComputeChannelEntropy(const Image *image,const double *histogram,
  ChannelStatistics *channel_statistics,const ssize_t i)
{
  ChannelStatistics
    *cs;

  double
    number_bins;

  long double
    area;

  ssize_t
    j;

  PixelChannel channel = GetPixelChannelChannel(image,i);
  PixelTrait traits = GetPixelChannelTraits(image,channel);
  if ((traits & UpdatePixelTrait) == 0)
    return;
  cs=channel_statistics+(ssize_t) channel;
  NormalizeChannelSums(cs);
  /*
    Compute pixel entropy.
  */
  number_bins=CountHistogramBins(image,histogram,i);
  area=MagickSafeReciprocalLD(channel_statistics[channel].area);
  number_bins=(double) MagickSafeReciprocalLD((long double) log2(number_bins));
  for (j=0; j <= (ssize_t) MaxMap; j++)
  {
    double
      entropy,
      count;

    count=(double) (area*histogram[(ssize_t) GetPixelChannels(image)*j+i]);
    entropy=-count*log2(count)*number_bins;
    if (IsNaN(entropy) != 0)
      continue;
    channel_statistics[channel].entropy+=(double) entropy;
    channel_statistics[CompositePixelChannel].entropy+=((double) entropy/
      GetPixelChannels(image));
  }
}

static void IncreaseChannelDepth(ChannelStatistics *channel_statistics,
  ChannelStatistics *cs)
{
  cs->depth++;
  if (cs->depth > channel_statistics[CompositePixelChannel].depth)
    channel_statistics[CompositePixelChannel].depth=cs->depth;
}

typedef struct _ChannelSample
{
  const Image
    *image;

  const Quantum
    *p;

  ssize_t
    i;
} ChannelSample;

static void AccumulateChannelSample(const ChannelSample *sample,
  ChannelStatistics *cs,double *histogram)
{
  const Image
    *const image = sample->image;

  const Quantum
    *const p = sample->p;

  const ssize_t
    i = sample->i;

  cs->area++;
  if ((double) p[i] < cs->minima)
    cs->minima=(double) p[i];
  if ((double) p[i] > cs->maxima)
    cs->maxima=(double) p[i];
  histogram[(ssize_t) GetPixelChannels(image)*ScaleQuantumToMap(
    ClampToQuantum((double) p[i]))+i]++;
  cs->sumLD+=(long double) p[i];
  /*
    sum_squared, sum_cubed and sum_fourth_power are not used in
    MagickCore or MagickWand, but are made available in
    Magick++/lib/Statistic.cpp, so we need to calculate these.
  */
  cs->sum_squared+=(double) p[i]*(double) p[i];
  cs->sum_cubed+=(double) p[i]*(double) p[i]*(double) p[i];
  cs->sum_fourth_power+=(double) p[i]*(double) p[i]*(double) p[i]*
    (double) p[i];
  UpdateWelfordMoments(cs,p,i);
}

typedef struct _MedianRow
{
  const Image
    *image;

  ssize_t
    i;
} MedianRow;

static size_t CollectMedianRow(const MedianRow *row,const Quantum *p,
  Quantum *median,size_t n)
{
  const Image
    *const image = row->image;

  const ssize_t
    i = row->i;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelReadMask(image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    median[n++]=p[i];
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(n);
}

typedef struct _MedianPass
{
  const Image
    *image;

  ExceptionInfo
    *exception;

  Quantum
    *median;

  ssize_t
    i;
} MedianPass;

static void CollectMedianPass(const MedianPass *pass,ssize_t *y,size_t *n)
{
  for (*y=0; *y < (ssize_t) pass->image->rows; (*y)++)
  {
    const Quantum
      *magick_restrict p;

    MedianRow
      row;

    p=GetVirtualPixels(pass->image,0,*y,pass->image->columns,1,
      pass->exception);
    if (p == (const Quantum *) NULL)
      break;
    row.image=pass->image;
    row.i=pass->i;
    *n=CollectMedianRow(&row,p,pass->median,*n);
  }
}

static ChannelStatistics *RelinquishImageStatistics(const Image *image,
  double *histogram,ChannelStatistics *channel_statistics,
  ExceptionInfo *exception)
{
  if (histogram != (double *) NULL)
    histogram=(double *) RelinquishMagickMemory(histogram);
  if (channel_statistics != (ChannelStatistics *) NULL)
    channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
      channel_statistics);
  (void) ThrowMagickException(exception,GetMagickModule(),
    ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
  return(channel_statistics);
}

typedef struct _MedianStatistics
{
  const Image
    *image;

  ExceptionInfo
    *exception;

  ChannelStatistics
    *channel_statistics;

  MemoryInfo
    *median_info;
} MedianStatistics;

static void ComputeChannelMedians(const MedianStatistics *medians,ssize_t *y)
{
  Quantum
    *median;

  ssize_t
    i;

  median=(Quantum *) GetVirtualMemoryBlob(medians->median_info);
  for (i=0; i < (ssize_t) GetPixelChannels(medians->image); i++)
  {
    MedianPass
      pass;

    size_t
      n = 0;

    /*
      Compute median statistics for each channel.
    */
    PixelChannel channel = GetPixelChannelChannel(medians->image,i);
    PixelTrait traits = GetPixelChannelTraits(medians->image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    pass.image=medians->image;
    pass.exception=medians->exception;
    pass.median=median;
    pass.i=i;
    CollectMedianPass(&pass,y,&n);
    medians->channel_statistics[channel].median=(double) median[
      GetMedianPixel(median,n)];
  }
}

static MagickBooleanType CheckChannelDepth(
  ChannelStatistics *channel_statistics,ChannelStatistics *cs,
  const Quantum *magick_restrict p,const ssize_t i)
{
  MagickStatusType
    status;

  QuantumAny
    range;

  size_t
    depth;

  if (cs->depth != MAGICKCORE_QUANTUM_DEPTH)
    {
      depth=cs->depth;
      range=GetQuantumRange(depth);
      status=p[i] != ScaleAnyToQuantum(ScaleQuantumToAny(p[i],range),
        range) ? MagickTrue : MagickFalse;
      if (status != MagickFalse)
        {
          IncreaseChannelDepth(channel_statistics,cs);
          return(MagickFalse);
        }
    }
  return(MagickTrue);
}

static void AccumulatePixelStatistics(const Image *image,
  const Quantum *magick_restrict p,ChannelStatistics *channel_statistics,
  double *histogram)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ChannelStatistics
      *cs;

    ChannelSample
      sample;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    cs=channel_statistics+channel;
    if (CheckChannelDepth(channel_statistics,cs,p,i) == MagickFalse)
      {
        i--;
        continue;
      }
    sample.image=image;
    sample.p=p;
    sample.i=i;
    AccumulateChannelSample(&sample,cs,histogram);
  }
}

static void AccumulateRowStatistics(const Image *image,
  const Quantum *magick_restrict p,ChannelStatistics *channel_statistics,
  double *histogram)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelReadMask(image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    AccumulatePixelStatistics(image,p,channel_statistics,histogram);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void FinalizeChannelStatistics(const Image *image,
  const double *histogram,ChannelStatistics *channel_statistics)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    ComputeChannelStatisticsMoments(image,channel_statistics,i);
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    ComputeChannelEntropy(image,histogram,channel_statistics,i);
}

MagickExport ChannelStatistics *GetImageStatistics(const Image *image,
  ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  double
    *histogram;

  MedianStatistics
    medians;

  MemoryInfo
    *median_info;

  Quantum
    *median;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  histogram=(double *) AcquireQuantumMemory(MaxMap+1UL,
    MagickMax(GetPixelChannels(image),1)*sizeof(*histogram));
  channel_statistics=(ChannelStatistics *) AcquireQuantumMemory(
    MaxPixelChannels+1,sizeof(*channel_statistics));
  if ((channel_statistics == (ChannelStatistics *) NULL) ||
      (histogram == (double *) NULL))
    return(RelinquishImageStatistics(image,histogram,channel_statistics,
      exception));
  (void) memset(channel_statistics,0,(MaxPixelChannels+1)*
    sizeof(*channel_statistics));
  ResetChannelStatistics(channel_statistics);
  (void) memset(histogram,0,(MaxMap+1)*GetPixelChannels(image)*
    sizeof(*histogram));
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    /*
      Compute pixel statistics.
    */
    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    AccumulateRowStatistics(image,p,channel_statistics,histogram);
  }
  FinalizeChannelStatistics(image,histogram,channel_statistics);
  histogram=(double *) RelinquishMagickMemory(histogram);
  median_info=AcquireVirtualMemory(image->columns,image->rows*sizeof(*median));
  if (median_info == (MemoryInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
  else
    {
      medians.image=image;
      medians.exception=exception;
      medians.channel_statistics=channel_statistics;
      medians.median_info=median_info;
      ComputeChannelMedians(&medians,&y);
      median_info=RelinquishVirtualMemory(median_info);
    }
  ComputeCompositeStatistics(image,channel_statistics);
  if (y < (ssize_t) image->rows)
    channel_statistics=(ChannelStatistics *) RelinquishMagickMemory(
      channel_statistics);
  return(channel_statistics);
}
