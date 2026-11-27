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
%                      W   W  RRRR   IIIII  TTTTT  EEEEE                      %
%                      W   W  R   R    I      T    E                          %
%                      W W W  RRRR     I      T    EEE                        %
%                      WW WW  R R      I      T    E                          %
%                      W   W  R  R   IIIII    T    EEEEE                      %
%                                                                             %
%                                                                             %
%                      MagickCore Raw Blob Write Method                       %
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
