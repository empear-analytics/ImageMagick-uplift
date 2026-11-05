/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      SSSSS  H   H  EEEEE   AAA   RRRR                       %
%                      SS     H   H  E      A   A  R   R                      %
%                       SSS   HHHHH  EEE    AAAAA  RRRR                       %
%                         SS  H   H  E      A   A  R R                        %
%                      SSSSS  H   H  EEEEE  A   A  R  R                       %
%                                                                             %
%                  RRRR    OOO   TTTTT   AAA   TTTTT  EEEEE                   %
%                  R   R  O   O    T    A   A    T    E                       %
%                  RRRR   O   O    T    AAAAA    T    EEE                     %
%                  R R    O   O    T    A   A    T    E                       %
%                  R  R    OOO     T    A   A    T    EEEEE                   %
%                                                                             %
%                                                                             %
%                      MagickCore Integral Rotate Method                      %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                  July 1992                                  %
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
%  The XShearImage() and YShearImage() methods are based on the paper "A Fast
%  Algorithm for General Raster Rotation" by Alan W. Paeth, Graphics
%  Interface '86 (Vancouver).  ShearRotateImage() is adapted from a similar
%  method based on the Paeth paper written by Michael Halle of the Spatial
%  Imaging Group, MIT Media Lab.
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/list.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resource_.h"
#include "MagickCore/shear.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e g r a l R o t a t e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IntegralRotateImage() rotates the image an integral of 90 degrees.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the rotated image.
%
%  The format of the IntegralRotateImage method is:
%
%      Image *IntegralRotateImage(const Image *image,size_t rotations,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o rotations: Specifies the number of 90 degree rotations.
%
*/
static void CopyRotatedPixelChannels(const Image *image,
  const Image *rotate_image,const Quantum *p,Quantum *q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait rotate_traits = GetPixelChannelTraits(rotate_image,
      channel);
    if ((traits == UndefinedPixelTrait) ||
        (rotate_traits == UndefinedPixelTrait))
      continue;
    SetPixelChannel(rotate_image,channel,p[i],q);
  }
}

static Image *CloneRotatedImage(const Image *image,size_t rotations,
  ExceptionInfo *exception)
{
  Image
    *rotate_image;

  switch (rotations)
  {
    case 0:
    default:
    {
      rotate_image=CloneImage(image,0,0,MagickTrue,exception);
      break;
    }
    case 2:
    {
      rotate_image=CloneImage(image,image->columns,image->rows,MagickTrue,
        exception);
      break;
    }
    case 1:
    case 3:
    {
      rotate_image=CloneImage(image,image->rows,image->columns,MagickTrue,
        exception);
      break;
    }
  }
  return(rotate_image);
}

static void ReverseRotatedRow(const Image *image,const Image *rotate_image,
  const Quantum *p,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    q-=GetPixelChannels(rotate_image);
    CopyRotatedPixelChannels(image,rotate_image,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static size_t ClipTileExtent(const size_t tile_width,const ssize_t tile_x,
  const size_t extent)
{
  size_t
    width;

  width=tile_width;
  if ((tile_width+(size_t) tile_x) > extent)
    width=(size_t) ((ssize_t) tile_width-(tile_x+(ssize_t) tile_width-
      (ssize_t) extent));
  return(width);
}

static void AppendRotateTransform(const Image *image,const size_t rotations,
  Image *rotate_image,ExceptionInfo *exception)
{
  char
    transform[MagickPathExtent];

  (void) FormatLocaleString(transform,MagickPathExtent,
    "rotate %.17gx%.17g %.17g",(double) image->columns,
    (double) image->rows,(double) rotations);
  AppendImageProfileProperty(rotate_image,"hdrgm","hdrgm:Transform",
    transform,exception);
}

#define RotateImageTag  "Rotate/Image"

static void UpdateRotateTileProgress(const Image *image,
  const size_t tile_height,MagickBooleanType *status,
  MagickOffsetType *progress)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,RotateImageTag,
        (*progress)+=(MagickOffsetType) tile_height,image->rows);
      if (proceed == MagickFalse)
        (*status)=MagickFalse;
    }
}

typedef struct _RotateTile
{
  const Image
    *image;

  Image
    *rotate_image;

  CacheView
    *rotate_view;

  const Quantum
    *magick_restrict p;

  size_t
    height,
    width;

  ssize_t
    tile_x,
    tile_y;

  ExceptionInfo
    *exception;
} RotateTile;

static MagickBooleanType RotateTileLineHead270(const RotateTile *tile,
  const ssize_t y,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict tile_pixels;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=QueueCacheViewAuthenticPixels(tile->rotate_view,tile->tile_y,y+(ssize_t)
    tile->rotate_image->rows-(tile->tile_x+(ssize_t) tile->width),
    tile->height,1,tile->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  tile_pixels=tile->p+(((ssize_t) tile->width-1)-y)*(ssize_t)
    GetPixelChannels(tile->image);
  for (x=0; x < (ssize_t) tile->height; x++)
  {
    CopyRotatedPixelChannels(tile->image,tile->rotate_image,tile_pixels,q);
    tile_pixels+=tile->width*GetPixelChannels(tile->image);
    q+=(ptrdiff_t) GetPixelChannels(tile->rotate_image);
  }
  return(MagickTrue);
}

static void TestRotateSync(const MagickBooleanType sync,
  MagickBooleanType *status)
{
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

static void RotateTileColumns270(const RotateTile *tile,
  MagickBooleanType *status)
{
  const size_t
    width = tile->width;

  CacheView
    *rotate_view = tile->rotate_view;

  ExceptionInfo
    *exception = tile->exception;

  MagickBooleanType
    sync;

  ssize_t
    y;

  for (y=0; y < (ssize_t) width; y++)
  {
    if (RotateTileLineHead270(tile,y,status) == MagickFalse)
      continue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp critical (MagickCore_IntegralRotateImage)
#endif
    sync=SyncCacheViewAuthenticPixels(rotate_view,exception);
    TestRotateSync(sync,status);
  }
}

typedef struct _RotateTiles
{
  const Image
    *image;

  CacheView
    *image_view;

  Image
    *rotate_image;

  CacheView
    *rotate_view;

  size_t
    tile_height,
    tile_width;

  ExceptionInfo
    *exception;
} RotateTiles;

static MagickBooleanType RotateTile270(const RotateTiles *tiles,
  const ssize_t tile_x,const ssize_t tile_y,MagickBooleanType *status)
{
  const Image
    *image = tiles->image;

  CacheView
    *image_view = tiles->image_view;

  Image
    *rotate_image = tiles->rotate_image;

  CacheView
    *rotate_view = tiles->rotate_view;

  const size_t
    tile_height = tiles->tile_height,
    tile_width = tiles->tile_width;

  ExceptionInfo
    *exception = tiles->exception;

  const Quantum
    *magick_restrict p;

  RotateTile
    tile;

  size_t
    height,
    width;

  width=ClipTileExtent(tile_width,tile_x,image->columns);
  height=ClipTileExtent(tile_height,tile_y,image->rows);
  p=GetCacheViewVirtualPixels(image_view,tile_x,tile_y,width,height,
    exception);
  if (p == (const Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  tile.image=image;
  tile.rotate_image=rotate_image;
  tile.rotate_view=rotate_view;
  tile.p=p;
  tile.height=height;
  tile.width=width;
  tile.tile_x=tile_x;
  tile.tile_y=tile_y;
  tile.exception=exception;
  RotateTileColumns270(&tile,status);
  return(MagickTrue);
}

static void RotateTileRow270(const RotateTiles *tiles,const ssize_t tile_y,
  MagickBooleanType *status,MagickOffsetType *progress)
{
  const Image
    *image = tiles->image;

  const size_t
    tile_height = tiles->tile_height,
    tile_width = tiles->tile_width;

  ssize_t
    tile_x;

  if ((*status) == MagickFalse)
    return;
  tile_x=0;
  for ( ; tile_x < (ssize_t) image->columns; tile_x+=(ssize_t) tile_width)
  {
    if (RotateTile270(tiles,tile_x,tile_y,status) == MagickFalse)
      break;
  }
  UpdateRotateTileProgress(image,tile_height,status,progress);
}

static void RotateTileLine90(const RotateTile *tile,const ssize_t y,
  MagickBooleanType *status)
{
  const Quantum
    *magick_restrict tile_pixels;

  MagickBooleanType
    sync;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  q=QueueCacheViewAuthenticPixels(tile->rotate_view,(ssize_t)
    tile->rotate_image->columns-(tile->tile_y+(ssize_t) tile->height),y+
    tile->tile_x,tile->height,1,tile->exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  tile_pixels=tile->p+(((ssize_t) tile->height-1)*(ssize_t) tile->width+y)*
    (ssize_t) GetPixelChannels(tile->image);
  for (x=0; x < (ssize_t) tile->height; x++)
  {
    CopyRotatedPixelChannels(tile->image,tile->rotate_image,tile_pixels,q);
    tile_pixels-=tile->width*GetPixelChannels(tile->image);
    q+=(ptrdiff_t) GetPixelChannels(tile->rotate_image);
  }
  sync=SyncCacheViewAuthenticPixels(tile->rotate_view,tile->exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

static void RotateTileRow90(const RotateTiles *tiles,const ssize_t tile_y,
  MagickBooleanType *status,MagickOffsetType *progress)
{
  const Image
    *image = tiles->image;

  CacheView
    *image_view = tiles->image_view;

  Image
    *rotate_image = tiles->rotate_image;

  CacheView
    *rotate_view = tiles->rotate_view;

  const size_t
    tile_height = tiles->tile_height,
    tile_width = tiles->tile_width;

  ExceptionInfo
    *exception = tiles->exception;

  ssize_t
    tile_x;

  if ((*status) == MagickFalse)
    return;
  tile_x=0;
  for ( ; tile_x < (ssize_t) image->columns; tile_x+=(ssize_t) tile_width)
  {
    const Quantum
      *magick_restrict p;

    RotateTile
      tile;

    size_t
      height,
      width;

    ssize_t
      y;

    width=ClipTileExtent(tile_width,tile_x,image->columns);
    height=ClipTileExtent(tile_height,tile_y,image->rows);
    p=GetCacheViewVirtualPixels(image_view,tile_x,tile_y,width,height,
      exception);
    if (p == (const Quantum *) NULL)
      {
        (*status)=MagickFalse;
        break;
      }
    tile.image=image;
    tile.rotate_image=rotate_image;
    tile.rotate_view=rotate_view;
    tile.p=p;
    tile.height=height;
    tile.width=width;
    tile.tile_x=tile_x;
    tile.tile_y=tile_y;
    tile.exception=exception;
    for (y=0; y < (ssize_t) width; y++)
    {
      RotateTileLine90(&tile,y,status);
    }
  }
  UpdateRotateTileProgress(image,tile_height,status,progress);
}

typedef struct _RotateRowInfo
{
  const Image
    *image;

  CacheView
    *image_view;

  Image
    *rotate_image;

  CacheView
    *rotate_view;

  ExceptionInfo
    *exception;
} RotateRowInfo;

static void RotateRow180(const RotateRowInfo *row,const ssize_t y,
  MagickBooleanType *status,MagickOffsetType *progress)
{
  const Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  Image
    *rotate_image = row->rotate_image;

  CacheView
    *rotate_view = row->rotate_view;

  ExceptionInfo
    *exception = row->exception;

  const Quantum
    *magick_restrict p;

  MagickBooleanType
    sync;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=QueueCacheViewAuthenticPixels(rotate_view,0,(ssize_t) image->rows-y-1,
    image->columns,1,exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  q+=(ptrdiff_t) GetPixelChannels(rotate_image)*image->columns;
  ReverseRotatedRow(image,rotate_image,p,q);
  sync=SyncCacheViewAuthenticPixels(rotate_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,RotateImageTag,(*progress)++,
        image->rows);
      if (proceed == MagickFalse)
        (*status)=MagickFalse;
    }
}

MagickExport Image *IntegralRotateImage(const Image *image,size_t rotations,
  ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *rotate_view;

  Image
    *rotate_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  RectangleInfo
    page;

  /*
    Initialize rotated image attributes.
  */
  assert(image != (Image *) NULL);
  page=image->page;
  rotations%=4;
  rotate_image=CloneRotatedImage(image,rotations,exception);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  if (rotations == 0)
    return(rotate_image);
  /*
    Integral rotate the image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  rotate_view=AcquireAuthenticCacheView(rotate_image,exception);
  switch (rotations)
  {
    case 1:
    {
      size_t
        tile_height,
        tile_width;

      ssize_t
        tile_y;

      /*
        Rotate 90 degrees.
      */
      GetPixelCacheTileSize(image,&tile_width,&tile_height);
      tile_width=image->columns;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,rotate_image,image->rows/tile_height,2)
#endif
      for (tile_y=0; tile_y < (ssize_t) image->rows; tile_y+=(ssize_t) tile_height)
      {
        const RotateTiles tiles = { image, image_view, rotate_image,
          rotate_view, tile_height, tile_width, exception };

        RotateTileRow90(&tiles,tile_y,&status,&progress);
      }
      (void) SetImageProgress(image,RotateImageTag,(MagickOffsetType)
        image->rows-1,image->rows);
      Swap(page.width,page.height);
      Swap(page.x,page.y);
      if (page.width != 0)
        page.x=(ssize_t) page.width-(ssize_t) rotate_image->columns-page.x;
      break;
    }
    case 2:
    {
      ssize_t
        y;

      /*
        Rotate 180 degrees.
      */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,rotate_image,image->rows,2)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        const RotateRowInfo row = { image, image_view, rotate_image,
          rotate_view, exception };

        RotateRow180(&row,y,&status,&progress);
      }
      (void) SetImageProgress(image,RotateImageTag,(MagickOffsetType)
        image->rows-1,image->rows);
      if (page.width != 0)
        page.x=(ssize_t) page.width-(ssize_t) rotate_image->columns-page.x;
      if (page.height != 0)
        page.y=(ssize_t) page.height-(ssize_t) rotate_image->rows-page.y;
      break;
    }
    case 3:
    {
      size_t
        tile_height,
        tile_width;

      ssize_t
        tile_y;

      /*
        Rotate 270 degrees.
      */
      GetPixelCacheTileSize(image,&tile_width,&tile_height);
      tile_width=image->columns;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(image,rotate_image,image->rows/tile_height,2)
#endif
      for (tile_y=0; tile_y < (ssize_t) image->rows; tile_y+=(ssize_t) tile_height)
      {
        const RotateTiles tiles = { image, image_view, rotate_image,
          rotate_view, tile_height, tile_width, exception };

        RotateTileRow270(&tiles,tile_y,&status,&progress);
      }
      (void) SetImageProgress(image,RotateImageTag,(MagickOffsetType)
        image->rows-1,image->rows);
      Swap(page.width,page.height);
      Swap(page.x,page.y);
      if (page.height != 0)
        page.y=(ssize_t) page.height-(ssize_t) rotate_image->rows-page.y;
      break;
    }
    default:
      break;
  }
  rotate_view=DestroyCacheView(rotate_view);
  image_view=DestroyCacheView(image_view);
  rotate_image->type=image->type;
  rotate_image->page=page;
  if (status != MagickFalse)
    AppendRotateTransform(image,rotations,rotate_image,exception);
  if (status == MagickFalse)
    rotate_image=DestroyImage(rotate_image);
  return(rotate_image);
}
