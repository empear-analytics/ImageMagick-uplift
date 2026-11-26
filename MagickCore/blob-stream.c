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
