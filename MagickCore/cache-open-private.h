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

  MagickCore private methods of cache.c: the nexus composite mask and
  OpenPixelCache with its helpers, ApplyPixelCompositeMask to
  AcquirePersistentPixelCache; included by that file only.
*/
#ifndef MAGICKCORE_CACHE_OPEN_PRIVATE_H
#define MAGICKCORE_CACHE_OPEN_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   M a s k P i x e l C a c h e N e x u s                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MaskPixelCacheNexus() masks the cache nexus as defined by the composite mask.
%  The method returns MagickTrue if the pixel region is masked, otherwise
%  MagickFalse.
%
%  The format of the MaskPixelCacheNexus() method is:
%
%      MagickBooleanType MaskPixelCacheNexus(Image *image,
%        NexusInfo *nexus_info,ExceptionInfo *exception)
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

static inline Quantum ApplyPixelCompositeMask(const Quantum p,
  const MagickRealType alpha,const Quantum q,const MagickRealType beta)
{
  double
    gamma;

  if (fabs((double) (alpha-(double) TransparentAlpha)) < MagickEpsilon)
    return(q);
  gamma=1.0-QuantumScale*QuantumScale*alpha*beta;
  gamma=MagickSafeReciprocal(gamma);
  return(ClampToQuantum(gamma*MagickOver_((double) p,alpha,(double) q,beta)));
}

static void MaskPixelChannels(const Image *image,const double alpha,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->number_channels; i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    q[i]=ApplyPixelCompositeMask(q[i],alpha,p[i],GetPixelAlpha(image,p));
  }
}

static void MaskNexusPixel(const Image *image,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  double
    alpha;

  alpha=(double) GetPixelCompositeMask(image,p);
  MaskPixelChannels(image,alpha,p,q);
}

static MagickBooleanType MaskPixelCacheNexus(Image *image,NexusInfo *nexus_info,
  ExceptionInfo *exception)
{
  return(ApplyNexusMask(image,nexus_info,CompositeNexusMask,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   O p e n P i x e l C a c h e                                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenPixelCache() allocates the pixel cache.  This includes defining the cache
%  dimensions, allocating space for the image pixels and optionally the
%  metacontent, and memory mapping the cache if it is disk based.  The cache
%  nexus array is initialized as well.
%
%  The format of the OpenPixelCache() method is:
%
%      MagickBooleanType OpenPixelCache(Image *image,const MapMode mode,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o mode: ReadMode, WriteMode, or IOMode.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType CacheOverflowSanityCheckGetSize(
  const MagickSizeType count,const size_t quantum,MagickSizeType *const extent)
{
  MagickSizeType
    length;

  if ((count == 0) || (quantum == 0))
    return(MagickTrue);
  length=count*quantum;
  if (quantum != (length/count))
    {
      errno=ENOMEM;
      return(MagickTrue);
    }
  if (extent != NULL)
    *extent=length;
  return(MagickFalse);
}

static int OpenPixelCacheFile(const CacheInfo *cache_info,const MapMode mode)
{
  int
    file;

  switch (mode)
  {
    case ReadMode:
    {
      file=open_utf8(cache_info->cache_filename,O_RDONLY | O_CLOEXEC | O_BINARY,0);
      break;
    }
    case WriteMode:
    {
      file=open_utf8(cache_info->cache_filename,O_WRONLY | O_CLOEXEC | O_CREAT |
        O_BINARY | O_EXCL,S_MODE);
      if (file == -1)
        file=open_utf8(cache_info->cache_filename,O_WRONLY | O_CLOEXEC | O_BINARY,S_MODE);
      break;
    }
    case IOMode:
    default:
    {
      file=open_utf8(cache_info->cache_filename,O_RDWR | O_CLOEXEC | O_CREAT | O_BINARY |
        O_EXCL,S_MODE);
      if (file == -1)
        file=open_utf8(cache_info->cache_filename,O_RDWR | O_CLOEXEC | O_BINARY,S_MODE);
      break;
    }
  }
  return(file);
}

static MagickBooleanType OpenPixelCacheOnDisk(CacheInfo *cache_info,
  const MapMode mode)
{
  int
    file;

  /*
    Open pixel cache on disk.
  */
  if ((cache_info->file != -1) && (cache_info->disk_mode == mode))
    return(MagickTrue);  /* cache already open and in the proper mode */
  if (*cache_info->cache_filename == '\0')
    file=AcquireUniqueFileResource(cache_info->cache_filename);
  else
    file=OpenPixelCacheFile(cache_info,mode);
  if (file == -1)
    return(MagickFalse);
  (void) AcquireMagickResource(FileResource,1);
  if (cache_info->file != -1)
    (void) ClosePixelCacheOnDisk(cache_info);
  cache_info->file=file;
  cache_info->disk_mode=mode;
  return(MagickTrue);
}

static inline MagickOffsetType WritePixelCacheRegion(
  const CacheInfo *magick_restrict cache_info,const MagickOffsetType offset,
  const MagickSizeType length,const unsigned char *magick_restrict buffer)
{
  MagickOffsetType
    i;

  ssize_t
    count = 0;

  if (lseek(cache_info->file,offset,SEEK_SET) < 0)
    return((MagickOffsetType) -1);
  for (i=0; i < (MagickOffsetType) length; i+=count)
  {
    count=MagickWrite(cache_info->file,buffer+i,(size_t) MagickMin(length-
      (MagickSizeType) i,MagickMaxBufferExtent));
    if (count <= 0)
      break;
  }
  return(i);
}

static MagickBooleanType SetPixelCacheExtent(Image *image,MagickSizeType length)
{
  CacheInfo
    *magick_restrict cache_info;

  MagickOffsetType
    offset;

  cache_info=(CacheInfo *) image->cache;
  if (cache_info->debug != MagickFalse)
    {
      char
        format[MagickPathExtent],
        message[MagickPathExtent];

      (void) FormatMagickSize(length,MagickFalse,"B",MagickPathExtent,format);
      (void) FormatLocaleString(message,MagickPathExtent,
        "extend %s (%s[%d], disk, %s)",cache_info->filename,
        cache_info->cache_filename,cache_info->file,format);
      (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",message);
    }
  if (length != (MagickSizeType) ((MagickOffsetType) length))
    return(MagickFalse);
  offset=(MagickOffsetType) lseek(cache_info->file,0,SEEK_END);
  if (offset < 0)
    return(MagickFalse);
  if ((MagickSizeType) offset < length)
    {
      MagickOffsetType
        count,
        extent;

      extent=(MagickOffsetType) length-1;
      count=WritePixelCacheRegion(cache_info,extent,1,(const unsigned char *)
        "");
      if (count != 1)
        return(MagickFalse);
#if defined(MAGICKCORE_HAVE_POSIX_FALLOCATE)
      if (cache_info->synchronize != MagickFalse)
        if (posix_fallocate(cache_info->file,offset+1,extent-offset) != 0)
          return(MagickFalse);
#endif
    }
  offset=(MagickOffsetType) lseek(cache_info->file,0,SEEK_SET);
  if (offset < 0)
    return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsCacheGeometryInvalid(
  const CacheInfo *cache_info,const size_t columns)
{
  return(((cache_info->columns != columns) ||
    ((ssize_t) cache_info->columns < 0) ||
    ((ssize_t) cache_info->rows < 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsMemoryCacheCandidate(
  const MagickBooleanType status,const MagickSizeType length,
  const CacheInfo *cache_info)
{
  return(((status != MagickFalse) &&
    (length == (MagickSizeType) ((size_t) length)) &&
    ((cache_info->type == UndefinedCache) ||
    (cache_info->type == MemoryCache))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsCacheClonePending(const CacheInfo source_info,
  const MapMode mode,const CacheInfo *cache_info)
{
  return(((source_info.storage_class != UndefinedClass) && (mode != ReadMode) &&
    (cache_info->mode != PersistMode)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsCacheSourcePending(
  const CacheInfo source_info,const MapMode mode)
{
  return(((source_info.storage_class != UndefinedClass) && (mode != ReadMode)) ?
    MagickTrue : MagickFalse);
}

typedef struct _PixelCacheOpenInfo
{
  Image
    *image;

  MapMode
    mode;

  MagickSizeType
    number_pixels;

  ExceptionInfo
    *exception;
} PixelCacheOpenInfo;

static MagickBooleanType CloneCacheSource(CacheInfo *cache_info,
  CacheInfo *source_info,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=ClonePixelCacheRepository(cache_info,source_info,exception);
  RelinquishPixelCachePixels(source_info);
  return(status);
}

static MagickBooleanType FinishPixelCacheOpen(CacheInfo *cache_info,
  CacheInfo *source_info,const MapMode mode,const MagickBooleanType status)
{
  if (status == 0)
    {
      if (IsCacheSourcePending(*source_info,mode) != MagickFalse)
        RelinquishPixelCachePixels(source_info);
      cache_info->type=UndefinedCache;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void LogMemoryPixelCacheOpen(const CacheInfo *cache_info)
{
  char
    format[MagickPathExtent],
    message[MagickPathExtent];

  const char
    *type;

  if (cache_info->debug != MagickFalse)
    {
      (void) FormatMagickSize(cache_info->length,MagickTrue,"B",
        MagickPathExtent,format);
      type=CommandOptionToMnemonic(MagickCacheOptions,(ssize_t)
        cache_info->type);
      (void) FormatLocaleString(message,MagickPathExtent,
        "open %s (%s %s, %.17gx%.17gx%.17g %s)",
        cache_info->filename,cache_info->mapped != MagickFalse ?
        "Anonymous" : "Heap",type,(double) cache_info->columns,
        (double) cache_info->rows,(double)
        cache_info->number_channels,format);
      (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
        message);
    }
}

static MagickBooleanType CreateMemoryPixelCache(CacheInfo *cache_info,
  const PixelCacheOpenInfo *open_info,CacheInfo source_info,
  MagickBooleanType status)
{
  Image
    *const image = open_info->image;

  const MapMode
    mode = open_info->mode;

  const MagickSizeType
    number_pixels = open_info->number_pixels;

  ExceptionInfo
    *const exception = open_info->exception;

  /*
    Create memory pixel cache.
  */
  cache_info->type=MemoryCache;
  cache_info->metacontent=(void *) NULL;
  if (cache_info->metacontent_extent != 0)
    cache_info->metacontent=(void *) (cache_info->pixels+
      cache_info->number_channels*number_pixels);
  if (IsCacheSourcePending(source_info,mode) != MagickFalse)
    status=CloneCacheSource(cache_info,&source_info,exception);
  LogMemoryPixelCacheOpen(cache_info);
  cache_info->storage_class=image->storage_class;
  return(FinishPixelCacheOpen(cache_info,&source_info,mode,status));
}

static MagickBooleanType CreateMappedPixelCache(CacheInfo *cache_info,
  const PixelCacheOpenInfo *open_info,CacheInfo source_info,
  MagickBooleanType status)
{
  const MapMode
    mode = open_info->mode;

  const MagickSizeType
    number_pixels = open_info->number_pixels;

  ExceptionInfo
    *const exception = open_info->exception;

  char
    format[MagickPathExtent],
    message[MagickPathExtent];

  const char
    *type;

  /*
    Create file-backed memory-mapped pixel cache.
  */
  (void) ClosePixelCacheOnDisk(cache_info);
  cache_info->type=MapCache;
  cache_info->mapped=MagickTrue;
  cache_info->metacontent=(void *) NULL;
  if (cache_info->metacontent_extent != 0)
    cache_info->metacontent=(void *) (cache_info->pixels+
      cache_info->number_channels*number_pixels);
  if (IsCacheSourcePending(source_info,mode) != MagickFalse)
    status=CloneCacheSource(cache_info,&source_info,exception);
  if (cache_info->debug != MagickFalse)
    {
      (void) FormatMagickSize(cache_info->length,MagickTrue,"B",
        MagickPathExtent,format);
      type=CommandOptionToMnemonic(MagickCacheOptions,(ssize_t)
        cache_info->type);
      (void) FormatLocaleString(message,MagickPathExtent,
        "open %s (%s[%d], %s, %.17gx%.17gx%.17g %s)",
        cache_info->filename,cache_info->cache_filename,
        cache_info->file,type,(double) cache_info->columns,
        (double) cache_info->rows,(double)
        cache_info->number_channels,format);
      (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
        message);
    }
  return(FinishPixelCacheOpen(cache_info,&source_info,mode,status));
}

static MagickBooleanType CreateDistributedPixelCache(CacheInfo *cache_info,
  const PixelCacheOpenInfo *open_info,CacheInfo source_info,
  DistributeCacheInfo *server_info)
{
  const MapMode
    mode = open_info->mode;

  ExceptionInfo
    *const exception = open_info->exception;

  char
    format[MagickPathExtent],
    message[MagickPathExtent];

  const char
    *type;

  MagickBooleanType
    status;

  /*
    Create a distributed pixel cache.
  */
  status=MagickTrue;
  cache_info->type=DistributedCache;
  cache_info->server_info=server_info;
  (void) FormatLocaleString(cache_info->cache_filename,
    MagickPathExtent,"%s:%d",GetDistributeCacheHostname(
    (DistributeCacheInfo *) cache_info->server_info),
    GetDistributeCachePort((DistributeCacheInfo *)
    cache_info->server_info));
  if (IsCacheSourcePending(source_info,mode) != MagickFalse)
    status=CloneCacheSource(cache_info,&source_info,exception);
  if (cache_info->debug != MagickFalse)
    {
      (void) FormatMagickSize(cache_info->length,MagickFalse,"B",
        MagickPathExtent,format);
      type=CommandOptionToMnemonic(MagickCacheOptions,(ssize_t)
        cache_info->type);
      (void) FormatLocaleString(message,MagickPathExtent,
        "open %s (%s[%d], %s, %.17gx%.17gx%.17g %s)",
        cache_info->filename,cache_info->cache_filename,
        GetDistributeCacheFile((DistributeCacheInfo *)
        cache_info->server_info),type,(double) cache_info->columns,
        (double) cache_info->rows,(double)
        cache_info->number_channels,format);
      (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
        message);
    }
  return(FinishPixelCacheOpen(cache_info,&source_info,mode,status));
}

static MagickBooleanType AcquireListLengthResource(const Image *image,
  MagickSizeType *length,ExceptionInfo *exception)
{
  if (GetMagickResourceLimit(ListLengthResource) != MagickResourceInfinity)
    {
      *length=GetImageListLength(image);
      if (AcquireMagickResource(ListLengthResource,*length) == MagickFalse)
        ThrowBinaryException(ResourceLimitError,"ListLengthExceedsLimit",
          image->filename);
    }
  return(MagickTrue);
}

static MagickBooleanType AddMetacontentPacketSize(const Image *image,
  CacheInfo *cache_info,size_t *packet_size,ExceptionInfo *exception)
{
  if (cache_info->metacontent_extent != 0)
    {
      if (HeapOverflowCheckAdd(*packet_size,cache_info->metacontent_extent) != MagickFalse)
        {
          cache_info->storage_class=UndefinedClass;
          cache_info->length=0;
          ThrowBinaryException(ResourceLimitError,"PixelCacheAllocationFailed",
            image->filename);
        }
      *packet_size+=cache_info->metacontent_extent;
    }
  return(MagickTrue);
}

static MagickBooleanType ExhaustPixelCacheResources(CacheInfo *cache_info,
  const PixelCacheOpenInfo *open_info,CacheInfo source_info)
{
  Image
    *const image = open_info->image;

  const MapMode
    mode = open_info->mode;

  ExceptionInfo
    *const exception = open_info->exception;

  if (IsCacheSourcePending(source_info,mode) != MagickFalse)
    RelinquishPixelCachePixels(&source_info);
  cache_info->type=UndefinedCache;
  (void) memset(image->channel_map,0,MaxPixelChannels*
    sizeof(*image->channel_map));
  (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
    "CacheResourcesExhausted","`%s'",image->filename);
  return(MagickFalse);
}

static MagickBooleanType FailPixelCacheOnDisk(CacheInfo *cache_info,
  const PixelCacheOpenInfo *open_info,CacheInfo source_info,const char *tag)
{
  Image
    *const image = open_info->image;

  const MapMode
    mode = open_info->mode;

  ExceptionInfo
    *const exception = open_info->exception;

  if (IsCacheSourcePending(source_info,mode) != MagickFalse)
    RelinquishPixelCachePixels(&source_info);
  cache_info->type=UndefinedCache;
  ThrowFileException(exception,CacheError,tag,
    image->filename);
  return(MagickFalse);
}

typedef struct _MemoryPixelCacheTry
{
  MagickBooleanType
    status;

  MagickSizeType
    length;

  CacheInfo
    *cache_info;

  PixelCacheOpenInfo
    open_info;

  CacheInfo
    source_info;
} MemoryPixelCacheTry;

static MagickBooleanType TryMemoryPixelCache(const MemoryPixelCacheTry *attempt,
  MagickBooleanType *result)
{
  MagickBooleanType
    status = attempt->status;

  const MagickSizeType
    length = attempt->length;

  CacheInfo
    *const cache_info = attempt->cache_info;

  const PixelCacheOpenInfo
    open_info = attempt->open_info;

  const CacheInfo
    source_info = attempt->source_info;

  if (IsMemoryCacheCandidate(status,length,cache_info) != MagickFalse)
    {
      status=AcquireMagickResource(MemoryResource,cache_info->length);
      if (status != MagickFalse)
        {
          status=MagickTrue;
          if (cache_anonymous_memory <= 0)
            {
              cache_info->mapped=MagickFalse;
              cache_info->pixels=(Quantum *) MagickAssumeAligned(
                AcquireAlignedMemory(1,(size_t) cache_info->length));
            }
          else
            {
              cache_info->mapped=MagickTrue;
              cache_info->pixels=(Quantum *) MapBlob(-1,IOMode,0,(size_t)
                cache_info->length);
            }
          if (cache_info->pixels == (Quantum *) NULL)
            {
              RelinquishMagickResource(MemoryResource,cache_info->length);
              cache_info->mapped=source_info.mapped;
              cache_info->pixels=source_info.pixels;
            }
          else
            {
              (*result)=CreateMemoryPixelCache(cache_info,&open_info,
                source_info,status);
              return(MagickTrue);
            }
        }
    }
  return(MagickFalse);
}

static MagickBooleanType CheckPixelCacheGeometryLimits(const Image *image,
  const CacheInfo *magick_restrict cache_info,ExceptionInfo *exception)
{
  if (((MagickSizeType) image->columns > cache_info->width_limit) ||
      ((MagickSizeType) image->rows > cache_info->height_limit))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "WidthOrHeightExceedsLimit","`%s' (%.17gx%.17g) > (%.17gx%.17g)",
        image->filename, (double) image->columns, (double) image->rows,
        (double) cache_info->width_limit,(double) cache_info->height_limit);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void LogDiskPixelCacheOpen(const CacheInfo *cache_info)
{
  char
    format[MagickPathExtent],
    message[MagickPathExtent];

  const char
    *type;

  if (cache_info->debug != MagickFalse)
    {
      (void) FormatMagickSize(cache_info->length,MagickFalse,"B",
        MagickPathExtent,format);
      type=CommandOptionToMnemonic(MagickCacheOptions,(ssize_t)
        cache_info->type);
      (void) FormatLocaleString(message,MagickPathExtent,
        "open %s (%s[%d], %s, %.17gx%.17gx%.17g %s)",cache_info->filename,
        cache_info->cache_filename,cache_info->file,type,(double)
        cache_info->columns,(double) cache_info->rows,(double)
        cache_info->number_channels,format);
      (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",message);
    }
}

static MagickBooleanType OpenPixelCache(Image *image,const MapMode mode,
  ExceptionInfo *exception)
{
  CacheInfo
    *magick_restrict cache_info,
    source_info;

  const char
    *hosts;

  MagickBooleanType
    returned,
    status;

  MagickSizeType
    length = 0,
    number_pixels;

  size_t
    columns,
    packet_size;

  MemoryPixelCacheTry
    memory_attempt;

  PixelCacheOpenInfo
    open_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->cache != (Cache) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (cache_anonymous_memory < 0)
    {
      char
        *value;

      /*
        Does the security policy require anonymous mapping for pixel cache?
      */
      cache_anonymous_memory=0;
      value=GetPolicyValue("pixel-cache-memory");
      if (value == (char *) NULL)
        value=GetPolicyValue("cache:memory-map");
      if (LocaleCompare(value,"anonymous") == 0)
        {
#if defined(MAGICKCORE_HAVE_MMAP) && defined(MAP_ANONYMOUS)
          cache_anonymous_memory=1;
#else
          (void) ThrowMagickException(exception,GetMagickModule(),
            MissingDelegateError,"DelegateLibrarySupportNotBuiltIn",
            "`%s' (policy requires anonymous memory mapping)",image->filename);
#endif
        }
      value=DestroyString(value);
    }
  if ((image->columns == 0) || (image->rows == 0))
    ThrowBinaryException(CacheError,"NoPixelsDefinedInCache",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  if (CheckPixelCacheGeometryLimits(image,cache_info,exception) == MagickFalse)
    return(MagickFalse);
  if (AcquireListLengthResource(image,&length,exception) == MagickFalse)
    return(MagickFalse);
  source_info=(*cache_info);
  source_info.file=(-1);
  (void) FormatLocaleString(cache_info->filename,MagickPathExtent,"%s[%.17g]",
    image->filename,(double) image->scene);
  cache_info->storage_class=image->storage_class;
  cache_info->colorspace=image->colorspace;
  cache_info->alpha_trait=image->alpha_trait;
  cache_info->channels=image->channels;
  cache_info->rows=image->rows;
  cache_info->columns=image->columns;
  status=ResetPixelChannelMap(image,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  cache_info->number_channels=GetPixelChannels(image);
  (void) memcpy(cache_info->channel_map,image->channel_map,MaxPixelChannels*
    sizeof(*image->channel_map));
  cache_info->metacontent_extent=image->metacontent_extent;
  cache_info->mode=mode;
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  packet_size=MagickMax(cache_info->number_channels,1)*sizeof(Quantum);
  if (AddMetacontentPacketSize(image,cache_info,&packet_size,exception) == MagickFalse)
    return(MagickFalse);
  if (CacheOverflowSanityCheckGetSize(number_pixels,packet_size,&length) != MagickFalse)
    {
      cache_info->storage_class=UndefinedClass;
      cache_info->length=0;
      ThrowBinaryException(ResourceLimitError,"PixelCacheAllocationFailed",
        image->filename);
    }
  columns=(size_t) (length/cache_info->rows/packet_size);
  if (IsCacheGeometryInvalid(cache_info,columns) != MagickFalse)
    {
      cache_info->storage_class=UndefinedClass;
      cache_info->length=0;
      ThrowBinaryException(ResourceLimitError,"PixelCacheAllocationFailed",
        image->filename);
    }
  cache_info->length=length;
  if (image->ping != MagickFalse)
    {
      cache_info->type=PingCache;
      return(MagickTrue);
    }
  status=AcquireMagickResource(AreaResource,(MagickSizeType)
    cache_info->columns*cache_info->rows);
  if (cache_info->mode == PersistMode)
    status=MagickFalse;
  length=number_pixels*(cache_info->number_channels*sizeof(Quantum)+
    cache_info->metacontent_extent);
  open_info.image=image;
  open_info.mode=mode;
  open_info.number_pixels=number_pixels;
  open_info.exception=exception;
  memory_attempt.status=status;
  memory_attempt.length=length;
  memory_attempt.cache_info=cache_info;
  memory_attempt.open_info=open_info;
  memory_attempt.source_info=source_info;
  if (TryMemoryPixelCache(&memory_attempt,&returned) != MagickFalse)
    return(returned);
  status=AcquireMagickResource(DiskResource,cache_info->length);
  hosts=(const char *) GetImageRegistry(StringRegistryType,"cache:hosts",
    exception);
  if ((status == MagickFalse) && (hosts != (const char *) NULL))
    {
      DistributeCacheInfo
        *server_info;

      /*
        Distribute the pixel cache to a remote server.
      */
      server_info=AcquireDistributeCacheInfo(exception);
      if (server_info != (DistributeCacheInfo *) NULL)
        {
          status=OpenDistributePixelCache(server_info,image);
          if (status == MagickFalse)
            {
              ThrowFileException(exception,CacheError,"UnableToOpenPixelCache",
                GetDistributeCacheHostname(server_info));
              server_info=DestroyDistributeCacheInfo(server_info);
            }
          else
            return(CreateDistributedPixelCache(cache_info,&open_info,
              source_info,server_info));
        }
      return(ExhaustPixelCacheResources(cache_info,&open_info,source_info));
    }
  /*
    Create pixel cache on disk.
  */
  if (status == MagickFalse)
    return(ExhaustPixelCacheResources(cache_info,&open_info,source_info));
  if (IsCacheClonePending(source_info,mode,cache_info) != MagickFalse)
    {
      (void) ClosePixelCacheOnDisk(cache_info);
      *cache_info->cache_filename='\0';
    }
  if (OpenPixelCacheOnDisk(cache_info,mode) == MagickFalse)
    return(FailPixelCacheOnDisk(cache_info,&open_info,source_info,
      "UnableToOpenPixelCache"));
  status=SetPixelCacheExtent(image,(MagickSizeType) cache_info->offset+
    cache_info->length);
  if (status == MagickFalse)
    return(FailPixelCacheOnDisk(cache_info,&open_info,source_info,
      "UnableToExtendCache"));
  cache_info->type=DiskCache;
  length=number_pixels*(cache_info->number_channels*sizeof(Quantum)+
    cache_info->metacontent_extent);
  if (length == (MagickSizeType) ((size_t) length))
    {
      status=AcquireMagickResource(MapResource,cache_info->length);
      if (status != MagickFalse)
        {
          cache_info->pixels=(Quantum *) MapBlob(cache_info->file,mode,
            cache_info->offset,(size_t) cache_info->length);
          if (cache_info->pixels == (Quantum *) NULL)
            {
              cache_info->mapped=source_info.mapped;
              cache_info->pixels=source_info.pixels;
              RelinquishMagickResource(MapResource,cache_info->length);
            }
          else
            return(CreateMappedPixelCache(cache_info,&open_info,source_info,
              status));
        }
    }
  status=MagickTrue;
  if (IsCacheSourcePending(source_info,mode) != MagickFalse)
    {
      status=ClonePixelCacheRepository(cache_info,&source_info,exception);
      RelinquishPixelCachePixels(&source_info);
    }
  LogDiskPixelCacheOpen(cache_info);
  if (status == 0)
    {
      cache_info->type=UndefinedCache;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static CacheInfo *AcquirePersistentPixelCache(
  CacheInfo *magick_restrict cache_info,const char *filename,
  const MagickOffsetType *offset)
{
  CacheInfo
    *magick_restrict clone_info;

  clone_info=(CacheInfo *) ClonePixelCache(cache_info);
  clone_info->type=DiskCache;
  (void) CopyMagickString(clone_info->cache_filename,filename,MagickPathExtent);
  clone_info->file=(-1);
  clone_info->storage_class=cache_info->storage_class;
  clone_info->colorspace=cache_info->colorspace;
  clone_info->alpha_trait=cache_info->alpha_trait;
  clone_info->channels=cache_info->channels;
  clone_info->columns=cache_info->columns;
  clone_info->rows=cache_info->rows;
  clone_info->number_channels=cache_info->number_channels;
  clone_info->metacontent_extent=cache_info->metacontent_extent;
  clone_info->mode=PersistMode;
  clone_info->length=cache_info->length;
  (void) memcpy(clone_info->channel_map,cache_info->channel_map,
    MaxPixelChannels*sizeof(*cache_info->channel_map));
  clone_info->offset=(*offset);
  return(clone_info);
}

#endif
