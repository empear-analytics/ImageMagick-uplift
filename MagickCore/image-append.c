/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                   AAA   PPPP   PPPP   EEEEE  N   N  DDDD                    %
%                  A   A  P   P  P   P  E      NN  N  D   D                   %
%                  AAAAA  PPPP   PPPP   EEE    N N N  D   D                   %
%                  A   A  P      P      E      N  NN  D   D                   %
%                  A   A  P      P      EEEEE  N   N  DDDD                    %
%                                                                             %
%                                                                             %
%                           Append, Smush and Mask                            %
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
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/display.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/magick-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/random_.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/timer.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/xwindow-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A p p e n d I m a g e s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AppendImages() takes all images from the current image pointer to the end
%  of the image list and appends them to each other top-to-bottom if the
%  stack parameter is true, otherwise left-to-right.
%
%  The current gravity setting effects how the image is justified in the
%  final image.
%
%  The format of the AppendImages method is:
%
%      Image *AppendImages(const Image *images,const MagickBooleanType stack,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image sequence.
%
%    o stack: A value other than 0 stacks the images top-to-bottom.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType SetAppendImageClass(Image *append_image,
  const ImageType image_type,const MagickBooleanType homogeneous_colorspace,
  ExceptionInfo *exception)
{
  if (image_type != BilevelType)
    {
      if (SetImageStorageClass(append_image,DirectClass,exception) == MagickFalse)
        {
          append_image=DestroyImage(append_image);
          return(MagickFalse);
        }
      if (homogeneous_colorspace == MagickFalse)
        (void) SetImageColorspace(append_image,sRGBColorspace,exception);
    }
  return(MagickTrue);
}

static void AdvanceAppendOffset(const Image *next,const MagickBooleanType stack,
  ssize_t *x_offset,ssize_t *y_offset)
{
  if (stack == MagickFalse)
    {
      *x_offset+=(ssize_t) next->columns;
      *y_offset=0;
    }
  else
    {
      *x_offset=0;
      *y_offset+=(ssize_t) next->rows;
    }
}

static void AddAppendExtent(const Image *next,const MagickBooleanType stack,
  size_t *width,size_t *height)
{
  if (stack != MagickFalse)
    {
      if (next->columns > *width)
        *width=next->columns;
      *height+=next->rows;
      return;
    }
  *width+=next->columns;
  if (next->rows > *height)
    *height=next->rows;
}

typedef struct _AppendRow
{
  const Image
    *next;

  Image
    *append_image;

  CacheView
    *append_view,
    *image_view;

  ssize_t
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;
} AppendRow;

static void AppendImageRow(const AppendRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  const Image
    *next = row->next;

  Image
    *append_image = row->append_image;

  CacheView
    *append_view = row->append_view,
    *image_view = row->image_view;

  const ssize_t
    x_offset = row->x_offset,
    y_offset = row->y_offset;

  ExceptionInfo
    *exception = row->exception;

  MagickBooleanType
    sync;

  PixelInfo
    pixel;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(image_view,0,y,next->columns,1,exception);
  q=QueueCacheViewAuthenticPixels(append_view,x_offset,y+y_offset,
    next->columns,1,exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  GetPixelInfo(next,&pixel);
  for (x=0; x < (ssize_t) next->columns; x++)
  {
    GetPixelInfoPixel(next,p,&pixel);
    SetPixelViaPixelInfo(append_image,&pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(next);
    q+=(ptrdiff_t) GetPixelChannels(append_image);
  }
  sync=SyncCacheViewAuthenticPixels(append_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

static void AccumulateAppendDepth(const Image *next,const Image *images,
  size_t *depth,ImageType *image_type)
{
  if (next->depth > (*depth))
    (*depth)=next->depth;
  if (next->type != images->type)
    (*image_type)=UndefinedType;
}

static void AccumulateAppendTraits(const Image *next,const Image *images,
  MagickBooleanType *homogeneous_colorspace,PixelTrait *alpha_trait)
{
  if (next->colorspace != images->colorspace)
    (*homogeneous_colorspace)=MagickFalse;
  if (next->alpha_trait != UndefinedPixelTrait)
    (*alpha_trait)=BlendPixelTrait;
}

static Image *DestroyImageOnFailure(Image *image,
  const MagickBooleanType status)
{
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

static void SubtractAppendGeometry(const RectangleInfo *geometry,
  const MagickBooleanType stack,ssize_t *x_offset,ssize_t *y_offset)
{
  if (stack != MagickFalse)
    (*x_offset)-=geometry->x;
  else
    (*y_offset)-=geometry->y;
}

static void CheckAppendArguments(const Image *images,
  ExceptionInfo *exception)
{
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
}

MagickExport Image *AppendImages(const Image *images,
  const MagickBooleanType stack,ExceptionInfo *exception)
{
#define AppendImageTag  "Append/Image"

  CacheView
    *append_view;

  Image
    *append_image;

  ImageType
    image_type;

  MagickBooleanType
    homogeneous_colorspace,
    status;

  MagickOffsetType
    n;

  PixelTrait
    alpha_trait;

  RectangleInfo
    geometry;

  const Image
    *next;

  size_t
    depth,
    height,
    number_images,
    width;

  ssize_t
    x_offset,
    y,
    y_offset;

  /*
    Compute maximum area of appended area.
  */
  CheckAppendArguments(images,exception);
  alpha_trait=images->alpha_trait;
  number_images=1;
  width=images->columns;
  height=images->rows;
  depth=images->depth;
  image_type=images->type;
  homogeneous_colorspace=MagickTrue;
  next=GetNextImageInList(images);
  for ( ; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    AccumulateAppendDepth(next,images,&depth,&image_type);
    AccumulateAppendTraits(next,images,&homogeneous_colorspace,&alpha_trait);
    number_images++;
    AddAppendExtent(next,stack,&width,&height);
  }
  /*
    Append images.
  */
  append_image=CloneImage(images,width,height,MagickTrue,exception);
  if (append_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetAppendImageClass(append_image,image_type,homogeneous_colorspace,
      exception) == MagickFalse)
    return((Image *) NULL);
  append_image->depth=depth;
  append_image->alpha_trait=alpha_trait;
  append_image->page=images->page;
  (void) SetImageBackgroundColor(append_image,exception);
  status=MagickTrue;
  x_offset=0;
  y_offset=0;
  next=images;
  append_view=AcquireAuthenticCacheView(append_image,exception);
  for (n=0; n < (MagickOffsetType) number_images; n++)
  {
    CacheView
      *image_view;

    MagickBooleanType
      proceed;

    SetGeometry(append_image,&geometry);
    GravityAdjustGeometry(next->columns,next->rows,next->gravity,&geometry);
    SubtractAppendGeometry(&geometry,stack,&x_offset,&y_offset);
    image_view=AcquireVirtualCacheView(next,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp parallel for schedule(static) shared(status) \
      magick_number_threads(next,next,next->rows,2)
#endif
    for (y=0; y < (ssize_t) next->rows; y++)
    {
      const AppendRow row = { next, append_image, append_view, image_view,
        x_offset, y_offset, exception };

      AppendImageRow(&row,y,&status);
    }
    image_view=DestroyCacheView(image_view);
    AdvanceAppendOffset(next,stack,&x_offset,&y_offset);
    proceed=SetImageProgress(append_image,AppendImageTag,n,number_images);
    if (proceed == MagickFalse)
      break;
    next=GetNextImageInList(next);
  }
  append_view=DestroyCacheView(append_view);
  return(DestroyImageOnFailure(append_image,status));
}

static MagickBooleanType HasMaskChannel(const Image *image,
  const PixelMask type)
{
  switch (type)
  {
    case ReadPixelMask:
    {
      if ((image->channels & ReadMaskChannel) == 0)
        return(MagickFalse);
      break;
    }
    case WritePixelMask:
    {
      if ((image->channels & WriteMaskChannel) == 0)
        return(MagickFalse);
      break;
    }
    default:
    {
      if ((image->channels & CompositeMaskChannel) == 0)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e M a s k                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageMask() returns the mask associated with the image.
%
%  The format of the GetImageMask method is:
%
%      Image *GetImageMask(const Image *image,const PixelMask type,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o type: the mask type, ReadPixelMask or WritePixelMask.
%
*/
typedef struct _GetMaskRow
{
  const Image
    *image;

  Image
    *mask_image;

  CacheView
    *image_view,
    *mask_view;

  PixelMask
    type;

  ExceptionInfo
    *exception;
} GetMaskRow;

static void ConvertMaskRow(const GetMaskRow *row,const Quantum *p,
  Quantum *q)
{
  const Image
    *image = row->image;

  Image
    *mask_image = row->mask_image;

  const PixelMask
    type = row->type;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    switch (type)
    {
      case ReadPixelMask:
      {
        SetPixelGray(mask_image,GetPixelReadMask(image,p),q);
        break;
      }
      case WritePixelMask:
      {
        SetPixelGray(mask_image,GetPixelWriteMask(image,p),q);
        break;
      }
      default:
      {
        SetPixelGray(mask_image,GetPixelCompositeMask(image,p),q);
        break;
      }
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(mask_image);
  }
}

static void GetImageMaskRow(const GetMaskRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  const Image
    *image = row->image;

  Image
    *mask_image = row->mask_image;

  CacheView
    *image_view = row->image_view,
    *mask_view = row->mask_view;

  ExceptionInfo
    *exception = row->exception;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=GetCacheViewAuthenticPixels(mask_view,0,y,mask_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  ConvertMaskRow(row,p,q);
  if (SyncCacheViewAuthenticPixels(mask_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport Image *GetImageMask(const Image *image,const PixelMask type,
  ExceptionInfo *exception)
{
  CacheView
    *mask_view,
    *image_view;

  Image
    *mask_image;

  MagickBooleanType
    status;

  ssize_t
    y;

  /*
    Get image mask.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (HasMaskChannel(image,type) == MagickFalse)
    return((Image *) NULL);
  mask_image=AcquireImage((ImageInfo *) NULL,exception);
  status=SetImageExtent(mask_image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    return(DestroyImage(mask_image));
  status=MagickTrue;
  mask_image->alpha_trait=UndefinedPixelTrait;
  (void) SetImageColorspace(mask_image,GRAYColorspace,exception);
  image_view=AcquireVirtualCacheView(image,exception);
  mask_view=AcquireAuthenticCacheView(mask_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const GetMaskRow row = { image, mask_image, image_view, mask_view, type,
      exception };

    GetImageMaskRow(&row,y,&status);
  }
  mask_view=DestroyCacheView(mask_view);
  image_view=DestroyCacheView(image_view);
  return(DestroyImageOnFailure(mask_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S m u s h I m a g e s                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SmushImages() takes all images from the current image pointer to the end
%  of the image list and smushes them to each other top-to-bottom if the
%  stack parameter is true, otherwise left-to-right.
%
%  The current gravity setting now effects how the image is justified in the
%  final image.
%
%  The format of the SmushImages method is:
%
%      Image *SmushImages(const Image *images,const MagickBooleanType stack,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image sequence.
%
%    o stack: A value other than 0 stacks the images top-to-bottom.
%
%    o offset: minimum distance in pixels between images.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsLeftGapEdge(const Image *left_image,
  const Quantum *p,const ssize_t x,const size_t gap)
{
  return(((p == (const Quantum *) NULL) ||
    (GetPixelAlpha(left_image,p) != TransparentAlpha) ||
    (((ssize_t) left_image->columns-x-1) >= (ssize_t) gap)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsRightGapEdge(const Image *right_image,
  const Quantum *p,const ssize_t x,const ssize_t i,const size_t gap)
{
  return(((p == (const Quantum *) NULL) ||
    (GetPixelAlpha(right_image,p) != TransparentAlpha) ||
    ((x+i) >= (ssize_t) gap)) ? MagickTrue : MagickFalse);
}

typedef struct _SmushXRows
{
  const Image
    *left_image,
    *right_image;

  CacheView
    *left_view,
    *right_view;

  RectangleInfo
    left_geometry,
    right_geometry;

  ExceptionInfo
    *exception;
} SmushXRows;

static ssize_t FindLeftGapEdge(const SmushXRows *rows,const ssize_t y,
  const size_t gap)
{
  const Quantum
    *p;

  ssize_t
    x;

  for (x=(ssize_t) rows->left_image->columns-1; x > 0; x--)
  {
    p=GetCacheViewVirtualPixels(rows->left_view,x,rows->left_geometry.y+y,1,1,
      rows->exception);
    if (IsLeftGapEdge(rows->left_image,p,x,gap) != MagickFalse)
      break;
  }
  return(x);
}

static ssize_t FindRightGapEdge(const SmushXRows *rows,const ssize_t y,
  const ssize_t i,const size_t gap)
{
  const Quantum
    *p;

  ssize_t
    x;

  for (x=0; x < (ssize_t) rows->right_image->columns; x++)
  {
    p=GetCacheViewVirtualPixels(rows->right_view,x,rows->right_geometry.y+y,1,1,
      rows->exception);
    if (IsRightGapEdge(rows->right_image,p,x,i,gap) != MagickFalse)
      break;
  }
  return(x);
}

static size_t SmushXRow(const SmushXRows *rows,const ssize_t y,size_t gap)
{
  ssize_t
    i,
    x;

  x=FindLeftGapEdge(rows,y,gap);
  i=(ssize_t) rows->left_image->columns-x-1;
  x=FindRightGapEdge(rows,y,i,gap);
  if ((x+i) < (ssize_t) gap)
    gap=(size_t) (x+i);
  return(gap);
}

static ssize_t SmushXGap(const Image *smush_image,const Image *images,
  const ssize_t offset,ExceptionInfo *exception)
{
  CacheView
    *left_view,
    *right_view;

  const Image
    *left_image,
    *right_image;

  RectangleInfo
    left_geometry,
    right_geometry;

  size_t
    gap;

  SmushXRows
    rows;

  ssize_t
    y;

  if (images->previous == (Image *) NULL)
    return(0);
  right_image=images;
  SetGeometry(smush_image,&right_geometry);
  GravityAdjustGeometry(right_image->columns,right_image->rows,
    right_image->gravity,&right_geometry);
  left_image=images->previous;
  SetGeometry(smush_image,&left_geometry);
  GravityAdjustGeometry(left_image->columns,left_image->rows,
    left_image->gravity,&left_geometry);
  gap=right_image->columns;
  left_view=AcquireVirtualCacheView(left_image,exception);
  right_view=AcquireVirtualCacheView(right_image,exception);
  rows.left_image=left_image;
  rows.right_image=right_image;
  rows.left_view=left_view;
  rows.right_view=right_view;
  rows.left_geometry=left_geometry;
  rows.right_geometry=right_geometry;
  rows.exception=exception;
  for (y=0; y < (ssize_t) smush_image->rows; y++)
  {
    gap=SmushXRow(&rows,y,gap);
  }
  right_view=DestroyCacheView(right_view);
  left_view=DestroyCacheView(left_view);
  if (y < (ssize_t) smush_image->rows)
    return(offset);
  return((ssize_t) gap-offset);
}

static inline MagickBooleanType IsTopGapEdge(const Image *top_image,
  const Quantum *p,const ssize_t y,const size_t gap)
{
  return(((p == (const Quantum *) NULL) ||
    (GetPixelAlpha(top_image,p) != TransparentAlpha) ||
    (((ssize_t) top_image->rows-y-1) >= (ssize_t) gap)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsBottomGapEdge(const Image *bottom_image,
  const Quantum *p,const ssize_t y,const ssize_t i,const size_t gap)
{
  return(((p == (const Quantum *) NULL) ||
    (GetPixelAlpha(bottom_image,p) != TransparentAlpha) ||
    ((y+i) >= (ssize_t) gap)) ? MagickTrue : MagickFalse);
}

typedef struct _SmushYRows
{
  const Image
    *bottom_image,
    *top_image;

  CacheView
    *bottom_view,
    *top_view;

  RectangleInfo
    bottom_geometry,
    top_geometry;

  ExceptionInfo
    *exception;
} SmushYRows;

static ssize_t FindTopGapEdge(const SmushYRows *rows,const ssize_t x,
  const size_t gap)
{
  const Quantum
    *p;

  ssize_t
    y;

  for (y=(ssize_t) rows->top_image->rows-1; y > 0; y--)
  {
    p=GetCacheViewVirtualPixels(rows->top_view,rows->top_geometry.x+x,y,1,1,
      rows->exception);
    if (IsTopGapEdge(rows->top_image,p,y,gap) != MagickFalse)
      break;
  }
  return(y);
}

static ssize_t FindBottomGapEdge(const SmushYRows *rows,const ssize_t x,
  const ssize_t i,const size_t gap)
{
  const Quantum
    *p;

  ssize_t
    y;

  for (y=0; y < (ssize_t) rows->bottom_image->rows; y++)
  {
    p=GetCacheViewVirtualPixels(rows->bottom_view,rows->bottom_geometry.x+x,y,1,
      1,rows->exception);
    if (IsBottomGapEdge(rows->bottom_image,p,y,i,gap) != MagickFalse)
      break;
  }
  return(y);
}

static size_t SmushYRow(const SmushYRows *rows,const ssize_t x,size_t gap)
{
  ssize_t
    i,
    y;

  y=FindTopGapEdge(rows,x,gap);
  i=(ssize_t) rows->top_image->rows-y-1;
  y=FindBottomGapEdge(rows,x,i,gap);
  if ((y+i) < (ssize_t) gap)
    gap=(size_t) (y+i);
  return(gap);
}

static ssize_t SmushYGap(const Image *smush_image,const Image *images,
  const ssize_t offset,ExceptionInfo *exception)
{
  CacheView
    *bottom_view,
    *top_view;

  const Image
    *bottom_image,
    *top_image;

  RectangleInfo
    bottom_geometry,
    top_geometry;

  size_t
    gap;

  SmushYRows
    rows;

  ssize_t
    x;

  if (images->previous == (Image *) NULL)
    return(0);
  bottom_image=images;
  SetGeometry(smush_image,&bottom_geometry);
  GravityAdjustGeometry(bottom_image->columns,bottom_image->rows,
    bottom_image->gravity,&bottom_geometry);
  top_image=images->previous;
  SetGeometry(smush_image,&top_geometry);
  GravityAdjustGeometry(top_image->columns,top_image->rows,top_image->gravity,
    &top_geometry);
  gap=bottom_image->rows;
  top_view=AcquireVirtualCacheView(top_image,exception);
  bottom_view=AcquireVirtualCacheView(bottom_image,exception);
  rows.bottom_image=bottom_image;
  rows.top_image=top_image;
  rows.bottom_view=bottom_view;
  rows.top_view=top_view;
  rows.bottom_geometry=bottom_geometry;
  rows.top_geometry=top_geometry;
  rows.exception=exception;
  for (x=0; x < (ssize_t) smush_image->columns; x++)
  {
    gap=SmushYRow(&rows,x,gap);
  }
  bottom_view=DestroyCacheView(bottom_view);
  top_view=DestroyCacheView(top_view);
  if (x < (ssize_t) smush_image->columns)
    return(offset);
  return((ssize_t) gap-offset);
}

static void AddSmushStackExtent(const Image *next,const ssize_t offset,
  size_t *width,size_t *height)
{
  if (next->columns > (*width))
    (*width)=next->columns;
  (*height)+=next->rows;
  if (next->previous != (Image *) NULL)
    (*height)=(size_t) MagickMax((ssize_t) (*height)+offset,0U);
}

static void AddSmushRowExtent(const Image *next,const ssize_t offset,
  size_t *width,size_t *height)
{
  (*width)+=next->columns;
  if (next->previous != (Image *) NULL)
    (*width)=(size_t) MagickMax((ssize_t) (*width)+offset,0U);
  if (next->rows > (*height))
    (*height)=next->rows;
}

static void SetSmushExtent(Image *smush_image,const MagickBooleanType stack,
  const ssize_t x_offset,const ssize_t y_offset)
{
  if (stack == MagickFalse)
    smush_image->columns=(size_t) x_offset;
  else
    smush_image->rows=(size_t) y_offset;
}

typedef struct _SmushExtentTargets
{
  PixelTrait
    *alpha_trait;

  size_t
    *number_images,
    *width,
    *height;
} SmushExtentTargets;

static void ComputeSmushExtent(const Image *image,
  const MagickBooleanType stack,const ssize_t offset,
  const SmushExtentTargets *targets)
{
  PixelTrait
    *const alpha_trait = targets->alpha_trait;

  size_t
    *const number_images = targets->number_images,
    *const width = targets->width,
    *const height = targets->height;

  const Image
    *next;

  (*alpha_trait)=image->alpha_trait;
  (*number_images)=1;
  (*width)=image->columns;
  (*height)=image->rows;
  next=GetNextImageInList(image);
  for ( ; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    if (next->alpha_trait != UndefinedPixelTrait)
      (*alpha_trait)=BlendPixelTrait;
    (*number_images)++;
    if (stack != MagickFalse)
      {
        AddSmushStackExtent(next,offset,width,height);
        continue;
      }
    AddSmushRowExtent(next,offset,width,height);
  }
}

MagickExport Image *SmushImages(const Image *images,
  const MagickBooleanType stack,const ssize_t offset,ExceptionInfo *exception)
{
#define SmushImageTag  "Smush/Image"

  const Image
    *image;

  Image
    *smush_image;

  MagickBooleanType
    proceed,
    status;

  MagickOffsetType
    n;

  PixelTrait
    alpha_trait;

  RectangleInfo
    geometry;

  size_t
    height,
    number_images,
    width;

  ssize_t
    x_offset,
    y_offset;

  const SmushExtentTargets targets = { &alpha_trait, &number_images, &width,
    &height };

  /*
    Compute maximum area of smushed area.
  */
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  image=images;
  ComputeSmushExtent(image,stack,offset,&targets);
  /*
    Smush images.
  */
  smush_image=CloneImage(image,width,height,MagickTrue,exception);
  if (smush_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(smush_image,DirectClass,exception) == MagickFalse)
    {
      smush_image=DestroyImage(smush_image);
      return((Image *) NULL);
    }
  smush_image->alpha_trait=alpha_trait;
  (void) SetImageBackgroundColor(smush_image,exception);
  status=MagickTrue;
  x_offset=0;
  y_offset=0;
  for (n=0; n < (MagickOffsetType) number_images; n++)
  {
    SetGeometry(smush_image,&geometry);
    GravityAdjustGeometry(image->columns,image->rows,image->gravity,&geometry);
    if (stack != MagickFalse)
      {
        x_offset-=geometry.x;
        y_offset-=SmushYGap(smush_image,image,offset,exception);
      }
    else
      {
        x_offset-=SmushXGap(smush_image,image,offset,exception);
        y_offset-=geometry.y;
      }
    status=CompositeImage(smush_image,image,OverCompositeOp,MagickTrue,x_offset,
      y_offset,exception);
    proceed=SetImageProgress(image,SmushImageTag,n,number_images);
    if (proceed == MagickFalse)
      break;
    AdvanceAppendOffset(image,stack,&x_offset,&y_offset);
    image=GetNextImageInList(image);
  }
  SetSmushExtent(smush_image,stack,x_offset,y_offset);
  return(DestroyImageOnFailure(smush_image,status));
}
