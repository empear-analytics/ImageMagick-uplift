/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            GGGG  EEEEE   OOO   M   M  EEEEE  TTTTT  RRRR   Y   Y            %
%           G      E      O   O  MM MM  E        T    R   R   Y Y             %
%           G  GG  EEE    O   O  M M M  EEE      T    RRRR     Y              %
%           G   G  E      O   O  M   M  E        T    R R      Y              %
%            GGG   EEEEE   OOO   M   M  EEEEE    T    R  R     Y              %
%                                                                             %
%                GGGG  RRRR    AAA   V   V  IIIII  TTTTT  Y   Y               %
%               G      R   R  A   A  V   V    I      T     Y Y                %
%               G  GG  RRRR   AAAAA  V   V    I      T      Y                 %
%               G   G  R R    A   A   V V     I      T      Y                 %
%                GGG   R  R   A   A    V    IIIII    T      Y                 %
%                                                                             %
%                                                                             %
%                         Gravity Geometry Adjustment                         %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%  Copyright 1999 ImageMagick Studio LLC, a non-profit organization           %
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
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/geometry-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G r a v i t y A d j u s t G e o m e t r y                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GravityAdjustGeometry() adjusts the offset of a region with regard to the
%  given: width, height and gravity; against which it is positioned.
%
%  The region should also have an appropriate width and height to correctly
%  set the right offset of the top left corner of the region.
%
%  The format of the GravityAdjustGeometry method is:
%
%      void GravityAdjustGeometry(const size_t width, const size_t height,
%        const GravityType gravity,RectangleInfo *region);
%
%  A description of each parameter follows:
%
%    o width, height:  the larger area the region is relative to
%
%    o gravity: the edge/corner the current offset is relative to
%
%    o region:  The region requiring a offset adjustment relative to gravity
%
*/
MagickExport void GravityAdjustGeometry(const size_t width,
  const size_t height,const GravityType gravity,RectangleInfo *region)
{
  if (region->height == 0)
    region->height=height;
  if (region->width == 0)
    region->width=width;
  switch (gravity)
  {
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
    {
      region->x=CastDoubleToSsizeT((double) width-region->width-region->x);
      break;
    }
    case NorthGravity:
    case SouthGravity:
    case CenterGravity:
    {
      region->x=CastDoubleToSsizeT((double) width/2.0-region->width/2.0+
        region->x);
      break;
    }
    case ForgetGravity:
    case NorthWestGravity:
    case WestGravity:
    case SouthWestGravity:
    default:
      break;
  }
  switch (gravity)
  {
    case SouthWestGravity:
    case SouthGravity:
    case SouthEastGravity:
    {
      region->y=CastDoubleToSsizeT((double) height-region->height-region->y);
      break;
    }
    case EastGravity:
    case WestGravity:
    case CenterGravity:
    {
      region->y=CastDoubleToSsizeT((double) height/2.0-region->height/2.0+
        region->y);
      break;
    }
    case ForgetGravity:
    case NorthWestGravity:
    case NorthGravity:
    case NorthEastGravity:
    default:
      break;
  }
  return;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e G r a v i t y G e o m e t r y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseGravityGeometry() returns a region as defined by the geometry string
%  with respect to the given image page (canvas) dimensions and the images
%  gravity setting.
%
%  This is typically used for specifying a area within a given image for
%  cropping images to a smaller size, chopping out rows and or columns, or
%  resizing and positioning overlay images.
%
%  Percentages are relative to image size and not page size, and are set to
%  nearest integer (pixel) size.
%
%  The format of the ParseGravityGeometry method is:
%
%      MagickStatusType ParseGravityGeometry(Image *image,const char *geometry,
%        RectangleInfo *region_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry string (e.g. "100x100+10+10").
%
%    o region_info: the region as defined by the geometry string with respect
%      to the image dimensions and its gravity.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickStatusType SetPercentRegion(const Image *image,
  const char *geometry,RectangleInfo *region_info,MagickStatusType flags)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    status;

  PointInfo
    scale;

  /*
    Geometry is a percentage of the image size, not canvas size
  */
  if (image->gravity != UndefinedGravity)
    flags|=XValue | YValue;
  status=ParseGeometry(geometry,&geometry_info);
  scale.x=geometry_info.rho;
  if ((status & RhoValue) == 0)
    scale.x=100.0;
  scale.y=geometry_info.sigma;
  if ((status & SigmaValue) == 0)
    scale.y=scale.x;
  region_info->width=CastDoubleToSizeT(scale.x*image->columns/100.0+0.5);
  region_info->height=CastDoubleToSizeT(scale.y*image->rows/100.0+0.5);
  return(flags);
}

static MagickStatusType SetAspectRatioRegion(const Image *image,
  const char *geometry,RectangleInfo *region_info,MagickStatusType flags)
{
  double
    geometry_ratio,
    image_ratio;

  GeometryInfo
    geometry_info;

  /*
    Geometry is a relative to image size and aspect ratio.
  */
  if (image->gravity != UndefinedGravity)
    flags|=XValue | YValue;
  (void) ParseGeometry(geometry,&geometry_info);
  geometry_ratio=geometry_info.rho;
  image_ratio=(double) image->columns/image->rows;
  region_info->width=image->columns;
  region_info->height=image->rows;
  if ((flags & MaximumValue) != 0)
    {
      if (geometry_ratio < image_ratio)
        region_info->height=CastDoubleToSizeT((double) image->rows*
          image_ratio/geometry_ratio+0.5);
      else
        region_info->width=CastDoubleToSizeT((double) image->columns*
          geometry_ratio/image_ratio+0.5);
    }
  else
    if (geometry_ratio >= image_ratio)
      region_info->height=CastDoubleToSizeT((double) image->rows*
        image_ratio/geometry_ratio+0.5);
    else
      region_info->width=CastDoubleToSizeT((double) image->columns*
        geometry_ratio/image_ratio+0.5);
  return(flags);
}

static void SetPageGeometryDefaults(const Image *image,
  RectangleInfo *region_info)
{
  SetGeometry(image,region_info);
  if (image->page.width != 0)
    region_info->width=image->page.width;
  if (image->page.height != 0)
    region_info->height=image->page.height;
}

MagickExport MagickStatusType ParseGravityGeometry(const Image *image,
  const char *geometry,RectangleInfo *region_info,ExceptionInfo *exception)
{
  MagickStatusType
    flags;

  size_t
    height,
    width;

  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",geometry);
  if ((geometry == (char *) NULL) || (*geometry == '\0'))
    return(NoValue);
  SetPageGeometryDefaults(image,region_info);
  flags=ParseAbsoluteGeometry(geometry,region_info);
  if (flags == NoValue)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidGeometry","`%s'",geometry);
      return(flags);
    }
  if ((flags & PercentValue) != 0)
    flags=SetPercentRegion(image,geometry,region_info,flags);
  if ((flags & AspectRatioValue) != 0)
    flags=SetAspectRatioRegion(image,geometry,region_info,flags);
  /*
    Adjust offset according to gravity setting.
  */
  width=region_info->width;
  height=region_info->height;
  if (width == 0)
    region_info->width=image->page.width | image->columns;
  if (height == 0)
    region_info->height=image->page.height | image->rows;
  GravityAdjustGeometry(image->columns,image->rows,image->gravity,region_info);
  if ((flags & LessValue) != 0)
    if ((region_info->width < image->columns) &&
        (region_info->height < image->rows))
      {
        SetGeometry(image,region_info);
        return(NoValue);
      }
  if ((flags & GreaterValue) != 0)
    if ((region_info->width > image->columns) &&
        (region_info->height > image->rows))
      {
        SetGeometry(image,region_info);
        return(NoValue);
      }
  region_info->width=width;
  region_info->height=height;
  return(flags);
}
