/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               FFFFF  EEEEE   AAA   TTTTT  U   U  RRRR   EEEEE               %
%               F      E      A   A    T    U   U  R   R  E                   %
%               FFF    EEE    AAAAA    T    U   U  RRRR   EEE                 %
%               F      E      A   A    T    U   U  R R    E                   %
%               F      EEEEE  A   A    T     UUU   R  R   EEEEE               %
%                                                                             %
%               TTTTT  EEEEE  X   X  TTTTT  U   U  RRRR   EEEEE               %
%                 T    E       X X     T    U   U  R   R  E                   %
%                 T    EEE      X      T    U   U  RRRR   EEE                 %
%                 T    E       X X     T    U   U  R R    E                   %
%                 T    EEEEE  X   X    T     UUU   R  R   EEEEE               %
%                                                                             %
%                                                                             %
%                   MagickCore Image Texture Feature Method                   %
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
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
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
#include "MagickCore/feature.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e F e a t u r e s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageFeatures() returns features for each channel in the image in
%  each of four directions (horizontal, vertical, left and right diagonals)
%  for the specified distance.  The features include the angular second
%  moment, contrast, correlation, sum of squares: variance, inverse difference
%  moment, sum average, sum variance, sum entropy, entropy, difference variance,
%  difference entropy, information measures of correlation 1, information
%  measures of correlation 2, and maximum correlation coefficient.  You can
%  access the red channel contrast, for example, like this:
%
%      channel_features=GetImageFeatures(image,1,exception);
%      contrast=channel_features[RedPixelChannel].contrast[0];
%
%  Use MagickRelinquishMemory() to free the features buffer.
%
%  The format of the GetImageFeatures method is:
%
%      ChannelFeatures *GetImageFeatures(const Image *image,
%        const size_t distance,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o distance: the distance.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _FeatureStatistics
{
  PixelInfo
    direction[4];  /* horizontal, vertical, left and right diagonals */
} FeatureStatistics;

static inline MagickBooleanType IsFeatureMemoryMissing(
  FeatureStatistics **cooccurrence,FeatureStatistics *density_x,
  FeatureStatistics *density_xy,FeatureStatistics *density_y,
  FeatureStatistics **Q,FeatureStatistics *sum)
{
  return(((cooccurrence == (FeatureStatistics **) NULL) ||
    (density_x == (FeatureStatistics *) NULL) ||
    (density_xy == (FeatureStatistics *) NULL) ||
    (density_y == (FeatureStatistics *) NULL) ||
    (Q == (FeatureStatistics **) NULL) ||
    (sum == (FeatureStatistics *) NULL)) ? MagickTrue : MagickFalse);
}

static unsigned int GetFeatureGrayLevels(const Image *image,
  const PixelPacket gray)
{
  unsigned int
    number_grays;

  number_grays=gray.red;
  if (gray.green > number_grays)
    number_grays=gray.green;
  if (gray.blue > number_grays)
    number_grays=gray.blue;
  if ((image->colorspace == CMYKColorspace) && (gray.black > number_grays))
    number_grays=gray.black;
  if ((image->alpha_trait != UndefinedPixelTrait) &&
      (gray.alpha > number_grays))
    number_grays=gray.alpha;
  return(number_grays);
}

static PixelPacket CompactFeatureGrays(const Image *image,PixelPacket *grays)
{
  PixelPacket
    gray;

  ssize_t
    i;

  (void) memset(&gray,0,sizeof(gray));
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    if (grays[i].red != ~0U)
      grays[gray.red++].red=grays[i].red;
    if (grays[i].green != ~0U)
      grays[gray.green++].green=grays[i].green;
    if (grays[i].blue != ~0U)
      grays[gray.blue++].blue=grays[i].blue;
    if ((image->colorspace == CMYKColorspace) && (grays[i].black != ~0U))
      grays[gray.black++].black=grays[i].black;
    if ((image->alpha_trait != UndefinedPixelTrait) &&
        (grays[i].alpha != ~0U))
      grays[gray.alpha++].alpha=grays[i].alpha;
  }
  return(gray);
}

static ssize_t GetFeatureAdjacencyOffset(const Image *image,
  const size_t distance,const ssize_t i)
{
  ssize_t
    offset;

  switch (i)
  {
    case 0:
    default:
    {
      /*
        Horizontal adjacency.
      */
      offset=(ssize_t) distance;
      break;
    }
    case 1:
    {
      /*
        Vertical adjacency.
      */
      offset=(ssize_t) (image->columns+2*distance);
      break;
    }
    case 2:
    {
      /*
        Right diagonal adjacency.
      */
      offset=(ssize_t) ((image->columns+2*distance)-distance);
      break;
    }
    case 3:
    {
      /*
        Left diagonal adjacency.
      */
      offset=(ssize_t) ((image->columns+2*distance)+distance);
      break;
    }
  }
  return(offset);
}

static double GetFeatureNormalization(const Image *image,
  const size_t distance,const ssize_t i)
{
  double
    normalize;

  switch (i)
  {
    case 0:
    default:
    {
      /*
        Horizontal adjacency.
      */
      normalize=2.0*image->rows*(image->columns-distance);
      break;
    }
    case 1:
    {
      /*
        Vertical adjacency.
      */
      normalize=2.0*(image->rows-distance)*image->columns;
      break;
    }
    case 2:
    {
      /*
        Right diagonal adjacency.
      */
      normalize=2.0*(image->rows-distance)*(image->columns-distance);
      break;
    }
    case 3:
    {
      /*
        Left diagonal adjacency.
      */
      normalize=2.0*(image->rows-distance)*(image->columns-distance);
      break;
    }
  }
  return(normalize);
}

static void AddFeatureStatistics(const Image *image,PixelInfo *target,
  const PixelInfo *source)
{
  target->red+=source->red;
  target->green+=source->green;
  target->blue+=source->blue;
  if (image->colorspace == CMYKColorspace)
    target->black+=source->black;
  if (image->alpha_trait != UndefinedPixelTrait)
    target->alpha+=source->alpha;
}

static void SubtractFeatureEntropy(const Image *image,PixelInfo *entropy,
  const PixelInfo *density)
{
  entropy->red-=(density->red*log2(density->red));
  entropy->green-=(density->green*log2(density->green));
  entropy->blue-=(density->blue*log2(density->blue));
  if (image->colorspace == CMYKColorspace)
    entropy->black-=(density->black*log2(density->black));
  if (image->alpha_trait != UndefinedPixelTrait)
    entropy->alpha-=(density->alpha*log2(density->alpha));
}

typedef struct _TextureDirection
{
  const Image
    *image;

  ChannelFeatures
    *channel_features;

  FeatureStatistics
    **cooccurrence,
    *density_x,
    *density_xy,
    *density_y,
    **Q,
    *sum;

  unsigned int
    number_grays;

  ssize_t
    i;
} TextureDirection;

typedef struct _TextureSlots
{
  PixelInfo
    *correlation,
    *entropy_x,
    *entropy_xy,
    *entropy_xy1,
    *entropy_xy2,
    *entropy_y,
    *mean,
    *sum_squares,
    *variance;
} TextureSlots;

static void AccumulateMomentAndCorrelation(const TextureDirection *texture,
  PixelInfo *correlation,const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const sum = texture->sum;

  const ssize_t
    i = texture->i;

  /*
    Angular second moment:  measure of homogeneity of the image.
  */
  channel_features[RedPixelChannel].angular_second_moment[i]+=
    cooccurrence[x][y].direction[i].red*
    cooccurrence[x][y].direction[i].red;
  channel_features[GreenPixelChannel].angular_second_moment[i]+=
    cooccurrence[x][y].direction[i].green*
    cooccurrence[x][y].direction[i].green;
  channel_features[BluePixelChannel].angular_second_moment[i]+=
    cooccurrence[x][y].direction[i].blue*
    cooccurrence[x][y].direction[i].blue;
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].angular_second_moment[i]+=
      cooccurrence[x][y].direction[i].black*
      cooccurrence[x][y].direction[i].black;
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].angular_second_moment[i]+=
      cooccurrence[x][y].direction[i].alpha*
      cooccurrence[x][y].direction[i].alpha;
  /*
    Correlation: measure of linear-dependencies in the image.
  */
  AddFeatureStatistics(image,&sum[y].direction[i],
    &cooccurrence[x][y].direction[i]);
  correlation->red+=x*y*cooccurrence[x][y].direction[i].red;
  correlation->green+=x*y*
    cooccurrence[x][y].direction[i].green;
  correlation->blue+=x*y*
    cooccurrence[x][y].direction[i].blue;
  if (image->colorspace == CMYKColorspace)
    correlation->black+=x*y*
      cooccurrence[x][y].direction[i].black;
  if (image->alpha_trait != UndefinedPixelTrait)
    correlation->alpha+=x*y*
      cooccurrence[x][y].direction[i].alpha;
}

static void AccumulateInverseDifferenceAndSum(const TextureDirection *texture,
  const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_xy = texture->density_xy;

  const ssize_t
    i = texture->i;

  /*
    Inverse Difference Moment.
  */
  channel_features[RedPixelChannel].inverse_difference_moment[i]+=
    cooccurrence[x][y].direction[i].red/((y-x)*(y-x)+1);
  channel_features[GreenPixelChannel].inverse_difference_moment[i]+=
    cooccurrence[x][y].direction[i].green/((y-x)*(y-x)+1);
  channel_features[BluePixelChannel].inverse_difference_moment[i]+=
    cooccurrence[x][y].direction[i].blue/((y-x)*(y-x)+1);
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].inverse_difference_moment[i]+=
      cooccurrence[x][y].direction[i].black/((y-x)*(y-x)+1);
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].inverse_difference_moment[i]+=
      cooccurrence[x][y].direction[i].alpha/((y-x)*(y-x)+1);
  /*
    Sum average.
  */
  AddFeatureStatistics(image,&density_xy[y+x+2].direction[i],
    &cooccurrence[x][y].direction[i]);
}

static void AccumulateEntropyAndDensities(const TextureDirection *texture,
  const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_x = texture->density_x,
    *const density_y = texture->density_y;

  const ssize_t
    i = texture->i;

  /*
    Entropy.
  */
  channel_features[RedPixelChannel].entropy[i]-=
    cooccurrence[x][y].direction[i].red*
    log2(cooccurrence[x][y].direction[i].red);
  channel_features[GreenPixelChannel].entropy[i]-=
    cooccurrence[x][y].direction[i].green*
    log2(cooccurrence[x][y].direction[i].green);
  channel_features[BluePixelChannel].entropy[i]-=
    cooccurrence[x][y].direction[i].blue*
    log2(cooccurrence[x][y].direction[i].blue);
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].entropy[i]-=
      cooccurrence[x][y].direction[i].black*
      log2(cooccurrence[x][y].direction[i].black);
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].entropy[i]-=
      cooccurrence[x][y].direction[i].alpha*
      log2(cooccurrence[x][y].direction[i].alpha);
  /*
    Information Measures of Correlation.
  */
  density_x[x].direction[i].red+=cooccurrence[x][y].direction[i].red;
  density_x[x].direction[i].green+=cooccurrence[x][y].direction[i].green;
  density_x[x].direction[i].blue+=cooccurrence[x][y].direction[i].blue;
  if (image->alpha_trait != UndefinedPixelTrait)
    density_x[x].direction[i].alpha+=
      cooccurrence[x][y].direction[i].alpha;
  if (image->colorspace == CMYKColorspace)
    density_x[x].direction[i].black+=
      cooccurrence[x][y].direction[i].black;
  AddFeatureStatistics(image,&density_y[y].direction[i],
    &cooccurrence[x][y].direction[i]);
}

static void AccumulateTextureMean(const TextureDirection *texture,
  PixelInfo *mean,PixelInfo *sum_squares,const ssize_t y)
{
  const Image
    *const image = texture->image;

  FeatureStatistics
    *const sum = texture->sum;

  const ssize_t
    i = texture->i;

  mean->red+=y*sum[y].direction[i].red;
  sum_squares->red+=y*y*sum[y].direction[i].red;
  mean->green+=y*sum[y].direction[i].green;
  sum_squares->green+=y*y*sum[y].direction[i].green;
  mean->blue+=y*sum[y].direction[i].blue;
  sum_squares->blue+=y*y*sum[y].direction[i].blue;
  if (image->colorspace == CMYKColorspace)
    {
      mean->black+=y*sum[y].direction[i].black;
      sum_squares->black+=y*y*sum[y].direction[i].black;
    }
  if (image->alpha_trait != UndefinedPixelTrait)
    {
      mean->alpha+=y*sum[y].direction[i].alpha;
      sum_squares->alpha+=y*y*sum[y].direction[i].alpha;
    }
}

static void SetTextureCorrelation(const TextureDirection *texture,
  const TextureSlots *slots)
{
  const PixelInfo
    *const correlation = slots->correlation,
    *const mean = slots->mean,
    *const sum_squares = slots->sum_squares;

  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  const ssize_t
    i = texture->i;

  /*
    Correlation: measure of linear-dependencies in the image.
  */
  channel_features[RedPixelChannel].correlation[i]=
    (correlation->red-mean->red*
    mean->red)/(sqrt(sum_squares->red-
    (mean->red*mean->red))*sqrt(
    sum_squares->red-(mean->red*
    mean->red)));
  channel_features[GreenPixelChannel].correlation[i]=
    (correlation->green-mean->green*
    mean->green)/(sqrt(sum_squares->green-
    (mean->green*mean->green))*sqrt(
    sum_squares->green-(mean->green*
    mean->green)));
  channel_features[BluePixelChannel].correlation[i]=
    (correlation->blue-mean->blue*
    mean->blue)/(sqrt(sum_squares->blue-
    (mean->blue*mean->blue))*sqrt(
    sum_squares->blue-(mean->blue*
    mean->blue)));
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].correlation[i]=
      (correlation->black-mean->black*
      mean->black)/(sqrt(sum_squares->black-
      (mean->black*mean->black))*sqrt(
      sum_squares->black-(mean->black*
      mean->black)));
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].correlation[i]=
      (correlation->alpha-mean->alpha*
      mean->alpha)/(sqrt(sum_squares->alpha-
      (mean->alpha*mean->alpha))*sqrt(
      sum_squares->alpha-(mean->alpha*
      mean->alpha)));
}

static void ComputeTextureFeatures(const TextureDirection *texture,
  const TextureSlots *slots)
{
  const unsigned int
    number_grays = texture->number_grays;

  ssize_t
    y;

  for (y=0; y < (ssize_t) number_grays; y++)
  {
    ssize_t
      x;

    for (x=0; x < (ssize_t) number_grays; x++)
    {
      AccumulateMomentAndCorrelation(texture,slots->correlation,x,y);
      AccumulateInverseDifferenceAndSum(texture,x,y);
      AccumulateEntropyAndDensities(texture,x,y);
    }
    AccumulateTextureMean(texture,slots->mean,slots->sum_squares,y);
  }
  SetTextureCorrelation(texture,slots);
}

static void AccumulateSumEntropyAndVariance(const TextureDirection *texture,
  const ssize_t x)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    *const density_xy = texture->density_xy;

  const ssize_t
    i = texture->i;

  /*
    Sum entropy.
  */
  channel_features[RedPixelChannel].sum_entropy[i]-=
    density_xy[x].direction[i].red*
    log2(density_xy[x].direction[i].red);
  channel_features[GreenPixelChannel].sum_entropy[i]-=
    density_xy[x].direction[i].green*
    log2(density_xy[x].direction[i].green);
  channel_features[BluePixelChannel].sum_entropy[i]-=
    density_xy[x].direction[i].blue*
    log2(density_xy[x].direction[i].blue);
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].sum_entropy[i]-=
      density_xy[x].direction[i].black*
      log2(density_xy[x].direction[i].black);
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].sum_entropy[i]-=
      density_xy[x].direction[i].alpha*
      log2(density_xy[x].direction[i].alpha);
  /*
    Sum variance.
  */
  channel_features[RedPixelChannel].sum_variance[i]+=
    (x-channel_features[RedPixelChannel].sum_entropy[i])*
    (x-channel_features[RedPixelChannel].sum_entropy[i])*
    density_xy[x].direction[i].red;
  channel_features[GreenPixelChannel].sum_variance[i]+=
    (x-channel_features[GreenPixelChannel].sum_entropy[i])*
    (x-channel_features[GreenPixelChannel].sum_entropy[i])*
    density_xy[x].direction[i].green;
  channel_features[BluePixelChannel].sum_variance[i]+=
    (x-channel_features[BluePixelChannel].sum_entropy[i])*
    (x-channel_features[BluePixelChannel].sum_entropy[i])*
    density_xy[x].direction[i].blue;
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].sum_variance[i]+=
      (x-channel_features[BlackPixelChannel].sum_entropy[i])*
      (x-channel_features[BlackPixelChannel].sum_entropy[i])*
      density_xy[x].direction[i].black;
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].sum_variance[i]+=
      (x-channel_features[AlphaPixelChannel].sum_entropy[i])*
      (x-channel_features[AlphaPixelChannel].sum_entropy[i])*
      density_xy[x].direction[i].alpha;
}

static void ComputeSumFeatures(const TextureDirection *texture)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    *const density_xy = texture->density_xy;

  const unsigned int
    number_grays = texture->number_grays;

  const ssize_t
    i = texture->i;

  ssize_t
    x;

  for (x=2; x < (ssize_t) (2*number_grays); x++)
  {
    /*
      Sum average.
    */
    channel_features[RedPixelChannel].sum_average[i]+=
      x*density_xy[x].direction[i].red;
    channel_features[GreenPixelChannel].sum_average[i]+=
      x*density_xy[x].direction[i].green;
    channel_features[BluePixelChannel].sum_average[i]+=
      x*density_xy[x].direction[i].blue;
    if (image->colorspace == CMYKColorspace)
      channel_features[BlackPixelChannel].sum_average[i]+=
        x*density_xy[x].direction[i].black;
    if (image->alpha_trait != UndefinedPixelTrait)
      channel_features[AlphaPixelChannel].sum_average[i]+=
        x*density_xy[x].direction[i].alpha;
    AccumulateSumEntropyAndVariance(texture,x);
  }
}

static void AccumulateVarianceAndDifference(const TextureDirection *texture,
  const TextureSlots *slots,const ssize_t x,const ssize_t y)
{
  const PixelInfo
    *const mean = slots->mean;

  PixelInfo
    *const variance = slots->variance;

  const Image
    *const image = texture->image;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_xy = texture->density_xy;

  const ssize_t
    i = texture->i;

  /*
    Sum of Squares: Variance
  */
  variance->red+=(y-mean->red+1)*
    (y-mean->red+1)*cooccurrence[x][y].direction[i].red;
  variance->green+=(y-mean->green+1)*
    (y-mean->green+1)*cooccurrence[x][y].direction[i].green;
  variance->blue+=(y-mean->blue+1)*
    (y-mean->blue+1)*cooccurrence[x][y].direction[i].blue;
  if (image->colorspace == CMYKColorspace)
    variance->black+=(y-mean->black+1)*
      (y-mean->black+1)*cooccurrence[x][y].direction[i].black;
  if (image->alpha_trait != UndefinedPixelTrait)
    variance->alpha+=(y-mean->alpha+1)*
      (y-mean->alpha+1)*
      cooccurrence[x][y].direction[i].alpha;
  /*
    Sum average / Difference Variance.
  */
  AddFeatureStatistics(image,
    &density_xy[MagickAbsoluteValue(y-x)].direction[i],
    &cooccurrence[x][y].direction[i]);
}

static void AccumulateCorrelationEntropies(const TextureDirection *texture,
  const TextureSlots *slots,const ssize_t x,const ssize_t y)
{
  PixelInfo
    *const entropy_xy = slots->entropy_xy,
    *const entropy_xy1 = slots->entropy_xy1,
    *const entropy_xy2 = slots->entropy_xy2;

  const Image
    *const image = texture->image;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_x = texture->density_x,
    *const density_y = texture->density_y;

  const ssize_t
    i = texture->i;

  /*
    Information Measures of Correlation.
  */
  entropy_xy->red-=cooccurrence[x][y].direction[i].red*
    log2(cooccurrence[x][y].direction[i].red);
  entropy_xy->green-=cooccurrence[x][y].direction[i].green*
    log2(cooccurrence[x][y].direction[i].green);
  entropy_xy->blue-=cooccurrence[x][y].direction[i].blue*
    log2(cooccurrence[x][y].direction[i].blue);
  if (image->colorspace == CMYKColorspace)
    entropy_xy->black-=cooccurrence[x][y].direction[i].black*
      log2(cooccurrence[x][y].direction[i].black);
  if (image->alpha_trait != UndefinedPixelTrait)
    entropy_xy->alpha-=
      cooccurrence[x][y].direction[i].alpha*log2(
      cooccurrence[x][y].direction[i].alpha);
  entropy_xy1->red-=(cooccurrence[x][y].direction[i].red*
    log2(density_x[x].direction[i].red*density_y[y].direction[i].red));
  entropy_xy1->green-=(cooccurrence[x][y].direction[i].green*
    log2(density_x[x].direction[i].green*
    density_y[y].direction[i].green));
  entropy_xy1->blue-=(cooccurrence[x][y].direction[i].blue*
    log2(density_x[x].direction[i].blue*density_y[y].direction[i].blue));
  if (image->colorspace == CMYKColorspace)
    entropy_xy1->black-=(
      cooccurrence[x][y].direction[i].black*log2(
      density_x[x].direction[i].black*density_y[y].direction[i].black));
  if (image->alpha_trait != UndefinedPixelTrait)
    entropy_xy1->alpha-=(
      cooccurrence[x][y].direction[i].alpha*log2(
      density_x[x].direction[i].alpha*density_y[y].direction[i].alpha));
  entropy_xy2->red-=(density_x[x].direction[i].red*
    density_y[y].direction[i].red*log2(density_x[x].direction[i].red*
    density_y[y].direction[i].red));
  entropy_xy2->green-=(density_x[x].direction[i].green*
    density_y[y].direction[i].green*log2(density_x[x].direction[i].green*
    density_y[y].direction[i].green));
  entropy_xy2->blue-=(density_x[x].direction[i].blue*
    density_y[y].direction[i].blue*log2(density_x[x].direction[i].blue*
    density_y[y].direction[i].blue));
  if (image->colorspace == CMYKColorspace)
    entropy_xy2->black-=(density_x[x].direction[i].black*
      density_y[y].direction[i].black*log2(
      density_x[x].direction[i].black*density_y[y].direction[i].black));
  if (image->alpha_trait != UndefinedPixelTrait)
    entropy_xy2->alpha-=(density_x[x].direction[i].alpha*
      density_y[y].direction[i].alpha*log2(
      density_x[x].direction[i].alpha*density_y[y].direction[i].alpha));
}

static void ComputeVarianceFeatures(const TextureDirection *texture,
  const TextureSlots *slots)
{
  const PixelInfo
    *const variance = slots->variance;

  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  const unsigned int
    number_grays = texture->number_grays;

  const ssize_t
    i = texture->i;

  ssize_t
    y;

  for (y=0; y < (ssize_t) number_grays; y++)
  {
    ssize_t
      x;

    for (x=0; x < (ssize_t) number_grays; x++)
    {
      AccumulateVarianceAndDifference(texture,slots,x,y);
      AccumulateCorrelationEntropies(texture,slots,x,y);
    }
  }
  channel_features[RedPixelChannel].variance_sum_of_squares[i]=
    variance->red;
  channel_features[GreenPixelChannel].variance_sum_of_squares[i]=
    variance->green;
  channel_features[BluePixelChannel].variance_sum_of_squares[i]=
    variance->blue;
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].variance_sum_of_squares[i]=
      variance->black;
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].variance_sum_of_squares[i]=
      variance->alpha;
}

static void AccumulateDifferenceVariance(const TextureDirection *texture,
  const TextureSlots *slots,const ssize_t x)
{
  PixelInfo
    *const variance = slots->variance,
    *const sum_squares = slots->sum_squares;

  const Image
    *const image = texture->image;

  FeatureStatistics
    *const density_xy = texture->density_xy;

  const ssize_t
    i = texture->i;

  /*
    Difference variance.
  */
  AddFeatureStatistics(image,variance,
    &density_xy[x].direction[i]);
  sum_squares->red+=density_xy[x].direction[i].red*
    density_xy[x].direction[i].red;
  sum_squares->green+=density_xy[x].direction[i].green*
    density_xy[x].direction[i].green;
  sum_squares->blue+=density_xy[x].direction[i].blue*
    density_xy[x].direction[i].blue;
  if (image->colorspace == CMYKColorspace)
    sum_squares->black+=density_xy[x].direction[i].black*
      density_xy[x].direction[i].black;
  if (image->alpha_trait != UndefinedPixelTrait)
    sum_squares->alpha+=density_xy[x].direction[i].alpha*
      density_xy[x].direction[i].alpha;
}

static void AccumulateDifferenceEntropies(const TextureDirection *texture,
  const TextureSlots *slots,const ssize_t x)
{
  PixelInfo
    *const entropy_x = slots->entropy_x,
    *const entropy_y = slots->entropy_y;

  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    *const density_x = texture->density_x,
    *const density_xy = texture->density_xy,
    *const density_y = texture->density_y;

  const ssize_t
    i = texture->i;

  /*
    Difference entropy.
  */
  channel_features[RedPixelChannel].difference_entropy[i]-=
    density_xy[x].direction[i].red*
    log2(density_xy[x].direction[i].red);
  channel_features[GreenPixelChannel].difference_entropy[i]-=
    density_xy[x].direction[i].green*
    log2(density_xy[x].direction[i].green);
  channel_features[BluePixelChannel].difference_entropy[i]-=
    density_xy[x].direction[i].blue*
    log2(density_xy[x].direction[i].blue);
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].difference_entropy[i]-=
      density_xy[x].direction[i].black*
      log2(density_xy[x].direction[i].black);
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].difference_entropy[i]-=
      density_xy[x].direction[i].alpha*
      log2(density_xy[x].direction[i].alpha);
  /*
    Information Measures of Correlation.
  */
  SubtractFeatureEntropy(image,entropy_x,
    &density_x[x].direction[i]);
  SubtractFeatureEntropy(image,entropy_y,
    &density_y[x].direction[i]);
}

static void SetFirstMeasureOfCorrelation(const TextureDirection *texture,
  const TextureSlots *slots)
{
  const PixelInfo
    *const entropy_x = slots->entropy_x,
    *const entropy_xy = slots->entropy_xy,
    *const entropy_xy1 = slots->entropy_xy1,
    *const entropy_y = slots->entropy_y;

  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  const ssize_t
    i = texture->i;

  /*
    Information Measures of Correlation.
  */
  channel_features[RedPixelChannel].measure_of_correlation_1[i]=
    (entropy_xy->red-entropy_xy1->red)/
    (entropy_x->red > entropy_y->red ?
     entropy_x->red : entropy_y->red);
  channel_features[GreenPixelChannel].measure_of_correlation_1[i]=
    (entropy_xy->green-entropy_xy1->green)/
    (entropy_x->green > entropy_y->green ?
     entropy_x->green : entropy_y->green);
  channel_features[BluePixelChannel].measure_of_correlation_1[i]=
    (entropy_xy->blue-entropy_xy1->blue)/
    (entropy_x->blue > entropy_y->blue ?
     entropy_x->blue : entropy_y->blue);
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].measure_of_correlation_1[i]=
      (entropy_xy->black-entropy_xy1->black)/
      (entropy_x->black > entropy_y->black ?
       entropy_x->black : entropy_y->black);
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].measure_of_correlation_1[i]=
      (entropy_xy->alpha-entropy_xy1->alpha)/
      (entropy_x->alpha > entropy_y->alpha ?
       entropy_x->alpha : entropy_y->alpha);
}

static void SetSecondMeasureOfCorrelation(const TextureDirection *texture,
  const TextureSlots *slots)
{
  const PixelInfo
    *const entropy_xy = slots->entropy_xy,
    *const entropy_xy2 = slots->entropy_xy2;

  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  const ssize_t
    i = texture->i;

  channel_features[RedPixelChannel].measure_of_correlation_2[i]=
    (sqrt(fabs(1.0-exp(-2.0*(double) (entropy_xy2->red-
    entropy_xy->red)))));
  channel_features[GreenPixelChannel].measure_of_correlation_2[i]=
    (sqrt(fabs(1.0-exp(-2.0*(double) (entropy_xy2->green-
    entropy_xy->green)))));
  channel_features[BluePixelChannel].measure_of_correlation_2[i]=
    (sqrt(fabs(1.0-exp(-2.0*(double) (entropy_xy2->blue-
    entropy_xy->blue)))));
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].measure_of_correlation_2[i]=
      (sqrt(fabs(1.0-exp(-2.0*(double) (entropy_xy2->black-
      entropy_xy->black)))));
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].measure_of_correlation_2[i]=
      (sqrt(fabs(1.0-exp(-2.0*(double) (entropy_xy2->alpha-
      entropy_xy->alpha)))));
}

static void ComputeDifferenceFeatures(const TextureDirection *texture,
  const TextureSlots *slots)
{
  const PixelInfo
    *const sum_squares = slots->sum_squares,
    *const variance = slots->variance;

  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  const unsigned int
    number_grays = texture->number_grays;

  const ssize_t
    i = texture->i;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_grays; x++)
  {
    AccumulateDifferenceVariance(texture,slots,x);
    AccumulateDifferenceEntropies(texture,slots,x);
  }
  /*
    Difference variance.
  */
  channel_features[RedPixelChannel].difference_variance[i]=
    (((double) number_grays*number_grays*sum_squares->red)-
    (variance->red*variance->red))/
    ((double) number_grays*number_grays*number_grays*number_grays);
  channel_features[GreenPixelChannel].difference_variance[i]=
    (((double) number_grays*number_grays*sum_squares->green)-
    (variance->green*variance->green))/
    ((double) number_grays*number_grays*number_grays*number_grays);
  channel_features[BluePixelChannel].difference_variance[i]=
    (((double) number_grays*number_grays*sum_squares->blue)-
    (variance->blue*variance->blue))/
    ((double) number_grays*number_grays*number_grays*number_grays);
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].difference_variance[i]=
      (((double) number_grays*number_grays*sum_squares->black)-
      (variance->black*variance->black))/
      ((double) number_grays*number_grays*number_grays*number_grays);
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].difference_variance[i]=
      (((double) number_grays*number_grays*sum_squares->alpha)-
      (variance->alpha*variance->alpha))/
      ((double) number_grays*number_grays*number_grays*number_grays);
  SetFirstMeasureOfCorrelation(texture,slots);
  SetSecondMeasureOfCorrelation(texture,slots);
}

static void AccumulateCorrelationQ(const TextureDirection *texture,
  const ssize_t x,const ssize_t y,const ssize_t z)
{
  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_x = texture->density_x,
    *const density_y = texture->density_y,
    **const Q = texture->Q;

  const ssize_t
    i = texture->i;

  /*
    Maximum Correlation Coefficient.
  */
  if ((fabs(density_x[z].direction[i].red) > MagickEpsilon) &&
      (fabs(density_y[x].direction[i].red) > MagickEpsilon))
    Q[z][y].direction[i].red+=cooccurrence[z][x].direction[i].red*
      cooccurrence[y][x].direction[i].red/density_x[z].direction[i].red/
      density_y[x].direction[i].red;
  if ((fabs(density_x[z].direction[i].green) > MagickEpsilon) &&
      (fabs(density_y[x].direction[i].red) > MagickEpsilon))
    Q[z][y].direction[i].green+=cooccurrence[z][x].direction[i].green*
      cooccurrence[y][x].direction[i].green/
      density_x[z].direction[i].green/density_y[x].direction[i].red;
  if ((fabs(density_x[z].direction[i].blue) > MagickEpsilon) &&
      (fabs(density_y[x].direction[i].blue) > MagickEpsilon))
    Q[z][y].direction[i].blue+=cooccurrence[z][x].direction[i].blue*
      cooccurrence[y][x].direction[i].blue/
      density_x[z].direction[i].blue/density_y[x].direction[i].blue;
}

static void AccumulateCorrelationQBlack(const TextureDirection *texture,
  const ssize_t x,const ssize_t y,const ssize_t z)
{
  const Image
    *const image = texture->image;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_x = texture->density_x,
    *const density_y = texture->density_y,
    **const Q = texture->Q;

  const ssize_t
    i = texture->i;

  if (image->colorspace == CMYKColorspace)
    if ((fabs(density_x[z].direction[i].black) > MagickEpsilon) &&
        (fabs(density_y[x].direction[i].black) > MagickEpsilon))
      Q[z][y].direction[i].black+=cooccurrence[z][x].direction[i].black*
        cooccurrence[y][x].direction[i].black/
        density_x[z].direction[i].black/density_y[x].direction[i].black;
}

static void AccumulateCorrelationQAlpha(const TextureDirection *texture,
  const ssize_t x,const ssize_t y,const ssize_t z)
{
  const Image
    *const image = texture->image;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_x = texture->density_x,
    *const density_y = texture->density_y,
    **const Q = texture->Q;

  const ssize_t
    i = texture->i;

  if (image->alpha_trait != UndefinedPixelTrait)
    if ((fabs(density_x[z].direction[i].alpha) > MagickEpsilon) &&
        (fabs(density_y[x].direction[i].alpha) > MagickEpsilon))
      Q[z][y].direction[i].alpha+=
        cooccurrence[z][x].direction[i].alpha*
        cooccurrence[y][x].direction[i].alpha/
        density_x[z].direction[i].alpha/
        density_y[x].direction[i].alpha;
}

static void AccumulateContrastRow(const TextureDirection *texture,
  PixelInfo *pixel,const ssize_t y,const ssize_t z)
{
  const Image
    *const image = texture->image;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence;

  const unsigned int
    number_grays = texture->number_grays;

  const ssize_t
    i = texture->i;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_grays; x++)
  {
    /*
      Contrast:  amount of local variations present in an image.
    */
    if (((y-x) == z) || ((x-y) == z))
      {
        AddFeatureStatistics(image,pixel,
          &cooccurrence[x][y].direction[i]);
      }
    AccumulateCorrelationQ(texture,x,y,z);
    AccumulateCorrelationQBlack(texture,x,y,z);
    AccumulateCorrelationQAlpha(texture,x,y,z);
  }
}

static void SetMaximumCorrelation(const TextureDirection *texture)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  const ssize_t
    i = texture->i;

  /*
    Maximum Correlation Coefficient.
    Future: return second largest eigenvalue of Q.
  */
  channel_features[RedPixelChannel].maximum_correlation_coefficient[i]=
    sqrt(-1.0);
  channel_features[GreenPixelChannel].maximum_correlation_coefficient[i]=
    sqrt(-1.0);
  channel_features[BluePixelChannel].maximum_correlation_coefficient[i]=
    sqrt(-1.0);
  if (image->colorspace == CMYKColorspace)
    channel_features[BlackPixelChannel].maximum_correlation_coefficient[i]=
      sqrt(-1.0);
  if (image->alpha_trait != UndefinedPixelTrait)
    channel_features[AlphaPixelChannel].maximum_correlation_coefficient[i]=
      sqrt(-1.0);
}

MagickExport ChannelFeatures *GetImageFeatures(const Image *image,
  const size_t distance,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  ChannelFeatures
    *channel_features;

  FeatureStatistics
    **cooccurrence,
    correlation,
    *density_x,
    *density_xy,
    *density_y,
    entropy_x,
    entropy_xy,
    entropy_xy1,
    entropy_xy2,
    entropy_y,
    mean,
    **Q,
    *sum,
    sum_squares,
    variance;

  PixelPacket
    gray,
    *grays;

  MagickBooleanType
    status;

  ssize_t
    i,
    r;

  size_t
    length;

  unsigned int
    number_grays;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->columns < (distance+1)) || (image->rows < (distance+1)))
    return((ChannelFeatures *) NULL);
  length=MaxPixelChannels+1UL;
  channel_features=(ChannelFeatures *) AcquireQuantumMemory(length,
    sizeof(*channel_features));
  if (channel_features == (ChannelFeatures *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(channel_features);
    }
  (void) memset(channel_features,0,length*
    sizeof(*channel_features));
  /*
    Form grays.
  */
  grays=(PixelPacket *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*grays));
  if (grays == (PixelPacket *) NULL)
    {
      channel_features=(ChannelFeatures *) RelinquishMagickMemory(
        channel_features);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(channel_features);
    }
  for (i=0; i <= (ssize_t) MaxMap; i++)
  {
    grays[i].red=(~0U);
    grays[i].green=(~0U);
    grays[i].blue=(~0U);
    grays[i].alpha=(~0U);
    grays[i].black=(~0U);
  }
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (r=0; r < (ssize_t) image->rows; r++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,r,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      grays[ScaleQuantumToMap(GetPixelRed(image,p))].red=
        ScaleQuantumToMap(GetPixelRed(image,p));
      grays[ScaleQuantumToMap(GetPixelGreen(image,p))].green=
        ScaleQuantumToMap(GetPixelGreen(image,p));
      grays[ScaleQuantumToMap(GetPixelBlue(image,p))].blue=
        ScaleQuantumToMap(GetPixelBlue(image,p));
      if (image->colorspace == CMYKColorspace)
        grays[ScaleQuantumToMap(GetPixelBlack(image,p))].black=
          ScaleQuantumToMap(GetPixelBlack(image,p));
      if (image->alpha_trait != UndefinedPixelTrait)
        grays[ScaleQuantumToMap(GetPixelAlpha(image,p))].alpha=
          ScaleQuantumToMap(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    {
      grays=(PixelPacket *) RelinquishMagickMemory(grays);
      channel_features=(ChannelFeatures *) RelinquishMagickMemory(
        channel_features);
      return(channel_features);
    }
  gray=CompactFeatureGrays(image,grays);
  /*
    Allocate spatial dependence matrix.
  */
  number_grays=GetFeatureGrayLevels(image,gray);
  cooccurrence=(FeatureStatistics **) AcquireQuantumMemory(number_grays,
    sizeof(*cooccurrence));
  density_x=(FeatureStatistics *) AcquireQuantumMemory(number_grays+1,
    2*sizeof(*density_x));
  density_xy=(FeatureStatistics *) AcquireQuantumMemory(number_grays+1,
    2*sizeof(*density_xy));
  density_y=(FeatureStatistics *) AcquireQuantumMemory(number_grays+1,
    2*sizeof(*density_y));
  Q=(FeatureStatistics **) AcquireQuantumMemory(number_grays,sizeof(*Q));
  sum=(FeatureStatistics *) AcquireQuantumMemory(number_grays,sizeof(*sum));
  if (IsFeatureMemoryMissing(cooccurrence,density_x,density_xy,density_y,Q,
        sum) != MagickFalse)
    {
      if (Q != (FeatureStatistics **) NULL)
        Q=(FeatureStatistics **) RelinquishMagickMemory(Q);
      if (sum != (FeatureStatistics *) NULL)
        sum=(FeatureStatistics *) RelinquishMagickMemory(sum);
      if (density_y != (FeatureStatistics *) NULL)
        density_y=(FeatureStatistics *) RelinquishMagickMemory(density_y);
      if (density_xy != (FeatureStatistics *) NULL)
        density_xy=(FeatureStatistics *) RelinquishMagickMemory(density_xy);
      if (density_x != (FeatureStatistics *) NULL)
        density_x=(FeatureStatistics *) RelinquishMagickMemory(density_x);
      if (cooccurrence != (FeatureStatistics **) NULL)
        cooccurrence=(FeatureStatistics **) RelinquishMagickMemory(
          cooccurrence);
      grays=(PixelPacket *) RelinquishMagickMemory(grays);
      channel_features=(ChannelFeatures *) RelinquishMagickMemory(
        channel_features);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(channel_features);
    }
  (void) memset(&correlation,0,sizeof(correlation));
  (void) memset(density_x,0,2*(number_grays+1)*sizeof(*density_x));
  (void) memset(density_xy,0,2*(number_grays+1)*sizeof(*density_xy));
  (void) memset(density_y,0,2*(number_grays+1)*sizeof(*density_y));
  (void) memset(&mean,0,sizeof(mean));
  (void) memset(sum,0,number_grays*sizeof(*sum));
  (void) memset(&sum_squares,0,sizeof(sum_squares));
  (void) memset(density_xy,0,2*number_grays*sizeof(*density_xy));
  (void) memset(&entropy_x,0,sizeof(entropy_x));
  (void) memset(&entropy_xy,0,sizeof(entropy_xy));
  (void) memset(&entropy_xy1,0,sizeof(entropy_xy1));
  (void) memset(&entropy_xy2,0,sizeof(entropy_xy2));
  (void) memset(&entropy_y,0,sizeof(entropy_y));
  (void) memset(&variance,0,sizeof(variance));
  for (i=0; i < (ssize_t) number_grays; i++)
  {
    cooccurrence[i]=(FeatureStatistics *) AcquireQuantumMemory(number_grays,
      sizeof(**cooccurrence));
    Q[i]=(FeatureStatistics *) AcquireQuantumMemory(number_grays,sizeof(**Q));
    if ((cooccurrence[i] == (FeatureStatistics *) NULL) ||
        (Q[i] == (FeatureStatistics *) NULL))
      break;
    (void) memset(cooccurrence[i],0,number_grays*
      sizeof(**cooccurrence));
    (void) memset(Q[i],0,number_grays*sizeof(**Q));
  }
  if (i < (ssize_t) number_grays)
    {
      for (i--; i >= 0; i--)
      {
        if (Q[i] != (FeatureStatistics *) NULL)
          Q[i]=(FeatureStatistics *) RelinquishMagickMemory(Q[i]);
        if (cooccurrence[i] != (FeatureStatistics *) NULL)
          cooccurrence[i]=(FeatureStatistics *)
            RelinquishMagickMemory(cooccurrence[i]);
      }
      Q=(FeatureStatistics **) RelinquishMagickMemory(Q);
      cooccurrence=(FeatureStatistics **) RelinquishMagickMemory(cooccurrence);
      sum=(FeatureStatistics *) RelinquishMagickMemory(sum);
      density_y=(FeatureStatistics *) RelinquishMagickMemory(density_y);
      density_xy=(FeatureStatistics *) RelinquishMagickMemory(density_xy);
      density_x=(FeatureStatistics *) RelinquishMagickMemory(density_x);
      grays=(PixelPacket *) RelinquishMagickMemory(grays);
      channel_features=(ChannelFeatures *) RelinquishMagickMemory(
        channel_features);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(channel_features);
    }
  /*
    Initialize spatial dependence matrix.
  */
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);
  for (r=0; r < (ssize_t) image->rows; r++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    ssize_t
      offset,
      u,
      v;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-(ssize_t) distance,r,image->columns+
      2*distance,distance+2,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    p+=(ptrdiff_t) distance*GetPixelChannels(image);;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      for (i=0; i < 4; i++)
      {
        offset=GetFeatureAdjacencyOffset(image,distance,i);
        u=0;
        v=0;
        while (grays[u].red != ScaleQuantumToMap(GetPixelRed(image,p)))
          u++;
        while (grays[v].red != ScaleQuantumToMap(GetPixelRed(image,p+offset*(ssize_t) GetPixelChannels(image))))
          v++;
        cooccurrence[u][v].direction[i].red++;
        cooccurrence[v][u].direction[i].red++;
        u=0;
        v=0;
        while (grays[u].green != ScaleQuantumToMap(GetPixelGreen(image,p)))
          u++;
        while (grays[v].green != ScaleQuantumToMap(GetPixelGreen(image,p+offset*(ssize_t) GetPixelChannels(image))))
          v++;
        cooccurrence[u][v].direction[i].green++;
        cooccurrence[v][u].direction[i].green++;
        u=0;
        v=0;
        while (grays[u].blue != ScaleQuantumToMap(GetPixelBlue(image,p)))
          u++;
        while (grays[v].blue != ScaleQuantumToMap(GetPixelBlue(image,p+offset*(ssize_t) GetPixelChannels(image))))
          v++;
        cooccurrence[u][v].direction[i].blue++;
        cooccurrence[v][u].direction[i].blue++;
        if (image->colorspace == CMYKColorspace)
          {
            u=0;
            v=0;
            while (grays[u].black != ScaleQuantumToMap(GetPixelBlack(image,p)))
              u++;
            while (grays[v].black != ScaleQuantumToMap(GetPixelBlack(image,p+offset*(ssize_t) GetPixelChannels(image))))
              v++;
            cooccurrence[u][v].direction[i].black++;
            cooccurrence[v][u].direction[i].black++;
          }
        if (image->alpha_trait != UndefinedPixelTrait)
          {
            u=0;
            v=0;
            while (grays[u].alpha != ScaleQuantumToMap(GetPixelAlpha(image,p)))
              u++;
            while (grays[v].alpha != ScaleQuantumToMap(GetPixelAlpha(image,p+offset*(ssize_t) GetPixelChannels(image))))
              v++;
            cooccurrence[u][v].direction[i].alpha++;
            cooccurrence[v][u].direction[i].alpha++;
          }
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  grays=(PixelPacket *) RelinquishMagickMemory(grays);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    {
      for (i=0; i < (ssize_t) number_grays; i++)
        cooccurrence[i]=(FeatureStatistics *)
          RelinquishMagickMemory(cooccurrence[i]);
      cooccurrence=(FeatureStatistics **) RelinquishMagickMemory(cooccurrence);
      channel_features=(ChannelFeatures *) RelinquishMagickMemory(
        channel_features);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(channel_features);
    }
  /*
    Normalize spatial dependence matrix.
  */
  for (i=0; i < 4; i++)
  {
    double
      normalize;

    ssize_t
      y;

    normalize=GetFeatureNormalization(image,distance,i);
    normalize=MagickSafeReciprocal(normalize);
    for (y=0; y < (ssize_t) number_grays; y++)
    {
      ssize_t
        x;

      for (x=0; x < (ssize_t) number_grays; x++)
      {
        cooccurrence[x][y].direction[i].red*=normalize;
        cooccurrence[x][y].direction[i].green*=normalize;
        cooccurrence[x][y].direction[i].blue*=normalize;
        if (image->colorspace == CMYKColorspace)
          cooccurrence[x][y].direction[i].black*=normalize;
        if (image->alpha_trait != UndefinedPixelTrait)
          cooccurrence[x][y].direction[i].alpha*=normalize;
      }
    }
  }
  /*
    Compute texture features.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,number_grays,1)
#endif
  for (i=0; i < 4; i++)
  {
    const TextureDirection
      texture = { image, channel_features, cooccurrence, density_x,
        density_xy, density_y, Q, sum, number_grays, i };

    const TextureSlots
      slots = { &correlation.direction[i], &entropy_x.direction[i],
        &entropy_xy.direction[i], &entropy_xy1.direction[i],
        &entropy_xy2.direction[i], &entropy_y.direction[i],
        &mean.direction[i], &sum_squares.direction[i],
        &variance.direction[i] };

    ComputeTextureFeatures(&texture,&slots);
  }
  /*
    Compute more texture features.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,number_grays,1)
#endif
  for (i=0; i < 4; i++)
  {
    const TextureDirection
      texture = { image, channel_features, cooccurrence, density_x,
        density_xy, density_y, Q, sum, number_grays, i };

    ComputeSumFeatures(&texture);
  }
  /*
    Compute more texture features.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,number_grays,1)
#endif
  for (i=0; i < 4; i++)
  {
    const TextureDirection
      texture = { image, channel_features, cooccurrence, density_x,
        density_xy, density_y, Q, sum, number_grays, i };

    const TextureSlots
      slots = { &correlation.direction[i], &entropy_x.direction[i],
        &entropy_xy.direction[i], &entropy_xy1.direction[i],
        &entropy_xy2.direction[i], &entropy_y.direction[i],
        &mean.direction[i], &sum_squares.direction[i],
        &variance.direction[i] };

    ComputeVarianceFeatures(&texture,&slots);
  }
  /*
    Compute more texture features.
  */
  (void) memset(&variance,0,sizeof(variance));
  (void) memset(&sum_squares,0,sizeof(sum_squares));
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,number_grays,1)
#endif
  for (i=0; i < 4; i++)
  {
    const TextureDirection
      texture = { image, channel_features, cooccurrence, density_x,
        density_xy, density_y, Q, sum, number_grays, i };

    const TextureSlots
      slots = { &correlation.direction[i], &entropy_x.direction[i],
        &entropy_xy.direction[i], &entropy_xy1.direction[i],
        &entropy_xy2.direction[i], &entropy_y.direction[i],
        &mean.direction[i], &sum_squares.direction[i],
        &variance.direction[i] };

    ComputeDifferenceFeatures(&texture,&slots);
  }
  /*
    Compute more texture features.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,number_grays,1)
#endif
  for (i=0; i < 4; i++)
  {
    const TextureDirection
      texture = { image, channel_features, cooccurrence, density_x,
        density_xy, density_y, Q, sum, number_grays, i };

    ssize_t
      z;

    for (z=0; z < (ssize_t) number_grays; z++)
    {
      ssize_t
        y;

      FeatureStatistics
        pixel;

      (void) memset(&pixel,0,sizeof(pixel));
      for (y=0; y < (ssize_t) number_grays; y++)
      {
        AccumulateContrastRow(&texture,&pixel.direction[i],y,z);
      }
      channel_features[RedPixelChannel].contrast[i]+=z*z*
        pixel.direction[i].red;
      channel_features[GreenPixelChannel].contrast[i]+=z*z*
        pixel.direction[i].green;
      channel_features[BluePixelChannel].contrast[i]+=z*z*
        pixel.direction[i].blue;
      if (image->colorspace == CMYKColorspace)
        channel_features[BlackPixelChannel].contrast[i]+=z*z*
          pixel.direction[i].black;
      if (image->alpha_trait != UndefinedPixelTrait)
        channel_features[AlphaPixelChannel].contrast[i]+=z*z*
          pixel.direction[i].alpha;
    }
    SetMaximumCorrelation(&texture);
  }
  /*
    Relinquish resources.
  */
  sum=(FeatureStatistics *) RelinquishMagickMemory(sum);
  for (i=0; i < (ssize_t) number_grays; i++)
    Q[i]=(FeatureStatistics *) RelinquishMagickMemory(Q[i]);
  Q=(FeatureStatistics **) RelinquishMagickMemory(Q);
  density_y=(FeatureStatistics *) RelinquishMagickMemory(density_y);
  density_xy=(FeatureStatistics *) RelinquishMagickMemory(density_xy);
  density_x=(FeatureStatistics *) RelinquishMagickMemory(density_x);
  for (i=0; i < (ssize_t) number_grays; i++)
    cooccurrence[i]=(FeatureStatistics *)
      RelinquishMagickMemory(cooccurrence[i]);
  cooccurrence=(FeatureStatistics **) RelinquishMagickMemory(cooccurrence);
  return(channel_features);
}
