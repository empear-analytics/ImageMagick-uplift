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
%                         SSSSS  EEEEE  EEEEE  K   K                          %
%                         SS     E      E      K  K                           %
%                          SSS   EEE    EEE    KKK                            %
%                            SS  E      E      K  K                           %
%                         SSSSS  EEEEE  EEEEE  K   K                          %
%                                                                             %
%                                                                             %
%                    MagickCore Blob Seek and Tell Methods                    %
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
+  S e e k B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SeekBlob() sets the offset in bytes from the beginning of a blob or file
%  and returns the resulting offset.
%
%  The format of the SeekBlob method is:
%
%      MagickOffsetType SeekBlob(Image *image,const MagickOffsetType offset,
%        const int whence)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o offset:  Specifies an integer representing the offset in bytes.
%
%    o whence:  Specifies an integer representing how the offset is
%      treated relative to the beginning of the blob as follows:
%
%        SEEK_SET  Set position equal to offset bytes.
%        SEEK_CUR  Set position to current location plus offset.
%        SEEK_END  Set position to EOF plus offset.
%
*/
static inline MagickBooleanType IsBlobOffsetOverflow(
  const MagickOffsetType offset,const BlobInfo *blob_info)
{
  return((((offset > 0) && (blob_info->offset > (MAGICK_SSIZE_MAX-offset))) ||
    ((offset < 0) &&
    (blob_info->offset < (MAGICK_SSIZE_MIN-offset)))) ? MagickTrue :
    MagickFalse);
}

MagickExport MagickOffsetType SeekBlob(Image *image,
  const MagickOffsetType offset,const int whence)
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
    case PipeStream:
      return(-1);
    case FileStream:
    {
      if ((offset < 0) && (whence == SEEK_SET))
        return(-1);
      if (fseek(blob_info->file_info.file,offset,whence) < 0)
        return(-1);
      blob_info->offset=TellBlob(image);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      if (gzseek(blob_info->file_info.gzfile,(long) offset,whence) < 0)
        return(-1);
#endif
      blob_info->offset=TellBlob(image);
      break;
    }
    case BZipStream:
      return(-1);
    case FifoStream:
      return(-1);
    case BlobStream:
    {
      switch (whence)
      {
        case SEEK_SET:
        default:
        {
          if (offset < 0)
            return(-1);
          blob_info->offset=offset;
          break;
        }
        case SEEK_CUR:
        {
          if (IsBlobOffsetOverflow(offset,blob_info) != MagickFalse)
            {
              errno=EOVERFLOW;
              return(-1);
            }
          if ((blob_info->offset+offset) < 0)
            return(-1);
          blob_info->offset+=offset;
          break;
        }
        case SEEK_END:
        {
          if (((MagickOffsetType) blob_info->length+offset) < 0)
            return(-1);
          blob_info->offset=(MagickOffsetType) blob_info->length+offset;
          break;
        }
      }
      if (blob_info->offset < (MagickOffsetType) ((off_t) blob_info->length))
        {
          blob_info->eof=MagickFalse;
          break;
        }
      break;
    }
    case CustomStream:
    {
      if (blob_info->custom_stream->seeker == (CustomStreamSeeker) NULL)
        return(-1);
      blob_info->offset=blob_info->custom_stream->seeker(offset,whence,
        blob_info->custom_stream->data);
      break;
    }
  }
  return(blob_info->offset);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  T e l l B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TellBlob() obtains the current value of the blob or file position.
%
%  The format of the TellBlob method is:
%
%      MagickOffsetType TellBlob(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickOffsetType TellBlob(const Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  MagickOffsetType
    offset;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blob_info=image->blob;
  offset=(-1);
  switch (blob_info->type)
  {
    case UndefinedStream:
    case StandardStream:
      break;
    case FileStream:
    {
      offset=ftell(blob_info->file_info.file);
      break;
    }
    case PipeStream:
      break;
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      offset=(MagickOffsetType) gztell(blob_info->file_info.gzfile);
#endif
      break;
    }
    case BZipStream:
      break;
    case FifoStream:
      break;
    case BlobStream:
    {
      offset=blob_info->offset;
      break;
    }
    case CustomStream:
    {
      if (blob_info->custom_stream->teller != (CustomStreamTeller) NULL)
        offset=blob_info->custom_stream->teller(blob_info->custom_stream->data);
      break;
    }
  }
  return(offset);
}
