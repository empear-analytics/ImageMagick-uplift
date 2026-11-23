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
%                       CCCC   OOO   DDDD   EEEEE   CCCC                      %
%                      C      O   O  D   D  E      C                          %
%                      C      O   O  D   D  EEE    C                          %
%                      C      O   O  D   D  E      C                          %
%                       CCCC   OOO   DDDD   EEEEE   CCCC                      %
%                                                                             %
%                                                                             %
%                   MagickCore Read and Write Image Methods                   %
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
typedef struct _InterpretedPropertyTarget
{
  ImageInfo
    *read_info;

  Image
    *next;

  ExceptionInfo
    *exception;
} InterpretedPropertyTarget;

typedef struct _ConstituteInfo
{
  const char
    *caption,
    *comment,
    *dispose,
    *label;

  MagickBooleanType
    sync_from_exif,
    sync_from_tiff;

  MagickStatusType
    delay_flags;

  size_t
    delay;

  ssize_t
    ticks_per_second;
} ConstituteInfo;

static void SyncDelayFromConstituteInfo(Image *next,
  const ConstituteInfo *constitute_info)
{
  if ((constitute_info->delay_flags & GreaterValue) != 0)
    {
      if (next->delay > constitute_info->delay)
        next->delay=constitute_info->delay;
    }
  else
    if ((constitute_info->delay_flags & LessValue) != 0)
      {
        if (next->delay < constitute_info->delay)
          next->delay=constitute_info->delay;
      }
    else
      next->delay=constitute_info->delay;
  if ((constitute_info->delay_flags & SigmaValue) != 0)
    next->ticks_per_second=constitute_info->ticks_per_second;
}

static void SetImageDateProperties(Image *next,ExceptionInfo *exception)
{
  char
    timestamp[MagickTimeExtent];

  (void) FormatMagickTime(next->timestamp,sizeof(timestamp),timestamp);
  (void) SetImageProperty(next,"date:timestamp",timestamp,exception);
  (void) FormatMagickTime((time_t) GetBlobProperties(next)->st_mtime,
    sizeof(timestamp),timestamp);
  (void) SetImageProperty(next,"date:modify",timestamp,exception);
  (void) FormatMagickTime((time_t) GetBlobProperties(next)->st_ctime,
    sizeof(timestamp),timestamp);
  (void) SetImageProperty(next,"date:create",timestamp,exception);
}

static void SyncDisposeFromConstituteInfo(Image *next,
  const ConstituteInfo *constitute_info)
{
  ssize_t
    option_type;

  option_type=ParseCommandOption(MagickDisposeOptions,MagickFalse,
    constitute_info->dispose);
  if (option_type >= 0)
    next->dispose=(DisposeType) option_type;
}

static Image *ResizeToExtractRegion(const ImageInfo *read_info,Image *next,
  RectangleInfo *geometry,ExceptionInfo *exception)
{
  Image
    *resize_image;

  (void) ParseRegionGeometry(next,read_info->extract,geometry,exception);
  if ((geometry->width != 0) && (geometry->height != 0))
    {
      resize_image=ResizeImage(next,geometry->width,geometry->height,
        next->filter,exception);
      if (resize_image != (Image *) NULL)
        ReplaceImageInList(&next,resize_image);
    }
  return(next);
}

static Image *ApplyExtractGeometry(const ImageInfo *read_info,Image *next,
  ExceptionInfo *exception)
{
  RectangleInfo
    geometry;

  MagickStatusType
    flags;

  SetGeometry(next,&geometry);
  flags=ParseAbsoluteGeometry(read_info->extract,&geometry);
  if ((next->columns != geometry.width) ||
      (next->rows != geometry.height))
    {
      if (((flags & XValue) != 0) || ((flags & YValue) != 0))
        {
          Image *crop_image = CropImage(next,&geometry,exception);
          if (crop_image != (Image *) NULL)
            ReplaceImageInList(&next,crop_image);
        }
      else
        if (((flags & WidthValue) != 0) || ((flags & HeightValue) != 0))
          next=ResizeToExtractRegion(read_info,next,&geometry,exception);
    }
  return(next);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadImage() reads an image or image sequence from a file or file handle.
%  The method returns a NULL if there is a memory shortage or if the image
%  cannot be read.  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%  The format of the ReadImage method is:
%
%      Image *ReadImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: Read the image defined by the file or filename members of
%      this structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void InitializeConstituteInfo(const ImageInfo *image_info,
  ConstituteInfo *constitute_info)
{
  const char
    *option;

  memset(constitute_info,0,sizeof(*constitute_info));
  constitute_info->sync_from_exif=MagickTrue;
  constitute_info->sync_from_tiff=MagickTrue;
  option=GetImageOption(image_info,"exif:sync-image");
  if (IsStringFalse(option) != MagickFalse)
    constitute_info->sync_from_exif=MagickFalse;
  option=GetImageOption(image_info,"tiff:sync-image");
  if (IsStringFalse(option) != MagickFalse)
    constitute_info->sync_from_tiff=MagickFalse;
  constitute_info->caption=GetImageOption(image_info,"caption");
  constitute_info->comment=GetImageOption(image_info,"comment");
  constitute_info->label=GetImageOption(image_info,"label");
  option=GetImageOption(image_info,"delay");
  if (option != (const char *) NULL)
    {
      GeometryInfo
        geometry_info;

      constitute_info->delay_flags=ParseGeometry(option,&geometry_info);
      if (constitute_info->delay_flags != NoValue)
        {
          constitute_info->delay=(size_t) floor(geometry_info.rho+0.5);
          if ((constitute_info->delay_flags & SigmaValue) != 0)
            constitute_info->ticks_per_second=CastDoubleToSsizeT(floor(
              geometry_info.sigma+0.5));
        }
    }
}

static const char *SyncOrientationProperty(Image *image,
  const char *property,ExceptionInfo *exception)
{
  const char
    *orientation;

  orientation=GetImageProperty(image,property,exception);
  if (orientation != (const char *) NULL)
    {
      image->orientation=(OrientationType) StringToLong(orientation);
      (void) DeleteImageProperty(image,property);
    }
  return(orientation);
}

static void SyncOrientationFromProperties(Image *image,
  ConstituteInfo *constitute_info,ExceptionInfo *exception)
{
  const char
    *orientation;

  orientation=(const char *) NULL;
  if (constitute_info->sync_from_exif != MagickFalse)
    {
      orientation=SyncOrientationProperty(image,"exif:Orientation",
        exception);
    }
  if ((orientation == (const char *) NULL) &&
      (constitute_info->sync_from_tiff != MagickFalse))
    {
      orientation=SyncOrientationProperty(image,"tiff:Orientation",
        exception);
    }
}

static inline MagickBooleanType IsTIFFResolutionFallback(
  const char *resolution_x,const char *resolution_y,
  const ConstituteInfo *constitute_info)
{
  return((MagickBooleanType) ((resolution_x == (const char *) NULL) &&
    (resolution_y == (const char *) NULL) &&
    (constitute_info->sync_from_tiff != MagickFalse)));
}

static void SetResolutionFromStrings(Image *image,const char *resolution_x,
  const char *resolution_y)
{
  GeometryInfo
    geometry_info;

  geometry_info.rho=image->resolution.x;
  geometry_info.sigma=1.0;
  (void) ParseGeometry(resolution_x,&geometry_info);
  if (geometry_info.sigma != 0)
    image->resolution.x=geometry_info.rho/geometry_info.sigma;
  if (strchr(resolution_x,',') != (char *) NULL)
    image->resolution.x=geometry_info.rho+geometry_info.sigma/1000.0;
  geometry_info.rho=image->resolution.y;
  geometry_info.sigma=1.0;
  (void) ParseGeometry(resolution_y,&geometry_info);
  if (geometry_info.sigma != 0)
    image->resolution.y=geometry_info.rho/geometry_info.sigma;
  if (strchr(resolution_y,',') != (char *) NULL)
    image->resolution.y=geometry_info.rho+geometry_info.sigma/1000.0;
}

static void DeleteResolutionProperties(Image *image,
  const MagickBooleanType used_tiff)
{
  if (used_tiff == MagickFalse)
    {
      (void) DeleteImageProperty(image,"exif:XResolution");
      (void) DeleteImageProperty(image,"exif:YResolution");
      (void) DeleteImageProperty(image,"exif:ResolutionUnit");
    }
  else
    {
      (void) DeleteImageProperty(image,"tiff:XResolution");
      (void) DeleteImageProperty(image,"tiff:YResolution");
      (void) DeleteImageProperty(image,"tiff:ResolutionUnit");
    }
}

static inline MagickBooleanType HasResolutionPair(const char *resolution_x,
  const char *resolution_y)
{
  return(((resolution_x != (const char *) NULL) &&
    (resolution_y != (const char *) NULL)) ? MagickTrue : MagickFalse);
}

static void SetResolutionUnitsFromString(Image *image,
  const char *resolution_units)
{
  ssize_t
    option_type;

  option_type=ParseCommandOption(MagickResolutionOptions,MagickFalse,
    resolution_units);
  if (option_type >= 0)
    image->units=(ResolutionType) option_type;
}

static void SyncResolutionUnits(Image *image,const char *resolution_units)
{
  if (resolution_units != (char *) NULL)
    SetResolutionUnitsFromString(image,resolution_units);
}

typedef struct _ResolutionSource
{
  Image
    *image;

  ExceptionInfo
    *exception;
} ResolutionSource;

static void GetExifResolution(const ResolutionSource *source,
  const char **resolution_x,const char **resolution_y,
  const char **resolution_units)
{
  Image
    *const image = source->image;

  ExceptionInfo
    *const exception = source->exception;

  (*resolution_x)=GetImageProperty(image,"exif:XResolution",exception);
  (*resolution_y)=GetImageProperty(image,"exif:YResolution",exception);
  if (HasResolutionPair((*resolution_x),(*resolution_y)) != MagickFalse)
    (*resolution_units)=GetImageProperty(image,"exif:ResolutionUnit",
      exception);
}

static void SyncResolutionFromProperties(Image *image,
  ConstituteInfo *constitute_info, ExceptionInfo *exception)
{
  const char
    *resolution_x,
    *resolution_y,
    *resolution_units;

  MagickBooleanType
    used_tiff;

  resolution_x=(const char *) NULL;
  resolution_y=(const char *) NULL;
  resolution_units=(const char *) NULL;
  used_tiff=MagickFalse;
  if (constitute_info->sync_from_exif != MagickFalse)
    {
      ResolutionSource
        source;

      source.image=image;
      source.exception=exception;
      GetExifResolution(&source,&resolution_x,&resolution_y,
        &resolution_units);
    }
  if (IsTIFFResolutionFallback(resolution_x,resolution_y,constitute_info) !=
      MagickFalse)
    {
      resolution_x=GetImageProperty(image,"tiff:XResolution",exception);
      resolution_y=GetImageProperty(image,"tiff:YResolution",exception);
      if (HasResolutionPair(resolution_x,resolution_y) != MagickFalse)
        {
          used_tiff=MagickTrue;
          resolution_units=GetImageProperty(image,"tiff:ResolutionUnit",
            exception);
        }
    }
  if (HasResolutionPair(resolution_x,resolution_y) != MagickFalse)
    {
      SetResolutionFromStrings(image,resolution_x,resolution_y);
      SyncResolutionUnits(image,resolution_units);
      DeleteResolutionProperties(image,used_tiff);
    }
}

static void SetInterpretedProperty(const InterpretedPropertyTarget *target,
  const char *name,const char *value)
{
  ImageInfo
    *const read_info = target->read_info;

  Image
    *const next = target->next;

  ExceptionInfo
    *const exception = target->exception;

  char
    *property;

  if (value != (const char *) NULL)
    {
      property=InterpretImageProperties(read_info,next,value,exception);
      (void) SetImageProperty(next,name,property,exception);
      property=DestroyString(property);
    }
}

static MagickBooleanType CopySeekableReadStream(Image *image,
  ImageInfo *read_info,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  if (IsBlobSeekable(image) == MagickFalse)
    {
      /*
        Coder requires a seekable stream.
      */
      *read_info->filename='\0';
      status=ImageToFile(image,read_info->filename,exception);
      if (status == MagickFalse)
        {
          (void) CloseBlob(image);
          read_info=DestroyImageInfo(read_info);
          image=DestroyImage(image);
          return(MagickFalse);
        }
      read_info->temporary=MagickTrue;
    }
  return(MagickTrue);
}

static MagickBooleanType OpenSeekableReadStream(const ImageInfo *image_info,
  ImageInfo *read_info,const MagickInfo *magick_info,ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    status;

  if ((magick_info != (const MagickInfo *) NULL) &&
      (GetMagickDecoderSeekableStream(magick_info) != MagickFalse))
    {
      image=AcquireImage(read_info,exception);
      (void) CopyMagickString(image->filename,read_info->filename,
        MagickPathExtent);
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          read_info=DestroyImageInfo(read_info);
          image=DestroyImage(image);
          return(MagickFalse);
        }
      if (CopySeekableReadStream(image,read_info,exception) == MagickFalse)
        return(MagickFalse);
      (void) CloseBlob(image);
      image=DestroyImage(image);
    }
  return(MagickTrue);
}

static void SetReadImageNames(Image *next,const char *magick,
  const char *magick_filename)
{
  char
    magick_path[MagickPathExtent];

  GetPathComponent(magick_filename,MagickPath,magick_path);
  if ((*magick_path == '\0') && (*next->magick == '\0'))
    (void) CopyMagickString(next->magick,magick,MagickPathExtent);
  (void) CopyMagickString(next->magick_filename,magick_filename,
    MagickPathExtent);
}

static void SyncReadImageMetadata(ImageInfo *read_info,Image *next,
  ConstituteInfo *constitute_info,ExceptionInfo *exception)
{
  const InterpretedPropertyTarget
    target = { read_info, next, exception };

  if (next->magick_columns == 0)
    next->magick_columns=next->columns;
  if (next->magick_rows == 0)
    next->magick_rows=next->rows;
  (void) GetImageProperty(next,"exif:*",exception);
  (void) GetImageProperty(next,"icc:*",exception);
  (void) GetImageProperty(next,"iptc:*",exception);
  (void) GetImageProperty(next,"xmp:*",exception);
  SyncOrientationFromProperties(next,constitute_info,exception);
  SyncResolutionFromProperties(next,constitute_info,exception);
  if (next->page.width == 0)
    next->page.width=next->columns;
  if (next->page.height == 0)
    next->page.height=next->rows;
  SetInterpretedProperty(&target,"caption",constitute_info->caption);
  SetInterpretedProperty(&target,"comment",constitute_info->comment);
  SetInterpretedProperty(&target,"label",constitute_info->label);
  if (LocaleCompare(next->magick,"TEXT") == 0)
    (void) ParseAbsoluteGeometry("0x0+0+0",&next->page);
}

static Image *SelectImageScenes(Image *image,const char *scenes,
  ExceptionInfo *exception)
{
  Image
    *clones;

  clones=CloneImages(image,scenes,exception);
  image=DestroyImageList(image);
  if (clones != (Image *) NULL)
    image=GetFirstImageInList(clones);
  return(image);
}

static MagickBooleanType InvokeDecodeDelegate(ImageInfo *read_info,
  Image *image,const DelegateInfo *delegate_info,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  (void) CopyMagickString(image->filename,read_info->filename,
    MagickPathExtent);
  *read_info->filename='\0';
  if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
    LockSemaphoreInfo(delegate_info->semaphore);
  status=InvokeDelegate(read_info,image,read_info->magick,(char *) NULL,
    exception);
  if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
    UnlockSemaphoreInfo(delegate_info->semaphore);
  return(status);
}

static Image *DecodeAuthorizedImage(const MagickInfo *magick_info,
  DecodeImageHandler *decoder,ImageInfo *read_info,ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    status;

  status=IsCoderAuthorized(magick_info->magick_module,magick_info->name,
    ReadPolicyRights,exception);
  image=(Image *) NULL;
  if (status != MagickFalse)
    image=decoder(read_info,exception);
  return(image);
}

static void SetReadEndian(const ImageInfo *image_info,ImageInfo *read_info,
  const MagickInfo *magick_info)
{
  if (GetMagickEndianSupport(magick_info) == MagickFalse)
    read_info->endian=UndefinedEndian;
  else
    if ((image_info->endian == UndefinedEndian) &&
        (GetMagickRawSupport(magick_info) != MagickFalse))
      read_info->endian=GetHostEndian();
}

static void ReleaseTemporaryReadFile(ImageInfo *read_info,Image *image,
  const char *filename)
{
  (void) RelinquishUniqueFileResource(read_info->filename);
  read_info->temporary=MagickFalse;
  if (image != (Image *) NULL)
    (void) CopyMagickString(image->filename,filename,MagickPathExtent);
}

static void LookupReadImageProfiles(const Image *next)
{
  const StringInfo
    *profile;

  profile=GetImageProfile(next,"icc");
  if (profile == (const StringInfo *) NULL)
    profile=GetImageProfile(next,"icm");
  profile=GetImageProfile(next,"iptc");
  if (profile == (const StringInfo *) NULL)
    profile=GetImageProfile(next,"8bim");
}

static inline MagickBooleanType IsExtractRequested(const ImageInfo *read_info)
{
  return(((read_info->extract != (char *) NULL) &&
    (read_info->stream == (StreamHandler) NULL)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEmptyImageSize(const Image *image)
{
  return(((image->columns == 0) || (image->rows == 0)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsSceneSelection(const ImageInfo *read_info,
  const Image *image)
{
  return(((IsSceneGeometry(read_info->scenes,MagickFalse) != MagickFalse) &&
    (GetImageListLength(image) != 1)) ? MagickTrue : MagickFalse);
}

static const MagickInfo *GetReadMagickInfo(const ImageInfo *read_info,
  ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  ExceptionInfo
    *sans_exception;

  sans_exception=AcquireExceptionInfo();
  magick_info=GetMagickInfo(read_info->magick,sans_exception);
  if (sans_exception->severity == PolicyError)
    InheritException(exception,sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  return(magick_info);
}

static void FinishReadImageMetadata(const ImageInfo *read_info,Image *next,
  const ConstituteInfo *constitute_info,ExceptionInfo *exception)
{
  LookupReadImageProfiles(next);
  if (IsSourceDataEpochSet() == MagickFalse)
    SetImageDateProperties(next,exception);
  if (constitute_info->delay_flags != NoValue)
    SyncDelayFromConstituteInfo(next,constitute_info);
  if (constitute_info->dispose != (const char *) NULL)
    SyncDisposeFromConstituteInfo(next,constitute_info);
  if (read_info->verbose != MagickFalse)
    (void) IdentifyImage(next,stderr,MagickFalse,exception);
}

static Image *DecodeLockedImage(const MagickInfo *magick_info,
  DecodeImageHandler *decoder,ImageInfo *read_info,ExceptionInfo *exception)
{
  Image
    *image;

  if (GetMagickDecoderThreadSupport(magick_info) == MagickFalse)
    LockSemaphoreInfo(magick_info->semaphore);
  image=DecodeAuthorizedImage(magick_info,decoder,read_info,exception);
  if (GetMagickDecoderThreadSupport(magick_info) == MagickFalse)
    UnlockSemaphoreInfo(magick_info->semaphore);
  return(image);
}

static void RelinquishTemporaryReadFile(const ImageInfo *read_info)
{
  if (read_info->temporary != MagickFalse)
    (void) RelinquishUniqueFileResource(read_info->filename);
}

static MagickBooleanType DecodeDelegateImage(ImageInfo *read_info,
  Image **image,ExceptionInfo *exception)
{
  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  DecodeImageHandler
    *decoder;

  MagickBooleanType
    status;

  delegate_info=GetDelegateInfo(read_info->magick,(char *) NULL,exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
        read_info->filename);
      RelinquishTemporaryReadFile(read_info);
      read_info=DestroyImageInfo(read_info);
      return(MagickFalse);
    }
  /*
    Let our decoding delegate process the image.
  */
  *image=AcquireImage(read_info,exception);
  if ((*image) == (Image *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      return(MagickFalse);
    }
  status=InvokeDecodeDelegate(read_info,(*image),delegate_info,exception);
  *image=DestroyImageList((*image));
  read_info->temporary=MagickTrue;
  if (status != MagickFalse)
    (void) SetImageInfo(read_info,0,exception);
  magick_info=GetMagickInfo(read_info->magick,exception);
  decoder=GetImageDecoder(magick_info);
  if (decoder == (DecodeImageHandler *) NULL)
    {
      if (IsPathAccessible(read_info->filename) != MagickFalse)
        (void) ThrowMagickException(exception,GetMagickModule(),
          MissingDelegateError,"NoDecodeDelegateForThisImageFormat","`%s'",
          read_info->magick);
      else
        ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
          read_info->filename);
      read_info=DestroyImageInfo(read_info);
      return(MagickFalse);
    }
  /*
    Call appropriate image reader based on image type.
  */
  *image=DecodeLockedImage(magick_info,decoder,read_info,exception);
  return(MagickTrue);
}

static MagickBooleanType HasPingImageSizes(const ImageInfo *read_info,
  Image *image)
{
  Image
    *next;

  if (read_info->ping != MagickFalse)
    {
      for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
      {
        if (IsEmptyImageSize(image) != MagickFalse)
          return(MagickFalse);
      }
    }
  return(MagickTrue);
}

typedef struct _DecodeFallbackInfo
{
  ImageInfo
    *read_info;

  char
    *filename;

  ExceptionInfo
    *exception;
} DecodeFallbackInfo;

static void FindFallbackDecoder(const DecodeFallbackInfo *info,
  const MagickInfo **magick_info,DecodeImageHandler **decoder)
{
  ImageInfo
    *const read_info = info->read_info;

  char
    *const filename = info->filename;

  ExceptionInfo
    *const exception = info->exception;

  const DelegateInfo
    *delegate_info;

  if ((*decoder) == (DecodeImageHandler *) NULL)
    {
      delegate_info=GetDelegateInfo(read_info->magick,(char *) NULL,exception);
      if (delegate_info == (const DelegateInfo *) NULL)
        {
          (void) SetImageInfo(read_info,0,exception);
          (void) CopyMagickString(read_info->filename,filename,
            MagickPathExtent);
          (*magick_info)=GetMagickInfo(read_info->magick,exception);
          (*decoder)=GetImageDecoder((*magick_info));
        }
    }
}

static void CheckReadImageArguments(const ImageInfo *image_info,
  const ExceptionInfo *exception)
{
  magick_unreferenced(exception);
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image_info->filename != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
}

static void ReportDecodedImage(const ImageInfo *read_info,const Image *image,
  const ExceptionInfo *exception)
{
  if (exception->severity >= ErrorException)
    (void) LogMagickEvent(ExceptionEvent,GetMagickModule(),
      "Coder (%s) generated an image despite an error (%d), "
      "notify the developers",image->magick,exception->severity);
  if (IsBlobTemporary(image) != MagickFalse)
    (void) RelinquishUniqueFileResource(read_info->filename);
}

typedef struct _ReadImageFinish
{
  ImageInfo
    *read_info;

  char
    *magick,
    *magick_filename,
    *filename;

  ExceptionInfo
    *exception;
} ReadImageFinish;

static Image *FinishReadImageList(const ReadImageFinish *finish,Image *image)
{
  ImageInfo
    *const read_info = finish->read_info;

  char
    *const magick = finish->magick,
    *const magick_filename = finish->magick_filename,
    *const filename = finish->filename;

  ExceptionInfo
    *const exception = finish->exception;

  ConstituteInfo
    constitute_info;

  Image
    *next;

  InitializeConstituteInfo(read_info,&constitute_info);
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    next->taint=MagickFalse;
    SetReadImageNames(next,magick,magick_filename);
    if (IsBlobTemporary(image) != MagickFalse)
      (void) CopyMagickString(next->filename,filename,MagickPathExtent);
    SyncReadImageMetadata(read_info,next,&constitute_info,exception);
    if (IsExtractRequested(read_info) != MagickFalse)
      next=ApplyExtractGeometry(read_info,next,exception);
    FinishReadImageMetadata(read_info,next,&constitute_info,exception);
    image=next;
  }
  return(image);
}

MagickExport Image *ReadImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    magick[MagickPathExtent],
    magick_filename[MagickPathExtent];

  const MagickInfo
    *magick_info;

  DecodeImageHandler
    *decoder;

  Image
    *image;

  ImageInfo
    *read_info;

  ReadImageFinish
    finish;

  /*
    Determine image type from filename prefix or suffix (e.g. image.jpg).
  */
  CheckReadImageArguments(image_info,exception);
  read_info=CloneImageInfo(image_info);
  (void) CopyMagickString(magick_filename,read_info->filename,MagickPathExtent);
  (void) SetImageInfo(read_info,0,exception);
  (void) CopyMagickString(filename,read_info->filename,MagickPathExtent);
  (void) CopyMagickString(magick,read_info->magick,MagickPathExtent);
  /*
    Call appropriate image reader based on image type.
  */
  magick_info=GetReadMagickInfo(read_info,exception);
  if (magick_info != (const MagickInfo *) NULL)
    SetReadEndian(image_info,read_info,magick_info);
  if (OpenSeekableReadStream(image_info,read_info,magick_info,exception) ==
      MagickFalse)
    return((Image *) NULL);
  image=NewImageList();
  decoder=GetImageDecoder(magick_info);
  {
    const DecodeFallbackInfo
      fallback = { read_info, filename, exception };

    FindFallbackDecoder(&fallback,&magick_info,&decoder);
  }
  if (decoder != (DecodeImageHandler *) NULL)
    {
      /*
        Call appropriate image reader based on image type.
      */
      image=DecodeLockedImage(magick_info,decoder,read_info,exception);
    }
  else
    if (DecodeDelegateImage(read_info,&image,exception) == MagickFalse)
      return((Image *) NULL);
  if (read_info->temporary != MagickFalse)
    ReleaseTemporaryReadFile(read_info,image,filename);
  if (image == (Image *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      return(image);
    }
  ReportDecodedImage(read_info,image,exception);
  if (HasPingImageSizes(read_info,image) == MagickFalse)
    {
      read_info=DestroyImageInfo(read_info);
      ThrowReaderException(ImageError,"NegativeOrZeroImageSize");
    }
  if (IsSceneSelection(read_info,image) != MagickFalse)
    {
      image=SelectImageScenes(image,read_info->scenes,exception);
      if (image == (Image *) NULL)
        {
          read_info=DestroyImageInfo(read_info);
          return(image);
        }
    }
  finish.read_info=read_info;
  finish.magick=magick;
  finish.magick_filename=magick_filename;
  finish.filename=filename;
  finish.exception=exception;
  image=FinishReadImageList(&finish,image);
  read_info=DestroyImageInfo(read_info);
  if (GetBlobError(image) != MagickFalse)
    ThrowReaderException(CorruptImageError,"UnableToReadImageData");
  return(GetFirstImageInList(image));
}


