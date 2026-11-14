/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                     IIIII  M   M   AAA    GGGG  EEEEE                       %
%                       I    MM MM  A   A  G      E                           %
%                       I    M M M  AAAAA  G  GG  EEE                         %
%                       I    M   M  A   A  G   G  E                           %
%                     IIIII  M   M  A   A   GGGG  EEEEE                       %
%                                                                             %
%                                                                             %
%                           MagickCore Image Methods                          %
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

static void LimitImageDelay(Image *image,const double rho)
{
  if ((double) image->delay > floor(rho+0.5))
    image->delay=(size_t) CastDoubleToSsizeT(floor(rho+0.5));
}

static void SetImageSize(Image *image,const char *size)
{
  if (size != (char *) NULL)
    {
      (void) ParseAbsoluteGeometry(size,&image->extract_info);
      image->columns=image->extract_info.width;
      image->rows=image->extract_info.height;
      image->offset=image->extract_info.x;
      image->extract_info.x=0;
      image->extract_info.y=0;
    }
}

static void SetImageExtract(Image *image,const char *extract)
{
  MagickStatusType
    flags;

  if (extract != (char *) NULL)
    {
      RectangleInfo
        geometry;

      (void) memset(&geometry,0,sizeof(geometry));
      flags=ParseAbsoluteGeometry(extract,&geometry);
      if (((flags & XValue) != 0) || ((flags & YValue) != 0))
        {
          image->extract_info=geometry;
          Swap(image->columns,image->extract_info.width);
          Swap(image->rows,image->extract_info.height);
        }
    }
}

static void SetImageDensity(Image *image,const char *density)
{
  MagickStatusType
    flags;

  if (density != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      flags=ParseGeometry(density,&geometry_info);
      if ((flags & RhoValue) != 0)
        image->resolution.x=geometry_info.rho;
      image->resolution.y=image->resolution.x;
      if ((flags & SigmaValue) != 0)
        image->resolution.y=geometry_info.sigma;
    }
}

static void SetImagePage(Image *image,const char *page)
{
  if (page != (char *) NULL)
    {
      char
        *geometry;

      image->page=image->extract_info;
      geometry=GetPageGeometry(page);
      (void) ParseAbsoluteGeometry(geometry,&image->page);
      geometry=DestroyString(geometry);
    }
}

static void LimitImageTicks(Image *image,const GeometryInfo *geometry_info)
{
  if ((double) image->delay < floor(geometry_info->rho+0.5))
    image->ticks_per_second=CastDoubleToSsizeT(floor(geometry_info->sigma+0.5));
}

static void SetImageDelay(Image *image,const char *delay)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  if (delay == (const char *) NULL)
    return;
  flags=ParseGeometry(delay,&geometry_info);
  if ((flags & GreaterValue) != 0)
    LimitImageDelay(image,geometry_info.rho);
  else if ((flags & LessValue) != 0)
    LimitImageTicks(image,&geometry_info);
  else
    image->delay=(size_t) CastDoubleToSsizeT(floor(geometry_info.rho+0.5));
  if ((flags & SigmaValue) != 0)
    image->ticks_per_second=CastDoubleToSsizeT(floor(geometry_info.sigma+0.5));
}

static void SetImageDefaults(Image *image)
{
  MagickSizeType
    time_limit;

  (void) CopyMagickString(image->magick,"MIFF",MagickPathExtent);
  image->storage_class=DirectClass;
  image->depth=MAGICKCORE_QUANTUM_DEPTH;
  image->colorspace=sRGBColorspace;
  image->rendering_intent=PerceptualIntent;
  image->gamma=1.000/2.200;
  image->chromaticity.red_primary.x=0.6400;
  image->chromaticity.red_primary.y=0.3300;
  image->chromaticity.red_primary.z=0.0300;
  image->chromaticity.green_primary.x=0.3000;
  image->chromaticity.green_primary.y=0.6000;
  image->chromaticity.green_primary.z=0.1000;
  image->chromaticity.blue_primary.x=0.1500;
  image->chromaticity.blue_primary.y=0.0600;
  image->chromaticity.blue_primary.z=0.7900;
  image->chromaticity.white_point.x=0.3127;
  image->chromaticity.white_point.y=0.3290;
  image->chromaticity.white_point.z=0.3583;
  image->interlace=NoInterlace;
  image->ticks_per_second=UndefinedTicksPerSecond;
  image->compose=OverCompositeOp;
  GetPixelInfoRGBA(BackgroundColorRGBA,&image->background_color);
  GetPixelInfoRGBA(BorderColorRGBA,&image->border_color);
  GetPixelInfoRGBA(MatteColorRGBA,&image->matte_color);
  GetPixelInfoRGBA(TransparentColorRGBA,&image->transparent_color);
  GetTimerInfo(&image->timer);
  image->cache=AcquirePixelCache(0);
  image->channel_mask=AllChannels;
  image->channel_map=AcquirePixelChannelMap();
  image->blob=CloneBlobInfo((BlobInfo *) NULL);
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
  image->timestamp=0;  /* Deterministic for fuzzing */
#else
  image->timestamp=time((time_t *) NULL);
#endif
  time_limit=GetMagickResourceLimit(TimeResource);
  if (time_limit != MagickResourceInfinity)
    image->ttl=image->timestamp+(time_t) time_limit;
  image->debug=(GetLogEventMask() & (ImageEvent | TransformEvent | CoderEvent))
    != 0 ? MagickTrue : MagickFalse;
  image->reference_count=1;
  image->semaphore=AcquireSemaphoreInfo();
  image->signature=MagickCoreSignature;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireImage() returns a pointer to an image structure initialized to
%  default values.
%
%  The format of the AcquireImage method is:
%
%      Image *AcquireImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Many of the image default values are set from this
%      structure.  For example, filename, compression, depth, background color,
%      and others.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *AcquireImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  const char
    *option;

  Image
    *image;

  /*
    Allocate image structure.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  image=(Image *) AcquireCriticalMemory(sizeof(*image));
  (void) memset(image,0,sizeof(*image));
  /*
    Initialize Image structure.
  */
  SetImageDefaults(image);
  if (image_info == (ImageInfo *) NULL)
    return(image);
  /*
    Transfer image info.
  */
  SetBlobExempt(image,image_info->file != (FILE *) NULL ? MagickTrue :
    MagickFalse);
  (void) CopyMagickString(image->filename,image_info->filename,
    MagickPathExtent);
  (void) CopyMagickString(image->magick_filename,image_info->filename,
    MagickPathExtent);
  (void) CopyMagickString(image->magick,image_info->magick,MagickPathExtent);
  SetImageSize(image,image_info->size);
  SetImageExtract(image,image_info->extract);
  image->compression=image_info->compression;
  image->quality=image_info->quality;
  image->endian=image_info->endian;
  image->interlace=image_info->interlace;
  image->units=image_info->units;
  SetImageDensity(image,image_info->density);
  SetImagePage(image,image_info->page);
  if (image_info->depth != 0)
    image->depth=image_info->depth;
  image->dither=image_info->dither;
  image->matte_color=image_info->matte_color;
  image->background_color=image_info->background_color;
  image->border_color=image_info->border_color;
  image->transparent_color=image_info->transparent_color;
  image->ping=image_info->ping;
  image->progress_monitor=image_info->progress_monitor;
  image->client_data=image_info->client_data;
  if (image_info->cache != (void *) NULL)
    ClonePixelCacheMethods(image->cache,image_info->cache);
  /*
    Set all global options that map to per-image settings.
  */
  (void) SyncImageSettings(image_info,image,exception);
  /*
    Global options that are only set for new images.
  */
  SetImageDelay(image,GetImageOption(image_info,"delay"));
  option=GetImageOption(image_info,"dispose");
  if (option != (const char *) NULL)
    image->dispose=(DisposeType) ParseCommandOption(MagickDisposeOptions,
      MagickFalse,option);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e I m a g e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireImageInfo() allocates the ImageInfo structure.
%
%  The format of the AcquireImageInfo method is:
%
%      ImageInfo *AcquireImageInfo(void)
%
*/
MagickExport ImageInfo *AcquireImageInfo(void)
{
  ImageInfo
    *image_info;

  image_info=(ImageInfo *) AcquireCriticalMemory(sizeof(*image_info));
  GetImageInfo(image_info);
  return(image_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e N e x t I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireNextImage() initializes the next image in a sequence to
%  default values.  The next member of image points to the newly allocated
%  image.  If there is a memory shortage, next is assigned NULL.
%
%  The format of the AcquireNextImage method is:
%
%      void AcquireNextImage(const ImageInfo *image_info,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Many of the image default values are set from this
%      structure.  For example, filename, compression, depth, background color,
%      and others.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport void AcquireNextImage(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  /*
    Allocate image structure.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image->next=AcquireImage(image_info,exception);
  if (GetNextImageInList(image) == (Image *) NULL)
    return;
  (void) CopyMagickString(GetNextImageInList(image)->filename,image->filename,
    MagickPathExtent);
  if (image_info != (ImageInfo *) NULL)
    (void) CopyMagickString(GetNextImageInList(image)->filename,
      image_info->filename,MagickPathExtent);
  DestroyBlob(GetNextImageInList(image));
  image->next->blob=ReferenceBlob(image->blob);
  image->next->endian=image->endian;
  image->next->scene=image->scene+1;
  image->next->previous=image;
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C a t c h I m a g e E x c e p t i o n                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CatchImageException() returns if no exceptions are found in the image
%  sequence, otherwise it determines the most severe exception and reports
%  it as a warning or error depending on the severity.
%
%  The format of the CatchImageException method is:
%
%      ExceptionType CatchImageException(Image *image)
%
%  A description of each parameter follows:
%
%    o image: An image sequence.
%
*/
MagickExport ExceptionType CatchImageException(Image *image)
{
  ExceptionInfo
    *exception;

  ExceptionType
    severity;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  exception=AcquireExceptionInfo();
  CatchException(exception);
  severity=exception->severity;
  exception=DestroyExceptionInfo(exception);
  return(severity);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l i p I m a g e P a t h                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClipImagePath() sets the image clip mask based any clipping path information
%  if it exists.
%
%  The format of the ClipImagePath method is:
%
%      MagickBooleanType ClipImagePath(Image *image,const char *pathname,
%        const MagickBooleanType inside,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o pathname: name of clipping path resource. If name is preceded by #, use
%      clipping path numbered by name.
%
%    o inside: if non-zero, later operations take effect inside clipping path.
%      Otherwise later operations take effect outside clipping path.
%
%    o exception: return any errors or warnings in this structure.
%
*/

MagickExport MagickBooleanType ClipImage(Image *image,ExceptionInfo *exception)
{
  return(ClipImagePath(image,"#1",MagickTrue,exception));
}

MagickExport MagickBooleanType ClipImagePath(Image *image,const char *pathname,
  const MagickBooleanType inside,ExceptionInfo *exception)
{
#define ClipImagePathTag  "ClipPath/Image"

  char
    *property,
    *sanitized_pathname;

  const char
    *value;

  Image
    *clip_mask;

  ImageInfo
    *image_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(pathname != NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  property=AcquireString(pathname);
  (void) FormatLocaleString(property,MagickPathExtent,"8BIM:1999,2998:%s",
    pathname);
  value=GetImageProperty(image,property,exception);
  property=DestroyString(property);
  if (value == (const char *) NULL)
    {
      ThrowFileException(exception,OptionError,"NoClipPathDefined",
        image->filename);
      return(MagickFalse);
    }
  image_info=AcquireImageInfo();
  (void) CopyMagickString(image_info->filename,image->filename,
     MagickPathExtent);
  (void) ConcatenateMagickString(image_info->filename,"_",MagickPathExtent);
  sanitized_pathname=SanitizeString(pathname);
  (void) ConcatenateMagickString(image_info->filename,sanitized_pathname,
    MagickPathExtent);
  sanitized_pathname=DestroyString(sanitized_pathname);
  clip_mask=BlobToImage(image_info,value,strlen(value),exception);
  image_info=DestroyImageInfo(image_info);
  if (clip_mask == (Image *) NULL)
    return(MagickFalse);
  if (clip_mask->storage_class == PseudoClass)
    {
      (void) SyncImage(clip_mask,exception);
      if (SetImageStorageClass(clip_mask,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
    }
  if (inside != MagickFalse)
    (void) NegateImage(clip_mask,MagickFalse,exception);
  (void) FormatLocaleString(clip_mask->magick_filename,MagickPathExtent,
    "8BIM:1999,2998:%s\nPS",pathname);
  (void) SetImageMask(image,WritePixelMask,clip_mask,exception);
  clip_mask=DestroyImage(clip_mask);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImage() copies an image and returns the copy as a new image object.
%
%  If the specified columns and rows is 0, an exact copy of the image is
%  returned, otherwise the pixel data is undefined and must be initialized
%  with the QueueAuthenticPixels() and SyncAuthenticPixels() methods.  On
%  failure, a NULL image is returned and exception describes the reason for the
%  failure.
%
%  The format of the CloneImage method is:
%
%      Image *CloneImage(const Image *image,const size_t columns,
%        const size_t rows,const MagickBooleanType orphan,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the cloned image.
%
%    o rows: the number of rows in the cloned image.
%
%    o detach:  With a value other than 0, the cloned image is detached from
%      its parent I/O stream.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType CloneImageColormap(Image *clone_image,
  const Image *image)
{
  size_t
    length;

  if (image->colormap != (PixelInfo *) NULL)
    {
      /*
        Allocate and copy the image colormap.
      */
      clone_image->colors=image->colors;
      length=(size_t) image->colors;
      clone_image->colormap=(PixelInfo *) AcquireQuantumMemory(length+1,
        sizeof(*clone_image->colormap));
      if (clone_image->colormap == (PixelInfo *) NULL)
        {
          clone_image=DestroyImage(clone_image);
          return(MagickFalse);
        }
      (void) memcpy(clone_image->colormap,image->colormap,length*
        sizeof(*clone_image->colormap));
    }
  return(MagickTrue);
}

static void CopyImageAttributes(Image *clone_image,const Image *image,
  const MagickBooleanType detach)
{
  clone_image->signature=MagickCoreSignature;
  clone_image->storage_class=image->storage_class;
  clone_image->number_channels=image->number_channels;
  clone_image->number_meta_channels=image->number_meta_channels;
  clone_image->metacontent_extent=image->metacontent_extent;
  clone_image->colorspace=image->colorspace;
  clone_image->alpha_trait=image->alpha_trait;
  clone_image->channels=image->channels;
  clone_image->mask_trait=image->mask_trait;
  clone_image->columns=image->columns;
  clone_image->rows=image->rows;
  clone_image->dither=image->dither;
  clone_image->image_info=CloneImageInfo(image->image_info);
  (void) CloneImageProfiles(clone_image,image);
  (void) CloneImageProperties(clone_image,image);
  (void) CloneImageArtifacts(clone_image,image);
  GetTimerInfo(&clone_image->timer);
  if (image->ascii85 != (void *) NULL)
    Ascii85Initialize(clone_image);
  clone_image->extent=image->extent;
  clone_image->magick_columns=image->magick_columns;
  clone_image->magick_rows=image->magick_rows;
  clone_image->type=image->type;
  clone_image->channel_mask=image->channel_mask;
  clone_image->channel_map=ClonePixelChannelMap(image->channel_map);
  (void) CopyMagickString(clone_image->magick_filename,image->magick_filename,
    MagickPathExtent);
  (void) CopyMagickString(clone_image->magick,image->magick,MagickPathExtent);
  (void) CopyMagickString(clone_image->filename,image->filename,
    MagickPathExtent);
  clone_image->progress_monitor=image->progress_monitor;
  clone_image->client_data=image->client_data;
  clone_image->reference_count=1;
  clone_image->next=image->next;
  clone_image->previous=image->previous;
  clone_image->list=NewImageList();
  if (detach == MagickFalse)
    clone_image->blob=ReferenceBlob(image->blob);
  else
    {
      clone_image->next=NewImageList();
      clone_image->previous=NewImageList();
      clone_image->blob=CloneBlobInfo((BlobInfo *) NULL);
    }
  clone_image->ping=image->ping;
  clone_image->timestamp=image->timestamp;
  clone_image->ttl=image->ttl;
  clone_image->debug=image->debug;
  clone_image->semaphore=AcquireSemaphoreInfo();
}

static void ScaleClonePage(Image *clone_image,const Image *image,
  const size_t columns,const size_t rows)
{
  double
    scale_x,
    scale_y;

  scale_x=1.0;
  scale_y=1.0;
  if (image->columns != 0)
    scale_x=(double) columns/(double) image->columns;
  if (image->rows != 0)
    scale_y=(double) rows/(double) image->rows;
  clone_image->page.width=(size_t) CastDoubleToSsizeT(floor((double) scale_x*
    (double) image->page.width+0.5));
  clone_image->page.height=(size_t) CastDoubleToSsizeT(floor((double) scale_y*
    (double) image->page.height+0.5));
  if (MagickAbsoluteValue(scale_x-scale_y) < 2.0)
    scale_x=scale_y=MagickMin(scale_x,scale_y);
  clone_image->page.x=CastDoubleToSsizeT(ceil((double) scale_x*
    (double) image->page.x-0.5));
  clone_image->tile_offset.x=CastDoubleToSsizeT(ceil((double) scale_x*
    (double) image->tile_offset.x-0.5));
  clone_image->page.y=CastDoubleToSsizeT(ceil((double) scale_y*(double)
    (double) image->page.y-0.5));
  clone_image->tile_offset.y=CastDoubleToSsizeT(ceil((double) scale_y*
    (double) image->tile_offset.y-0.5));
}

static void CloneMontageStrings(Image *clone_image,const Image *image)
{
  if (image->montage != (char *) NULL)
    (void) CloneString(&clone_image->montage,image->montage);
  if (image->directory != (char *) NULL)
    (void) CloneString(&clone_image->directory,image->directory);
}

MagickExport Image *CloneImage(const Image *image,const size_t columns,
  const size_t rows,const MagickBooleanType detach,ExceptionInfo *exception)
{
  Image
    *clone_image;

  /*
    Clone the image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->columns == 0) || (image->rows == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
        "NegativeOrZeroImageSize","`%s'",image->filename);
      return((Image *) NULL);
    }
  clone_image=(Image *) AcquireCriticalMemory(sizeof(*clone_image));
  (void) memset(clone_image,0,sizeof(*clone_image));
  CopyImageAttributes(clone_image,image,detach);
  if (CloneImageColormap(clone_image,image) == MagickFalse)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  if ((columns == 0) || (rows == 0))
    {
      CloneMontageStrings(clone_image,image);
      clone_image->cache=ReferencePixelCache(image->cache);
      return(clone_image);
    }
  ScaleClonePage(clone_image,image,columns,rows);
  clone_image->cache=ClonePixelCache(image->cache);
  if (SetImageExtent(clone_image,columns,rows,exception) == MagickFalse)
    clone_image=DestroyImage(clone_image);
  return(clone_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageInfo() makes a copy of the given image info structure.  If
%  NULL is specified, a new image info structure is created initialized to
%  default values.
%
%  The format of the CloneImageInfo method is:
%
%      ImageInfo *CloneImageInfo(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
*/
static void CloneImageInfoRenderStrings(ImageInfo *clone_info,
  const ImageInfo *image_info)
{
  if (image_info->sampling_factor != (char *) NULL)
    (void) CloneString(&clone_info->sampling_factor,
      image_info->sampling_factor);
  if (image_info->server_name != (char *) NULL)
    (void) CloneString(&clone_info->server_name,image_info->server_name);
  if (image_info->font != (char *) NULL)
    (void) CloneString(&clone_info->font,image_info->font);
  if (image_info->texture != (char *) NULL)
    (void) CloneString(&clone_info->texture,image_info->texture);
  if (image_info->density != (char *) NULL)
    (void) CloneString(&clone_info->density,image_info->density);
}

MagickExport ImageInfo *CloneImageInfo(const ImageInfo *image_info)
{
  ImageInfo
    *clone_info;

  clone_info=AcquireImageInfo();
  if (image_info == (ImageInfo *) NULL)
    return(clone_info);
  clone_info->compression=image_info->compression;
  clone_info->temporary=image_info->temporary;
  clone_info->adjoin=image_info->adjoin;
  clone_info->antialias=image_info->antialias;
  clone_info->scene=image_info->scene;
  clone_info->number_scenes=image_info->number_scenes;
  clone_info->depth=image_info->depth;
  if (image_info->size != (char *) NULL)
    (void) CloneString(&clone_info->size,image_info->size);
  if (image_info->extract != (char *) NULL)
    (void) CloneString(&clone_info->extract,image_info->extract);
  if (image_info->scenes != (char *) NULL)
    (void) CloneString(&clone_info->scenes,image_info->scenes);
  if (image_info->page != (char *) NULL)
    (void) CloneString(&clone_info->page,image_info->page);
  clone_info->interlace=image_info->interlace;
  clone_info->endian=image_info->endian;
  clone_info->units=image_info->units;
  clone_info->quality=image_info->quality;
  CloneImageInfoRenderStrings(clone_info,image_info);
  clone_info->pointsize=image_info->pointsize;
  clone_info->fuzz=image_info->fuzz;
  clone_info->matte_color=image_info->matte_color;
  clone_info->background_color=image_info->background_color;
  clone_info->border_color=image_info->border_color;
  clone_info->transparent_color=image_info->transparent_color;
  clone_info->dither=image_info->dither;
  clone_info->monochrome=image_info->monochrome;
  clone_info->colorspace=image_info->colorspace;
  clone_info->type=image_info->type;
  clone_info->orientation=image_info->orientation;
  clone_info->ping=image_info->ping;
  clone_info->verbose=image_info->verbose;
  clone_info->progress_monitor=image_info->progress_monitor;
  clone_info->client_data=image_info->client_data;
  clone_info->cache=image_info->cache;
  if (image_info->cache != (void *) NULL)
    clone_info->cache=ReferencePixelCache(image_info->cache);
  if (image_info->profile != (void *) NULL)
    clone_info->profile=(void *) CloneStringInfo((StringInfo *)
      image_info->profile);
  SetImageInfoFile(clone_info,image_info->file);
  SetImageInfoBlob(clone_info,image_info->blob,image_info->length);
  clone_info->stream=image_info->stream;
  clone_info->custom_stream=image_info->custom_stream;
  (void) CopyMagickString(clone_info->magick,image_info->magick,
    MagickPathExtent);
  (void) CopyMagickString(clone_info->unique,image_info->unique,
    MagickPathExtent);
  (void) CopyMagickString(clone_info->filename,image_info->filename,
    MagickPathExtent);
  clone_info->channel=image_info->channel;
  (void) CloneImageOptions(clone_info,image_info);
  clone_info->coder_depth=image_info->coder_depth;
  clone_info->debug=image_info->debug;
  clone_info->signature=image_info->signature;
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o p y I m a g e P i x e l s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CopyImagePixels() copies pixels from the source image as defined by the
%  geometry the destination image at the specified offset.
%
%  The format of the CopyImagePixels method is:
%
%      MagickBooleanType CopyImagePixels(Image *image,const Image *source_image,
%        const RectangleInfo *geometry,const OffsetInfo *offset,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the destination image.
%
%    o source_image: the source image.
%
%    o geometry: define the dimensions of the source pixel rectangle.
%
%    o offset: define the offset in the destination image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsOffsetRegionOutside(const Image *image,
  const RectangleInfo *geometry,const OffsetInfo *offset)
{
  return(((offset->x < 0) || (offset->y < 0) ||
    ((offset->x+(ssize_t) geometry->width) > (ssize_t) image->columns) ||
    ((offset->y+(ssize_t) geometry->height) > (ssize_t) image->rows)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsChannelNotCopied(const PixelTrait traits,
  const PixelTrait source_traits)
{
  return(((traits == UndefinedPixelTrait) ||
    ((traits & UpdatePixelTrait) == 0) ||
    (source_traits == UndefinedPixelTrait)) ? MagickTrue : MagickFalse);
}

static inline void CopyPixelChannels(const Image *image,
  const Image *source_image,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait source_traits=GetPixelChannelTraits(source_image,channel);
    if (IsChannelNotCopied(traits,source_traits) != MagickFalse)
      continue;
    SetPixelChannel(image,channel,p[i],q);
  }
}

typedef struct _CopyRow
{
  Image
    *image;

  const Image
    *source_image;

  const RectangleInfo
    *geometry;

  const OffsetInfo
    *offset;

  CacheView
    *image_view,
    *source_view;

  ExceptionInfo
    *exception;
} CopyRow;

static MagickBooleanType CopyImageRow(const CopyRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  const Image
    *source_image = row->source_image;

  const RectangleInfo
    *geometry = row->geometry;

  const OffsetInfo
    *offset = row->offset;

  CacheView
    *image_view = row->image_view,
    *source_view = row->source_view;

  ExceptionInfo
    *exception = row->exception;

  MagickBooleanType
    sync;

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(source_view,geometry->x,y+geometry->y,
    geometry->width,1,exception);
  q=QueueCacheViewAuthenticPixels(image_view,offset->x,y+offset->y,
    geometry->width,1,exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) geometry->width; x++)
  {
    CopyPixelChannels(image,source_image,p,q);
    p+=(ptrdiff_t) GetPixelChannels(source_image);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  sync=SyncCacheViewAuthenticPixels(image_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

#define CopyImageTag  "Copy/Image"

static void SetCopyImageProgress(Image *image,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,CopyImageTag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport MagickBooleanType CopyImagePixels(Image *image,
  const Image *source_image,const RectangleInfo *geometry,
  const OffsetInfo *offset,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *source_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(source_image != (Image *) NULL);
  assert(geometry != (RectangleInfo *) NULL);
  assert(offset != (OffsetInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (IsOffsetRegionOutside(image,geometry,offset) != MagickFalse)
    ThrowBinaryException(OptionError,"GeometryDoesNotContainImage",
      image->filename);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Copy image pixels.
  */
  status=MagickTrue;
  progress=0;
  source_view=AcquireVirtualCacheView(source_image,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,source_image,geometry->height,2)
#endif
  for (y=0; y < (ssize_t) geometry->height; y++)
  {
    const CopyRow row = { image, source_image, geometry, offset, image_view,
      source_view, exception };

    if (CopyImageRow(&row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetCopyImageProgress(image,progress,&status);
      }
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImage() dereferences an image, deallocating memory associated with
%  the image if the reference count becomes zero.
%
%  The format of the DestroyImage method is:
%
%      Image *DestroyImage(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static void DestroyImageResources(Image *image)
{
  DestroyImagePixels(image);
  image->channel_map=DestroyPixelChannelMap(image->channel_map);
  if (image->montage != (char *) NULL)
    image->montage=DestroyString(image->montage);
  if (image->directory != (char *) NULL)
    image->directory=DestroyString(image->directory);
  if (image->colormap != (PixelInfo *) NULL)
    image->colormap=(PixelInfo *) RelinquishMagickMemory(image->colormap);
  if (image->geometry != (char *) NULL)
    image->geometry=DestroyString(image->geometry);
  DestroyImageProfiles(image);
  DestroyImageProperties(image);
  DestroyImageArtifacts(image);
  if (image->ascii85 != (Ascii85Info *) NULL)
    image->ascii85=(Ascii85Info *) RelinquishMagickMemory(image->ascii85);
  if (image->image_info != (ImageInfo *) NULL)
    image->image_info=DestroyImageInfo(image->image_info);
  DestroyBlob(image);
  if (image->semaphore != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&image->semaphore);
}

MagickExport Image *DestroyImage(Image *image)
{
  MagickBooleanType
    destroy;

  /*
    Dereference image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  destroy=MagickFalse;
  LockSemaphoreInfo(image->semaphore);
  image->reference_count--;
  if (image->reference_count == 0)
    destroy=MagickTrue;
  UnlockSemaphoreInfo(image->semaphore);
  if (destroy == MagickFalse)
    return((Image *) NULL);
  /*
    Destroy image.
  */
  DestroyImageResources(image);
  image->signature=(~MagickCoreSignature);
  image=(Image *) RelinquishMagickMemory(image);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageInfo() deallocates memory associated with an ImageInfo
%  structure.
%
%  The format of the DestroyImageInfo method is:
%
%      ImageInfo *DestroyImageInfo(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
*/
static void DestroyImageInfoStrings(ImageInfo *image_info)
{
  if (image_info->size != (char *) NULL)
    image_info->size=DestroyString(image_info->size);
  if (image_info->extract != (char *) NULL)
    image_info->extract=DestroyString(image_info->extract);
  if (image_info->scenes != (char *) NULL)
    image_info->scenes=DestroyString(image_info->scenes);
  if (image_info->page != (char *) NULL)
    image_info->page=DestroyString(image_info->page);
  if (image_info->sampling_factor != (char *) NULL)
    image_info->sampling_factor=DestroyString(
      image_info->sampling_factor);
  if (image_info->server_name != (char *) NULL)
    image_info->server_name=DestroyString(
      image_info->server_name);
  if (image_info->font != (char *) NULL)
    image_info->font=DestroyString(image_info->font);
}

MagickExport ImageInfo *DestroyImageInfo(ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  DestroyImageInfoStrings(image_info);
  if (image_info->texture != (char *) NULL)
    image_info->texture=DestroyString(image_info->texture);
  if (image_info->density != (char *) NULL)
    image_info->density=DestroyString(image_info->density);
  if (image_info->cache != (void *) NULL)
    image_info->cache=DestroyPixelCache(image_info->cache);
  if (image_info->profile != (StringInfo *) NULL)
    image_info->profile=(void *) DestroyStringInfo((StringInfo *)
      image_info->profile);
  DestroyImageOptions(image_info);
  image_info->signature=(~MagickCoreSignature);
  image_info=(ImageInfo *) RelinquishMagickMemory(image_info);
  return(image_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D i s a s s o c i a t e I m a g e S t r e a m                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DisassociateImageStream() disassociates the image stream.  It checks if the
%  blob of the specified image is referenced by other images. If the reference
%  count is higher then 1 a new blob is assigned to the specified image.
%
%  The format of the DisassociateImageStream method is:
%
%      void DisassociateImageStream(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void DisassociateImageStream(Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  DisassociateBlob(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageInfo() initializes image_info to default values.
%
%  The format of the GetImageInfo method is:
%
%      void GetImageInfo(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
*/
MagickExport void GetImageInfo(ImageInfo *image_info)
{
  char
    *synchronize;

  /*
    File and image dimension members.
  */
  assert(image_info != (ImageInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  (void) memset(image_info,0,sizeof(*image_info));
  image_info->adjoin=MagickTrue;
  image_info->interlace=NoInterlace;
  image_info->channel=AllChannels;
  image_info->quality=UndefinedCompressionQuality;
  image_info->antialias=MagickTrue;
  image_info->dither=MagickTrue;
  image_info->depth=0;
  synchronize=GetEnvironmentValue("MAGICK_SYNCHRONIZE");
  if (synchronize != (const char *) NULL)
    {
      image_info->synchronize=IsStringTrue(synchronize);
      synchronize=DestroyString(synchronize);
    }
  GetPixelInfoRGBA(BackgroundColorRGBA,&image_info->background_color);
  GetPixelInfoRGBA(BorderColorRGBA,&image_info->border_color);
  GetPixelInfoRGBA(MatteColorRGBA,&image_info->matte_color);
  GetPixelInfoRGBA(TransparentColorRGBA,&image_info->transparent_color);
  image_info->coder_depth=0;
  image_info->debug=(GetLogEventMask() & ImageEvent) != 0 ? MagickTrue :
    MagickFalse;
  image_info->signature=MagickCoreSignature;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e I n f o F i l e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageInfoFile() returns the image info file member.
%
%  The format of the GetImageInfoFile method is:
%
%      FILE *GetImageInfoFile(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
*/
MagickExport FILE *GetImageInfoFile(const ImageInfo *image_info)
{
  return(image_info->file);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I m a g e R e f e r e n c e C o u n t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageReferenceCount() returns the image reference count.
%
%  The format of the GetReferenceCount method is:
%
%      ssize_t GetImageReferenceCount(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport ssize_t GetImageReferenceCount(Image *image)
{
  ssize_t
    reference_count;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  LockSemaphoreInfo(image->semaphore);
  reference_count=image->reference_count;
  UnlockSemaphoreInfo(image->semaphore);
  return(reference_count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e V i r t u a l P i x e l M e t h o d                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageVirtualPixelMethod() gets the "virtual pixels" method for the
%  image.  A virtual pixel is any pixel access that is outside the boundaries
%  of the image cache.
%
%  The format of the GetImageVirtualPixelMethod() method is:
%
%      VirtualPixelMethod GetImageVirtualPixelMethod(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport VirtualPixelMethod GetImageVirtualPixelMethod(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(GetPixelCacheVirtualMethod(image));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s H i g h D y n a m i c R a n g e I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsHighDynamicRangeImage() returns MagickTrue if any pixel component is
%  non-integer or exceeds the bounds of the quantum depth (e.g. for Q16
%  0..65535.
%
%  The format of the IsHighDynamicRangeImage method is:
%
%      MagickBooleanType IsHighDynamicRangeImage(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#if defined(MAGICKCORE_HDRI_SUPPORT)
static inline MagickBooleanType IsHighDynamicRangeValue(const double pixel)
{
  return(((pixel < 0.0) || (pixel > (double) QuantumRange) ||
    (pixel != (double) ((QuantumAny) pixel))) ? MagickTrue : MagickFalse);
}

static MagickBooleanType IsIntegralQuantumPixel(const Image *image,
  const Quantum *p)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      pixel;

    PixelTrait
      traits;

    traits=GetPixelChannelTraits(image,(PixelChannel) i);
    if (traits == UndefinedPixelTrait)
      continue;
    pixel=(double) p[i];
    if (IsHighDynamicRangeValue(pixel) != MagickFalse)
      {
        status=MagickFalse;
        break;
      }
  }
  return(status);
}
#endif

#if defined(MAGICKCORE_HDRI_SUPPORT)
static MagickBooleanType HasNonIntegralPixel(const Image *image,
  const Quantum *p)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (IsIntegralQuantumPixel(image,p) == MagickFalse)
      return(MagickTrue);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(MagickFalse);
}
#endif

MagickExport MagickBooleanType IsHighDynamicRangeImage(const Image *image,
  ExceptionInfo *exception)
{
#if !defined(MAGICKCORE_HDRI_SUPPORT)
  (void) image;
  (void) exception;
  return(MagickFalse);
#else
  CacheView
    *image_view;

  MagickBooleanType
    hdri = MagickFalse;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image_view=AcquireVirtualCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(hdri) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *p;

    if (hdri != MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      continue;
    if (HasNonIntegralPixel(image,p) != MagickFalse)
      hdri=MagickTrue;
  }
  image_view=DestroyCacheView(image_view);
  return(hdri);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I s I m a g e O b j e c t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsImageObject() returns MagickTrue if the image sequence contains a valid
%  set of image objects.
%
%  The format of the IsImageObject method is:
%
%      MagickBooleanType IsImageObject(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType IsImageObject(const Image *image)
{
  const Image
    *p;

  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
    if (p->signature != MagickCoreSignature)
      return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     I s T a i n t I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsTaintImage() returns MagickTrue any pixel in the image has been altered
%  since it was first constituted.
%
%  The format of the IsTaintImage method is:
%
%      MagickBooleanType IsTaintImage(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType IsTaintImage(const Image *image)
{
  char
    magick[MagickPathExtent],
    filename[MagickPathExtent];

  const Image
    *p;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  (void) CopyMagickString(magick,image->magick,MagickPathExtent);
  (void) CopyMagickString(filename,image->filename,MagickPathExtent);
  for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (p->taint != MagickFalse)
      return(MagickTrue);
    if (LocaleCompare(p->magick,magick) != 0)
      return(MagickTrue);
    if (LocaleCompare(p->filename,filename) != 0)
      return(MagickTrue);
  }
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o d i f y I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModifyImage() ensures that there is only a single reference to the image
%  to be modified, updating the provided image pointer to point to a clone of
%  the original image if necessary.
%
%  The format of the ModifyImage method is:
%
%      MagickBooleanType ModifyImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ModifyImage(Image **image,
  ExceptionInfo *exception)
{
  Image
    *clone_image;

  assert(image != (Image **) NULL);
  assert(*image != (Image *) NULL);
  assert((*image)->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
  if (GetImageReferenceCount(*image) <= 1)
    return(MagickTrue);
  clone_image=CloneImage(*image,0,0,MagickTrue,exception);
  LockSemaphoreInfo((*image)->semaphore);
  (*image)->reference_count--;
  UnlockSemaphoreInfo((*image)->semaphore);
  *image=clone_image;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N e w M a g i c k I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewMagickImage() creates a blank image canvas of the specified size and
%  background color.
%
%  The format of the NewMagickImage method is:
%
%      Image *NewMagickImage(const ImageInfo *image_info,const size_t width,
%        const size_t height,const PixelInfo *background,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o width: the image width.
%
%    o height: the image height.
%
%    o background: the image color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *NewMagickImage(const ImageInfo *image_info,
  const size_t width,const size_t height,const PixelInfo *background,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  Image
    *image;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(background != (const PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  image=AcquireImage(image_info,exception);
  image->columns=width;
  image->rows=height;
  image->colorspace=background->colorspace;
  image->alpha_trait=background->alpha_trait;
  image->fuzz=background->fuzz;
  image->depth=background->depth;
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      SetPixelViaPixelInfo(image,background,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e f e r e n c e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReferenceImage() increments the reference count associated with an image
%  returning a pointer to the image.
%
%  The format of the ReferenceImage method is:
%
%      Image *ReferenceImage(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport Image *ReferenceImage(Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  LockSemaphoreInfo(image->semaphore);
  image->reference_count++;
  UnlockSemaphoreInfo(image->semaphore);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e P a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImagePage() resets the image page canvas and position.
%
%  The format of the ResetImagePage method is:
%
%      MagickBooleanType ResetImagePage(Image *image,const char *page)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o page: the relative page specification.
%
*/
static void AddRelativePageOffset(Image *image,const MagickStatusType flags,
  const RectangleInfo geometry)
{
  if ((flags & XValue) != 0)
    image->page.x+=geometry.x;
  if ((flags & YValue) != 0)
    image->page.y+=geometry.y;
}

static void SetAbsolutePageX(Image *image,const ssize_t x)
{
  image->page.x=x;
  if ((image->page.width == 0) && (x > 0))
    image->page.width=(size_t) ((ssize_t) image->columns+x);
}

static void SetAbsolutePageY(Image *image,const ssize_t y)
{
  image->page.y=y;
  if ((image->page.height == 0) && (y > 0))
    image->page.height=(size_t) ((ssize_t) image->rows+y);
}

static void SetAbsolutePageOffset(Image *image,const MagickStatusType flags,
  const RectangleInfo geometry)
{
  if ((flags & XValue) != 0)
    SetAbsolutePageX(image,geometry.x);
  if ((flags & YValue) != 0)
    SetAbsolutePageY(image,geometry.y);
}

MagickExport MagickBooleanType ResetImagePage(Image *image,const char *page)
{
  MagickStatusType
    flags;

  RectangleInfo
    geometry;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  flags=ParseAbsoluteGeometry(page,&geometry);
  if ((flags & WidthValue) != 0)
    {
      if ((flags & HeightValue) == 0)
        geometry.height=geometry.width;
      image->page.width=geometry.width;
      image->page.height=geometry.height;
    }
  if ((flags & AspectValue) != 0)
    AddRelativePageOffset(image,flags,geometry);
  else
    SetAbsolutePageOffset(image,flags,geometry);
  return(MagickTrue);
}

typedef struct _ResetRow
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} ResetRow;

static void ResetImagePixelsRow(const ResetRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  ExceptionInfo
    *exception = row->exception;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    (void) memset(q,0,GetPixelChannels(image)*sizeof(Quantum));
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e P i x e l s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImagePixels() reset the image pixels, that is, all the pixel components
%  are zeroed.
%
%  The format of the SetImage method is:
%
%      MagickBooleanType ResetImagePixels(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ResetImagePixels(Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  size_t
    length;

  ssize_t
    y;

  void
    *pixels;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  pixels=AcquirePixelCachePixels(image,&length,exception);
  if (pixels != (void *) NULL)
    {
      /*
        Reset in-core image pixels.
      */
      (void) memset(pixels,0,length);
      return(MagickTrue);
    }
  /*
    Reset image pixels.
  */
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const ResetRow row = { image, image_view, exception };

    ResetImagePixelsRow(&row,y,&status);
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _AlphaRow
{
  Image
    *image;

  CacheView
    *image_view;

  Quantum
    alpha;

  ExceptionInfo
    *exception;
} AlphaRow;

static void SetImageAlphaRow(const AlphaRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  const Quantum
    alpha = row->alpha;

  ExceptionInfo
    *exception = row->exception;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelWriteMask(image,q) > (QuantumRange/2))
      SetPixelAlpha(image,alpha,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S e t I m a g e A l p h a                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageAlpha() sets the alpha levels of the image.
%
%  The format of the SetImageAlpha method is:
%
%      MagickBooleanType SetImageAlpha(Image *image,const Quantum alpha,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o alpha: the level of transparency: 0 is fully transparent and QuantumRange
%      is fully opaque.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageAlpha(Image *image,const Quantum alpha,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  PixelTrait
    original_mask;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  image->alpha_trait=BlendPixelTrait;
  /* Disable mask to make sure that all pixels are changed */
  original_mask=image->mask_trait;
  image->mask_trait=UndefinedPixelTrait;
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const AlphaRow row = { image, image_view, alpha, exception };

    SetImageAlphaRow(&row,y,&status);
  }
  image_view=DestroyCacheView(image_view);
  image->mask_trait=original_mask;
  return(status);
}

typedef struct _BackgroundRow
{
  Image
    *image;

  CacheView
    *image_view;

  PixelInfo
    background;

  ExceptionInfo
    *exception;
} BackgroundRow;

static void SetImageBackgroundRow(const BackgroundRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  const PixelInfo
    background = row->background;

  ExceptionInfo
    *exception = row->exception;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    SetPixelViaPixelInfo(image,&background,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e B a c k g r o u n d C o l o r                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageBackgroundColor() initializes the image pixels to the image
%  background color.  The background color is defined by the background_color
%  member of the image structure.
%
%  The format of the SetImage method is:
%
%      MagickBooleanType SetImageBackgroundColor(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageBackgroundColor(Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  PixelInfo
    background;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((image->background_color.alpha_trait != UndefinedPixelTrait) &&
      ((image->alpha_trait & BlendPixelTrait) == 0))
    (void) SetImageAlphaChannel(image,ActivateAlphaChannel,exception);
  ConformPixelInfo(image,&image->background_color,&background,exception);
  /*
    Set image background color.
  */
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const BackgroundRow row = { image, image_view, background, exception };

    SetImageBackgroundRow(&row,y,&status);
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e C h a n n e l M a s k                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageChannelMask() sets the image channel mask from the specified channel
%  mask.
%
%  The format of the SetImageChannelMask method is:
%
%      ChannelType SetImageChannelMask(Image *image,
%        const ChannelType channel_mask)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o channel_mask: the channel mask.
%
*/
MagickExport ChannelType SetImageChannelMask(Image *image,
  const ChannelType channel_mask)
{
  return(SetPixelChannelMask(image,channel_mask));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e C o l o r                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageColor() set the entire image canvas to the specified color.
%
%  The format of the SetImageColor method is:
%
%      MagickBooleanType SetImageColor(Image *image,const PixelInfo *color,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o background: the image color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageColor(Image *image,
  const PixelInfo *color,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(color != (const PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  image->colorspace=color->colorspace;
  image->alpha_trait=color->alpha_trait;
  image->fuzz=color->fuzz;
  image->depth=color->depth;
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      SetPixelViaPixelInfo(image,color,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e S t o r a g e C l a s s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageStorageClass() sets the image class: DirectClass for true color
%  images or PseudoClass for colormapped images.
%
%  The format of the SetImageStorageClass method is:
%
%      MagickBooleanType SetImageStorageClass(Image *image,
%        const ClassType storage_class,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o storage_class:  The image class.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageStorageClass(Image *image,
  const ClassType storage_class,ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image->storage_class=storage_class;
  return(SyncImagePixelCache(image,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e E x t e n t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageExtent() sets the image size (i.e. columns & rows).
%
%  The format of the SetImageExtent method is:
%
%      MagickBooleanType SetImageExtent(Image *image,const size_t columns,
%        const size_t rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns:  The image width in pixels.
%
%    o rows:  The image height in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageExtent(Image *image,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
  if ((columns == 0) || (rows == 0))
    ThrowBinaryException(ImageError,"NegativeOrZeroImageSize",image->filename);
  if ((columns > (size_t) MAGICK_SSIZE_MAX) ||
      (rows > (size_t) MAGICK_SSIZE_MAX))
    ThrowBinaryException(ImageError,"ImageSizeLimitExceeded",image->filename);
  image->columns=columns;
  image->rows=rows;
  if (image->depth == 0)
    {
      image->depth=8;
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageDepthNotSupported","`%s'",image->filename);
    }
  if (image->depth > (8*sizeof(MagickSizeType)))
    {
      image->depth=8*sizeof(MagickSizeType);
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageDepthNotSupported","`%s'",image->filename);
    }
  return(SyncImagePixelCache(image,exception));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e I n f o B l o b                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageInfoBlob() sets the image info blob member.
%
%  The format of the SetImageInfoBlob method is:
%
%      void SetImageInfoBlob(ImageInfo *image_info,const void *blob,
%        const size_t length)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o blob: the blob.
%
%    o length: the blob length.
%
*/
MagickExport void SetImageInfoBlob(ImageInfo *image_info,const void *blob,
  const size_t length)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image_info->blob=(void *) blob;
  image_info->length=length;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e I n f o C u s t o m S t r e a m                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageInfoCustomStream() sets the image info custom stream handlers.
%
%  The format of the SetImageInfoCustomStream method is:
%
%      void SetImageInfoCustomStream(ImageInfo *image_info,
%        CustomStreamInfo *custom_stream)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o custom_stream: your custom stream methods.
%
*/
MagickExport void SetImageInfoCustomStream(ImageInfo *image_info,
  CustomStreamInfo *custom_stream)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image_info->custom_stream=(CustomStreamInfo *) custom_stream;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e I n f o F i l e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageInfoFile() sets the image info file member.
%
%  The format of the SetImageInfoFile method is:
%
%      void SetImageInfoFile(ImageInfo *image_info,FILE *file)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o file: the file.
%
*/
MagickExport void SetImageInfoFile(ImageInfo *image_info,FILE *file)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image_info->file=file;
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e V i r t u a l P i x e l M e t h o d                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageVirtualPixelMethod() sets the "virtual pixels" method for the
%  image and returns the previous setting.  A virtual pixel is any pixel access
%  that is outside the boundaries of the image cache.
%
%  The format of the SetImageVirtualPixelMethod() method is:
%
%      VirtualPixelMethod SetImageVirtualPixelMethod(Image *image,
%        const VirtualPixelMethod virtual_pixel_method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o virtual_pixel_method: choose the type of virtual pixel.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport VirtualPixelMethod SetImageVirtualPixelMethod(Image *image,
  const VirtualPixelMethod virtual_pixel_method,ExceptionInfo *exception)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(SetPixelCacheVirtualMethod(image,virtual_pixel_method,exception));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i p I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StripImage() strips an image of all profiles and comments.
%
%  The format of the StripImage method is:
%
%      MagickBooleanType StripImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType StripImage(Image *image,
  ExceptionInfo *magick_unused(exception))
{
  MagickBooleanType
    status;

  magick_unreferenced(exception);
  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  DestroyImageProfiles(image);
  (void) DeleteImageProperty(image,"comment");
  (void) DeleteImageProperty(image,"date:create");
  (void) DeleteImageProperty(image,"date:modify");
  (void) DeleteImageProperty(image,"date:timestamp");
  status=SetImageArtifact(image,"png:exclude-chunk",
    "bKGD,caNv,cHRM,eXIf,gAMA,iCCP,iTXt,pHYs,sRGB,tEXt,zCCP,zTXt,date");
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncImage() initializes the red, green, and blue intensities of each pixel
%  as defined by the colormap index.
%
%  The format of the SyncImage method is:
%
%      MagickBooleanType SyncImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline Quantum PushColormapIndex(Image *image,const Quantum index,
  MagickBooleanType *range_exception)
{
  if ((size_t) index < image->colors)
    return(index);
  *range_exception=MagickTrue;
  return((Quantum) 0);
}

typedef struct _SyncRow
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} SyncRow;

static void SyncImageRow(const SyncRow *row,const ssize_t y,
  MagickBooleanType *status,
  MagickBooleanType *range_exception)
{
  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  ExceptionInfo
    *exception = row->exception;

  Quantum
    index;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    index=PushColormapIndex(image,GetPixelIndex(image,q),range_exception);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t) index,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport MagickBooleanType SyncImage(Image *image,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    range_exception,
    status,
    taint;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (image->ping != MagickFalse)
    return(MagickTrue);
  if (image->storage_class != PseudoClass)
    return(MagickFalse);
  assert(image->colormap != (PixelInfo *) NULL);
  range_exception=MagickFalse;
  status=MagickTrue;
  taint=image->taint;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(range_exception,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const SyncRow row = { image, image_view, exception };

    SyncImageRow(&row,y,&status,&range_exception);
  }
  image_view=DestroyCacheView(image_view);
  image->taint=taint;
  if ((image->ping == MagickFalse) && (range_exception != MagickFalse))
    (void) ThrowMagickException(exception,GetMagickModule(),
      CorruptImageWarning,"InvalidColormapIndex","`%s'",image->filename);
  return(status);
}

