/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      PPPP    AAA   IIIII  N   N  TTTTT                      %
%                      P   P  A   A    I    NN  N    T                        %
%                      PPPP   AAAAA    I    N N N    T                        %
%                      P      A   A    I    N  NN    T                        %
%                      P      A   A  IIIII  N   N    T                        %
%                                                                             %
%                                                                             %
%                        Methods to Paint on an Image                         %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1998                                   %
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
*/

/*
 Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F l o o d f i l l P a i n t I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FloodfillPaintImage() changes the color value of any pixel that matches
%  target and is an immediate neighbor.  If the method FillToBorderMethod is
%  specified, the color value is changed for any neighbor pixel that does not
%  match the bordercolor member of image.
%
%  By default target must match a particular pixel color exactly.  However,
%  in many cases two colors may differ by a small amount.  The fuzz member of
%  image defines how much tolerance is acceptable to consider two colors as
%  the same.  For example, set fuzz to 10 and the color red at intensities of
%  100 and 102 respectively are now interpreted as the same color for the
%  purposes of the floodfill.
%
%  The format of the FloodfillPaintImage method is:
%
%      MagickBooleanType FloodfillPaintImage(Image *image,
%        const DrawInfo *draw_info,const PixelInfo target,
%        const ssize_t x_offset,const ssize_t y_offset,
%        const MagickBooleanType invert,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o target: the RGB value of the target color.
%
%    o x_offset,y_offset: the starting location of the operation.
%
%    o invert: paint any pixel that does not match the target color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline void SetMaskedFillChannels(const Image *image,
  const PixelInfo *fill_color,Quantum *magick_restrict q)
{
  if ((image->channel_mask & RedChannel) != 0)
    SetPixelRed(image,(Quantum) fill_color->red,q);
  if ((image->channel_mask & GreenChannel) != 0)
    SetPixelGreen(image,(Quantum) fill_color->green,q);
  if ((image->channel_mask & BlueChannel) != 0)
    SetPixelBlue(image,(Quantum) fill_color->blue,q);
  if ((image->channel_mask & BlackChannel) != 0)
    SetPixelBlack(image,(Quantum) fill_color->black,q);
  if (((image->channel_mask & AlphaChannel) != 0) &&
      ((image->alpha_trait & BlendPixelTrait) != 0))
    SetPixelAlpha(image,(Quantum) fill_color->alpha,q);
}

typedef struct _FloodfillScan
{
  Image
    *image,
    *floodplane_image;

  const PixelInfo
    *target;

  MagickBooleanType
    invert;
} FloodfillScan;

static ssize_t FillFloodplaneSpan(const FloodfillScan *scan,
  const Quantum *magick_restrict p,Quantum *magick_restrict q,ssize_t x)
{
  PixelInfo
    pixel;

  for ( ; x < (ssize_t) scan->image->columns; x++)
  {
    if (GetPixelGray(scan->floodplane_image,q) != 0)
      break;
    GetPixelInfoPixel(scan->image,p,&pixel);
    if (IsFuzzyEquivalencePixelInfo(&pixel,scan->target) == scan->invert)
      break;
    SetPixelGray(scan->floodplane_image,QuantumRange,q);
    p+=(ptrdiff_t) GetPixelChannels(scan->image);
    q+=(ptrdiff_t) GetPixelChannels(scan->floodplane_image);
  }
  return(x);
}

static MagickBooleanType PrepareFloodfillImage(Image *image,
  const DrawInfo *draw_info,ExceptionInfo *exception)
{
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  if (((image->alpha_trait & BlendPixelTrait) == 0) &&
      (draw_info->fill.alpha_trait != UndefinedPixelTrait))
    (void) SetImageAlpha(image,OpaqueAlpha,exception);
  return(MagickTrue);
}

static MagickBooleanType AcquireFloodplaneImage(Image *image,
  Image **floodplane_image,ExceptionInfo *exception)
{
  *floodplane_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*floodplane_image == (Image *) NULL)
    return(MagickFalse);
  (*floodplane_image)->alpha_trait=UndefinedPixelTrait;
  (*floodplane_image)->colorspace=GRAYColorspace;
  (void) QueryColorCompliance("#000",AllCompliance,
    &(*floodplane_image)->background_color,exception);
  (void) SetImageBackgroundColor(*floodplane_image,exception);
  return(MagickTrue);
}

typedef struct _FloodfillTile
{
  Image
    *image,
    *floodplane_image;

  const DrawInfo
    *draw_info;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} FloodfillTile;

static void TileFloodplaneRow(const FloodfillTile *tile,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) tile->image->columns; x++)
  {
    if (GetPixelGray(tile->floodplane_image,p) != 0)
      {
        PixelInfo
          fill_color;

        GetFillColor(tile->draw_info,x,tile->y,&fill_color,tile->exception);
        SetMaskedFillChannels(tile->image,&fill_color,q);
      }
    p+=(ptrdiff_t) GetPixelChannels(tile->floodplane_image);
    q+=(ptrdiff_t) GetPixelChannels(tile->image);
  }
}

typedef struct _FloodfillRow
{
  Image
    *image,
    *floodplane_image;

  CacheView
    *image_view,
    *floodplane_view;

  const PixelInfo
    *target;

  MagickBooleanType
    invert;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} FloodfillRow;

static MagickBooleanType FillFloodplaneRun(const FloodfillRow *row,ssize_t *x)
{
  const Quantum
    *magick_restrict p;

  FloodfillScan
    scan;

  MagickBooleanType
    status;

  Quantum
    *magick_restrict q;

  if (*x < (ssize_t) row->image->columns)
    {
      p=GetCacheViewVirtualPixels(row->image_view,*x,row->y,(size_t)
        ((ssize_t) row->image->columns-*x),1,row->exception);
      q=GetCacheViewAuthenticPixels(row->floodplane_view,*x,row->y,(size_t)
        ((ssize_t) row->image->columns-*x),1,row->exception);
      if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
        return(MagickFalse);
      scan.image=row->image;
      scan.floodplane_image=row->floodplane_image;
      scan.target=row->target;
      scan.invert=row->invert;
      *x=FillFloodplaneSpan(&scan,p,q,*x);
      status=SyncCacheViewAuthenticPixels(row->floodplane_view,row->exception);
      if (status == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType SkipFloodplaneRun(const FloodfillRow *row,
  const ssize_t x2,ssize_t *x)
{
  const Quantum
    *magick_restrict p;

  PixelInfo
    pixel;

  Quantum
    *magick_restrict q;

  p=GetCacheViewVirtualPixels(row->image_view,*x,row->y,(size_t) (x2-*x+1),1,
    row->exception);
  q=GetCacheViewAuthenticPixels(row->floodplane_view,*x,row->y,(size_t)
    (x2-*x+1),1,row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    return(MagickFalse);
  for ( ; *x <= x2; (*x)++)
  {
    if (GetPixelGray(row->floodplane_image,q) != 0)
      break;
    GetPixelInfoPixel(row->image,p,&pixel);
    if (IsFuzzyEquivalencePixelInfo(&pixel,row->target) != row->invert)
      break;
    p+=(ptrdiff_t) GetPixelChannels(row->image);
    q+=(ptrdiff_t) GetPixelChannels(row->floodplane_image);
  }
  return(MagickTrue);
}

MagickExport MagickBooleanType FloodfillPaintImage(Image *image,
  const DrawInfo *draw_info,const PixelInfo *target,const ssize_t x_offset,
  const ssize_t y_offset,const MagickBooleanType invert,
  ExceptionInfo *exception)
{
#define MaxStacksize  524288UL
#define PushSegmentStack(up,left,right,delta) \
{ \
  if (s >= (segment_stack+MaxStacksize)) \
    { \
      segment_info=RelinquishVirtualMemory(segment_info); \
      image_view=DestroyCacheView(image_view); \
      floodplane_view=DestroyCacheView(floodplane_view); \
      floodplane_image=DestroyImage(floodplane_image); \
      ThrowBinaryException(DrawError,"SegmentStackOverflow",image->filename) \
    } \
  else \
    { \
      if ((((up)+(delta)) >= 0) && (((up)+(delta)) < (ssize_t) image->rows)) \
        { \
          s->x1=(double) (left); \
          s->y1=(double) (up); \
          s->x2=(double) (right); \
          s->y2=(double) (delta); \
          s++; \
        } \
    } \
}

  CacheView
    *floodplane_view,
    *image_view;

  Image
    *floodplane_image;

  MagickBooleanType
    skip,
    status;

  MemoryInfo
    *segment_info;

  PixelInfo
    pixel;

  SegmentInfo
    *s;

  SegmentInfo
    *segment_stack;

  ssize_t
    offset,
    start,
    x1,
    x2,
    y;

  /*
    Check boundary conditions.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((x_offset < 0) || (x_offset >= (ssize_t) image->columns))
    return(MagickFalse);
  if ((y_offset < 0) || (y_offset >= (ssize_t) image->rows))
    return(MagickFalse);
  if (PrepareFloodfillImage(image,draw_info,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Set floodfill state.
  */
  if (AcquireFloodplaneImage(image,&floodplane_image,exception) == MagickFalse)
    return(MagickFalse);
  segment_info=AcquireVirtualMemory(MaxStacksize,sizeof(*segment_stack));
  if (segment_info == (MemoryInfo *) NULL)
    {
      floodplane_image=DestroyImage(floodplane_image);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  segment_stack=(SegmentInfo *) GetVirtualMemoryBlob(segment_info);
  /*
    Push initial segment on stack.
  */
  status=MagickTrue;
  start=0;
  s=segment_stack;
  GetPixelInfo(image,&pixel);
  image_view=AcquireVirtualCacheView(image,exception);
  floodplane_view=AcquireAuthenticCacheView(floodplane_image,exception);
  PushSegmentStack(y_offset,x_offset,x_offset,1);
  PushSegmentStack(y_offset+1,x_offset,x_offset,-1);
  while (s > segment_stack)
  {
    const Quantum
      *magick_restrict p;

    FloodfillRow
      row;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    /*
      Pop segment off stack.
    */
    s--;
    x1=(ssize_t) s->x1;
    x2=(ssize_t) s->x2;
    offset=(ssize_t) s->y2;
    y=(ssize_t) s->y1+offset;
    /*
      Recolor neighboring pixels.
    */
    p=GetCacheViewVirtualPixels(image_view,0,y,(size_t) (x1+1),1,exception);
    q=GetCacheViewAuthenticPixels(floodplane_view,0,y,(size_t) (x1+1),1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      break;
    p+=(ptrdiff_t) x1*(ssize_t) GetPixelChannels(image);
    q+=(ptrdiff_t) x1*(ssize_t) GetPixelChannels(floodplane_image);
    for (x=x1; x >= 0; x--)
    {
      if (GetPixelGray(floodplane_image,q) != 0)
        break;
      GetPixelInfoPixel(image,p,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,target) == invert)
        break;
      SetPixelGray(floodplane_image,QuantumRange,q);
      p-=(ptrdiff_t)GetPixelChannels(image);
      q-=GetPixelChannels(floodplane_image);
    }
    if (SyncCacheViewAuthenticPixels(floodplane_view,exception) == MagickFalse)
      break;
    skip=x >= x1 ? MagickTrue : MagickFalse;
    if (skip == MagickFalse)
      {
        start=x+1;
        if (start < x1)
          PushSegmentStack(y,start,x1-1,-offset);
        x=x1+1;
      }
    do
    {
      if (skip == MagickFalse)
        {
          row.image=image;
          row.floodplane_image=floodplane_image;
          row.image_view=image_view;
          row.floodplane_view=floodplane_view;
          row.target=target;
          row.invert=invert;
          row.y=y;
          row.exception=exception;
          if (FillFloodplaneRun(&row,&x) == MagickFalse)
            break;
          PushSegmentStack(y,start,x-1,offset);
          if (x > (x2+1))
            PushSegmentStack(y,x2+1,x-1,-offset);
        }
      skip=MagickFalse;
      x++;
      if (x <= x2)
        {
          row.image=image;
          row.floodplane_image=floodplane_image;
          row.image_view=image_view;
          row.floodplane_view=floodplane_view;
          row.target=target;
          row.invert=invert;
          row.y=y;
          row.exception=exception;
          if (SkipFloodplaneRun(&row,x2,&x) == MagickFalse)
            break;
        }
      start=x;
    } while (x <= x2);
  }
  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(floodplane_image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    FloodfillTile
      tile;

    Quantum
      *magick_restrict q;

    /*
      Tile fill color onto floodplane.
    */
    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(floodplane_view,0,y,image->columns,1,exception);
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    tile.image=image;
    tile.floodplane_image=floodplane_image;
    tile.draw_info=draw_info;
    tile.y=y;
    tile.exception=exception;
    TileFloodplaneRow(&tile,p,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  floodplane_view=DestroyCacheView(floodplane_view);
  image_view=DestroyCacheView(image_view);
  segment_info=RelinquishVirtualMemory(segment_info);
  floodplane_image=DestroyImage(floodplane_image);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     G r a d i e n t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GradientImage() applies a continuously smooth color transitions along a
%  vector from one color to another.
%
%  Note, the interface of this method will change in the future to support
%  more than one transition.
%
%  The format of the GradientImage method is:
%
%      MagickBooleanType GradientImage(Image *image,const GradientType type,
%        const SpreadMethod method,const PixelInfo *start_color,
%        const PixelInfo *stop_color,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o type: the gradient type: linear or radial.
%
%    o spread: the gradient spread method: pad, reflect, or repeat.
%
%    o start_color: the start color.
%
%    o stop_color: the stop color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsGradientDefaultDirection(const Image *image)
{
  return(((GetImageArtifact(image,"gradient:angle") == (const char *) NULL) &&
    (GetImageArtifact(image,"gradient:direction") == (const char *) NULL) &&
    (GetImageArtifact(image,"gradient:extent") == (const char *) NULL) &&
    (GetImageArtifact(image,"gradient:vector") ==
    (const char *) NULL)) ? MagickTrue : MagickFalse);
}

static void SetGradientSouthDirection(GradientInfo *gradient,
  const Image *image,const GravityType direction)
{
  switch (direction)
  {
    case SouthWestGravity:
    {
      gradient->gradient_vector.x1=(double) image->columns-1;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=(double) image->rows-1;
      break;
    }
    case SouthGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=(double) image->rows-1;
      break;
    }
    case SouthEastGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=(double) image->columns-1;
      gradient->gradient_vector.y2=(double) image->rows-1;
      break;
    }
    default:
      break;
  }
}

static void SetGradientDirection(GradientInfo *gradient,const Image *image,
  const char *artifact)
{
  GravityType
    direction;

  direction=(GravityType) ParseCommandOption(MagickGravityOptions,
    MagickFalse,artifact);
  switch (direction)
  {
    case NorthWestGravity:
    {
      gradient->gradient_vector.x1=(double) image->columns-1;
      gradient->gradient_vector.y1=(double) image->rows-1;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    case NorthGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=(double) image->rows-1;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    case NorthEastGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=(double) image->rows-1;
      gradient->gradient_vector.x2=(double) image->columns-1;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    case WestGravity:
    {
      gradient->gradient_vector.x1=(double) image->columns-1;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=0.0;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    case EastGravity:
    {
      gradient->gradient_vector.x1=0.0;
      gradient->gradient_vector.y1=0.0;
      gradient->gradient_vector.x2=(double) image->columns-1;
      gradient->gradient_vector.y2=0.0;
      break;
    }
    default:
    {
      SetGradientSouthDirection(gradient,image,direction);
      break;
    }
  }
}

static void SetGradientAngleVector(GradientInfo *gradient,const Image *image)
{
  double
    sine,
    cosine,
    distance;

  /*
    Reference https://drafts.csswg.org/css-images-3/#linear-gradients.
  */
  sine=sin((double) DegreesToRadians(gradient->angle-90.0));
  cosine=cos((double) DegreesToRadians(gradient->angle-90.0));
  distance=fabs((double) (image->columns-1.0)*cosine)+
    fabs((double) (image->rows-1.0)*sine);
  gradient->gradient_vector.x1=0.5*((image->columns-1.0)-distance*cosine);
  gradient->gradient_vector.y1=0.5*((image->rows-1.0)-distance*sine);
  gradient->gradient_vector.x2=0.5*((image->columns-1.0)+distance*cosine);
  gradient->gradient_vector.y2=0.5*((image->rows-1.0)+distance*sine);
}

static void SetGradientExtent(GradientInfo *gradient,const Image *image,
  const char *artifact)
{
  if (LocaleCompare(artifact,"Circle") == 0)
    {
      gradient->radii.x=(double) MagickMax((image->columns-1.0),
        (image->rows-1.0))/2.0;
      gradient->radii.y=gradient->radii.x;
    }
  if (LocaleCompare(artifact,"Diagonal") == 0)
    {
      gradient->radii.x=(double) (sqrt((double) (image->columns-1.0)*
        (image->columns-1.0)+(image->rows-1.0)*(image->rows-1.0)))/2.0;
      gradient->radii.y=gradient->radii.x;
    }
  if (LocaleCompare(artifact,"Ellipse") == 0)
    {
      gradient->radii.x=(double) (image->columns-1.0)/2.0;
      gradient->radii.y=(double) (image->rows-1.0)/2.0;
    }
  if (LocaleCompare(artifact,"Maximum") == 0)
    {
      gradient->radii.x=(double) MagickMax((image->columns-1.0),
        (image->rows-1.0))/2.0;
      gradient->radii.y=gradient->radii.x;
    }
  if (LocaleCompare(artifact,"Minimum") == 0)
    {
      gradient->radii.x=(double) (MagickMin((image->columns-1.0),
        (image->rows-1.0)))/2.0;
      gradient->radii.y=gradient->radii.x;
    }
}

static void SetGradientRadii(GradientInfo *gradient,const Image *image)
{
  const char
    *artifact;

  gradient->radii.x=(double) MagickMax((image->columns-1.0),(image->rows-1.0))/
    2.0;
  gradient->radii.y=gradient->radii.x;
  artifact=GetImageArtifact(image,"gradient:extent");
  if (artifact != (const char *) NULL)
    SetGradientExtent(gradient,image,artifact);
  artifact=GetImageArtifact(image,"gradient:radii");
  if (artifact != (const char *) NULL)
    (void) MagickSscanf(artifact,"%lf%*[ ,]%lf",&gradient->radii.x,
      &gradient->radii.y);
  gradient->radius=MagickMax(gradient->radii.x,gradient->radii.y);
}

static void SetGradientVector(GradientInfo *gradient,const Image *image,
  const GradientType type)
{
  const char
    *artifact;

  gradient->gradient_vector.x2=(double) image->columns-1;
  gradient->gradient_vector.y2=(double) image->rows-1;
  artifact=GetImageArtifact(image,"gradient:direction");
  if (artifact != (const char *) NULL)
    SetGradientDirection(gradient,image,artifact);
  artifact=GetImageArtifact(image,"gradient:angle");
  if (artifact != (const char *) NULL)
    gradient->angle=StringToDouble(artifact,(char **) NULL);
  artifact=GetImageArtifact(image,"gradient:vector");
  if (artifact != (const char *) NULL)
    (void) MagickSscanf(artifact,"%lf%*[ ,]%lf%*[ ,]%lf%*[ ,]%lf",
      &gradient->gradient_vector.x1,&gradient->gradient_vector.y1,
      &gradient->gradient_vector.x2,&gradient->gradient_vector.y2);
  if (IsGradientDefaultDirection(image) != MagickFalse)
    if ((type == LinearGradient) && (gradient->gradient_vector.y2 != 0.0))
      gradient->gradient_vector.x2=0.0;
}

MagickExport MagickBooleanType GradientImage(Image *image,
  const GradientType type,const SpreadMethod method,const StopInfo *stops,
  const size_t number_stops,ExceptionInfo *exception)
{
  const char
    *artifact;

  DrawInfo
    *draw_info;

  GradientInfo
    *gradient;

  MagickBooleanType
    status;

  /*
    Set gradient start-stop end points.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(stops != (const StopInfo *) NULL);
  assert(number_stops > 0);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  draw_info=AcquireDrawInfo();
  gradient=(&draw_info->gradient);
  gradient->type=type;
  gradient->bounding_box.width=image->columns;
  gradient->bounding_box.height=image->rows;
  artifact=GetImageArtifact(image,"gradient:bounding-box");
  if (artifact != (const char *) NULL)
    (void) ParseAbsoluteGeometry(artifact,&gradient->bounding_box);
  SetGradientVector(gradient,image,type);
  gradient->center.x=(double) gradient->gradient_vector.x2/2.0;
  gradient->center.y=(double) gradient->gradient_vector.y2/2.0;
  artifact=GetImageArtifact(image,"gradient:center");
  if (artifact != (const char *) NULL)
    (void) MagickSscanf(artifact,"%lf%*[ ,]%lf",&gradient->center.x,
      &gradient->center.y);
  artifact=GetImageArtifact(image,"gradient:angle");
  if ((type == LinearGradient) && (artifact != (const char *) NULL))
    SetGradientAngleVector(gradient,image);
  SetGradientRadii(gradient,image);
  gradient->spread=method;
  /*
    Define the gradient to fill between the stops.
  */
  gradient->number_stops=number_stops;
  gradient->stops=(StopInfo *) AcquireQuantumMemory(gradient->number_stops,
    sizeof(*gradient->stops));
  if (gradient->stops == (StopInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) memcpy(gradient->stops,stops,(size_t) number_stops*sizeof(*stops));
  /*
    Draw a gradient on the image.
  */
  status=DrawGradientImage(image,draw_info,exception);
  draw_info=DestroyDrawInfo(draw_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O i l P a i n t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OilPaintImage() applies a special effect filter that simulates an oil
%  painting.  Each pixel is replaced by the most frequent color occurring
%  in a circular region defined by radius.
%
%  The format of the OilPaintImage method is:
%
%      Image *OilPaintImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the circular neighborhood.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static size_t **DestroyHistogramTLS(size_t **histogram)
{
  ssize_t
    i;

  assert(histogram != (size_t **) NULL);
  for (i=0; i < (ssize_t) GetMagickResourceLimit(ThreadResource); i++)
    if (histogram[i] != (size_t *) NULL)
      histogram[i]=(size_t *) RelinquishMagickMemory(histogram[i]);
  histogram=(size_t **) RelinquishMagickMemory(histogram);
  return(histogram);
}

static size_t **AcquireHistogramTLS(const size_t count)
{
  ssize_t
    i;

  size_t
    **histogram,
    number_threads;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  histogram=(size_t **) AcquireQuantumMemory(number_threads,sizeof(*histogram));
  if (histogram == (size_t **) NULL)
    return((size_t **) NULL);
  (void) memset(histogram,0,number_threads*sizeof(*histogram));
  for (i=0; i < (ssize_t) number_threads; i++)
  {
    histogram[i]=(size_t *) AcquireQuantumMemory(count,sizeof(**histogram));
    if (histogram[i] == (size_t *) NULL)
      return(DestroyHistogramTLS(histogram));
  }
  return(histogram);
}

#define NumberPaintBins  256
#define OilPaintImageTag  "OilPaint/Image"

static ssize_t GetMostFrequentOffset(const Image *linear_image,
  const Quantum *magick_restrict p,size_t *histogram,const size_t width)
{
  size_t
    count;

  ssize_t
    j,
    k,
    n,
    u,
    v;

  k=0;
  j=0;
  count=0;
  (void) memset(histogram,0,NumberPaintBins* sizeof(*histogram));
  for (v=0; v < (ssize_t) width; v++)
  {
    for (u=0; u < (ssize_t) width; u++)
    {
      n=(ssize_t) ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(
        linear_image,p+(ssize_t) GetPixelChannels(linear_image)*(u+k))));
      histogram[n]++;
      if (histogram[n] > count)
        {
          j=k+u;
          count=histogram[n];
        }
    }
    k+=(ssize_t) (linear_image->columns+width);
  }
  return(j);
}

static inline void SetOilPaintChannels(const Image *linear_image,
  const Image *paint_image,const Quantum *magick_restrict p,
  Quantum *magick_restrict q,const ssize_t center,const ssize_t j)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(linear_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(linear_image,i);
    PixelTrait traits = GetPixelChannelTraits(linear_image,channel);
    PixelTrait paint_traits=GetPixelChannelTraits(paint_image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (paint_traits == UndefinedPixelTrait))
      continue;
    if ((paint_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(paint_image,channel,p[center+i],q);
        continue;
      }
    SetPixelChannel(paint_image,channel,p[j*(ssize_t)
      GetPixelChannels(linear_image)+i],q);
  }
}

static Image *DestroyOilPaintClones(Image *linear_image,Image *paint_image)
{
  if (linear_image != (Image *) NULL)
    linear_image=DestroyImage(linear_image);
  if (paint_image != (Image *) NULL)
    linear_image=DestroyImage(paint_image);
  return((Image *) NULL);
}

MagickExport Image *OilPaintImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *paint_view;

  Image
    *linear_image,
    *paint_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  size_t
    **histograms,
    width;

  ssize_t
    center,
    y;

  /*
    Initialize painted image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth2D(radius,sigma);
  linear_image=CloneImage(image,0,0,MagickTrue,exception);
  paint_image=CloneImage(image,0,0,MagickTrue,exception);
  if ((linear_image == (Image *) NULL) || (paint_image == (Image *) NULL))
    return(DestroyOilPaintClones(linear_image,paint_image));
  if (SetImageStorageClass(paint_image,DirectClass,exception) == MagickFalse)
    {
      linear_image=DestroyImage(linear_image);
      paint_image=DestroyImage(paint_image);
      return((Image *) NULL);
    }
  histograms=AcquireHistogramTLS(NumberPaintBins);
  if (histograms == (size_t **) NULL)
    {
      linear_image=DestroyImage(linear_image);
      paint_image=DestroyImage(paint_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Oil paint image.
  */
  status=MagickTrue;
  progress=0;
  center=(ssize_t) (GetPixelChannels(linear_image)*(linear_image->columns+
    width)*(width/2L)+GetPixelChannels(linear_image)*(width/2L));
  image_view=AcquireVirtualCacheView(linear_image,exception);
  paint_view=AcquireAuthenticCacheView(paint_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(linear_image,paint_image,linear_image->rows,1)
#endif
  for (y=0; y < (ssize_t) linear_image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    size_t
      *histogram;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,-((ssize_t) width/2L),y-(ssize_t)
      (width/2L),linear_image->columns+width,width,exception);
    q=QueueCacheViewAuthenticPixels(paint_view,0,y,paint_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    histogram=histograms[GetOpenMPThreadId()];
    for (x=0; x < (ssize_t) linear_image->columns; x++)
    {
      ssize_t
        j;

      /*
        Assign most frequent color.
      */
      j=GetMostFrequentOffset(linear_image,p,histogram,width);
      SetOilPaintChannels(linear_image,paint_image,p,q,center,j);
      p+=(ptrdiff_t) GetPixelChannels(linear_image);
      q+=(ptrdiff_t) GetPixelChannels(paint_image);
    }
    if (SyncCacheViewAuthenticPixels(paint_view,exception) == MagickFalse)
      status=MagickFalse;
    if (linear_image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(linear_image,OilPaintImageTag,progress,
          linear_image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  paint_view=DestroyCacheView(paint_view);
  image_view=DestroyCacheView(image_view);
  histograms=DestroyHistogramTLS(histograms);
  linear_image=DestroyImage(linear_image);
  if (status == MagickFalse)
    paint_image=DestroyImage(paint_image);
  return(paint_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     O p a q u e P a i n t I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpaquePaintImage() changes any pixel that matches color with the color
%  defined by fill argument.
%
%  By default color must match a particular pixel color exactly.  However, in
%  many cases two colors may differ by a small amount.  Fuzz defines how much
%  tolerance is acceptable to consider two colors as the same.  For example,
%  set fuzz to 10 and the color red at intensities of 100 and 102 respectively
%  are now interpreted as the same color.
%
%  The format of the OpaquePaintImage method is:
%
%      MagickBooleanType OpaquePaintImage(Image *image,const PixelInfo *target,
%        const PixelInfo *fill,const MagickBooleanType invert,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o target: the RGB value of the target color.
%
%    o fill: the replacement color.
%
%    o invert: paint any pixel that does not match the target color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline void SetUpdatableFillChannels(const Image *image,
  const PixelInfo *conform_fill,Quantum *magick_restrict q)
{
  PixelTrait
    traits;

  traits=GetPixelChannelTraits(image,RedPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelRed(image,(Quantum) conform_fill->red,q);
  traits=GetPixelChannelTraits(image,GreenPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelGreen(image,(Quantum) conform_fill->green,q);
  traits=GetPixelChannelTraits(image,BluePixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelBlue(image,(Quantum) conform_fill->blue,q);
  traits=GetPixelChannelTraits(image,BlackPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelBlack(image,(Quantum) conform_fill->black,q);
  traits=GetPixelChannelTraits(image,AlphaPixelChannel);
  if ((traits & UpdatePixelTrait) != 0)
    SetPixelAlpha(image,(Quantum) conform_fill->alpha,q);
}

MagickExport MagickBooleanType OpaquePaintImage(Image *image,
  const PixelInfo *target,const PixelInfo *fill,const MagickBooleanType invert,
  ExceptionInfo *exception)
{
#define OpaquePaintImageTag  "Opaque/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    conform_fill,
    conform_target,
    zero;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(target != (PixelInfo *) NULL);
  assert(fill != (PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  ConformPixelInfo(image,fill,&conform_fill,exception);
  ConformPixelInfo(image,target,&conform_target,exception);
  /*
    Make image color opaque.
  */
  status=MagickTrue;
  progress=0;
  GetPixelInfo(image,&zero);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    PixelInfo
      pixel;

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
    pixel=zero;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,&conform_target) != invert)
        SetUpdatableFillChannels(image,&conform_fill,q);
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
        proceed=SetImageProgress(image,OpaquePaintImageTag,progress,
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
%     T r a n s p a r e n t P a i n t I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransparentPaintImage() changes the opacity value associated with any pixel
%  that matches color to the value defined by opacity.
%
%  By default color must match a particular pixel color exactly.  However, in
%  many cases two colors may differ by a small amount.  Fuzz defines how much
%  tolerance is acceptable to consider two colors as the same.  For example,
%  set fuzz to 10 and the color red at intensities of 100 and 102 respectively
%  are now interpreted as the same color.
%
%  The format of the TransparentPaintImage method is:
%
%      MagickBooleanType TransparentPaintImage(Image *image,
%        const PixelInfo *target,const Quantum opacity,
%        const MagickBooleanType invert,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o target: the target color.
%
%    o opacity: the replacement opacity value.
%
%    o invert: paint any pixel that does not match the target color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType TransparentPaintImage(Image *image,
  const PixelInfo *target,const Quantum opacity,const MagickBooleanType invert,
  ExceptionInfo *exception)
{
#define TransparentPaintImageTag  "Transparent/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    zero;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(target != (PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  /*
    Make image color transparent.
  */
  status=MagickTrue;
  progress=0;
  GetPixelInfo(image,&zero);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    PixelInfo
      pixel;

    ssize_t
      x;

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
    pixel=zero;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,target) != invert)
        SetPixelAlpha(image,opacity,q);
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
        proceed=SetImageProgress(image,TransparentPaintImageTag,progress,
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
%     T r a n s p a r e n t P a i n t I m a g e C h r o m a                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransparentPaintImageChroma() changes the opacity value associated with any
%  pixel that matches color to the value defined by opacity.
%
%  As there is one fuzz value for the all the channels, TransparentPaintImage()
%  is not suitable for the operations like chroma, where the tolerance for
%  similarity of two color component (RGB) can be different. Thus we define
%  this method to take two target pixels (one low and one high) and all the
%  pixels of an image which are lying between these two pixels are made
%  transparent.
%
%  The format of the TransparentPaintImageChroma method is:
%
%      MagickBooleanType TransparentPaintImageChroma(Image *image,
%        const PixelInfo *low,const PixelInfo *high,const Quantum opacity,
%        const MagickBooleanType invert,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o low: the low target color.
%
%    o high: the high target color.
%
%    o opacity: the replacement opacity value.
%
%    o invert: paint any pixel that does not match the target color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPixelInChromaRange(const PixelInfo *pixel,
  const PixelInfo *low,const PixelInfo *high)
{
  return(((pixel->red >= low->red) && (pixel->red <= high->red) &&
    (pixel->green >= low->green) && (pixel->green <= high->green) &&
    (pixel->blue  >= low->blue) && (pixel->blue <= high->blue)) ? MagickTrue :
    MagickFalse);
}

MagickExport MagickBooleanType TransparentPaintImageChroma(Image *image,
  const PixelInfo *low,const PixelInfo *high,const Quantum opacity,
  const MagickBooleanType invert,ExceptionInfo *exception)
{
#define TransparentPaintImageTag  "Transparent/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(high != (PixelInfo *) NULL);
  assert(low != (PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  /*
    Make image color transparent.
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
    MagickBooleanType
      match;

    PixelInfo
      pixel;

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
    GetPixelInfo(image,&pixel);
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      match=IsPixelInChromaRange(&pixel,low,high);
      if (match != invert)
        SetPixelAlpha(image,opacity,q);
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
        proceed=SetImageProgress(image,TransparentPaintImageTag,progress,
          image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
