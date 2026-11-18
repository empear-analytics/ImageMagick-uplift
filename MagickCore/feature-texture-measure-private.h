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

  MagickCore private methods of feature-texture.c: the co-occurrence
  statistics and the first texture measures of GetImageFeatures, from its doc
  banner and the FeatureStatistics typedef to SetFirstMeasureOfCorrelation;
  included by that file only.
*/
#ifndef MAGICKCORE_FEATURE_TEXTURE_MEASURE_PRIVATE_H
#define MAGICKCORE_FEATURE_TEXTURE_MEASURE_PRIVATE_H

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

typedef struct _FeatureBuffers
{
  FeatureStatistics
    **cooccurrence,
    *density_x,
    *density_xy,
    *density_y,
    **Q,
    *sum;
} FeatureBuffers;

static inline MagickBooleanType IsFeatureMemoryMissing(
  const FeatureBuffers *buffers)
{
  FeatureStatistics
    **const cooccurrence = buffers->cooccurrence,
    *const density_x = buffers->density_x,
    *const density_xy = buffers->density_xy,
    *const density_y = buffers->density_y,
    **const Q = buffers->Q,
    *const sum = buffers->sum;

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

static inline MagickBooleanType IsFeatureBlackGray(const Image *image,
  const PixelPacket *grays,const ssize_t i)
{
  return(((image->colorspace == CMYKColorspace) && (grays[i].black != ~0U)) ?
    MagickTrue : MagickFalse);
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
    if (IsFeatureBlackGray(image,grays,i) != MagickFalse)
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

static void AccumulateAngularSecondMoment(const TextureDirection *texture,
  const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence;

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
}

static void AccumulateMomentAndCorrelation(const TextureDirection *texture,
  PixelInfo *correlation,const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const sum = texture->sum;

  const ssize_t
    i = texture->i;

  AccumulateAngularSecondMoment(texture,x,y);
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

static void AccumulateInverseDifferenceMoment(
  const TextureDirection *texture,const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence;

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
}

static void AccumulateInverseDifferenceAndSum(const TextureDirection *texture,
  const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_xy = texture->density_xy;

  const ssize_t
    i = texture->i;

  AccumulateInverseDifferenceMoment(texture,x,y);
  /*
    Sum average.
  */
  AddFeatureStatistics(image,&density_xy[y+x+2].direction[i],
    &cooccurrence[x][y].direction[i]);
}

static void AccumulateCooccurrenceEntropy(const TextureDirection *texture,
  const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence;

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
}

static void AccumulateEntropyAndDensities(const TextureDirection *texture,
  const ssize_t x,const ssize_t y)
{
  const Image
    *const image = texture->image;

  FeatureStatistics
    **const cooccurrence = texture->cooccurrence,
    *const density_x = texture->density_x,
    *const density_y = texture->density_y;

  const ssize_t
    i = texture->i;

  AccumulateCooccurrenceEntropy(texture,x,y);
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

static void AccumulateSumAverage(const TextureDirection *texture,
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
}

static void ComputeSumFeatures(const TextureDirection *texture)
{
  const unsigned int
    number_grays = texture->number_grays;

  const ssize_t
    i = texture->i;

  ssize_t
    x;

  for (x=2; x < (ssize_t) (2*number_grays); x++)
  {
    AccumulateSumAverage(texture,x);
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

static void SetVarianceSumOfSquares(const TextureDirection *texture,
  const TextureSlots *slots)
{
  const PixelInfo
    *const variance = slots->variance;

  const Image
    *const image = texture->image;

  ChannelFeatures
    *const channel_features = texture->channel_features;

  const ssize_t
    i = texture->i;

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

static void ComputeVarianceFeatures(const TextureDirection *texture,
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
      AccumulateVarianceAndDifference(texture,slots,x,y);
      AccumulateCorrelationEntropies(texture,slots,x,y);
    }
  }
  SetVarianceSumOfSquares(texture,slots);
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

#endif
