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
%                      TTTTT  IIIII  L      EEEEE  SSSSS                      %
%                        T      I    L      E      SS                         %
%                        T      I    L      EEE     SSS                       %
%                        T      I    L      E         SS                      %
%                        T    IIIII  LLLLL  EEEEE  SSSSS                      %
%                                                                             %
%                                                                             %
%                    MagickCore Image Crop To Tiles Method                    %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C r o p I m a g e T o T i l e s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CropImageToTiles() crops a single image, into a possible list of tiles.
%  This may include a single sub-region of the image.  This basically applies
%  all the normal geometry flags for Crop.
%
%      Image *CropImageToTiles(const Image *image,
%        const RectangleInfo *crop_geometry, ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image The transformed image is returned as this parameter.
%
%    o crop_geometry: A crop geometry string.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline ssize_t PixelRoundOffset(double x)
{
  /*
    Round the fraction to nearest integer.
  */
  if ((x-floor(x)) < (ceil(x)-x))
    return(CastDoubleToSsizeT(floor(x)));
  return(CastDoubleToSsizeT(ceil(x)));
}

static inline MagickBooleanType IsSingleRegionCrop(
  const RectangleInfo geometry,const MagickStatusType flags)
{
  return((((geometry.width == 0) && (geometry.height == 0)) ||
    ((flags & XValue) != 0) || ((flags & YValue) != 0)) ?
    MagickTrue : MagickFalse);
}

typedef struct _AreaTileRow
{
  const Image
    *image;

  size_t
    width;

  MagickStatusType
    flags;

  RectangleInfo
    geometry;

  PointInfo
    delta;

  ExceptionInfo
    *exception;
} AreaTileRow;

static void CropAreaTileRow(const AreaTileRow *row,PointInfo *offset,
  RectangleInfo *crop,Image **crop_image)
{
  const Image
    *const image = row->image;

  const size_t
    width = row->width;

  const MagickStatusType
    flags = row->flags;

  const RectangleInfo
    geometry = row->geometry;

  const PointInfo
    delta = row->delta;

  ExceptionInfo
    *const exception = row->exception;

  Image
    *next;

  for ((*offset).x=0; (*offset).x < (double) width; )
  {
    if ((flags & AspectValue) == 0)
      {
        (*crop).x=PixelRoundOffset((double) ((*offset).x-
          (geometry.x > 0 ? 0 : geometry.x)));
        (*offset).x+=delta.x;  /* increment now to find height */
        (*crop).width=(size_t) PixelRoundOffset((double) ((*offset).x+
          (geometry.x < 0 ? 0 : geometry.x)));
      }
    else
      {
        (*crop).x=PixelRoundOffset((double) ((*offset).x-
          (geometry.x > 0 ? geometry.x : 0)));
        (*offset).x+=delta.x;  /* increment now to find height */
        (*crop).width=(size_t) PixelRoundOffset((double) ((*offset).x+
          (geometry.x < 0 ? geometry.x : 0)));
      }
    (*crop).width=(size_t) ((ssize_t) (*crop).width-(*crop).x);
    (*crop).x+=image->page.x;
    next=CropImage(image,crop,exception);
    if (next != (Image *) NULL)
      AppendImageToList(crop_image,next);
  }
}

MagickExport Image *CropImageToTiles(const Image *image,
  const char *crop_geometry,ExceptionInfo *exception)
{
  Image
    *next,
    *crop_image;

  MagickStatusType
    flags;

  RectangleInfo
    geometry;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  flags=ParseGravityGeometry(image,crop_geometry,&geometry,exception);
  if ((flags & AreaValue) != 0)
    {
      PointInfo
        delta,
        offset;

      RectangleInfo
        crop;

      size_t
        height,
        width;

      /*
        Crop into NxM tiles (@ flag).
      */
      crop_image=NewImageList();
      width=image->columns;
      height=image->rows;
      if (geometry.width == 0)
        geometry.width=1;
      if (geometry.height == 0)
        geometry.height=1;
      if ((flags & AspectValue) == 0)
        {
          width=(size_t) ((ssize_t) width-(geometry.x < 0 ? -1 : 1)*geometry.x);
          height=(size_t) ((ssize_t) height-(geometry.y < 0 ? -1 : 1)*
            geometry.y);
        }
      else
        {
          width=(size_t) ((ssize_t) width+(geometry.x < 0 ? -1 : 1)*geometry.x);
          height=(size_t) ((ssize_t) height+(geometry.y < 0 ? -1 : 1)*
            geometry.y);
        }
      delta.x=(double) width/geometry.width;
      delta.y=(double) height/geometry.height;
      if (delta.x < 1.0)
        delta.x=1.0;
      if (delta.y < 1.0)
        delta.y=1.0;
      for (offset.y=0; offset.y < (double) height; )
      {
        AreaTileRow
          row;

        if ((flags & AspectValue) == 0)
          {
            crop.y=PixelRoundOffset((double) (offset.y-
              (geometry.y > 0 ? 0 : geometry.y)));
            offset.y+=delta.y;   /* increment now to find width */
            crop.height=(size_t) PixelRoundOffset((double) (offset.y+
              (geometry.y < 0 ? 0 : geometry.y)));
          }
        else
          {
            crop.y=PixelRoundOffset((double) (offset.y-
              (geometry.y > 0 ? geometry.y : 0)));
            offset.y+=delta.y;  /* increment now to find width */
            crop.height=(size_t) PixelRoundOffset((double)
              (offset.y+(geometry.y < -1 ? geometry.y : 0)));
          }
        crop.height=(size_t) ((ssize_t) crop.height-crop.y);
        crop.y+=image->page.y;
        row.image=image;
        row.width=width;
        row.flags=flags;
        row.geometry=geometry;
        row.delta=delta;
        row.exception=exception;
        CropAreaTileRow(&row,&offset,&crop,&crop_image);
      }
      ClearMagickException(exception);
      return(crop_image);
    }
  if (IsSingleRegionCrop(geometry,flags) != MagickFalse)
    {
      /*
        Crop a single region at +X+Y.
      */
      crop_image=CropImage(image,&geometry,exception);
      if ((crop_image != (Image *) NULL) && ((flags & AspectValue) != 0))
        {
          crop_image->page.width=geometry.width;
          crop_image->page.height=geometry.height;
          crop_image->page.x-=geometry.x;
          crop_image->page.y-=geometry.y;
        }
      return(crop_image);
    }
  if ((image->columns > geometry.width) || (image->rows > geometry.height))
    {
      RectangleInfo
        page;

      size_t
        height,
        width;

      ssize_t
        x,
        y;

      /*
        Crop into tiles of fixed size WxH.
      */
      page=image->page;
      if (page.width == 0)
        page.width=image->columns;
      if (page.height == 0)
        page.height=image->rows;
      width=geometry.width;
      if (width == 0)
        width=page.width;
      height=geometry.height;
      if (height == 0)
        height=page.height;
      next=(Image *) NULL;
      crop_image=NewImageList();
      for (y=0; y < (ssize_t) page.height; y+=(ssize_t) height)
      {
        for (x=0; x < (ssize_t) page.width; x+=(ssize_t) width)
        {
          geometry.width=width;
          geometry.height=height;
          geometry.x=x;
          geometry.y=y;
          next=CropImage(image,&geometry,exception);
          if (next == (Image *) NULL)
            break;
          AppendImageToList(&crop_image,next);
        }
        if (next == (Image *) NULL)
          break;
      }
      return(crop_image);
    }
  return(CloneImage(image,0,0,MagickTrue,exception));
}
