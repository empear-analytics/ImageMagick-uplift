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
%                       CCCC  L       OOO   N   N  EEEEE                      %
%                      C      L      O   O  NN  N  E                          %
%                      C      L      O   O  N N N  EEE                        %
%                      C      L      O   O  N  NN  E                          %
%                       CCCC  LLLLL   OOO   N   N  EEEEE                      %
%                                                                             %
%                                                                             %
%                       MagickCore Image Clone Methods                        %
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

static void CloneStringIfSet(char **destination,const char *source)
{
  if (source != (char *) NULL)
    (void) CloneString(destination,source);
}

static void CloneMontageStrings(Image *clone_image,const Image *image)
{
  CloneStringIfSet(&clone_image->montage,image->montage);
  CloneStringIfSet(&clone_image->directory,image->directory);
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
  CloneStringIfSet(&clone_info->sampling_factor,image_info->sampling_factor);
  CloneStringIfSet(&clone_info->server_name,image_info->server_name);
  CloneStringIfSet(&clone_info->font,image_info->font);
  CloneStringIfSet(&clone_info->texture,image_info->texture);
  CloneStringIfSet(&clone_info->density,image_info->density);
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
  CloneStringIfSet(&clone_info->size,image_info->size);
  CloneStringIfSet(&clone_info->extract,image_info->extract);
  CloneStringIfSet(&clone_info->scenes,image_info->scenes);
  CloneStringIfSet(&clone_info->page,image_info->page);
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
