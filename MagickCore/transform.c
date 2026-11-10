/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%       TTTTT  RRRR    AAA   N   N  SSSSS  FFFFF   OOO   RRRR   M   M         %
%         T    R   R  A   A  NN  N  SS     F      O   O  R   R  MM MM         %
%         T    RRRR   AAAAA  N N N   SSS   FFF    O   O  RRRR   M M M         %
%         T    R R    A   A  N  NN     SS  F      O   O  R R    M   M         %
%         T    R  R   A   A  N   N  SSSSS  F       OOO   R  R   M   M         %
%                                                                             %
%                                                                             %
%                    MagickCore Image Transform Methods                       %
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
%   A u t o O r i e n t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AutoOrientImage() adjusts an image so that its orientation is suitable for
%  viewing (i.e. top-left orientation).
%
%  The format of the AutoOrientImage method is:
%
%      Image *AutoOrientImage(const Image *image,
%        const OrientationType orientation,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o orientation: Current image orientation.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static void OrientTopBottomImage(const Image *image,
  const OrientationType orientation,ExceptionInfo *exception,
  Image **orient_image)
{
  switch (orientation)
  {
    case UndefinedOrientation:
    case TopLeftOrientation:
    default:
    {
      (*orient_image)=CloneImage(image,0,0,MagickTrue,exception);
      break;
    }
    case TopRightOrientation:
    {
      (*orient_image)=FlopImage(image,exception);
      break;
    }
    case BottomRightOrientation:
    {
      (*orient_image)=RotateImage(image,180.0,exception);
      break;
    }
    case BottomLeftOrientation:
    {
      (*orient_image)=FlipImage(image,exception);
      break;
    }
  }
}

static Image *ResetImageOrientation(Image *orient_image)
{
  if (orient_image != (Image *) NULL)
    orient_image->orientation=TopLeftOrientation;
  return(orient_image);
}

MagickExport Image *AutoOrientImage(const Image *image,
  const OrientationType orientation,ExceptionInfo *exception)
{
  Image
    *orient_image;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  orient_image=(Image *) NULL;
  switch (orientation)
  {
    default:
    {
      OrientTopBottomImage(image,orientation,exception,&orient_image);
      break;
    }
    case LeftTopOrientation:
    {
      orient_image=TransposeImage(image,exception);
      break;
    }
    case RightTopOrientation:
    {
      orient_image=RotateImage(image,90.0,exception);
      break;
    }
    case RightBottomOrientation:
    {
      orient_image=TransverseImage(image,exception);
      break;
    }
    case LeftBottomOrientation:
    {
      orient_image=RotateImage(image,270.0,exception);
      break;
    }
  }
  return(ResetImageOrientation(orient_image));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     C o n s o l i d a t e C M Y K I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConsolidateCMYKImage() consolidates separate C, M, Y, and K planes into a
%  single image.
%
%  The format of the ConsolidateCMYKImage method is:
%
%      Image *ConsolidateCMYKImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image sequence.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetCMYKPlanePixel(const Image *cmyk_image,const ssize_t i,
  const Quantum pixel,Quantum *q)
{
  switch (i)
  {
    case 0: SetPixelCyan(cmyk_image,pixel,q);  break;
    case 1: SetPixelMagenta(cmyk_image,pixel,q);  break;
    case 2: SetPixelYellow(cmyk_image,pixel,q);  break;
    case 3: SetPixelBlack(cmyk_image,pixel,q);  break;
    default: break;
  }
}

static void ConsolidateCMYKPlane(const Image *images,Image *cmyk_image,
  const ssize_t i,ExceptionInfo *exception)
{
  CacheView
    *cmyk_view,
    *image_view;

  ssize_t
    y;

  image_view=AcquireVirtualCacheView(images,exception);
  cmyk_view=AcquireAuthenticCacheView(cmyk_image,exception);
  for (y=0; y < (ssize_t) images->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    ssize_t
      x;

    Quantum
      *magick_restrict q;

    p=GetCacheViewVirtualPixels(image_view,0,y,images->columns,1,exception);
    q=QueueCacheViewAuthenticPixels(cmyk_view,0,y,cmyk_image->columns,1,
      exception);
    if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
      break;
    for (x=0; x < (ssize_t) images->columns; x++)
    {
      Quantum
        pixel;

      pixel=ClampToQuantum((double) QuantumRange-
        GetPixelIntensity(images,p));
      SetCMYKPlanePixel(cmyk_image,i,pixel,q);
      p+=(ptrdiff_t) GetPixelChannels(images);
      q+=(ptrdiff_t) GetPixelChannels(cmyk_image);
    }
    if (SyncCacheViewAuthenticPixels(cmyk_view,exception) == MagickFalse)
      break;
  }
  cmyk_view=DestroyCacheView(cmyk_view);
  image_view=DestroyCacheView(image_view);
}

static const Image *ConsolidateCMYKPlanes(const Image *images,
  Image *cmyk_image,ExceptionInfo *exception)
{
  ssize_t
    i;

  for (i=0; i < 4; i++)
  {
    ConsolidateCMYKPlane(images,cmyk_image,i,exception);
    images=GetNextImageInList(images);
    if (images == (Image *) NULL)
      break;
  }
  return(images);
}

MagickExport Image *ConsolidateCMYKImages(const Image *images,
  ExceptionInfo *exception)
{
  Image
    *cmyk_image,
    *cmyk_images;

  ssize_t
    j;

  /*
    Consolidate separate C, M, Y, and K planes into a single image.
  */
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  cmyk_images=NewImageList();
  for (j=0; j < (ssize_t) GetImageListLength(images); j+=4)
  {
    assert(images != (Image *) NULL);
    cmyk_image=CloneImage(images,0,0,MagickTrue,
      exception);
    if (cmyk_image == (Image *) NULL)
      break;
    if (SetImageStorageClass(cmyk_image,DirectClass,exception) == MagickFalse)
      break;
    (void) SetImageColorspace(cmyk_image,CMYKColorspace,exception);
    images=ConsolidateCMYKPlanes(images,cmyk_image,exception);
    AppendImageToList(&cmyk_images,cmyk_image);
  }
  return(cmyk_images);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x t e n t I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExtentImage() extends the image as defined by the geometry, gravity, and
%  image background color.  Set the (x,y) offset of the geometry to move the
%  original image relative to the extended image.
%
%  The format of the ExtentImage method is:
%
%      Image *ExtentImage(const Image *image,const RectangleInfo *geometry,
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
static Image *CompositeExtentImage(const Image *image,Image *extent_image,
  const RectangleInfo *geometry,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  DisableCompositeClampUnlessSpecified(extent_image);
  status=CompositeImage(extent_image,image,image->compose,MagickTrue,
    -geometry->x,-geometry->y,exception);
  if (status != MagickFalse)
    Update8BIMClipPath(extent_image,image->columns,image->rows,geometry);
  return(extent_image);
}

MagickExport Image *ExtentImage(const Image *image,
  const RectangleInfo *geometry,ExceptionInfo *exception)
{
  Image
    *extent_image;

  MagickBooleanType
    status;

  /*
    Allocate extent image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(geometry != (const RectangleInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  extent_image=CloneImage(image,geometry->width,geometry->height,MagickTrue,
    exception);
  if (extent_image == (Image *) NULL)
    return((Image *) NULL);
  status=SetImageBackgroundColor(extent_image,exception);
  if (status == MagickFalse)
    {
      extent_image=DestroyImage(extent_image);
      return((Image *) NULL);
    }
  return(CompositeExtentImage(image,extent_image,geometry,exception));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S h a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShaveImage() shaves pixels from the image edges.  It allocates the memory
%  necessary for the new Image structure and returns a pointer to the new
%  image.
%
%  The format of the ShaveImage method is:
%
%      Image *ShaveImage(const Image *image,const RectangleInfo *shave_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o shave_image: Method ShaveImage returns a pointer to the shaved
%      image.  A null image is returned if there is a memory shortage or
%      if the image width or height is zero.
%
%    o image: the image.
%
%    o shave_info: Specifies a pointer to a RectangleInfo which defines the
%      region of the image to crop.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static RectangleInfo GetShaveGeometry(const Image *image,
  const RectangleInfo *shave_info)
{
  RectangleInfo
    geometry;

  SetGeometry(image,&geometry);
  geometry.width-=2*shave_info->width;
  geometry.height-=2*shave_info->height;
  geometry.x=(ssize_t) shave_info->width+image->page.x;
  geometry.y=(ssize_t) shave_info->height+image->page.y;
  return(geometry);
}

static void ShaveImagePage(Image *shave_image,
  const RectangleInfo *shave_info)
{
  shave_image->page.width-=2*shave_info->width;
  shave_image->page.height-=2*shave_info->height;
  shave_image->page.x-=(ssize_t) shave_info->width;
  shave_image->page.y-=(ssize_t) shave_info->height;
}

static inline MagickBooleanType IsShaveTooLarge(const Image *image,
  const RectangleInfo *shave_info)
{
  return((((2*shave_info->width) >= image->columns) ||
    ((2*shave_info->height) >= image->rows)) ? MagickTrue : MagickFalse);
}

MagickExport Image *ShaveImage(const Image *image,
  const RectangleInfo *shave_info,ExceptionInfo *exception)
{
  Image
    *shave_image;

  RectangleInfo
    geometry;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsShaveTooLarge(image,shave_info) != MagickFalse)
    ThrowImageException(OptionWarning,"GeometryDoesNotContainImage");
  geometry=GetShaveGeometry(image,shave_info);
  shave_image=CropImage(image,&geometry,exception);
  if (shave_image == (Image *) NULL)
    return((Image *) NULL);
  ShaveImagePage(shave_image,shave_info);
  return(shave_image);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s f o r m I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransformImage() is a convenience method that behaves like ResizeImage() or
%  CropImage() but accepts scaling and/or cropping information as a region
%  geometry specification.  If the operation fails, the original image handle
%  is left as is.
%
%  This should only be used for single images.
%
%  This function destroys what it assumes to be a single image list.
%  If the input image is part of a larger list, all other images in that list
%  will be simply 'lost', not destroyed.
%
%  Also if the crop generates a list of images only the first image is resized.
%  And finally if the crop succeeds and the resize failed, you will get a
%  cropped image, as well as a 'false' or 'failed' report.
%
%  This function and should probably be deprecated in favor of direct calls
%  to CropImageToTiles() or ResizeImage(), as appropriate.
%
%  The format of the TransformImage method is:
%
%      MagickBooleanType TransformImage(Image **image,const char *crop_geometry,
%        const char *image_geometry,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image The transformed image is returned as this parameter.
%
%    o crop_geometry: A crop geometry string.  This geometry defines a
%      subregion of the image to crop.
%
%    o image_geometry: An image geometry string.  This geometry defines the
%      final size of the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *CropTransformImage(Image **image,Image *transform_image,
  const char *crop_geometry,ExceptionInfo *exception)
{
  Image
    *crop_image;

  /*
    Crop image to a user specified size.
  */
  crop_image=CropImageToTiles(*image,crop_geometry,exception);
  if (crop_image == (Image *) NULL)
    transform_image=CloneImage(*image,0,0,MagickTrue,exception);
  else
    {
      transform_image=DestroyImage(transform_image);
      transform_image=GetFirstImageInList(crop_image);
    }
  *image=transform_image;
  return(transform_image);
}

static MagickBooleanType ResizeTransformImage(Image **image,
  Image *transform_image,const char *image_geometry,ExceptionInfo *exception)
{
  Image
    *resize_image;

  RectangleInfo
    geometry;

  (void) ParseRegionGeometry(transform_image,image_geometry,&geometry,
    exception);
  if ((transform_image->columns == geometry.width) &&
      (transform_image->rows == geometry.height))
    return(MagickTrue);
  resize_image=ResizeImage(transform_image,geometry.width,geometry.height,
    transform_image->filter,exception);
  if (resize_image == (Image *) NULL)
    return(MagickFalse);
  transform_image=DestroyImage(transform_image);
  transform_image=resize_image;
  *image=transform_image;
  return(MagickTrue);
}

static Image *SelectTransformImage(Image **image,const char *crop_geometry,
  ExceptionInfo *exception)
{
  Image
    *transform_image;

  transform_image=(*image);
  if (crop_geometry != (const char *) NULL)
    transform_image=CropTransformImage(image,transform_image,crop_geometry,
      exception);
  return(transform_image);
}

MagickPrivate MagickBooleanType TransformImage(Image **image,
  const char *crop_geometry,const char *image_geometry,ExceptionInfo *exception)
{
  Image
    *transform_image;

  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
  transform_image=SelectTransformImage(image,crop_geometry,exception);
  if (transform_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Scale image to a user specified size.
  */
  if (image_geometry == (const char *) NULL)
    return(MagickTrue);
  return(ResizeTransformImage(image,transform_image,image_geometry,exception));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r i m I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TrimImage() trims pixels from the image edges.  It allocates the memory
%  necessary for the new Image structure and returns a pointer to the new
%  image.
%
%  The format of the TrimImage method is:
%
%      Image *TrimImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static RectangleInfo ClampTrimExtent(const Image *image,
  const RectangleInfo minSize,RectangleInfo geometry)
{
  if (geometry.width < minSize.width)
    geometry.width=minSize.width;
  if (geometry.height < minSize.height)
    geometry.height=minSize.height;
  if (geometry.width > image->columns)
    geometry.width=image->columns;
  if (geometry.height > image->rows)
    geometry.height=image->rows;
  return(geometry);
}

static RectangleInfo ClampTrimOffset(const Image *image,
  RectangleInfo geometry)
{
  if (geometry.x < 0)
    geometry.x=0;
  if ((geometry.x+(ssize_t) geometry.width) > (ssize_t) image->columns)
    geometry.x=(ssize_t) image->columns-(ssize_t) geometry.width;
  if (geometry.y < 0)
    geometry.y=0;
  if ((geometry.y+(ssize_t) geometry.height) > (ssize_t) image->rows)
    geometry.y=(ssize_t) image->rows-(ssize_t) geometry.height;
  return(geometry);
}

static Image *TrimEmptyImage(const Image *image,ExceptionInfo *exception)
{
  Image
    *trim_image;

  /*
    The image is empty: return a minimal 1x1 transparent placeholder.
  */
  trim_image=CloneImage(image,1,1,MagickTrue,exception);
  if (trim_image != (Image *) NULL)
    {
      trim_image->background_color.alpha_trait=BlendPixelTrait;
      trim_image->background_color.alpha=(MagickRealType) TransparentAlpha;
      (void) SetImageBackgroundColor(trim_image,exception);
      trim_image->page.width=image->columns;
      trim_image->page.height=image->rows;
      trim_image->page.x=(-1);
      trim_image->page.y=(-1);
    }
  return(trim_image);
}

static void SetTrimPage(const Image *image,const RectangleInfo geometry,
  Image *trim_image)
{
  trim_image->page.width=image->columns;
  trim_image->page.height=image->rows;
  if (trim_image->page.x == 0)
    trim_image->page.x=geometry.x;
  if (trim_image->page.y == 0)
    trim_image->page.y=geometry.y;
}

static RectangleInfo SetTrimShortfall(const RectangleInfo geometry,
  RectangleInfo minSize)
{
  minSize.x=(geometry.width < minSize.width) ? (ssize_t)
    (minSize.width-geometry.width) : 0;
  minSize.y=(geometry.height < minSize.height) ? (ssize_t)
    (minSize.height-geometry.height) : 0;
  return(minSize);
}

static void OffsetTrimGeometry(const Image *image,const RectangleInfo minSize,
  RectangleInfo *geometry)
{
  switch (image->gravity)
  {
    case NorthWestGravity:
    {
      (*geometry).x-=minSize.x;
      (*geometry).y-=minSize.y;
      break;
    }
    case NorthGravity:
    {
      (*geometry).x-=minSize.x/2;
      (*geometry).y-=minSize.y;
      break;
    }
    case NorthEastGravity:
    {
      (*geometry).y-=minSize.y;
      break;
    }
    case WestGravity:
    {
      (*geometry).x-=minSize.x;
      (*geometry).y-=minSize.y/2;
      break;
    }
    case UndefinedGravity:
    case SouthEastGravity:
    default:
      break;
  }
}

static RectangleInfo ApplyTrimMinSize(const Image *image,
  const char *artifact,RectangleInfo geometry)
{
  RectangleInfo
    minSize;

  /*
    Ensure trim is not less than min size dimensions relative to gravity.
  */
  minSize=geometry;
  (void) ParseAbsoluteGeometry(artifact,&minSize);
  if ((geometry.width < minSize.width) ||
      (geometry.height < minSize.height))
    {
      minSize=SetTrimShortfall(geometry,minSize);
      switch (image->gravity)
      {
        case EastGravity:
        {
          geometry.y-=minSize.y/2;
          break;
        }
        case SouthWestGravity:
        {
          geometry.x-=minSize.x;
          break;
        }
        case SouthGravity:
        {
          geometry.x-=minSize.x/2;
          break;
        }
        case CenterGravity:
        {
          geometry.x-=minSize.x/2;
          geometry.y-=minSize.y/2;
          break;
        }
        default:
        {
          OffsetTrimGeometry(image,minSize,&geometry);
          break;
        }
      }
      geometry=ClampTrimExtent(image,minSize,geometry);
      geometry=ClampTrimOffset(image,geometry);
    }
  return(geometry);
}

static RectangleInfo OffsetTrimByPage(const Image *image,
  RectangleInfo geometry)
{
  geometry.x+=image->page.x;
  geometry.y+=image->page.y;
  return(geometry);
}

static RectangleInfo ApplyTrimArtifact(const Image *image,
  RectangleInfo geometry)
{
  const char
    *artifact;

  artifact=GetImageArtifact(image,"trim:minSize");
  if (artifact != (const char *) NULL)
    geometry=ApplyTrimMinSize(image,artifact,geometry);
  return(geometry);
}

static Image *CropTrimImage(const Image *image,RectangleInfo geometry,
  ExceptionInfo *exception)
{
  Image
    *trim_image;

  trim_image=CropImage(image,&geometry,exception);
  if (trim_image != (Image *) NULL)
    SetTrimPage(image,geometry,trim_image);
  return(trim_image);
}

MagickExport Image *TrimImage(const Image *image,ExceptionInfo *exception)
{
  RectangleInfo
    geometry;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  geometry=GetImageBoundingBox(image,exception);
  if ((geometry.width == 0) || (geometry.height == 0))
    return(TrimEmptyImage(image,exception));
  geometry=ApplyTrimArtifact(image,geometry);
  geometry=OffsetTrimByPage(image,geometry);
  return(CropTrimImage(image,geometry,exception));
}
