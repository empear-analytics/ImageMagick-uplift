/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      SSSSS  H   H  EEEEE   AAA    RRRR                      %
%                      SS     H   H  E      A   A   R   R                     %
%                       SSS   HHHHH  EEE    AAAAA   RRRR                      %
%                         SS  H   H  E      A   A   R R                       %
%                      SSSSS  H   H  EEEEE  A   A   R  R                      %
%                                                                             %
%                                                                             %
%    MagickCore Methods to Shear or Rotate an Image by an Arbitrary Angle     %
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
  Typedef declarations.
*/
typedef struct _ShearRegion
{
  size_t
    width,
    height;

  ssize_t
    x_offset,
    y_offset;
} ShearRegion;

typedef struct _CropToFitRegion
{
  double
    x_shear,
    y_shear,
    width,
    height;

  MagickBooleanType
    rotate;
} CropToFitRegion;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C r o p T o F i t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CropToFitImage() crops the sheared image as determined by the bounding box
%  as defined by width and height and shearing angles.
%
%  The format of the CropToFitImage method is:
%
%      MagickBooleanType CropToFitImage(Image **image,
%        const CropToFitRegion *region,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o region: the x_shear, y_shear, width and height that define the region
%      of the image to crop, and whether the image was rotated.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType CropToFitImage(Image **image,
  const CropToFitRegion *region,ExceptionInfo *exception)
{
  const double
    x_shear = region->x_shear,
    y_shear = region->y_shear,
    width = region->width,
    height = region->height;

  const MagickBooleanType
    rotate = region->rotate;

  Image
    *crop_image;

  PointInfo
    extent[4],
    min,
    max;

  RectangleInfo
    geometry,
    page;

  ssize_t
    i;

  /*
    Calculate the rotated image size.
  */
  extent[0].x=(double) (-width/2.0);
  extent[0].y=(double) (-height/2.0);
  extent[1].x=(double) width/2.0;
  extent[1].y=(double) (-height/2.0);
  extent[2].x=(double) (-width/2.0);
  extent[2].y=(double) height/2.0;
  extent[3].x=(double) width/2.0;
  extent[3].y=(double) height/2.0;
  for (i=3; i >= 0; i--)
  {
    extent[i].x+=x_shear*extent[i].y;
    extent[i].y+=y_shear*extent[i].x;
    if (rotate != MagickFalse)
      extent[i].x+=x_shear*extent[i].y;
    extent[i].x+=(double) (*image)->columns/2.0;
    extent[i].y+=(double) (*image)->rows/2.0;
  }
  min=extent[0];
  max=extent[0];
  for (i=1; i < 4; i++)
  {
    if (min.x > extent[i].x)
      min.x=extent[i].x;
    if (min.y > extent[i].y)
      min.y=extent[i].y;
    if (max.x < extent[i].x)
      max.x=extent[i].x;
    if (max.y < extent[i].y)
      max.y=extent[i].y;
  }
  geometry.x=CastDoubleToSsizeT(ceil(min.x-0.5));
  geometry.y=CastDoubleToSsizeT(ceil(min.y-0.5));
  geometry.width=(size_t) CastDoubleToSsizeT(floor(max.x-min.x+0.5));
  geometry.height=(size_t) CastDoubleToSsizeT(floor(max.y-min.y+0.5));
  page=(*image)->page;
  (void) ParseAbsoluteGeometry("0x0+0+0",&(*image)->page);
  crop_image=CropImage(*image,&geometry,exception);
  if (crop_image == (Image *) NULL)
    return(MagickFalse);
  crop_image->page=page;
  *image=DestroyImage(*image);
  *image=crop_image;
  return(MagickTrue);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S h e a r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XShearImage() shears the image in the X direction with a shear angle of
%  'degrees'.  Positive angles shear counter-clockwise (right-hand rule), and
%  negative angles shear clockwise.  Angles are measured relative to a vertical
%  Y-axis.  X shears will widen an image creating 'empty' triangles on the left
%  and right sides of the source image.
%
%  The format of the XShearImage method is:
%
%      MagickBooleanType XShearImage(Image *image,const double degrees,
%        const ShearRegion *region,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o degrees: A double representing the shearing angle along the X
%      axis.
%
%    o region: the width, height, x_offset and y_offset that define the
%      region of the image to shear.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType XShearImage(Image *image,const double degrees,
  const ShearRegion *region,ExceptionInfo *exception)
{
#define XShearImageTag  "XShear/Image"

  const size_t
    width = region->width,
    height = region->height;

  const ssize_t
    x_offset = region->x_offset,
    y_offset = region->y_offset;

  typedef enum
  {
    LEFT,
    RIGHT
  } ShearDirection;

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    background;

  ssize_t
    y;

  /*
    X shear image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  background=image->background_color;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,height,1)
#endif
  for (y=0; y < (ssize_t) height; y++)
  {
    double
      area,
      displacement;

    PixelInfo
      pixel,
      source,
      destination;

    Quantum
      *magick_restrict p,
      *magick_restrict q;

    ShearDirection
      direction;

    ssize_t
      i,
      step;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewAuthenticPixels(image_view,0,y_offset+y,image->columns,1,
      exception);
    if (p == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    p+=(ptrdiff_t) x_offset*(ssize_t) GetPixelChannels(image);
    displacement=degrees*(double) (y-height/2.0);
    if (displacement == 0.0)
      continue;
    if (displacement > 0.0)
      direction=RIGHT;
    else
      {
        displacement*=(-1.0);
        direction=LEFT;
      }
    step=CastDoubleToSsizeT(floor((double) displacement));
    area=(double) (displacement-step);
    step++;
    pixel=background;
    GetPixelInfo(image,&source);
    GetPixelInfo(image,&destination);
    switch (direction)
    {
      case LEFT:
      {
        /*
          Transfer pixels left-to-right.
        */
        if (step > x_offset)
          break;
        q=p-step*(ssize_t) GetPixelChannels(image);
        for (i=0; i < (ssize_t) width; i++)
        {
          if ((x_offset+i) < step)
            {
              p+=(ptrdiff_t) GetPixelChannels(image);
              GetPixelInfoPixel(image,p,&pixel);
              q+=(ptrdiff_t) GetPixelChannels(image);
              continue;
            }
          GetPixelInfoPixel(image,p,&source);
          CompositePixelInfoAreaBlend(&pixel,(double) pixel.alpha,
            &source,(double) GetPixelAlpha(image,p),area,&destination);
          SetPixelViaPixelInfo(image,&destination,q);
          GetPixelInfoPixel(image,p,&pixel);
          p+=(ptrdiff_t) GetPixelChannels(image);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        CompositePixelInfoAreaBlend(&pixel,(double) pixel.alpha,
          &background,(double) background.alpha,area,&destination);
        SetPixelViaPixelInfo(image,&destination,q);
        q+=(ptrdiff_t) GetPixelChannels(image);
        for (i=0; i < (step-1); i++)
        {
          SetPixelViaPixelInfo(image,&background,q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        break;
      }
      case RIGHT:
      {
        /*
          Transfer pixels right-to-left.
        */
        p+=(ptrdiff_t) width*GetPixelChannels(image);
        q=p+step*(ssize_t) GetPixelChannels(image);
        for (i=0; i < (ssize_t) width; i++)
        {
          p-=(ptrdiff_t)GetPixelChannels(image);
          q-=GetPixelChannels(image);
          if ((size_t) (x_offset+(ssize_t) width+step-i) > image->columns)
            continue;
          GetPixelInfoPixel(image,p,&source);
          CompositePixelInfoAreaBlend(&pixel,(double) pixel.alpha,
            &source,(double) GetPixelAlpha(image,p),area,&destination);
          SetPixelViaPixelInfo(image,&destination,q);
          GetPixelInfoPixel(image,p,&pixel);
        }
        CompositePixelInfoAreaBlend(&pixel,(double) pixel.alpha,
          &background,(double) background.alpha,area,&destination);
        q-=GetPixelChannels(image);
        SetPixelViaPixelInfo(image,&destination,q);
        for (i=0; i < (step-1); i++)
        {
          q-=GetPixelChannels(image);
          SetPixelViaPixelInfo(image,&background,q);
        }
        break;
      }
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
        proceed=SetImageProgress(image,XShearImageTag,progress,height);
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
+   Y S h e a r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  YShearImage shears the image in the Y direction with a shear angle of
%  'degrees'.  Positive angles shear counter-clockwise (right-hand rule), and
%  negative angles shear clockwise.  Angles are measured relative to a
%  horizontal X-axis.  Y shears will increase the height of an image creating
%  'empty' triangles on the top and bottom of the source image.
%
%  The format of the YShearImage method is:
%
%      MagickBooleanType YShearImage(Image *image,const double degrees,
%        const ShearRegion *region,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o degrees: A double representing the shearing angle along the Y
%      axis.
%
%    o region: the width, height, x_offset and y_offset that define the
%      region of the image to shear.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType YShearImage(Image *image,const double degrees,
  const ShearRegion *region,ExceptionInfo *exception)
{
#define YShearImageTag  "YShear/Image"

  const size_t
    width = region->width,
    height = region->height;

  const ssize_t
    x_offset = region->x_offset,
    y_offset = region->y_offset;

  typedef enum
  {
    UP,
    DOWN
  } ShearDirection;

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    background;

  ssize_t
    x;

  /*
    Y Shear image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  progress=0;
  background=image->background_color;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,width,1)
#endif
  for (x=0; x < (ssize_t) width; x++)
  {
    double
      area,
      displacement;

    PixelInfo
      pixel,
      source,
      destination;

    Quantum
      *magick_restrict p,
      *magick_restrict q;

    ShearDirection
      direction;

    ssize_t
      i,
      step;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewAuthenticPixels(image_view,x_offset+x,0,1,image->rows,
      exception);
    if (p == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    p+=(ptrdiff_t) y_offset*(ssize_t) GetPixelChannels(image);
    displacement=degrees*(double) (x-width/2.0);
    if (displacement == 0.0)
      continue;
    if (displacement > 0.0)
      direction=DOWN;
    else
      {
        displacement*=(-1.0);
        direction=UP;
      }
    step=CastDoubleToSsizeT(floor((double) displacement));
    area=(double) (displacement-step);
    step++;
    pixel=background;
    GetPixelInfo(image,&source);
    GetPixelInfo(image,&destination);
    switch (direction)
    {
      case UP:
      {
        /*
          Transfer pixels top-to-bottom.
        */
        if (step > y_offset)
          break;
        q=p-step*(ssize_t) GetPixelChannels(image);
        for (i=0; i < (ssize_t) height; i++)
        {
          if ((y_offset+i) < step)
            {
              p+=(ptrdiff_t) GetPixelChannels(image);
              GetPixelInfoPixel(image,p,&pixel);
              q+=(ptrdiff_t) GetPixelChannels(image);
              continue;
            }
          GetPixelInfoPixel(image,p,&source);
          CompositePixelInfoAreaBlend(&pixel,(double) pixel.alpha,
            &source,(double) GetPixelAlpha(image,p),area,
            &destination);
          SetPixelViaPixelInfo(image,&destination,q);
          GetPixelInfoPixel(image,p,&pixel);
          p+=(ptrdiff_t) GetPixelChannels(image);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        CompositePixelInfoAreaBlend(&pixel,(double) pixel.alpha,
          &background,(double) background.alpha,area,&destination);
        SetPixelViaPixelInfo(image,&destination,q);
        q+=(ptrdiff_t) GetPixelChannels(image);
        for (i=0; i < (step-1); i++)
        {
          SetPixelViaPixelInfo(image,&background,q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
        break;
      }
      case DOWN:
      {
        /*
          Transfer pixels bottom-to-top.
        */
        p+=(ptrdiff_t) height*GetPixelChannels(image);
        q=p+step*(ssize_t) GetPixelChannels(image);
        for (i=0; i < (ssize_t) height; i++)
        {
          p-=(ptrdiff_t)GetPixelChannels(image);
          q-=GetPixelChannels(image);
          if ((size_t) (y_offset+(ssize_t) height+step-i) > image->rows)
            continue;
          GetPixelInfoPixel(image,p,&source);
          CompositePixelInfoAreaBlend(&pixel,(double) pixel.alpha,
            &source,(double) GetPixelAlpha(image,p),area,
            &destination);
          SetPixelViaPixelInfo(image,&destination,q);
          GetPixelInfoPixel(image,p,&pixel);
        }
        CompositePixelInfoAreaBlend(&pixel,(double) pixel.alpha,
          &background,(double) background.alpha,area,&destination);
        q-=GetPixelChannels(image);
        SetPixelViaPixelInfo(image,&destination,q);
        for (i=0; i < (step-1); i++)
        {
          q-=GetPixelChannels(image);
          SetPixelViaPixelInfo(image,&background,q);
        }
        break;
      }
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
        proceed=SetImageProgress(image,YShearImageTag,progress,image->rows);
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
%   S h e a r I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShearImage() creates a new image that is a shear_image copy of an existing
%  one.  Shearing slides one edge of an image along the X or Y axis, creating
%  a parallelogram.  An X direction shear slides an edge along the X axis,
%  while a Y direction shear slides an edge along the Y axis.  The amount of
%  the shear is controlled by a shear angle.  For X direction shears, x_shear
%  is measured relative to the Y axis, and similarly, for Y direction shears
%  y_shear is measured relative to the X axis.  Empty triangles left over from
%  shearing the image are filled with the background color defined by member
%  'background_color' of the image..  ShearImage() allocates the memory
%  necessary for the new Image structure and returns a pointer to the new image.
%
%  ShearImage() is based on the paper "A Fast Algorithm for General Raster
%  Rotation" by Alan W. Paeth.
%
%  The format of the ShearImage method is:
%
%      Image *ShearImage(const Image *image,const double x_shear,
%        const double y_shear,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o x_shear, y_shear: Specifies the number of degrees to shear the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ShearIntegralImage(const Image *image,Image *integral_image,
  const PointInfo shear,ExceptionInfo *exception)
{
  Image
    *shear_image;

  MagickBooleanType
    status;

  RectangleInfo
    border_info,
    bounds;

  ShearRegion
    first_pass,
    second_pass;

  /*
    Compute image size.
  */
  bounds.width=(size_t) ((ssize_t) image->columns+CastDoubleToSsizeT(floor(
    fabs(shear.x)*image->rows+0.5)));
  bounds.x=CastDoubleToSsizeT(ceil((double) image->columns+((fabs(shear.x)*
    image->rows)-image->columns)/2.0-0.5));
  bounds.y=CastDoubleToSsizeT(ceil((double) image->rows+((fabs(shear.y)*
    bounds.width)-image->rows)/2.0-0.5));
  /*
    Surround image with border.
  */
  integral_image->border_color=integral_image->background_color;
  integral_image->compose=CopyCompositeOp;
  border_info.width=(size_t) bounds.x;
  border_info.height=(size_t) bounds.y;
  shear_image=BorderImage(integral_image,&border_info,image->compose,exception);
  integral_image=DestroyImage(integral_image);
  if (shear_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Shear the image.
  */
  if (shear_image->alpha_trait == UndefinedPixelTrait)
    (void) SetImageAlphaChannel(shear_image,OpaqueAlphaChannel,exception);
  first_pass.width=image->columns;
  first_pass.height=image->rows;
  first_pass.x_offset=bounds.x;
  first_pass.y_offset=(ssize_t) (shear_image->rows-image->rows)/2;
  status=XShearImage(shear_image,shear.x,&first_pass,exception);
  if (status == MagickFalse)
    {
      shear_image=DestroyImage(shear_image);
      return((Image *) NULL);
    }
  second_pass.width=bounds.width;
  second_pass.height=image->rows;
  second_pass.x_offset=(ssize_t) (shear_image->columns-bounds.width)/2;
  second_pass.y_offset=bounds.y;
  status=YShearImage(shear_image,shear.y,&second_pass,exception);
  if (status == MagickFalse)
    {
      shear_image=DestroyImage(shear_image);
      return((Image *) NULL);
    }
  return(shear_image);
}

MagickExport Image *ShearImage(const Image *image,const double x_shear,
  const double y_shear,ExceptionInfo *exception)
{
  CropToFitRegion
    crop_region;

  Image
    *integral_image,
    *shear_image;

  MagickBooleanType
    status;

  PointInfo
    shear;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((x_shear != 0.0) && (fmod(x_shear,90.0) == 0.0))
    ThrowImageException(ImageError,"AngleIsDiscontinuous");
  if ((y_shear != 0.0) && (fmod(y_shear,90.0) == 0.0))
    ThrowImageException(ImageError,"AngleIsDiscontinuous");
  /*
    Initialize shear angle.
  */
  integral_image=CloneImage(image,0,0,MagickTrue,exception);
  if (integral_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  shear.x=(-tan(DegreesToRadians(fmod(x_shear,360.0))));
  shear.y=tan(DegreesToRadians(fmod(y_shear,360.0)));
  if ((shear.x == 0.0) && (shear.y == 0.0))
    return(integral_image);
  if (SetImageStorageClass(integral_image,DirectClass,exception) == MagickFalse)
    {
      integral_image=DestroyImage(integral_image);
      return(integral_image);
    }
  if (integral_image->alpha_trait == UndefinedPixelTrait)
    (void) SetImageAlphaChannel(integral_image,OpaqueAlphaChannel,exception);
  shear_image=ShearIntegralImage(image,integral_image,shear,exception);
  if (shear_image == (Image *) NULL)
    return((Image *) NULL);
  crop_region.x_shear=shear.x;
  crop_region.y_shear=shear.y;
  crop_region.width=(MagickRealType) image->columns;
  crop_region.height=(MagickRealType) image->rows;
  crop_region.rotate=MagickFalse;
  status=CropToFitImage(&shear_image,&crop_region,exception);
  shear_image->alpha_trait=image->alpha_trait;
  shear_image->compose=image->compose;
  shear_image->page.width=0;
  shear_image->page.height=0;
  if (status == MagickFalse)
    shear_image=DestroyImage(shear_image);
  return(shear_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S h e a r R o t a t e I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShearRotateImage() creates a new image that is a rotated copy of an existing
%  one.  Positive angles rotate counter-clockwise (right-hand rule), while
%  negative angles rotate clockwise.  Rotated images are usually larger than
%  the originals and have 'empty' triangular corners.  X axis.  Empty
%  triangles left over from shearing the image are filled with the background
%  color defined by member 'background_color' of the image.  ShearRotateImage
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  ShearRotateImage() is based on the paper "A Fast Algorithm for General
%  Raster Rotation" by Alan W. Paeth.  ShearRotateImage is adapted from a
%  similar method based on the Paeth paper written by Michael Halle of the
%  Spatial Imaging Group, MIT Media Lab.
%
%  The format of the ShearRotateImage method is:
%
%      Image *ShearRotateImage(const Image *image,const double degrees,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o degrees: Specifies the number of degrees to rotate the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *ShearRotateImage(const Image *image,const double degrees,
  ExceptionInfo *exception)
{
  CropToFitRegion
    crop_region;

  Image
    *integral_image,
    *rotate_image;

  MagickBooleanType
    status;

  MagickRealType
    angle;

  PointInfo
    shear;

  RectangleInfo
    border_info,
    bounds;

  ShearRegion
    first_pass,
    second_pass,
    third_pass;

  size_t
    height,
    rotations,
    shear_width,
    width;

  /*
    Adjust rotation angle.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  angle=fmod(degrees,360.0);
  if (angle < -45.0)
    angle+=360.0;
  for (rotations=0; angle > 45.0; rotations++)
    angle-=90.0;
  rotations%=4;
  /*
    Calculate shear equations.
  */
  integral_image=IntegralRotateImage(image,rotations,exception);
  if (integral_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  shear.x=(-tan((double) DegreesToRadians(angle)/2.0));
  shear.y=sin((double) DegreesToRadians(angle));
  if ((shear.x == 0.0) && (shear.y == 0.0))
    return(integral_image);
  if (SetImageStorageClass(integral_image,DirectClass,exception) == MagickFalse)
    {
      integral_image=DestroyImage(integral_image);
      return(integral_image);
    }
  if (integral_image->alpha_trait == UndefinedPixelTrait)
    (void) SetImageAlphaChannel(integral_image,OpaqueAlphaChannel,exception);
  /*
    Compute maximum bounds for 3 shear operations.
  */
  width=integral_image->columns;
  height=integral_image->rows;
  bounds.width=CastDoubleToSizeT(fabs((double) height*shear.x)+width+0.5);
  bounds.height=CastDoubleToSizeT(fabs((double) bounds.width*shear.y)+height+0.5);
  shear_width=CastDoubleToSizeT(fabs((double) bounds.height*shear.x)+
    bounds.width+0.5);
  bounds.x=CastDoubleToSsizeT(floor((double) ((shear_width > bounds.width) ?
    width : bounds.width-shear_width+2)/2.0+0.5));
  bounds.y=CastDoubleToSsizeT(floor(((double) bounds.height-height+2)/2.0+0.5));
  /*
    Surround image with a border.
  */
  integral_image->border_color=integral_image->background_color;
  integral_image->compose=CopyCompositeOp;
  border_info.width=(size_t) bounds.x;
  border_info.height=(size_t) bounds.y;
  rotate_image=BorderImage(integral_image,&border_info,image->compose,
    exception);
  integral_image=DestroyImage(integral_image);
  if (rotate_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Rotate the image.
  */
  first_pass.width=width;
  first_pass.height=height;
  first_pass.x_offset=bounds.x;
  first_pass.y_offset=(ssize_t) (rotate_image->rows-height)/2;
  status=XShearImage(rotate_image,shear.x,&first_pass,exception);
  if (status == MagickFalse)
    {
      rotate_image=DestroyImage(rotate_image);
      return((Image *) NULL);
    }
  second_pass.width=bounds.width;
  second_pass.height=height;
  second_pass.x_offset=(ssize_t) (rotate_image->columns-bounds.width)/2;
  second_pass.y_offset=bounds.y;
  status=YShearImage(rotate_image,shear.y,&second_pass,exception);
  if (status == MagickFalse)
    {
      rotate_image=DestroyImage(rotate_image);
      return((Image *) NULL);
    }
  third_pass.width=bounds.width;
  third_pass.height=bounds.height;
  third_pass.x_offset=(ssize_t) (rotate_image->columns-bounds.width)/2;
  third_pass.y_offset=(ssize_t) (rotate_image->rows-bounds.height)/2;
  status=XShearImage(rotate_image,shear.x,&third_pass,exception);
  if (status == MagickFalse)
    {
      rotate_image=DestroyImage(rotate_image);
      return((Image *) NULL);
    }
  crop_region.x_shear=shear.x;
  crop_region.y_shear=shear.y;
  crop_region.width=(MagickRealType) width;
  crop_region.height=(MagickRealType) height;
  crop_region.rotate=MagickTrue;
  status=CropToFitImage(&rotate_image,&crop_region,exception);
  rotate_image->alpha_trait=image->alpha_trait;
  rotate_image->compose=image->compose;
  rotate_image->page.width=0;
  rotate_image->page.height=0;
  if (status == MagickFalse)
    rotate_image=DestroyImage(rotate_image);
  return(rotate_image);
}
