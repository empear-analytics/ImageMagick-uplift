/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     CCCC   OOO   N   N  SSSSS  TTTTT  IIIII  TTTTT  U   U  TTTTT  EEEEE     %
%    C      O   O  NN  N  SS       T      I      T    U   U    T    E         %
%    C      O   O  N N N   SSS     T      I      T    U   U    T    EEE       %
%    C      O   O  N  NN     SS    T      I      T    U   U    T    E         %
%     CCCC   OOO   N   N  SSSSS    T    IIIII    T     UUU     T    EEEEE     %
%                                                                             %
%                  EEEEE  N   N   CCCC   OOO   DDDD   EEEEE                   %
%                  E      NN  N  C      O   O  D   D  E                       %
%                  EEE    N N N  C      O   O  D   D  EEE                     %
%                  E      N  NN  C      O   O  D   D  E                       %
%                  EEEEE  N   N   CCCC   OOO   DDDD   EEEEE                   %
%                                                                             %
%                                                                             %
%                       MagickCore Image Encode Methods                       %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               October 1998                                  %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/client.h"
#include "MagickCore/coder-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/constitute-private.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/identify.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/policy.h"
#include "MagickCore/profile.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/constitute-codec-private.h"

/*
  Typedef declarations.
*/
typedef struct _EncodeTarget
{
  ImageInfo
    *write_info;

  Image
    *image;

  ExceptionInfo
    *exception;
} EncodeTarget;

static const MagickInfo *GetWriteMagickInfo(ImageInfo *write_info,
  const Image *image,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  ExceptionInfo
    *sans_exception;

  sans_exception=AcquireExceptionInfo();
  magick_info=GetMagickInfo(write_info->magick,sans_exception);
  if (sans_exception->severity == PolicyError)
    magick_info=GetMagickInfo(write_info->magick,exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  if ((write_info->affirm == MagickFalse) &&
      (magick_info == (const MagickInfo *) NULL))
    {
      (void) CopyMagickString(write_info->magick,image->magick,
        MagickPathExtent);
      magick_info=GetMagickInfo(write_info->magick,exception);
    }
  return(magick_info);
}

static MagickBooleanType CopyTemporaryImageFile(ImageInfo *write_info,
  Image *image,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  /*
    Copy temporary image file to permanent.
  */
  status=OpenBlob(write_info,image,ReadBinaryBlobMode,exception);
  if (status != MagickFalse)
    {
      (void) RelinquishUniqueFileResource(write_info->filename);
      status=ImageToFile(image,write_info->filename,exception);
    }
  if (CloseBlob(image) == MagickFalse)
    status=MagickFalse;
  (void) RelinquishUniqueFileResource(image->filename);
  (void) CopyMagickString(image->filename,write_info->filename,
    MagickPathExtent);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteImage() writes an image or an image sequence to a file or file handle.
%  If writing to a file is on disk, the name is defined by the filename member
%  of the image structure.  WriteImage() returns MagickFalse is there is a
%  memory shortage or if the image cannot be written.  Check the exception
%  member of image to determine the cause for any failure.
%
%  The format of the WriteImage method is:
%
%      MagickBooleanType WriteImage(const ImageInfo *image_info,Image *image,
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
static inline MagickBooleanType IsSingleImageDelegateCopy(const char *option,
  const ImageInfo *write_info,const Image *image)
{
  return((MagickBooleanType) ((IsStringTrue(option) != MagickFalse) &&
    (write_info->page == (char *) NULL) &&
    (GetPreviousImageInList(image) == (Image *) NULL) &&
    (GetNextImageInList(image) == (Image *) NULL) &&
    (IsTaintImage(image) == MagickFalse)));
}

static inline MagickBooleanType IsDirectDelegate(
  const DelegateInfo *delegate_info,const Image *image)
{
  return((MagickBooleanType) ((delegate_info != (const DelegateInfo *) NULL) &&
    (GetDelegateMode(delegate_info) == 0) &&
    (IsPathAccessible(image->magick_filename) != MagickFalse)));
}

static void SyncEndianFromMagickInfo(const ImageInfo *image_info,Image *image,
  const MagickInfo *magick_info)
{
  if (GetMagickEndianSupport(magick_info) == MagickFalse)
    image->endian=UndefinedEndian;
  else
    if ((image_info->endian == UndefinedEndian) &&
        (GetMagickRawSupport(magick_info) != MagickFalse))
      image->endian=GetHostEndian();
}

typedef struct _EncodeFallbackInfo
{
  ImageInfo
    *write_info;

  Image
    *image;

  char
    *filename;

  ExceptionInfo
    *exception;
} EncodeFallbackInfo;

static const MagickInfo *GetExtensionMagickInfo(const Image *image,
  ExceptionInfo *exception)
{
  char
    extension[MagickPathExtent];

  const MagickInfo
    *magick_info;

  GetPathComponent(image->filename,ExtensionPath,extension);
  if (*extension != '\0')
    magick_info=GetMagickInfo(extension,exception);
  else
    magick_info=GetMagickInfo(image->magick,exception);
  return(magick_info);
}

static void FindFallbackEncoder(const EncodeFallbackInfo *info,
  const MagickInfo **magick_info,EncodeImageHandler **encoder)
{
  ImageInfo
    *const write_info = info->write_info;

  Image
    *const image = info->image;

  const char
    *const filename = info->filename;

  ExceptionInfo
    *const exception = info->exception;

  *magick_info=GetWriteMagickInfo(write_info,image,exception);
  *encoder=GetImageEncoder(*magick_info);
  if (*encoder == (EncodeImageHandler *) NULL)
    {
      *magick_info=GetExtensionMagickInfo(image,exception);
      (void) CopyMagickString(image->filename,filename,
        MagickPathExtent);
      *encoder=GetImageEncoder(*magick_info);
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateWarning,"NoEncodeDelegateForThisImageFormat",
        "`%s'",write_info->magick);
    }
  if (*encoder == (EncodeImageHandler *) NULL)
    {
      *magick_info=GetMagickInfo(image->magick,exception);
      *encoder=GetImageEncoder(*magick_info);
      if (*encoder == (EncodeImageHandler *) NULL)
        (void) ThrowMagickException(exception,GetMagickModule(),
          MissingDelegateError,"NoEncodeDelegateForThisImageFormat",
          "`%s'",write_info->magick);
    }
}

static MagickBooleanType EncodeImageWithInfo(const MagickInfo *magick_info,
  EncodeImageHandler *encoder,const EncodeTarget *target)
{
  ImageInfo
    *const write_info = target->write_info;

  Image
    *const image = target->image;

  ExceptionInfo
    *const exception = target->exception;

  MagickBooleanType
    status;

  if (GetMagickEncoderThreadSupport(magick_info) == MagickFalse)
    LockSemaphoreInfo(magick_info->semaphore);
  status=IsCoderAuthorized(magick_info->magick_module,magick_info->name,
    WritePolicyRights,exception);
  if (status != MagickFalse)
    status=encoder(write_info,image,exception);
  if (GetMagickEncoderThreadSupport(magick_info) == MagickFalse)
    UnlockSemaphoreInfo(magick_info->semaphore);
  return(status);
}

static MagickBooleanType CloseSeekableWriteBlob(ImageInfo *write_info,
  Image *image,MagickBooleanType *temporary)
{
  if (IsBlobSeekable(image) == MagickFalse)
    {
      /*
        A seekable stream is required by the encoder.
      */
      write_info->adjoin=MagickTrue;
      (void) CopyMagickString(write_info->filename,image->filename,
        MagickPathExtent);
      (void) AcquireUniqueFilename(image->filename);
      *temporary=MagickTrue;
    }
  return(CloseBlob(image));
}

static MagickBooleanType InvokeLockedWriteDelegate(
  const DelegateInfo *delegate_info,ImageInfo *write_info,Image *image,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
    LockSemaphoreInfo(delegate_info->semaphore);
  status=InvokeDelegate(write_info,image,(char *) NULL,
    write_info->magick,exception);
  if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
    UnlockSemaphoreInfo(delegate_info->semaphore);
  return(status);
}

static void EncodeImageWithFallback(const EncodeFallbackInfo *info,
  MagickBooleanType *status)
{
  ImageInfo
    *const write_info = info->write_info;

  Image
    *const image = info->image;

  char
    *const filename = info->filename;

  ExceptionInfo
    *const exception = info->exception;

  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  EncodeImageHandler
    *encoder;

  delegate_info=GetDelegateInfo((char *) NULL,write_info->magick,exception);
  if (delegate_info != (DelegateInfo *) NULL)
    {
      /*
        Process the image with delegate.
      */
      *write_info->filename='\0';
      (*status)=InvokeLockedWriteDelegate(delegate_info,write_info,image,
        exception);
      (void) CopyMagickString(image->filename,filename,MagickPathExtent);
    }
  else
    {
      FindFallbackEncoder(info,&magick_info,&encoder);
      if (encoder != (EncodeImageHandler *) NULL)
        {
          const EncodeTarget
            target = { write_info, image, exception };

          /*
            Call appropriate image writer based on image type.
          */
          (*status)=EncodeImageWithInfo(magick_info,encoder,&target);
        }
    }
}

static const MagickInfo *GetEncoderMagickInfo(const ImageInfo *write_info,
  ExceptionInfo *sans_exception,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  magick_info=GetMagickInfo(write_info->magick,sans_exception);
  if (sans_exception->severity == PolicyError)
    magick_info=GetMagickInfo(write_info->magick,exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  return(magick_info);
}

MagickExport MagickBooleanType WriteImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  const char
    *option;

  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  EncodeImageHandler
    *encoder;

  ExceptionInfo
    *sans_exception;

  ImageInfo
    *write_info;

  MagickBooleanType
    status,
    temporary;

  /*
    Determine image type from filename prefix or suffix (e.g. image.jpg).
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  sans_exception=AcquireExceptionInfo();
  write_info=CloneImageInfo(image_info);
  (void) CopyMagickString(write_info->filename,image->filename,
    MagickPathExtent);
  (void) SetImageInfo(write_info,1,sans_exception);
  if (*write_info->magick == '\0')
    (void) CopyMagickString(write_info->magick,image->magick,MagickPathExtent);
  (void) CopyMagickString(filename,image->filename,MagickPathExtent);
  (void) CopyMagickString(image->filename,write_info->filename,
    MagickPathExtent);
  /*
    Call appropriate image writer based on image type.
  */
  magick_info=GetEncoderMagickInfo(write_info,sans_exception,exception);
  if (magick_info != (const MagickInfo *) NULL)
    SyncEndianFromMagickInfo(image_info,image,magick_info);
  if ((image->ping != MagickFalse) &&
      (SyncImagePixelCache(image,exception) == MagickFalse))
    {
      write_info=DestroyImageInfo(write_info);
      return(MagickFalse);
    }
  SyncImageProfiles(image);
  DisassociateImageStream(image);
  option=GetImageOption(image_info,"delegate:bimodal");
  if (IsSingleImageDelegateCopy(option,write_info,image) != MagickFalse)
    {
      delegate_info=GetDelegateInfo(image->magick,write_info->magick,exception);
      if (IsDirectDelegate(delegate_info,image) != MagickFalse)
        {
          /*
            Process image with bi-modal delegate.
          */
          (void) CopyMagickString(image->filename,image->magick_filename,
            MagickPathExtent);
          status=InvokeDelegate(write_info,image,image->magick,
            write_info->magick,exception);
          write_info=DestroyImageInfo(write_info);
          (void) CopyMagickString(image->filename,filename,MagickPathExtent);
          return(status);
        }
    }
  status=MagickFalse;
  temporary=MagickFalse;
  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetMagickEncoderSeekableStream(magick_info) != MagickFalse))
    {
      char
        image_filename[MagickPathExtent];

      (void) CopyMagickString(image_filename,image->filename,MagickPathExtent);
      status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
      (void) CopyMagickString(image->filename, image_filename,MagickPathExtent);
      if ((status != MagickFalse) &&
          (CloseSeekableWriteBlob(write_info,image,&temporary) == MagickFalse))
        status=MagickFalse;
    }
  encoder=GetImageEncoder(magick_info);
  if (encoder != (EncodeImageHandler *) NULL)
    {
      const EncodeTarget
        target = { write_info, image, exception };

      /*
        Call appropriate image writer based on image type.
      */
      status=EncodeImageWithInfo(magick_info,encoder,&target);
    }
  else
    {
      const EncodeFallbackInfo
        fallback = { write_info, image, filename, exception };

      EncodeImageWithFallback(&fallback,&status);
    }
  if (temporary != MagickFalse)
    status=CopyTemporaryImageFile(write_info,image,exception);
  if ((LocaleCompare(write_info->magick,"info") != 0) &&
      (write_info->verbose != MagickFalse))
    (void) IdentifyImage(image,stdout,MagickFalse,exception);
  write_info=DestroyImageInfo(write_info);
  if (GetBlobError(image) != MagickFalse)
    ThrowWriterException(FileOpenError,"UnableToWriteFile");
  return(status);
}
