/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%              EEEEE  N   N  H   H   AAA   N   N   CCCC  EEEEE                %
%              E      NN  N  H   H  A   A  NN  N  C      E                    %
%              EEE    N N N  HHHHH  AAAAA  N N N  C      EEE                  %
%              E      N  NN  H   H  A   A  N  NN  C      E                    %
%              EEEEE  N   N  H   H  A   A  N   N   CCCC  EEEEE                %
%                                                                             %
%                                                                             %
%                    MagickCore Image Enhancement Methods                     %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/token.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
  Typedef declarations.
*/
typedef struct _ModulateInfo
{
  ColorspaceType
    colorspace;

  double
    percent_hue,
    percent_saturation,
    percent_brightness;

  IlluminantType
    illuminant;
} ModulateInfo;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A u t o G a m m a I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AutoGammaImage() extract the 'mean' from the image and adjust the image
%  to try make set its gamma appropriately.
%
%  The format of the AutoGammaImage method is:
%
%      MagickBooleanType AutoGammaImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-level
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType AutoGammaImage(Image *image,
  ExceptionInfo *exception)
{
  double
    gamma,
    log_mean,
    mean,
    sans;

  MagickStatusType
    status;

  ssize_t
    i;

  log_mean=log(0.5);
  if (image->channel_mask == AllChannels)
    {
      /*
        Apply gamma correction equally across all given channels.
      */
      (void) GetImageMean(image,&mean,&sans,exception);
      gamma=log(mean*QuantumScale)/log_mean;
      return(LevelImage(image,0.0,(double) QuantumRange,gamma,exception));
    }
  /*
    Auto-gamma each channel separately.
  */
  status=MagickTrue;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ChannelType
      channel_mask;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    channel_mask=SetImageChannelMask(image,(ChannelType) (1UL << i));
    status=GetImageMean(image,&mean,&sans,exception);
    gamma=log(mean*QuantumScale)/log_mean;
    status&=(MagickStatusType) LevelImage(image,0.0,(double) QuantumRange,gamma,
      exception);
    (void) SetImageChannelMask(image,channel_mask);
    if (status == MagickFalse)
      break;
  }
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A u t o L e v e l I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AutoLevelImage() adjusts the levels of a particular image channel by
%  scaling the minimum and maximum values to the full quantum range.
%
%  The format of the LevelImage method is:
%
%      MagickBooleanType AutoLevelImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-level
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType AutoLevelImage(Image *image,
  ExceptionInfo *exception)
{
  return(MinMaxStretchImage(image,0.0,0.0,1.0,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     B r i g h t n e s s C o n t r a s t I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BrightnessContrastImage() changes the brightness and/or contrast of an
%  image.  It converts the brightness and contrast parameters into slope and
%  intercept and calls a polynomial function to apply to the image.
%
%  The format of the BrightnessContrastImage method is:
%
%      MagickBooleanType BrightnessContrastImage(Image *image,
%        const double brightness,const double contrast,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o brightness: the brightness percent (-100 .. 100).
%
%    o contrast: the contrast percent (-100 .. 100).
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType BrightnessContrastImage(Image *image,
  const double brightness,const double contrast,ExceptionInfo *exception)
{
#define BrightnessContrastImageTag  "BrightnessContrast/Image"

  double
    coefficients[2],
    intercept,
    slope;

  MagickBooleanType
    status;

  /*
    Compute slope and intercept.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  slope=100.0*MagickSafeReciprocal(100.0-contrast);
  if (contrast < 0.0)
    slope=0.01*contrast+1.0;
  intercept=(0.01*brightness-0.5)*slope+0.5;
  coefficients[0]=slope;
  coefficients[1]=intercept;
  status=FunctionImage(image,PolynomialFunction,2,coefficients,exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C L A H E I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CLAHEImage() is a variant of adaptive histogram equalization in which the
%  contrast amplification is limited, so as to reduce this problem of noise
%  amplification.
%
%  Adapted from implementation by Karel Zuiderveld, karel@cv.ruu.nl in
%  "Graphics Gems IV", Academic Press, 1994.
%
%  The format of the CLAHEImage method is:
%
%      MagickBooleanType CLAHEImage(Image *image,const size_t width,
%        const size_t height,const size_t number_bins,const double clip_limit,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o width: the width of the tile divisions to use in horizontal direction.
%
%    o height: the height of the tile divisions to use in vertical direction.
%
%    o number_bins: number of bins for histogram ("dynamic range").
%
%    o clip_limit: contrast limit for localised changes in contrast. A limit
%      less than 1 results in standard non-contrast limited AHE.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _RangeInfo
{
  unsigned short
    min,
    max;
} RangeInfo;

static ssize_t RedistributeCLAHEExcess(const double clip_limit,
  const size_t number_bins,size_t *histogram,ssize_t cumulative_excess)
{
  size_t
    *p;

  size_t
    *q;

  ssize_t
    step;

  p=histogram;
  q=histogram+number_bins;
  while ((cumulative_excess != 0) && (p < q))
  {
    step=(ssize_t) number_bins/cumulative_excess;
    if (step < 1)
      step=1;
    for (p=histogram; (p < q) && (cumulative_excess != 0); p+=(ptrdiff_t) step)
      if ((double) *p < clip_limit)
        {
          (*p)++;
          cumulative_excess--;
        }
    p++;
  }
  return(cumulative_excess);
}

static ssize_t GetCLAHEExcess(const double clip_limit,
  const size_t number_bins,const size_t *histogram)
{
  ssize_t
    cumulative_excess,
    i;

  cumulative_excess=0;
  for (i=0; i < (ssize_t) number_bins; i++)
    if (histogram[i] > clip_limit)
      cumulative_excess+=(ssize_t) (histogram[i]-clip_limit);
  return(cumulative_excess);
}

static void ClipCLAHEHistogram(const double clip_limit,const size_t number_bins,
  size_t *histogram)
{
#define NumberCLAHEGrays  (65536)

  ssize_t
    cumulative_excess,
    excess,
    i,
    previous_excess,
    step;

  /*
    Compute total number of excess pixels.
  */
  if (number_bins == 0)
    return;
  cumulative_excess=GetCLAHEExcess(clip_limit,number_bins,histogram);
  /*
    Clip histogram and redistribute excess pixels across all bins.
  */
  step=cumulative_excess/(ssize_t) number_bins;
  excess=(ssize_t) (clip_limit-step);
  for (i=0; i < (ssize_t) number_bins; i++)
  {
    if ((double) histogram[i] > clip_limit)
      histogram[i]=(size_t) clip_limit;
    else
      if ((ssize_t) histogram[i] > excess)
        {
          cumulative_excess-=(ssize_t) histogram[i]-excess;
          histogram[i]=(size_t) clip_limit;
        }
      else
        {
          cumulative_excess-=step;
          histogram[i]+=(size_t) step;
        }
  }
  /*
    Redistribute remaining excess.
  */
  do
  {
    previous_excess=cumulative_excess;
    cumulative_excess=RedistributeCLAHEExcess(clip_limit,number_bins,
      histogram,cumulative_excess);
  } while ((cumulative_excess != 0) && (cumulative_excess < previous_excess));
}

static void GenerateCLAHEHistogram(const RectangleInfo *clahe_info,
  const RectangleInfo *tile_info,const size_t number_bins,
  const unsigned short *lut,const unsigned short *pixels,size_t *histogram)
{
  const unsigned short
    *p;

  ssize_t
    i;

  /*
    Classify the pixels into a gray histogram.
  */
  for (i=0; i < (ssize_t) number_bins; i++)
    histogram[i]=0L;
  p=pixels;
  for (i=0; i < (ssize_t) tile_info->height; i++)
  {
    const unsigned short
      *q;

    q=p+tile_info->width;
    while (p < q)
      histogram[lut[*p++]]++;
    q+=(ptrdiff_t) clahe_info->width;
    p=q-tile_info->width;
  }
}

static void InterpolateCLAHE(const RectangleInfo *clahe_info,const size_t *Q12,
  const size_t *Q22,const size_t *Q11,const size_t *Q21,
  const RectangleInfo *tile,const unsigned short *lut,unsigned short *pixels)
{
  ssize_t
    y;

  unsigned short
    intensity;

  /*
    Bilinear interpolate four tiles to eliminate boundary artifacts.
  */
  for (y=(ssize_t) tile->height; y > 0; y--)
  {
    ssize_t
      x;

    for (x=(ssize_t) tile->width; x > 0; x--)
    {
      intensity=lut[*pixels];
      *pixels++=(unsigned short) (MagickSafeReciprocal((double) tile->width*
        tile->height)*(y*((double) x*Q12[intensity]+((double) tile->width-x)*
        Q22[intensity])+((double) tile->height-y)*((double) x*Q11[intensity]+
        ((double) tile->width-x)*Q21[intensity])));
    }
    pixels+=(clahe_info->width-tile->width);
  }
}

static void GenerateCLAHELut(const RangeInfo *range_info,
  const size_t number_bins,unsigned short *lut)
{
  ssize_t
    i;

  unsigned short
    delta;

  /*
    Scale input image [intensity min,max] to [0,number_bins-1].
  */
  delta=(unsigned short) ((range_info->max-range_info->min)/number_bins+1);
  for (i=(ssize_t) range_info->min; i <= (ssize_t) range_info->max; i++)
    lut[i]=(unsigned short) ((i-range_info->min)/delta);
}

static void MapCLAHEHistogram(const RangeInfo *range_info,
  const size_t number_bins,const size_t number_pixels,size_t *histogram)
{
  double
    scale,
    sum;

  ssize_t
    i;

  /*
    Rescale histogram to range [min-intensity .. max-intensity].
  */
  scale=(double) (range_info->max-range_info->min)/number_pixels;
  sum=0.0;
  for (i=0; i < (ssize_t) number_bins; i++)
  {
    sum+=histogram[i];
    histogram[i]=(size_t) (range_info->min+scale*sum);
    if (histogram[i] > range_info->max)
      histogram[i]=range_info->max;
  }
}

static MagickBooleanType CLAHE(const RectangleInfo *clahe_info,
  const RectangleInfo *tile_info,const RangeInfo *range_info,
  const size_t number_bins,const double clip_limit,unsigned short *pixels)
{
  MemoryInfo
    *tile_cache;

  size_t
    limit,
    *tiles;

  ssize_t
    y;

  unsigned short
    *lut,
    *p;

  /*
    Contrast limited adapted histogram equalization.
  */
  if (clip_limit == 1.0)
    return(MagickTrue);
  tile_cache=AcquireVirtualMemory((size_t) clahe_info->x*number_bins,(size_t)
    clahe_info->y*sizeof(*tiles));
  if (tile_cache == (MemoryInfo *) NULL)
    return(MagickFalse);
  lut=(unsigned short *) AcquireQuantumMemory(NumberCLAHEGrays,sizeof(*lut));
  if (lut == (unsigned short *) NULL)
    {
      tile_cache=RelinquishVirtualMemory(tile_cache);
      return(MagickFalse);
    }
  tiles=(size_t *) GetVirtualMemoryBlob(tile_cache);
  limit=(size_t) (clip_limit*((double) tile_info->width*tile_info->height)/
    number_bins);
  if (limit < 1UL)
    limit=1UL;
  /*
    Generate greylevel mappings for each tile.
  */
  GenerateCLAHELut(range_info,number_bins,lut);
  p=pixels;
  for (y=0; y < (ssize_t) clahe_info->y; y++)
  {
    ssize_t
      x;

    for (x=0; x < (ssize_t) clahe_info->x; x++)
    {
      size_t
        *histogram;

      histogram=tiles+((ssize_t) number_bins*(y*clahe_info->x+x));
      GenerateCLAHEHistogram(clahe_info,tile_info,number_bins,lut,p,histogram);
      ClipCLAHEHistogram((double) limit,number_bins,histogram);
      MapCLAHEHistogram(range_info,number_bins,tile_info->width*
        tile_info->height,histogram);
      p+=(ptrdiff_t) tile_info->width;
    }
    p+=CastDoubleToPtrdiffT((double) clahe_info->width*(tile_info->height-1));
  }
  /*
    Interpolate greylevel mappings to get CLAHE image.
  */
  p=pixels;
  for (y=0; y <= (ssize_t) clahe_info->y; y++)
  {
    OffsetInfo
      offset;

    RectangleInfo
      tile;

    ssize_t
      x;

    tile.height=tile_info->height;
    tile.y=y-1;
    offset.y=tile.y+1;
    if (y == 0)
      {
        /*
          Top row.
        */
        tile.height=tile_info->height >> 1;
        tile.y=0;
        offset.y=0;
      }
    else
      if (y == (ssize_t) clahe_info->y)
        {
          /*
            Bottom row.
          */
          tile.height=(tile_info->height+1) >> 1;
          tile.y=clahe_info->y-1;
          offset.y=tile.y;
        }
    for (x=0; x <= (ssize_t) clahe_info->x; x++)
    {
      double
        Q11,
        Q12,
        Q21,
        Q22;

      tile.width=tile_info->width;
      tile.x=x-1;
      offset.x=tile.x+1;
      if (x == 0)
        {
          /*
            Left column.
          */
          tile.width=tile_info->width >> 1;
          tile.x=0;
          offset.x=0;
        }
      else
        if (x == (ssize_t) clahe_info->x)
          {
            /*
              Right column.
            */
            tile.width=(tile_info->width+1) >> 1;
            tile.x=clahe_info->x-1;
            offset.x=tile.x;
          }
      Q12=(double) number_bins*(tile.y*clahe_info->x+tile.x);
      Q22=(double) number_bins*(tile.y*clahe_info->x+offset.x);
      Q11=(double) number_bins*(offset.y*clahe_info->x+tile.x);
      Q21=(double) number_bins*(offset.y*clahe_info->x+offset.x);
      InterpolateCLAHE(clahe_info,tiles+CastDoubleToPtrdiffT(Q12),
        tiles+CastDoubleToPtrdiffT(Q22),tiles+CastDoubleToPtrdiffT(Q11),
        tiles+CastDoubleToPtrdiffT(Q21),&tile,lut,p);
      p+=(ptrdiff_t) tile.width;
    }
    p+=CastDoubleToPtrdiffT((double) clahe_info->width*(tile.height-1));
  }
  lut=(unsigned short *) RelinquishMagickMemory(lut);
  tile_cache=RelinquishVirtualMemory(tile_cache);
  return(MagickTrue);
}

static RectangleInfo GetCLAHETileInfo(const Image *image,const size_t width,
  const size_t height)
{
  RectangleInfo
    tile_info;

  tile_info.width=width;
  if (tile_info.width == 0)
    tile_info.width=image->columns >> 3;
  if (tile_info.width < 2)
    tile_info.width=2;
  tile_info.height=height;
  if (tile_info.height == 0)
    tile_info.height=image->rows >> 3;
  if (tile_info.height < 2)
    tile_info.height=2;
  tile_info.x=0;
  if ((image->columns % tile_info.width) != 0)
    tile_info.x=(ssize_t) (tile_info.width-(image->columns % tile_info.width));
  tile_info.y=0;
  if ((image->rows % tile_info.height) != 0)
    tile_info.y=(ssize_t) (tile_info.height-(image->rows % tile_info.height));
  return(tile_info);
}

MagickExport MagickBooleanType CLAHEImage(Image *image,const size_t width,
  const size_t height,const size_t number_bins,const double clip_limit,
  ExceptionInfo *exception)
{
#define CLAHEImageTag  "CLAHE/Image"

  CacheView
    *image_view;

  ColorspaceType
    colorspace;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MemoryInfo
    *pixel_cache;

  RangeInfo
    range_info;

  RectangleInfo
    clahe_info,
    tile_info;

  size_t
    n;

  ssize_t
    y;

  unsigned short
    *pixels;

  /*
    Configure CLAHE parameters.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  range_info.min=0;
  range_info.max=NumberCLAHEGrays-1;
  tile_info=GetCLAHETileInfo(image,width,height);
  clahe_info.width=(size_t) ((ssize_t) image->columns+tile_info.x);
  clahe_info.height=(size_t) ((ssize_t) image->rows+tile_info.y);
  clahe_info.x=(ssize_t) (clahe_info.width/tile_info.width);
  clahe_info.y=(ssize_t) (clahe_info.height/tile_info.height);
  pixel_cache=AcquireVirtualMemory(clahe_info.width,clahe_info.height*
    sizeof(*pixels));
  if (pixel_cache == (MemoryInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  pixels=(unsigned short *) GetVirtualMemoryBlob(pixel_cache);
  colorspace=image->colorspace;
  if (TransformImageColorspace(image,LabColorspace,exception) == MagickFalse)
    {
      pixel_cache=RelinquishVirtualMemory(pixel_cache);
      return(MagickFalse);
    }
  /*
    Initialize CLAHE pixels.
  */
  image_view=AcquireVirtualCacheView(image,exception);
  progress=0;
  status=MagickTrue;
  n=0;
  for (y=0; y < (ssize_t) clahe_info.height; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-(tile_info.x >> 1),y-
      (tile_info.y >> 1),clahe_info.width,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) clahe_info.width; x++)
    {
      pixels[n++]=ScaleQuantumToShort(p[0]);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

        progress++;
        proceed=SetImageProgress(image,CLAHEImageTag,progress,2*
          GetPixelChannels(image));
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  status=CLAHE(&clahe_info,&tile_info,&range_info,number_bins == 0 ?
    (size_t) 128 : MagickMin(number_bins,256),clip_limit,pixels);
  if (status == MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
  /*
    Push CLAHE pixels to CLAHE image.
  */
  image_view=AcquireAuthenticCacheView(image,exception);
  n=clahe_info.width*(size_t) (tile_info.y/2);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    n+=(size_t) (tile_info.x/2);
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      q[0]=ScaleShortToQuantum(pixels[n++]);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    n+=(size_t) ((ssize_t) clahe_info.width-(ssize_t) image->columns-
      (tile_info.x/2));
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

        progress++;
        proceed=SetImageProgress(image,CLAHEImageTag,progress,2*
          GetPixelChannels(image));
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  pixel_cache=RelinquishVirtualMemory(pixel_cache);
  if (TransformImageColorspace(image,colorspace,exception) == MagickFalse)
    status=MagickFalse;
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n t r a s t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ContrastImage() enhances the intensity differences between the lighter and
%  darker elements of the image.  Set sharpen to a MagickTrue to increase the
%  image contrast otherwise the contrast is reduced.
%
%  The format of the ContrastImage method is:
%
%      MagickBooleanType ContrastImage(Image *image,
%        const MagickBooleanType sharpen,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o sharpen: Increase or decrease image contrast.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void Contrast(const int sign,double *red,double *green,
  double *blue)
{
  double
    brightness = 0.0,
    hue = 0.0,
    saturation = 0.0;

  /*
    Enhance contrast: dark color become darker, light color become lighter.
  */
  ConvertRGBToHSB(*red,*green,*blue,&hue,&saturation,&brightness);
  brightness+=0.5*sign*(0.5*(sin((double) (MagickPI*(brightness-0.5)))+1.0)-
    brightness);
  if (brightness > 1.0)
    brightness=1.0;
  else
    if (brightness < 0.0)
      brightness=0.0;
  ConvertHSBToRGB(hue,saturation,brightness,red,green,blue);
}

static void ContrastImageRow(const Image *image,const int sign,Quantum *q)
{
  double
    blue,
    green,
    red;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    red=(double) GetPixelRed(image,q);
    green=(double) GetPixelGreen(image,q);
    blue=(double) GetPixelBlue(image,q);
    Contrast(sign,&red,&green,&blue);
    SetPixelRed(image,ClampToQuantum(red),q);
    SetPixelGreen(image,ClampToQuantum(green),q);
    SetPixelBlue(image,ClampToQuantum(blue),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ContrastColormap(Image *image,const int sign)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    double
      blue,
      green,
      red;

    red=(double) image->colormap[i].red;
    green=(double) image->colormap[i].green;
    blue=(double) image->colormap[i].blue;
    Contrast(sign,&red,&green,&blue);
    image->colormap[i].red=(MagickRealType) red;
    image->colormap[i].green=(MagickRealType) green;
    image->colormap[i].blue=(MagickRealType) blue;
  }
}

MagickExport MagickBooleanType ContrastImage(Image *image,
  const MagickBooleanType sharpen,ExceptionInfo *exception)
{
#define ContrastImageTag  "Contrast/Image"

  CacheView
    *image_view;

  int
    sign;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateContrastImage(image,sharpen,exception) != MagickFalse)
    return(MagickTrue);
#endif
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  sign=sharpen != MagickFalse ? 1 : -1;
  /*
    Contrast enhance colormap.
  */
  if (image->storage_class == PseudoClass)
    ContrastColormap(image,sign);
  /*
    Contrast enhance image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    ContrastImageRow(image,sign,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ContrastImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E n h a n c e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EnhanceImage() applies a digital filter that improves the quality of a
%  noisy image.
%
%  The format of the EnhanceImage method is:
%
%      Image *EnhanceImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *EnhanceImage(const Image *image,ExceptionInfo *exception)
{
#define EnhanceImageTag  "Enhance/Image"
#define EnhancePixel(weight) \
  mean=QuantumScale*((double) GetPixelRed(image,r)+pixel.red)/2.0; \
  distance=QuantumScale*((double) GetPixelRed(image,r)-pixel.red); \
  distance_squared=(4.0+mean)*distance*distance; \
  mean=QuantumScale*((double) GetPixelGreen(image,r)+pixel.green)/2.0; \
  distance=QuantumScale*((double) GetPixelGreen(image,r)-pixel.green); \
  distance_squared+=(7.0-mean)*distance*distance; \
  mean=QuantumScale*((double) GetPixelBlue(image,r)+pixel.blue)/2.0; \
  distance=QuantumScale*((double) GetPixelBlue(image,r)-pixel.blue); \
  distance_squared+=(5.0-mean)*distance*distance; \
  mean=QuantumScale*((double) GetPixelBlack(image,r)+pixel.black)/2.0; \
  distance=QuantumScale*((double) GetPixelBlack(image,r)-pixel.black); \
  distance_squared+=(5.0-mean)*distance*distance; \
  mean=QuantumScale*((double) GetPixelAlpha(image,r)+pixel.alpha)/2.0; \
  distance=QuantumScale*((double) GetPixelAlpha(image,r)-pixel.alpha); \
  distance_squared+=(5.0-mean)*distance*distance; \
  if (distance_squared < 0.069) \
    { \
      aggregate.red+=(weight)*(double) GetPixelRed(image,r); \
      aggregate.green+=(weight)*(double) GetPixelGreen(image,r); \
      aggregate.blue+=(weight)*(double) GetPixelBlue(image,r); \
      aggregate.black+=(weight)*(double) GetPixelBlack(image,r); \
      aggregate.alpha+=(weight)*(double) GetPixelAlpha(image,r); \
      total_weight+=(weight); \
    } \
  r+=(ptrdiff_t) GetPixelChannels(image);

  CacheView
    *enhance_view,
    *image_view;

  Image
    *enhance_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Initialize enhanced image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  enhance_image=CloneImage(image,0,0,MagickTrue,
    exception);
  if (enhance_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(enhance_image,DirectClass,exception) == MagickFalse)
    {
      enhance_image=DestroyImage(enhance_image);
      return((Image *) NULL);
    }
  /*
    Enhance image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  enhance_view=AcquireAuthenticCacheView(enhance_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,enhance_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    PixelInfo
      pixel;

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    ssize_t
      center;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-2,y-2,image->columns+4,5,exception);
    q=QueueCacheViewAuthenticPixels(enhance_view,0,y,enhance_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    center=(ssize_t) GetPixelChannels(image)*(2*((ssize_t) image->columns+4)+2);
    GetPixelInfo(image,&pixel);
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      double
        distance,
        distance_squared,
        mean,
        total_weight;

      PixelInfo
        aggregate;

      const Quantum
        *magick_restrict r;

      GetPixelInfo(image,&aggregate);
      total_weight=0.0;
      GetPixelInfoPixel(image,p+center,&pixel);
      r=p;
      EnhancePixel(5.0); EnhancePixel(8.0); EnhancePixel(10.0);
        EnhancePixel(8.0); EnhancePixel(5.0);
      r=p+GetPixelChannels(image)*(image->columns+4);
      EnhancePixel(8.0); EnhancePixel(20.0); EnhancePixel(40.0);
        EnhancePixel(20.0); EnhancePixel(8.0);
      r=p+2*GetPixelChannels(image)*(image->columns+4);
      EnhancePixel(10.0); EnhancePixel(40.0); EnhancePixel(80.0);
        EnhancePixel(40.0); EnhancePixel(10.0);
      r=p+3*GetPixelChannels(image)*(image->columns+4);
      EnhancePixel(8.0); EnhancePixel(20.0); EnhancePixel(40.0);
        EnhancePixel(20.0); EnhancePixel(8.0);
      r=p+4*GetPixelChannels(image)*(image->columns+4);
      EnhancePixel(5.0); EnhancePixel(8.0); EnhancePixel(10.0);
        EnhancePixel(8.0); EnhancePixel(5.0);
      if (total_weight > MagickEpsilon)
        {
          pixel.red=((aggregate.red+total_weight/2.0)/total_weight);
          pixel.green=((aggregate.green+total_weight/2.0)/total_weight);
          pixel.blue=((aggregate.blue+total_weight/2.0)/total_weight);
          pixel.black=((aggregate.black+total_weight/2.0)/total_weight);
          pixel.alpha=((aggregate.alpha+total_weight/2.0)/total_weight);
        }
      SetPixelViaPixelInfo(enhance_image,&pixel,q);
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(enhance_image);
    }
    if (SyncCacheViewAuthenticPixels(enhance_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,EnhanceImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  enhance_view=DestroyCacheView(enhance_view);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    enhance_image=DestroyImage(enhance_image);
  return(enhance_image);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L i n e a r S t r e t c h I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LinearStretchImage() discards any pixels below the black point and above
%  the white point and levels the remaining pixels.
%
%  The format of the LinearStretchImage method is:
%
%      MagickBooleanType LinearStretchImage(Image *image,
%        const double black_point,const double white_point,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o black_point: the black point.
%
%    o white_point: the white point.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ssize_t FindLinearBlackLevel(const double *histogram,
  const double black_point)
{
  double
    intensity;

  ssize_t
    black;

  intensity=0.0;
  for (black=0; black < (ssize_t) MaxMap; black++)
  {
    intensity+=histogram[black];
    if (intensity >= black_point)
      break;
  }
  return(black);
}

static ssize_t FindLinearWhiteLevel(const double *histogram,
  const double white_point)
{
  double
    intensity;

  ssize_t
    white;

  intensity=0.0;
  for (white=(ssize_t) MaxMap; white != 0; white--)
  {
    intensity+=histogram[white];
    if (intensity >= white_point)
      break;
  }
  return(white);
}

MagickExport MagickBooleanType LinearStretchImage(Image *image,
  const double black_point,const double white_point,ExceptionInfo *exception)
{
#define LinearStretchImageTag  "LinearStretch/Image"

  CacheView
    *image_view;

  char
    property[MagickPathExtent];

  double
    *histogram,
    intensity;

  MagickBooleanType
    status;

  ssize_t
    black,
    white,
    y;

  /*
    Allocate histogram and linear map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  histogram=(double *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*histogram));
  if (histogram == (double *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Form histogram.
  */
  (void) memset(histogram,0,(MaxMap+1)*sizeof(*histogram));
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      intensity=GetPixelIntensity(image,p);
      histogram[ScaleQuantumToMap(ClampToQuantum(intensity))]++;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  image_view=DestroyCacheView(image_view);
  /*
    Find the histogram boundaries by locating the black and white point levels.
  */
  black=FindLinearBlackLevel(histogram,black_point);
  white=FindLinearWhiteLevel(histogram,white_point);
  histogram=(double *) RelinquishMagickMemory(histogram);
  status=LevelImage(image,(double) ScaleMapToQuantum((MagickRealType) black),
    (double) ScaleMapToQuantum((MagickRealType) white),1.0,exception);
  (void) FormatLocaleString(property,MagickPathExtent,"%gx%g%%",100.0*black/
    MaxMap,100.0*white/MaxMap);
  (void) SetImageProperty(image,"histogram:linear-stretch",property,exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o d u l a t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModulateImage() lets you control the brightness, saturation, and hue
%  of an image.  Modulate represents the brightness, saturation, and hue
%  as one parameter (e.g. 90,150,100).  If the image colorspace is HSL, the
%  modulation is lightness, saturation, and hue.  For HWB, use blackness,
%  whiteness, and hue. And for HCL, use chrome, luma, and hue.
%
%  The format of the ModulateImage method is:
%
%      MagickBooleanType ModulateImage(Image *image,const char *modulate,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o modulate: Define the percent change in brightness, saturation, and hue.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void ModulateHCL(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_hue = modulate_info->percent_hue,
    percent_chroma = modulate_info->percent_saturation,
    percent_luma = modulate_info->percent_brightness;

  double
    hue,
    luma,
    chroma;

  /*
    Increase or decrease color luma, chroma, or hue.
  */
  ConvertRGBToHCL(*red,*green,*blue,&hue,&chroma,&luma);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  chroma*=0.01*percent_chroma;
  luma*=0.01*percent_luma;
  ConvertHCLToRGB(hue,chroma,luma,red,green,blue);
}

static inline void ModulateHCLp(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_hue = modulate_info->percent_hue,
    percent_chroma = modulate_info->percent_saturation,
    percent_luma = modulate_info->percent_brightness;

  double
    hue,
    luma,
    chroma;

  /*
    Increase or decrease color luma, chroma, or hue.
  */
  ConvertRGBToHCLp(*red,*green,*blue,&hue,&chroma,&luma);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  chroma*=0.01*percent_chroma;
  luma*=0.01*percent_luma;
  ConvertHCLpToRGB(hue,chroma,luma,red,green,blue);
}

static inline void ModulateHSB(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_hue = modulate_info->percent_hue,
    percent_saturation = modulate_info->percent_saturation,
    percent_brightness = modulate_info->percent_brightness;

  double
    brightness,
    hue,
    saturation;

  /*
    Increase or decrease color brightness, saturation, or hue.
  */
  ConvertRGBToHSB(*red,*green,*blue,&hue,&saturation,&brightness);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  saturation*=0.01*percent_saturation;
  brightness*=0.01*percent_brightness;
  ConvertHSBToRGB(hue,saturation,brightness,red,green,blue);
}

static inline void ModulateHSI(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_hue = modulate_info->percent_hue,
    percent_saturation = modulate_info->percent_saturation,
    percent_intensity = modulate_info->percent_brightness;

  double
    intensity,
    hue,
    saturation;

  /*
    Increase or decrease color intensity, saturation, or hue.
  */
  ConvertRGBToHSI(*red,*green,*blue,&hue,&saturation,&intensity);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  saturation*=0.01*percent_saturation;
  intensity*=0.01*percent_intensity;
  ConvertHSIToRGB(hue,saturation,intensity,red,green,blue);
}

static inline void ModulateHSL(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_hue = modulate_info->percent_hue,
    percent_saturation = modulate_info->percent_saturation,
    percent_lightness = modulate_info->percent_brightness;

  double
    hue,
    lightness,
    saturation;

  /*
    Increase or decrease color lightness, saturation, or hue.
  */
  ConvertRGBToHSL(*red,*green,*blue,&hue,&saturation,&lightness);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  saturation*=0.01*percent_saturation;
  lightness*=0.01*percent_lightness;
  ConvertHSLToRGB(hue,saturation,lightness,red,green,blue);
}

static inline void ModulateHSV(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_hue = modulate_info->percent_hue,
    percent_saturation = modulate_info->percent_saturation,
    percent_value = modulate_info->percent_brightness;

  double
    hue,
    saturation,
    value;

  /*
    Increase or decrease color value, saturation, or hue.
  */
  ConvertRGBToHSV(*red,*green,*blue,&hue,&saturation,&value);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  saturation*=0.01*percent_saturation;
  value*=0.01*percent_value;
  ConvertHSVToRGB(hue,saturation,value,red,green,blue);
}

static inline void ModulateHWB(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_hue = modulate_info->percent_hue,
    percent_whiteness = modulate_info->percent_saturation,
    percent_blackness = modulate_info->percent_brightness;

  double
    blackness,
    hue,
    whiteness;

  /*
    Increase or decrease color blackness, whiteness, or hue.
  */
  ConvertRGBToHWB(*red,*green,*blue,&hue,&whiteness,&blackness);
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  blackness*=0.01*percent_blackness;
  whiteness*=0.01*percent_whiteness;
  ConvertHWBToRGB(hue,whiteness,blackness,red,green,blue);
}

static inline void ModulateLCHab(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_luma = modulate_info->percent_brightness,
    percent_chroma = modulate_info->percent_saturation,
    percent_hue = modulate_info->percent_hue;

  const IlluminantType
    illuminant = modulate_info->illuminant;

  double
    hue,
    luma,
    chroma;

  /*
    Increase or decrease color luma, chroma, or hue.
  */
  ConvertRGBToLCHab(*red,*green,*blue,illuminant,&luma,&chroma,&hue);
  luma*=0.01*percent_luma;
  chroma*=0.01*percent_chroma;
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  ConvertLCHabToRGB(luma,chroma,hue,illuminant,red,green,blue);
}

static inline void ModulateLCHuv(const ModulateInfo *modulate_info,
  double *red,double *green,double *blue)
{
  const double
    percent_luma = modulate_info->percent_brightness,
    percent_chroma = modulate_info->percent_saturation,
    percent_hue = modulate_info->percent_hue;

  const IlluminantType
    illuminant = modulate_info->illuminant;

  double
    hue,
    luma,
    chroma;

  /*
    Increase or decrease color luma, chroma, or hue.
  */
  ConvertRGBToLCHuv(*red,*green,*blue,illuminant,&luma,&chroma,&hue);
  luma*=0.01*percent_luma;
  chroma*=0.01*percent_chroma;
  hue+=fmod((percent_hue-100.0),200.0)/200.0;
  ConvertLCHuvToRGB(luma,chroma,hue,illuminant,red,green,blue);
}

static void ModulateColor(const ModulateInfo *modulate_info,double *red,
  double *green,double *blue)
{
  const ColorspaceType
    colorspace = modulate_info->colorspace;

  switch (colorspace)
  {
    case HCLColorspace:
    {
      ModulateHCL(modulate_info,red,green,blue);
      break;
    }
    case HCLpColorspace:
    {
      ModulateHCLp(modulate_info,red,green,blue);
      break;
    }
    case HSBColorspace:
    {
      ModulateHSB(modulate_info,red,green,blue);
      break;
    }
    case HSIColorspace:
    {
      ModulateHSI(modulate_info,red,green,blue);
      break;
    }
    case HSLColorspace:
    default:
    {
      ModulateHSL(modulate_info,red,green,blue);
      break;
    }
    case HSVColorspace:
    {
      ModulateHSV(modulate_info,red,green,blue);
      break;
    }
    case HWBColorspace:
    {
      ModulateHWB(modulate_info,red,green,blue);
      break;
    }
    case LCHColorspace:
    case LCHabColorspace:
    {
      ModulateLCHab(modulate_info,red,green,blue);
      break;
    }
    case LCHuvColorspace:
    {
      ModulateLCHuv(modulate_info,red,green,blue);
      break;
    }
  }
}

static void ParseModulatePercents(const char *modulate,
  double *percent_brightness,double *percent_saturation,double *percent_hue)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  flags=ParseGeometry(modulate,&geometry_info);
  if ((flags & RhoValue) != 0)
    *percent_brightness=geometry_info.rho;
  if ((flags & SigmaValue) != 0)
    *percent_saturation=geometry_info.sigma;
  if ((flags & XiValue) != 0)
    *percent_hue=geometry_info.xi;
}

static void GetModulateColorspace(const Image *image,
  ColorspaceType *colorspace,IlluminantType *illuminant)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"modulate:colorspace");
  if (artifact != (const char *) NULL)
    *colorspace=(ColorspaceType) ParseCommandOption(MagickColorspaceOptions,
      MagickFalse,artifact);
  artifact=GetImageArtifact(image,"color:illuminant");
  if (artifact != (const char *) NULL)
    {
      ssize_t
        illuminant_type;

      illuminant_type=ParseCommandOption(MagickIlluminantOptions,MagickFalse,
        artifact);
      if (illuminant_type < 0)
        {
          *illuminant=UndefinedIlluminant;
          *colorspace=UndefinedColorspace;
        }
      else
        *illuminant=(IlluminantType) illuminant_type;
    }
}

MagickExport MagickBooleanType ModulateImage(Image *image,const char *modulate,
  ExceptionInfo *exception)
{
#define ModulateImageTag  "Modulate/Image"

  CacheView
    *image_view;

  ColorspaceType
    colorspace = UndefinedColorspace;

  double
    percent_brightness = 100.0,
    percent_hue = 100.0,
    percent_saturation = 100.0;

  IlluminantType
    illuminant = D65Illuminant;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    i;

  ssize_t
    y;

  /*
    Initialize modulate table.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (modulate == (char *) NULL)
    return(MagickFalse);
  if (IssRGBCompatibleColorspace(image->colorspace) == MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  ParseModulatePercents(modulate,&percent_brightness,&percent_saturation,
    &percent_hue);
  GetModulateColorspace(image,&colorspace,&illuminant);
  if (image->storage_class == PseudoClass)
    for (i=0; i < (ssize_t) image->colors; i++)
    {
      const ModulateInfo modulate_info = { colorspace, percent_hue,
        percent_saturation, percent_brightness, illuminant };

      double
        blue,
        green,
        red;

      /*
        Modulate image colormap.
      */
      red=(double) image->colormap[i].red;
      green=(double) image->colormap[i].green;
      blue=(double) image->colormap[i].blue;
      ModulateColor(&modulate_info,&red,&green,&blue);
      image->colormap[i].red=red;
      image->colormap[i].green=green;
      image->colormap[i].blue=blue;
    }
  /*
    Modulate image.
  */
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  if (AccelerateModulateImage(image,percent_brightness,percent_hue,
        percent_saturation,colorspace,exception) != MagickFalse)
    return(MagickTrue);
#endif
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      const ModulateInfo modulate_info = { colorspace, percent_hue,
        percent_saturation, percent_brightness, illuminant };

      double
        blue,
        green,
        red;

      red=(double) GetPixelRed(image,q);
      green=(double) GetPixelGreen(image,q);
      blue=(double) GetPixelBlue(image,q);
      ModulateColor(&modulate_info,&red,&green,&blue);
      SetPixelRed(image,ClampToQuantum(red),q);
      SetPixelGreen(image,ClampToQuantum(green),q);
      SetPixelBlue(image,ClampToQuantum(blue),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,ModulateImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     N o r m a l i z e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The NormalizeImage() method enhances the contrast of a color image by
%  mapping the darkest 2 percent of all pixel to black and the brightest
%  1 percent to white.
%
%  The format of the NormalizeImage method is:
%
%      MagickBooleanType NormalizeImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType NormalizeImage(Image *image,
  ExceptionInfo *exception)
{
  double
    black_point,
    white_point;

  black_point=0.02*image->columns*image->rows;
  white_point=0.99*image->columns*image->rows;
  return(ContrastStretchImage(image,black_point,white_point,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S i g m o i d a l C o n t r a s t I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SigmoidalContrastImage() adjusts the contrast of an image with a non-linear
%  sigmoidal contrast algorithm.  Increase the contrast of the image using a
%  sigmoidal transfer function without saturating highlights or shadows.
%  Contrast indicates how much to increase the contrast (0 is none; 3 is
%  typical; 20 is pushing it); mid-point indicates where midtones fall in the
%  resultant image (0 is white; 50% is middle-gray; 100% is black).  Set
%  sharpen to MagickTrue to increase the image contrast otherwise the contrast
%  is reduced.
%
%  The format of the SigmoidalContrastImage method is:
%
%      MagickBooleanType SigmoidalContrastImage(Image *image,
%        const MagickBooleanType sharpen,const char *levels,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o sharpen: Increase or decrease image contrast.
%
%    o contrast: strength of the contrast, the larger the number the more
%      'threshold-like' it becomes.
%
%    o midpoint: midpoint of the function as a color value 0 to QuantumRange.
%
%    o exception: return any errors or warnings in this structure.
%
*/

/*
  ImageMagick 6 has a version of this function which uses LUTs.
*/

/*
  Sigmoidal function Sigmoidal with inflexion point moved to b and "slope
  constant" set to a.

  The first version, based on the hyperbolic tangent tanh, when combined with
  the scaling step, is an exact arithmetic clone of the sigmoid function
  based on the logistic curve. The equivalence is based on the identity

    1/(1+exp(-t)) = (1+tanh(t/2))/2

  (http://de.wikipedia.org/wiki/Sigmoidfunktion) and the fact that the
  scaled sigmoidal derivation is invariant under affine transformations of
  the ordinate.

  The tanh version is almost certainly more accurate and cheaper.  The 0.5
  factor in the argument is to clone the legacy ImageMagick behavior. The
  reason for making the define depend on atanh even though it only uses tanh
  has to do with the construction of the inverse of the scaled sigmoidal.
*/
#if defined(MAGICKCORE_HAVE_ATANH)
#define Sigmoidal(a,b,x) ( tanh((0.5*(a))*((x)-(b))) )
#else
#define Sigmoidal(a,b,x) ( 1.0/(1.0+exp((a)*((b)-(x)))) )
#endif
/*
  Scaled sigmoidal function:

    ( Sigmoidal(a,b,x) - Sigmoidal(a,b,0) ) /
    ( Sigmoidal(a,b,1) - Sigmoidal(a,b,0) )

  See http://osdir.com/ml/video.image-magick.devel/2005-04/msg00006.html and
  http://www.cs.dartmouth.edu/farid/downloads/tutorials/fip.pdf.  The limit
  of ScaledSigmoidal as a->0 is the identity, but a=0 gives a division by
  zero. This is fixed below by exiting immediately when contrast is small,
  leaving the image (or colormap) unmodified. This appears to be safe because
  the series expansion of the logistic sigmoidal function around x=b is

  1/2-a*(b-x)/4+...

  so that the key denominator s(1)-s(0) is about a/4 (a/2 with tanh).
*/
#define ScaledSigmoidal(a,b,x) (                    \
  (Sigmoidal((a),(b),(x))-Sigmoidal((a),(b),0.0)) / \
  (Sigmoidal((a),(b),1.0)-Sigmoidal((a),(b),0.0)) )
/*
  Inverse of ScaledSigmoidal, used for +sigmoidal-contrast.  Because b
  may be 0 or 1, the argument of the hyperbolic tangent (resp. logistic
  sigmoidal) may be outside of the interval (-1,1) (resp. (0,1)), even
  when creating a LUT from in gamut values, hence the branching.  In
  addition, HDRI may have out of gamut values.
  InverseScaledSigmoidal is not a two-sided inverse of ScaledSigmoidal:
  It is only a right inverse. This is unavoidable.
*/
static inline double InverseScaledSigmoidal(const double a,const double b,
  const double x)
{
  const double sig0=Sigmoidal(a,b,0.0);
  const double sig1=Sigmoidal(a,b,1.0);
  const double argument=(sig1-sig0)*x+sig0;
  const double clamped=
    (
#if defined(MAGICKCORE_HAVE_ATANH)
      argument < -1+MagickEpsilon
      ?
      -1+MagickEpsilon
      :
      ( argument > 1-MagickEpsilon ? 1-MagickEpsilon : argument )
    );
  return(b+(2.0/a)*atanh(clamped));
#else
      argument < MagickEpsilon
      ?
      MagickEpsilon
      :
      ( argument > 1-MagickEpsilon ? 1-MagickEpsilon : argument )
    );
  return(b-log(1.0/clamped-1.0)/a);
#endif
}

#define ScaledSig(x) (ClampToQuantum((double) QuantumRange* \
  ScaledSigmoidal(contrast,QuantumScale*midpoint,QuantumScale*((double) x))) )
#define InverseScaledSig(x) (ClampToQuantum((double) QuantumRange* \
  InverseScaledSigmoidal(contrast,QuantumScale*midpoint,QuantumScale* \
  ((double) x))) )

static void SigmoidalContrastColormap(Image *image,
  const MagickBooleanType sharpen,const double contrast,const double midpoint)
{
  ssize_t
    i;

  if( sharpen != MagickFalse )
    for (i=0; i < (ssize_t) image->colors; i++)
    {
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].red=(MagickRealType) ScaledSig(
          image->colormap[i].red);
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].green=(MagickRealType) ScaledSig(
          image->colormap[i].green);
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].blue=(MagickRealType) ScaledSig(
          image->colormap[i].blue);
      if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].alpha=(MagickRealType) ScaledSig(
          image->colormap[i].alpha);
    }
  else
    for (i=0; i < (ssize_t) image->colors; i++)
    {
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].red=(MagickRealType) InverseScaledSig(
          image->colormap[i].red);
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].green=(MagickRealType) InverseScaledSig(
          image->colormap[i].green);
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].blue=(MagickRealType) InverseScaledSig(
          image->colormap[i].blue);
      if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].alpha=(MagickRealType) InverseScaledSig(
          image->colormap[i].alpha);
    }
}

static void SigmoidalContrastImageRow(const Image *image,
  const MagickBooleanType sharpen,const double contrast,const double midpoint,
  Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      if( sharpen != MagickFalse )
        q[i]=ScaledSig(q[i]);
      else
        q[i]=InverseScaledSig(q[i]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport MagickBooleanType SigmoidalContrastImage(Image *image,
  const MagickBooleanType sharpen,const double contrast,const double midpoint,
  ExceptionInfo *exception)
{
#define SigmoidalContrastImageTag  "SigmoidalContrast/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Convenience macros.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Side effect: may clamp values unless contrast<MagickEpsilon, in which
    case nothing is done.
  */
  if (contrast < MagickEpsilon)
    return(MagickTrue);
  /*
    Sigmoidal-contrast enhance colormap.
  */
  if (image->storage_class == PseudoClass)
    SigmoidalContrastColormap(image,sharpen,contrast,midpoint);
  /*
    Sigmoidal-contrast enhance image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    SigmoidalContrastImageRow(image,sharpen,contrast,midpoint,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SigmoidalContrastImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     W h i t e B a l a n c e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WhiteBalanceImage() applies white balancing to an image according to a
%  grayworld assumption in the LAB colorspace.
%
%  The format of the WhiteBalanceImage method is:
%
%      MagickBooleanType WhiteBalanceImage(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image to auto-level
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ShiftChromaRow(const Image *image,const double a_mean,
  const double b_mean,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    double
      a,
      b;

    /*
      Scale the chroma distance shifted according to amount of luminance.
    */
    a=(double) GetPixela(image,q)-1.1*(double) GetPixelL(image,q)*a_mean;
    b=(double) GetPixelb(image,q)-1.1*(double) GetPixelL(image,q)*b_mean;
    SetPixela(image,ClampToQuantum(a),q);
    SetPixelb(image,ClampToQuantum(b),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickStatusType LevelVibrance(Image *image,const char *artifact,
  MagickStatusType status,ExceptionInfo *exception)
{
  ChannelType
    channel_mask;

  double
    black_point = 0.0;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  /*
    Level the a & b channels.
  */
  flags=ParseGeometry(artifact,&geometry_info);
  if ((flags & RhoValue) != 0)
    black_point=geometry_info.rho;
  if ((flags & PercentValue) != 0)
    black_point*=((double) QuantumRange/100.0);
  channel_mask=SetImageChannelMask(image,(ChannelType) (aChannel |
    bChannel));
  status&=(MagickStatusType) LevelImage(image,black_point,(double)
    QuantumRange-black_point,1.0,exception);
  (void) SetImageChannelMask(image,channel_mask);
  return(status);
}

MagickExport MagickBooleanType WhiteBalanceImage(Image *image,
  ExceptionInfo *exception)
{
#define WhiteBalanceImageTag  "WhiteBalance/Image"

  CacheView
    *image_view;

  const char
    *artifact;

  double
    a_mean,
    b_mean;

  MagickOffsetType
    progress;

  MagickStatusType
    status;

  ssize_t
    y;

  /*
    White balance image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  status=TransformImageColorspace(image,LabColorspace,exception);
  a_mean=0.0;
  b_mean=0.0;
  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      a_mean+=QuantumScale*(double) GetPixela(image,p)-0.5;
      b_mean+=QuantumScale*(double) GetPixelb(image,p)-0.5;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  a_mean/=((double) image->columns*image->rows);
  b_mean/=((double) image->columns*image->rows);
  progress=0;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    ShiftChromaRow(image,a_mean,b_mean,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,WhiteBalanceImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  artifact=GetImageArtifact(image,"white-balance:vibrance");
  if (artifact != (const char *) NULL)
    status=LevelVibrance(image,artifact,status,exception);
  status&=(MagickStatusType) TransformImageColorspace(image,sRGBColorspace,
    exception);
  return(status != 0 ? MagickTrue : MagickFalse);
}
