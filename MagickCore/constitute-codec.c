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

/*
  Typedef declarations.
*/
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

static MagickBooleanType IsCoderAuthorized(const char *module,
  const char *coder,const PolicyRights rights,ExceptionInfo *exception)
{
  if (IsRightsAuthorized(CoderPolicyDomain,rights,coder) == MagickFalse)
    ThrowPolicyException(coder,MagickFalse);
  if (IsRightsAuthorized(ModulePolicyDomain,rights,module) == MagickFalse)
    ThrowPolicyException(module,MagickFalse);
  return(MagickTrue);
}

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
      resolution_x=GetImageProperty(image,"exif:XResolution",exception);
      resolution_y=GetImageProperty(image,"exif:YResolution",exception);
      if (HasResolutionPair(resolution_x,resolution_y) != MagickFalse)
        resolution_units=GetImageProperty(image,"exif:ResolutionUnit",
          exception);
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
      if (resolution_units != (char *) NULL)
        SetResolutionUnitsFromString(image,resolution_units);
      DeleteResolutionProperties(image,used_tiff);
    }
}

static void SetInterpretedProperty(ImageInfo *read_info,Image *next,
  const char *name,const char *value,ExceptionInfo *exception)
{
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
  SetInterpretedProperty(read_info,next,"caption",constitute_info->caption,
    exception);
  SetInterpretedProperty(read_info,next,"comment",constitute_info->comment,
    exception);
  SetInterpretedProperty(read_info,next,"label",constitute_info->label,
    exception);
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
      if (read_info->temporary != MagickFalse)
        (void) RelinquishUniqueFileResource(read_info->filename);
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

MagickExport Image *ReadImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    magick[MagickPathExtent],
    magick_filename[MagickPathExtent];

  ConstituteInfo
    constitute_info;

  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  DecodeImageHandler
    *decoder;

  Image
    *image,
    *next;

  ImageInfo
    *read_info;

  /*
    Determine image type from filename prefix or suffix (e.g. image.jpg).
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image_info->filename != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
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
  if (decoder == (DecodeImageHandler *) NULL)
    {
      delegate_info=GetDelegateInfo(read_info->magick,(char *) NULL,exception);
      if (delegate_info == (const DelegateInfo *) NULL)
        {
          (void) SetImageInfo(read_info,0,exception);
          (void) CopyMagickString(read_info->filename,filename,
            MagickPathExtent);
          magick_info=GetMagickInfo(read_info->magick,exception);
          decoder=GetImageDecoder(magick_info);
        }
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
  if (exception->severity >= ErrorException)
    (void) LogMagickEvent(ExceptionEvent,GetMagickModule(),
      "Coder (%s) generated an image despite an error (%d), "
      "notify the developers",image->magick,exception->severity);
  if (IsBlobTemporary(image) != MagickFalse)
    (void) RelinquishUniqueFileResource(read_info->filename);
  if (read_info->ping != MagickFalse)
    {
      for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
      {
        if (IsEmptyImageSize(image) != MagickFalse)
          {
            read_info=DestroyImageInfo(read_info);
            ThrowReaderException(ImageError,"NegativeOrZeroImageSize");
          }
      }
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
  read_info=DestroyImageInfo(read_info);
  if (GetBlobError(image) != MagickFalse)
    ThrowReaderException(CorruptImageError,"UnableToReadImageData");
  return(GetFirstImageInList(image));
}

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

static void FindFallbackEncoder(ImageInfo *write_info,Image *image,
  const char *filename,const MagickInfo **magick_info,
  EncodeImageHandler **encoder,ExceptionInfo *exception)
{
  *magick_info=GetWriteMagickInfo(write_info,image,exception);
  *encoder=GetImageEncoder(*magick_info);
  if (*encoder == (EncodeImageHandler *) NULL)
    {
      char
        extension[MagickPathExtent];

      GetPathComponent(image->filename,ExtensionPath,extension);
      if (*extension != '\0')
        *magick_info=GetMagickInfo(extension,exception);
      else
        *magick_info=GetMagickInfo(image->magick,exception);
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
  EncodeImageHandler *encoder,ImageInfo *write_info,Image *image,
  ExceptionInfo *exception)
{
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
  magick_info=GetMagickInfo(write_info->magick,sans_exception);
  if (sans_exception->severity == PolicyError)
    magick_info=GetMagickInfo(write_info->magick,exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
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
      if (status != MagickFalse)
        {
          if (CloseSeekableWriteBlob(write_info,image,&temporary) ==
              MagickFalse)
            status=MagickFalse;
        }
    }
  encoder=GetImageEncoder(magick_info);
  if (encoder != (EncodeImageHandler *) NULL)
    {
      /*
        Call appropriate image writer based on image type.
      */
      status=EncodeImageWithInfo(magick_info,encoder,write_info,image,
        exception);
    }
  else
    {
      delegate_info=GetDelegateInfo((char *) NULL,write_info->magick,exception);
      if (delegate_info != (DelegateInfo *) NULL)
        {
          /*
            Process the image with delegate.
          */
          *write_info->filename='\0';
          if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
            LockSemaphoreInfo(delegate_info->semaphore);
          status=InvokeDelegate(write_info,image,(char *) NULL,
            write_info->magick,exception);
          if (GetDelegateThreadSupport(delegate_info) == MagickFalse)
            UnlockSemaphoreInfo(delegate_info->semaphore);
          (void) CopyMagickString(image->filename,filename,MagickPathExtent);
        }
      else
        {
          FindFallbackEncoder(write_info,image,filename,&magick_info,&encoder,
            exception);
          if (encoder != (EncodeImageHandler *) NULL)
            {
              /*
                Call appropriate image writer based on image type.
              */
              status=EncodeImageWithInfo(magick_info,encoder,write_info,
                image,exception);
            }
        }
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

static void SyncImageSceneNumbers(Image *images)
{
  Image
    *p;

  p=images;
  for ( ; GetNextImageInList(p) != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (p->scene >= GetNextImageInList(p)->scene)
      {
        ssize_t
          i;

        /*
          Generate consistent scene numbers.
        */
        i=(ssize_t) images->scene;
        for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
          p->scene=(size_t) i++;
        break;
      }
  }
}

static void PrepareWriteImagesInfo(ImageInfo *write_info,Image *images,
  const char *filename)
{
  ExceptionInfo
    *sans_exception;

  Image
    *p;

  if (filename != (const char *) NULL)
    for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
      (void) CopyMagickString(p->filename,filename,MagickPathExtent);
  (void) CopyMagickString(write_info->filename,images->filename,
    MagickPathExtent);
  sans_exception=AcquireExceptionInfo();
  (void) SetImageInfo(write_info,(unsigned int) GetImageListLength(images),
    sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  if (*write_info->magick == '\0')
    (void) CopyMagickString(write_info->magick,images->magick,MagickPathExtent);
  SyncImageSceneNumbers(images);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e I m a g e s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteImages() writes an image sequence into one or more files.  While
%  WriteImage() can write an image sequence, it is limited to writing
%  the sequence into a single file using a format which supports multiple
%  frames.   WriteImages(), however, does not have this limitation, instead it
%  generates multiple output files if necessary (or when requested).  When
%  ImageInfo's adjoin flag is set to MagickFalse, the file name is expected
%  to include a printf-style formatting string for the frame number (e.g.
%  "image%02d.png").
%
%  The format of the WriteImages method is:
%
%      MagickBooleanType WriteImages(const ImageInfo *image_info,Image *images,
%        const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o images: the image list.
%
%    o filename: the image filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SyncWriteBlobStream(ImageInfo *write_info,Image *p,
  Image *images)
{
  if (write_info->blob != (void *) NULL)
    SyncBlobStream(write_info,p,images);
}

static void RestoreWriteProgressMonitor(Image *p,
  const MagickSizeType number_images,MagickProgressMonitor progress_monitor)
{
  if (number_images != 1)
    (void) SetImageProgressMonitor(p,progress_monitor,p->client_data);
}

MagickExport MagickBooleanType WriteImages(const ImageInfo *image_info,
  Image *images,const char *filename,ExceptionInfo *exception)
{
#define WriteImageTag  "Write/Image"

  ImageInfo
    *write_info;

  MagickBooleanType
    proceed;

  MagickOffsetType
    progress;

  MagickProgressMonitor
    progress_monitor;

  MagickSizeType
    number_images;

  MagickStatusType
    status;

  Image
    *p;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  write_info=CloneImageInfo(image_info);
  *write_info->magick='\0';
  images=GetFirstImageInList(images);
  if (images == (Image *) NULL)
    return(MagickFalse);
  PrepareWriteImagesInfo(write_info,images,filename);
  /*
    Write images.
  */
  status=MagickTrue;
  progress_monitor=(MagickProgressMonitor) NULL;
  progress=0;
  number_images=GetImageListLength(images);
  for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (number_images != 1)
      progress_monitor=SetImageProgressMonitor(p,(MagickProgressMonitor) NULL,
        p->client_data);
    status&=(MagickStatusType) WriteImage(write_info,p,exception);
    SyncWriteBlobStream(write_info,p,images);
    RestoreWriteProgressMonitor(p,number_images,progress_monitor);
    if (write_info->adjoin != MagickFalse)
      break;
    if (number_images != 1)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(p,WriteImageTag,progress,number_images);
        if (proceed == MagickFalse)
          break;
      }
  }
  write_info=DestroyImageInfo(write_info);
  return((MagickBooleanType) (status != 0));
}
