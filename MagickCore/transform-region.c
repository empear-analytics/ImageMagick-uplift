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
static void SetTransformProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static inline MagickBooleanType IsPixelRowMissing(const Quantum *p,
  const Quantum *q)
{
  return(((p == (const Quantum *) NULL) || (q == (Quantum *) NULL)) ?
    MagickTrue : MagickFalse);
}

static void CopyPixelChannelsTo(const Image *image,const Image *destination,
  const Quantum *p,Quantum *q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait destination_traits=GetPixelChannelTraits(destination,channel);
    if ((traits == UndefinedPixelTrait) ||
        (destination_traits == UndefinedPixelTrait))
      continue;
    SetPixelChannel(destination,channel,p[i],q);
  }
}

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

static Image *FinishChopImage(const Image *image,Image *chop_image,
  CacheView *image_view,const MagickBooleanType status)
{
  image_view=DestroyCacheView(image_view);
  chop_image->type=image->type;
  if (status == MagickFalse)
    chop_image=DestroyImage(chop_image);
  return(chop_image);
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

typedef struct _TransformLine
{
  const Image
    *image;

  Image
    *transform_image;

  CacheView
    *image_view,
    *transform_view;

  ExceptionInfo
    *exception;
} TransformLine;

static void CopyPixelRow(const Image *image,const Image *destination,
  const Quantum *p,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) destination->columns; x++)
  {
    CopyPixelChannelsTo(image,destination,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(destination);
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
static void AppendTransformProperty(const Image *image,
  Image *transform_image,const char *format,ExceptionInfo *exception)
{
  char
    transform[MagickPathExtent];

  (void) FormatLocaleString(transform,MagickPathExtent,format,
    (double) image->columns,(double) image->rows);
  AppendImageProfileProperty(transform_image,"hdrgm","hdrgm:Transform",
    transform,exception);
}

static void SetFlipPage(Image *flip_image,RectangleInfo page)
{
  if (page.height != 0)
    page.y=((ssize_t) page.height-(ssize_t) flip_image->rows-page.y);
  flip_image->page=page;
}

typedef struct _TransformFinish
{
  const Image
    *image;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;

  const char
    *format;
} TransformFinish;

static Image *FinishTransformImage(const TransformFinish *finish,
  Image *transform_image)
{
  const Image
    *const image = finish->image;

  const MagickBooleanType
    status = finish->status;

  ExceptionInfo
    *const exception = finish->exception;

  const char
    *const format = finish->format;

  if (status != MagickFalse)
    AppendTransformProperty(image,transform_image,format,exception);
  if (status == MagickFalse)
    transform_image=DestroyImage(transform_image);
  return(transform_image);
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

static void SyncTransformView(CacheView *view,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(view,exception) == MagickFalse)
    (*status)=MagickFalse;
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
