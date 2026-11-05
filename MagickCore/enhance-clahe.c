/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               EEEEE  N   N  H   H   AAA   N   N   CCCC  EEEEE               %
%               E      NN  N  H   H  A   A  NN  N  C      E                   %
%               EEE    N N N  HHHHH  AAAAA  N N N  C      EEE                 %
%               E      N  NN  H   H  A   A  N  NN  C      E                   %
%               EEEEE  N   N  H   H  A   A  N   N   CCCC  EEEEE               %
%                                                                             %
%                       CCCC  L       AAA   H   H  EEEEE                      %
%                      C      L      A   A  H   H  E                          %
%                      C      L      AAAAA  HHHHH  EEE                        %
%                      C      L      A   A  H   H  E                          %
%                       CCCC  LLLLL  A   A  H   H  EEEEE                      %
%                                                                             %
%                                                                             %
%                       MagickCore CLAHE Image Methods                        %
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

typedef struct _CLAHERedistribution
{
  size_t
    *histogram,
    *q;

  double
    clip_limit;

  ssize_t
    step;
} CLAHERedistribution;

static void RedistributeCLAHEBins(const CLAHERedistribution *redistribution,
  size_t **p,ssize_t *cumulative_excess)
{
  size_t
    *const histogram = redistribution->histogram,
    *const q = redistribution->q;

  const double
    clip_limit = redistribution->clip_limit;

  const ssize_t
    step = redistribution->step;

  for ((*p)=histogram; ((*p) < q) && ((*cumulative_excess) != 0); (*p)+=(ptrdiff_t) step)
    if ((double) *(*p) < clip_limit)
      {
        (*(*p))++;
        (*cumulative_excess)--;
      }
}

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
    CLAHERedistribution
      redistribution;

    step=(ssize_t) number_bins/cumulative_excess;
    if (step < 1)
      step=1;
    redistribution.histogram=histogram;
    redistribution.q=q;
    redistribution.clip_limit=clip_limit;
    redistribution.step=step;
    RedistributeCLAHEBins(&redistribution,&p,&cumulative_excess);
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

typedef struct _CLAHETileColumn
{
  const RectangleInfo
    *clahe_info,
    *tile_info;

  ssize_t
    x;
} CLAHETileColumn;

static void SelectCLAHETileColumn(const CLAHETileColumn *tile_column,
  RectangleInfo *tile,OffsetInfo *offset)
{
  const RectangleInfo
    *const clahe_info = tile_column->clahe_info,
    *const tile_info = tile_column->tile_info;

  const ssize_t
    x = tile_column->x;

  (*tile).width=tile_info->width;
  (*tile).x=x-1;
  (*offset).x=(*tile).x+1;
  if (x == 0)
    {
      /*
        Left column.
      */
      (*tile).width=tile_info->width >> 1;
      (*tile).x=0;
      (*offset).x=0;
    }
  else
    if (x == (ssize_t) clahe_info->x)
      {
        /*
          Right column.
        */
        (*tile).width=(tile_info->width+1) >> 1;
        (*tile).x=clahe_info->x-1;
        (*offset).x=(*tile).x;
      }
}

typedef struct _CLAHEInterpolation
{
  const RectangleInfo
    *clahe_info,
    *tile_info;

  size_t
    number_bins;

  size_t
    *tiles;

  unsigned short
    *lut;
} CLAHEInterpolation;

static void InterpolateCLAHETiles(const CLAHEInterpolation *interpolation,
  unsigned short *pixels)
{
  const RectangleInfo
    *const clahe_info = interpolation->clahe_info,
    *const tile_info = interpolation->tile_info;

  const size_t
    number_bins = interpolation->number_bins;

  size_t
    *const tiles = interpolation->tiles;

  ssize_t
    y;

  unsigned short
    *const lut = interpolation->lut,
    *p;

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

      CLAHETileColumn
        tile_column;

      tile_column.clahe_info=clahe_info;
      tile_column.tile_info=tile_info;
      tile_column.x=x;
      SelectCLAHETileColumn(&tile_column,&tile,&offset);
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
}

static MagickBooleanType CLAHE(const RectangleInfo *clahe_info,
  const RectangleInfo *tile_info,const RangeInfo *range_info,
  const size_t number_bins,const double clip_limit,unsigned short *pixels)
{
  CLAHEInterpolation
    interpolation;

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
  interpolation.clahe_info=clahe_info;
  interpolation.tile_info=tile_info;
  interpolation.number_bins=number_bins;
  interpolation.tiles=tiles;
  interpolation.lut=lut;
  InterpolateCLAHETiles(&interpolation,pixels);
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

#define CLAHEImageTag  "CLAHE/Image"

static void UpdateCLAHEProgress(const Image *image,MagickOffsetType *progress,
  MagickBooleanType *status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      (*progress)++;
      proceed=SetImageProgress(image,CLAHEImageTag,(*progress),2*
        GetPixelChannels(image));
      if (proceed == MagickFalse)
        (*status)=MagickFalse;
    }
}

typedef struct _CLAHEPixelLoad
{
  Image
    *image;

  CacheView
    *image_view;

  RectangleInfo
    clahe_info,
    tile_info;

  unsigned short
    *pixels;

  ExceptionInfo
    *exception;
} CLAHEPixelLoad;

static void LoadCLAHEPixels(const CLAHEPixelLoad *pixel_load,
  MagickBooleanType *status,MagickOffsetType *progress)
{
  Image
    *const image = pixel_load->image;

  CacheView
    *const image_view = pixel_load->image_view;

  const RectangleInfo
    clahe_info = pixel_load->clahe_info,
    tile_info = pixel_load->tile_info;

  unsigned short
    *const pixels = pixel_load->pixels;

  ExceptionInfo
    *const exception = pixel_load->exception;

  size_t
    n;

  ssize_t
    y;

  n=0;
  for (y=0; y < (ssize_t) clahe_info.height; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    if ((*status) == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-(tile_info.x >> 1),y-
      (tile_info.y >> 1),clahe_info.width,1,exception);
    if (p == (const Quantum *) NULL)
      {
        (*status)=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) clahe_info.width; x++)
    {
      pixels[n++]=ScaleQuantumToShort(p[0]);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
    UpdateCLAHEProgress(image,progress,status);
  }
}

static size_t StoreCLAHERow(const Image *image,const unsigned short *pixels,
  size_t n,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    q[0]=ScaleShortToQuantum(pixels[n++]);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(n);
}

static void StoreCLAHEPixels(const CLAHEPixelLoad *pixel_load,
  MagickBooleanType *status,MagickOffsetType *progress)
{
  Image
    *const image = pixel_load->image;

  CacheView
    *const image_view = pixel_load->image_view;

  const RectangleInfo
    clahe_info = pixel_load->clahe_info,
    tile_info = pixel_load->tile_info;

  unsigned short
    *const pixels = pixel_load->pixels;

  ExceptionInfo
    *const exception = pixel_load->exception;

  size_t
    n;

  ssize_t
    y;

  n=clahe_info.width*(size_t) (tile_info.y/2);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if ((*status) == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        (*status)=MagickFalse;
        continue;
      }
    n+=(size_t) (tile_info.x/2);
    n=StoreCLAHERow(image,pixels,n,q);
    n+=(size_t) ((ssize_t) clahe_info.width-(ssize_t) image->columns-
      (tile_info.x/2));
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      (*status)=MagickFalse;
    UpdateCLAHEProgress(image,progress,status);
  }
}

static RectangleInfo GetCLAHEInfo(const Image *image,
  const RectangleInfo tile_info)
{
  RectangleInfo
    clahe_info;

  clahe_info.width=(size_t) ((ssize_t) image->columns+tile_info.x);
  clahe_info.height=(size_t) ((ssize_t) image->rows+tile_info.y);
  clahe_info.x=(ssize_t) (clahe_info.width/tile_info.width);
  clahe_info.y=(ssize_t) (clahe_info.height/tile_info.height);
  return(clahe_info);
}

typedef struct _CLAHEPixelSource
{
  Image
    *image;

  RectangleInfo
    clahe_info,
    tile_info;

  unsigned short
    *pixels;

  ExceptionInfo
    *exception;
} CLAHEPixelSource;

static void InitializeCLAHEPixels(const CLAHEPixelSource *pixel_source,
  MagickBooleanType *status,MagickOffsetType *progress)
{
  CacheView
    *image_view;

  CLAHEPixelLoad
    pixel_load;

  image_view=AcquireVirtualCacheView(pixel_source->image,
    pixel_source->exception);
  (*progress)=0;
  (*status)=MagickTrue;
  pixel_load.image=pixel_source->image;
  pixel_load.image_view=image_view;
  pixel_load.clahe_info=pixel_source->clahe_info;
  pixel_load.tile_info=pixel_source->tile_info;
  pixel_load.pixels=pixel_source->pixels;
  pixel_load.exception=pixel_source->exception;
  LoadCLAHEPixels(&pixel_load,&(*status),&(*progress));
  image_view=DestroyCacheView(image_view);
}

MagickExport MagickBooleanType CLAHEImage(Image *image,const size_t width,
  const size_t height,const size_t number_bins,const double clip_limit,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  CLAHEPixelLoad
    pixel_load;

  CLAHEPixelSource
    pixel_source;

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
  clahe_info=GetCLAHEInfo(image,tile_info);
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
  pixel_source.image=image;
  pixel_source.clahe_info=clahe_info;
  pixel_source.tile_info=tile_info;
  pixel_source.pixels=pixels;
  pixel_source.exception=exception;
  InitializeCLAHEPixels(&pixel_source,&status,&progress);
  status=CLAHE(&clahe_info,&tile_info,&range_info,number_bins == 0 ?
    (size_t) 128 : MagickMin(number_bins,256),clip_limit,pixels);
  if (status == MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
  /*
    Push CLAHE pixels to CLAHE image.
  */
  image_view=AcquireAuthenticCacheView(image,exception);
  pixel_load.image=image;
  pixel_load.image_view=image_view;
  pixel_load.clahe_info=clahe_info;
  pixel_load.tile_info=tile_info;
  pixel_load.pixels=pixels;
  pixel_load.exception=exception;
  StoreCLAHEPixels(&pixel_load,&status,&progress);
  image_view=DestroyCacheView(image_view);
  pixel_cache=RelinquishVirtualMemory(pixel_cache);
  if (TransformImageColorspace(image,colorspace,exception) == MagickFalse)
    status=MagickFalse;
  return(status);
}
