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
%                         M   M  EEEEE  TTTTT   AAA                           %
%                         MM MM  E        T    A   A                          %
%                         M M M  EEE      T    AAAAA                          %
%                         M   M  E        T    A   A                          %
%                         M   M  EEEEE    T    A   A                          %
%                                                                             %
%                                                                             %
%                   MagickCore Meta Geometry Parsing Method                   %
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
+   P a r s e M e t a G e o m e t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseMetaGeometry() is similar to GetGeometry() except the returned
%  geometry is modified as determined by the meta characters:  %, !, <, >, @,
%  :, and ^ in relation to image resizing.
%
%  Final image dimensions are adjusted so as to preserve the aspect ratio as
%  much as possible, while generating a integer (pixel) size, and fitting the
%  image within the specified geometry width and height.
%
%  Flags are interpreted...
%     %   geometry size is given percentage of original width and height given
%     !   do not try to preserve aspect ratio
%     <   only enlarge images smaller that geometry
%     >   only shrink images larger than geometry
%     @   fit image to contain at most this many pixels
%     :   width and height denotes an aspect ratio
%     ^   contain the given geometry given, (minimal dimensions given)
%
%  The format of the ParseMetaGeometry method is:
%
%      MagickStatusType ParseMetaGeometry(const char *geometry,ssize_t *x,
%        ssize_t *y, size_t *width,size_t *height)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry string (e.g. "100x100+10+10").
%
%    o x,y:  The x and y offset, set according to the geometry specification.
%
%    o width,height:  The width and height of original image, modified by
%      the given geometry specification.
%
*/
static inline MagickBooleanType IsMetaGeometryAspectKept(
  const MagickStatusType flags,const size_t *width,const size_t *height,
  const size_t stasis_width,const size_t stasis_height)
{
  return((((flags & AspectValue) != 0) || ((*width == stasis_width) &&
    (*height == stasis_height))) ? MagickTrue : MagickFalse);
}

static void ShrinkGeometryToSize(size_t *width,size_t *height,
  const size_t stasis_width,const size_t stasis_height)
{
  if (stasis_width < *width)
    *width=stasis_width;
  if (stasis_height < *height)
    *height=stasis_height;
}

static void EnlargeGeometryToSize(size_t *width,size_t *height,
  const size_t stasis_width,const size_t stasis_height)
{
  if (stasis_width > *width)
    *width=stasis_width;
  if (stasis_height > *height)
    *height=stasis_height;
}

static double GetAspectScaleFactor(const MagickStatusType flags,
  const size_t *width,const size_t *height,const size_t stasis_width,
  const size_t stasis_height)
{
  double
    scale_factor;

  if ((stasis_width == 0) || (stasis_height == 0))
    scale_factor=1.0;
  else
    if (((flags & RhoValue) != 0) && (flags & SigmaValue) != 0)
      {
        scale_factor=(double) *width/(double) stasis_width;
        if ((flags & MinimumValue) == 0)
          {
            if (scale_factor > ((double) *height/(double) stasis_height))
              scale_factor=(double) *height/(double) stasis_height;
          }
        else
          if (scale_factor < ((double) *height/(double) stasis_height))
            scale_factor=(double) *height/(double) stasis_height;
      }
    else
      if ((flags & RhoValue) != 0)
        {
          scale_factor=(double) *width/(double) stasis_width;
          if (((flags & MinimumValue) != 0) &&
              (scale_factor < ((double) *width/(double) stasis_height)))
            scale_factor=(double) *width/(double) stasis_height;
        }
      else
        {
          scale_factor=(double) *height/(double) stasis_height;
          if (((flags & MinimumValue) != 0) &&
              (scale_factor < ((double) *height/(double) stasis_width)))
            scale_factor=(double) *height/(double) stasis_width;
        }
  return(scale_factor);
}

static void LimitGeometryArea(const char *geometry,size_t *width,
  size_t *height,const size_t stasis_width,const size_t stasis_height)
{
  GeometryInfo
    geometry_info;

  double
    area,
    distance;

  PointInfo
    scale;

  /*
    Geometry is a maximum area in pixels.
  */
  (void) ParseGeometry(geometry,&geometry_info);
  area=geometry_info.rho+sqrt(MagickEpsilon);
  distance=sqrt((double) stasis_width*stasis_height);
  scale.x=(double) stasis_width*MagickSafeReciprocal(distance*
    MagickSafeReciprocal(sqrt(area)));
  scale.y=(double) stasis_height*MagickSafeReciprocal(distance*
    MagickSafeReciprocal(sqrt(area)));
  if ((scale.x < (double) *width) || (scale.y < (double) *height))
    {
      *width=CastDoubleToSizeT((double) stasis_width*MagickSafeReciprocal(
        distance*MagickSafeReciprocal(sqrt(area)))+0.5);
      *height=CastDoubleToSizeT((double) stasis_height*MagickSafeReciprocal(
        distance*MagickSafeReciprocal(sqrt(area)))+0.5);
    }
}

MagickExport MagickStatusType ParseMetaGeometry(const char *geometry,ssize_t *x,
  ssize_t *y,size_t *width,size_t *height)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  size_t
    stasis_height,
    stasis_width;

  /*
    Ensure the image geometry is valid.
  */
  assert(x != (ssize_t *) NULL);
  assert(y != (ssize_t *) NULL);
  assert(width != (size_t *) NULL);
  assert(height != (size_t *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",geometry);
  if ((geometry == (char *) NULL) || (*geometry == '\0'))
    return(NoValue);
  /*
    Parse geometry using GetGeometry.
  */
  stasis_width=(*width);
  stasis_height=(*height);
  SetGeometryInfo(&geometry_info);
  flags=GetGeometry(geometry,x,y,width,height);
  if ((flags & PercentValue) != 0)
    {
      MagickStatusType
        percent_flags;

      PointInfo
        scale;

      /*
        Geometry is a percentage of the image size.
      */
      percent_flags=ParseGeometry(geometry,&geometry_info);
      scale.x=geometry_info.rho;
      if ((percent_flags & RhoValue) == 0)
        scale.x=100.0;
      scale.y=geometry_info.sigma;
      if ((percent_flags & SigmaValue) == 0)
        scale.y=scale.x;
      *width=CastDoubleToSizeT(scale.x*stasis_width/100.0+0.5);
      *height=CastDoubleToSizeT(scale.y*stasis_height/100.0+0.5);
      stasis_width=(*width);
      stasis_height=(*height);
    }
  if ((flags & AspectRatioValue) != 0)
    {
      double
        geometry_ratio,
        image_ratio;

      /*
        Geometry is a relative to image size and aspect ratio.
      */
      (void) ParseGeometry(geometry,&geometry_info);
      geometry_ratio=geometry_info.rho;
      image_ratio=(double) stasis_width*MagickSafeReciprocal((double)
        stasis_height);
      if (geometry_ratio >= image_ratio)
        {
          *width=stasis_width;
          *height=CastDoubleToSizeT((double) (MagickSafeReciprocal(
            geometry_ratio)*stasis_height*image_ratio)+0.5);
        }
      else
        {
          *width=CastDoubleToSizeT(MagickSafeReciprocal(image_ratio)*
            stasis_width*geometry_ratio+0.5);
          *height=stasis_height;
        }
      stasis_width=(*width);
      stasis_height=(*height);
    }
  if (IsMetaGeometryAspectKept(flags,width,height,stasis_width,
        stasis_height) != MagickFalse)
    {
      if ((flags & RhoValue) == 0)
        *width=stasis_width;
      if ((flags & SigmaValue) == 0)
        *height=stasis_height;
    }
  else
    {
      double
        scale_factor;

      /*
        Respect aspect ratio of the image.
      */
      scale_factor=GetAspectScaleFactor(flags,width,height,stasis_width,
        stasis_height);
      *width=CastDoubleToSizeT(MagickMax(floor(scale_factor*stasis_width+0.5),
        1.0));
      *height=CastDoubleToSizeT(MagickMax(floor(scale_factor*stasis_height+0.5),
        1.0));
    }
  if ((flags & GreaterValue) != 0)
    ShrinkGeometryToSize(width,height,stasis_width,stasis_height);
  if ((flags & LessValue) != 0)
    EnlargeGeometryToSize(width,height,stasis_width,stasis_height);
  if ((flags & AreaValue) != 0)
    LimitGeometryArea(geometry,width,height,stasis_width,stasis_height);
  return(flags);
}
