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
%        FFFFF  L       OOO    OOO   DDDD   FFFFF  IIIII  L      L            %
%        F      L      O   O  O   O  D   D  F        I    L      L            %
%        FFF    L      O   O  O   O  D   D  FFF      I    L      L            %
%        F      L      O   O  O   O  D   D  F        I    L      L            %
%        F      LLLLL   OOO    OOO   DDDD   F      IIIII  LLLLL  LLLLL        %
%                                                                             %
%                                                                             %
%                     MagickCore Floodfill Paint Methods                      %
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

static MagickBooleanType FillFloodplaneLeft(const FloodfillRow *row,
  const ssize_t x1,ssize_t *x)
{
  const Quantum
    *magick_restrict p;

  PixelInfo
    pixel;

  Quantum
    *magick_restrict q;

  p=GetCacheViewVirtualPixels(row->image_view,0,row->y,(size_t) (x1+1),1,
    row->exception);
  q=GetCacheViewAuthenticPixels(row->floodplane_view,0,row->y,(size_t) (x1+1),1,
    row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    return(MagickFalse);
  p+=(ptrdiff_t) x1*(ssize_t) GetPixelChannels(row->image);
  q+=(ptrdiff_t) x1*(ssize_t) GetPixelChannels(row->floodplane_image);
  for (*x=x1; *x >= 0; (*x)--)
  {
    if (GetPixelGray(row->floodplane_image,q) != 0)
      break;
    GetPixelInfoPixel(row->image,p,&pixel);
    if (IsFuzzyEquivalencePixelInfo(&pixel,row->target) == row->invert)
      break;
    SetPixelGray(row->floodplane_image,QuantumRange,q);
    p-=(ptrdiff_t)GetPixelChannels(row->image);
    q-=GetPixelChannels(row->floodplane_image);
  }
  if (SyncCacheViewAuthenticPixels(row->floodplane_view,row->exception) ==
      MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _FloodfillTileView
{
  Image
    *image,
    *floodplane_image;

  CacheView
    *floodplane_view,
    *image_view;

  const DrawInfo
    *draw_info;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} FloodfillTileView;

static MagickBooleanType TileFloodplaneView(const FloodfillTileView *view)
{
  const Quantum
    *magick_restrict p;

  FloodfillTile
    tile;

  MagickBooleanType
    status;

  Quantum
    *magick_restrict q;

  status=MagickTrue;
  p=GetCacheViewVirtualPixels(view->floodplane_view,0,view->y,
    view->image->columns,1,view->exception);
  q=GetCacheViewAuthenticPixels(view->image_view,0,view->y,view->image->columns,
    1,view->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      status=MagickFalse;
      return(status);
    }
  tile.image=view->image;
  tile.floodplane_image=view->floodplane_image;
  tile.draw_info=view->draw_info;
  tile.y=view->y;
  tile.exception=view->exception;
  TileFloodplaneRow(&tile,p,q);
  if (SyncCacheViewAuthenticPixels(view->image_view,view->exception) ==
      MagickFalse)
    status=MagickFalse;
  return(status);
}

typedef struct _FloodfillAdvance
{
  MagickBooleanType
    *skip;

  ssize_t
    *start,
    *x;
} FloodfillAdvance;

static MagickBooleanType AdvanceFloodfillRun(const FloodfillRow *row,
  const ssize_t x2,const FloodfillAdvance *advance)
{
  MagickBooleanType
    *const skip = advance->skip;

  ssize_t
    *const start = advance->start,
    *const x = advance->x;

  (*skip)=MagickFalse;
  (*x)++;
  if ((*x) <= x2)
    {
      if (SkipFloodplaneRun(row,x2,x) == MagickFalse)
        return(MagickFalse);
    }
  (*start)=(*x);
  return(MagickTrue);
}

static void TileFloodplaneLine(const FloodfillTileView *view,
  MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return;
  if (TileFloodplaneView(view) == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType CheckFloodfillOffsets(const Image *image,
  const ssize_t x_offset,const ssize_t y_offset)
{
  if ((x_offset < 0) || (x_offset >= (ssize_t) image->columns))
    return(MagickFalse);
  if ((y_offset < 0) || (y_offset >= (ssize_t) image->rows))
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _FloodfillPlane
{
  Image
    *image,
    *floodplane_image;

  CacheView
    *floodplane_view,
    *image_view;

  const DrawInfo
    *draw_info;

  ExceptionInfo
    *exception;
} FloodfillPlane;

static MagickBooleanType TileFloodplaneImage(const FloodfillPlane *plane)
{
  CacheView
    *const floodplane_view = plane->floodplane_view,
    *const image_view = plane->image_view;

  const DrawInfo
    *const draw_info = plane->draw_info;

  ExceptionInfo
    *const exception = plane->exception;

  Image
    *const floodplane_image = plane->floodplane_image,
    *const image = plane->image;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(floodplane_image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const FloodfillTileView
      view = { image, floodplane_image, floodplane_view, image_view,
        draw_info, y, exception };

    /*
      Tile fill color onto floodplane.
    */
    TileFloodplaneLine(&view,&status);
  }
  return(status);
}

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

typedef struct _FloodfillStack
{
  Image
    *image,
    *floodplane_image;

  CacheView
    *floodplane_view,
    *image_view;

  ExceptionInfo
    *exception;

  MemoryInfo
    *segment_info;

  SegmentInfo
    *segment_stack,
    **top;
} FloodfillStack;

typedef struct _FloodfillSegment
{
  ssize_t
    up,
    left,
    right,
    delta;
} FloodfillSegment;

static MagickBooleanType PushFloodfillSegment(const FloodfillStack *stack,
  const FloodfillSegment *segment)
{
  CacheView
    *floodplane_view = stack->floodplane_view,
    *image_view = stack->image_view;

  const ssize_t
    delta = segment->delta,
    left = segment->left,
    right = segment->right,
    up = segment->up;

  ExceptionInfo
    *const exception = stack->exception;

  Image
    *floodplane_image = stack->floodplane_image,
    *const image = stack->image;

  MemoryInfo
    *segment_info = stack->segment_info;

  SegmentInfo
    *s = *stack->top;

  SegmentInfo
    *const segment_stack = stack->segment_stack;

  PushSegmentStack(up,left,right,delta);
  *stack->top=s;
  return(MagickTrue);
}

typedef struct _FloodfillSegments
{
  Image
    *image,
    *floodplane_image;

  CacheView
    *floodplane_view,
    *image_view;

  const PixelInfo
    *target;

  MagickBooleanType
    invert;

  ExceptionInfo
    *exception;

  MemoryInfo
    *segment_info;

  SegmentInfo
    *segment_stack,
    *s;

  ssize_t
    start,
    x_offset,
    y_offset;
} FloodfillSegments;

static MagickBooleanType FillFloodplaneSegments(
  const FloodfillSegments *segments)
{
  FloodfillStack
    stack;

  MagickBooleanType
    skip;

  SegmentInfo
    *s = segments->s;

  ssize_t
    offset,
    start = segments->start,
    x1,
    x2,
    y;

  stack.image=segments->image;
  stack.floodplane_image=segments->floodplane_image;
  stack.floodplane_view=segments->floodplane_view;
  stack.image_view=segments->image_view;
  stack.exception=segments->exception;
  stack.segment_info=segments->segment_info;
  stack.segment_stack=segments->segment_stack;
  stack.top=&s;
  {
    const FloodfillSegment
      segment = { segments->y_offset, segments->x_offset, segments->x_offset,
        1 };

    if (PushFloodfillSegment(&stack,&segment) == MagickFalse)
      return(MagickFalse);
  }
  {
    const FloodfillSegment
      segment = { segments->y_offset+1, segments->x_offset,
        segments->x_offset, -1 };

    if (PushFloodfillSegment(&stack,&segment) == MagickFalse)
      return(MagickFalse);
  }
  while (s > segments->segment_stack)
  {
    FloodfillRow
      row;

    ssize_t
      x;

    const FloodfillAdvance
      advance = { &skip, &start, &x };

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
    row.image=segments->image;
    row.floodplane_image=segments->floodplane_image;
    row.image_view=segments->image_view;
    row.floodplane_view=segments->floodplane_view;
    row.target=segments->target;
    row.invert=segments->invert;
    row.y=y;
    row.exception=segments->exception;
    if (FillFloodplaneLeft(&row,x1,&x) == MagickFalse)
      break;
    skip=x >= x1 ? MagickTrue : MagickFalse;
    if (skip == MagickFalse)
      {
        start=x+1;
        if (start < x1)
          {
            const FloodfillSegment
              segment = { y, start, x1-1, -offset };

            if (PushFloodfillSegment(&stack,&segment) == MagickFalse)
              return(MagickFalse);
          }
        x=x1+1;
      }
    do
    {
      if (skip == MagickFalse)
        {
          if (FillFloodplaneRun(&row,&x) == MagickFalse)
            break;
          {
            const FloodfillSegment
              segment = { y, start, x-1, offset };

            if (PushFloodfillSegment(&stack,&segment) == MagickFalse)
              return(MagickFalse);
          }
          if (x > (x2+1))
            {
              const FloodfillSegment
                segment = { y, x2+1, x-1, -offset };

              if (PushFloodfillSegment(&stack,&segment) == MagickFalse)
                return(MagickFalse);
            }
        }
      if (AdvanceFloodfillRun(&row,x2,&advance) == MagickFalse)
        break;
    } while (x <= x2);
  }
  return(MagickTrue);
}

MagickExport MagickBooleanType FloodfillPaintImage(Image *image,
  const DrawInfo *draw_info,const PixelInfo *target,const ssize_t x_offset,
  const ssize_t y_offset,const MagickBooleanType invert,
  ExceptionInfo *exception)
{
  CacheView
    *floodplane_view,
    *image_view;

  Image
    *floodplane_image;

  MagickBooleanType
    status;

  FloodfillPlane
    plane;

  MemoryInfo
    *segment_info;

  PixelInfo
    pixel;

  SegmentInfo
    *s;

  SegmentInfo
    *segment_stack;

  ssize_t
    start;

  /*
    Check boundary conditions.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse) 
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CheckFloodfillOffsets(image,x_offset,y_offset) == MagickFalse)
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
  {
    const FloodfillSegments
      segments = { image, floodplane_image, floodplane_view, image_view,
        target, invert, exception, segment_info, segment_stack, s, start,
        x_offset, y_offset };

    if (FillFloodplaneSegments(&segments) == MagickFalse)
      return(MagickFalse);
  }
  plane.image=image;
  plane.floodplane_image=floodplane_image;
  plane.floodplane_view=floodplane_view;
  plane.image_view=image_view;
  plane.draw_info=draw_info;
  plane.exception=exception;
  status=TileFloodplaneImage(&plane);
  floodplane_view=DestroyCacheView(floodplane_view);
  image_view=DestroyCacheView(image_view);
  segment_info=RelinquishVirtualMemory(segment_info);
  floodplane_image=DestroyImage(floodplane_image);
  return(status);
}
