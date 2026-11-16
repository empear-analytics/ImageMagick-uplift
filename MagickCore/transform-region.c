/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        TTTTT  RRRR    AAA   N   N  SSSSS  FFFFF   OOO   RRRR   M   M        %
%          T    R   R  A   A  NN  N  SS     F      O   O  R   R  MM MM        %
%          T    RRRR   AAAAA  N N N   SSS   FFF    O   O  RRRR   M M M        %
%          T    R R    A   A  N  NN     SS  F      O   O  R R    M   M        %
%          T    R  R   A   A  N   N  SSSSS  F       OOO   R  R   M   M        %
%                                                                             %
%                  RRRR   EEEEE   GGGG  IIIII   OOO   N   N                   %
%                  R   R  E      G        I    O   O  NN  N                   %
%                  RRRR   EEE    G  GG    I    O   O  N N N                   %
%                  R R    E      G   G    I    O   O  N  NN                   %
%                  R  R   EEEEE   GGG   IIIII   OOO   N   N                   %
%                                                                             %
%                                                                             %
%               MagickCore Image Region and Orientation Methods               %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/attribute.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/memory_.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/property.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resize.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/transform-region-private.h"

/*
  Typedef declarations.
*/
typedef struct _CopyRegion
{
  size_t
    columns,
    rows;

  ssize_t
    sx,
    sy,
    dx,
    dy;
} CopyRegion;


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F l i p I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FlipImage() creates a vertical mirror image by reflecting the pixels
%  around the central x-axis.
%
%  The format of the FlipImage method is:
%
%      Image *FlipImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetFlipPage(Image *flip_image,RectangleInfo page)
{
  if (page.height != 0)
    page.y=((ssize_t) page.height-(ssize_t) flip_image->rows-page.y);
  flip_image->page=page;
}

static Image *FinishFlipImage(const Image *image,Image *flip_image,
  const MagickBooleanType status,ExceptionInfo *exception)
{
  TransformFinish
    finish;

  finish.image=image;
  finish.status=status;
  finish.exception=exception;
  finish.format="flip %.17gx%.17g";
  return(FinishTransformImage(&finish,flip_image));
}

static MagickBooleanType FlipImageLine(const TransformLine *line,
  const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(line->image_view,0,y,line->image->columns,1,
    line->exception);
  q=QueueCacheViewAuthenticPixels(line->transform_view,0,((ssize_t)
    line->transform_image->rows-y-1),line->transform_image->columns,1,
    line->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  CopyPixelRow(line->image,line->transform_image,p,q);
  if (SyncCacheViewAuthenticPixels(line->transform_view,line->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport Image *FlipImage(const Image *image,ExceptionInfo *exception)
{
#define FlipImageTag  "Flip/Image"

  CacheView
    *flip_view,
    *image_view;

  Image
    *flip_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  RectangleInfo
    page;

  ssize_t
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  flip_image=CloneImage(image,0,0,MagickTrue,exception);
  if (flip_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Flip image.
  */
  status=MagickTrue;
  progress=0;
  page=image->page;
  image_view=AcquireVirtualCacheView(image,exception);
  flip_view=AcquireAuthenticCacheView(flip_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,flip_image,flip_image->rows,2)
#endif
  for (y=0; y < (ssize_t) flip_image->rows; y++)
  {
    TransformLine
      line;

    line.image=image;
    line.transform_image=flip_image;
    line.image_view=image_view;
    line.transform_view=flip_view;
    line.exception=exception;
    if (FlipImageLine(&line,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetTransformProgress(image,FlipImageTag,progress,&status);
      }
  }
  flip_view=DestroyCacheView(flip_view);
  image_view=DestroyCacheView(image_view);
  flip_image->type=image->type;
  SetFlipPage(flip_image,page);
  return(FinishFlipImage(image,flip_image,status,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F l o p I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FlopImage() creates a horizontal mirror image by reflecting the pixels
%  around the central y-axis.
%
%  The format of the FlopImage method is:
%
%      Image *FlopImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType FlopImageLine(const TransformLine *line,
  const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(line->image_view,0,y,line->image->columns,1,
    line->exception);
  q=QueueCacheViewAuthenticPixels(line->transform_view,0,y,
    line->transform_image->columns,1,line->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  q+=(ptrdiff_t) GetPixelChannels(line->transform_image)*
    line->transform_image->columns;
  for (x=0; x < (ssize_t) line->transform_image->columns; x++)
  {
    q-=GetPixelChannels(line->transform_image);
    CopyPixelChannelsTo(line->image,line->transform_image,p,q);
    p+=(ptrdiff_t) GetPixelChannels(line->image);
  }
  if (SyncCacheViewAuthenticPixels(line->transform_view,line->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void SetFlopPage(Image *flop_image,RectangleInfo page)
{
  if (page.width != 0)
    page.x=((ssize_t) page.width-(ssize_t) flop_image->columns-page.x);
  flop_image->page=page;
}

MagickExport Image *FlopImage(const Image *image,ExceptionInfo *exception)
{
#define FlopImageTag  "Flop/Image"

  CacheView
    *flop_view,
    *image_view;

  Image
    *flop_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  RectangleInfo
    page;

  ssize_t
    y;

  TransformFinish
    finish;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  flop_image=CloneImage(image,0,0,MagickTrue,exception);
  if (flop_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Flop each row.
  */
  status=MagickTrue;
  progress=0;
  page=image->page;
  image_view=AcquireVirtualCacheView(image,exception);
  flop_view=AcquireAuthenticCacheView(flop_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,flop_image,flop_image->rows,2)
#endif
  for (y=0; y < (ssize_t) flop_image->rows; y++)
  {
    TransformLine
      line;

    line.image=image;
    line.transform_image=flop_image;
    line.image_view=image_view;
    line.transform_view=flop_view;
    line.exception=exception;
    if (FlopImageLine(&line,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetTransformProgress(image,FlopImageTag,progress,&status);
      }
  }
  flop_view=DestroyCacheView(flop_view);
  image_view=DestroyCacheView(image_view);
  flop_image->type=image->type;
  SetFlopPage(flop_image,page);
  finish.image=image;
  finish.status=status;
  finish.exception=exception;
  finish.format="flop %.17gx%.17g";
  return(FinishTransformImage(&finish,flop_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R o l l I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RollImage() offsets an image as defined by x_offset and y_offset.
%
%  The format of the RollImage method is:
%
%      Image *RollImage(const Image *image,const ssize_t x_offset,
%        const ssize_t y_offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x_offset: the number of columns to roll in the horizontal direction.
%
%    o y_offset: the number of rows to roll in the vertical direction.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _CopyRegionRow
{
  CacheView
    *source_view,
    *destination_view;

  const Image
    *source;

  Image
    *destination;

  ExceptionInfo
    *exception;

  size_t
    columns;

  ssize_t
    sx,
    sy,
    dx,
    dy,
    y;
} CopyRegionRow;

static MagickBooleanType CopyRegionScanline(const CopyRegionRow *row)
{
  MagickBooleanType
    status,
    sync;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  status=MagickTrue;
  p=GetCacheViewVirtualPixels(row->source_view,row->sx,row->sy+row->y,
    row->columns,1,row->exception);
  q=GetCacheViewAuthenticPixels(row->destination_view,row->dx,row->dy+row->y,
    row->columns,1,row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      status=MagickFalse;
      return(status);
    }
  for (x=0; x < (ssize_t) row->columns; x++)
  {
    CopyPixelChannelsTo(row->source,row->destination,p,q);
    p+=(ptrdiff_t) GetPixelChannels(row->source);
    q+=(ptrdiff_t) GetPixelChannels(row->destination);
  }
  sync=SyncCacheViewAuthenticPixels(row->destination_view,row->exception);
  if (sync == MagickFalse)
    status=MagickFalse;
  return(status);
}

static MagickBooleanType CopyImageRegion(Image *destination,const Image *source,
  const CopyRegion *region,ExceptionInfo *exception)
{
  const size_t
    columns = region->columns,
    rows = region->rows;

  const ssize_t
    sx = region->sx,
    sy = region->sy,
    dx = region->dx,
    dy = region->dy;

  CacheView
    *source_view,
    *destination_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  if (columns == 0)
    return(MagickTrue);
  status=MagickTrue;
  source_view=AcquireVirtualCacheView(source,exception);
  destination_view=AcquireAuthenticCacheView(destination,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(source,destination,rows,2)
#endif
  for (y=0; y < (ssize_t) rows; y++)
  {
    CopyRegionRow
      row;

    /*
      Transfer scanline.
    */
    if (status == MagickFalse)
      continue;
    row.source_view=source_view;
    row.destination_view=destination_view;
    row.source=source;
    row.destination=destination;
    row.exception=exception;
    row.columns=columns;
    row.sx=sx;
    row.sy=sy;
    row.dx=dx;
    row.dy=dy;
    row.y=y;
    if (CopyRegionScanline(&row) == MagickFalse)
      status=MagickFalse;
  }
  destination_view=DestroyCacheView(destination_view);
  source_view=DestroyCacheView(source_view);
  return(status);
}

static ssize_t WrapRollOffset(ssize_t offset,const size_t extent)
{
  while (offset < 0)
    offset+=(ssize_t) extent;
  while (offset >= (ssize_t) extent)
    offset-=(ssize_t) extent;
  return(offset);
}

static void SetRollOffset(const Image *image,const ssize_t x_offset,
  const ssize_t y_offset,RectangleInfo *offset)
{
  (*offset).x=x_offset;
  (*offset).y=y_offset;
  (*offset).x=WrapRollOffset((*offset).x,image->columns);
  (*offset).y=WrapRollOffset((*offset).y,image->rows);
}

static Image *FinishRollImage(const Image *image,Image *roll_image,
  const MagickStatusType status)
{
  roll_image->type=image->type;
  if (status == MagickFalse)
    roll_image=DestroyImage(roll_image);
  return(roll_image);
}

MagickExport Image *RollImage(const Image *image,const ssize_t x_offset,
  const ssize_t y_offset,ExceptionInfo *exception)
{
#define RollImageTag  "Roll/Image"

  CopyRegion
    first_region,
    fourth_region,
    second_region,
    third_region;

  Image
    *roll_image;

  MagickStatusType
    status;

  RectangleInfo
    offset;

  /*
    Initialize roll image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  roll_image=CloneImage(image,0,0,MagickTrue,exception);
  if (roll_image == (Image *) NULL)
    return((Image *) NULL);
  SetRollOffset(image,x_offset,y_offset,&offset);
  /*
    Roll image.
  */
  first_region.columns=(size_t) offset.x;
  first_region.rows=(size_t) offset.y;
  first_region.sx=(ssize_t) image->columns-offset.x;
  first_region.sy=(ssize_t) image->rows-offset.y;
  first_region.dx=0;
  first_region.dy=0;
  status=CopyImageRegion(roll_image,image,&first_region,exception);
  (void) SetImageProgress(image,RollImageTag,0,3);
  second_region.columns=(size_t) ((ssize_t) image->columns-offset.x);
  second_region.rows=(size_t) offset.y;
  second_region.sx=0;
  second_region.sy=(ssize_t) image->rows-offset.y;
  second_region.dx=offset.x;
  second_region.dy=0;
  status&=(MagickStatusType) CopyImageRegion(roll_image,image,&second_region,
    exception);
  (void) SetImageProgress(image,RollImageTag,1,3);
  third_region.columns=(size_t) offset.x;
  third_region.rows=(size_t) ((ssize_t) image->rows-offset.y);
  third_region.sx=(ssize_t) image->columns-offset.x;
  third_region.sy=0;
  third_region.dx=0;
  third_region.dy=offset.y;
  status&=(MagickStatusType) CopyImageRegion(roll_image,image,&third_region,
    exception);
  (void) SetImageProgress(image,RollImageTag,2,3);
  fourth_region.columns=(size_t) ((ssize_t) image->columns-offset.x);
  fourth_region.rows=(size_t) ((ssize_t) image->rows-offset.y);
  fourth_region.sx=0;
  fourth_region.sy=0;
  fourth_region.dx=offset.x;
  fourth_region.dy=offset.y;
  status&=(MagickStatusType) CopyImageRegion(roll_image,image,&fourth_region,
    exception);
  (void) SetImageProgress(image,RollImageTag,3,3);
  return(FinishRollImage(image,roll_image,status));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s p o s e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransposeImage() creates a horizontal mirror image by reflecting the pixels
%  around the central y-axis while rotating them by 90 degrees.
%
%  The format of the TransposeImage method is:
%
%      Image *TransposeImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define TransposeImageTag  "Transpose/Image"

typedef struct _TransposeRows
{
  const Image
    *image;

  Image
    *transpose_image;

  CacheView
    *image_view,
    *transpose_view;

  ExceptionInfo
    *exception;
} TransposeRows;

static MagickBooleanType TransposeImageRows(const TransposeRows *rows,
  MagickBooleanType status,MagickOffsetType progress)
{
  const Image
    *const image = rows->image;

  Image
    *const transpose_image = rows->transpose_image;

  CacheView
    *const image_view = rows->image_view,
    *const transpose_view = rows->transpose_view;

  ExceptionInfo
    *const exception = rows->exception;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,transpose_image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,(ssize_t) image->rows-y-1,
      image->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(transpose_view,(ssize_t) image->rows-y-1,
      0,1,transpose_image->rows,exception);
    if (IsPixelRowMissing(p,q) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      CopyPixelChannelsTo(image,transpose_image,p,q);
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(transpose_image);
    }
    SyncTransformView(transpose_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetTransformProgress(image,TransposeImageTag,progress,&status);
      }
  }
  return(status);
}

MagickExport Image *TransposeImage(const Image *image,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *transpose_view;

  Image
    *transpose_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  RectangleInfo
    page;

  TransposeRows
    rows;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  transpose_image=CloneImage(image,image->rows,image->columns,MagickTrue,
    exception);
  if (transpose_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Transpose image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  transpose_view=AcquireAuthenticCacheView(transpose_image,exception);
  rows.image=image;
  rows.transpose_image=transpose_image;
  rows.image_view=image_view;
  rows.transpose_view=transpose_view;
  rows.exception=exception;
  status=TransposeImageRows(&rows,status,progress);
  transpose_view=DestroyCacheView(transpose_view);
  image_view=DestroyCacheView(image_view);
  transpose_image->type=image->type;
  page=transpose_image->page;
  Swap(page.width,page.height);
  Swap(page.x,page.y);
  transpose_image->page=page;
  if (status != MagickFalse)
    AppendTransformProperty(image,transpose_image,"transpose %.17gx%.17g",
      exception);
  if (status == MagickFalse)
    transpose_image=DestroyImage(transpose_image);
  return(transpose_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s v e r s e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransverseImage() creates a vertical mirror image by reflecting the pixels
%  around the central x-axis while rotating them by 270 degrees.
%
%  The format of the TransverseImage method is:
%
%      Image *TransverseImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetTransversePage(Image *transverse_image)
{
  RectangleInfo
    page;

  page=transverse_image->page;
  Swap(page.width,page.height);
  Swap(page.x,page.y);
  if (page.width != 0)
    page.x=(ssize_t) page.width-(ssize_t) transverse_image->columns-page.x;
  if (page.height != 0)
    page.y=(ssize_t) page.height-(ssize_t) transverse_image->rows-page.y;
  transverse_image->page=page;
}

static void TransversePixelRow(const Image *image,
  const Image *transverse_image,const Quantum *p,Quantum *q)
{
  ssize_t
    x;

  q+=(ptrdiff_t) GetPixelChannels(transverse_image)*image->columns;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    q-=GetPixelChannels(transverse_image);
    CopyPixelChannelsTo(image,transverse_image,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static Image *FinishTransverseImage(const Image *image,
  Image *transverse_image,const MagickBooleanType status,
  ExceptionInfo *exception)
{
  transverse_image->type=image->type;
  SetTransversePage(transverse_image);
  if (status != MagickFalse)
    AppendTransformProperty(image,transverse_image,"transverse %.17gx%.17g",
      exception);
  if (status == MagickFalse)
    transverse_image=DestroyImage(transverse_image);
  return(transverse_image);
}

static MagickBooleanType SyncTransverseRow(CacheView *transverse_view,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status,
    sync;

  status=MagickTrue;
  sync=SyncCacheViewAuthenticPixels(transverse_view,exception);
  if (sync == MagickFalse)
    status=MagickFalse;
  return(status);
}

typedef struct _TransverseRow
{
  const Image
    *image;

  Image
    *transverse_image;

  CacheView
    *image_view,
    *transverse_view;

  ExceptionInfo
    *exception;

  ssize_t
    y;
} TransverseRow;

static MagickBooleanType FetchTransverseRow(const TransverseRow *row,
  const Quantum *magick_restrict *p,Quantum *magick_restrict *q,
  MagickBooleanType *status)
{
  const Image
    *const image = row->image;

  Image
    *const transverse_image = row->transverse_image;

  CacheView
    *const image_view = row->image_view,
    *const transverse_view = row->transverse_view;

  ExceptionInfo
    *const exception = row->exception;

  const ssize_t
    y = row->y;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*p)=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,
    exception);
  (*q)=QueueCacheViewAuthenticPixels(transverse_view,
    (ssize_t) image->rows-y-1,0,1,transverse_image->rows,exception);
  if (IsPixelRowMissing((*p),(*q)) != MagickFalse)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickExport Image *TransverseImage(const Image *image,ExceptionInfo *exception)
{
#define TransverseImageTag  "Transverse/Image"

  CacheView
    *image_view,
    *transverse_view;

  Image
    *transverse_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  transverse_image=CloneImage(image,image->rows,image->columns,MagickTrue,
    exception);
  if (transverse_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Transverse image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  transverse_view=AcquireAuthenticCacheView(transverse_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,transverse_image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    const TransverseRow
      row = { image, transverse_image, image_view, transverse_view,
        exception, y };

    if (FetchTransverseRow(&row,&p,&q,&status) == MagickFalse)
      continue;
    TransversePixelRow(image,transverse_image,p,q);
    if (SyncTransverseRow(transverse_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetTransformProgress(image,TransverseImageTag,progress,&status);
      }
  }
  transverse_view=DestroyCacheView(transverse_view);
  image_view=DestroyCacheView(image_view);
  return(FinishTransverseImage(image,transverse_image,status,exception));
}
