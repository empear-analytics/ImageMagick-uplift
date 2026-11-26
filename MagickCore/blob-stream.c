/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                         BBBB   L       OOO   BBBB                           %
%                         B   B  L      O   O  B   B                          %
%                         BBBB   L      O   O  BBBB                           %
%                         B   B  L      O   O  B   B                          %
%                         BBBB   LLLLL   OOO   BBBB                           %
%                                                                             %
%                  SSSSS  TTTTT  RRRR   EEEEE   AAA   M   M                   %
%                  SS       T    R   R  E      A   A  MM MM                   %
%                   SSS     T    RRRR   EEE    AAAAA  M M M                   %
%                     SS    T    R R    E      A   A  M   M                   %
%                  SSSSS    T    R  R   EEEEE  A   A  M   M                   %
%                                                                             %
%                                                                             %
%         MagickCore Blob Stream Open, Close, Read and Write Methods          %
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
#ifdef __VMS
#include  <types.h>
#include  <mman.h>
#endif
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/client.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#if defined(MAGICKCORE_ZLIB_DELEGATE)
#include "zlib.h"
#endif
#if defined(MAGICKCORE_BZLIB_DELEGATE)
#include "bzlib.h"
#endif

#include "MagickCore/blob-info-private.h"
#include "MagickCore/blob-io-private.h"
#include "MagickCore/blob-sync-private.h"

/*
  Forward declarations.
*/
static int
  SyncBlob(const Image *);


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  D i s c a r d B l o b B y t e s                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DiscardBlobBytes() discards bytes in a blob.
%
%  The format of the DiscardBlobBytes method is:
%
%      MagickBooleanType DiscardBlobBytes(Image *image,
%        const MagickSizeType length)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o length:  the number of bytes to skip.
%
*/
MagickExport MagickBooleanType DiscardBlobBytes(Image *image,
  const MagickSizeType length)
{
  MagickSizeType
    i;

  size_t
    quantum;

  ssize_t
    count;

  unsigned char
    buffer[MagickMinBufferExtent >> 1];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (length != (MagickSizeType) ((MagickOffsetType) length))
    return(MagickFalse);
  count=0;
  for (i=0; i < length; i+=(MagickSizeType) count)
  {
    quantum=(size_t) MagickMin(length-i,sizeof(buffer));
    (void) ReadBlobStream(image,quantum,buffer,&count);
    if (count <= 0)
      {
        if (count == 0)
          break; /* EOF is terminal, errno may hold a stale EINTR */
        count=0;
        if (errno != EINTR)
          break;
      }
  }
  return(i < (MagickSizeType) length ? MagickFalse : MagickTrue);
}




/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlob() reads data from the blob or image file and returns it.  It
%  returns the number of bytes read. If length is zero, ReadBlob() returns
%  zero and has no other results. If length is greater than MAGICK_SSIZE_MAX,
%  the result is unspecified.
%
%  The format of the ReadBlob method is:
%
%      ssize_t ReadBlob(Image *image,const size_t length,void *data)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length:  Specifies an integer representing the number of bytes to read
%      from the file.
%
%    o data:  Specifies an area to place the information requested from the
%      file.
%
*/
static ssize_t ReadBlobFile(BlobInfo *magick_restrict blob_info,
  const size_t length,unsigned char *q,ssize_t count)
{
  int
    c;

  switch (length)
  {
    default:
    {
      count=(ssize_t) fread(q,1,length,blob_info->file_info.file);
      break;
    }
    case 4:
    {
      c=getc(blob_info->file_info.file);
      if (c == EOF)
        break;
      *q++=(unsigned char) c;
      count++;
      magick_fallthrough;
    }
    case 3:
    {
      c=getc(blob_info->file_info.file);
      if (c == EOF)
        break;
      *q++=(unsigned char) c;
      count++;
      magick_fallthrough;
    }
    case 2:
    {
      c=getc(blob_info->file_info.file);
      if (c == EOF)
        break;
      *q++=(unsigned char) c;
      count++;
      magick_fallthrough;
    }
    case 1:
    {
      c=getc(blob_info->file_info.file);
      if (c == EOF)
        break;
      *q++=(unsigned char) c;
      count++;
      magick_fallthrough;
    }
    case 0:
      break;
  }
  if ((count != (ssize_t) length) &&
      (ferror(blob_info->file_info.file) != 0))
    ThrowBlobException(blob_info);
  return(count);
}

#if defined(MAGICKCORE_ZLIB_DELEGATE)
static ssize_t ReadBlobGzip(BlobInfo *magick_restrict blob_info,
  const size_t length,unsigned char *q,ssize_t count)
{
  size_t
    i;

  for (i=0; i < length; i+=(size_t) count)
  {
    count=(ssize_t) gzread(blob_info->file_info.gzfile,q+i,
      (unsigned int) MagickMin(length-i,MagickMaxBufferExtent));
    if (count <= 0)
      {
        count=0;
        if (errno != EINTR)
          break;
      }
  }
  count=(ssize_t) i;
  return(count);
}
#endif

#if defined(MAGICKCORE_BZLIB_DELEGATE)
static ssize_t ReadBlobBZip(BlobInfo *magick_restrict blob_info,
  const size_t length,unsigned char *q,ssize_t count)
{
  size_t
    i;

  for (i=0; i < length; i+=(size_t) count)
  {
    count=(ssize_t) BZ2_bzread(blob_info->file_info.bzfile,q+i,(int)
      MagickMin(length-i,MagickMaxBufferExtent));
    if (count <= 0)
      {
        count=0;
        if (errno != EINTR)
          break;
      }
  }
  count=(ssize_t) i;
  return(count);
}
#endif

MagickExport ssize_t ReadBlob(Image *image,const size_t length,void *data)
{
  BlobInfo
    *magick_restrict blob_info;

  ssize_t
    count;

  unsigned char
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  if (length == 0)
    return(0);
  assert(data != (void *) NULL);
  blob_info=image->blob;
  count=0;
  q=(unsigned char *) data;
  switch (blob_info->type)
  {
    case UndefinedStream:
      break;
    case StandardStream:
    case FileStream:
    case PipeStream:
    {
      count=ReadBlobFile(blob_info,length,q,count);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      int
        c,
        status;

      switch (length)
      {
        default:
        {
          count=ReadBlobGzip(blob_info,length,q,count);
          break;
        }
        case 4:
        {
          c=gzgetc(blob_info->file_info.gzfile);
          if (c == EOF)
            break;
          *q++=(unsigned char) c;
          count++;
          magick_fallthrough;
        }
        case 3:
        {
          c=gzgetc(blob_info->file_info.gzfile);
          if (c == EOF)
            break;
          *q++=(unsigned char) c;
          count++;
          magick_fallthrough;
        }
        case 2:
        {
          c=gzgetc(blob_info->file_info.gzfile);
          if (c == EOF)
            break;
          *q++=(unsigned char) c;
          count++;
          magick_fallthrough;
        }
        case 1:
        {
          c=gzgetc(blob_info->file_info.gzfile);
          if (c == EOF)
            break;
          *q++=(unsigned char) c;
          count++;
        }
        case 0:
          break;
      }
      status=Z_OK;
      (void) gzerror(blob_info->file_info.gzfile,&status);
      if ((count != (ssize_t) length) && (status != Z_OK))
        ThrowBlobException(blob_info);
      if (blob_info->eof == MagickFalse)
        blob_info->eof=gzeof(blob_info->file_info.gzfile) != 0 ? MagickTrue :
          MagickFalse;
#endif
      break;
    }
    case BZipStream:
    {
#if defined(MAGICKCORE_BZLIB_DELEGATE)
      int
        status;

      count=ReadBlobBZip(blob_info,length,q,count);
      status=BZ_OK;
      (void) BZ2_bzerror(blob_info->file_info.bzfile,&status);
      if ((count != (ssize_t) length) && (status != BZ_OK))
        ThrowBlobException(blob_info);
      if ((count != (ssize_t) length) && (status == BZ_OK))
        blob_info->eof=MagickTrue;
#endif
      break;
    }
    case FifoStream:
      break;
    case BlobStream:
    {
      const unsigned char
        *p;

      if (blob_info->offset >= (MagickOffsetType) blob_info->length)
        {
          blob_info->eof=MagickTrue;
          break;
        }
      p=blob_info->data+blob_info->offset;
      count=(ssize_t) MagickMin((MagickOffsetType) length,(MagickOffsetType)
        blob_info->length-blob_info->offset);
      blob_info->offset+=count;
      if (count != (ssize_t) length)
        blob_info->eof=MagickTrue;
      (void) memcpy(q,p,(size_t) count);
      break;
    }
    case CustomStream:
    {
      if (blob_info->custom_stream->reader != (CustomStreamHandler) NULL)
        {
          count=blob_info->custom_stream->reader(q,length,
            blob_info->custom_stream->data);
          if (count <= 0)
            {
              blob_info->eof=MagickTrue;
              count=0;
            }
        }
      break;
    }
  }
  return(count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S e t B l o b E x t e n t                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetBlobExtent() ensures enough space is allocated for the blob.  If the
%  method is successful, subsequent writes to bytes in the specified range are
%  guaranteed not to fail.
%
%  The format of the SetBlobExtent method is:
%
%      MagickBooleanType SetBlobExtent(Image *image,const MagickSizeType extent)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o extent:  the blob maximum extent.
%
*/
MagickExport MagickBooleanType SetBlobExtent(Image *image,
  const MagickSizeType extent)
{
  BlobInfo
    *magick_restrict blob_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blob_info=image->blob;
  switch (blob_info->type)
  {
    case UndefinedStream:
      break;
    case StandardStream:
      return(MagickFalse);
    case FileStream:
    {
      MagickOffsetType
        offset;

      ssize_t
        count;

      if (extent != (MagickSizeType) ((off_t) extent))
        return(MagickFalse);
      offset=SeekBlob(image,0,SEEK_END);
      if (offset < 0)
        return(MagickFalse);
      if ((MagickSizeType) offset >= extent)
        break;
      offset=SeekBlob(image,(MagickOffsetType) extent-1,SEEK_SET);
      if (offset < 0)
        break;
      count=(ssize_t) fwrite((const unsigned char *) "",1,1,
        blob_info->file_info.file);
#if defined(MAGICKCORE_HAVE_POSIX_FALLOCATE)
      if (blob_info->synchronize != MagickFalse)
        {
          int
            file;

          file=fileno(blob_info->file_info.file);
          if ((file == -1) || (offset < 0))
            return(MagickFalse);
          (void) posix_fallocate(file,offset,(MagickOffsetType) extent-offset);
        }
#endif
      offset=SeekBlob(image,offset,SEEK_SET);
      if (count != 1)
        return(MagickFalse);
      break;
    }
    case PipeStream:
    case ZipStream:
      return(MagickFalse);
    case BZipStream:
      return(MagickFalse);
    case FifoStream:
      return(MagickFalse);
    case BlobStream:
    {
      if (extent != (MagickSizeType) ((size_t) extent))
        return(MagickFalse);
      if (blob_info->mapped != MagickFalse)
        {
          MagickOffsetType
            offset;

          ssize_t
            count;

          (void) UnmapBlob(blob_info->data,blob_info->length);
          RelinquishMagickResource(MapResource,blob_info->length);
          if (extent != (MagickSizeType) ((off_t) extent))
            return(MagickFalse);
          offset=SeekBlob(image,0,SEEK_END);
          if (offset < 0)
            return(MagickFalse);
          if ((MagickSizeType) offset >= extent)
            break;
          offset=SeekBlob(image,(MagickOffsetType) extent-1,SEEK_SET);
          count=(ssize_t) fwrite((const unsigned char *) "",1,1,
            blob_info->file_info.file);
#if defined(MAGICKCORE_HAVE_POSIX_FALLOCATE)
          if (blob_info->synchronize != MagickFalse)
            {
              int
                file;

              file=fileno(blob_info->file_info.file);
              if ((file == -1) || (offset < 0))
                return(MagickFalse);
              (void) posix_fallocate(file,offset,(MagickOffsetType) extent-
                offset);
            }
#endif
          offset=SeekBlob(image,offset,SEEK_SET);
          if (count != 1)
            return(MagickFalse);
          (void) AcquireMagickResource(MapResource,extent);
          blob_info->data=(unsigned char*) MapBlob(fileno(
            blob_info->file_info.file),WriteMode,0,(size_t) extent);
          blob_info->extent=(size_t) extent;
          blob_info->length=(size_t) extent;
          (void) SyncBlob(image);
          break;
        }
      blob_info->extent=(size_t) extent;
      blob_info->data=(unsigned char *) ResizeQuantumMemory(blob_info->data,
        blob_info->extent+1,sizeof(*blob_info->data));
      (void) SyncBlob(image);
      if (blob_info->data == (unsigned char *) NULL)
        {
          (void) DetachBlob(blob_info);
          return(MagickFalse);
        }
      break;
    }
    case CustomStream:
      break;
  }
  return(MagickTrue);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b                                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlob() writes data to a blob or image file.  It returns the number of
%  bytes written.
%
%  The format of the WriteBlob method is:
%
%      ssize_t WriteBlob(Image *image,const size_t length,const void *data)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length:  Specifies an integer representing the number of bytes to
%      write to the file.
%
%    o data:  The address of the data to write to the blob or file.
%
*/
#if defined(MAGICKCORE_ZLIB_DELEGATE)
static ssize_t WriteBlobGzip(BlobInfo *magick_restrict blob_info,
  const size_t length,unsigned char *q,ssize_t count)
{
  size_t
    i;

  for (i=0; i < length; i+=(size_t) count)
  {
    count=(ssize_t) gzwrite(blob_info->file_info.gzfile,q+i,
      (unsigned int) MagickMin(length-i,MagickMaxBufferExtent));
    if (count <= 0)
      {
        count=0;
        if (errno != EINTR)
          break;
      }
  }
  count=(ssize_t) i;
  return(count);
}
#endif

#if defined(MAGICKCORE_BZLIB_DELEGATE)
static ssize_t WriteBlobBZip(BlobInfo *magick_restrict blob_info,
  const size_t length,unsigned char *q,ssize_t count)
{
  size_t
    i;

  for (i=0; i < length; i+=(size_t) count)
  {
    count=(ssize_t) BZ2_bzwrite(blob_info->file_info.bzfile,q+i,
      (int) MagickMin(length-i,MagickMaxBufferExtent));
    if (count <= 0)
      {
        count=0;
        if (errno != EINTR)
          break;
      }
  }
  count=(ssize_t) i;
  return(count);
}
#endif

MagickExport ssize_t WriteBlob(Image *image,const size_t length,
  const void *data)
{
  BlobInfo
    *magick_restrict blob_info;

  int
    c;

  const unsigned char
    *p;

  unsigned char
    *q;

  ssize_t
    count;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  if (length == 0)
    return(0);
  assert(data != (const void *) NULL);
  blob_info=image->blob;
  count=0;
  p=(const unsigned char *) data;
  q=(unsigned char *) data;
  switch (blob_info->type)
  {
    case UndefinedStream:
      break;
    case StandardStream:
    case FileStream:
    case PipeStream:
    {
      switch (length)
      {
        default:
        {
          count=(ssize_t) fwrite((const char *) data,1,length,
            blob_info->file_info.file);
          break;
        }
        case 4:
        {
          c=putc((int) *p++,blob_info->file_info.file);
          if (c == EOF)
            break;
          count++;
          magick_fallthrough;
        }
        case 3:
        {
          c=putc((int) *p++,blob_info->file_info.file);
          if (c == EOF)
            break;
          count++;
          magick_fallthrough;
        }
        case 2:
        {
          c=putc((int) *p++,blob_info->file_info.file);
          if (c == EOF)
            break;
          count++;
          magick_fallthrough;
        }
        case 1:
        {
          c=putc((int) *p++,blob_info->file_info.file);
          if (c == EOF)
            break;
          count++;
          magick_fallthrough;
        }
        case 0:
          break;
      }
      if ((count != (ssize_t) length) &&
          (ferror(blob_info->file_info.file) != 0))
        ThrowBlobException(blob_info);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      int
        status;

      switch (length)
      {
        default:
        {
          count=WriteBlobGzip(blob_info,length,q,count);
          break;
        }
        case 4:
        {
          c=gzputc(blob_info->file_info.gzfile,(int) *p++);
          if (c == EOF)
            break;
          count++;
          magick_fallthrough;
        }
        case 3:
        {
          c=gzputc(blob_info->file_info.gzfile,(int) *p++);
          if (c == EOF)
            break;
          count++;
          magick_fallthrough;
        }
        case 2:
        {
          c=gzputc(blob_info->file_info.gzfile,(int) *p++);
          if (c == EOF)
            break;
          count++;
          magick_fallthrough;
        }
        case 1:
        {
          c=gzputc(blob_info->file_info.gzfile,(int) *p++);
          if (c == EOF)
            break;
          count++;
          magick_fallthrough;
        }
        case 0:
          break;
      }
      status=Z_OK;
      (void) gzerror(blob_info->file_info.gzfile,&status);
      if ((count != (ssize_t) length) && (status != Z_OK))
        ThrowBlobException(blob_info);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(MAGICKCORE_BZLIB_DELEGATE)
      int
        status;

      count=WriteBlobBZip(blob_info,length,q,count);
      status=BZ_OK;
      (void) BZ2_bzerror(blob_info->file_info.bzfile,&status);
      if ((count != (ssize_t) length) && (status != BZ_OK))
        ThrowBlobException(blob_info);
#endif
      break;
    }
    case FifoStream:
    {
      count=(ssize_t) blob_info->stream(image,data,length);
      break;
    }
    case BlobStream:
    {
      MagickSizeType
        extent;

      if (blob_info->offset > (MagickOffsetType) (MAGICK_SSIZE_MAX-length))
        {
          errno=EOVERFLOW;
          return(0);
        }
      extent=(MagickSizeType) (blob_info->offset+(MagickOffsetType) length);
      if (extent >= blob_info->extent)
        {
          extent+=blob_info->quantum+length;
          blob_info->quantum<<=1;
          if (SetBlobExtent(image,extent) == MagickFalse)
            return(0);
        }
      q=blob_info->data+blob_info->offset;
      (void) memcpy(q,p,length);
      blob_info->offset+=(MagickOffsetType) length;
      if (blob_info->offset >= (MagickOffsetType) blob_info->length)
        blob_info->length=(size_t) blob_info->offset;
      count=(ssize_t) length;
      break;
    }
    case CustomStream:
    {
      if (blob_info->custom_stream->writer != (CustomStreamHandler) NULL)
        count=blob_info->custom_stream->writer((unsigned char *) data,
          length,blob_info->custom_stream->data);
      break;
    }
  }
  return(count);
}
