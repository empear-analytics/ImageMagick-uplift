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

  MagickCore private methods of cache.c: the pixel and metacontent readers,
  ReadPixelCacheRegion to ReadPixelCachePixels; included by that file only.
*/
#ifndef MAGICKCORE_CACHE_READ_PRIVATE_H
#define MAGICKCORE_CACHE_READ_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e a d P i x e l C a c h e M e t a c o n t e n t                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPixelCacheMetacontent() reads metacontent from the specified region of
%  the pixel cache.
%
%  The format of the ReadPixelCacheMetacontent() method is:
%
%      MagickBooleanType ReadPixelCacheMetacontent(CacheInfo *cache_info,
%        NexusInfo *nexus_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: the pixel cache.
%
%    o nexus_info: the cache nexus to read the metacontent.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickOffsetType ReadPixelCacheRegion(
  const CacheInfo *magick_restrict cache_info,const MagickOffsetType offset,
  const MagickSizeType length,unsigned char *magick_restrict buffer)
{
  MagickOffsetType
    i;

  ssize_t
    count = 0;

  if (lseek(cache_info->file,offset,SEEK_SET) < 0)
    return((MagickOffsetType) -1);
  for (i=0; i < (MagickOffsetType) length; i+=count)
  {
    count=MagickRead(cache_info->file,buffer+i,(size_t) MagickMin(length-
      (MagickSizeType) i,(size_t) MagickMaxBufferExtent));
    if (count <= 0)
      break;
  }
  return(i);
}

static MagickBooleanType LockPixelCacheOnDisk(CacheInfo *cache_info,
  ExceptionInfo *exception)
{
  LockSemaphoreInfo(cache_info->file_semaphore);
  if (OpenPixelCacheOnDisk(cache_info,IOMode) == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
        cache_info->cache_filename);
      UnlockSemaphoreInfo(cache_info->file_semaphore);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType ReadPixelCacheMetacontent(
  CacheInfo *magick_restrict cache_info,NexusInfo *magick_restrict nexus_info,
  ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    extent,
    length;

  ssize_t
    y;

  unsigned char
    *magick_restrict q;

  size_t
    rows;

  if (cache_info->metacontent_extent == 0)
    return(MagickFalse);
  if (nexus_info->authentic_pixel_cache != MagickFalse)
    return(MagickTrue);
  if (IsValidPixelOffset(nexus_info->region.y,cache_info->columns) == MagickFalse)
    return(MagickFalse);
  offset=nexus_info->region.y*(MagickOffsetType) cache_info->columns+
    nexus_info->region.x;
  length=(MagickSizeType) nexus_info->region.width*
    cache_info->metacontent_extent;
  extent=length*nexus_info->region.height;
  rows=nexus_info->region.height;
  y=0;
  q=(unsigned char *) nexus_info->metacontent;
  switch (cache_info->type)
  {
    case MemoryCache:
    case MapCache:
    {
      unsigned char
        *magick_restrict p;

      /*
        Read meta-content from memory.
      */
      if ((cache_info->columns == nexus_info->region.width) &&
          (extent == (MagickSizeType) ((size_t) extent)))
        {
          length=extent;
          rows=1UL;
        }
      p=(unsigned char *) cache_info->metacontent+offset*(MagickOffsetType)
        cache_info->metacontent_extent;
      for (y=0; y < (ssize_t) rows; y++)
      {
        (void) memcpy(q,p,(size_t) length);
        p+=(ptrdiff_t) (cache_info->metacontent_extent*cache_info->columns);
        q+=(ptrdiff_t) (cache_info->metacontent_extent*
          nexus_info->region.width);
      }
      break;
    }
    case DiskCache:
    {
      /*
        Read meta content from disk.
      */
      if (LockPixelCacheOnDisk(cache_info,exception) == MagickFalse)
        return(MagickFalse);
      if ((cache_info->columns == nexus_info->region.width) &&
          (extent <= MagickMaxBufferExtent))
        {
          length=extent;
          rows=1UL;
        }
      extent=(MagickSizeType) cache_info->columns*cache_info->rows;
      for (y=0; y < (ssize_t) rows; y++)
      {
        count=ReadPixelCacheRegion(cache_info,cache_info->offset+
          (MagickOffsetType) extent*(MagickOffsetType)
          cache_info->number_channels*(MagickOffsetType) sizeof(Quantum)+offset*
          (MagickOffsetType) cache_info->metacontent_extent,length,
          (unsigned char *) q);
        if (count != (MagickOffsetType) length)
          break;
        offset+=(MagickOffsetType) cache_info->columns;
        q+=(ptrdiff_t) (cache_info->metacontent_extent*
          nexus_info->region.width);
      }
      if (IsFileDescriptorLimitExceeded() != MagickFalse)
        (void) ClosePixelCacheOnDisk(cache_info);
      UnlockSemaphoreInfo(cache_info->file_semaphore);
      break;
    }
    case DistributedCache:
    {
      RectangleInfo
        region;

      /*
        Read metacontent from distributed cache.
      */
      LockSemaphoreInfo(cache_info->file_semaphore);
      region=nexus_info->region;
      if ((cache_info->columns != nexus_info->region.width) ||
          (extent > MagickMaxBufferExtent))
        region.height=1UL;
      else
        {
          length=extent;
          rows=1UL;
        }
      for (y=0; y < (ssize_t) rows; y++)
      {
        count=ReadDistributePixelCacheMetacontent((DistributeCacheInfo *)
          cache_info->server_info,&region,length,(unsigned char *) q);
        if (count != (MagickOffsetType) length)
          break;
        q+=(ptrdiff_t) (cache_info->metacontent_extent*
          nexus_info->region.width);
        region.y++;
      }
      UnlockSemaphoreInfo(cache_info->file_semaphore);
      break;
    }
    default:
      break;
  }
  if (y < (ssize_t) rows)
    {
      ThrowFileException(exception,CacheError,"UnableToReadPixelCache",
        cache_info->cache_filename);
      return(MagickFalse);
    }
  if ((cache_info->debug != MagickFalse) &&
      (CacheTick(nexus_info->region.y,cache_info->rows) != MagickFalse))
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),
      "%s[%.17gx%.17g%+.20g%+.20g]",cache_info->filename,(double)
      nexus_info->region.width,(double) nexus_info->region.height,(double)
      nexus_info->region.x,(double) nexus_info->region.y);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e a d P i x e l C a c h e P i x e l s                                   %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPixelCachePixels() reads pixels from the specified region of the pixel
%  cache.
%
%  The format of the ReadPixelCachePixels() method is:
%
%      MagickBooleanType ReadPixelCachePixels(CacheInfo *cache_info,
%        NexusInfo *nexus_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: the pixel cache.
%
%    o nexus_info: the cache nexus to read the pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType ReadPixelCachePixels(
  CacheInfo *magick_restrict cache_info,NexusInfo *magick_restrict nexus_info,
  ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    extent,
    length;

  Quantum
    *magick_restrict q;

  size_t
    number_channels,
    rows;

  ssize_t
    y;

  if (nexus_info->authentic_pixel_cache != MagickFalse)
    return(MagickTrue);
  if (IsValidPixelOffset(nexus_info->region.y,cache_info->columns) == MagickFalse)
    return(MagickFalse);
  offset=nexus_info->region.y*(MagickOffsetType) cache_info->columns;
  if ((offset/(ssize_t) cache_info->columns) != nexus_info->region.y)
    return(MagickFalse);
  offset+=nexus_info->region.x;
  number_channels=cache_info->number_channels;
  length=(MagickSizeType) number_channels*nexus_info->region.width*
    sizeof(Quantum);
  if ((length/number_channels/sizeof(Quantum)) != nexus_info->region.width)
    return(MagickFalse);
  rows=nexus_info->region.height;
  extent=length*rows;
  if ((extent == 0) || ((extent/length) != rows))
    return(MagickFalse);
  y=0;
  q=nexus_info->pixels;
  switch (cache_info->type)
  {
    case MemoryCache:
    case MapCache:
    {
      Quantum
        *magick_restrict p;

      /*
        Read pixels from memory.
      */
      if ((cache_info->columns == nexus_info->region.width) &&
          (extent == (MagickSizeType) ((size_t) extent)))
        {
          length=extent;
          rows=1UL;
        }
      p=cache_info->pixels+(MagickOffsetType) cache_info->number_channels*
        offset;
      for (y=0; y < (ssize_t) rows; y++)
      {
        (void) memcpy(q,p,(size_t) length);
        p+=(ptrdiff_t) (cache_info->number_channels*cache_info->columns);
        q+=(ptrdiff_t) (cache_info->number_channels*nexus_info->region.width);
      }
      break;
    }
    case DiskCache:
    {
      /*
        Read pixels from disk.
      */
      if (LockPixelCacheOnDisk(cache_info,exception) == MagickFalse)
        return(MagickFalse);
      if ((cache_info->columns == nexus_info->region.width) &&
          (extent <= MagickMaxBufferExtent))
        {
          length=extent;
          rows=1UL;
        }
      for (y=0; y < (ssize_t) rows; y++)
      {
        count=ReadPixelCacheRegion(cache_info,cache_info->offset+offset*
          (MagickOffsetType) cache_info->number_channels*(MagickOffsetType)
          sizeof(*q),length,(unsigned char *) q);
        if (count != (MagickOffsetType) length)
          break;
        offset+=(MagickOffsetType) cache_info->columns;
        q+=(ptrdiff_t) (cache_info->number_channels*nexus_info->region.width);
      }
      if (IsFileDescriptorLimitExceeded() != MagickFalse)
        (void) ClosePixelCacheOnDisk(cache_info);
      UnlockSemaphoreInfo(cache_info->file_semaphore);
      break;
    }
    case DistributedCache:
    {
      RectangleInfo
        region;

      /*
        Read pixels from distributed cache.
      */
      LockSemaphoreInfo(cache_info->file_semaphore);
      region=nexus_info->region;
      if ((cache_info->columns != nexus_info->region.width) ||
          (extent > MagickMaxBufferExtent))
        region.height=1UL;
      else
        {
          length=extent;
          rows=1UL;
        }
      for (y=0; y < (ssize_t) rows; y++)
      {
        count=ReadDistributePixelCachePixels((DistributeCacheInfo *)
          cache_info->server_info,&region,length,(unsigned char *) q);
        if (count != (MagickOffsetType) length)
          break;
        q+=(ptrdiff_t) (cache_info->number_channels*nexus_info->region.width);
        region.y++;
      }
      UnlockSemaphoreInfo(cache_info->file_semaphore);
      break;
    }
    default:
      break;
  }
  if (y < (ssize_t) rows)
    {
      ThrowFileException(exception,CacheError,"UnableToReadPixelCache",
        cache_info->cache_filename);
      return(MagickFalse);
    }
  if ((cache_info->debug != MagickFalse) &&
      (CacheTick(nexus_info->region.y,cache_info->rows) != MagickFalse))
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),
      "%s[%.17gx%.17g%+.20g%+.20g]",cache_info->filename,(double)
      nexus_info->region.width,(double) nexus_info->region.height,(double)
      nexus_info->region.x,(double) nexus_info->region.y);
  return(MagickTrue);
}

#endif
