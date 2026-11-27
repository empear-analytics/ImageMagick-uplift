/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      CCCC   AAA    CCCC  H   H  EEEEE                       %
%                     C      A   A  C      H   H  E                           %
%                     C      AAAAA  C      HHHHH  EEE                         %
%                     C      A   A  C      H   H  E                           %
%                      CCCC  A   A   CCCC  H   H  EEEEE                       %
%                                                                             %
%                                                                             %
%                       MagickCore Pixel Cache Methods                        %
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
  Define declarations.
*/
#define CacheTick(offset,extent)  QuantumTick((MagickOffsetType) offset,extent)
#define IsFileDescriptorLimitExceeded() (GetMagickResource(FileResource) > \
  GetMagickResourceLimit(FileResource) ? MagickTrue : MagickFalse)

/*
  Typedef declarations.
*/
typedef struct _MagickModulo
{
  ssize_t
    quotient,
    remainder;
} MagickModulo;

/*
  Forward declarations.
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static Cache
  GetImagePixelCache(Image *,const MagickBooleanType,ExceptionInfo *)
    magick_hot_spot;

static MagickBooleanType
  OpenPixelCache(Image *,const MapMode,ExceptionInfo *),
  OpenPixelCacheOnDisk(CacheInfo *,const MapMode),
  ReadPixelCachePixels(CacheInfo *magick_restrict,NexusInfo *magick_restrict,
    ExceptionInfo *),
  ReadPixelCacheMetacontent(CacheInfo *magick_restrict,
    NexusInfo *magick_restrict,ExceptionInfo *),
  WritePixelCachePixels(CacheInfo *magick_restrict,NexusInfo *magick_restrict,
    ExceptionInfo *),
  WritePixelCacheMetacontent(CacheInfo *,NexusInfo *magick_restrict,
    ExceptionInfo *);

static Quantum
  *SetPixelCacheNexusPixels(const CacheInfo *magick_restrict,const MapMode,
    const ssize_t,const ssize_t,const size_t,const size_t,
    const MagickBooleanType,NexusInfo *magick_restrict,ExceptionInfo *)
    magick_hot_spot;

#if defined(MAGICKCORE_OPENCL_SUPPORT)
static void
  CopyOpenCLBuffer(CacheInfo *magick_restrict);
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/*
  Global declarations.
*/
static SemaphoreInfo
  *cache_semaphore = (SemaphoreInfo *) NULL;

static ssize_t
  cache_anonymous_memory = (-1);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   A c q u i r e P i x e l C a c h e                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquirePixelCache() acquires a pixel cache.
%
%  The format of the AcquirePixelCache() method is:
%
%      Cache AcquirePixelCache(const size_t number_threads)
%
%  A description of each parameter follows:
%
%    o number_threads: the number of nexus threads.
%
*/
MagickPrivate Cache AcquirePixelCache(const size_t number_threads)
{
  CacheInfo
    *magick_restrict cache_info;

  char
    *value;

  cache_info=(CacheInfo *) AcquireAlignedMemory(1,sizeof(*cache_info));
  if (cache_info == (CacheInfo *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) memset(cache_info,0,sizeof(*cache_info));
  cache_info->type=UndefinedCache;
  cache_info->mode=IOMode;
  cache_info->disk_mode=IOMode;
  cache_info->colorspace=sRGBColorspace;
  cache_info->file=(-1);
  cache_info->id=GetMagickThreadId();
  cache_info->number_threads=number_threads;
  if (GetOpenMPMaximumThreads() > cache_info->number_threads)
    cache_info->number_threads=GetOpenMPMaximumThreads();
  if (cache_info->number_threads == 0)
    cache_info->number_threads=1;
  cache_info->nexus_info=AcquirePixelCacheNexus(cache_info->number_threads);
  value=GetEnvironmentValue("MAGICK_SYNCHRONIZE");
  if (value != (const char *) NULL)
    {
      cache_info->synchronize=IsStringTrue(value);
      value=DestroyString(value);
    }
  value=GetPolicyValue("cache:synchronize");
  if (value != (const char *) NULL)
    {
      cache_info->synchronize=IsStringTrue(value);
      value=DestroyString(value);
    }
  cache_info->width_limit=MagickMin(GetMagickResourceLimit(WidthResource),
    (MagickSizeType) MAGICK_SSIZE_MAX);
  cache_info->height_limit=MagickMin(GetMagickResourceLimit(HeightResource),
    (MagickSizeType) MAGICK_SSIZE_MAX);
  cache_info->semaphore=AcquireSemaphoreInfo();
  cache_info->reference_count=1;
  cache_info->file_semaphore=AcquireSemaphoreInfo();
  cache_info->debug=(GetLogEventMask() & CacheEvent) != 0 ? MagickTrue :
    MagickFalse;
  cache_info->signature=MagickCoreSignature;
  return((Cache ) cache_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   A c q u i r e P i x e l C a c h e N e x u s                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquirePixelCacheNexus() allocates the NexusInfo structure.
%
%  The format of the AcquirePixelCacheNexus method is:
%
%      NexusInfo **AcquirePixelCacheNexus(const size_t number_threads)
%
%  A description of each parameter follows:
%
%    o number_threads: the number of nexus threads.
%
*/
MagickPrivate NexusInfo **AcquirePixelCacheNexus(const size_t number_threads)
{
  NexusInfo
    **magick_restrict nexus_info;

  ssize_t
    i;

  nexus_info=(NexusInfo **) MagickAssumeAligned(AcquireAlignedMemory(2*
    number_threads,sizeof(*nexus_info)));
  if (nexus_info == (NexusInfo **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  *nexus_info=(NexusInfo *) AcquireQuantumMemory(number_threads,
    2*sizeof(**nexus_info));
  if (*nexus_info == (NexusInfo *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) memset(*nexus_info,0,2*number_threads*sizeof(**nexus_info));
  for (i=0; i < (ssize_t) (2*number_threads); i++)
  {
    nexus_info[i]=(*nexus_info+i);
    if (i < (ssize_t) number_threads)
      nexus_info[i]->virtual_nexus=(*nexus_info+number_threads+i);
    nexus_info[i]->signature=MagickCoreSignature;
  }
  return(nexus_info);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   C a c h e C o m p o n e n t G e n e s i s                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CacheComponentGenesis() instantiates the cache component.
%
%  The format of the CacheComponentGenesis method is:
%
%      MagickBooleanType CacheComponentGenesis(void)
%
*/
MagickPrivate MagickBooleanType CacheComponentGenesis(void)
{
  if (cache_semaphore == (SemaphoreInfo *) NULL)
    cache_semaphore=AcquireSemaphoreInfo();
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   C a c h e C o m p o n e n t T e r m i n u s                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CacheComponentTerminus() destroys the cache component.
%
%  The format of the CacheComponentTerminus() method is:
%
%      CacheComponentTerminus(void)
%
*/
MagickPrivate void CacheComponentTerminus(void)
{
  if (cache_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&cache_semaphore);
  /* no op-- nothing to destroy */
  RelinquishSemaphoreInfo(&cache_semaphore);
}

#include "MagickCore/cache-clone-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   D e s t r o y P i x e l C a c h e                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyPixelCache() deallocates memory associated with the pixel cache.
%
%  The format of the DestroyPixelCache() method is:
%
%      Cache DestroyPixelCache(Cache cache)
%
%  A description of each parameter follows:
%
%    o cache: the pixel cache.
%
*/
MagickPrivate Cache DestroyPixelCache(Cache cache)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  LockSemaphoreInfo(cache_info->semaphore);
  cache_info->reference_count--;
  if (cache_info->reference_count != 0)
    {
      UnlockSemaphoreInfo(cache_info->semaphore);
      return((Cache) NULL);
    }
  UnlockSemaphoreInfo(cache_info->semaphore);
  if (cache_info->debug != MagickFalse)
    {
      char
        message[MagickPathExtent];

      (void) FormatLocaleString(message,MagickPathExtent,"destroy %s",
        cache_info->filename);
      (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",message);
    }
  RelinquishPixelCachePixels(cache_info);
  RelinquishPixelCacheResources(cache_info);
  cache_info->signature=(~MagickCoreSignature);
  cache_info=(CacheInfo *) RelinquishAlignedMemory(cache_info);
  cache=(Cache) NULL;
  return(cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   D e s t r o y P i x e l C a c h e N e x u s                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyPixelCacheNexus() destroys a pixel cache nexus.
%
%  The format of the DestroyPixelCacheNexus() method is:
%
%      NexusInfo **DestroyPixelCacheNexus(NexusInfo *nexus_info,
%        const size_t number_threads)
%
%  A description of each parameter follows:
%
%    o nexus_info: the nexus to destroy.
%
%    o number_threads: the number of nexus threads.
%
*/

static inline void RelinquishCacheNexusPixels(NexusInfo *nexus_info)
{
  if (nexus_info->mapped == MagickFalse)
    (void) RelinquishAlignedMemory(nexus_info->cache);
  else
    (void) UnmapBlob(nexus_info->cache,(size_t) nexus_info->length);
  nexus_info->cache=(Quantum *) NULL;
  nexus_info->pixels=(Quantum *) NULL;
  nexus_info->metacontent=(void *) NULL;
  nexus_info->length=0;
  nexus_info->mapped=MagickFalse;
}

MagickPrivate NexusInfo **DestroyPixelCacheNexus(NexusInfo **nexus_info,
  const size_t number_threads)
{
  ssize_t
    i;

  assert(nexus_info != (NexusInfo **) NULL);
  for (i=0; i < (ssize_t) (2*number_threads); i++)
  {
    if (nexus_info[i]->cache != (Quantum *) NULL)
      RelinquishCacheNexusPixels(nexus_info[i]);
    nexus_info[i]->signature=(~MagickCoreSignature);
  }
  *nexus_info=(NexusInfo *) RelinquishMagickMemory(*nexus_info);
  nexus_info=(NexusInfo **) RelinquishAlignedMemory(nexus_info);
  return(nexus_info);
}


#if defined(MAGICKCORE_OPENCL_SUPPORT)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t A u t h e n t i c O p e n C L B u f f e r                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticOpenCLBuffer() returns an OpenCL buffer used to execute OpenCL
%  operations.
%
%  The format of the GetAuthenticOpenCLBuffer() method is:
%
%      cl_mem GetAuthenticOpenCLBuffer(const Image *image,
%        MagickCLDevice device,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o device: the device to use.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickPrivate cl_mem GetAuthenticOpenCLBuffer(const Image *image,
  MagickCLDevice device,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (const Image *) NULL);
  assert(device != (const MagickCLDevice) NULL);
  cache_info=(CacheInfo *) image->cache;
  if ((cache_info->type == UndefinedCache) || (cache_info->reference_count > 1))
    {
      (void) SyncImagePixelCache((Image *) image,exception);
      cache_info=(CacheInfo *) image->cache;
    }
  if ((cache_info->type != MemoryCache) || (cache_info->mapped != MagickFalse))
    return((cl_mem) NULL);
  LockSemaphoreInfo(cache_info->semaphore);
  if ((cache_info->opencl != (MagickCLCacheInfo) NULL) &&
      (cache_info->opencl->device->context != device->context))
    cache_info->opencl=CopyMagickCLCacheInfo(cache_info->opencl);
  if (cache_info->opencl == (MagickCLCacheInfo) NULL)
    {
      assert(cache_info->pixels != (Quantum *) NULL);
      cache_info->opencl=AcquireMagickCLCacheInfo(device,cache_info->pixels,
        cache_info->length);
    }
  if (cache_info->opencl != (MagickCLCacheInfo) NULL)
    RetainOpenCLMemObject(cache_info->opencl->buffer);
  UnlockSemaphoreInfo(cache_info->semaphore);
  if (cache_info->opencl == (MagickCLCacheInfo) NULL)
    return((cl_mem) NULL);
  assert(cache_info->opencl->pixels == cache_info->pixels);
  return(cache_info->opencl->buffer);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t A u t h e n t i c P i x e l C a c h e N e x u s                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetAuthenticPixelCacheNexus() gets authentic pixels from the in-memory or
%  disk pixel cache as defined by the geometry parameters.   A pointer to the
%  pixels is returned if the pixels are transferred, otherwise a NULL is
%  returned.
%
%  The format of the GetAuthenticPixelCacheNexus() method is:
%
%      Quantum *GetAuthenticPixelCacheNexus(Image *image,const ssize_t x,
%        const ssize_t y,const size_t columns,const size_t rows,
%        NexusInfo *nexus_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o nexus_info: the cache nexus to return.
%
%    o exception: return any errors or warnings in this structure.
%
*/

MagickPrivate Quantum *GetAuthenticPixelCacheNexus(Image *image,const ssize_t x,
  const ssize_t y,const size_t columns,const size_t rows,NexusInfo *nexus_info,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  Quantum
    *magick_restrict pixels;

  /*
    Transfer pixels from the cache.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  pixels=QueueAuthenticPixelCacheNexus(image,x,y,columns,rows,MagickTrue,
    nexus_info,exception);
  if (pixels == (Quantum *) NULL)
    return((Quantum *) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (nexus_info->authentic_pixel_cache != MagickFalse)
    return(pixels);
  if (ReadPixelCachePixels(cache_info,nexus_info,exception) == MagickFalse)
    return((Quantum *) NULL);
  if (cache_info->metacontent_extent != 0)
    if (ReadPixelCacheMetacontent(cache_info,nexus_info,exception) == MagickFalse)
      return((Quantum *) NULL);
  return(pixels);
}


#include "MagickCore/cache-image-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   G e t V i r t u a l P i x e l C a c h e N e x u s                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetVirtualPixelCacheNexus() gets virtual pixels from the in-memory or disk
%  pixel cache as defined by the geometry parameters.   A pointer to the pixels
%  is returned if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the GetVirtualPixelCacheNexus() method is:
%
%      Quantum *GetVirtualPixelCacheNexus(const Image *image,
%        const VirtualPixelMethod method,const ssize_t x,const ssize_t y,
%        const size_t columns,const size_t rows,NexusInfo *nexus_info,
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
%    o nexus_info: the cache nexus to acquire.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static ssize_t
  DitherMatrix[64] =
  {
     0,  48,  12,  60,   3,  51,  15,  63,
    32,  16,  44,  28,  35,  19,  47,  31,
     8,  56,   4,  52,  11,  59,   7,  55,
    40,  24,  36,  20,  43,  27,  39,  23,
     2,  50,  14,  62,   1,  49,  13,  61,
    34,  18,  46,  30,  33,  17,  45,  29,
    10,  58,   6,  54,   9,  57,   5,  53,
    42,  26,  38,  22,  41,  25,  37,  21
  };

#include "MagickCore/cache-virtual-private.h"

static void SetVirtualPixelColorTail(const Image *image,
  const CacheInfo *magick_restrict cache_info,
  const VirtualPixelMethod virtual_pixel_method,Quantum *virtual_pixel)
{
  ssize_t
    i;

  switch (virtual_pixel_method)
  {
    case MaskVirtualPixelMethod:
    case WhiteVirtualPixelMethod:
    {
      for (i=0; i < (ssize_t) cache_info->number_channels; i++)
        SetPixelChannel(image,(PixelChannel) i,QuantumRange,virtual_pixel);
      SetPixelAlpha(image,OpaqueAlpha,virtual_pixel);
      break;
    }
    default:
    {
      SetPixelRed(image,ClampToQuantum(image->background_color.red),
        virtual_pixel);
      SetPixelGreen(image,ClampToQuantum(image->background_color.green),
        virtual_pixel);
      SetPixelBlue(image,ClampToQuantum(image->background_color.blue),
        virtual_pixel);
      SetPixelBlack(image,ClampToQuantum(image->background_color.black),
        virtual_pixel);
      SetPixelAlpha(image,ClampToQuantum(image->background_color.alpha),
        virtual_pixel);
      break;
    }
  }
}

static void SetVirtualPixelColor(const Image *image,
  const CacheInfo *magick_restrict cache_info,
  const VirtualPixelMethod virtual_pixel_method,Quantum *virtual_pixel)
{
  ssize_t
    i;

  switch (virtual_pixel_method)
  {
    case BlackVirtualPixelMethod:
    {
      for (i=0; i < (ssize_t) cache_info->number_channels; i++)
        SetPixelChannel(image,(PixelChannel) i,(Quantum) 0,virtual_pixel);
      SetPixelAlpha(image,OpaqueAlpha,virtual_pixel);
      break;
    }
    case GrayVirtualPixelMethod:
    {
      for (i=0; i < (ssize_t) cache_info->number_channels; i++)
        SetPixelChannel(image,(PixelChannel) i,QuantumRange/2,
          virtual_pixel);
      SetPixelAlpha(image,OpaqueAlpha,virtual_pixel);
      break;
    }
    case TransparentVirtualPixelMethod:
    {
      for (i=0; i < (ssize_t) cache_info->number_channels; i++)
        SetPixelChannel(image,(PixelChannel) i,(Quantum) 0,virtual_pixel);
      SetPixelAlpha(image,TransparentAlpha,virtual_pixel);
      break;
    }
    default:
    {
      SetVirtualPixelColorTail(image,cache_info,virtual_pixel_method,
        virtual_pixel);
      break;
    }
  }
}

static MagickBooleanType AcquireVirtualMetacontent(const Image *image,
  const CacheInfo *magick_restrict cache_info,
  void *magick_restrict *virtual_metacontent,
  ExceptionInfo *exception)
{
  if (cache_info->metacontent_extent != 0)
    {
      /*
        Acquire a metacontent buffer.
      */
      (*virtual_metacontent)=(void *) AcquireQuantumMemory(1,
        cache_info->metacontent_extent);
      if ((*virtual_metacontent) == (void *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            CacheError,"UnableToGetCacheNexus","`%s'",image->filename);
          return(MagickFalse);
        }
      (void) memset((*virtual_metacontent),0,cache_info->metacontent_extent);
    }
  return(MagickTrue);
}

MagickPrivate const Quantum *GetVirtualPixelCacheNexus(const Image *image,
  const VirtualPixelMethod virtual_pixel_method,const ssize_t x,const ssize_t y,
  const size_t columns,const size_t rows,NexusInfo *nexus_info,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  MagickOffsetType
    offset;

  MagickSizeType
    length,
    number_pixels;

  NexusInfo
    *magick_restrict virtual_nexus;

  Quantum
    *magick_restrict pixels,
    *magick_restrict q,
    virtual_pixel[MaxPixelChannels];

  ssize_t
    v;

  unsigned char
    *magick_restrict s;

  void
    *magick_restrict virtual_metacontent;

  /*
    Acquire pixels.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->type == UndefinedCache)
    return((const Quantum *) NULL);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  CopyOpenCLBuffer(cache_info);
#endif
  pixels=SetPixelCacheNexusPixels(cache_info,ReadMode,x,y,columns,rows,
    ((image->channels & WriteMaskChannel) != 0) ||
    ((image->channels & CompositeMaskChannel) != 0) ? MagickTrue : MagickFalse,
    nexus_info,exception);
  if (pixels == (Quantum *) NULL)
    return((const Quantum *) NULL);
  if (IsValidPixelOffset(nexus_info->region.y,cache_info->columns) == MagickFalse)
    return((const Quantum *) NULL);
  offset=nexus_info->region.y*(MagickOffsetType) cache_info->columns;
  if (IsOffsetOverflow(offset,(MagickOffsetType) nexus_info->region.x) == MagickFalse)
    return((const Quantum *) NULL);
  offset+=nexus_info->region.x;
  length=(MagickSizeType) (nexus_info->region.height-1L)*cache_info->columns+
    nexus_info->region.width-1L;
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  if ((offset >= 0) && (((MagickSizeType) offset+length) < number_pixels))
    if (IsRegionInsideCache(cache_info,x,y,columns,rows) != MagickFalse)
      return(ReadCacheRegionPixels(cache_info,nexus_info,pixels,exception));
  /*
    Pixel request is outside cache extents.
  */
  virtual_nexus=nexus_info->virtual_nexus;
  q=pixels;
  s=(unsigned char *) nexus_info->metacontent;
  (void) memset(virtual_pixel,0,cache_info->number_channels*
    sizeof(*virtual_pixel));
  virtual_metacontent=(void *) NULL;
  switch (virtual_pixel_method)
  {
    case BackgroundVirtualPixelMethod:
    case BlackVirtualPixelMethod:
    case GrayVirtualPixelMethod:
    case TransparentVirtualPixelMethod:
    case MaskVirtualPixelMethod:
    case WhiteVirtualPixelMethod:
    case EdgeVirtualPixelMethod:
    case CheckerTileVirtualPixelMethod:
    case HorizontalTileVirtualPixelMethod:
    case VerticalTileVirtualPixelMethod:
    {
      if (AcquireVirtualMetacontent(image,cache_info,&virtual_metacontent,
            exception) == MagickFalse)
        return((const Quantum *) NULL);
      SetVirtualPixelColor(image,cache_info,virtual_pixel_method,
        virtual_pixel);
      break;
    }
    default:
      break;
  }
  {
    const VirtualPixelRows
      virtual_rows = { image, virtual_pixel_method, x, y, columns, rows,
        cache_info, virtual_nexus, virtual_pixel, virtual_metacontent,
        exception };

    v=GetVirtualPixelRows(&virtual_rows,q,s);
  }
  /*
    Free resources.
  */
  if (virtual_metacontent != (void *) NULL)
    virtual_metacontent=(void *) RelinquishMagickMemory(virtual_metacontent);
  if (v < (ssize_t) rows)
    return((const Quantum *) NULL);
  return(pixels);
}


#include "MagickCore/cache-open-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   P e r s i s t P i x e l C a c h e                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PersistPixelCache() attaches to or initializes a persistent pixel cache.  A
%  persistent pixel cache is one that resides on disk and is not destroyed
%  when the program exits.
%
%  The format of the PersistPixelCache() method is:
%
%      MagickBooleanType PersistPixelCache(Image *image,const char *filename,
%        const MagickBooleanType attach,MagickOffsetType *offset,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o filename: the persistent pixel cache filename.
%
%    o attach: A value other than zero initializes the persistent pixel cache.
%
%    o initialize: A value other than zero initializes the persistent pixel
%      cache.
%
%    o offset: the offset in the persistent cache to store pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType PersistPixelCache(Image *image,
  const char *filename,const MagickBooleanType attach,MagickOffsetType *offset,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info,
    *magick_restrict clone_info;

  MagickBooleanType
    status;

  ssize_t
    page_size;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (void *) NULL);
  assert(filename != (const char *) NULL);
  assert(offset != (MagickOffsetType *) NULL);
  page_size=GetMagickPageSize();
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  CopyOpenCLBuffer(cache_info);
#endif
  if (attach != MagickFalse)
    {
      /*
        Attach existing persistent pixel cache.
      */
      if (cache_info->debug != MagickFalse)
        (void) LogMagickEvent(CacheEvent,GetMagickModule(),
          "attach persistent cache");
      (void) CopyMagickString(cache_info->cache_filename,filename,
        MagickPathExtent);
      cache_info->type=MapCache;
      cache_info->offset=(*offset);
      if (OpenPixelCache(image,ReadMode,exception) == MagickFalse)
        return(MagickFalse);
      *offset=(*offset+(MagickOffsetType) cache_info->length+page_size-
        ((MagickOffsetType) cache_info->length % page_size));
      return(MagickTrue);
    }
  /*
    Clone persistent pixel cache.
  */
  status=AcquireMagickResource(DiskResource,cache_info->length);
  if (status == MagickFalse)
    {
      cache_info->type=UndefinedCache;
      (void) memset(image->channel_map,0,MaxPixelChannels*
        sizeof(*image->channel_map));
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "CacheResourcesExhausted","`%s'",image->filename);
      return(MagickFalse);
    }
  clone_info=AcquirePersistentPixelCache(cache_info,filename,offset);
  status=OpenPixelCacheOnDisk(clone_info,WriteMode);
  if (status != MagickFalse)
    status=ClonePixelCacheRepository(clone_info,cache_info,exception);
  *offset=(*offset+(MagickOffsetType) cache_info->length+page_size-
    ((MagickOffsetType) cache_info->length % page_size));
  clone_info=(CacheInfo *) DestroyPixelCache(clone_info);
  return(status);
}

static inline MagickBooleanType HasNexusMaskChannels(const Image *image)
{
  return(((image->channels & WriteMaskChannel) != 0) ||
    ((image->channels & CompositeMaskChannel) != 0) ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   Q u e u e A u t h e n t i c P i x e l C a c h e N e x u s                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QueueAuthenticPixelCacheNexus() allocates an region to store image pixels as
%  defined by the region rectangle and returns a pointer to the region.  This
%  region is subsequently transferred from the pixel cache with
%  SyncAuthenticPixelsCache().  A pointer to the pixels is returned if the
%  pixels are transferred, otherwise a NULL is returned.
%
%  The format of the QueueAuthenticPixelCacheNexus() method is:
%
%      Quantum *QueueAuthenticPixelCacheNexus(Image *image,const ssize_t x,
%        const ssize_t y,const size_t columns,const size_t rows,
%        const MagickBooleanType clone,NexusInfo *nexus_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o nexus_info: the cache nexus to set.
%
%    o clone: clone the pixel cache.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsOutsideCachePixel(const CacheInfo *cache_info,
  const ssize_t x,const ssize_t y)
{
  return(((cache_info->columns == 0) || (cache_info->rows == 0) || (x < 0) ||
    (y < 0) || (x >= (ssize_t) cache_info->columns) ||
    (y >= (ssize_t) cache_info->rows)) ? MagickTrue : MagickFalse);
}

MagickPrivate Quantum *QueueAuthenticPixelCacheNexus(Image *image,
  const ssize_t x,const ssize_t y,const size_t columns,const size_t rows,
  const MagickBooleanType clone,NexusInfo *nexus_info,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  MagickOffsetType
    offset;

  MagickSizeType
    number_pixels;

  Quantum
    *magick_restrict pixels;

  /*
    Validate pixel cache geometry.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) GetImagePixelCache(image,clone,exception);
  if (cache_info == (Cache) NULL)
    return((Quantum *) NULL);
  assert(cache_info->signature == MagickCoreSignature);
  if (IsOutsideCachePixel(cache_info,x,y) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "PixelsAreNotAuthentic","`%s'",image->filename);
      return((Quantum *) NULL);
    }
  if (IsValidPixelOffset(y,cache_info->columns) == MagickFalse)
    return((Quantum *) NULL);
  offset=y*(MagickOffsetType) cache_info->columns;
  if (IsOffsetOverflow(offset,(MagickOffsetType) x) == MagickFalse)
    return((Quantum *) NULL);
  offset+=x;
  if (offset < 0)
    return((Quantum *) NULL);
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  offset+=((MagickOffsetType) rows-1)*(MagickOffsetType) cache_info->columns;
  if (IsOffsetOverflow(offset,(MagickOffsetType) columns-1) == MagickFalse)
    return((Quantum *) NULL);
  offset+=(MagickOffsetType) columns-1;
  if ((MagickSizeType) offset >= number_pixels)
    return((Quantum *) NULL);
  /*
    Return pixel cache.
  */
  pixels=SetPixelCacheNexusPixels(cache_info,WriteMode,x,y,columns,rows,
    HasNexusMaskChannels(image),nexus_info,exception);
  return(pixels);
}


#include "MagickCore/cache-read-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e s e t C a c h e A n o n y m o u s M e m o r y                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetCacheAnonymousMemory() resets the anonymous_memory value.
%
%  The format of the ResetCacheAnonymousMemory method is:
%
%      void ResetCacheAnonymousMemory(void)
%
*/
MagickPrivate void ResetCacheAnonymousMemory(void)
{
  cache_anonymous_memory=0;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   R e s h a p e P i x e l C a c h e                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReshapePixelCache() reshapes an existing pixel cache.
%
%  The format of the ReshapePixelCache() method is:
%
%      MagickBooleanType ReshapePixelCache(Image *image,const size_t columns,
%        const size_t rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the reshaped pixel cache.
%
%    o rows: number of rows in the reshaped pixel cache.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ReshapePixelCache(Image *image,
  const size_t columns,const size_t rows,ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  MagickSizeType
    extent;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (void *) NULL);
  extent=(MagickSizeType) columns*rows;
  if (extent > ((MagickSizeType) image->columns*image->rows))
    ThrowBinaryException(ImageError,"WidthOrHeightExceedsLimit",
      image->filename);
  image->columns=columns;
  image->rows=rows;
  cache_info=(CacheInfo *) image->cache;
  cache_info->columns=columns;
  cache_info->rows=rows;
  return(SyncImagePixelCache(image,exception));
}

static void SetPixelCacheGetMethods(CacheInfo *magick_restrict cache_info,
  const CacheMethods *cache_methods)
{
  if (cache_methods->get_virtual_pixel_handler != (GetVirtualPixelHandler) NULL)
    cache_info->methods.get_virtual_pixel_handler=
      cache_methods->get_virtual_pixel_handler;
  if (cache_methods->destroy_pixel_handler != (DestroyPixelHandler) NULL)
    cache_info->methods.destroy_pixel_handler=
      cache_methods->destroy_pixel_handler;
  if (cache_methods->get_virtual_metacontent_from_handler !=
      (GetVirtualMetacontentFromHandler) NULL)
    cache_info->methods.get_virtual_metacontent_from_handler=
      cache_methods->get_virtual_metacontent_from_handler;
  if (cache_methods->get_authentic_pixels_handler !=
      (GetAuthenticPixelsHandler) NULL)
    cache_info->methods.get_authentic_pixels_handler=
      cache_methods->get_authentic_pixels_handler;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   S e t P i x e l C a c h e M e t h o d s                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelCacheMethods() sets the image pixel methods to the specified ones.
%
%  The format of the SetPixelCacheMethods() method is:
%
%      SetPixelCacheMethods(Cache *,CacheMethods *cache_methods)
%
%  A description of each parameter follows:
%
%    o cache: the pixel cache.
%
%    o cache_methods: Specifies a pointer to a CacheMethods structure.
%
*/
MagickPrivate void SetPixelCacheMethods(Cache cache,CacheMethods *cache_methods)
{
  CacheInfo
    *magick_restrict cache_info;

  GetOneAuthenticPixelFromHandler
    get_one_authentic_pixel_from_handler;

  GetOneVirtualPixelFromHandler
    get_one_virtual_pixel_from_handler;

  /*
    Set cache pixel methods.
  */
  assert(cache != (Cache) NULL);
  assert(cache_methods != (CacheMethods *) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  SetPixelCacheGetMethods(cache_info,cache_methods);
  if (cache_methods->queue_authentic_pixels_handler !=
      (QueueAuthenticPixelsHandler) NULL)
    cache_info->methods.queue_authentic_pixels_handler=
      cache_methods->queue_authentic_pixels_handler;
  if (cache_methods->sync_authentic_pixels_handler !=
      (SyncAuthenticPixelsHandler) NULL)
    cache_info->methods.sync_authentic_pixels_handler=
      cache_methods->sync_authentic_pixels_handler;
  if (cache_methods->get_authentic_pixels_from_handler !=
      (GetAuthenticPixelsFromHandler) NULL)
    cache_info->methods.get_authentic_pixels_from_handler=
      cache_methods->get_authentic_pixels_from_handler;
  if (cache_methods->get_authentic_metacontent_from_handler !=
      (GetAuthenticMetacontentFromHandler) NULL)
    cache_info->methods.get_authentic_metacontent_from_handler=
      cache_methods->get_authentic_metacontent_from_handler;
  get_one_virtual_pixel_from_handler=
    cache_info->methods.get_one_virtual_pixel_from_handler;
  if (get_one_virtual_pixel_from_handler !=
      (GetOneVirtualPixelFromHandler) NULL)
    cache_info->methods.get_one_virtual_pixel_from_handler=
      cache_methods->get_one_virtual_pixel_from_handler;
  get_one_authentic_pixel_from_handler=
    cache_methods->get_one_authentic_pixel_from_handler;
  if (get_one_authentic_pixel_from_handler !=
      (GetOneAuthenticPixelFromHandler) NULL)
    cache_info->methods.get_one_authentic_pixel_from_handler=
      cache_methods->get_one_authentic_pixel_from_handler;
}

#include "MagickCore/cache-nexus-private.h"

static void SetBackgroundVirtualPixels(Image *image,ExceptionInfo *exception)
{
  if ((image->background_color.alpha_trait != UndefinedPixelTrait) &&
      ((image->alpha_trait & BlendPixelTrait) == 0))
    (void) SetCacheAlphaChannel(image,OpaqueAlpha,exception);
  if ((IsPixelInfoGray(&image->background_color) == MagickFalse) &&
      (IsGrayColorspace(image->colorspace) != MagickFalse))
    (void) SetImageColorspace(image,sRGBColorspace,exception);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   S e t P i x e l C a c h e V i r t u a l M e t h o d                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelCacheVirtualMethod() sets the "virtual pixels" method for the
%  pixel cache and returns the previous setting.  A virtual pixel is any pixel
%  access that is outside the boundaries of the image cache.
%
%  The format of the SetPixelCacheVirtualMethod() method is:
%
%      VirtualPixelMethod SetPixelCacheVirtualMethod(Image *image,
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
MagickPrivate VirtualPixelMethod SetPixelCacheVirtualMethod(Image *image,
  const VirtualPixelMethod virtual_pixel_method,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  VirtualPixelMethod
    method;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  method=cache_info->virtual_pixel_method;
  cache_info->virtual_pixel_method=virtual_pixel_method;
  if ((image->columns != 0) && (image->rows != 0))
    switch (virtual_pixel_method)
    {
      case BackgroundVirtualPixelMethod:
      {
        SetBackgroundVirtualPixels(image,exception);
        break;
      }
      case TransparentVirtualPixelMethod:
      {
        if ((image->alpha_trait & BlendPixelTrait) == 0)
          (void) SetCacheAlphaChannel(image,OpaqueAlpha,exception);
        break;
      }
      default:
        break;
    }
  return(method);
}

#if defined(MAGICKCORE_OPENCL_SUPPORT)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   S y n c A u t h e n t i c O p e n C L B u f f e r                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncAuthenticOpenCLBuffer() makes sure that all the OpenCL operations have
%  been completed and updates the host memory.
%
%  The format of the SyncAuthenticOpenCLBuffer() method is:
%
%      void SyncAuthenticOpenCLBuffer(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/

static void CopyOpenCLBuffer(CacheInfo *magick_restrict cache_info)
{
  assert(cache_info != (CacheInfo *) NULL);
  assert(cache_info->signature == MagickCoreSignature);
  if ((cache_info->type != MemoryCache) ||
      (cache_info->opencl == (MagickCLCacheInfo) NULL))
    return;
  /*
    Ensure single threaded access to OpenCL environment.
  */
  LockSemaphoreInfo(cache_info->semaphore);
  cache_info->opencl=CopyMagickCLCacheInfo(cache_info->opencl);
  UnlockSemaphoreInfo(cache_info->semaphore);
}

MagickPrivate void SyncAuthenticOpenCLBuffer(const Image *image)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (const Image *) NULL);
  cache_info=(CacheInfo *) image->cache;
  CopyOpenCLBuffer(cache_info);
}
#endif

static MagickBooleanType ApplyPixelCacheNexusMasks(Image *image,
  NexusInfo *magick_restrict nexus_info,ExceptionInfo *exception)
{
  if ((image->mask_trait & UpdatePixelTrait) != 0)
    {
      if (((image->channels & WriteMaskChannel) != 0) &&
          (ClipPixelCacheNexus(image,nexus_info,exception) == MagickFalse))
        return(MagickFalse);
      if (((image->channels & CompositeMaskChannel) != 0) &&
          (MaskPixelCacheNexus(image,nexus_info,exception) == MagickFalse))
        return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   S y n c A u t h e n t i c P i x e l C a c h e N e x u s                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncAuthenticPixelCacheNexus() saves the authentic image pixels to the
%  in-memory or disk cache.  The method returns MagickTrue if the pixel region
%  is synced, otherwise MagickFalse.
%
%  The format of the SyncAuthenticPixelCacheNexus() method is:
%
%      MagickBooleanType SyncAuthenticPixelCacheNexus(Image *image,
%        NexusInfo *nexus_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o nexus_info: the cache nexus to sync.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickPrivate MagickBooleanType SyncAuthenticPixelCacheNexus(Image *image,
  NexusInfo *magick_restrict nexus_info,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  MagickBooleanType
    status;

  /*
    Transfer pixels to the cache.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->cache == (Cache) NULL)
    ThrowBinaryException(CacheError,"PixelCacheIsNotOpen",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->type == UndefinedCache)
    return(MagickFalse);
  if (ApplyPixelCacheNexusMasks(image,nexus_info,exception) == MagickFalse)
    return(MagickFalse);
  if (nexus_info->authentic_pixel_cache != MagickFalse)
    {
      if (image->taint == MagickFalse)
        image->taint=MagickTrue;
      return(MagickTrue);
    }
  assert(cache_info->signature == MagickCoreSignature);
  status=WritePixelCachePixels(cache_info,nexus_info,exception);
  if ((cache_info->metacontent_extent != 0) &&
      (WritePixelCacheMetacontent(cache_info,nexus_info,exception) == MagickFalse))
    return(MagickFalse);
  if ((status != MagickFalse) && (image->taint == MagickFalse))
    image->taint=MagickTrue;
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   S y n c I m a g e P i x e l C a c h e                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncImagePixelCache() saves the image pixels to the in-memory or disk cache.
%  The method returns MagickTrue if the pixel region is flushed, otherwise
%  MagickFalse.
%
%  The format of the SyncImagePixelCache() method is:
%
%      MagickBooleanType SyncImagePixelCache(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickPrivate MagickBooleanType SyncImagePixelCache(Image *image,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  assert(image != (Image *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  cache_info=(CacheInfo *) GetImagePixelCache(image,MagickTrue,exception);
  return(cache_info == (CacheInfo *) NULL ? MagickFalse : MagickTrue);
}

#include "MagickCore/cache-write-private.h"
