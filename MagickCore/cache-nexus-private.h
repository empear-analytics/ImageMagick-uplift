/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private methods of cache.c: the nexus pixel helpers,
  AcquireCacheNexusPixels to SetCacheAlphaChannel; included by that file
  only.
*/
#ifndef MAGICKCORE_CACHE_NEXUS_PRIVATE_H
#define MAGICKCORE_CACHE_NEXUS_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   S e t P i x e l C a c h e N e x u s P i x e l s                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelCacheNexusPixels() defines the region of the cache for the
%  specified cache nexus.
%
%  The format of the SetPixelCacheNexusPixels() method is:
%
%      Quantum SetPixelCacheNexusPixels(
%        const CacheInfo *magick_restrict cache_info,const MapMode mode,
%        const ssize_t x,const ssize_t y,const size_t width,const size_t height,
%        const MagickBooleanType buffered,NexusInfo *magick_restrict nexus_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: the pixel cache.
%
%    o mode: ReadMode, WriteMode, or IOMode.
%
%    o x,y,width,height: define the region of this particular cache nexus.
%
%    o buffered: if true, nexus pixels are buffered.
%
%    o nexus_info: the cache nexus to set.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType AcquireCacheNexusPixels(
  const CacheInfo *magick_restrict cache_info,const MagickSizeType length,
  NexusInfo *magick_restrict nexus_info,ExceptionInfo *exception)
{
  if (length != (MagickSizeType) ((size_t) length))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"PixelCacheAllocationFailed","`%s'",
        cache_info->filename);
      return(MagickFalse);
    }
  nexus_info->length=0;
  nexus_info->mapped=MagickFalse;
  if (cache_anonymous_memory <= 0)
    {
      nexus_info->cache=(Quantum *) MagickAssumeAligned(AcquireAlignedMemory(1,
        (size_t) length));
      if (nexus_info->cache != (Quantum *) NULL)
        (void) memset(nexus_info->cache,0,(size_t) length);
    }
  else
    {
      nexus_info->cache=(Quantum *) MapBlob(-1,IOMode,0,(size_t) length);
      if (nexus_info->cache != (Quantum *) NULL)
        nexus_info->mapped=MagickTrue;
    }
  if (nexus_info->cache == (Quantum *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"PixelCacheAllocationFailed","`%s'",
        cache_info->filename);
      return(MagickFalse);
    }
  nexus_info->length=length;
  return(MagickTrue);
}

static inline void PrefetchPixelCacheNexusPixels(const NexusInfo *nexus_info,
  const MapMode mode)
{
  if (nexus_info->length < CACHE_LINE_SIZE)
    return;
  if (mode == ReadMode)
    {
      MagickCachePrefetch((unsigned char *) nexus_info->pixels+CACHE_LINE_SIZE,
        0,1);
      return;
    }
  MagickCachePrefetch((unsigned char *) nexus_info->pixels+CACHE_LINE_SIZE,1,1);
}

static inline MagickBooleanType IsUnbufferedMemoryCache(
  const CacheInfo *cache_info,const MagickBooleanType buffered)
{
  return((((cache_info->type == MemoryCache) ||
    (cache_info->type == MapCache)) &&
    (buffered == MagickFalse)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsDirectCacheRegion(const CacheInfo *cache_info,
  const ssize_t x,const ssize_t y,const size_t width,const size_t height)
{
  return((((x >= 0) && (y >= 0) &&
    (((ssize_t) height+y-1) < (ssize_t) cache_info->rows)) &&
    (((x == 0) && (width == cache_info->columns)) || ((height == 1) &&
    (((ssize_t) width+x-1) < (ssize_t) cache_info->columns)))) ? MagickTrue :
    MagickFalse);
}

static Quantum *SetPixelCacheNexusPixels(
  const CacheInfo *magick_restrict cache_info,const MapMode mode,
  const ssize_t x,const ssize_t y,const size_t width,const size_t height,
  const MagickBooleanType buffered,NexusInfo *magick_restrict nexus_info,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  MagickSizeType
    length,
    number_pixels;

  assert(cache_info != (const CacheInfo *) NULL);
  assert(cache_info->signature == MagickCoreSignature);
  if (cache_info->type == UndefinedCache)
    return((Quantum *) NULL);
  assert(nexus_info->signature == MagickCoreSignature);
  (void) memset(&nexus_info->region,0,sizeof(nexus_info->region));
  if ((width == 0) || (height == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "NoPixelsDefinedInCache","`%s'",cache_info->filename);
      return((Quantum *) NULL);
    }
  if (((MagickSizeType) width > cache_info->width_limit) ||
      ((MagickSizeType) height > cache_info->height_limit))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "WidthOrHeightExceedsLimit","`%s'",cache_info->filename);
      return((Quantum *) NULL);
    }
  if ((IsValidPixelOffset(x,width) == MagickFalse) ||
      (IsValidPixelOffset(y,height) == MagickFalse))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
        "InvalidPixel","`%s'",cache_info->filename);
      return((Quantum *) NULL);
    }
  if (IsUnbufferedMemoryCache(cache_info,buffered) != MagickFalse)
    {
      if (IsDirectCacheRegion(cache_info,x,y,width,height) != MagickFalse)
        {
          MagickOffsetType
            offset;

          /*
            Pixels are accessed directly from memory.
          */
          if (IsValidPixelOffset(y,cache_info->columns) == MagickFalse)
            return((Quantum *) NULL);
          offset=y*(MagickOffsetType) cache_info->columns+x;
          nexus_info->pixels=cache_info->pixels+(MagickOffsetType)
            cache_info->number_channels*offset;
          nexus_info->metacontent=(void *) NULL;
          if (cache_info->metacontent_extent != 0)
            nexus_info->metacontent=(unsigned char *) cache_info->metacontent+
              offset*(MagickOffsetType) cache_info->metacontent_extent;
          nexus_info->region.width=width;
          nexus_info->region.height=height;
          nexus_info->region.x=x;
          nexus_info->region.y=y;
          nexus_info->authentic_pixel_cache=MagickTrue;
          PrefetchPixelCacheNexusPixels(nexus_info,mode);
          return(nexus_info->pixels);
        }
    }
  /*
    Pixels are stored in a staging region until they are synced to the cache.
  */
  if (CacheOverflowSanityCheckGetSize(width,height,&number_pixels) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",
        cache_info->filename);
      return((Quantum *) NULL);
    }
  {
    MagickSizeType
      extent,
      packets;

    packets=MagickMax(number_pixels,MagickMax(cache_info->columns,
      cache_info->rows));
    if ((CacheOverflowSanityCheckGetSize(packets,cache_info->number_channels,&length) != MagickFalse) ||
        (CacheOverflowSanityCheckGetSize(length,sizeof(*nexus_info->pixels),&length) != MagickFalse))
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'",
          cache_info->filename);
        return((Quantum *) NULL);
      }
    if (cache_info->metacontent_extent != 0)
      {
        if ((CacheOverflowSanityCheckGetSize(number_pixels,cache_info->metacontent_extent,&extent) != MagickFalse) ||
            (extent > (MagickSizeType) (~((MagickSizeType) 0))-length))
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'",
              cache_info->filename);
            return((Quantum *) NULL);
          }
        length+=extent;
      }
   }
  status=MagickTrue;
  if (nexus_info->cache == (Quantum *) NULL)
    status=AcquireCacheNexusPixels(cache_info,length,nexus_info,exception);
  else
    if (nexus_info->length < length)
      {
        RelinquishCacheNexusPixels(nexus_info);
        status=AcquireCacheNexusPixels(cache_info,length,nexus_info,exception);
      }
  if (status == MagickFalse)
    return((Quantum *) NULL);
  nexus_info->pixels=nexus_info->cache;
  nexus_info->metacontent=(void *) NULL;
  if (cache_info->metacontent_extent != 0)
    nexus_info->metacontent=(void *) (nexus_info->pixels+
      cache_info->number_channels*number_pixels);
  nexus_info->region.width=width;
  nexus_info->region.height=height;
  nexus_info->region.x=x;
  nexus_info->region.y=y;
  nexus_info->authentic_pixel_cache=cache_info->type == PingCache ?
    MagickTrue : MagickFalse;
  PrefetchPixelCacheNexusPixels(nexus_info,mode);
  return(nexus_info->pixels);
}

static MagickBooleanType SetCacheAlphaChannel(Image *image,const Quantum alpha,
  ExceptionInfo *exception)
{
  CacheView
    *magick_restrict image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  image->alpha_trait=BlendPixelTrait;
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);  /* must be virtual */
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
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      SetPixelAlpha(image,alpha,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status=SyncCacheViewAuthenticPixels(image_view,exception);
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

#endif
