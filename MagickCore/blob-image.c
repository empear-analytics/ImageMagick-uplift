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
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%             MagickCore Image to Blob and Blob to Image Methods              %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   B l o b T o F i l e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlobToFile() writes a blob to a file.  It returns MagickFalse if an error
%  occurs otherwise MagickTrue.
%
%  The format of the BlobToFile method is:
%
%       MagickBooleanType BlobToFile(char *filename,const void *blob,
%         const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: Write the blob to this file. The filename buffer length must
%      be a minimum of MagickPathExtent characters.
%
%    o blob: the address of a blob.
%
%    o length: This length in bytes of the blob.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType BlobToFile(char *filename,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  int
    file;

  size_t
    i;

  ssize_t
    count;

  assert(filename != (const char *) NULL);
  assert(blob != (const void *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  if (*filename == '\0')
    file=AcquireUniqueFileResource(filename);
  else
    file=open_utf8(filename,O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL | O_BINARY,P_MODE);
  if (file == -1)
    {
      ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
      return(MagickFalse);
    }
  for (i=0; i < length; i+=(size_t) count)
  {
    count=MagickWrite(file,(const char *) blob+i,MagickMin(length-i,(size_t)
      MagickMaxBufferExtent));
    if (count <= 0)
      break;
  }
  file=close_utf8(file);
  if ((file == -1) || (i < length))
    {
      ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B l o b T o I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlobToImage() implements direct to memory image formats.  It returns the
%  blob as an image.
%
%  The format of the BlobToImage method is:
%
%      Image *BlobToImage(const ImageInfo *image_info,const void *blob,
%        const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o blob: the address of a character stream in one of the image formats
%      understood by ImageMagick.
%
%    o length: This size_t integer reflects the length in bytes of the blob.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *BlobToImage(const ImageInfo *image_info,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  Image
    *image;

  ImageInfo
    *blob_info,
    *clone_info;

  MagickBooleanType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if ((blob == (const void *) NULL) || (length == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),BlobError,
        "ZeroLengthBlobNotPermitted","`%s'",image_info->filename);
      return((Image *) NULL);
    }
  blob_info=CloneImageInfo(image_info);
  blob_info->blob=(void *) blob;
  blob_info->length=length;
  if (*blob_info->magick == '\0')
    (void) SetImageInfo(blob_info,0,exception);
  magick_info=GetMagickInfo(blob_info->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
        blob_info->magick);
      blob_info=DestroyImageInfo(blob_info);
      return((Image *) NULL);
    }
  if (GetMagickBlobSupport(magick_info) != MagickFalse)
    {
      char
        filename[MagickPathExtent];

      /*
        Native blob support for this image format.
      */
      (void) CopyMagickString(filename,blob_info->filename,MagickPathExtent);
      (void) FormatLocaleString(blob_info->filename,MagickPathExtent,"%s:%s",
        blob_info->magick,filename);
      image=ReadImage(blob_info,exception);
      if (image != (Image *) NULL)
        (void) DetachBlob(image->blob);
      blob_info=DestroyImageInfo(blob_info);
      return(image);
    }
  /*
    Write blob to a temporary file on disk.
  */
  blob_info->blob=(void *) NULL;
  blob_info->length=0;
  *blob_info->filename='\0';
  status=BlobToFile(blob_info->filename,blob,length,exception);
  if (status == MagickFalse)
    {
      (void) RelinquishUniqueFileResource(blob_info->filename);
      blob_info=DestroyImageInfo(blob_info);
      return((Image *) NULL);
    }
  clone_info=CloneImageInfo(blob_info);
  (void) FormatLocaleString(clone_info->filename,MagickPathExtent,"%s:%s",
    blob_info->magick,blob_info->filename);
  image=ReadImage(clone_info,exception);
  if (image != (Image *) NULL)
    {
      Image
        *images;

      /*
        Restore original filenames and image format.
      */
      for (images=GetFirstImageInList(image); images != (Image *) NULL; )
      {
        (void) CopyMagickString(images->filename,image_info->filename,
          MagickPathExtent);
        (void) CopyMagickString(images->magick_filename,image_info->filename,
          MagickPathExtent);
        (void) CopyMagickString(images->magick,magick_info->name,
          MagickPathExtent);
        images=GetNextImageInList(images);
      }
    }
  clone_info=DestroyImageInfo(clone_info);
  (void) RelinquishUniqueFileResource(blob_info->filename);
  blob_info=DestroyImageInfo(blob_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C u s t o m S t r e a m T o I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CustomStreamToImage() is the equivalent of ReadImage(), but reads the
%  formatted "file" from the supplied method rather than to an actual file.
%
%  The format of the CustomStreamToImage method is:
%
%      Image *CustomStreamToImage(const ImageInfo *image_info,
%         ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void RestoreCustomStreamFilenames(const ImageInfo *image_info,
  const MagickInfo *magick_info,Image *image)
{
  Image
    *images;

  /*
    Restore original filenames and image format.
  */
  for (images=GetFirstImageInList(image); images != (Image *) NULL; )
  {
    (void) CopyMagickString(images->filename,image_info->filename,
      MagickPathExtent);
    (void) CopyMagickString(images->magick_filename,
      image_info->filename,MagickPathExtent);
    (void) CopyMagickString(images->magick,magick_info->name,
      MagickPathExtent);
    images=GetNextImageInList(images);
  }
}

MagickExport Image *CustomStreamToImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  Image
    *image;

  ImageInfo
    *blob_info;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image_info->custom_stream != (CustomStreamInfo *) NULL);
  assert(image_info->custom_stream->signature == MagickCoreSignature);
  assert(image_info->custom_stream->reader != (CustomStreamHandler) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  blob_info=CloneImageInfo(image_info);
  if (*blob_info->magick == '\0')
    (void) SetImageInfo(blob_info,0,exception);
  magick_info=GetMagickInfo(blob_info->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
        blob_info->magick);
      blob_info=DestroyImageInfo(blob_info);
      return((Image *) NULL);
    }
  image=(Image *) NULL;
  if ((GetMagickBlobSupport(magick_info) != MagickFalse) ||
      (*blob_info->filename != '\0'))
    {
      char
        filename[MagickPathExtent];

      /*
        Native blob support for this image format or SetImageInfo changed the
        blob to a file.
      */
      (void) CopyMagickString(filename,blob_info->filename,MagickPathExtent);
      (void) FormatLocaleString(blob_info->filename,MagickPathExtent,"%s:%s",
        blob_info->magick,filename);
      image=ReadImage(blob_info,exception);
    }
  else
    {
      char
        unique[MagickPathExtent];

      int
        file;

      ImageInfo
        *clone_info;

      unsigned char
        *blob;

      /*
        Write data to file on disk.
      */
      blob_info->custom_stream=(CustomStreamInfo *) NULL;
      blob=(unsigned char *) AcquireQuantumMemory(MagickMaxBufferExtent,
        sizeof(*blob));
      if (blob == (unsigned char *) NULL)
        {
          ThrowFileException(exception,BlobError,"UnableToReadBlob",
            image_info->filename);
          blob_info=DestroyImageInfo(blob_info);
          return((Image *) NULL);
        }
      file=AcquireUniqueFileResource(unique);
      if (file == -1)
        {
          ThrowFileException(exception,BlobError,"UnableToReadBlob",
            image_info->filename);
          blob=(unsigned char *) RelinquishMagickMemory(blob);
          blob_info=DestroyImageInfo(blob_info);
          return((Image *) NULL);
        }
      clone_info=CloneImageInfo(blob_info);
      blob_info->file=fdopen(file,"wb+");
      if (blob_info->file != (FILE *) NULL)
        {
          ssize_t
            count;

          count=(ssize_t) MagickMaxBufferExtent;
          while (count == (ssize_t) MagickMaxBufferExtent)
          {
            count=image_info->custom_stream->reader(blob,MagickMaxBufferExtent,
              image_info->custom_stream->data);
            count=(ssize_t) write(file,(const char *) blob,(size_t) count);
          }
          (void) fclose(blob_info->file);
          (void) FormatLocaleString(clone_info->filename,MagickPathExtent,
            "%s:%s",blob_info->magick,unique);
          image=ReadImage(clone_info,exception);
          if (image != (Image *) NULL)
            RestoreCustomStreamFilenames(image_info,magick_info,image);
        }
      clone_info=DestroyImageInfo(clone_info);
      blob=(unsigned char *) RelinquishMagickMemory(blob);
      (void) RelinquishUniqueFileResource(unique);
    }
  blob_info=DestroyImageInfo(blob_info);
  if (image != (Image *) NULL)
    if (CloseBlob(image) == MagickFalse)
      image=DestroyImageList(image);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e T o B l o b                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToBlob() returns the contents of a file as a buffer terminated with
%  the '\0' character.  The length of the buffer (not including the extra
%  terminating '\0' character) is returned via the 'length' parameter.  Free
%  the buffer with RelinquishMagickMemory().
%
%  The format of the FileToBlob method is:
%
%      void *FileToBlob(const char *filename,const size_t extent,
%        size_t *length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o blob:  FileToBlob() returns the contents of a file as a blob.  If
%      an error occurs NULL is returned.
%
%    o filename: the filename.
%
%    o extent:  The maximum length of the blob.
%
%    o length: On return, this reflects the actual length of the blob.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsUnseekableBlobFile(const int file,
  const MagickOffsetType offset)
{
  return(((file == fileno(stdin)) || (offset < 0) ||
    (offset != (MagickOffsetType) ((ssize_t) offset))) ? MagickTrue :
    MagickFalse);
}

MagickExport void *FileToBlob(const char *filename,const size_t extent,
  size_t *length,ExceptionInfo *exception)
{
  int
    file;

  MagickBooleanType
    status;

  MagickOffsetType
    offset;

  size_t
    i;

  ssize_t
    count;

  struct stat
    attributes;

  unsigned char
    *blob;

  void
    *map;

  assert(filename != (const char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  *length=0;
  if (IsPathAuthorized(ReadPolicyRights,filename) == MagickFalse)
    ThrowPolicyException(filename,NULL);
  file=fileno(stdin);
  if (LocaleCompare(filename,"-") != 0)
    {
      int
        flags = O_RDONLY | O_CLOEXEC | O_BINARY;

      status=GetPathAttributes(filename,&attributes);
      if ((status == MagickFalse) || (S_ISDIR(attributes.st_mode) != 0))
        {
          ThrowFileException(exception,BlobError,"UnableToReadBlob",filename);
          return(NULL);
        }
      file=open_utf8(filename,flags,0);
    }
  if (file == -1)
    {
      ThrowFileException(exception,BlobError,"UnableToOpenFile",filename);
      return(NULL);
    }
  if (IsPathAuthorized(ReadPolicyRights,filename) == MagickFalse)
    {
      file=close_utf8(file)-1;
      ThrowPolicyException(filename,NULL);
    }
  offset=(MagickOffsetType) lseek(file,0,SEEK_END);
  count=0;
  if (IsUnseekableBlobFile(file,offset) != MagickFalse)
    {
      size_t
        quantum;

      struct stat
        file_stats;

      /*
        Stream is not seekable.
      */
      offset=(MagickOffsetType) lseek(file,0,SEEK_SET);
      quantum=(size_t) MagickMaxBufferExtent;
      if ((fstat(file,&file_stats) == 0) && (file_stats.st_size > 0))
        quantum=(size_t) MagickMin(file_stats.st_size,MagickMaxBufferExtent);
      blob=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*blob));
      for (i=0; blob != (unsigned char *) NULL; i+=(size_t) count)
      {
        count=MagickRead(file,blob+i,quantum);
        if (count <= 0)
          break;
        if (~i < ((size_t) count+quantum+1))
          {
            blob=(unsigned char *) RelinquishMagickMemory(blob);
            break;
          }
        blob=(unsigned char *) ResizeQuantumMemory(blob,i+(size_t) count+
          quantum+1,sizeof(*blob));
        if ((i+(size_t) count) >= extent)
          break;
      }
      if (LocaleCompare(filename,"-") != 0)
        file=close_utf8(file);
      if (blob == (unsigned char *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
          return(NULL);
        }
      if ((count < 0) || (file == -1))
        {
          blob=(unsigned char *) RelinquishMagickMemory(blob);
          ThrowFileException(exception,BlobError,"UnableToReadBlob",filename);
          return(NULL);
        }
      *length=(size_t) MagickMin(i+(size_t) count,extent);
      blob[*length]='\0';
      return(blob);
    }
  *length=(size_t) MagickMin(offset,(MagickOffsetType)
    MagickMin(extent,(size_t) MAGICK_SSIZE_MAX));
  blob=(unsigned char *) NULL;
  if (~(*length) >= (MagickPathExtent-1))
    blob=(unsigned char *) AcquireQuantumMemory(*length+MagickPathExtent,
      sizeof(*blob));
  if (blob == (unsigned char *) NULL)
    {
      file=close_utf8(file);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
      return(NULL);
    }
  map=MapBlob(file,ReadMode,0,*length);
  if (map != (unsigned char *) NULL)
    {
      (void) memcpy(blob,map,*length);
      (void) UnmapBlob(map,*length);
    }
  else
    {
      (void) lseek(file,0,SEEK_SET);
      for (i=0; i < *length; i+=(size_t) count)
      {
        count=MagickRead(file,blob+i,(size_t) MagickMin(*length-i,(size_t)
          MagickMaxBufferExtent));
        if (count <= 0)
          break;
      }
      if (i < *length)
        {
          file=close_utf8(file)-1;
          blob=(unsigned char *) RelinquishMagickMemory(blob);
          ThrowFileException(exception,BlobError,"UnableToReadBlob",filename);
          return(NULL);
        }
    }
  blob[*length]='\0';
  if (LocaleCompare(filename,"-") != 0)
    file=close_utf8(file);
  if (file == -1)
    {
      blob=(unsigned char *) RelinquishMagickMemory(blob);
      ThrowFileException(exception,BlobError,"UnableToReadBlob",filename);
    }
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m a g e T o B l o b                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImageToBlob() implements direct to memory image formats.  It returns the
%  image as a formatted blob and its length.  The magick member of the Image
%  structure determines the format of the returned blob (GIF, JPEG, PNG,
%  etc.).  This method is the equivalent of WriteImage(), but writes the
%  formatted "file" to a memory buffer rather than to an actual file.
%
%  The format of the ImageToBlob method is:
%
%      void *ImageToBlob(const ImageInfo *image_info,Image *image,
%        size_t *length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o length: return the actual length of the blob.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void *DetachImageBlob(ImageInfo *blob_info,Image *image,
  const MagickBooleanType status,size_t *length)
{
  void
    *blob;

  blob=DetachBlob(image->blob);
  if (blob != (void *) NULL)
    {
      if (status == MagickFalse)
        blob=RelinquishMagickMemory(blob);
      else
        blob=ResizeQuantumMemory(blob,*length+1,sizeof(unsigned char));
    }
  else
    if ((status == MagickFalse) && (image->blob->extent == 0))
      blob_info->blob=RelinquishMagickMemory(blob_info->blob);
  return(blob);
}

MagickExport void *ImageToBlob(const ImageInfo *image_info,
  Image *image,size_t *length,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  ImageInfo
    *blob_info;

  MagickBooleanType
    status;

  void
    *blob;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  *length=0;
  blob=(unsigned char *) NULL;
  blob_info=CloneImageInfo(image_info);
  blob_info->adjoin=MagickFalse;
  (void) SetImageInfo(blob_info,1,exception);
  if (*blob_info->magick != '\0')
    (void) CopyMagickString(image->magick,blob_info->magick,MagickPathExtent);
  magick_info=GetMagickInfo(image->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"NoEncodeDelegateForThisImageFormat","`%s'",
        image->magick);
      blob_info=DestroyImageInfo(blob_info);
      return(blob);
    }
  (void) CopyMagickString(blob_info->magick,image->magick,MagickPathExtent);
  if (GetMagickBlobSupport(magick_info) != MagickFalse)
    {
      /*
        Native blob support for this image format.
      */
      blob_info->length=0;
      blob_info->blob=AcquireQuantumMemory(MagickMaxBlobExtent,
        sizeof(unsigned char));
      if (blob_info->blob == NULL)
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      else
        {
          (void) CloseBlob(image);
          image->blob->exempt=MagickTrue;
          image->blob->extent=0;
          *image->filename='\0';
          status=WriteImage(blob_info,image,exception);
          SyncBlobStream(blob_info,image,image);
          *length=image->blob->length;
          blob=DetachImageBlob(blob_info,image,status,length);
        }
    }
  else
    {
      char
        unique[MagickPathExtent];

      int
        file;

      /*
        Write file to disk in blob image format.
      */
      file=AcquireUniqueFileResource(unique);
      if (file == -1)
        {
          ThrowFileException(exception,BlobError,"UnableToWriteBlob",
            image_info->filename);
        }
      else
        {
          blob_info->file=fdopen(file,"wb");
          if (blob_info->file != (FILE *) NULL)
            {
              (void) FormatLocaleString(image->filename,MagickPathExtent,
                "%s:%s",image->magick,unique);
              status=WriteImage(blob_info,image,exception);
              (void) fclose(blob_info->file);
              if (status != MagickFalse)
                blob=FileToBlob(unique,SIZE_MAX,length,exception);
            }
          (void) RelinquishUniqueFileResource(unique);
        }
    }
  blob_info=DestroyImageInfo(blob_info);
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  I m a g e T o C u s t o m S t r e a m                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImageToCustomStream() is the equivalent of WriteImage(), but writes the
%  formatted "file" to the custom stream rather than to an actual file.
%
%  The format of the ImageToCustomStream method is:
%
%      void ImageToCustomStream(const ImageInfo *image_info,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType GetCustomStreamBlobSupport(
  const ImageInfo *clone_info,const MagickInfo *magick_info)
{
  MagickBooleanType
    blob_support;

  blob_support=GetMagickBlobSupport(magick_info);
  if ((blob_support != MagickFalse) &&
      (GetMagickEncoderSeekableStream(magick_info) != MagickFalse))
    {
      if ((clone_info->custom_stream->seeker == (CustomStreamSeeker) NULL) ||
          (clone_info->custom_stream->teller == (CustomStreamTeller) NULL))
        blob_support=MagickFalse;
    }
  return(blob_support);
}

static void WriteFileToCustomStream(const ImageInfo *image_info,
  ImageInfo *clone_info,unsigned char *blob)
{
  ssize_t
    count;

  (void) fseek(clone_info->file,0,SEEK_SET);
  count=(ssize_t) MagickMaxBufferExtent;
  while (count == (ssize_t) MagickMaxBufferExtent)
  {
    count=(ssize_t) fread(blob,sizeof(*blob),MagickMaxBufferExtent,
      clone_info->file);
    (void) image_info->custom_stream->writer(blob,(size_t) count,
      image_info->custom_stream->data);
  }
}

MagickExport void ImageToCustomStream(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  ImageInfo
    *clone_info;

  MagickBooleanType
    blob_support,
    status;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image_info->custom_stream != (CustomStreamInfo *) NULL);
  assert(image_info->custom_stream->signature == MagickCoreSignature);
  assert(image_info->custom_stream->writer != (CustomStreamHandler) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  clone_info=CloneImageInfo(image_info);
  clone_info->adjoin=MagickFalse;
  (void) SetImageInfo(clone_info,1,exception);
  if (*clone_info->magick != '\0')
    (void) CopyMagickString(image->magick,clone_info->magick,MagickPathExtent);
  magick_info=GetMagickInfo(image->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"NoEncodeDelegateForThisImageFormat","`%s'",
        image->magick);
      clone_info=DestroyImageInfo(clone_info);
      return;
    }
  (void) CopyMagickString(clone_info->magick,image->magick,MagickPathExtent);
  blob_support=GetCustomStreamBlobSupport(clone_info,magick_info);
  if (blob_support != MagickFalse)
    {
      /*
        Native blob support for this image format.
      */
      (void) CloseBlob(image);
      *image->filename='\0';
      (void) WriteImage(clone_info,image,exception);
    }
  else
    {
      char
        unique[MagickPathExtent];

      int
        file;

      unsigned char
        *blob;

      /*
        Write file to disk in blob image format.
      */
      clone_info->custom_stream=(CustomStreamInfo *) NULL;
      blob=(unsigned char *) AcquireQuantumMemory(MagickMaxBufferExtent,
        sizeof(*blob));
      if (blob == (unsigned char *) NULL)
        {
          ThrowFileException(exception,BlobError,"UnableToWriteBlob",
            image_info->filename);
          clone_info=DestroyImageInfo(clone_info);
          return;
        }
      file=AcquireUniqueFileResource(unique);
      if (file == -1)
        {
          ThrowFileException(exception,BlobError,"UnableToWriteBlob",
            image_info->filename);
          blob=(unsigned char *) RelinquishMagickMemory(blob);
          clone_info=DestroyImageInfo(clone_info);
          return;
        }
      clone_info->file=fdopen(file,"wb+");
      if (clone_info->file != (FILE *) NULL)
        {
          (void) FormatLocaleString(image->filename,MagickPathExtent,"%s:%s",
            image->magick,unique);
          status=WriteImage(clone_info,image,exception);
          if (status != MagickFalse)
            WriteFileToCustomStream(image_info,clone_info,blob);
          (void) fclose(clone_info->file);
        }
      blob=(unsigned char *) RelinquishMagickMemory(blob);
      (void) RelinquishUniqueFileResource(unique);
    }
  clone_info=DestroyImageInfo(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m a g e T o F i l e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImageToFile() writes an image to a file.  It returns MagickFalse if an error
%  occurs otherwise MagickTrue.
%
%  The format of the ImageToFile method is:
%
%       MagickBooleanType ImageToFile(Image *image,char *filename,
%         ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o filename: Write the image to this file.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ImageToFile(Image *image,char *filename,
  ExceptionInfo *exception)
{
  int
    file;

  const unsigned char
    *p;

  size_t
    i;

  size_t
    length,
    quantum;

  ssize_t
    count;

  struct stat
    file_stats;

  unsigned char
    *buffer;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  assert(filename != (const char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  if (*filename == '\0')
    file=AcquireUniqueFileResource(filename);
  else
    if (LocaleCompare(filename,"-") == 0)
      file=fileno(stdout);
    else
      file=open_utf8(filename,O_RDWR | O_CLOEXEC | O_CREAT | O_EXCL | O_BINARY,P_MODE);
  if (file == -1)
    {
      ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
      return(MagickFalse);
    }
  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(file,&file_stats) == 0) && (file_stats.st_size > 0))
    quantum=(size_t) MagickMin(file_stats.st_size,MagickMaxBufferExtent);
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      file=close_utf8(file)-1;
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationError","`%s'",filename);
      return(MagickFalse);
    }
  length=0;
  p=(const unsigned char *) ReadBlobStream(image,quantum,buffer,&count);
  for (i=0; count > 0; )
  {
    length=(size_t) count;
    for (i=0; i < length; i+=(size_t) count)
    {
      count=MagickWrite(file,p+i,(size_t) (length-i));
      if (count <= 0)
        break;
    }
    if (i < length)
      break;
    p=(const unsigned char *) ReadBlobStream(image,quantum,buffer,&count);
  }
  if (LocaleCompare(filename,"-") != 0)
    file=close_utf8(file);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  if ((file == -1) || (i < length))
    {
      if (file != -1)
        file=close_utf8(file);
      ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m a g e s T o B l o b                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImagesToBlob() implements direct to memory image formats.  It returns the
%  image sequence as a blob and its length.  The magick member of the ImageInfo
%  structure determines the format of the returned blob (GIF, JPEG,  PNG, etc.)
%
%  Note, some image formats do not permit multiple images to the same image
%  stream (e.g. JPEG).  in this instance, just the first image of the
%  sequence is returned as a blob.
%
%  The format of the ImagesToBlob method is:
%
%      void *ImagesToBlob(const ImageInfo *image_info,Image *images,
%        size_t *length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o images: the image list.
%
%    o length: return the actual length of the blob.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _TemporaryBlobWrite
{
  ImageInfo
    *blob_info;

  Image
    *images;

  size_t
    *length;

  ExceptionInfo
    *exception;
} TemporaryBlobWrite;

static void *WriteImagesBlobFile(const TemporaryBlobWrite *temporary,
  const char *unique,void *blob)
{
  ImageInfo
    *const blob_info = temporary->blob_info;

  Image
    *const images = temporary->images;

  size_t
    *const length = temporary->length;

  ExceptionInfo
    *const exception = temporary->exception;

  char
    filename[MagickPathExtent];

  MagickBooleanType
    status;

  (void) FormatLocaleString(filename,MagickPathExtent,"%s:%s",
    images->magick,unique);
  status=WriteImages(blob_info,images,filename,exception);
  (void) fclose(blob_info->file);
  if (status != MagickFalse)
    blob=FileToBlob(unique,SIZE_MAX,length,exception);
  return(blob);
}

MagickExport void *ImagesToBlob(const ImageInfo *image_info,Image *images,
  size_t *length,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  ImageInfo
    *blob_info;

  MagickBooleanType
    status;

  void
    *blob;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  *length=0;
  blob=(unsigned char *) NULL;
  blob_info=CloneImageInfo(image_info);
  (void) SetImageInfo(blob_info,(unsigned int) GetImageListLength(images),
    exception);
  if (*blob_info->magick != '\0')
    (void) CopyMagickString(images->magick,blob_info->magick,MagickPathExtent);
  magick_info=GetMagickInfo(images->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"NoEncodeDelegateForThisImageFormat","`%s'",
        images->magick);
      blob_info=DestroyImageInfo(blob_info);
      return(blob);
    }
  if (GetMagickAdjoin(magick_info) == MagickFalse)
    {
      blob_info=DestroyImageInfo(blob_info);
      return(ImageToBlob(image_info,images,length,exception));
    }
  (void) CopyMagickString(blob_info->magick,images->magick,MagickPathExtent);
  if (GetMagickBlobSupport(magick_info) != MagickFalse)
    {
      /*
        Native blob support for this images format.
      */
      blob_info->length=0;
      blob_info->blob=AcquireQuantumMemory(MagickMaxBlobExtent,
        sizeof(unsigned char));
      if (blob_info->blob == (void *) NULL)
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'",images->filename);
      else
        {
          (void) CloseBlob(images);
          images->blob->exempt=MagickTrue;
          images->blob->extent=0;
          *images->filename='\0';
          status=WriteImages(blob_info,images,images->filename,exception);
          SyncBlobStream(blob_info,images,images);
          *length=images->blob->length;
          blob=DetachImageBlob(blob_info,images,status,length);
        }
    }
  else
    {
      char
        unique[MagickPathExtent];

      int
        file;

      /*
        Write file to disk in blob images format.
      */
      file=AcquireUniqueFileResource(unique);
      if (file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToWriteBlob",
            image_info->filename);
        }
      else
        {
          blob_info->file=fdopen(file,"wb");
          if (blob_info->file != (FILE *) NULL)
            {
              const TemporaryBlobWrite
                temporary = { blob_info, images, length, exception };

              blob=WriteImagesBlobFile(&temporary,unique,blob);
            }
          (void) RelinquishUniqueFileResource(unique);
        }
    }
  blob_info=DestroyImageInfo(blob_info);
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  I m a g e s T o C u s t o m B l o b                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImagesToCustomStream() is the equivalent of WriteImages(), but writes the
%  formatted "file" to the custom stream rather than to an actual file.
%
%  The format of the ImageToCustomStream method is:
%
%      void ImagesToCustomStream(const ImageInfo *image_info,Image *images,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o images: the image list.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport void ImagesToCustomStream(const ImageInfo *image_info,
  Image *images,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  ImageInfo
    *clone_info;

  MagickBooleanType
    blob_support,
    status;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(image_info->custom_stream != (CustomStreamInfo *) NULL);
  assert(image_info->custom_stream->signature == MagickCoreSignature);
  assert(image_info->custom_stream->writer != (CustomStreamHandler) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  clone_info=CloneImageInfo(image_info);
  (void) SetImageInfo(clone_info,(unsigned int) GetImageListLength(images),
    exception);
  if (*clone_info->magick != '\0')
    (void) CopyMagickString(images->magick,clone_info->magick,MagickPathExtent);
  magick_info=GetMagickInfo(images->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"NoEncodeDelegateForThisImageFormat","`%s'",
        images->magick);
      clone_info=DestroyImageInfo(clone_info);
      return;
    }
  (void) CopyMagickString(clone_info->magick,images->magick,MagickPathExtent);
  blob_support=GetCustomStreamBlobSupport(clone_info,magick_info);
  if (blob_support != MagickFalse)
    {
      /*
        Native blob support for this image format.
      */
      (void) CloseBlob(images);
      *images->filename='\0';
      (void) WriteImages(clone_info,images,images->filename,exception);
    }
  else
    {
      char
        filename[MagickPathExtent],
        unique[MagickPathExtent];

      int
        file;

      unsigned char
        *blob;

      /*
        Write file to disk in blob image format.
      */
      clone_info->custom_stream=(CustomStreamInfo *) NULL;
      blob=(unsigned char *) AcquireQuantumMemory(MagickMaxBufferExtent,
        sizeof(*blob));
      if (blob == (unsigned char *) NULL)
        {
          ThrowFileException(exception,BlobError,"UnableToWriteBlob",
            image_info->filename);
          clone_info=DestroyImageInfo(clone_info);
          return;
        }
      file=AcquireUniqueFileResource(unique);
      if (file == -1)
        {
          ThrowFileException(exception,BlobError,"UnableToWriteBlob",
            image_info->filename);
          blob=(unsigned char *) RelinquishMagickMemory(blob);
          clone_info=DestroyImageInfo(clone_info);
          return;
        }
      clone_info->file=fdopen(file,"wb+");
      if (clone_info->file != (FILE *) NULL)
        {
          (void) FormatLocaleString(filename,MagickPathExtent,"%s:%s",
            images->magick,unique);
          status=WriteImages(clone_info,images,filename,exception);
          if (status != MagickFalse)
            WriteFileToCustomStream(image_info,clone_info,blob);
          (void) fclose(clone_info->file);
        }
      blob=(unsigned char *) RelinquishMagickMemory(blob);
      (void) RelinquishUniqueFileResource(unique);
    }
  clone_info=DestroyImageInfo(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   P i n g B l o b                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PingBlob() returns all the attributes of an image or image sequence except
%  for the pixels.  It is much faster and consumes far less memory than
%  BlobToImage().  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the PingBlob method is:
%
%      Image *PingBlob(const ImageInfo *image_info,const void *blob,
%        const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o blob: the address of a character stream in one of the image formats
%      understood by ImageMagick.
%
%    o length: This size_t integer reflects the length in bytes of the blob.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static size_t PingStream(const Image *magick_unused(image),
  const void *magick_unused(pixels),const size_t columns)
{
  magick_unreferenced(image);
  magick_unreferenced(pixels);
  return(columns);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport Image *PingBlob(const ImageInfo *image_info,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  Image
    *image;

  ImageInfo
    *clone_info,
    *ping_info;

  MagickBooleanType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if ((blob == (const void *) NULL) || (length == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),BlobError,
        "ZeroLengthBlobNotPermitted","`%s'",image_info->filename);
      return((Image *) NULL);
    }
  ping_info=CloneImageInfo(image_info);
  ping_info->blob=(void *) blob;
  ping_info->length=length;
  ping_info->ping=MagickTrue;
  if (*ping_info->magick == '\0')
    (void) SetImageInfo(ping_info,0,exception);
  magick_info=GetMagickInfo(ping_info->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
        ping_info->magick);
      ping_info=DestroyImageInfo(ping_info);
      return((Image *) NULL);
    }
  if (GetMagickBlobSupport(magick_info) != MagickFalse)
    {
      char
        filename[MagickPathExtent];

      /*
        Native blob support for this image format.
      */
      (void) CopyMagickString(filename,ping_info->filename,MagickPathExtent);
      (void) FormatLocaleString(ping_info->filename,MagickPathExtent,"%s:%s",
        ping_info->magick,filename);
      image=ReadStream(ping_info,&PingStream,exception);
      if (image != (Image *) NULL)
        (void) DetachBlob(image->blob);
      ping_info=DestroyImageInfo(ping_info);
      return(image);
    }
  /*
    Write blob to a temporary file on disk.
  */
  ping_info->blob=(void *) NULL;
  ping_info->length=0;
  *ping_info->filename='\0';
  status=BlobToFile(ping_info->filename,blob,length,exception);
  if (status == MagickFalse)
    {
      (void) RelinquishUniqueFileResource(ping_info->filename);
      ping_info=DestroyImageInfo(ping_info);
      return((Image *) NULL);
    }
  clone_info=CloneImageInfo(ping_info);
  (void) FormatLocaleString(clone_info->filename,MagickPathExtent,"%s:%s",
    ping_info->magick,ping_info->filename);
  image=ReadStream(clone_info,&PingStream,exception);
  if (image != (Image *) NULL)
    RestoreCustomStreamFilenames(image_info,magick_info,image);
  clone_info=DestroyImageInfo(clone_info);
  (void) RelinquishUniqueFileResource(ping_info->filename);
  ping_info=DestroyImageInfo(ping_info);
  return(image);
}
