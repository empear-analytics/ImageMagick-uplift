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

  MagickCore private methods of cache.c: the nexus clip mask and the pixel
  cache clone and release helpers, ClipPixelChannels to
  RelinquishPixelCacheResources; included by that file only.
*/
#ifndef MAGICKCORE_CACHE_CLONE_PRIVATE_H
#define MAGICKCORE_CACHE_CLONE_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   C l i p P i x e l C a c h e N e x u s                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClipPixelCacheNexus() clips the cache nexus as defined by the image clip
%  mask.  The method returns MagickTrue if the pixel region is clipped,
%  otherwise MagickFalse.
%
%  The format of the ClipPixelCacheNexus() method is:
%
%      MagickBooleanType ClipPixelCacheNexus(Image *image,NexusInfo *nexus_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o nexus_info: the cache nexus to clip.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void ClipPixelChannels(const Image *image,const double mask,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  double
    mask_alpha,
    dst_alpha;

  Quantum
    src_alpha;

  ssize_t
    i;

  src_alpha=GetPixelAlpha(image,p);
  mask_alpha=QuantumScale*mask*(double) src_alpha;
  dst_alpha=(double) GetPixelAlpha(image,q);
  for (i=0; i < (ssize_t) image->number_channels; i++)
  {
    PixelTrait
      traits;

    PixelChannel channel = GetPixelChannelChannel(image,i);
    if (channel == AlphaPixelChannel)
      continue;
    traits=GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=ClampToQuantum(MagickOver_((double) p[i],mask_alpha,
      (double) q[i],dst_alpha));
  }
  SetPixelAlpha(image,src_alpha,q);
}

static void ClipNexusPixel(const Image *image,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  double
    mask;

  mask=(double) GetPixelWriteMask(image,p);
  if (fabs(mask) >= MagickEpsilon)
    ClipPixelChannels(image,mask,p,q);
}

typedef enum
{
  ClipNexusMask,
  CompositeNexusMask
} NexusMaskSide;

static void
  MaskNexusPixel(const Image *,const Quantum *magick_restrict,
    Quantum *magick_restrict);

static MagickBooleanType ApplyNexusMaskPixels(Image *image,
  NexusInfo *nexus_info,const NexusMaskSide side,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict p,
    *magick_restrict q;

  ssize_t
    y;

  p=GetAuthenticPixelCacheNexus(image,nexus_info->region.x,nexus_info->region.y,
    nexus_info->region.width,nexus_info->region.height,
    nexus_info->virtual_nexus,exception);
  q=nexus_info->pixels;
  if ((p == (Quantum *) NULL) || (q == (Quantum *) NULL))
    return(MagickFalse);
  for (y=0; y < (ssize_t) nexus_info->region.height; y++)
  {
    ssize_t
      x;

    for (x=0; x < (ssize_t) nexus_info->region.width; x++)
    {
      ((side == ClipNexusMask) ? ClipNexusPixel(image,p,q) :
        MaskNexusPixel(image,p,q));
      p+=(ptrdiff_t) GetPixelChannels(image);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(MagickTrue);
}

static inline MagickBooleanType ApplyNexusMask(Image *image,
  NexusInfo *nexus_info,const NexusMaskSide side,ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info;

  /*
    Apply clip mask.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->channels & ((side == ClipNexusMask) ?
    WriteMaskChannel : CompositeMaskChannel)) == 0)
    return(MagickTrue);
  if ((nexus_info->region.width == 0) || (nexus_info->region.height == 0))
    return(MagickTrue);
  cache_info=(CacheInfo *) image->cache;
  if (cache_info == ((side == ClipNexusMask) ? (CacheInfo *) NULL :
      (Cache) NULL))
    return(MagickFalse);
  return(ApplyNexusMaskPixels(image,nexus_info,side,exception));
}

static MagickBooleanType ClipPixelCacheNexus(Image *image,
  NexusInfo *nexus_info,ExceptionInfo *exception)
{
  return(ApplyNexusMask(image,nexus_info,ClipNexusMask,exception));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   C l o n e P i x e l C a c h e R e p o s i t o r y                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClonePixelCacheRepository() clones the source pixel cache to the destination
%  cache.
%
%  The format of the ClonePixelCacheRepository() method is:
%
%      MagickBooleanType ClonePixelCacheRepository(CacheInfo *clone_info,
%        CacheInfo *cache_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o clone_info: the pixel cache.
%
%    o cache_info: the source pixel cache.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType WritePixelCacheCloneBuffer(
  CacheInfo *magick_restrict clone_info,unsigned char *buffer,
  const ssize_t count,MagickSizeType *extent)
{
  ssize_t
    number_bytes,
    offset = 0;

  while (offset < count)
  {
    number_bytes=MagickWrite(clone_info->file,buffer+offset,(size_t) (count-
      offset));
    if (number_bytes <= 0)
      return(MagickFalse);
    offset+=number_bytes;
  }
  *extent+=(MagickSizeType) offset;
  return(MagickTrue);
}

static MagickBooleanType ClonePixelCacheOnDisk(
  CacheInfo *magick_restrict cache_info,CacheInfo *magick_restrict clone_info)
{
  MagickSizeType
    extent;

  size_t
    quantum;

  ssize_t
    count;

  struct stat
    file_stats;

  unsigned char
    *buffer;

  /*
    Clone pixel cache on disk with identical morphology.
  */
  if ((OpenPixelCacheOnDisk(cache_info,ReadMode) == MagickFalse) ||
      (OpenPixelCacheOnDisk(clone_info,IOMode) == MagickFalse))
    return(MagickFalse);
  if ((lseek(cache_info->file,0,SEEK_SET) < 0) ||
      (lseek(clone_info->file,0,SEEK_SET) < 0))
    return(MagickFalse);
  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(cache_info->file,&file_stats) == 0) && (file_stats.st_size > 0))
    {
#if defined(MAGICKCORE_HAVE_LINUX_SENDFILE)
      if (cache_info->length < 0x7ffff000)
        {
          do
          {
            count=sendfile(clone_info->file,cache_info->file,(off_t *) NULL,
              (size_t) cache_info->length);
          } while ((count < 0) && (errno == EINTR));
          if (count == (ssize_t) cache_info->length)
            return(MagickTrue);
          if (count < 0)
            return(MagickFalse);
          if ((lseek(cache_info->file,0,SEEK_SET) < 0) ||
              (lseek(clone_info->file,0,SEEK_SET) < 0))
            return(MagickFalse);
        }
#endif
      quantum=(size_t) MagickMin(file_stats.st_size,MagickMaxBufferExtent);
    }
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    return(MagickFalse);
  extent=0;
  while (extent < cache_info->length)
  {
    size_t
      length;

    length=(size_t) MagickMin((MagickSizeType) quantum,cache_info->length-
      extent);
    count=MagickRead(cache_info->file,buffer,length);
    if (count <= 0)
      {
        buffer=(unsigned char *) RelinquishMagickMemory(buffer);
        return(MagickFalse);
      }
    if (WritePixelCacheCloneBuffer(clone_info,buffer,count,&extent) ==
        MagickFalse)
      {
        buffer=(unsigned char *) RelinquishMagickMemory(buffer);
        return(MagickFalse);
      }
  }
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  return(extent == cache_info->length ? MagickTrue : MagickFalse);
}

#if defined(MAGICKCORE_OPENMP_SUPPORT)
static inline MagickBooleanType IsNonMemoryCachePair(const CacheInfo *source,
  const CacheInfo *destination)
{
  return((((source->type != MemoryCache) && (source->type != MapCache)) ||
    ((destination->type != MemoryCache) &&
    (destination->type != MapCache))) ? MagickTrue : MagickFalse);
}

static inline int GetCacheNumberThreads(const CacheInfo *source,
  const CacheInfo *destination,const size_t chunk,const int factor)
{
  size_t
    max_threads = (size_t) GetMagickResourceLimit(ThreadResource),
    number_threads = 1UL,
    workload_factor = 64UL << factor;
  
  /*
    Determine number of threads based on workload.
  */
  number_threads=(chunk <= workload_factor) ? 1UL :
    (chunk >= (workload_factor << 6)) ? max_threads :
    1UL+(chunk-workload_factor)*(max_threads-1L)/(((workload_factor << 6))-1L);
  /*
    Limit threads for non-memory or non-map cache sources/destinations.
  */
  if (IsNonMemoryCachePair(source,destination) != MagickFalse)
    number_threads=MagickMin(number_threads,4);
  return((int) number_threads);
}
#endif

static inline MagickBooleanType IsSameCacheMorphology(
  const CacheInfo *cache_info,const CacheInfo *clone_info,const size_t length)
{
  return(((cache_info->storage_class == clone_info->storage_class) &&
    (cache_info->colorspace == clone_info->colorspace) &&
    (cache_info->alpha_trait == clone_info->alpha_trait) &&
    (cache_info->channels == clone_info->channels) &&
    (cache_info->columns == clone_info->columns) &&
    (cache_info->rows == clone_info->rows) &&
    (cache_info->number_channels == clone_info->number_channels) &&
    (memcmp(cache_info->channel_map,clone_info->channel_map,length) == 0) &&
    (cache_info->metacontent_extent == clone_info->metacontent_extent)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsMemoryCachePair(const CacheInfo *cache_info,
  const CacheInfo *clone_info)
{
  return((((cache_info->type == MemoryCache) ||
    (cache_info->type == MapCache)) &&
    ((clone_info->type == MemoryCache) ||
    (clone_info->type == MapCache))) ? MagickTrue : MagickFalse);
}

static void ClonePixelCacheChannels(const CacheInfo *magick_restrict cache_info,
  const CacheInfo *magick_restrict clone_info,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) cache_info->columns; x++)
  {
    ssize_t
      i;

    if (x == (ssize_t) clone_info->columns)
      break;
    for (i=0; i < (ssize_t) clone_info->number_channels; i++)
    {
      PixelChannel
        channel;

      PixelTrait
        traits;

      channel=clone_info->channel_map[i].channel;
      traits=cache_info->channel_map[channel].traits;
      if (traits != UndefinedPixelTrait)
        *q=*(p+cache_info->channel_map[channel].offset);
      q++;
    }
    p+=(ptrdiff_t) cache_info->number_channels;
  }
}

static MagickBooleanType ClonePixelCacheRepository(
  CacheInfo *magick_restrict clone_info,CacheInfo *magick_restrict cache_info,
  ExceptionInfo *exception)
{
#define cache_number_threads(source,destination,chunk,factor) \
  num_threads(GetCacheNumberThreads((source),(destination),(chunk),(factor)))

  MagickBooleanType
    optimize,
    status;

  NexusInfo
    **magick_restrict cache_nexus,
    **magick_restrict clone_nexus;

  size_t
    length;

  ssize_t
    y;

  assert(cache_info != (CacheInfo *) NULL);
  assert(clone_info != (CacheInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (cache_info->type == PingCache)
    return(MagickTrue);
  length=cache_info->number_channels*sizeof(*cache_info->channel_map);
  if (IsSameCacheMorphology(cache_info,clone_info,length) != MagickFalse)
    {
      /*
        Identical pixel cache morphology.
      */
      if (IsMemoryCachePair(cache_info,clone_info) != MagickFalse)
        {
          (void) memcpy(clone_info->pixels,cache_info->pixels,
            cache_info->number_channels*cache_info->columns*cache_info->rows*
            sizeof(*cache_info->pixels));
          if ((cache_info->metacontent_extent != 0) &&
              (clone_info->metacontent_extent != 0))
            (void) memcpy(clone_info->metacontent,cache_info->metacontent,
              cache_info->columns*cache_info->rows*
              clone_info->metacontent_extent*sizeof(unsigned char));
          return(MagickTrue);
        }
      if ((cache_info->type == DiskCache) && (clone_info->type == DiskCache))
        return(ClonePixelCacheOnDisk(cache_info,clone_info));
    }
  /*
    Mismatched pixel cache morphology.
  */
  cache_nexus=AcquirePixelCacheNexus(cache_info->number_threads);
  clone_nexus=AcquirePixelCacheNexus(clone_info->number_threads);
  length=cache_info->number_channels*sizeof(*cache_info->channel_map);
  optimize=(cache_info->number_channels == clone_info->number_channels) &&
    (memcmp(cache_info->channel_map,clone_info->channel_map,length) == 0) ?
    MagickTrue : MagickFalse;
  length=(size_t) MagickMin(cache_info->number_channels*cache_info->columns,
    clone_info->number_channels*clone_info->columns);
  status=MagickTrue;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    cache_number_threads(cache_info,clone_info,cache_info->rows,3)
#endif
  for (y=0; y < (ssize_t) cache_info->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    Quantum
      *pixels;

    if (status == MagickFalse)
      continue;
    if (y >= (ssize_t) clone_info->rows)
      continue;
    pixels=SetPixelCacheNexusPixels(cache_info,ReadMode,0,y,
      cache_info->columns,1,MagickFalse,cache_nexus[id],exception);
    if (pixels == (Quantum *) NULL)
      continue;
    status=ReadPixelCachePixels(cache_info,cache_nexus[id],exception);
    if (status == MagickFalse)
      continue;
    pixels=SetPixelCacheNexusPixels(clone_info,WriteMode,0,y,
      clone_info->columns,1,MagickFalse,clone_nexus[id],exception);
    if (pixels == (Quantum *) NULL)
      continue;
    (void) memset(clone_nexus[id]->pixels,0,(size_t) clone_nexus[id]->length);
    if (optimize != MagickFalse)
      (void) memcpy(clone_nexus[id]->pixels,cache_nexus[id]->pixels,length*
        sizeof(Quantum));
    else
      {
        const Quantum
          *magick_restrict p;

        Quantum
          *magick_restrict q;

        /*
          Mismatched pixel channel map.
        */
        p=cache_nexus[id]->pixels;
        q=clone_nexus[id]->pixels;
        ClonePixelCacheChannels(cache_info,clone_info,p,q);
      }
    status=WritePixelCachePixels(clone_info,clone_nexus[id],exception);
  }
  if ((cache_info->metacontent_extent != 0) &&
      (clone_info->metacontent_extent != 0))
    {
      /*
        Clone metacontent.
      */
      length=(size_t) MagickMin(cache_info->metacontent_extent,
        clone_info->metacontent_extent);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        cache_number_threads(cache_info,clone_info,cache_info->rows,3)
#endif
      for (y=0; y < (ssize_t) cache_info->rows; y++)
      {
        const int
          id = GetOpenMPThreadId();

        Quantum
          *pixels;

        if (status == MagickFalse)
          continue;
        if (y >= (ssize_t) clone_info->rows)
          continue;
        pixels=SetPixelCacheNexusPixels(cache_info,ReadMode,0,y,
          cache_info->columns,1,MagickFalse,cache_nexus[id],exception);
        if (pixels == (Quantum *) NULL)
          continue;
        status=ReadPixelCacheMetacontent(cache_info,cache_nexus[id],exception);
        if (status == MagickFalse)
          continue;
        pixels=SetPixelCacheNexusPixels(clone_info,WriteMode,0,y,
          clone_info->columns,1,MagickFalse,clone_nexus[id],exception);
        if (pixels == (Quantum *) NULL)
          continue;
        if ((clone_nexus[id]->metacontent != (void *) NULL) &&
            (cache_nexus[id]->metacontent != (void *) NULL))
          (void) memcpy(clone_nexus[id]->metacontent,
            cache_nexus[id]->metacontent,length*sizeof(unsigned char));
        status=WritePixelCacheMetacontent(clone_info,clone_nexus[id],exception);
      }
    }
  clone_nexus=DestroyPixelCacheNexus(clone_nexus,clone_info->number_threads);
  cache_nexus=DestroyPixelCacheNexus(cache_nexus,cache_info->number_threads);
  if (cache_info->debug != MagickFalse)
    {
      char
        message[MagickPathExtent];

      (void) FormatLocaleString(message,MagickPathExtent,"%s => %s",
        CommandOptionToMnemonic(MagickCacheOptions,(ssize_t) cache_info->type),
        CommandOptionToMnemonic(MagickCacheOptions,(ssize_t) clone_info->type));
      (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",message);
    }
  return(status);
}


static MagickBooleanType ClosePixelCacheOnDisk(CacheInfo *cache_info)
{
  int
    status;

  status=(-1);
  if (cache_info->file != -1)
    {
      status=close_utf8(cache_info->file);
      cache_info->file=(-1);
      RelinquishMagickResource(FileResource,1);
    }
  return(status == -1 ? MagickFalse : MagickTrue);
}

static inline void RelinquishPixelCachePixels(CacheInfo *cache_info)
{
  switch (cache_info->type)
  {
    case MemoryCache:
    {
      (void) ShredMagickMemory(cache_info->pixels,(size_t) cache_info->length);
#if defined(MAGICKCORE_OPENCL_SUPPORT)
      if (cache_info->opencl != (MagickCLCacheInfo) NULL)
        {
          cache_info->opencl=RelinquishMagickCLCacheInfo(cache_info->opencl,
            MagickTrue);
          cache_info->pixels=(Quantum *) NULL;
          break;
        }
#endif
      if (cache_info->mapped == MagickFalse)
        cache_info->pixels=(Quantum *) RelinquishAlignedMemory(
          cache_info->pixels);
      else
        {
          (void) UnmapBlob(cache_info->pixels,(size_t) cache_info->length);
          cache_info->pixels=(Quantum *) NULL;
        }
      RelinquishMagickResource(MemoryResource,cache_info->length);
      break;
    }
    case MapCache:
    {
      (void) UnmapBlob(cache_info->pixels,(size_t) cache_info->length);
      cache_info->pixels=(Quantum *) NULL;
      if ((cache_info->mode != ReadMode) && (cache_info->mode != PersistMode))
        (void) RelinquishUniqueFileResource(cache_info->cache_filename);
      *cache_info->cache_filename='\0';
      RelinquishMagickResource(MapResource,cache_info->length);
      magick_fallthrough;
    }
    case DiskCache:
    {
      if (cache_info->file != -1)
        (void) ClosePixelCacheOnDisk(cache_info);
      if ((cache_info->mode != ReadMode) && (cache_info->mode != PersistMode))
        (void) RelinquishUniqueFileResource(cache_info->cache_filename);
      *cache_info->cache_filename='\0';
      RelinquishMagickResource(DiskResource,cache_info->length);
      break;
    }
    case DistributedCache:
    {
      *cache_info->cache_filename='\0';
      (void) RelinquishDistributePixelCache((DistributeCacheInfo *)
        cache_info->server_info);
      break;
    }
    default:
      break;
  }
  cache_info->type=UndefinedCache;
  cache_info->mapped=MagickFalse;
  cache_info->metacontent=(void *) NULL;
}

static void RelinquishPixelCacheResources(
  CacheInfo *magick_restrict cache_info)
{
  if (cache_info->server_info != (DistributeCacheInfo *) NULL)
    cache_info->server_info=DestroyDistributeCacheInfo((DistributeCacheInfo *)
      cache_info->server_info);
  if (cache_info->nexus_info != (NexusInfo **) NULL)
    cache_info->nexus_info=DestroyPixelCacheNexus(cache_info->nexus_info,
      cache_info->number_threads);
  if (cache_info->random_info != (RandomInfo *) NULL)
    cache_info->random_info=DestroyRandomInfo(cache_info->random_info);
  if (cache_info->file_semaphore != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&cache_info->file_semaphore);
  if (cache_info->semaphore != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&cache_info->semaphore);
}

#endif
