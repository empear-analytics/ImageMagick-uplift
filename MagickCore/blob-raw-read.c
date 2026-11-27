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
%                             RRRR    AAA   W   W                             %
%                             R   R  A   A  W   W                             %
%                             RRRR   AAAAA  W W W                             %
%                             R R    A   A  WW WW                             %
%                             R  R   A   A  W   W                             %
%                                                                             %
%                         RRRR   EEEEE   AAA   DDDD                           %
%                         R   R  E      A   A  D   D                          %
%                         RRRR   EEE    AAAAA  D   D                          %
%                         R R    E      A   A  D   D                          %
%                         R  R   EEEEE  A   A  DDDD                           %
%                                                                             %
%                                                                             %
%                       MagickCore Raw Blob Read Method                       %
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
