/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     CCCC   OOO   N   N  SSSSS  TTTTT  IIIII  TTTTT  U   U  TTTTT  EEEEE     %
%    C      O   O  NN  N  SS       T      I      T    U   U    T    E         %
%    C      O   O  N N N  ESSS     T      I      T    U   U    T    EEE       %
%    C      O   O  N  NN     SS    T      I      T    U   U    T    E         %
%     CCCC   OOO   N   N  SSSSS    T    IIIII    T     UUU     T    EEEEE     %
%                                                                             %
%                                                                             %
%                  MagickCore Methods to Constitute an Image                  %
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

/*
  Define declarations.
*/
#define MaxReadRecursionDepth  100



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i n g I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PingImage() returns all the properties of an image or image sequence
%  except for the pixels.  It is much faster and consumes far less memory
%  than ReadImage().  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the PingImage method is:
%
%      Image *PingImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Ping the image defined by the file or filename members of
%      this structure.
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

MagickExport Image *PingImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *ping_info;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  ping_info=CloneImageInfo(image_info);
  ping_info->ping=MagickTrue;
  image=ReadStream(ping_info,&PingStream,exception);
  if (image != (Image *) NULL)
    {
      if ((image->columns == 0) || (image->rows == 0))
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
      ResetTimer(&image->timer);
      if (ping_info->verbose != MagickFalse)
        (void) IdentifyImage(image,stdout,MagickFalse,exception);
    }
  ping_info=DestroyImageInfo(ping_info);
  return(image);
}

static Image *PingSceneImages(const ImageInfo *image_info,
  ImageInfo *read_info,char *ping_filename,ExceptionInfo *exception)
{
  Image
    *image,
    *images;

  ssize_t
    extent,
    scene;

  (void) CopyMagickString(ping_filename,read_info->filename,
    MagickPathExtent);
  images=NewImageList();
  extent=(ssize_t) (read_info->scene+read_info->number_scenes);
  for (scene=(ssize_t) read_info->scene; scene < (ssize_t) extent; scene++)
  {
    (void) InterpretImageFilename(image_info,(Image *) NULL,ping_filename,
      (int) scene,read_info->filename,exception);
    image=PingImage(read_info,exception);
    if (image == (Image *) NULL)
      continue;
    AppendImageToList(&images,image);
  }
  return(images);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i n g I m a g e s                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PingImages() pings one or more images and returns them as an image list.
%
%  The format of the PingImage method is:
%
%      Image *PingImages(ImageInfo *image_info,const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o filename: the image filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *PingImages(ImageInfo *image_info,const char *filename,
  ExceptionInfo *exception)
{
  char
    ping_filename[MagickPathExtent];

  Image
    *images;

  ImageInfo
    *read_info;

  /*
    Ping image list from a file.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  (void) SetImageOption(image_info,"filename",filename);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) InterpretImageFilename(image_info,(Image *) NULL,image_info->filename,
    (int) image_info->scene,ping_filename,exception);
  if (LocaleCompare(ping_filename,image_info->filename) != 0)
    {
      ExceptionInfo
        *sans;

      /*
        Images of the form image-%d.png[1-5].
      */
      read_info=CloneImageInfo(image_info);
      sans=AcquireExceptionInfo();
      (void) SetImageInfo(read_info,0,sans);
      sans=DestroyExceptionInfo(sans);
      if (read_info->number_scenes == 0)
        {
          read_info=DestroyImageInfo(read_info);
          return(PingImage(image_info,exception));
        }
      images=PingSceneImages(image_info,read_info,ping_filename,exception);
      read_info=DestroyImageInfo(read_info);
      return(images);
    }
  return(PingImage(image_info,exception));
}


static Image *ReadSceneImages(const ImageInfo *image_info,
  ImageInfo *read_info,char *read_filename,ExceptionInfo *exception)
{
  Image
    *image,
    *images;

  ssize_t
    extent,
    scene;

  (void) CopyMagickString(read_filename,read_info->filename,
    MagickPathExtent);
  images=NewImageList();
  extent=(ssize_t) (read_info->scene+read_info->number_scenes);
  scene=(ssize_t) read_info->scene;
  for ( ; scene < (ssize_t) extent; scene++)
  {
    (void) InterpretImageFilename(image_info,(Image *) NULL,
      read_filename,(int) scene,read_info->filename,exception);
    image=ReadImage(read_info,exception);
    if (image == (Image *) NULL)
      continue;
    AppendImageToList(&images,image);
  }
  return(images);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d I m a g e s                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadImages() reads one or more images and returns them as an image list.
%
%  The format of the ReadImage method is:
%
%      Image *ReadImages(ImageInfo *image_info,const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o filename: the image filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *ReadImages(ImageInfo *image_info,const char *filename,
  ExceptionInfo *exception)
{
  char
    read_filename[MagickPathExtent];

  Image
    *image,
    *images;

  ImageInfo
    *read_info;

  /*
    Read image list from a file.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  read_info=CloneImageInfo(image_info);
  *read_info->magick='\0';
  (void) SetImageOption(read_info,"filename",filename);
  (void) CopyMagickString(read_info->filename,filename,MagickPathExtent);
  (void) InterpretImageFilename(read_info,(Image *) NULL,filename,
    (int) read_info->scene,read_filename,exception);
  if (LocaleCompare(read_filename,read_info->filename) != 0)
    {
      ExceptionInfo
        *sans;

      /*
        Images of the form image-%d.png[1-5].
      */
      sans=AcquireExceptionInfo();
      (void) SetImageInfo(read_info,0,sans);
      sans=DestroyExceptionInfo(sans);
      if (read_info->number_scenes != 0)
        {
          images=ReadSceneImages(image_info,read_info,read_filename,
            exception);
          read_info=DestroyImageInfo(read_info);
          return(images);
        }
    }
  (void) CopyMagickString(read_info->filename,filename,MagickPathExtent);
  image=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d I n l i n e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadInlineImage() reads a Base64-encoded inline image or image sequence.
%  The method returns a NULL if there is a memory shortage or if the image
%  cannot be read.  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the ReadInlineImage method is:
%
%      Image *ReadInlineImage(const ImageInfo *image_info,const char *content,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o content: the image encoded in Base64.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsDataTypeChar(const char *p)
{
  return(((*p != '\0') && (*p != ';') && (*p != ',')) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsMissingDataSubtype(const char *slash,
  const char *p)
{
  return(((slash == (const char *) NULL) || ((slash+1) >= p)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsImplicitDataFormat(
  const MagickInfo *magick_info)
{
  return(((magick_info == (const MagickInfo *) NULL) ||
    (magick_info->format_type == ImplicitFormatType)) ? MagickTrue :
    MagickFalse);
}

static void NormalizeDataImageType(char *magick)
{
  char
    *plus;

  plus=strchr(magick,'+');
  if (plus != (char *) NULL)
    *plus='\0';
  if (LocaleNCompare(magick,"x-",2) == 0)
    (void) memmove(magick,magick+2,strlen(magick+2)+1);
}

static MagickBooleanType GetImplicitDataImageType(const char *data_url,
  char *magick,ExceptionInfo *exception)
{
  const char
    *p,
    *type,
    *slash;

  const MagickInfo
    *magick_info;

  size_t
    offset;

  if (data_url == (const char *) NULL)
    return(MagickFalse);
  if (strncmp(data_url,"data:",5) != 0)
    return(MagickFalse);
  type=data_url+5;
  p=type;
  while (IsDataTypeChar(p) != MagickFalse)
    p++;
  if (p == type)
    return(MagickFalse);
  slash=(const char *) memchr(type,'/',(size_t) (p-type));
  if (IsMissingDataSubtype(slash,p) != MagickFalse)
    return(MagickFalse);
  offset=(size_t) (p-(slash+1));
  if (offset >= MagickPathExtent)
    return(MagickFalse);
  (void) memcpy(magick,slash+1,offset);
  magick[offset]='\0';
  NormalizeDataImageType(magick);
  magick_info=GetMagickInfo(magick,exception);
  if (IsImplicitDataFormat(magick_info) != MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

MagickExport Image *ReadInlineImage(const ImageInfo *image_info,
  const char *content,ExceptionInfo *exception)
{
  const char
    *p;

  Image
    *image = (Image *) NULL;

  ImageInfo
    *read_info;

  size_t
    length;

  unsigned char
    *blob;

  /*
    Skip over header (e.g. data:image/gif;base64,).
  */
  image=NewImageList();
  for (p=content; (*p != ',') && (*p != '\0'); p++) ;
  if (*p == '\0')
    ThrowReaderException(CorruptImageError,"CorruptImage");
  blob=Base64Decode(++p,&length);
  if (length == 0)
    {
      blob=(unsigned char *) RelinquishMagickMemory(blob);
      ThrowReaderException(CorruptImageError,"CorruptImage");
    }
  read_info=CloneImageInfo(image_info);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  *read_info->filename='\0';
  if (GetImplicitDataImageType(content,read_info->magick,exception) == MagickFalse)
    {
      blob=(unsigned char *) RelinquishMagickMemory(blob);
      read_info=DestroyImageInfo(read_info);
      ThrowReaderException(ImageError,"ImageTypeNotSupported");
    }
  image=BlobToImage(read_info,blob,length,exception);
  blob=(unsigned char *) RelinquishMagickMemory(blob);
  read_info=DestroyImageInfo(read_info);
  return(image);
}

