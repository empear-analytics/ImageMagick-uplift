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
%                         BBBB   L       OOO   BBBB                           %
%                         B   B  L      O   O  B   B                          %
%                         BBBB   L      O   O  BBBB                           %
%                         B   B  L      O   O  B   B                          %
%                         BBBB   LLLLL   OOO   BBBB                           %
%                                                                             %
%                                                                             %
%                      MagickCore Image to Blob Methods                       %
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

static void *WriteImageMemoryBlob(ImageInfo *blob_info,Image *image,
  size_t *length,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  void
    *blob;

  (void) CloseBlob(image);
  image->blob->exempt=MagickTrue;
  image->blob->extent=0;
  *image->filename='\0';
  status=WriteImage(blob_info,image,exception);
  SyncBlobStream(blob_info,image,image);
  *length=image->blob->length;
  blob=DetachImageBlob(blob_info,image,status,length);
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
        blob=WriteImageMemoryBlob(blob_info,image,length,exception);
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
