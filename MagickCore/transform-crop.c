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
%                          CCCC  RRRR    OOO   PPPP                           %
%                         C      R   R  O   O  P   P                          %
%                         C      RRRR   O   O  PPPP                           %
%                         C      R R    O   O  P                              %
%                          CCCC  R  R    OOO   P                              %
%                                                                             %
%                                                                             %
%              MagickCore Chop, Crop, Excerpt and Splice Methods              %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C h o p I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ChopImage() removes a region of an image and collapses the image to occupy
%  the removed portion.
%
%  The format of the ChopImage method is:
%
%      Image *ChopImage(const Image *image,const RectangleInfo *chop_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o chop_info: Define the region of the image to chop.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsChopOutsideImage(const Image *image,
  const RectangleInfo *chop_info)
{
  return((((chop_info->x+(ssize_t) chop_info->width) < 0) ||
    ((chop_info->y+(ssize_t) chop_info->height) < 0) ||
    (chop_info->x > (ssize_t) image->columns) ||
    (chop_info->y > (ssize_t) image->rows)) ? MagickTrue : MagickFalse);
}

static void ClipChopOffset(ssize_t *offset,size_t *extent)
{
  if (*offset < 0)
    {
      *extent-=(size_t) (-*offset);
      *offset=0;
    }
}

typedef struct _ChopRowImages
{
  const Image
    *image;

  Image
    *chop_image;

  CacheView
    *image_view,
    *chop_view;

  RectangleInfo
    extent;

  ExceptionInfo
    *exception;
} ChopRowImages;

static void ChopPixelRow(const ChopRowImages *images,
  const RectangleInfo extent,const Quantum *p,Quantum *q)
{
  const Image
    *const image = images->image;

  Image
    *const chop_image = images->chop_image;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if ((x < extent.x) || (x >= (extent.x+(ssize_t) extent.width)))
      {
        CopyPixelChannelsTo(image,chop_image,p,q);
        q+=(ptrdiff_t) GetPixelChannels(chop_image);
      }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType ChopImageRow(const ChopRowImages *images,
  const ssize_t source_y,const ssize_t target_y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  p=GetCacheViewVirtualPixels(images->image_view,0,source_y,
    images->image->columns,1,images->exception);
  q=QueueCacheViewAuthenticPixels(images->chop_view,0,target_y,
    images->chop_image->columns,1,images->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ChopPixelRow(images,images->extent,p,q);
  if (SyncCacheViewAuthenticPixels(images->chop_view,images->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static RectangleInfo ClampChopExtent(const Image *image,
  RectangleInfo extent)
{
  if ((extent.x+(ssize_t) extent.width) > (ssize_t) image->columns)
    extent.width=(size_t) ((ssize_t) image->columns-extent.x);
  if ((extent.y+(ssize_t) extent.height) > (ssize_t) image->rows)
    extent.height=(size_t) ((ssize_t) image->rows-extent.y);
  ClipChopOffset(&extent.x,&extent.width);
  ClipChopOffset(&extent.y,&extent.height);
  return(extent);
}

static inline MagickBooleanType IsChopExtentTooLarge(const Image *image,
  const RectangleInfo extent)
{
  return(((extent.width >= image->columns) || (extent.height >= image->rows)) ?
    MagickTrue : MagickFalse);
}

#define ChopImageTag  "Chop/Image"

static Image *ExtractChopImageBelow(const ChopRowImages *chop,
  MagickOffsetType progress,MagickBooleanType status)
{
  const Image
    *const image = chop->image;

  Image
    *const chop_image = chop->chop_image;

  CacheView
    *const image_view = chop->image_view,
    *chop_view = chop->chop_view;

  const RectangleInfo
    extent = chop->extent;

  ExceptionInfo
    *const exception = chop->exception;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,chop_image,image->rows-((size_t) extent.y+extent.height),2)
#endif
  for (y=0; y < (ssize_t) (image->rows-((size_t) extent.y+extent.height)); y++)
  {
    const ChopRowImages
      images = { image,chop_image,image_view,chop_view,extent,exception };

    if (status == MagickFalse)
      continue;
    if (ChopImageRow(&images,extent.y+(ssize_t) extent.height+y,extent.y+y,
          &status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetTransformProgress(image,ChopImageTag,progress,&status);
      }
  }
  chop_view=DestroyCacheView(chop_view);
  return(FinishChopImage(image,chop_image,image_view,status));
}

typedef struct _ChopImageInputs
{
  const Image
    *image;

  RectangleInfo
    extent;

  ExceptionInfo
    *exception;
} ChopImageInputs;

static Image *ExtractChopImage(Image *chop_image,const ChopImageInputs *inputs)
{
  const Image
    *const image = inputs->image;

  const RectangleInfo
    extent = inputs->extent;

  ExceptionInfo
    *const exception = inputs->exception;

  CacheView
    *chop_view,
    *image_view;

  ChopRowImages
    chop;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  chop_view=AcquireAuthenticCacheView(chop_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,chop_image,(size_t) extent.y,2)
#endif
  for (y=0; y < (ssize_t) extent.y; y++)
  {
    const ChopRowImages
      images = { image,chop_image,image_view,chop_view,extent,exception };

    if (status == MagickFalse)
      continue;
    if (ChopImageRow(&images,y,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetTransformProgress(image,ChopImageTag,progress,&status);
      }
  }
  /*
    Extract chop image.
  */
  chop.image=image;
  chop.chop_image=chop_image;
  chop.image_view=image_view;
  chop.chop_view=chop_view;
  chop.extent=extent;
  chop.exception=exception;
  return(ExtractChopImageBelow(&chop,progress,status));
}

static RectangleInfo GetChopExtent(const Image *image,
  const RectangleInfo *chop_info)
{
  RectangleInfo
    extent;

  extent=(*chop_info);
  extent=ClampChopExtent(image,extent);
  return(extent);
}

MagickExport Image *ChopImage(const Image *image,const RectangleInfo *chop_info,
  ExceptionInfo *exception)
{
  ChopImageInputs
    inputs;

  Image
    *chop_image;

  RectangleInfo
    extent;

  /*
    Check chop geometry.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  assert(chop_info != (RectangleInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsChopOutsideImage(image,chop_info) != MagickFalse)
    ThrowImageException(OptionWarning,"GeometryDoesNotContainImage");
  extent=GetChopExtent(image,chop_info);
  if (IsChopExtentTooLarge(image,extent) != MagickFalse)
    ThrowImageException(OptionWarning,"GeometryDoesNotContainImage");
  chop_image=CloneImage(image,image->columns-extent.width,image->rows-
    extent.height,MagickTrue,exception);
  if (chop_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Extract chop image.
  */
  inputs.image=image;
  inputs.extent=extent;
  inputs.exception=exception;
  return(ExtractChopImage(chop_image,&inputs));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C r o p I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CropImage() extracts a region of the image starting at the offset defined
%  by geometry.  Region must be fully defined, and no special handling of
%  geometry flags is performed.
%
%  The format of the CropImage method is:
%
%      Image *CropImage(const Image *image,const RectangleInfo *geometry,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o geometry: Define the region of the image to crop with members
%      x, y, width, and height.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsCropOutsideImage(const Image *image,
  const RectangleInfo bounding_box,const RectangleInfo page)
{
  return(((((double) bounding_box.x-page.x) >= (double) page.width) ||
    (((double) bounding_box.y-page.y) >= (double) page.height) ||
    (((double) page.x-bounding_box.x) > (double) image->columns) ||
    (((double) page.y-bounding_box.y) > (double) image->rows)) ?
    MagickTrue : MagickFalse);
}

static void ClipCropOffset(ssize_t *offset,size_t *extent,
  const ssize_t origin)
{
  if ((*offset < 0) && (origin >= 0))
    {
      *extent=CastDoubleToSizeT((double) *extent+*offset-origin);
      *offset=0;
    }
  else
    {
      *extent=CastDoubleToSizeT((double) *extent-(origin-*offset));
      *offset-=origin;
      if (*offset < 0)
        *offset=0;
    }
}

static Image *CropOutsideImage(const Image *image,
  const RectangleInfo bounding_box,ExceptionInfo *exception)
{
  Image
    *crop_image;

  crop_image=CloneImage(image,1,1,MagickTrue,exception);
  if (crop_image == (Image *) NULL)
    return((Image *) NULL);
  crop_image->background_color.alpha_trait=BlendPixelTrait;
  crop_image->background_color.alpha=(MagickRealType) TransparentAlpha;
  (void) SetImageBackgroundColor(crop_image,exception);
  crop_image->page=bounding_box;
  crop_image->page.x=(-1);
  crop_image->page.y=(-1);
  if (crop_image->dispose == BackgroundDispose)
    crop_image->dispose=NoneDispose;
  return(crop_image);
}

static RectangleInfo ClampCropExtent(const Image *image,
  const RectangleInfo *geometry,RectangleInfo page)
{
  if ((page.x+(ssize_t) page.width) > (ssize_t) image->columns)
    page.width=(size_t) ((ssize_t) image->columns-page.x);
  if ((geometry->width != 0) && (page.width > geometry->width))
    page.width=geometry->width;
  if ((page.y+(ssize_t) page.height) > (ssize_t) image->rows)
    page.height=CastDoubleToSizeT((double) image->rows-page.y);
  if ((geometry->height != 0) && (page.height > geometry->height))
    page.height=geometry->height;
  return(page);
}

static void AppendCropProperty(const Image *image,Image *crop_image,
  const RectangleInfo page,ExceptionInfo *exception)
{
  char
    transform[MagickPathExtent];

  (void) FormatLocaleString(transform,MagickPathExtent,
    "crop %.17gx%.17g %.17gx%.17g%+.20g%+.20g",
    (double) image->columns,(double) image->rows,(double) page.width,
    (double) page.height,(double) page.x,(double) page.y);
  AppendImageProfileProperty(crop_image,"hdrgm","hdrgm:Transform",
    transform,exception);
}

static void SetCropPage(const Image *image,
  const RectangleInfo bounding_box,Image *crop_image)
{
  OffsetInfo
    offset;

  crop_image->page.width=image->page.width;
  crop_image->page.height=image->page.height;
  offset.x=bounding_box.x+(ssize_t) bounding_box.width;
  offset.y=bounding_box.y+(ssize_t) bounding_box.height;
  if ((offset.x > (ssize_t) image->page.width) ||
      (offset.y > (ssize_t) image->page.height))
    {
      crop_image->page.width=bounding_box.width;
      crop_image->page.height=bounding_box.height;
    }
  crop_image->page.x=bounding_box.x;
  crop_image->page.y=bounding_box.y;
}

static RectangleInfo GetCropBoundingBox(const Image *image)
{
  RectangleInfo
    bounding_box;

  bounding_box=image->page;
  if ((bounding_box.width == 0) || (bounding_box.height == 0))
    {
      bounding_box.width=image->columns;
      bounding_box.height=image->rows;
    }
  return(bounding_box);
}

static RectangleInfo GetCropPage(const RectangleInfo *geometry,
  const RectangleInfo bounding_box)
{
  RectangleInfo
    page;

  page=(*geometry);
  if (page.width == 0)
    page.width=bounding_box.width;
  if (page.height == 0)
    page.height=bounding_box.height;
  return(page);
}

static MagickBooleanType CheckCropExtent(const Image *image,
  const RectangleInfo page,ExceptionInfo *exception)
{
  if ((page.width == 0) || (page.height == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "GeometryDoesNotContainImage","`%s'",image->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static Image *CropOutsideCanvas(const Image *image,
  const RectangleInfo *geometry,const RectangleInfo bounding_box,
  ExceptionInfo *exception)
{
  /*
    Crop is not within virtual canvas, return 1 pixel transparent image.
  */
  (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
    "GeometryDoesNotContainImage","(\"%.17gx%.17g%+.20g%+.20g\") `%s'",
    (double) geometry->width,(double) geometry->height,
    (double) geometry->x,(double) geometry->y,image->filename);
  return(CropOutsideImage(image,bounding_box,exception));
}

static RectangleInfo AdjustCropPage(const Image *image,
  const RectangleInfo *geometry,const RectangleInfo bounding_box,
  RectangleInfo page)
{
  ClipCropOffset(&page.x,&page.width,bounding_box.x);
  ClipCropOffset(&page.y,&page.height,bounding_box.y);
  page=ClampCropExtent(image,geometry,page);
  return(page);
}

static RectangleInfo OffsetCropBoundingBox(RectangleInfo bounding_box,
  const RectangleInfo page)
{
  bounding_box.x+=page.x;
  bounding_box.y+=page.y;
  return(bounding_box);
}

static MagickBooleanType CropImageLine(const TransformLine *line,
  const RectangleInfo *page,const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(line->image_view,page->x,page->y+y,
    line->transform_image->columns,1,line->exception);
  q=QueueCacheViewAuthenticPixels(line->transform_view,0,y,
    line->transform_image->columns,1,line->exception);
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

#define CropImageTag  "Crop/Image"

typedef struct _CropRows
{
  const Image
    *image;

  Image
    *crop_image;

  CacheView
    *image_view,
    *crop_view;

  ExceptionInfo
    *exception;

  RectangleInfo
    page;
} CropRows;

static MagickBooleanType CropImageRows(const CropRows *rows,
  MagickBooleanType status,MagickOffsetType progress)
{
  const Image
    *const image = rows->image;

  Image
    *const crop_image = rows->crop_image;

  CacheView
    *const image_view = rows->image_view,
    *const crop_view = rows->crop_view;

  ExceptionInfo
    *const exception = rows->exception;

  const RectangleInfo
    page = rows->page;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,crop_image,crop_image->rows,2)
#endif
  for (y=0; y < (ssize_t) crop_image->rows; y++)
  {
    const TransformLine
      line = { image, crop_image, image_view, crop_view, exception };

    if (CropImageLine(&line,&page,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetTransformProgress(image,CropImageTag,progress,&status);
      }
  }
  return(status);
}

MagickExport Image *CropImage(const Image *image,const RectangleInfo *geometry,
  ExceptionInfo *exception)
{
  CacheView
    *crop_view,
    *image_view;

  Image
    *crop_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  CropRows
    rows;

  RectangleInfo
    bounding_box,
    page;

  /*
    Check crop geometry.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(geometry != (const RectangleInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  bounding_box=GetCropBoundingBox(image);
  page=GetCropPage(geometry,bounding_box);
  if (IsCropOutsideImage(image,bounding_box,page) != MagickFalse)
    return(CropOutsideCanvas(image,geometry,bounding_box,exception));
  page=AdjustCropPage(image,geometry,bounding_box,page);
  bounding_box=OffsetCropBoundingBox(bounding_box,page);
  if (CheckCropExtent(image,page,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Initialize crop image attributes.
  */
  crop_image=CloneImage(image,page.width,page.height,MagickTrue,exception);
  if (crop_image == (Image *) NULL)
    return((Image *) NULL);
  SetCropPage(image,bounding_box,crop_image);
  /*
    Crop image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  crop_view=AcquireAuthenticCacheView(crop_image,exception);
  rows.image=image;
  rows.crop_image=crop_image;
  rows.image_view=image_view;
  rows.crop_view=crop_view;
  rows.exception=exception;
  rows.page=page;
  status=CropImageRows(&rows,status,progress);
  crop_view=DestroyCacheView(crop_view);
  image_view=DestroyCacheView(image_view);
  crop_image->type=image->type;
  if (status != MagickFalse)
    AppendCropProperty(image,crop_image,page,exception);
  if (status == MagickFalse)
    crop_image=DestroyImage(crop_image);
  return(crop_image);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x c e r p t I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExcerptImage() returns an excerpt of the image as defined by the geometry.
%
%  The format of the ExcerptImage method is:
%
%      Image *ExcerptImage(const Image *image,const RectangleInfo *geometry,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o geometry: Define the region of the image to extend with members
%      x, y, width, and height.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#define ExcerptImageTag  "Excerpt/Image"

typedef struct _ExcerptRows
{
  const Image
    *image;

  Image
    *excerpt_image;

  CacheView
    *image_view,
    *excerpt_view;

  const RectangleInfo
    *geometry;

  ExceptionInfo
    *exception;
} ExcerptRows;

static MagickBooleanType ExcerptImageRows(const ExcerptRows *rows,
  MagickBooleanType status,MagickOffsetType progress)
{
  const Image
    *const image = rows->image;

  Image
    *const excerpt_image = rows->excerpt_image;

  CacheView
    *const image_view = rows->image_view,
    *const excerpt_view = rows->excerpt_view;

  const RectangleInfo
    *const geometry = rows->geometry;

  ExceptionInfo
    *const exception = rows->exception;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,excerpt_image,excerpt_image->rows,2)
#endif
  for (y=0; y < (ssize_t) excerpt_image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,geometry->x,geometry->y+y,
      geometry->width,1,exception);
    q=GetCacheViewAuthenticPixels(excerpt_view,0,y,excerpt_image->columns,1,
      exception);
    if (IsPixelRowMissing(p,q) != MagickFalse)
      {
        status=MagickFalse;
        continue;
      }
    CopyPixelRow(image,excerpt_image,p,q);
    if (SyncCacheViewAuthenticPixels(excerpt_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetTransformProgress(image,ExcerptImageTag,progress,&status);
      }
  }
  return(status);
}

MagickExport Image *ExcerptImage(const Image *image,
  const RectangleInfo *geometry,ExceptionInfo *exception)
{
  CacheView
    *excerpt_view,
    *image_view;

  Image
    *excerpt_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ExcerptRows
    rows;

  /*
    Allocate excerpt image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(geometry != (const RectangleInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  excerpt_image=CloneImage(image,geometry->width,geometry->height,MagickTrue,
    exception);
  if (excerpt_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Excerpt each row.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  excerpt_view=AcquireAuthenticCacheView(excerpt_image,exception);
  rows.image=image;
  rows.excerpt_image=excerpt_image;
  rows.image_view=image_view;
  rows.excerpt_view=excerpt_view;
  rows.geometry=geometry;
  rows.exception=exception;
  status=ExcerptImageRows(&rows,status,progress);
  excerpt_view=DestroyCacheView(excerpt_view);
  return(FinishChopImage(image,excerpt_image,image_view,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S p l i c e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SpliceImage() splices a solid color into the image as defined by the
%  geometry.
%
%  The format of the SpliceImage method is:
%
%      Image *SpliceImage(const Image *image,const RectangleInfo *geometry,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o geometry: Define the region of the image to splice with members
%      x, y, width, and height.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void OffsetSpliceGeometry(const Image *image,
  RectangleInfo *splice_geometry)
{
  switch (image->gravity)
  {
    default:
    case UndefinedGravity:
    case NorthWestGravity:
      break;
    case NorthGravity:
    {
      (*splice_geometry).x+=(ssize_t) (*splice_geometry).width/2;
      break;
    }
    case NorthEastGravity:
    {
      (*splice_geometry).x+=(ssize_t) (*splice_geometry).width;
      break;
    }
    case WestGravity:
    {
      (*splice_geometry).y+=(ssize_t) (*splice_geometry).width/2;
      break;
    }
  }
}

static RectangleInfo ApplySpliceGravity(const Image *image,
  RectangleInfo splice_geometry)
{
  switch (image->gravity)
  {
    default:
    {
      OffsetSpliceGeometry(image,&splice_geometry);
      break;
    }
    case CenterGravity:
    {
      splice_geometry.x+=(ssize_t) splice_geometry.width/2;
      splice_geometry.y+=(ssize_t) splice_geometry.height/2;
      break;
    }
    case EastGravity:
    {
      splice_geometry.x+=(ssize_t) splice_geometry.width;
      splice_geometry.y+=(ssize_t) splice_geometry.height/2;
      break;
    }
    case SouthWestGravity:
    {
      splice_geometry.y+=(ssize_t) splice_geometry.height;
      break;
    }
    case SouthGravity:
    {
      splice_geometry.x+=(ssize_t) splice_geometry.width/2;
      splice_geometry.y+=(ssize_t) splice_geometry.height;
      break;
    }
    case SouthEastGravity:
    {
      splice_geometry.x+=(ssize_t) splice_geometry.width;
      splice_geometry.y+=(ssize_t) splice_geometry.height;
      break;
    }
  }
  return(splice_geometry);
}

typedef struct _SpliceRow
{
  const Image
    *image;

  Image
    *splice_image;

  CacheView
    *image_view,
    *splice_view;

  RectangleInfo
    splice_geometry;

  ExceptionInfo
    *exception;

  ssize_t
    columns;
} SpliceRow;

static void SplicePixelRow(const SpliceRow *row,
  const RectangleInfo splice_geometry,const Quantum *p,Quantum *q)
{
  const Image
    *const image = row->image;

  Image
    *const splice_image = row->splice_image;

  const ssize_t
    columns = row->columns;

  ssize_t
    x;

  for (x=0; x < columns; x++)
  {
    CopyPixelChannelsTo(image,splice_image,p,q);
    SetPixelRed(splice_image,GetPixelRed(image,p),q);
    SetPixelGreen(splice_image,GetPixelGreen(image,p),q);
    SetPixelBlue(splice_image,GetPixelBlue(image,p),q);
    SetPixelAlpha(splice_image,GetPixelAlpha(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(splice_image);
  }
  for ( ; x < (splice_geometry.x+(ssize_t) splice_geometry.width); x++)
    q+=(ptrdiff_t) GetPixelChannels(splice_image);
  for ( ; x < (ssize_t) splice_image->columns; x++)
  {
    CopyPixelChannelsTo(image,splice_image,p,q);
    SetPixelRed(splice_image,GetPixelRed(image,p),q);
    SetPixelGreen(splice_image,GetPixelGreen(image,p),q);
    SetPixelBlue(splice_image,GetPixelBlue(image,p),q);
    SetPixelAlpha(splice_image,GetPixelAlpha(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(splice_image);
  }
}

static MagickBooleanType SpliceImageRow(const SpliceRow *row,
  const ssize_t source_y,const ssize_t target_y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  p=GetCacheViewVirtualPixels(row->image_view,0,source_y,
    row->splice_image->columns,1,row->exception);
  q=QueueCacheViewAuthenticPixels(row->splice_view,0,target_y,
    row->splice_image->columns,1,row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  SplicePixelRow(row,row->splice_geometry,p,q);
  return(MagickTrue);
}

static void SetSpliceProgress(const SpliceRow *row,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(row->image,tag,progress,row->splice_image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static void SetSpliceBackground(Image *splice_image,
  ExceptionInfo *exception)
{
  if ((IsPixelInfoGray(&splice_image->background_color) == MagickFalse) &&
      (IsGrayColorspace(splice_image->colorspace) != MagickFalse))
    (void) SetImageColorspace(splice_image,sRGBColorspace,exception);
  if ((splice_image->background_color.alpha_trait != UndefinedPixelTrait) &&
      (splice_image->alpha_trait == UndefinedPixelTrait))
    (void) SetImageAlpha(splice_image,OpaqueAlpha,exception);
  (void) SetImageBackgroundColor(splice_image,exception);
}

static MagickBooleanType PrepareSpliceImage(Image *splice_image,
  ExceptionInfo *exception)
{
  if (SetImageStorageClass(splice_image,DirectClass,exception) == MagickFalse)
    {
      splice_image=DestroyImage(splice_image);
      return(MagickFalse);
    }
  SetSpliceBackground(splice_image,exception);
  return(MagickTrue);
}

static Image *FinishSpliceImage(Image *splice_image,CacheView *splice_view,
  CacheView *image_view,const MagickBooleanType status)
{
  splice_view=DestroyCacheView(splice_view);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    splice_image=DestroyImage(splice_image);
  return(splice_image);
}

static inline MagickBooleanType IsSpliceRowOutside(const Image *splice_image,
  const ssize_t y)
{
  return(((y < 0) || (y >= (ssize_t) splice_image->rows)) ? MagickTrue :
    MagickFalse);
}

static MagickBooleanType IsSpliceRowPending(Image *splice_image,
  const ssize_t y,const MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  if (IsSpliceRowOutside(splice_image,y) != MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

#define SpliceImageTag  "Splice/Image"

static Image *SpliceImageBelow(const SpliceRow *splice,
  MagickOffsetType progress,MagickBooleanType status)
{
  const Image
    *const image = splice->image;

  Image
    *const splice_image = splice->splice_image;

  CacheView
    *const image_view = splice->image_view,
    *const splice_view = splice->splice_view;

  const RectangleInfo
    splice_geometry = splice->splice_geometry;

  ExceptionInfo
    *const exception = splice->exception;

  const ssize_t
    columns = splice->columns;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,splice_image,splice_image->rows,2)
#endif
  for (y=splice_geometry.y+(ssize_t) splice_geometry.height; y < (ssize_t) splice_image->rows; y++)
  {
    const SpliceRow
      row = { image,splice_image,image_view,splice_view,splice_geometry,
        exception,columns };

    if (IsSpliceRowPending(splice_image,y,&status) == MagickFalse)
      continue;
    if (SpliceImageRow(&row,y-(ssize_t) splice_geometry.height,y,&status) ==
        MagickFalse)
      continue;
    SyncTransformView(splice_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetSpliceProgress(&row,SpliceImageTag,progress,&status);
      }
  }
  return(FinishSpliceImage(splice_image,splice_view,image_view,status));
}

typedef struct _SpliceImageInputs
{
  const Image
    *image;

  RectangleInfo
    splice_geometry;

  ExceptionInfo
    *exception;
} SpliceImageInputs;

static Image *SpliceImageRows(Image *splice_image,
  const SpliceImageInputs *inputs)
{
  const Image
    *const image = inputs->image;

  const RectangleInfo
    splice_geometry = inputs->splice_geometry;

  ExceptionInfo
    *const exception = inputs->exception;

  CacheView
    *image_view,
    *splice_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  SpliceRow
    splice;

  ssize_t
    columns,
    y;

  status=MagickTrue;
  progress=0;
  columns=MagickMin(splice_geometry.x,(ssize_t) splice_image->columns);
  image_view=AcquireVirtualCacheView(image,exception);
  splice_view=AcquireAuthenticCacheView(splice_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,splice_image,(size_t) splice_geometry.y,2)
#endif
  for (y=0; y < (ssize_t) splice_geometry.y; y++)
  {
    const SpliceRow
      row = { image,splice_image,image_view,splice_view,splice_geometry,
        exception,columns };

    if (status == MagickFalse)
      continue;
    if (SpliceImageRow(&row,y,y,&status) == MagickFalse)
      continue;
    SyncTransformView(splice_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetSpliceProgress(&row,SpliceImageTag,progress,&status);
      }
  }
  splice.image=image;
  splice.splice_image=splice_image;
  splice.image_view=image_view;
  splice.splice_view=splice_view;
  splice.splice_geometry=splice_geometry;
  splice.exception=exception;
  splice.columns=columns;
  return(SpliceImageBelow(&splice,progress,status));
}

MagickExport Image *SpliceImage(const Image *image,
  const RectangleInfo *geometry,ExceptionInfo *exception)
{
  Image
    *splice_image;

  RectangleInfo
    splice_geometry;

  SpliceImageInputs
    inputs;

  /*
    Allocate splice image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(geometry != (const RectangleInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  splice_geometry=(*geometry);
  splice_image=CloneImage(image,image->columns+splice_geometry.width,
    image->rows+splice_geometry.height,MagickTrue,exception);
  if (splice_image == (Image *) NULL)
    return((Image *) NULL);
  if (PrepareSpliceImage(splice_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Respect image geometry.
  */
  splice_geometry=ApplySpliceGravity(image,splice_geometry);
  /*
    Splice image.
  */
  inputs.image=image;
  inputs.splice_geometry=splice_geometry;
  inputs.exception=exception;
  return(SpliceImageRows(splice_image,&inputs));
}
