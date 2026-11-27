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
%                                TTTTT   OOO                                  %
%                                  T    O   O                                 %
%                                  T    O   O                                 %
%                                  T    O   O                                 %
%                                  T     OOO                                  %
%                                                                             %
%                   CCCC  U   U  SSSSS  TTTTT   OOO   M   M                   %
%                  C      U   U  SS       T    O   O  MM MM                   %
%                  C      U   U   SSS     T    O   O  M M M                   %
%                  C      U   U     SS    T    O   O  M   M                   %
%                   CCCC   UUU   SSSSS    T     OOO   M   M                   %
%                                                                             %
%                                                                             %
%                  MagickCore Image to Custom Stream Methods                  %
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
