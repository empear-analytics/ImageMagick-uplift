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
%                                                                             %
%                     MagickCore Binary Large OBjectS Methods                 %
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
+   A t t a c h B l o b                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AttachBlob() attaches a blob to the BlobInfo structure.
%
%  The format of the AttachBlob method is:
%
%      void AttachBlob(BlobInfo *blob_info,const void *blob,const size_t length)
%
%  A description of each parameter follows:
%
%    o blob_info: Specifies a pointer to a BlobInfo structure.
%
%    o blob: the address of a character stream in one of the image formats
%      understood by ImageMagick.
%
%    o length: This size_t integer reflects the length in bytes of the blob.
%
*/
MagickExport void AttachBlob(BlobInfo *blob_info,const void *blob,
  const size_t length)
{
  assert(blob_info != (BlobInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  blob_info->length=length;
  blob_info->extent=length;
  blob_info->quantum=(size_t) MagickMaxBlobExtent;
  blob_info->offset=0;
  blob_info->type=BlobStream;
  blob_info->file_info.file=(FILE *) NULL;
  blob_info->data=(unsigned char *) blob;
  blob_info->mapped=MagickFalse;
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o n e B l o b I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneBlobInfo() makes a duplicate of the given blob info structure, or if
%  blob info is NULL, a new one.
%
%  The format of the CloneBlobInfo method is:
%
%      BlobInfo *CloneBlobInfo(const BlobInfo *blob_info)
%
%  A description of each parameter follows:
%
%    o blob_info: the blob info.
%
*/
MagickExport BlobInfo *CloneBlobInfo(const BlobInfo *blob_info)
{
  BlobInfo
    *clone_info;

  SemaphoreInfo
    *semaphore;

  clone_info=(BlobInfo *) AcquireCriticalMemory(sizeof(*clone_info));
  GetBlobInfo(clone_info);
  if (blob_info == (BlobInfo *) NULL)
    return(clone_info);
  semaphore=clone_info->semaphore;
  (void) memcpy(clone_info,blob_info,sizeof(*clone_info));
  if (blob_info->mapped != MagickFalse)
    (void) AcquireMagickResource(MapResource,blob_info->length);
  clone_info->semaphore=semaphore;
  LockSemaphoreInfo(clone_info->semaphore);
  clone_info->reference_count=1;
  UnlockSemaphoreInfo(clone_info->semaphore);
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o s e B l o b                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloseBlob() closes a stream associated with the image.
%
%  The format of the CloseBlob method is:
%
%      MagickBooleanType CloseBlob(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/

static inline void ThrowBlobException(BlobInfo *blob_info)
{
  if ((blob_info->status == 0) && (errno != 0))
    blob_info->error_number=errno;
  blob_info->status=(-1);
}

static inline MagickBooleanType IsWrittenBlobFailure(const int status,
  const BlobInfo *blob_info)
{
  return(((status != 0) &&
    ((blob_info->mode == WriteBlobMode) ||
    (blob_info->mode == WriteBinaryBlobMode) ||
    (blob_info->mode == AppendBlobMode) ||
    (blob_info->mode == AppendBinaryBlobMode))) ? MagickTrue : MagickFalse);
}

static void CloseBlobFile(BlobInfo *blob_info)
{
  int
    status;

  if (blob_info->file_info.file != (FILE *) NULL)
    {
      status=fclose(blob_info->file_info.file);
      if (status != 0)
        ThrowBlobException(blob_info);
    }
}

static void SynchronizeBlobFile(BlobInfo *blob_info,int status)
{
  if (blob_info->synchronize != MagickFalse)
    {
      status=fflush(blob_info->file_info.file);
      if (status != 0)
        ThrowBlobException(blob_info);
      status=fsync(fileno(blob_info->file_info.file));
      if (status != 0)
        ThrowBlobException(blob_info);
    }
  if ((status != 0) && (ferror(blob_info->file_info.file) != 0))
    ThrowBlobException(blob_info);
}

MagickExport MagickBooleanType CloseBlob(Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  int
    status;

  /*
    Close image file.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blob_info=image->blob;
  if ((blob_info == (BlobInfo *) NULL) || (blob_info->type == UndefinedStream))
    return(MagickTrue);
  status=SyncBlob(image);
  /* Some platforms report an error when a read-only stream is flushed. */
  if (IsWrittenBlobFailure(status,blob_info) != MagickFalse)
    ThrowBlobException(blob_info);
  status=blob_info->status;
  switch (blob_info->type)
  {
    case UndefinedStream:
      break;
    case StandardStream:
    case FileStream:
    case PipeStream:
    {
      SynchronizeBlobFile(blob_info,status);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      status=Z_OK;
      (void) gzerror(blob_info->file_info.gzfile,&status);
      if (status != Z_OK)
        ThrowBlobException(blob_info);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(MAGICKCORE_BZLIB_DELEGATE)
      status=BZ_OK;
      (void) BZ2_bzerror(blob_info->file_info.bzfile,&status);
      if (status != BZ_OK)
        ThrowBlobException(blob_info);
#endif
      break;
    }
    case FifoStream:
      break;
    case BlobStream:
    {
      if (blob_info->file_info.file != (FILE *) NULL)
        SynchronizeBlobFile(blob_info,status);
      break;
    }
    case CustomStream:
      break;
  }
  blob_info->size=GetBlobSize(image);
  image->extent=blob_info->size;
  blob_info->eof=MagickFalse;
  blob_info->error=0;
  blob_info->mode=UndefinedBlobMode;
  if (blob_info->exempt != MagickFalse)
    {
      blob_info->type=UndefinedStream;
      return(blob_info->status != 0 ? MagickFalse : MagickTrue);
    }
  switch (blob_info->type)
  {
    case UndefinedStream:
    case StandardStream:
      break;
    case FileStream:
    {
      CloseBlobFile(blob_info);
      break;
    }
    case PipeStream:
    {
#if defined(MAGICKCORE_HAVE_PCLOSE)
      status=pclose(blob_info->file_info.file);
      if (status != 0)
        ThrowBlobException(blob_info);
#endif
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      status=gzclose(blob_info->file_info.gzfile);
      if (status != Z_OK)
        ThrowBlobException(blob_info);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(MAGICKCORE_BZLIB_DELEGATE)
      BZ2_bzclose(blob_info->file_info.bzfile);
#endif
      break;
    }
    case FifoStream:
      break;
    case BlobStream:
    {
      CloseBlobFile(blob_info);
      break;
    }
    case CustomStream:
      break;
  }
  (void) DetachBlob(blob_info);
  return(blob_info->status != 0 ? MagickFalse : MagickTrue);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y B l o b                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyBlob() deallocates memory associated with a blob.
%
%  The format of the DestroyBlob method is:
%
%      void DestroyBlob(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void DestroyBlob(Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  MagickBooleanType
    destroy;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blob_info=image->blob;
  destroy=MagickFalse;
  LockSemaphoreInfo(blob_info->semaphore);
  blob_info->reference_count--;
  assert(blob_info->reference_count >= 0);
  if (blob_info->reference_count == 0)
    destroy=MagickTrue;
  UnlockSemaphoreInfo(blob_info->semaphore);
  if (destroy == MagickFalse)
    {
      image->blob=(BlobInfo *) NULL;
      return;
    }
  (void) CloseBlob(image);
  if (blob_info->mapped != MagickFalse)
    {
      (void) UnmapBlob(blob_info->data,blob_info->length);
      RelinquishMagickResource(MapResource,blob_info->length);
    }
  if (blob_info->semaphore != (SemaphoreInfo *) NULL)
    RelinquishSemaphoreInfo(&blob_info->semaphore);
  blob_info->signature=(~MagickCoreSignature);
  image->blob=(BlobInfo *) RelinquishMagickMemory(blob_info);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e t a c h B l o b                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DetachBlob() detaches a blob from the BlobInfo structure.
%
%  The format of the DetachBlob method is:
%
%      void *DetachBlob(BlobInfo *blob_info)
%
%  A description of each parameter follows:
%
%    o blob_info: Specifies a pointer to a BlobInfo structure.
%
*/
MagickExport void *DetachBlob(BlobInfo *blob_info)
{
  void
    *data;

  assert(blob_info != (BlobInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (blob_info->mapped != MagickFalse)
    {
      (void) UnmapBlob(blob_info->data,blob_info->length);
      blob_info->data=NULL;
      RelinquishMagickResource(MapResource,blob_info->length);
    }
  blob_info->mapped=MagickFalse;
  blob_info->length=0;
  /*
    We should not reset blob_info->extent because we use it to check if the
    blob was opened inside ImagesToBlob and ImagesToBlob.
  */
  blob_info->offset=0;
  blob_info->mode=UndefinedBlobMode;
  blob_info->eof=MagickFalse;
  blob_info->error=0;
  blob_info->exempt=MagickFalse;
  blob_info->type=UndefinedStream;
  blob_info->file_info.file=(FILE *) NULL;
  data=blob_info->data;
  blob_info->data=(unsigned char *) NULL;
  blob_info->stream=(StreamHandler) NULL;
  blob_info->custom_stream=(CustomStreamInfo *) NULL;
  return(data);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D i s a s s o c i a t e B l o b                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DisassociateBlob() disassociates the image stream.  It checks if the
%  blob of the specified image is referenced by other images. If the reference
%  count is higher then 1 a new blob is assigned to the specified image.
%
%  The format of the DisassociateBlob method is:
%
%      void DisassociateBlob(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/

static inline void ResetDisassociatedBlobStorage(BlobInfo *blob_info)
{
  assert(blob_info != (BlobInfo *) NULL);
  assert(blob_info->signature == MagickCoreSignature);
  if (blob_info->mapped != MagickFalse)
    {
      RelinquishMagickResource(MapResource,blob_info->length);
      blob_info->mapped=MagickFalse;
    }
  blob_info->data=(unsigned char *) NULL;
  blob_info->length=0;
  blob_info->offset=0;
  blob_info->mode=UndefinedBlobMode;
  blob_info->type=UndefinedStream;
}

MagickExport void DisassociateBlob(Image *image)
{
  BlobInfo
    *magick_restrict blob_info,
    *clone_info;

  MagickBooleanType
    clone;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  blob_info=image->blob;
  clone=MagickFalse;
  LockSemaphoreInfo(blob_info->semaphore);
  assert(blob_info->reference_count >= 0);
  if (blob_info->reference_count > 1)
    clone=MagickTrue;
  UnlockSemaphoreInfo(blob_info->semaphore);
  if (clone == MagickFalse)
    return;
  clone_info=CloneBlobInfo(blob_info);
  ResetDisassociatedBlobStorage(clone_info);
  DestroyBlob(image);
  image->blob=clone_info;
}

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
+   D u p l i c a t e s B l o b                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DuplicateBlob() duplicates a blob descriptor.
%
%  The format of the DuplicateBlob method is:
%
%      void DuplicateBlob(Image *image,const Image *duplicate)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o duplicate: the duplicate image.
%
*/
MagickExport void DuplicateBlob(Image *image,const Image *duplicate)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(duplicate != (Image *) NULL);
  assert(duplicate->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  DestroyBlob(image);
  image->blob=ReferenceBlob(duplicate->blob);
}

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
%   F i l e T o I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToImage() write the contents of a file to an image.
%
%  The format of the FileToImage method is:
%
%      MagickBooleanType FileToImage(Image *,const char *filename)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o filename: the filename.
%
*/

static inline ssize_t WriteBlobStream(Image *image,const size_t length,
  const void *magick_restrict data)
{
  BlobInfo
    *magick_restrict blob_info;

  MagickSizeType
    extent;

  unsigned char
    *magick_restrict q;

  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  assert(data != NULL);
  blob_info=image->blob;
  if (blob_info->type != BlobStream)
    return(WriteBlob(image,length,(const unsigned char *) data));
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
  (void) memcpy(q,data,length);
  blob_info->offset+=(MagickOffsetType) length;
  if (blob_info->offset >= (MagickOffsetType) blob_info->length)
    blob_info->length=(size_t) blob_info->offset;
  return((ssize_t) length);
}

MagickExport MagickBooleanType FileToImage(Image *image,const char *filename,
  ExceptionInfo *exception)
{
  int
    file;

  size_t
    length,
    quantum;

  ssize_t
    count;

  struct stat
    file_stats;

  unsigned char
    *blob;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(filename != (const char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  if (IsPathAuthorized(ReadPolicyRights,filename) == MagickFalse)
    ThrowPolicyException(filename,MagickFalse);
  file=fileno(stdin);
  if (LocaleCompare(filename,"-") != 0)
    {
      int
        flags = O_RDONLY | O_CLOEXEC | O_BINARY;

      file=open_utf8(filename,flags,0);
    }
  if (file == -1)
    {
      ThrowFileException(exception,BlobError,"UnableToOpenBlob",filename);
      return(MagickFalse);
    }
  if (IsPathAuthorized(ReadPolicyRights,filename) == MagickFalse)
    ThrowPolicyException(filename,MagickFalse);
  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(file,&file_stats) == 0) && (file_stats.st_size > 0))
    quantum=(size_t) MagickMin(file_stats.st_size,MagickMaxBufferExtent);
  blob=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*blob));
  if (blob == (unsigned char *) NULL)
    {
      file=close_utf8(file);
      ThrowFileException(exception,ResourceLimitError,"MemoryAllocationFailed",
        filename);
      return(MagickFalse);
    }
  for ( ; ; )
  {
    count=MagickRead(file,blob,quantum);
    if (count <= 0)
      break;
    length=(size_t) count;
    count=WriteBlobStream(image,length,blob);
    if (count != (ssize_t) length)
      {
        ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
        break;
      }
  }
  file=close_utf8(file);
  if (file == -1)
    ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
  blob=(unsigned char *) RelinquishMagickMemory(blob);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t B l o b E r r o r                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobError() returns MagickTrue if the blob associated with the specified
%  image encountered an error.
%
%  The format of the GetBlobError method is:
%
%       MagickBooleanType GetBlobError(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType GetBlobError(const Image *image)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->blob->status != 0) && (image->blob->error_number != 0))
    errno=image->blob->error_number;
  return(image->blob->status == 0 ? MagickFalse : MagickTrue);
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
+   G e t B l o b I n f o                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobInfo() initializes the BlobInfo structure.
%
%  The format of the GetBlobInfo method is:
%
%      void GetBlobInfo(BlobInfo *blob_info)
%
%  A description of each parameter follows:
%
%    o blob_info: Specifies a pointer to a BlobInfo structure.
%
*/
MagickExport void GetBlobInfo(BlobInfo *blob_info)
{
  assert(blob_info != (BlobInfo *) NULL);
  (void) memset(blob_info,0,sizeof(*blob_info));
  blob_info->type=UndefinedStream;
  blob_info->quantum=(size_t) MagickMaxBlobExtent;
  blob_info->properties.st_mtime=GetMagickTime();
  blob_info->properties.st_ctime=blob_info->properties.st_mtime;
  blob_info->debug=GetLogEventMask() & BlobEvent ? MagickTrue : MagickFalse;
  blob_info->reference_count=1;
  blob_info->semaphore=AcquireSemaphoreInfo();
  blob_info->signature=MagickCoreSignature;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t B l o b P r o p e r t i e s                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobProperties() returns information about an image blob.
%
%  The format of the GetBlobProperties method is:
%
%      const struct stat *GetBlobProperties(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport const struct stat *GetBlobProperties(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(&image->blob->properties);
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
+   G e t B l o b S t r e a m D a t a                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobStreamData() returns the stream data for the image.
%
%  The format of the GetBlobStreamData method is:
%
%      void *GetBlobStreamData(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void *GetBlobStreamData(const Image *image)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  return(image->blob->data);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t B l o b S t r e a m H a n d l e r                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobStreamHandler() returns the stream handler for the image.
%
%  The format of the GetBlobStreamHandler method is:
%
%      StreamHandler GetBlobStreamHandler(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport StreamHandler GetBlobStreamHandler(const Image *image)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(image->blob->stream);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n j e c t I m a g e B l o b                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InjectImageBlob() injects the image with a copy of itself in the specified
%  format (e.g. inject JPEG into a PDF image).
%
%  The format of the InjectImageBlob method is:
%
%      MagickBooleanType InjectImageBlob(const ImageInfo *image_info,
%        Image *image,Image *inject_image,const char *format,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info..
%
%    o image: the image.
%
%    o inject_image: inject into the image stream.
%
%    o format: the image format.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType InjectImageBlob(const ImageInfo *image_info,
  Image *image,Image *inject_image,const char *format,ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  FILE
    *unique_file;

  Image
    *byte_image;

  ImageInfo
    *write_info;

  int
    file;

  MagickBooleanType
    status;

  size_t
    quantum;

  struct stat
    file_stats;

  unsigned char
    *buffer;

  /*
    Write inject image to a temporary file.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(inject_image != (Image *) NULL);
  assert(inject_image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  unique_file=(FILE *) NULL;
  file=AcquireUniqueFileResource(filename);
  if (file != -1)
    unique_file=fdopen(file,"wb");
  if ((file == -1) || (unique_file == (FILE *) NULL))
    {
      (void) CopyMagickString(image->filename,filename,MagickPathExtent);
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image->filename);
      return(MagickFalse);
    }
  byte_image=CloneImage(inject_image,0,0,MagickFalse,exception);
  if (byte_image == (Image *) NULL)
    {
      (void) fclose(unique_file);
      (void) RelinquishUniqueFileResource(filename);
      return(MagickFalse);
    }
  (void) FormatLocaleString(byte_image->filename,MagickPathExtent,"%s:%s",
    format,filename);
  DestroyBlob(byte_image);
  byte_image->blob=CloneBlobInfo((BlobInfo *) NULL);
  write_info=CloneImageInfo(image_info);
  SetImageInfoFile(write_info,unique_file);
  status=WriteImage(write_info,byte_image,exception);
  write_info=DestroyImageInfo(write_info);
  byte_image=DestroyImage(byte_image);
  (void) fclose(unique_file);
  if (status == MagickFalse)
    {
      (void) RelinquishUniqueFileResource(filename);
      return(MagickFalse);
    }
  /*
    Inject into image stream.
  */
  file=open_utf8(filename,O_RDONLY | O_CLOEXEC | O_BINARY,0);
  if (file == -1)
    {
      (void) RelinquishUniqueFileResource(filename);
      ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
        image_info->filename);
      return(MagickFalse);
    }
  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(file,&file_stats) == 0) && (file_stats.st_size > 0))
    quantum=(size_t) MagickMin(file_stats.st_size,MagickMaxBufferExtent);
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      (void) RelinquishUniqueFileResource(filename);
      file=close_utf8(file);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  for ( ; ; )
  {
    ssize_t count = read(file,buffer,quantum);
    if (count <= 0)
      {
        count=0;
        if (errno != EINTR)
          break;
      }
    status=WriteBlobStream(image,(size_t) count,buffer) == count ? MagickTrue :
      MagickFalse;
  }
  file=close_utf8(file);
  if (file == -1)
    ThrowFileException(exception,FileOpenError,"UnableToWriteBlob",filename);
  (void) RelinquishUniqueFileResource(filename);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s B l o b E x e m p t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsBlobExempt() returns true if the blob is exempt.
%
%  The format of the IsBlobExempt method is:
%
%       MagickBooleanType IsBlobExempt(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType IsBlobExempt(const Image *image)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(image->blob->exempt);
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s B l o b T e m p o r a r y                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsBlobTemporary() returns true if the blob is temporary.
%
%  The format of the IsBlobTemporary method is:
%
%       MagickBooleanType IsBlobTemporary(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType IsBlobTemporary(const Image *image)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(image->blob->temporary);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  M a p B l o b                                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MapBlob() creates a mapping from a file to a binary large object.
%
%  The format of the MapBlob method is:
%
%      void *MapBlob(int file,const MapMode mode,const MagickOffsetType offset,
%        const size_t length)
%
%  A description of each parameter follows:
%
%    o file: map this file descriptor.
%
%    o mode: ReadMode, WriteMode, or IOMode.
%
%    o offset: starting at this offset within the file.
%
%    o length: the length of the mapping is returned in this pointer.
%
*/
MagickExport void *MapBlob(int file,const MapMode mode,
  const MagickOffsetType offset,const size_t length)
{
#if defined(MAGICKCORE_HAVE_MMAP)
  int
    flags,
    protection;

  void
    *map;

  /*
    Map file.
  */
  flags=0;
  if (file == -1)
#if defined(MAP_ANONYMOUS)
    flags|=MAP_ANONYMOUS;
#else
    return(NULL);
#endif
  switch (mode)
  {
    case ReadMode:
    default:
    {
      protection=PROT_READ;
      flags|=MAP_PRIVATE;
      break;
    }
    case WriteMode:
    {
      protection=PROT_WRITE;
      flags|=MAP_SHARED;
      break;
    }
    case IOMode:
    {
      protection=PROT_READ | PROT_WRITE;
      flags|=MAP_SHARED;
      break;
    }
  }
#if !defined(MAGICKCORE_HAVE_HUGEPAGES) || !defined(MAP_HUGETLB)
  map=mmap((char *) NULL,length,protection,flags,file,offset);
#else
  map=mmap((char *) NULL,length,protection,flags | MAP_HUGETLB,file,offset);
  if (map == MAP_FAILED)
    map=mmap((char *) NULL,length,protection,flags,file,offset);
#endif
  if (map == MAP_FAILED)
    return(NULL);
  return(map);
#else
  (void) file;
  (void) mode;
  (void) offset;
  (void) length;
  return(NULL);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  M S B O r d e r L o n g                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MSBOrderLong() converts a least-significant byte first buffer of integers to
%  most-significant byte first.
%
%  The format of the MSBOrderLong method is:
%
%      void MSBOrderLong(unsigned char *buffer,const size_t length)
%
%  A description of each parameter follows.
%
%   o  buffer:  Specifies a pointer to a buffer of integers.
%
%   o  length:  Specifies the length of the buffer.
%
*/
MagickExport void MSBOrderLong(unsigned char *buffer,const size_t length)
{
  int
    c;

  unsigned char
    *p,
    *q;

  assert(buffer != (unsigned char *) NULL);
  q=buffer+length;
  while (buffer < q)
  {
    p=buffer+3;
    c=(int) (*p);
    *p=(*buffer);
    *buffer++=(unsigned char) c;
    p=buffer+1;
    c=(int) (*p);
    *p=(*buffer);
    *buffer++=(unsigned char) c;
    buffer+=2;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  M S B O r d e r S h o r t                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MSBOrderShort() converts a least-significant byte first buffer of integers
%  to most-significant byte first.
%
%  The format of the MSBOrderShort method is:
%
%      void MSBOrderShort(unsigned char *p,const size_t length)
%
%  A description of each parameter follows.
%
%   o  p:  Specifies a pointer to a buffer of integers.
%
%   o  length:  Specifies the length of the buffer.
%
*/
MagickExport void MSBOrderShort(unsigned char *p,const size_t length)
{
  int
    c;

  unsigned char
    *q;

  assert(p != (unsigned char *) NULL);
  q=p+length;
  while (p < q)
  {
    c=(int) (*p);
    *p=(*(p+1));
    p++;
    *p++=(unsigned char) c;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p e n B l o b                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenBlob() opens a file associated with the image.  A file name of '-' sets
%  the file to stdin for type 'r' and stdout for type 'w'.  If the filename
%  suffix is '.gz', the image is decompressed for type 'r' and compressed for
%  type 'w'.  If the filename prefix is '|', it is piped to or from a system
%  command.
%
%  The format of the OpenBlob method is:
%
%       MagickBooleanType OpenBlob(const ImageInfo *image_info,Image *image,
%        const BlobMode mode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o mode: the mode for opening the file.
%
*/

static inline MagickBooleanType SetStreamBuffering(const ImageInfo *image_info,
  const BlobInfo *blob_info)
{
  const char
    *option;

  int
    status;

  size_t
    size;

  size=MagickMinBufferExtent;
  option=GetImageOption(image_info,"stream:buffer-size");
  if (option != (const char *) NULL)
    size=StringToUnsignedLong(option);
  status=setvbuf(blob_info->file_info.file,(char *) NULL,size == 0 ?
    _IONBF : _IOFBF,size);
  return(status == 0 ? MagickTrue : MagickFalse);
}

#if defined(MAGICKCORE_ZLIB_DELEGATE)
static inline gzFile gzopen_utf8(const char *path,const char *mode)
{
#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__)
  return(gzopen(path,mode));
#else
   gzFile
     file;

   wchar_t
     *path_wide;

   path_wide=NTCreateWidePath(path);
   if (path_wide == (wchar_t *) NULL)
     return((gzFile) NULL);
   file=gzopen_w(path_wide,mode);
   path_wide=(wchar_t *) RelinquishMagickMemory(path_wide);
   return(file);
#endif
}
#endif

static inline MagickBooleanType IsStandardStreamName(const char *filename,
  const ImageInfo *image_info)
{
  return(((LocaleCompare(filename,"-") == 0) ||
    ((*filename == '\0') && (image_info->file == (FILE *) NULL))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsSharedListFilename(const char *filename,
  const Image *image)
{
  return(((LocaleCompare(filename,image->filename) == 0) &&
    ((GetPreviousImageInList(image) != (Image *) NULL) ||
    (GetNextImageInList(image) != (Image *) NULL))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsGzipMagick(const unsigned char *magick)
{
  return((((int) magick[0] == 0x1F) && ((int) magick[1] == 0x8B) &&
    ((int) magick[2] == 0x08)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsCompressedSVGExtension(const char *extension)
{
  return(((LocaleCompare(extension,"gz") == 0) ||
    (LocaleCompare(extension,"wmz") == 0) ||
    (LocaleCompare(extension,"svgz") == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType CanMapBlobFile(
  const MagickInfo *magick_info,const size_t length)
{
  return(((magick_info != (const MagickInfo *) NULL) &&
    (GetMagickBlobSupport(magick_info) != MagickFalse) &&
    (AcquireMagickResource(MapResource,length) != MagickFalse)) ?
    MagickTrue : MagickFalse);
}

static void MapBlobFile(const ImageInfo *image_info,
  BlobInfo *magick_restrict blob_info)
{
  const MagickInfo
    *magick_info;

  ExceptionInfo
    *sans_exception;

  size_t
    length;

  sans_exception=AcquireExceptionInfo();
  magick_info=GetMagickInfo(image_info->magick,sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  length=(size_t) blob_info->properties.st_size;
  if (CanMapBlobFile(magick_info,length) != MagickFalse)
    {
      void
        *blob;

      blob=MapBlob(fileno(blob_info->file_info.file),ReadMode,0,
        length);
      if (blob == (void *) NULL)
        RelinquishMagickResource(MapResource,length);
      else
        {
          /*
            Format supports blobs-- use memory-mapped I/O.
          */
          if (image_info->file != (FILE *) NULL)
            blob_info->exempt=MagickFalse;
          else
            {
              (void) fclose(blob_info->file_info.file);
              blob_info->file_info.file=(FILE *) NULL;
            }
          AttachBlob(blob_info,blob,length);
          blob_info->mapped=MagickTrue;
        }
    }
}

static void GetBlobModeFlags(const BlobMode mode,int *flags,
  const char **type)
{
  switch (mode)
  {
    case ReadBlobMode:
    {
      *flags=O_RDONLY | O_CLOEXEC;
      *type="r";
      break;
    }
    case ReadBinaryBlobMode:
    {
      *flags=O_RDONLY | O_CLOEXEC | O_BINARY;
      *type="rb";
      break;
    }
    case WriteBlobMode:
    {
      *flags=O_WRONLY | O_CLOEXEC | O_CREAT | O_TRUNC;
      *type="w";
      break;
    }
    case WriteBinaryBlobMode:
    {
      *flags=O_RDWR | O_CLOEXEC | O_CREAT | O_TRUNC | O_BINARY;
      *type="w+b";
      break;
    }
    case AppendBlobMode:
    {
      *flags=O_WRONLY | O_CLOEXEC | O_CREAT | O_APPEND;
      *type="a";
      break;
    }
    case AppendBinaryBlobMode:
    {
      *flags=O_RDWR | O_CLOEXEC | O_CREAT | O_APPEND | O_BINARY;
      *type="a+b";
      break;
    }
    default:
    {
      *flags=O_RDONLY | O_CLOEXEC;
      *type="r";
      break;
    }
  }
}

#if defined(MAGICKCORE_ZLIB_DELEGATE)
static void OpenGzipBlob(BlobInfo *magick_restrict blob_info,
  const char *filename)
{
  gzFile
    gzfile = gzopen_utf8(filename,"rb");

  if (gzfile != (gzFile) NULL)
    {
      if (blob_info->file_info.file != (FILE *) NULL)
        (void) fclose(blob_info->file_info.file);
      blob_info->file_info.file=(FILE *) NULL;
      blob_info->file_info.gzfile=gzfile;
      blob_info->type=ZipStream;
    }
}
#endif

#if defined(MAGICKCORE_BZLIB_DELEGATE)
static void OpenBZipBlob(BlobInfo *magick_restrict blob_info,
  const char *filename)
{
  BZFILE
    *bzfile = BZ2_bzopen(filename,"r");

  if (bzfile != (BZFILE *) NULL)
    {
      if (blob_info->file_info.file != (FILE *) NULL)
        (void) fclose(blob_info->file_info.file);
      blob_info->file_info.file=(FILE *) NULL;
      blob_info->file_info.bzfile=bzfile;
      blob_info->type=BZipStream;
    }
}
#endif

static void OpenCompressedBlob(const ImageInfo *image_info,
  BlobInfo *magick_restrict blob_info,const char *filename,
  unsigned char *magick)
{
#if !defined(MAGICKCORE_ZLIB_DELEGATE) && !defined(MAGICKCORE_BZLIB_DELEGATE)
  (void) filename;
  (void) magick;
#endif
#if defined(MAGICKCORE_ZLIB_DELEGATE)
  if (IsGzipMagick(magick) != MagickFalse)
    OpenGzipBlob(blob_info,filename);
#endif
#if defined(MAGICKCORE_BZLIB_DELEGATE)
  if (strncmp((char *) magick,"BZh",3) == 0)
    OpenBZipBlob(blob_info,filename);
#endif
  if (blob_info->type == FileStream)
    MapBlobFile(image_info,blob_info);
}

static void OpenReadBlobFile(BlobInfo *magick_restrict blob_info,
  const char *filename,const int flags,const char *type)
{
  int
    file;

  blob_info->file_info.file=(FILE *) NULL;
  file=open_utf8(filename,flags,0);
  if (file >= 0)
    blob_info->file_info.file=fdopen(file,type);
}

static MagickBooleanType AttachImageInfoBlob(const ImageInfo *image_info,
  BlobInfo *magick_restrict blob_info)
{
  if (image_info->stream != (StreamHandler) NULL)
    blob_info->stream=(StreamHandler) image_info->stream;
  AttachBlob(blob_info,image_info->blob,image_info->length);
  return(MagickTrue);
}

static MagickBooleanType AttachImageInfoCustomStream(
  const ImageInfo *image_info,BlobInfo *magick_restrict blob_info)
{
  blob_info->type=CustomStream;
  blob_info->custom_stream=image_info->custom_stream;
  if (blob_info->custom_stream->seeker != (CustomStreamSeeker) NULL)
    blob_info->custom_stream->seeker(0,SEEK_SET,
      blob_info->custom_stream->data);
  return(MagickTrue);
}

static void FormSharedListFilename(const Image *image,const char *extension,
  char *filename)
{
  char
    path[MagickPathExtent];

  GetPathComponent(image->filename,RootPath,path);
  if (*extension == '\0')
    (void) FormatLocaleString(filename,MagickPathExtent,"%s-%.17g",
      path,(double) image->scene);
  else
    (void) FormatLocaleString(filename,MagickPathExtent,
      "%s-%.17g.%s",path,(double) image->scene,extension);
}

MagickExport MagickBooleanType OpenBlob(const ImageInfo *image_info,
  Image *image,const BlobMode mode,ExceptionInfo *exception)
{
  BlobInfo
    *magick_restrict blob_info;

  char
    extension[MagickPathExtent],
    filename[MagickPathExtent];

  const char
    *type;

  int
    flags = O_RDONLY | O_CLOEXEC;

  MagickBooleanType
    status;

  PolicyRights
    rights;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  blob_info=image->blob;
  if (image_info->blob != (void *) NULL)
    return(AttachImageInfoBlob(image_info,blob_info));
  if ((image_info->custom_stream != (CustomStreamInfo *) NULL) &&
      (*image->filename == '\0'))
    return(AttachImageInfoCustomStream(image_info,blob_info));
  (void) DetachBlob(blob_info);
  blob_info->mode=mode;
  GetBlobModeFlags(mode,&flags,&type);
  if (*type != 'r')
    blob_info->synchronize=image_info->synchronize;
  if (image_info->stream != (StreamHandler) NULL)
    {
      blob_info->stream=image_info->stream;
      if (*type == 'w')
        {
          blob_info->type=FifoStream;
          return(MagickTrue);
        }
    }
  /*
    Open image file.
  */
  *filename='\0';
  (void) CopyMagickString(filename,image->filename,MagickPathExtent);
  rights=ReadPolicyRights;
  if (*type == 'w')
    rights=WritePolicyRights;
  if (IsPathAuthorized(rights,filename) == MagickFalse)
    ThrowPolicyException(filename,MagickFalse);
  if (IsStandardStreamName(filename,image_info) != MagickFalse)
    {
      blob_info->file_info.file=(*type == 'r') ? stdin : stdout;
#if defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__OS2__)
      if (strchr(type,'b') != (char *) NULL)
        (void) setmode(fileno(blob_info->file_info.file),_O_BINARY);
#endif
      blob_info->type=StandardStream;
      blob_info->exempt=MagickTrue;
      return(SetStreamBuffering(image_info,blob_info));
    }
  if ((LocaleNCompare(filename,"fd:",3) == 0) &&
      (IsGeometry(filename+3) != MagickFalse))
    {
      char
        fileMode[2];

      *fileMode=(*type);
      fileMode[1]='\0';
      blob_info->file_info.file=fdopen(StringToInteger(filename+3),fileMode);
      if (blob_info->file_info.file == (FILE *) NULL)
        {
          ThrowFileException(exception,BlobError,"UnableToOpenBlob",filename);
          return(MagickFalse);
        }
#if defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__OS2__)
      if (strchr(type,'b') != (char *) NULL)
        (void) setmode(fileno(blob_info->file_info.file),_O_BINARY);
#endif
      blob_info->type=FileStream;
      blob_info->exempt=MagickTrue;
      return(SetStreamBuffering(image_info,blob_info));
    }
#if defined(MAGICKCORE_HAVE_POPEN) && defined(MAGICKCORE_PIPES_SUPPORT)
  if (*filename == '|')
    {
      char
        fileMode[MagickPathExtent],
        *sanitize_command;

      /*
        Pipe image to or from a system command.
      */
#if defined(SIGPIPE)
      if (*type == 'w')
        (void) signal(SIGPIPE,SIG_IGN);
#endif
      *fileMode=(*type);
      fileMode[1]='\0';
      sanitize_command=SanitizeString(filename+1);
      blob_info->file_info.file=(FILE *) popen_utf8(sanitize_command,fileMode);
      sanitize_command=DestroyString(sanitize_command);
      if (blob_info->file_info.file == (FILE *) NULL)
        {
          ThrowFileException(exception,BlobError,"UnableToOpenBlob",filename);
          return(MagickFalse);
        }
      blob_info->type=PipeStream;
      blob_info->exempt=MagickTrue;
      return(SetStreamBuffering(image_info,blob_info));
    }
#endif
  status=GetPathAttributes(filename,&blob_info->properties);
#if defined(S_ISFIFO)
  if ((status != MagickFalse) && S_ISFIFO(blob_info->properties.st_mode))
    {
      blob_info->file_info.file=(FILE *) fopen_utf8(filename,type);
      if (blob_info->file_info.file == (FILE *) NULL)
        {
          ThrowFileException(exception,BlobError,"UnableToOpenBlob",filename);
          return(MagickFalse);
        }
      blob_info->type=FileStream;
      blob_info->exempt=MagickTrue;
      return(SetStreamBuffering(image_info,blob_info));
    }
#endif
  GetPathComponent(image->filename,ExtensionPath,extension);
  if (*type == 'w')
    {
      (void) CopyMagickString(filename,image->filename,MagickPathExtent);
      if ((image_info->adjoin == MagickFalse) ||
          (strchr(filename,'%') != (char *) NULL))
        {
          /*
            Form filename for multi-part images.
          */
          (void) InterpretImageFilename(image_info,image,image->filename,(int)
            image->scene,filename,exception);
          if (IsSharedListFilename(filename,image) != MagickFalse)
            FormSharedListFilename(image,extension,filename);
          (void) CopyMagickString(image->filename,filename,MagickPathExtent);
        }
      if (IsPathAuthorized(rights,filename) == MagickFalse)
        ThrowPolicyException(filename,MagickFalse);
    }
  if (image_info->file != (FILE *) NULL)
    {
      blob_info->file_info.file=image_info->file;
      blob_info->type=FileStream;
      blob_info->exempt=MagickTrue;
    }
  else
    if (*type == 'r')
      {
        OpenReadBlobFile(blob_info,filename,flags,type);
        if (blob_info->file_info.file != (FILE *) NULL)
          {
            size_t
              count;

            unsigned char
              magick[3];

            blob_info->type=FileStream;
            (void) SetStreamBuffering(image_info,blob_info);
            (void) memset(magick,0,sizeof(magick));
            count=fread(magick,1,sizeof(magick),blob_info->file_info.file);
            (void) fseek(blob_info->file_info.file,-((off_t) count),SEEK_CUR);
#if defined(MAGICKCORE_POSIX_SUPPORT)
            (void) fflush(blob_info->file_info.file);
#endif
            (void) LogMagickEvent(BlobEvent,GetMagickModule(),
               "  read %.17g magic header bytes",(double) count);
            OpenCompressedBlob(image_info,blob_info,filename,magick);
          }
      }
    else
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      if (IsCompressedSVGExtension(extension) != MagickFalse)
        {
          blob_info->file_info.gzfile=gzopen_utf8(filename,"wb");
          if (blob_info->file_info.gzfile != (gzFile) NULL)
            blob_info->type=ZipStream;
        }
      else
#endif
#if defined(MAGICKCORE_BZLIB_DELEGATE)
        if (LocaleCompare(extension,"bz2") == 0)
          {
            blob_info->file_info.bzfile=BZ2_bzopen(filename,"w");
            if (blob_info->file_info.bzfile != (BZFILE *) NULL)
              blob_info->type=BZipStream;
          }
        else
#endif
          {
            int
              file;

            blob_info->file_info.file=(FILE *) NULL;
            file=open_utf8(filename,flags,P_MODE);
            if (file >= 0)
              blob_info->file_info.file=fdopen(file,type);
            if (blob_info->file_info.file != (FILE *) NULL)
              {
                blob_info->type=FileStream;
                (void) SetStreamBuffering(image_info,blob_info);
              }
          }
  if (IsPathAuthorized(rights,filename) == MagickFalse)
    ThrowPolicyException(filename,MagickFalse);
  blob_info->status=0;
  blob_info->error_number=0;
  if (blob_info->type != UndefinedStream)
    blob_info->size=GetBlobSize(image);
  else
    {
      ThrowFileException(exception,BlobError,"UnableToOpenBlob",filename);
      return(MagickFalse);
    }
  return(MagickTrue);
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
+  R e a d B l o b B y t e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobByte() reads a single byte from the image file and returns it.
%
%  The format of the ReadBlobByte method is:
%
%      int ReadBlobByte(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport int ReadBlobByte(Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  int
    c;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  blob_info=image->blob;
  switch (blob_info->type)
  {
    case StandardStream:
    case FileStream:
    case PipeStream:
    {
      c=getc(blob_info->file_info.file);
      if (c == EOF)
        {
          if (ferror(blob_info->file_info.file) != 0)
            ThrowBlobException(blob_info);
          return(EOF);
        }
      break;
    }
    case BlobStream:
    {
      if (blob_info->offset >= (MagickOffsetType) blob_info->length)
        {
          blob_info->eof=MagickTrue;
          return(EOF);
        }
      c=(int) (*((unsigned char *) blob_info->data+blob_info->offset));
      blob_info->offset++;
      break;
    }
    default:
    {
      ssize_t
        count;

      unsigned char
        buffer[1];

      count=ReadBlob(image,1,buffer);
      if (count != 1)
        return(EOF);
      c=(int) *buffer;
      break;
    }
  }
  return(c);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b D o u b l e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobDouble() reads a double value as a 64-bit quantity in the byte-order
%  specified by the endian member of the image structure.
%
%  The format of the ReadBlobDouble method is:
%
%      double ReadBlobDouble(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport double ReadBlobDouble(Image *image)
{
  union
  {
    MagickSizeType
      unsigned_value;

    double
      double_value;
  } quantum;

  quantum.double_value=0.0;
  quantum.unsigned_value=ReadBlobLongLong(image);
  return(quantum.double_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b F l o a t                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobFloat() reads a float value as a 32-bit quantity in the byte-order
%  specified by the endian member of the image structure.
%
%  The format of the ReadBlobFloat method is:
%
%      float ReadBlobFloat(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport float ReadBlobFloat(Image *image)
{
  union
  {
    unsigned int
      unsigned_value;

    float
      float_value;
  } quantum;

  quantum.float_value=0.0;
  quantum.unsigned_value=ReadBlobLong(image);
  return(quantum.float_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L o n g                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLong() reads a unsigned int value as a 32-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the ReadBlobLong method is:
%
%      unsigned int ReadBlobLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned int ReadBlobLong(Image *image)
{
  const unsigned char
    *p;

  ssize_t
    count;

  unsigned char
    buffer[4];

  unsigned int
    value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,4,buffer,&count);
  if (count != 4)
    return(0UL);
  if (image->endian == LSBEndian)
    {
      value=(unsigned int) (*p++);
      value|=(unsigned int) (*p++) << 8;
      value|=(unsigned int) (*p++) << 16;
      value|=(unsigned int) (*p++) << 24;
      return(value);
    }
  value=(unsigned int) (*p++) << 24;
  value|=(unsigned int) (*p++) << 16;
  value|=(unsigned int) (*p++) << 8;
  value|=(unsigned int) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L o n g L o n g                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLongLong() reads a long long value as a 64-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the ReadBlobLongLong method is:
%
%      MagickSizeType ReadBlobLongLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport MagickSizeType ReadBlobLongLong(Image *image)
{
  MagickSizeType
    value;

  const unsigned char
    *p;

  ssize_t
    count;

  unsigned char
    buffer[8];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,8,buffer,&count);
  if (count != 8)
    return(MagickULLConstant(0));
  if (image->endian == LSBEndian)
    {
      value=(MagickSizeType) (*p++);
      value|=(MagickSizeType) (*p++) << 8;
      value|=(MagickSizeType) (*p++) << 16;
      value|=(MagickSizeType) (*p++) << 24;
      value|=(MagickSizeType) (*p++) << 32;
      value|=(MagickSizeType) (*p++) << 40;
      value|=(MagickSizeType) (*p++) << 48;
      value|=(MagickSizeType) (*p++) << 56;
      return(value);
    }
  value=(MagickSizeType) (*p++) << 56;
  value|=(MagickSizeType) (*p++) << 48;
  value|=(MagickSizeType) (*p++) << 40;
  value|=(MagickSizeType) (*p++) << 32;
  value|=(MagickSizeType) (*p++) << 24;
  value|=(MagickSizeType) (*p++) << 16;
  value|=(MagickSizeType) (*p++) << 8;
  value|=(MagickSizeType) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S h o r t                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobShort() reads a short value as a 16-bit quantity in the byte-order
%  specified by the endian member of the image structure.
%
%  The format of the ReadBlobShort method is:
%
%      unsigned short ReadBlobShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned short ReadBlobShort(Image *image)
{
  const unsigned char
    *p;

  unsigned short
    value;

  ssize_t
    count;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,2,buffer,&count);
  if (count != 2)
    return((unsigned short) 0U);
  if (image->endian == LSBEndian)
    {
      value=(unsigned short) (*p++);
      value|=(unsigned short) (*p++) << 8;
      return(value);
    }
  value=(unsigned short) ((unsigned short) (*p++) << 8);
  value|=(unsigned short) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B L o n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLSBLong() reads a unsigned int value as a 32-bit quantity in
%  least-significant byte first order.
%
%  The format of the ReadBlobLSBLong method is:
%
%      unsigned int ReadBlobLSBLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned int ReadBlobLSBLong(Image *image)
{
  const unsigned char
    *p;

  unsigned int
    value;

  ssize_t
    count;

  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,4,buffer,&count);
  if (count != 4)
    return(0U);
  value=(unsigned int) (*p++);
  value|=(unsigned int) (*p++) << 8;
  value|=(unsigned int) (*p++) << 16;
  value|=(unsigned int) (*p++) << 24;
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B S i g n e d L o n g                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLSBSignedLong() reads a signed int value as a 32-bit quantity in
%  least-significant byte first order.
%
%  The format of the ReadBlobLSBSignedLong method is:
%
%      signed int ReadBlobLSBSignedLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed int ReadBlobLSBSignedLong(Image *image)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobLSBLong(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B S h o r t                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLSBShort() reads a short value as a 16-bit quantity in
%  least-significant byte first order.
%
%  The format of the ReadBlobLSBShort method is:
%
%      unsigned short ReadBlobLSBShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned short ReadBlobLSBShort(Image *image)
{
  const unsigned char
    *p;

  unsigned short
    value;

  ssize_t
    count;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,2,buffer,&count);
  if (count != 2)
    return((unsigned short) 0U);
  value=(unsigned short) (*p++);
  value|=(unsigned short) (*p++) << 8;
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B S i g n e d S h o r t                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLSBSignedShort() reads a signed short value as a 16-bit quantity in
%  least-significant byte-order.
%
%  The format of the ReadBlobLSBSignedShort method is:
%
%      signed short ReadBlobLSBSignedShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed short ReadBlobLSBSignedShort(Image *image)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobLSBShort(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B L o n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBLong() reads a unsigned int value as a 32-bit quantity in
%  most-significant byte first order.
%
%  The format of the ReadBlobMSBLong method is:
%
%      unsigned int ReadBlobMSBLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned int ReadBlobMSBLong(Image *image)
{
  const unsigned char
    *p;

  unsigned int
    value;

  ssize_t
    count;

  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,4,buffer,&count);
  if (count != 4)
    return(0UL);
  value=(unsigned int) (*p++) << 24;
  value|=(unsigned int) (*p++) << 16;
  value|=(unsigned int) (*p++) << 8;
  value|=(unsigned int) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B L o n g L o n g                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBLongLong() reads a unsigned long long value as a 64-bit quantity
%  in most-significant byte first order.
%
%  The format of the ReadBlobMSBLongLong method is:
%
%      unsigned int ReadBlobMSBLongLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport MagickSizeType ReadBlobMSBLongLong(Image *image)
{
  const unsigned char
    *p;

  MagickSizeType
    value;

  ssize_t
    count;

  unsigned char
    buffer[8];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,8,buffer,&count);
  if (count != 8)
    return(MagickULLConstant(0));
  value=(MagickSizeType) (*p++) << 56;
  value|=(MagickSizeType) (*p++) << 48;
  value|=(MagickSizeType) (*p++) << 40;
  value|=(MagickSizeType) (*p++) << 32;
  value|=(MagickSizeType) (*p++) << 24;
  value|=(MagickSizeType) (*p++) << 16;
  value|=(MagickSizeType) (*p++) << 8;
  value|=(MagickSizeType) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B S h o r t                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBShort() reads a short value as a 16-bit quantity in
%  most-significant byte first order.
%
%  The format of the ReadBlobMSBShort method is:
%
%      unsigned short ReadBlobMSBShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned short ReadBlobMSBShort(Image *image)
{
  const unsigned char
    *p;

  unsigned short
    value;

  ssize_t
    count;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,2,buffer,&count);
  if (count != 2)
    return((unsigned short) 0U);
  value=(unsigned short) ((*p++) << 8);
  value|=(unsigned short) (*p++);
  return((unsigned short) (value & 0xffff));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B S i g n e d L o n g                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBSignedLong() reads a signed int value as a 32-bit quantity in
%  most-significant byte-order.
%
%  The format of the ReadBlobMSBSignedLong method is:
%
%      signed int ReadBlobMSBSignedLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed int ReadBlobMSBSignedLong(Image *image)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobMSBLong(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B S i g n e d S h o r t                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBSignedShort() reads a signed short value as a 16-bit quantity in
%  most-significant byte-order.
%
%  The format of the ReadBlobMSBSignedShort method is:
%
%      signed short ReadBlobMSBSignedShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed short ReadBlobMSBSignedShort(Image *image)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobMSBShort(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S i g n e d L o n g                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobSignedLong() reads a signed int value as a 32-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the ReadBlobSignedLong method is:
%
%      signed int ReadBlobSignedLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed int ReadBlobSignedLong(Image *image)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobLong(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S i g n e d S h o r t                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobSignedShort() reads a signed short value as a 16-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the ReadBlobSignedShort method is:
%
%      signed short ReadBlobSignedShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed short ReadBlobSignedShort(Image *image)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobShort(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S t r e a m                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobStream() reads data from the blob or image file and returns it.  It
%  returns a pointer to the data buffer you supply or to the image memory
%  buffer if its supported (zero-copy). If length is zero, ReadBlobStream()
%  returns a count of zero and has no other results. If length is greater than
%  MAGICK_SSIZE_MAX, the result is unspecified.
%
%  The format of the ReadBlobStream method is:
%
%      const void *ReadBlobStream(Image *image,const size_t length,
%        void *magick_restrict data,ssize_t *count)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length:  Specifies an integer representing the number of bytes to read
%      from the file.
%
%    o count: returns the number of bytes read.
%
%    o data:  Specifies an area to place the information requested from the
%      file.
%
*/
MagickExport magick_hot_spot const void *ReadBlobStream(Image *image,
  const size_t length,void *magick_restrict data,ssize_t *count)
{
  BlobInfo
    *magick_restrict blob_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  assert(count != (ssize_t *) NULL);
  blob_info=image->blob;
  if (blob_info->type != BlobStream)
    {
      assert(data != NULL);
      *count=ReadBlob(image,length,(unsigned char *) data);
      return(data);
    }
  if (blob_info->offset >= (MagickOffsetType) blob_info->length)
    {
      *count=0;
      blob_info->eof=MagickTrue;
      return(data);
    }
  data=blob_info->data+blob_info->offset;
  *count=(ssize_t) MagickMin((MagickOffsetType) length,(MagickOffsetType)
    blob_info->length-blob_info->offset);
  blob_info->offset+=(*count);
  if (*count != (ssize_t) length)
    blob_info->eof=MagickTrue;
  return(data);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d B l o b S t r i n g                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobString() reads characters from a blob or file until a newline
%  character is read or an end-of-file condition is encountered.
%
%  The format of the ReadBlobString method is:
%
%      char *ReadBlobString(Image *image,char *string)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o string: the address of a character buffer.
%
*/
static size_t ReadBlobLine(Image *image,BlobInfo *magick_restrict blob_info,
  char *string,size_t i)
{
  int
    c;

  do
  {
    c=ReadBlobByte(image);
    if (c == EOF)
      {
        blob_info->eof=MagickTrue;
        break;
      }
    string[i++]=(char) c;
    if (c == '\n')
      break;
  } while (i < (MaxTextExtent-2));
  string[i]='\0';
  return(i);
}

MagickExport char *ReadBlobString(Image *image,char *string)
{
  BlobInfo
    *magick_restrict blob_info;

  size_t
    i = 0;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  *string='\0';
  blob_info=image->blob;
  switch (blob_info->type)
  {
    case UndefinedStream:
      break;
    case StandardStream:
    case FileStream:
    {
      char *p = fgets(string,MagickPathExtent,blob_info->file_info.file);
      if (p == (char *) NULL)
        {
          if (ferror(blob_info->file_info.file) != 0)
            ThrowBlobException(blob_info);
          return((char *) NULL);
        }
      i=strlen(string);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      char *p = gzgets(blob_info->file_info.gzfile,string,MagickPathExtent);
      if (p == (char *) NULL)
        {
          int status = Z_OK;
          (void) gzerror(blob_info->file_info.gzfile,&status);
          if (status != Z_OK)
            ThrowBlobException(blob_info);
          return((char *) NULL);
        }
      i=strlen(string);
      break;
#endif
    }
    default:
    {
      i=ReadBlobLine(image,blob_info,string,i);
      break;
    }
  }
  /*
    Strip trailing newline.
  */
  if ((string[i] == '\r') || (string[i] == '\n'))
    string[i]='\0';
  if (i >= 1)
    if ((string[i-1] == '\r') || (string[i-1] == '\n'))
      string[i-1]='\0';
  if ((*string == '\0') && (blob_info->eof != MagickFalse))
    return((char *) NULL);
  return(string);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e f e r e n c e B l o b                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReferenceBlob() increments the reference count associated with the pixel
%  blob returning a pointer to the blob.
%
%  The format of the ReferenceBlob method is:
%
%      BlobInfo ReferenceBlob(BlobInfo *blob_info)
%
%  A description of each parameter follows:
%
%    o blob_info: the blob_info.
%
*/
MagickExport BlobInfo *ReferenceBlob(BlobInfo *blob)
{
  assert(blob != (BlobInfo *) NULL);
  assert(blob->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  LockSemaphoreInfo(blob->semaphore);
  blob->reference_count++;
  UnlockSemaphoreInfo(blob->semaphore);
  return(blob);
}

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
+   S e t B l o b E x e m p t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetBlobExempt() sets the blob exempt status.
%
%  The format of the SetBlobExempt method is:
%
%      MagickBooleanType SetBlobExempt(const Image *image,
%        const MagickBooleanType exempt)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exempt: Set to true if this blob is exempt from being closed.
%
*/
MagickExport void SetBlobExempt(Image *image,const MagickBooleanType exempt)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image->blob->exempt=exempt;
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
+  S y n c B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncBlob() flushes the datastream if it is a file or synchronizes the data
%  attributes if it is an blob.  It returns 0 on success; otherwise, it returns
%  -1 and set errno to indicate the error.
%
%  The format of the SyncBlob method is:
%
%      int SyncBlob(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
static inline MagickBooleanType IsWriteBlobMode(const BlobInfo *blob_info)
{
  return(((blob_info->mode == WriteBlobMode) ||
    (blob_info->mode == WriteBinaryBlobMode) ||
    (blob_info->mode == AppendBlobMode) ||
    (blob_info->mode == AppendBinaryBlobMode)) ? MagickTrue : MagickFalse);
}

static int SyncBlob(const Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  int
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (EOFBlob(image) != 0)
    return(0);
  blob_info=image->blob;
  status=0;
  switch (blob_info->type)
  {
    case UndefinedStream:
      break;
    case StandardStream:
    {
      if (IsWriteBlobMode(blob_info) != MagickFalse)
        status=fflush(blob_info->file_info.file);
      break;
    }
    case FileStream:
    case PipeStream:
    {
      status=fflush(blob_info->file_info.file);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      (void) gzflush(blob_info->file_info.gzfile,Z_SYNC_FLUSH);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(MAGICKCORE_BZLIB_DELEGATE)
      status=BZ2_bzflush(blob_info->file_info.bzfile);
#endif
      break;
    }
    case FifoStream:
      break;
    case BlobStream:
      break;
    case CustomStream:
      break;
  }
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S y n c B l o b S t r e a m                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncBlobStream() transfers ownership of an in-memory blob stream from the
%  image that has just been written back to the owner image and to image info.
%  The method copies the current buffer state onto the owner, refreshes
%  image info, and detaches the borrower so exactly one object owns the memory.
%
%  The format of the SyncBlobStream method is:
%
%      void SyncBlobStream(ImageInfo *image,const Image *image,Image *owner)
%
%  A description of each parameter follows:
%
%    o image_info: image info whose blob/length members are refreshed; may be
%      NULL.
%
%    o owner: the image ImagesToBlob()/ImageToBlob() harvests the blob from
%      (the head of the list).
%
%    o image: the image that was just written; may be the owner.
%
*/
MagickPrivate void SyncBlobStream(ImageInfo *image_info,Image *image,
  Image *owner)
{
  assert(owner != (Image *) NULL);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->blob == (BlobInfo *) NULL)
    return;
  if (image->blob->data == (unsigned char *) NULL)
    return;
  if (image_info != (ImageInfo *) NULL)
    {
      image_info->blob=(void *) image->blob->data;
      image_info->length=image->blob->length;
    }
  if ((image == owner) || (owner->blob == (BlobInfo *) NULL))
    return;
  if (owner->blob == (BlobInfo *) NULL)
    return;
  /*
    Promote the live buffer onto the owner before detaching the borrower;
    DetachBlob() clears image->blob->data.
  */
  owner->blob->data=image->blob->data;
  owner->blob->extent=image->blob->extent;
  owner->blob->length=image->blob->length;
  owner->blob->offset=image->blob->offset;
  owner->blob->quantum=image->blob->quantum;
  owner->blob->type=BlobStream;
  owner->blob->exempt=MagickTrue;
  (void) DetachBlob(image->blob);
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  U n m a p B l o b                                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnmapBlob() deallocates the binary large object previously allocated with
%  the MapBlob method.
%
%  The format of the UnmapBlob method is:
%
%       MagickBooleanType UnmapBlob(void *map,const size_t length)
%
%  A description of each parameter follows:
%
%    o map: the address  of the binary large object.
%
%    o length: the length of the binary large object.
%
*/
MagickExport MagickBooleanType UnmapBlob(void *map,const size_t length)
{
#if defined(MAGICKCORE_HAVE_MMAP)
  int
    status;

  status=munmap(map,length);
  return(status == -1 ? MagickFalse : MagickTrue);
#else
  (void) map;
  (void) length;
  return(MagickFalse);
#endif
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b B y t e                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobByte() write an integer to a blob.  It returns the number of bytes
%  written (either 0 or 1);
%
%  The format of the WriteBlobByte method is:
%
%      ssize_t WriteBlobByte(Image *image,const unsigned char value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobByte(Image *image,const unsigned char value)
{
  BlobInfo
    *magick_restrict blob_info;

  ssize_t
    count;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  blob_info=image->blob;
  count=0;
  switch (blob_info->type)
  {
    case StandardStream:
    case FileStream:
    case PipeStream:
    {
      int
        c;

      c=putc((int) value,blob_info->file_info.file);
      if (c == EOF)
        {
          if (ferror(blob_info->file_info.file) != 0)
            ThrowBlobException(blob_info);
          break;
        }
      count++;
      break;
    }
    default:
    {
      count=WriteBlobStream(image,1,&value);
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
+  W r i t e B l o b F l o a t                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobFloat() writes a float value as a 32-bit quantity in the byte-order
%  specified by the endian member of the image structure.
%
%  The format of the WriteBlobFloat method is:
%
%      ssize_t WriteBlobFloat(Image *image,const float value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobFloat(Image *image,const float value)
{
  union
  {
    unsigned int
      unsigned_value;

    float
      float_value;
  } quantum;

  quantum.unsigned_value=0U;
  quantum.float_value=value;
  return(WriteBlobLong(image,quantum.unsigned_value));
}

static ssize_t WriteBlobLSBLongBytes(Image *image,const unsigned int value)
{
  unsigned char
    buffer[4];

  buffer[0]=(unsigned char) value;
  buffer[1]=(unsigned char) (value >> 8);
  buffer[2]=(unsigned char) (value >> 16);
  buffer[3]=(unsigned char) (value >> 24);
  return(WriteBlobStream(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L o n g                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLong() writes a unsigned int value as a 32-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the WriteBlobLong method is:
%
%      ssize_t WriteBlobLong(Image *image,const unsigned int value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLong(Image *image,const unsigned int value)
{
  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->endian == LSBEndian)
    {
      return(WriteBlobLSBLongBytes(image,value));
    }
  buffer[0]=(unsigned char) (value >> 24);
  buffer[1]=(unsigned char) (value >> 16);
  buffer[2]=(unsigned char) (value >> 8);
  buffer[3]=(unsigned char) value;
  return(WriteBlobStream(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L o n g L o n g                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobMSBLongLong() writes a long long value as a 64-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the WriteBlobLongLong method is:
%
%      ssize_t WriteBlobLongLong(Image *image,const MagickSizeType value)
%
%  A description of each parameter follows.
%
%    o value:  Specifies the value to write.
%
%    o image: the image.
%
*/
MagickExport ssize_t WriteBlobLongLong(Image *image,const MagickSizeType value)
{
  unsigned char
    buffer[8];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->endian == LSBEndian)
    {
      buffer[0]=(unsigned char) value;
      buffer[1]=(unsigned char) (value >> 8);
      buffer[2]=(unsigned char) (value >> 16);
      buffer[3]=(unsigned char) (value >> 24);
      buffer[4]=(unsigned char) (value >> 32);
      buffer[5]=(unsigned char) (value >> 40);
      buffer[6]=(unsigned char) (value >> 48);
      buffer[7]=(unsigned char) (value >> 56);
      return(WriteBlobStream(image,8,buffer));
    }
  buffer[0]=(unsigned char) (value >> 56);
  buffer[1]=(unsigned char) (value >> 48);
  buffer[2]=(unsigned char) (value >> 40);
  buffer[3]=(unsigned char) (value >> 32);
  buffer[4]=(unsigned char) (value >> 24);
  buffer[5]=(unsigned char) (value >> 16);
  buffer[6]=(unsigned char) (value >> 8);
  buffer[7]=(unsigned char) value;
  return(WriteBlobStream(image,8,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b S h o r t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobShort() writes a short value as a 16-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the WriteBlobShort method is:
%
%      ssize_t WriteBlobShort(Image *image,const unsigned short value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value:  Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobShort(Image *image,const unsigned short value)
{
  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->endian == LSBEndian)
    {
      buffer[0]=(unsigned char) value;
      buffer[1]=(unsigned char) (value >> 8);
      return(WriteBlobStream(image,2,buffer));
    }
  buffer[0]=(unsigned char) (value >> 8);
  buffer[1]=(unsigned char) value;
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b S i g n e d L o n g                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobSignedLong() writes a signed value as a 32-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the WriteBlobSignedLong method is:
%
%      ssize_t WriteBlobSignedLong(Image *image,const signed int value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobSignedLong(Image *image,const signed int value)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  quantum.signed_value=value;
  if (image->endian == LSBEndian)
    {
      return(WriteBlobLSBLongBytes(image,quantum.unsigned_value));
    }
  buffer[0]=(unsigned char) (quantum.unsigned_value >> 24);
  buffer[1]=(unsigned char) (quantum.unsigned_value >> 16);
  buffer[2]=(unsigned char) (quantum.unsigned_value >> 8);
  buffer[3]=(unsigned char) quantum.unsigned_value;
  return(WriteBlobStream(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L S B L o n g                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLSBLong() writes a unsigned int value as a 32-bit quantity in
%  least-significant byte first order.
%
%  The format of the WriteBlobLSBLong method is:
%
%      ssize_t WriteBlobLSBLong(Image *image,const unsigned int value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLSBLong(Image *image,const unsigned int value)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  return(WriteBlobLSBLongBytes(image,value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b L S B S h o r t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLSBShort() writes a unsigned short value as a 16-bit quantity in
%  least-significant byte first order.
%
%  The format of the WriteBlobLSBShort method is:
%
%      ssize_t WriteBlobLSBShort(Image *image,const unsigned short value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value:  Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLSBShort(Image *image,const unsigned short value)
{
  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  buffer[0]=(unsigned char) value;
  buffer[1]=(unsigned char) (value >> 8);
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L S B S i g n e d L o n g                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLSBSignedLong() writes a signed value as a 32-bit quantity in
%  least-significant byte first order.
%
%  The format of the WriteBlobLSBSignedLong method is:
%
%      ssize_t WriteBlobLSBSignedLong(Image *image,const signed int value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLSBSignedLong(Image *image,const signed int value)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  quantum.signed_value=value;
  return(WriteBlobLSBLongBytes(image,quantum.unsigned_value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b L S B S i g n e d S h o r t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLSBSignedShort() writes a signed short value as a 16-bit quantity
%  in least-significant byte first order.
%
%  The format of the WriteBlobLSBSignedShort method is:
%
%      ssize_t WriteBlobLSBSignedShort(Image *image,const signed short value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value:  Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLSBSignedShort(Image *image,
  const signed short value)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  quantum.signed_value=value;
  buffer[0]=(unsigned char) quantum.unsigned_value;
  buffer[1]=(unsigned char) (quantum.unsigned_value >> 8);
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b M S B L o n g                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobMSBLong() writes a unsigned int value as a 32-bit quantity in
%  most-significant byte first order.
%
%  The format of the WriteBlobMSBLong method is:
%
%      ssize_t WriteBlobMSBLong(Image *image,const unsigned int value)
%
%  A description of each parameter follows.
%
%    o value:  Specifies the value to write.
%
%    o image: the image.
%
*/
MagickExport ssize_t WriteBlobMSBLong(Image *image,const unsigned int value)
{
  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  buffer[0]=(unsigned char) (value >> 24);
  buffer[1]=(unsigned char) (value >> 16);
  buffer[2]=(unsigned char) (value >> 8);
  buffer[3]=(unsigned char) value;
  return(WriteBlobStream(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b M S B S i g n e d S h o r t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobMSBSignedShort() writes a signed short value as a 16-bit quantity
%  in most-significant byte first order.
%
%  The format of the WriteBlobMSBSignedShort method is:
%
%      ssize_t WriteBlobMSBSignedShort(Image *image,const signed short value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value:  Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobMSBSignedShort(Image *image,
  const signed short value)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  quantum.signed_value=value;
  buffer[0]=(unsigned char) (quantum.unsigned_value >> 8);
  buffer[1]=(unsigned char) quantum.unsigned_value;
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b M S B S h o r t                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobMSBShort() writes a unsigned short value as a 16-bit quantity in
%  most-significant byte first order.
%
%  The format of the WriteBlobMSBShort method is:
%
%      ssize_t WriteBlobMSBShort(Image *image,const unsigned short value)
%
%  A description of each parameter follows.
%
%   o  value:  Specifies the value to write.
%
%   o  file:  Specifies the file to write the data to.
%
*/
MagickExport ssize_t WriteBlobMSBShort(Image *image,const unsigned short value)
{
  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  buffer[0]=(unsigned char) (value >> 8);
  buffer[1]=(unsigned char) value;
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b S t r i n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobString() write a string to a blob.  It returns the number of
%  characters written.
%
%  The format of the WriteBlobString method is:
%
%      ssize_t WriteBlobString(Image *image,const char *string)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o string: Specifies the string to write.
%
*/
MagickExport ssize_t WriteBlobString(Image *image,const char *string)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(string != (const char *) NULL);
  return(WriteBlobStream(image,strlen(string),(const unsigned char *) string));
}
