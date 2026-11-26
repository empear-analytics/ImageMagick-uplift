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
%                       QQQ   U   U  EEEEE  RRRR   Y   Y                      %
%                      Q   Q  U   U  E      R   R   Y Y                       %
%                      Q   Q  U   U  EEE    RRRR     Y                        %
%                      Q  QQ  U   U  E      R R      Y                        %
%                       QQQQ   UUU   EEEEE  R  R     Y                        %
%                                                                             %
%                                                                             %
%                MagickCore Blob Query and Positioning Methods                %
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
+  E O F B l o b                                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EOFBlob() returns a non-zero value when EOF has been detected reading from
%  a blob or file.
%
%  The format of the EOFBlob method is:
%
%      int EOFBlob(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport int EOFBlob(const Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  blob_info=image->blob;
  switch (blob_info->type)
  {
    case UndefinedStream:
    case StandardStream:
      break;
    case FileStream:
    case PipeStream:
    {
      blob_info->eof=feof(blob_info->file_info.file) != 0 ? MagickTrue :
        MagickFalse;
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
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

      status=0;
      (void) BZ2_bzerror(blob_info->file_info.bzfile,&status);
      if (status == BZ_UNEXPECTED_EOF)
        blob_info->eof=MagickTrue;
#endif
      break;
    }
    case FifoStream:
    {
      blob_info->eof=MagickFalse;
      break;
    }
    case BlobStream:
      break;
    case CustomStream:
      break;
  }
  return((int) blob_info->eof);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  E r r o r B l o b                                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ErrorBlob() returns a non-zero value when an error has been detected reading
%  from a blob or file.
%
%  The format of the ErrorBlob method is:
%
%      int ErrorBlob(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport int ErrorBlob(const Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  blob_info=image->blob;
  switch (blob_info->type)
  {
    case UndefinedStream:
    case StandardStream:
      break;
    case FileStream:
    case PipeStream:
    {
      blob_info->error=ferror(blob_info->file_info.file);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      (void) gzerror(blob_info->file_info.gzfile,&blob_info->error);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(MAGICKCORE_BZLIB_DELEGATE)
      (void) BZ2_bzerror(blob_info->file_info.bzfile,&blob_info->error);
#endif
      break;
    }
    case FifoStream:
    {
      blob_info->error=0;
      break;
    }
    case BlobStream:
      break;
    case CustomStream:
      break;
  }
  return(blob_info->error);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t B l o b F i l e H a n d l e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobFileHandle() returns the stdio file handle associated with the image
%  blob. If the blob is not backed by a FILE object, NULL is returned.
%
%  The format of the GetBlobFile method is:
%
%      FILE *GetBlobFileHandle(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport FILE *GetBlobFileHandle(const Image *image)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->signature == MagickCoreSignature);
  switch (image->blob->type)
  {
    case StandardStream:
    case FileStream:
    case PipeStream:
     return(image->blob->file_info.file);
    case UndefinedStream:
    case ZipStream:
    case BZipStream:
    case FifoStream:
    case BlobStream:
    case CustomStream:
      break;
  }
  return((FILE *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  G e t B l o b S i z e                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobSize() returns the current length of the image file or blob; zero is
%  returned if the size cannot be determined.
%
%  The format of the GetBlobSize method is:
%
%      MagickSizeType GetBlobSize(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickSizeType GetBlobSize(const Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  MagickSizeType
    extent;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blob_info=image->blob;
  extent=0;
  switch (blob_info->type)
  {
    case UndefinedStream:
    case StandardStream:
    {
      extent=blob_info->size;
      break;
    }
    case FileStream:
    {
      int
        file_descriptor;

      extent=(MagickSizeType) blob_info->properties.st_size;
      if (extent == 0)
        extent=blob_info->size;
      file_descriptor=fileno(blob_info->file_info.file);
      if (file_descriptor == -1)
        break;
      if (fstat(file_descriptor,&blob_info->properties) == 0)
        extent=(MagickSizeType) blob_info->properties.st_size;
      break;
    }
    case PipeStream:
    {
      extent=blob_info->size;
      break;
    }
    case ZipStream:
    case BZipStream:
    {
      MagickBooleanType
        status;

      status=GetPathAttributes(image->filename,&blob_info->properties);
      if (status != MagickFalse)
        extent=(MagickSizeType) blob_info->properties.st_size;
      break;
    }
    case FifoStream:
      break;
    case BlobStream:
    {
      extent=(MagickSizeType) blob_info->length;
      break;
    }
    case CustomStream:
    {
      if ((blob_info->custom_stream->teller != (CustomStreamTeller) NULL) &&
          (blob_info->custom_stream->seeker != (CustomStreamSeeker) NULL))
        {
          MagickOffsetType
            offset;

          offset=blob_info->custom_stream->teller(
            blob_info->custom_stream->data);
          extent=(MagickSizeType) blob_info->custom_stream->seeker(0,SEEK_END,
            blob_info->custom_stream->data);
          (void) blob_info->custom_stream->seeker(offset,SEEK_SET,
            blob_info->custom_stream->data);
        }
      break;
    }
  }
  return(extent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s B l o b S e e k a b l e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsBlobSeekable() returns true if the blob is seekable.
%
%  The format of the IsBlobSeekable method is:
%
%       MagickBooleanType IsBlobSeekable(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType IsBlobSeekable(const Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blob_info=image->blob;
  switch (blob_info->type)
  {
    case BlobStream:
      return(MagickTrue);
    case FileStream:
    {
      int
        status;

      if (blob_info->file_info.file == (FILE *) NULL)
        return(MagickFalse);
      status=fseek(blob_info->file_info.file,0,SEEK_CUR);
      return(status == -1 ? MagickFalse : MagickTrue);
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      MagickOffsetType
        offset;

      if (blob_info->file_info.gzfile == (gzFile) NULL)
        return(MagickFalse);
      offset=gzseek(blob_info->file_info.gzfile,0,SEEK_CUR);
      return(offset < 0 ? MagickFalse : MagickTrue);
#else
      break;
#endif
    }
    case UndefinedStream:
    case BZipStream:
    case FifoStream:
    case PipeStream:
    case StandardStream:
      break;
    case CustomStream:
    {
      if ((blob_info->custom_stream->seeker != (CustomStreamSeeker) NULL) &&
          (blob_info->custom_stream->teller != (CustomStreamTeller) NULL))
        return(MagickTrue);
      break;
    }
    default:
      break;
  }
  return(MagickFalse);
}

