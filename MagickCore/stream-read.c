/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS  TTTTT  RRRR   EEEEE   AAA   M   M                   %
%                  SS       T    R   R  E      A   A  MM MM                   %
%                   SSS     T    RRRR   EEE    AAAAA  M M M                   %
%                     SS    T    R R    E      A   A  M   M                   %
%                  SSSSS    T    R  R   EEEEE  A   A  M   M                   %
%                                                                             %
%                         RRRR   EEEEE   AAA   DDDD                           %
%                         R   R  E      A   A  D   D                          %
%                         RRRR   EEE    AAAAA  D   D                          %
%                         R R    E      A   A  D   D                          %
%                         R  R   EEEEE  A   A  DDDD                           %
%                                                                             %
%                                                                             %
%                       MagickCore Read Stream Methods                        %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 March 2000                                  %
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
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/policy.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/stream.h"
#include "MagickCore/stream-private.h"
#include "MagickCore/string_.h"

/*
  Forward declarations.
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static const Quantum
  *GetVirtualPixelStream(const Image *,const VirtualPixelMethod,const ssize_t,
    const ssize_t,const size_t,const size_t,ExceptionInfo *);

static MagickBooleanType
  SyncAuthenticPixelsStream(Image *,ExceptionInfo *);

static Quantum
  *QueueAuthenticPixelsStream(Image *,const ssize_t,const ssize_t,const size_t,
    const size_t,ExceptionInfo *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y P i x e l S t r e a m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyPixelStream() deallocates memory associated with the pixel stream.
%
%  The format of the DestroyPixelStream() method is:
%
%      void DestroyPixelStream(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/

static inline void RelinquishStreamPixels(CacheInfo *cache_info)
{
  assert(cache_info != (CacheInfo *) NULL);
  if (cache_info->pixels != (Quantum *) NULL)
    {
      if (cache_info->mapped == MagickFalse)
        cache_info->pixels=(Quantum *) RelinquishAlignedMemory(
          cache_info->pixels);
      else
        {
          (void) UnmapBlob(cache_info->pixels,(size_t) cache_info->length);
          cache_info->pixels=(Quantum *) NULL;
        }
    }
  cache_info->mapped=MagickFalse;
  cache_info->metacontent=(void *) NULL;
  cache_info->length=0;
}

static void DestroyPixelStream(Image *image)
{
  CacheInfo
    *cache_info;

  MagickBooleanType
    destroy;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  destroy=MagickFalse;
  LockSemaphoreInfo(cache_info->semaphore);
  cache_info->reference_count--;
  if (cache_info->reference_count == 0)
    destroy=MagickTrue;
  UnlockSemaphoreInfo(cache_info->semaphore);
  if (destroy == MagickFalse)
    return;
  RelinquishStreamPixels(cache_info);
  if (cache_info->nexus_info != (NexusInfo **) NULL)
    cache_info->nexus_info=DestroyPixelCacheNexus(cache_info->nexus_info,
      cache_info->number_threads);
  if (cache_info->file_semaphore != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&cache_info->file_semaphore);
  if (cache_info->semaphore != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&cache_info->semaphore);
  cache_info=(CacheInfo *) RelinquishAlignedMemory(cache_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t A u t h e n t i c M e t a c o n t e n t F r o m S t r e a m         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticMetacontentFromStream() returns the metacontent corresponding
%  with the last call to QueueAuthenticPixelsStream() or
%  GetAuthenticPixelsStream().
%
%  The format of the GetAuthenticMetacontentFromStream() method is:
%
%      void *GetAuthenticMetacontentFromStream(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static CacheInfo *GetStreamCacheInfo(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  return(cache_info);
}

static void *GetAuthenticMetacontentFromStream(const Image *image)
{
  CacheInfo
    *cache_info;

  cache_info=GetStreamCacheInfo(image);
  return(cache_info->metacontent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t A u t h e n t i c P i x e l S t r e a m                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticPixelsStream() gets pixels from the in-memory or disk pixel
%  cache as defined by the geometry parameters.   A pointer to the pixels is
%  returned if the pixels are transferred, otherwise a NULL is returned.  For
%  streams this method is a no-op.
%
%  The format of the GetAuthenticPixelsStream() method is:
%
%      Quantum *GetAuthenticPixelsStream(Image *image,const ssize_t x,
%        const ssize_t y,const size_t columns,const size_t rows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Quantum *GetAuthenticPixelsStream(Image *image,const ssize_t x,
  const ssize_t y,const size_t columns,const size_t rows,
  ExceptionInfo *exception)
{
  Quantum
    *pixels;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  pixels=QueueAuthenticPixelsStream(image,x,y,columns,rows,exception);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t A u t h e n t i c P i x e l F r o m S t e a m                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticPixelsFromStream() returns the pixels associated with the last
%  call to QueueAuthenticPixelsStream() or GetAuthenticPixelsStream().
%
%  The format of the GetAuthenticPixelsFromStream() method is:
%
%      Quantum *GetAuthenticPixelsFromStream(const Image image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static Quantum *GetAuthenticPixelsFromStream(const Image *image)
{
  CacheInfo
    *cache_info;

  cache_info=GetStreamCacheInfo(image);
  return(cache_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t O n e A u t h e n t i c P i x e l F r o m S t r e a m               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOneAuthenticPixelFromStream() returns a single pixel at the specified
%  (x,y) location.  The image background color is returned if an error occurs.
%
%  The format of the GetOneAuthenticPixelFromStream() method is:
%
%      MagickBooleanType GetOneAuthenticPixelFromStream(const Image image,
%        const ssize_t x,const ssize_t y,Quantum *pixel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o pixel: return a pixel at the specified (x,y) location.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetStreamBackgroundPixel(const Image *image,Quantum *pixel)
{
  pixel[RedPixelChannel]=ClampToQuantum(image->background_color.red);
  pixel[GreenPixelChannel]=ClampToQuantum(image->background_color.green);
  pixel[BluePixelChannel]=ClampToQuantum(image->background_color.blue);
  pixel[BlackPixelChannel]=ClampToQuantum(image->background_color.black);
  pixel[AlphaPixelChannel]=ClampToQuantum(image->background_color.alpha);
}

static MagickBooleanType GetOneAuthenticPixelFromStream(Image *image,
  const ssize_t x,const ssize_t y,Quantum *pixel,ExceptionInfo *exception)
{
  Quantum
    *p;

  ssize_t
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  (void) memset(pixel,0,MaxPixelChannels*sizeof(*pixel));
  p=GetAuthenticPixelsStream(image,x,y,1,1,exception);
  if (p == (Quantum *) NULL)
    {
      SetStreamBackgroundPixel(image,pixel);
      return(MagickFalse);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    pixel[channel]=p[i];
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t O n e V i r t u a l P i x e l F r o m S t r e a m                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOneVirtualPixelFromStream() returns a single pixel at the specified
%  (x.y) location.  The image background color is returned if an error occurs.
%
%  The format of the GetOneVirtualPixelFromStream() method is:
%
%      MagickBooleanType GetOneVirtualPixelFromStream(const Image image,
%        const VirtualPixelMethod virtual_pixel_method,const ssize_t x,
%        const ssize_t y,Quantum *pixel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o virtual_pixel_method: the virtual pixel method.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o pixel: return a pixel at the specified (x,y) location.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType GetOneVirtualPixelFromStream(const Image *image,
  const VirtualPixelMethod virtual_pixel_method,const ssize_t x,const ssize_t y,
  Quantum *pixel,ExceptionInfo *exception)
{
  const Quantum
    *p;

  ssize_t
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  (void) memset(pixel,0,MaxPixelChannels*sizeof(*pixel));
  p=GetVirtualPixelStream(image,virtual_pixel_method,x,y,1,1,exception);
  if (p == (const Quantum *) NULL)
    {
      pixel[RedPixelChannel]=ClampToQuantum(image->background_color.red);
      pixel[GreenPixelChannel]=ClampToQuantum(image->background_color.green);
      pixel[BluePixelChannel]=ClampToQuantum(image->background_color.blue);
      pixel[BlackPixelChannel]=ClampToQuantum(image->background_color.black);
      pixel[AlphaPixelChannel]=ClampToQuantum(image->background_color.alpha);
      return(MagickFalse);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    pixel[channel]=p[i];
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t V i r t u a l P i x e l s F r o m S t r e a m                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelsStream() returns the pixels associated with the last call to
%  QueueAuthenticPixelsStream() or GetVirtualPixelStream().
%
%  The format of the GetVirtualPixelsStream() method is:
%
%      const Quantum *GetVirtualPixelsStream(const Image *image)
%
%  A description of each parameter follows:
%
%    o pixels: return the pixels associated corresponding with the last call to
%      QueueAuthenticPixelsStream() or GetVirtualPixelStream().
%
%    o image: the image.
%
*/
static const Quantum *GetVirtualPixelsStream(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  return(cache_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t V i r t u a l I n d e x e s F r o m S t r e a m                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualMetacontentFromStream() returns the associated pixel channels
%  corresponding with the last call to QueueAuthenticPixelsStream() or
%  GetVirtualPixelStream().
%
%  The format of the GetVirtualMetacontentFromStream() method is:
%
%      const void *GetVirtualMetacontentFromStream(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static const void *GetVirtualMetacontentFromStream(const Image *image)
{
  CacheInfo
    *cache_info;

  cache_info=GetStreamCacheInfo(image);
  return(cache_info->metacontent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t V i r t u a l P i x e l S t r e a m                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelStream() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned if
%  the pixels are transferred, otherwise a NULL is returned.  For streams this
%  method is a no-op.
%
%  The format of the GetVirtualPixelStream() method is:
%
%      const Quantum *GetVirtualPixelStream(const Image *image,
%        const VirtualPixelMethod virtual_pixel_method,const ssize_t x,
%        const ssize_t y,const size_t columns,const size_t rows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o virtual_pixel_method: the virtual pixel method.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType AcquireStreamPixels(CacheInfo *cache_info,
  ExceptionInfo *exception)
{
  if (cache_info->length != (MagickSizeType) ((size_t) cache_info->length))
    return(MagickFalse);
  cache_info->pixels=(Quantum *) MagickAssumeAligned(AcquireAlignedMemory(1,
    (size_t) cache_info->length));
  if (cache_info->pixels != (Quantum *) NULL)
    (void) memset(cache_info->pixels,0,(size_t) cache_info->length);
  else
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",
        cache_info->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static inline MagickBooleanType IsOutsideStreamRegion(const Image *image,
  const ssize_t x,const ssize_t y,const size_t columns,const size_t rows)
{
  return(((x < 0) || (y < 0) ||
    ((x+(ssize_t) columns) > (ssize_t) image->columns) ||
    ((y+(ssize_t) rows) > (ssize_t) image->rows) ||
    (columns == 0) || (rows == 0)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType AcquireStreamLength(CacheInfo *cache_info,
  const size_t length,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  cache_info->length=length;
  status=AcquireStreamPixels(cache_info,exception);
  if (status == MagickFalse)
    {
      cache_info->length=0;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static Quantum *AcquireStreamRegion(CacheInfo *cache_info,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
  MagickSizeType
    number_pixels;

  size_t
    length;

  number_pixels=(MagickSizeType) columns*rows;
  length=(size_t) number_pixels*cache_info->number_channels*sizeof(Quantum);
  if (cache_info->number_channels == 0)
    length=(size_t) number_pixels*sizeof(Quantum);
  if (cache_info->metacontent_extent != 0)
    length+=(size_t) number_pixels*cache_info->metacontent_extent;
  if (cache_info->pixels == (Quantum *) NULL)
    {
      if (AcquireStreamLength(cache_info,length,exception) == MagickFalse)
        return((Quantum *) NULL);
    }
  else
    if (cache_info->length < length)
      {
        RelinquishStreamPixels(cache_info);
        if (AcquireStreamLength(cache_info,length,exception) == MagickFalse)
          return((Quantum *) NULL);
      }
  cache_info->metacontent=(void *) NULL;
  if (cache_info->metacontent_extent != 0)
    cache_info->metacontent=(void *) (cache_info->pixels+number_pixels*
      cache_info->number_channels);
  return(cache_info->pixels);
}

static const Quantum *GetVirtualPixelStream(const Image *image,
  const VirtualPixelMethod magick_unused(virtual_pixel_method),const ssize_t x,
  const ssize_t y,const size_t columns,const size_t rows,
  ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  magick_unreferenced(virtual_pixel_method);

  /*
    Validate pixel cache geometry.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsOutsideStreamRegion(image,x,y,columns,rows) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "ImageDoesNotContainTheStreamGeometry","`%s'",image->filename);
      return((Quantum *) NULL);
    }
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  /*
    Pixels are stored in a temporary buffer until they are synced to the cache.
  */
  return(AcquireStreamRegion(cache_info,columns,rows,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   Q u e u e A u t h e n t i c P i x e l s S t r e a m                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QueueAuthenticPixelsStream() allocates an area to store image pixels as
%  defined by the region rectangle and returns a pointer to the area.  This
%  area is subsequently transferred from the pixel cache with method
%  SyncAuthenticPixelsStream().  A pointer to the pixels is returned if the
%  pixels are transferred, otherwise a NULL is returned.
%
%  The format of the QueueAuthenticPixelsStream() method is:
%
%      Quantum *QueueAuthenticPixelsStream(Image *image,const ssize_t x,
%        const ssize_t y,const size_t columns,const size_t rows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/

static inline MagickBooleanType IsCacheGeometryChanged(const Image *image,
  const CacheInfo *cache_info)
{
  return((MagickBooleanType) ((image->storage_class != cache_info->storage_class) ||
    (image->colorspace != cache_info->colorspace) ||
    (image->alpha_trait != cache_info->alpha_trait) ||
    (image->channels != cache_info->channels) ||
    (image->columns != cache_info->columns) ||
    (image->rows != cache_info->rows) ||
    (image->number_channels != cache_info->number_channels)));
}

static inline MagickBooleanType IsCacheMorphologyChanged(const Image *image,
  const CacheInfo *cache_info,const PixelChannelMap *p,const PixelChannelMap *q)
{
  return((MagickBooleanType) (
    (IsCacheGeometryChanged(image,cache_info) != MagickFalse) ||
    (memcmp(p,q,image->number_channels*sizeof(*p)) != 0) ||
    (image->metacontent_extent != cache_info->metacontent_extent) ||
    (cache_info->nexus_info == (NexusInfo **) NULL)));
}

static inline MagickBooleanType ValidatePixelCacheMorphology(
  const Image *magick_restrict image)
{
  const CacheInfo
    *magick_restrict cache_info;

  const PixelChannelMap
    *magick_restrict p,
    *magick_restrict q;

  /*
    Does the image match the pixel cache morphology?
  */
  cache_info=(CacheInfo *) image->cache;
  p=image->channel_map;
  q=cache_info->channel_map;
  if (IsCacheMorphologyChanged(image,cache_info,p,q) != MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsOutsideStreamPixel(const Image *image,
  const ssize_t x,const ssize_t y)
{
  return(((image->columns == 0) || (image->rows == 0) || (x < 0) ||
    (y < 0) || (x >= (ssize_t) image->columns) ||
    (y >= (ssize_t) image->rows)) ? MagickTrue : MagickFalse);
}

static Quantum *QueueAuthenticPixelsStream(Image *image,const ssize_t x,
  const ssize_t y,const size_t columns,const size_t rows,
  ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  MagickBooleanType
    status;

  StreamHandler
    stream_handler;

  /*
    Validate pixel cache geometry.
  */
  assert(image != (Image *) NULL);
  if (IsOutsideStreamPixel(image,x,y) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "ImageDoesNotContainTheStreamGeometry","`%s'",image->filename);
      return((Quantum *) NULL);
    }
  stream_handler=GetBlobStreamHandler(image);
  if (stream_handler == (StreamHandler) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "NoStreamHandlerIsDefined","`%s'",image->filename);
      return((Quantum *) NULL);
    }
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (ValidatePixelCacheMorphology(image) == MagickFalse)
    {
      if (cache_info->storage_class == UndefinedClass)
        (void) stream_handler(image,(const void *) NULL,(size_t)
          cache_info->columns);
      cache_info->storage_class=image->storage_class;
      cache_info->colorspace=image->colorspace;
      cache_info->alpha_trait=image->alpha_trait;
      cache_info->channels=image->channels;
      cache_info->columns=image->columns;
      cache_info->rows=image->rows;
      cache_info->number_channels=image->number_channels;
      status=ResetPixelChannelMap(image,exception);
      if (status == MagickFalse)
        return((Quantum *) NULL);
      ResetPixelCacheChannels(image);
      image->cache=cache_info;
    }
  /*
    Pixels are stored in a temporary buffer until they are synced to the cache.
  */
  cache_info->columns=columns;
  cache_info->rows=rows;
  return(AcquireStreamRegion(cache_info,columns,rows,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d S t r e a m                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadStream() makes the image pixels available to a user supplied callback
%  method immediately upon reading a scanline with the ReadImage() method.
%
%  The format of the ReadStream() method is:
%
%      Image *ReadStream(const ImageInfo *image_info,StreamHandler stream,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o stream: a callback method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *ReadStream(const ImageInfo *image_info,StreamHandler stream,
  ExceptionInfo *exception)
{
  CacheMethods
    cache_methods;

  Image
    *image;

  ImageInfo
    *read_info;

  /*
    Stream image pixels.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  read_info=CloneImageInfo(image_info);
  read_info->cache=AcquirePixelCache(0);
  GetPixelCacheMethods(&cache_methods);
  cache_methods.get_virtual_pixel_handler=GetVirtualPixelStream;
  cache_methods.get_virtual_pixels_handler=GetVirtualPixelsStream;
  cache_methods.get_virtual_metacontent_from_handler=
    GetVirtualMetacontentFromStream;
  cache_methods.get_authentic_pixels_handler=GetAuthenticPixelsStream;
  cache_methods.queue_authentic_pixels_handler=QueueAuthenticPixelsStream;
  cache_methods.sync_authentic_pixels_handler=SyncAuthenticPixelsStream;
  cache_methods.get_authentic_pixels_from_handler=GetAuthenticPixelsFromStream;
  cache_methods.get_authentic_metacontent_from_handler=
    GetAuthenticMetacontentFromStream;
  cache_methods.get_one_virtual_pixel_from_handler=GetOneVirtualPixelFromStream;
  cache_methods.get_one_authentic_pixel_from_handler=
    GetOneAuthenticPixelFromStream;
  cache_methods.destroy_pixel_handler=DestroyPixelStream;
  SetPixelCacheMethods(read_info->cache,&cache_methods);
  read_info->stream=stream;
  image=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  if (image != (Image *) NULL)
    {
      MagickBooleanType status = ResetPixelChannelMap(image,exception);
      if (status == MagickFalse)
        return(DestroyImage(image));
      ResetPixelCacheChannels(image);
    }
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c A u t h e n t i c P i x e l s S t r e a m                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncAuthenticPixelsStream() calls the user supplied callback method with
%  the latest stream of pixels.
%
%  The format of the SyncAuthenticPixelsStream method is:
%
%      MagickBooleanType SyncAuthenticPixelsStream(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType SyncAuthenticPixelsStream(Image *image,
  ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  size_t
    length;

  StreamHandler
    stream_handler;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  stream_handler=GetBlobStreamHandler(image);
  if (stream_handler == (StreamHandler) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "NoStreamHandlerIsDefined","`%s'",image->filename);
      return(MagickFalse);
    }
  length=stream_handler(image,cache_info->pixels,(size_t) cache_info->columns);
  return(length == cache_info->columns ? MagickTrue : MagickFalse);
}
