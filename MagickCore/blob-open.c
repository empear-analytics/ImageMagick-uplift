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
%                          OOO   PPPP   EEEEE  N   N                          %
%                         O   O  P   P  E      NN  N                          %
%                         O   O  PPPP   EEE    N N N                          %
%                         O   O  P      E      N  NN                          %
%                          OOO   P      EEEEE  N   N                          %
%                                                                             %
%                                                                             %
%                   MagickCore Blob Open and Close Methods                    %
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

static void CheckBlobStreamStatus(BlobInfo *blob_info,int status)
{
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
}

static void CloseBlobStream(BlobInfo *blob_info)
{
  int
    status;

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
  CheckBlobStreamStatus(blob_info,status);
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
  CloseBlobStream(blob_info);
  (void) DetachBlob(blob_info);
  return(blob_info->status != 0 ? MagickFalse : MagickTrue);
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
