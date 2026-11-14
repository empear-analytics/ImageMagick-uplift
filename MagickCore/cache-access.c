/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                       CCCC   AAA    CCCC  H   H  EEEEE                      %
%                      C      A   A  C      H   H  E                          %
%                      C      AAAAA  C      HHHHH  EEE                        %
%                      C      A   A  C      H   H  E                          %
%                       CCCC  A   A   CCCC  H   H  EEEEE                      %
%                                                                             %
%                   AAA    CCCC   CCCC  EEEEE  SSSSS  SSSSS                   %
%                  A   A  C      C      E      SS     SS                      %
%                  AAAAA  C      C      EEE     SSS    SSS                    %
%                  A   A  C      C      E         SS     SS                   %
%                  A   A   CCCC   CCCC  EEEEE  SSSSS  SSSSS                   %
%                                                                             %
%                                                                             %
%                    MagickCore Pixel Cache Access Methods                    %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1999                                   %
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
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/distribute-cache-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/policy.h"
#include "MagickCore/quantum.h"
#include "MagickCore/random_.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#if defined(MAGICKCORE_HAVE_ERRNO_H)
#  include <errno.h>
#endif
#if defined(MAGICKCORE_HAVE_SYS_LOADAVG_H)
#  include <sys/loadavg.h>
#endif
#if defined(MAGICKCORE_ZLIB_DELEGATE)
#  include "zlib.h"
#endif

/*
  Forward declarations.
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static const Quantum
  *GetVirtualPixelCache(const Image *,const VirtualPixelMethod,const ssize_t,
    const ssize_t,const size_t,const size_t,ExceptionInfo *),
  *GetVirtualPixelsCache(const Image *);

static const void
  *GetVirtualMetacontentFromCache(const Image *);

static MagickBooleanType
  GetOneAuthenticPixelFromCache(Image *,const ssize_t,const ssize_t,Quantum *,
    ExceptionInfo *),
  GetOneVirtualPixelFromCache(const Image *,const VirtualPixelMethod,
    const ssize_t,const ssize_t,Quantum *,ExceptionInfo *),
  SyncAuthenticPixelsCache(Image *,ExceptionInfo *);

static Quantum
  *GetAuthenticPixelsCache(Image *,const ssize_t,const ssize_t,const size_t,
    const size_t,ExceptionInfo *),
  *QueueAuthenticPixelsCache(Image *,const ssize_t,const ssize_t,const size_t,
    const size_t,ExceptionInfo *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   A c q u i r e P i x e l C a c h e P i x e l s                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquirePixelCachePixels() returns the pixels associated with the specified
%  image.
%
%  The format of the AcquirePixelCachePixels() method is:
%
%      void *AcquirePixelCachePixels(const Image *image,size_t *length,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length: the pixel cache length.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport void *AcquirePixelCachePixels(const Image *image,size_t *length,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  (void) exception;
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  *length=0;
  if ((cache_info->type != MemoryCache) && (cache_info->type != MapCache))
    return((void *) NULL);
  *length=(size_t) cache_info->length;
  return(cache_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   C l o n e P i x e l C a c h e                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClonePixelCache() clones a pixel cache.
%
%  The format of the ClonePixelCache() method is:
%
%      Cache ClonePixelCache(const Cache cache)
%
%  A description of each parameter follows:
%
%    o cache: the pixel cache.
%
*/
MagickPrivate Cache ClonePixelCache(const Cache cache)
{
  CacheInfo
    *magick_restrict clone_info;

  const CacheInfo
    *magick_restrict cache_info;

  assert(cache != NULL);
  cache_info=(const CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  clone_info=(CacheInfo *) AcquirePixelCache(cache_info->number_threads);
  clone_info->virtual_pixel_method=cache_info->virtual_pixel_method;
  return((Cache ) clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   C l o n e P i x e l C a c h e M e t h o d s                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClonePixelCacheMethods() clones the pixel cache methods from one cache to
%  another.
%
%  The format of the ClonePixelCacheMethods() method is:
%
%      void ClonePixelCacheMethods(Cache clone,const Cache cache)
%
%  A description of each parameter follows:
%
%    o clone: Specifies a pointer to a Cache structure.
%
%    o cache: the pixel cache.
%
*/
MagickPrivate void ClonePixelCacheMethods(Cache clone,const Cache cache)
{
  CacheInfo
    *magick_restrict cache_info,
    *magick_restrict source_info;

  assert(clone != (Cache) NULL);
  source_info=(CacheInfo *) clone;
  assert(source_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      source_info->filename);
  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  source_info->methods=cache_info->methods;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   D e s t r o y I m a g e P i x e l C a c h e                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImagePixelCache() deallocates memory associated with the pixel cache.
%
%  The format of the DestroyImagePixelCache() method is:
%
%      void DestroyImagePixelCache(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static void DestroyImagePixelCache(Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->cache != (void *) NULL)
    image->cache=DestroyPixelCache(image->cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   D e s t r o y I m a g e P i x e l s                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImagePixels() deallocates memory associated with the pixel cache.
%
%  The format of the DestroyImagePixels() method is:
%
%      void DestroyImagePixels(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void DestroyImagePixels(Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->methods.destroy_pixel_handler != (DestroyPixelHandler) NULL)
    {
      cache_info->methods.destroy_pixel_handler(image);
      return;
    }
  image->cache=DestroyPixelCache(image->cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t A u t h e n t i c M e t a c o n t e n t                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticMetacontent() returns the authentic metacontent corresponding
%  with the last call to QueueAuthenticPixels() or GetVirtualPixels().  NULL is
%  returned if the associated pixels are not available.
%
%  The format of the GetAuthenticMetacontent() method is:
%
%      void *GetAuthenticMetacontent(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void *GetAuthenticMetacontent(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->methods.get_authentic_metacontent_from_handler !=
      (GetAuthenticMetacontentFromHandler) NULL)
    {
      void
        *metacontent;

      metacontent=cache_info->methods.
        get_authentic_metacontent_from_handler(image);
      return(metacontent);
    }
  assert(id < (int) cache_info->number_threads);
  return(cache_info->nexus_info[id]->metacontent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t A u t h e n t i c M e t a c o n t e n t F r o m C a c h e           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticMetacontentFromCache() returns the meta-content corresponding
%  with the last call to QueueAuthenticPixelsCache() or
%  GetAuthenticPixelsCache().
%
%  The format of the GetAuthenticMetacontentFromCache() method is:
%
%      void *GetAuthenticMetacontentFromCache(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static void *GetAuthenticMetacontentFromCache(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  return(cache_info->nexus_info[id]->metacontent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t A u t h e n t i c P i x e l s F r o m C a c h e                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticPixelsFromCache() returns the pixels associated with the last
%  call to the QueueAuthenticPixelsCache() or GetAuthenticPixelsCache() methods.
%
%  The format of the GetAuthenticPixelsFromCache() method is:
%
%      Quantum *GetAuthenticPixelsFromCache(const Image image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static Quantum *GetAuthenticPixelsFromCache(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  return(cache_info->nexus_info[id]->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t A u t h e n t i c P i x e l Q u e u e                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticPixelQueue() returns the authentic pixels associated
%  corresponding with the last call to QueueAuthenticPixels() or
%  GetAuthenticPixels().
%
%  The format of the GetAuthenticPixelQueue() method is:
%
%      Quantum *GetAuthenticPixelQueue(const Image image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static inline CacheInfo *GetImageCacheInfo(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  return(cache_info);
}

MagickExport Quantum *GetAuthenticPixelQueue(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  assert(image != (const Image *) NULL);
  cache_info=GetImageCacheInfo(image);
  if (cache_info->methods.get_authentic_pixels_from_handler !=
       (GetAuthenticPixelsFromHandler) NULL)
    return(cache_info->methods.get_authentic_pixels_from_handler(image));
  assert(id < (int) cache_info->number_threads);
  return(cache_info->nexus_info[id]->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t A u t h e n t i c P i x e l s                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticPixels() obtains a pixel region for read/write access. If the
%  region is successfully accessed, a pointer to a Quantum array
%  representing the region is returned, otherwise NULL is returned.
%
%  The returned pointer may point to a temporary working copy of the pixels
%  or it may point to the original pixels in memory. Performance is maximized
%  if the selected region is part of one row, or one or more full rows, since
%  then there is opportunity to access the pixels in-place (without a copy)
%  if the image is in memory, or in a memory-mapped file. The returned pointer
%  must *never* be deallocated by the user.
%
%  Pixels accessed via the returned pointer represent a simple array of type
%  Quantum.  If the image has corresponding metacontent,call
%  GetAuthenticMetacontent() after invoking GetAuthenticPixels() to obtain the
%  meta-content corresponding to the region.  Once the Quantum array has
%  been updated, the changes must be saved back to the underlying image using
%  SyncAuthenticPixels() or they may be lost.
%
%  The format of the GetAuthenticPixels() method is:
%
%      Quantum *GetAuthenticPixels(Image *image,const ssize_t x,
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
MagickExport Quantum *GetAuthenticPixels(Image *image,const ssize_t x,
  const ssize_t y,const size_t columns,const size_t rows,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  Quantum
    *pixels;

  assert(image != (Image *) NULL);
  cache_info=GetImageCacheInfo(image);
  if (cache_info->methods.get_authentic_pixels_handler !=
      (GetAuthenticPixelsHandler) NULL)
    {
      pixels=cache_info->methods.get_authentic_pixels_handler(image,x,y,columns,
        rows,exception);
      return(pixels);
    }
  assert(id < (int) cache_info->number_threads);
  pixels=GetAuthenticPixelCacheNexus(image,x,y,columns,rows,
    cache_info->nexus_info[id],exception);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t A u t h e n t i c P i x e l s C a c h e                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticPixelsCache() gets pixels from the in-memory or disk pixel cache
%  as defined by the geometry parameters.   A pointer to the pixels is returned
%  if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the GetAuthenticPixelsCache() method is:
%
%      Quantum *GetAuthenticPixelsCache(Image *image,const ssize_t x,
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
static Quantum *GetAuthenticPixelsCache(Image *image,const ssize_t x,
  const ssize_t y,const size_t columns,const size_t rows,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  Quantum
    *magick_restrict pixels;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  if (cache_info == (Cache) NULL)
    return((Quantum *) NULL);
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  pixels=GetAuthenticPixelCacheNexus(image,x,y,columns,rows,
    cache_info->nexus_info[id],exception);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t I m a g e E x t e n t                                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageExtent() returns the extent of the pixels associated corresponding
%  with the last call to QueueAuthenticPixels() or GetAuthenticPixels().
%
%  The format of the GetImageExtent() method is:
%
%      MagickSizeType GetImageExtent(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickSizeType GetImageExtent(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  return(GetPixelCacheNexusExtent(cache_info,cache_info->nexus_info[id]));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t I m a g e P i x e l C a c h e T y p e                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImagePixelCacheType() returns the pixel cache type: UndefinedCache,
%  DiskCache, MemoryCache, MapCache, or PingCache.
%
%  The format of the GetImagePixelCacheType() method is:
%
%      CacheType GetImagePixelCacheType(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport CacheType GetImagePixelCacheType(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  return(cache_info->type);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t O n e A u t h e n t i c P i x e l                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOneAuthenticPixel() returns a single pixel at the specified (x,y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the GetOneAuthenticPixel() method is:
%
%      MagickBooleanType GetOneAuthenticPixel(const Image image,const ssize_t x,
%        const ssize_t y,Quantum *pixel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o pixel: return a pixel at the specified (x,y) location.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType CopyPixel(const Image *image,
  const Quantum *source,Quantum *destination)
{
  ssize_t
    i;

  if (source == (const Quantum *) NULL)
    {
      destination[RedPixelChannel]=ClampToQuantum(image->background_color.red);
      destination[GreenPixelChannel]=ClampToQuantum(
        image->background_color.green);
      destination[BluePixelChannel]=ClampToQuantum(
        image->background_color.blue);
      destination[BlackPixelChannel]=ClampToQuantum(
        image->background_color.black);
      destination[AlphaPixelChannel]=ClampToQuantum(
        image->background_color.alpha);
      return(MagickFalse);
    }
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    destination[channel]=source[i];
  }
  return(MagickTrue);
}

MagickExport MagickBooleanType GetOneAuthenticPixel(Image *image,
  const ssize_t x,const ssize_t y,Quantum *pixel,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  Quantum
    *magick_restrict q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  (void) memset(pixel,0,MaxPixelChannels*sizeof(*pixel));
  if (cache_info->methods.get_one_authentic_pixel_from_handler != (GetOneAuthenticPixelFromHandler) NULL)
    return(cache_info->methods.get_one_authentic_pixel_from_handler(image,x,y,pixel,exception));
  q=GetAuthenticPixelsCache(image,x,y,1UL,1UL,exception);
  return(CopyPixel(image,q,pixel));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t O n e A u t h e n t i c P i x e l F r o m C a c h e                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOneAuthenticPixelFromCache() returns a single pixel at the specified (x,y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the GetOneAuthenticPixelFromCache() method is:
%
%      MagickBooleanType GetOneAuthenticPixelFromCache(const Image image,
%        const ssize_t x,const ssize_t y,Quantum *pixel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o pixel: return a pixel at the specified (x,y) location.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType GetOneAuthenticPixelFromCache(Image *image,
  const ssize_t x,const ssize_t y,Quantum *pixel,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  Quantum
    *magick_restrict q;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  (void) memset(pixel,0,MaxPixelChannels*sizeof(*pixel));
  q=GetAuthenticPixelCacheNexus(image,x,y,1UL,1UL,cache_info->nexus_info[id],
    exception);
  return(CopyPixel(image,q,pixel));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t O n e V i r t u a l P i x e l                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOneVirtualPixel() returns a single virtual pixel at the specified
%  (x,y) location.  The image background color is returned if an error occurs.
%  If you plan to modify the pixel, use GetOneAuthenticPixel() instead.
%
%  The format of the GetOneVirtualPixel() method is:
%
%      MagickBooleanType GetOneVirtualPixel(const Image image,const ssize_t x,
%        const ssize_t y,Quantum *pixel,ExceptionInfo exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o pixel: return a pixel at the specified (x,y) location.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetOneVirtualPixel(const Image *image,
  const ssize_t x,const ssize_t y,Quantum *pixel,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  const Quantum
    *p;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  (void) memset(pixel,0,MaxPixelChannels*sizeof(*pixel));
  if (cache_info->methods.get_one_virtual_pixel_from_handler !=
       (GetOneVirtualPixelFromHandler) NULL)
    return(cache_info->methods.get_one_virtual_pixel_from_handler(image,
      GetPixelCacheVirtualMethod(image),x,y,pixel,exception));
  assert(id < (int) cache_info->number_threads);
  p=GetVirtualPixelCacheNexus(image,GetPixelCacheVirtualMethod(image),x,y,
    1UL,1UL,cache_info->nexus_info[id],exception);
  return(CopyPixel(image,p,pixel));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t O n e V i r t u a l P i x e l F r o m C a c h e                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOneVirtualPixelFromCache() returns a single virtual pixel at the
%  specified (x,y) location.  The image background color is returned if an
%  error occurs.
%
%  The format of the GetOneVirtualPixelFromCache() method is:
%
%      MagickBooleanType GetOneVirtualPixelFromCache(const Image image,
%        const VirtualPixelMethod method,const ssize_t x,const ssize_t y,
%        Quantum *pixel,ExceptionInfo *exception)
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
static MagickBooleanType GetOneVirtualPixelFromCache(const Image *image,
  const VirtualPixelMethod virtual_pixel_method,const ssize_t x,const ssize_t y,
  Quantum *pixel,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  const Quantum
    *p;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  (void) memset(pixel,0,MaxPixelChannels*sizeof(*pixel));
  p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,x,y,1UL,1UL,
    cache_info->nexus_info[id],exception);
  return(CopyPixel(image,p,pixel));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t O n e V i r t u a l P i x e l I n f o                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOneVirtualPixelInfo() returns a single pixel at the specified (x,y)
%  location.  The image background color is returned if an error occurs.  If
%  you plan to modify the pixel, use GetOneAuthenticPixel() instead.
%
%  The format of the GetOneVirtualPixelInfo() method is:
%
%      MagickBooleanType GetOneVirtualPixelInfo(const Image image,
%        const VirtualPixelMethod virtual_pixel_method,const ssize_t x,
%        const ssize_t y,PixelInfo *pixel,ExceptionInfo exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o virtual_pixel_method: the virtual pixel method.
%
%    o x,y:  these values define the location of the pixel to return.
%
%    o pixel: return a pixel at the specified (x,y) location.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetOneVirtualPixelInfo(const Image *image,
  const VirtualPixelMethod virtual_pixel_method,const ssize_t x,const ssize_t y,
  PixelInfo *pixel,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  const Quantum
    *magick_restrict p;

  assert(image != (const Image *) NULL);
  cache_info=GetImageCacheInfo(image);
  assert(id < (int) cache_info->number_threads);
  GetPixelInfo(image,pixel);
  p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,x,y,1UL,1UL,
    cache_info->nexus_info[id],exception);
  if (p == (const Quantum *) NULL)
    return(MagickFalse);
  GetPixelInfoPixel(image,p,pixel);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t P i x e l C a c h e C o l o r s p a c e                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCacheColorspace() returns the colorspace of the pixel cache.
%
%  The format of the GetPixelCacheColorspace() method is:
%
%      Colorspace GetPixelCacheColorspace(const Cache cache)
%
%  A description of each parameter follows:
%
%    o cache: the pixel cache.
%
*/
MagickPrivate ColorspaceType GetPixelCacheColorspace(const Cache cache)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  return(cache_info->colorspace);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t P i x e l C a c h e F i l e n a m e                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCacheFilename() returns the filename associated with the pixel
%  cache.
%
%  The format of the GetPixelCacheFilename() method is:
%
%      const char *GetPixelCacheFilename(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport const char *GetPixelCacheFilename(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  return(cache_info->cache_filename);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t P i x e l C a c h e M e t h o d s                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCacheMethods() initializes the CacheMethods structure.
%
%  The format of the GetPixelCacheMethods() method is:
%
%      void GetPixelCacheMethods(CacheMethods *cache_methods)
%
%  A description of each parameter follows:
%
%    o cache_methods: Specifies a pointer to a CacheMethods structure.
%
*/
MagickPrivate void GetPixelCacheMethods(CacheMethods *cache_methods)
{
  assert(cache_methods != (CacheMethods *) NULL);
  (void) memset(cache_methods,0,sizeof(*cache_methods));
  cache_methods->get_virtual_pixel_handler=GetVirtualPixelCache;
  cache_methods->get_virtual_pixels_handler=GetVirtualPixelsCache;
  cache_methods->get_virtual_metacontent_from_handler=
    GetVirtualMetacontentFromCache;
  cache_methods->get_one_virtual_pixel_from_handler=GetOneVirtualPixelFromCache;
  cache_methods->get_authentic_pixels_handler=GetAuthenticPixelsCache;
  cache_methods->get_authentic_metacontent_from_handler=
    GetAuthenticMetacontentFromCache;
  cache_methods->get_authentic_pixels_from_handler=GetAuthenticPixelsFromCache;
  cache_methods->get_one_authentic_pixel_from_handler=
    GetOneAuthenticPixelFromCache;
  cache_methods->queue_authentic_pixels_handler=QueueAuthenticPixelsCache;
  cache_methods->sync_authentic_pixels_handler=SyncAuthenticPixelsCache;
  cache_methods->destroy_pixel_handler=DestroyImagePixelCache;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t P i x e l C a c h e N e x u s E x t e n t                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCacheNexusExtent() returns the extent of the pixels associated
%  corresponding with the last call to SetPixelCacheNexusPixels() or
%  GetPixelCacheNexusPixels().
%
%  The format of the GetPixelCacheNexusExtent() method is:
%
%      MagickSizeType GetPixelCacheNexusExtent(const Cache cache,
%        NexusInfo *nexus_info)
%
%  A description of each parameter follows:
%
%    o nexus_info: the nexus info.
%
*/
MagickPrivate MagickSizeType GetPixelCacheNexusExtent(const Cache cache,
  NexusInfo *magick_restrict nexus_info)
{
  CacheInfo
    *magick_restrict cache_info;

  MagickSizeType
    extent;

  assert(cache != NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  extent=(MagickSizeType) nexus_info->region.width*nexus_info->region.height;
  if (extent == 0)
    return((MagickSizeType) cache_info->columns*cache_info->rows);
  return(extent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t P i x e l C a c h e P i x e l s                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCachePixels() returns the pixels associated with the specified image.
%
%  The format of the GetPixelCachePixels() method is:
%
%      void *GetPixelCachePixels(Image *image,MagickSizeType *length,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length: the pixel cache length.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport void *GetPixelCachePixels(Image *image,MagickSizeType *length,
  ExceptionInfo *magick_unused(exception))
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  assert(length != (MagickSizeType *) NULL);
  magick_unreferenced(exception);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  *length=cache_info->length;
  if ((cache_info->type != MemoryCache) && (cache_info->type != MapCache))
    return((void *) NULL);
  return((void *) cache_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t P i x e l C a c h e S t o r a g e C l a s s                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCacheStorageClass() returns the class type of the pixel cache.
%
%  The format of the GetPixelCacheStorageClass() method is:
%
%      ClassType GetPixelCacheStorageClass(Cache cache)
%
%  A description of each parameter follows:
%
%    o type: GetPixelCacheStorageClass returns DirectClass or PseudoClass.
%
%    o cache: the pixel cache.
%
*/
MagickPrivate ClassType GetPixelCacheStorageClass(const Cache cache)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  return(cache_info->storage_class);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t P i x e l C a c h e T i l e S i z e                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCacheTileSize() returns the pixel cache tile size.
%
%  The format of the GetPixelCacheTileSize() method is:
%
%      void GetPixelCacheTileSize(const Image *image,size_t *width,
%        size_t *height)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o width: the optimized cache tile width in pixels.
%
%    o height: the optimized cache tile height in pixels.
%
*/
MagickPrivate void GetPixelCacheTileSize(const Image *image,size_t *width,
  size_t *height)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  *width=2048UL/(MagickMax(cache_info->number_channels,1)*sizeof(Quantum));
  if (GetImagePixelCacheType(image) == DiskCache)
    *width=8192UL/(MagickMax(cache_info->number_channels,1)*sizeof(Quantum));
  *height=(*width);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t P i x e l C a c h e V i r t u a l M e t h o d                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCacheVirtualMethod() gets the "virtual pixels" method for the
%  pixel cache.  A virtual pixel is any pixel access that is outside the
%  boundaries of the image cache.
%
%  The format of the GetPixelCacheVirtualMethod() method is:
%
%      VirtualPixelMethod GetPixelCacheVirtualMethod(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickPrivate VirtualPixelMethod GetPixelCacheVirtualMethod(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (Image *) NULL);
  cache_info=GetImageCacheInfo(image);
  return(cache_info->virtual_pixel_method);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t V i r t u a l M e t a c o n t e n t F r o m C a c h e               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualMetacontentFromCache() returns the meta-content corresponding with
%  the last call to QueueAuthenticPixelsCache() or GetVirtualPixelCache().
%
%  The format of the GetVirtualMetacontentFromCache() method is:
%
%      void *GetVirtualMetacontentFromCache(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static const void *GetVirtualMetacontentFromCache(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  const void
    *magick_restrict metacontent;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  metacontent=GetVirtualMetacontentFromNexus(cache_info,
    cache_info->nexus_info[id]);
  return(metacontent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t V i r t u a l M e t a c o n t e n t F r o m N e x u s               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualMetacontentFromNexus() returns the meta-content for the specified
%  cache nexus.
%
%  The format of the GetVirtualMetacontentFromNexus() method is:
%
%      const void *GetVirtualMetacontentFromNexus(const Cache cache,
%        NexusInfo *nexus_info)
%
%  A description of each parameter follows:
%
%    o cache: the pixel cache.
%
%    o nexus_info: the cache nexus to return the meta-content.
%
*/
MagickPrivate const void *GetVirtualMetacontentFromNexus(const Cache cache,
  NexusInfo *magick_restrict nexus_info)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->storage_class == UndefinedClass)
    return((void *) NULL);
  return(nexus_info->metacontent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t V i r t u a l M e t a c o n t e n t                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualMetacontent() returns the virtual metacontent corresponding with
%  the last call to QueueAuthenticPixels() or GetVirtualPixels().  NULL is
%  returned if the meta-content are not available.
%
%  The format of the GetVirtualMetacontent() method is:
%
%      const void *GetVirtualMetacontent(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport const void *GetVirtualMetacontent(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  const void
    *magick_restrict metacontent;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->methods.get_virtual_metacontent_from_handler != (GetVirtualMetacontentFromHandler) NULL)
    {
      metacontent=cache_info->methods.get_virtual_metacontent_from_handler(
        image);
      if (metacontent != (const void *) NULL)
        return(metacontent);
    }
  assert(id < (int) cache_info->number_threads);
  metacontent=GetVirtualMetacontentFromNexus(cache_info,
    cache_info->nexus_info[id]);
  return(metacontent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t V i r t u a l P i x e l C a c h e                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelCache() get virtual pixels from the in-memory or disk pixel
%  cache as defined by the geometry parameters.   A pointer to the pixels
%  is returned if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the GetVirtualPixelCache() method is:
%
%      const Quantum *GetVirtualPixelCache(const Image *image,
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
static const Quantum *GetVirtualPixelCache(const Image *image,
  const VirtualPixelMethod virtual_pixel_method,const ssize_t x,const ssize_t y,
  const size_t columns,const size_t rows,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  const Quantum
    *magick_restrict p;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,x,y,columns,rows,
    cache_info->nexus_info[id],exception);
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t V i r t u a l P i x e l Q u e u e                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelQueue() returns the virtual pixels associated corresponding
%  with the last call to QueueAuthenticPixels() or GetVirtualPixels().
%
%  The format of the GetVirtualPixelQueue() method is:
%
%      const Quantum *GetVirtualPixelQueue(const Image image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport const Quantum *GetVirtualPixelQueue(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  assert(image != (const Image *) NULL);
  cache_info=GetImageCacheInfo(image);
  if (cache_info->methods.get_virtual_pixels_handler != (GetVirtualPixelsHandler) NULL)
    return(cache_info->methods.get_virtual_pixels_handler(image));
  assert(id < (int) cache_info->number_threads);
  return(GetVirtualPixelsNexus(cache_info,cache_info->nexus_info[id]));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t V i r t u a l P i x e l s                                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixels() returns an immutable pixel region. If the
%  region is successfully accessed, a pointer to it is returned, otherwise
%  NULL is returned.  The returned pointer may point to a temporary working
%  copy of the pixels or it may point to the original pixels in memory.
%  Performance is maximized if the selected region is part of one row, or one
%  or more full rows, since there is opportunity to access the pixels in-place
%  (without a copy) if the image is in memory, or in a memory-mapped file.  The
%  returned pointer must *never* be deallocated by the user.
%
%  Pixels accessed via the returned pointer represent a simple array of type
%  Quantum.  If the image type is CMYK or the storage class is PseudoClass,
%  call GetAuthenticMetacontent() after invoking GetAuthenticPixels() to
%  access the meta-content (of type void) corresponding to the
%  region.
%
%  If you plan to modify the pixels, use GetAuthenticPixels() instead.
%
%  Note, the GetVirtualPixels() and GetAuthenticPixels() methods are not thread-
%  safe.  In a threaded environment, use GetCacheViewVirtualPixels() or
%  GetCacheViewAuthenticPixels() instead.
%
%  The format of the GetVirtualPixels() method is:
%
%      const Quantum *GetVirtualPixels(const Image *image,const ssize_t x,
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
MagickExport const Quantum *GetVirtualPixels(const Image *image,
  const ssize_t x,const ssize_t y,const size_t columns,const size_t rows,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  const Quantum
    *magick_restrict p;

  assert(image != (const Image *) NULL);
  cache_info=GetImageCacheInfo(image);
  if (cache_info->methods.get_virtual_pixel_handler !=
       (GetVirtualPixelHandler) NULL)
    return(cache_info->methods.get_virtual_pixel_handler(image,
      GetPixelCacheVirtualMethod(image),x,y,columns,rows,exception));
  assert(id < (int) cache_info->number_threads);
  p=GetVirtualPixelCacheNexus(image,GetPixelCacheVirtualMethod(image),x,y,
    columns,rows,cache_info->nexus_info[id],exception);
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t V i r t u a l P i x e l s F r o m C a c h e                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelsCache() returns the pixels associated corresponding with the
%  last call to QueueAuthenticPixelsCache() or GetVirtualPixelCache().
%
%  The format of the GetVirtualPixelsCache() method is:
%
%      Quantum *GetVirtualPixelsCache(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static const Quantum *GetVirtualPixelsCache(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  return(GetVirtualPixelsNexus(image->cache,cache_info->nexus_info[id]));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t V i r t u a l P i x e l s N e x u s                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelsNexus() returns the pixels associated with the specified
%  cache nexus.
%
%  The format of the GetVirtualPixelsNexus() method is:
%
%      const Quantum *GetVirtualPixelsNexus(const Cache cache,
%        NexusInfo *nexus_info)
%
%  A description of each parameter follows:
%
%    o cache: the pixel cache.
%
%    o nexus_info: the cache nexus to return the colormap pixels.
%
*/
MagickPrivate const Quantum *GetVirtualPixelsNexus(const Cache cache,
  NexusInfo *magick_restrict nexus_info)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->storage_class == UndefinedClass)
    return((Quantum *) NULL);
  return((const Quantum *) nexus_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   Q u e u e A u t h e n t i c P i x e l s C a c h e                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QueueAuthenticPixelsCache() allocates an region to store image pixels as
%  defined by the region rectangle and returns a pointer to the region.  This
%  region is subsequently transferred from the pixel cache with
%  SyncAuthenticPixelsCache().  A pointer to the pixels is returned if the
%  pixels are transferred, otherwise a NULL is returned.
%
%  The format of the QueueAuthenticPixelsCache() method is:
%
%      Quantum *QueueAuthenticPixelsCache(Image *image,const ssize_t x,
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
static Quantum *QueueAuthenticPixelsCache(Image *image,const ssize_t x,
  const ssize_t y,const size_t columns,const size_t rows,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  Quantum
    *magick_restrict pixels;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  pixels=QueueAuthenticPixelCacheNexus(image,x,y,columns,rows,MagickFalse,
    cache_info->nexus_info[id],exception);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   Q u e u e A u t h e n t i c P i x e l s                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QueueAuthenticPixels() queues a mutable pixel region.  If the region is
%  successfully initialized a pointer to a Quantum array representing the
%  region is returned, otherwise NULL is returned.  The returned pointer may
%  point to a temporary working buffer for the pixels or it may point to the
%  final location of the pixels in memory.
%
%  Write-only access means that any existing pixel values corresponding to
%  the region are ignored.  This is useful if the initial image is being
%  created from scratch, or if the existing pixel values are to be
%  completely replaced without need to refer to their preexisting values.
%  The application is free to read and write the pixel buffer returned by
%  QueueAuthenticPixels() any way it pleases. QueueAuthenticPixels() does not
%  initialize the pixel array values. Initializing pixel array values is the
%  application's responsibility.
%
%  Performance is maximized if the selected region is part of one row, or
%  one or more full rows, since then there is opportunity to access the
%  pixels in-place (without a copy) if the image is in memory, or in a
%  memory-mapped file. The returned pointer must *never* be deallocated
%  by the user.
%
%  Pixels accessed via the returned pointer represent a simple array of type
%  Quantum. If the image type is CMYK or the storage class is PseudoClass,
%  call GetAuthenticMetacontent() after invoking GetAuthenticPixels() to
%  obtain the meta-content (of type void) corresponding to the region.
%  Once the Quantum (and/or Quantum) array has been updated, the
%  changes must be saved back to the underlying image using
%  SyncAuthenticPixels() or they may be lost.
%
%  The format of the QueueAuthenticPixels() method is:
%
%      Quantum *QueueAuthenticPixels(Image *image,const ssize_t x,
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
MagickExport Quantum *QueueAuthenticPixels(Image *image,const ssize_t x,
  const ssize_t y,const size_t columns,const size_t rows,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  Quantum
    *magick_restrict pixels;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->methods.queue_authentic_pixels_handler !=
      (QueueAuthenticPixelsHandler) NULL)
    {
      pixels=cache_info->methods.queue_authentic_pixels_handler(image,x,y,
        columns,rows,exception);
      return(pixels);
    }
  assert(id < (int) cache_info->number_threads);
  pixels=QueueAuthenticPixelCacheNexus(image,x,y,columns,rows,MagickFalse,
    cache_info->nexus_info[id],exception);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e f e r e n c e P i x e l C a c h e                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReferencePixelCache() increments the reference count associated with the
%  pixel cache returning a pointer to the cache.
%
%  The format of the ReferencePixelCache method is:
%
%      Cache ReferencePixelCache(Cache cache_info)
%
%  A description of each parameter follows:
%
%    o cache_info: the pixel cache.
%
*/
MagickPrivate Cache ReferencePixelCache(Cache cache)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(cache != (Cache *) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  LockSemaphoreInfo(cache_info->semaphore);
  cache_info->reference_count++;
  UnlockSemaphoreInfo(cache_info->semaphore);
  return(cache_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e s e t P i x e l C a c h e C h a n n e l s                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetPixelCacheChannels() resets the pixel cache channels.
%
%  The format of the ResetPixelCacheChannels method is:
%
%      void ResetPixelCacheChannels(Image *)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickPrivate void ResetPixelCacheChannels(Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  cache_info->number_channels=GetPixelChannels(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   S y n c A u t h e n t i c P i x e l C a c h e                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncAuthenticPixelsCache() saves the authentic image pixels to the in-memory
%  or disk cache.  The method returns MagickTrue if the pixel region is synced,
%  otherwise MagickFalse.
%
%  The format of the SyncAuthenticPixelsCache() method is:
%
%      MagickBooleanType SyncAuthenticPixelsCache(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType SyncAuthenticPixelsCache(Image *image,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  assert(id < (int) cache_info->number_threads);
  status=SyncAuthenticPixelCacheNexus(image,cache_info->nexus_info[id],
    exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   S y n c A u t h e n t i c P i x e l s                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncAuthenticPixels() saves the image pixels to the in-memory or disk cache.
%  The method returns MagickTrue if the pixel region is flushed, otherwise
%  MagickFalse.
%
%  The format of the SyncAuthenticPixels() method is:
%
%      MagickBooleanType SyncAuthenticPixels(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SyncAuthenticPixels(Image *image,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  const int
    id = GetOpenMPThreadId();

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->methods.sync_authentic_pixels_handler != (SyncAuthenticPixelsHandler) NULL)
    {
      status=cache_info->methods.sync_authentic_pixels_handler(image,
        exception);
      return(status);
    }
  assert(id < (int) cache_info->number_threads);
  status=SyncAuthenticPixelCacheNexus(image,cache_info->nexus_info[id],
    exception);
  return(status);
}
