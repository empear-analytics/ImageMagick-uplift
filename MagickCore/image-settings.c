/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%           SSSSS  EEEEE  TTTTT  TTTTT  IIIII  N   N   GGGG  SSSSS            %
%           SS     E        T      T      I    NN  N  G      SS               %
%            SSS   EEE      T      T      I    N N N  G  GG   SSS             %
%              SS  E        T      T      I    N  NN  G   G     SS            %
%           SSSSS  EEEEE    T      T    IIIII  N   N   GGG   SSSSS            %
%                                                                             %
%                                                                             %
%                  Image Info Format Detection and Settings                   %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
#include "MagickCore/studio.h"
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/display.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/magick-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/random_.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/timer.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/xwindow-private.h"

static void UpdateSceneRange(ImageInfo *image_info,size_t first,size_t last)
{
  if (first > last)
    Swap(first,last);
  if (first < image_info->scene)
    image_info->scene=first;
  if (last > image_info->number_scenes)
    image_info->number_scenes=last;
}

static void ParseSceneRange(ImageInfo *image_info,const char *component)
{
  char
    *q;

  const char
    *p;

  size_t
    first,
    last;

  (void) CloneString(&image_info->scenes,component);
  image_info->scene=StringToUnsignedLong(image_info->scenes);
  image_info->number_scenes=image_info->scene;
  p=image_info->scenes;
  for (q=(char *) image_info->scenes; *q != '\0'; p++)
  {
    while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == ','))
      p++;
    first=(size_t) strtol(p,&q,10);
    last=first;
    while (isspace((int) ((unsigned char) *q)) != 0)
      q++;
    if (*q == '-')
      last=(size_t) strtol(q+1,&q,10);
    UpdateSceneRange(image_info,first,last);
    p=q;
  }
  image_info->number_scenes-=image_info->scene-1;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t I m a g e I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageInfo() initializes the 'magick' field of the ImageInfo structure.
%  It is set to a type of image format based on the prefix or suffix of the
%  filename.  For example, 'ps:image' returns PS indicating a Postscript image.
%  JPEG is returned for this filename: 'image.jpg'.  The filename prefix has
%  precedence over the suffix.  Use an optional index enclosed in brackets
%  after a file name to specify a desired scene of a multi-resolution image
%  format like Photo CD (e.g. img0001.pcd[4]).  A True (non-zero) return value
%  indicates success.
%
%  The format of the SetImageInfo method is:
%
%      MagickBooleanType SetImageInfo(ImageInfo *image_info,
%        const unsigned int frames,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o frames: the number of images you intend to write.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickFormatType FindExplicitFormatType(const char *magic,
  const char **format_type_formats,MagickFormatType format_type)
{
  ssize_t
    i;

  i=0;
  while ((format_type == UndefinedFormatType) &&
         (format_type_formats[i] != (char *) NULL))
  {
    if ((*magic == *format_type_formats[i]) &&
        (LocaleCompare(magic,format_type_formats[i]) == 0))
      format_type=ExplicitFormatType;
    i++;
  }
  return(format_type);
}

static const MagickInfo *SetImageInfoFromExtension(ImageInfo *image_info,
  const char *component,char *magic,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  MagickFormatType
    format_type;

  static const char
    *format_type_formats[] =
    {
      "AUTOTRACE",
      "BROWSE",
      "DCRAW",
      "EDIT",
      "LAUNCH",
      "MPEG:DECODE",
      "MPEG:ENCODE",
      "PRINT",
      "PS:ALPHA",
      "PS:CMYK",
      "PS:COLOR",
      "PS:GRAY",
      "PS:MONO",
      "SCAN",
      "SHOW",
      "WIN",
      (char *) NULL
    };

  /*
    User specified image format.
  */
  (void) CopyMagickString(magic,component,MagickPathExtent);
  LocaleUpper(magic);
  /*
    Look for explicit image formats.
  */
  format_type=UndefinedFormatType;
  magick_info=GetMagickInfo(magic,exception);
  if ((magick_info != (const MagickInfo *) NULL) &&
      (magick_info->format_type != UndefinedFormatType))
    format_type=magick_info->format_type;
  format_type=FindExplicitFormatType(magic,format_type_formats,format_type);
  if (format_type == UndefinedFormatType)
    (void) CopyMagickString(image_info->magick,magic,MagickPathExtent);
  else
    if (format_type == ExplicitFormatType)
      {
        image_info->affirm=MagickTrue;
        (void) CopyMagickString(image_info->magick,magic,MagickPathExtent);
      }
  if (LocaleCompare(magic,"RGB") == 0)
    image_info->affirm=MagickFalse;  /* maybe SGI disguised as RGB */
  return(magick_info);
}

static inline MagickBooleanType IsUndelegatedComponent(
  const DelegateInfo *delegate_info,const char *component)
{
  return(((delegate_info == (const DelegateInfo *) NULL) &&
    ((*component != '\0') && (IsGlob(component) == MagickFalse))) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsUsableMagick(const MagickInfo *magick_info,
  const DelegateInfo *delegate_info,const char *magic)
{
  return((((magick_info != (const MagickInfo *) NULL) ||
    (delegate_info != (const DelegateInfo *) NULL)) &&
    (IsMagickConflict(magic) == MagickFalse)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsExtensionMagickMatch(
  const MagickInfo *magick_info,const MagicInfo *magic_info)
{
  return(((magick_info != (const MagickInfo *) NULL) &&
    (GetMagickUseExtension(magick_info) != MagickFalse) &&
    (LocaleCompare(magick_info->magick_module,GetMagicName(
    magic_info)) == 0)) ? MagickTrue : MagickFalse);
}

static void AffirmUserSpecifiedFormat(ImageInfo *image_info,
  const char *magic,char *component,ExceptionInfo *exception)
{
  char *literal = (char *) GetImageRegistry(StringRegistryType,
    "filename:literal",exception);
  image_info->affirm=MagickTrue;
  (void) CopyMagickString(image_info->magick,magic,MagickPathExtent);
  GetPathComponent(image_info->filename,CanonicalPath,component);
  if (IsStringTrue(literal) != MagickFalse)
    GetPathComponent(image_info->filename,SubcanonicalPath,component);
  (void) CopyMagickString(image_info->filename,component,
    MagickPathExtent);
  if (literal != (char *) NULL)
    literal=DestroyString(literal);
}

static const MagickInfo *SetUserSpecifiedFormat(ImageInfo *image_info,
  char *magic,char *component,ExceptionInfo *sans_exception,
  ExceptionInfo *exception)
{
  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  /*
    User specified image format.
  */
  LocaleUpper(magic);
  magick_info=GetMagickInfo(magic,sans_exception);
  delegate_info=(const DelegateInfo *) NULL;
  if (magick_info == (const MagickInfo *) NULL)
    {
      delegate_info=GetDelegateInfo(magic,"*",sans_exception);
      if (delegate_info == (const DelegateInfo *) NULL)
        delegate_info=GetDelegateInfo("*",magic,sans_exception);
      if (IsUndelegatedComponent(delegate_info,component) != MagickFalse)
        {
          /*
            Retry in case GetMagickInfo loaded a custom module.
          */
          magick_info=SetImageInfoFromExtension(image_info,component,magic,
            sans_exception);
        }
    }
  if (IsUsableMagick(magick_info,delegate_info,magic) != MagickFalse)
    AffirmUserSpecifiedFormat(image_info,magic,component,exception);
  return(magick_info);
}

static void ParseSubimageSpecification(ImageInfo *image_info,
  const char *component)
{
  /*
    Look for scene specification (e.g. img0001.pcd[4]).
  */
  if (IsSceneGeometry(component,MagickFalse) == MagickFalse)
    {
      if (IsGeometry(component) != MagickFalse)
        (void) CloneString(&image_info->extract,component);
    }
  else
    ParseSceneRange(image_info,component);
}

static void ParseSubimageComponent(ImageInfo *image_info)
{
  char
    component[MagickPathExtent];

  *component='\0';
  GetPathComponent(image_info->filename,SubimagePath,component);
  if (*component == '\0')
    {   
      const char *image_frames = GetImageOption(image_info,"image:frames");
      if (image_frames != (const char *) NULL)
        (void) CopyMagickString(component,image_frames,MagickPathExtent);
    }     
  if (*component != '\0')
    ParseSubimageSpecification(image_info,component);
}

static void TestMultipleImageSupport(ImageInfo *image_info,
  const unsigned int frames,char *component,ExceptionInfo *exception)
{
  if ((image_info->adjoin == MagickFalse) || (frames <= 1))
    return;
  /*
    Test for multiple image support (e.g. image%02d.png).
  */
  (void) InterpretImageFilename(image_info,(Image *) NULL,
    image_info->filename,(int) image_info->scene,component,exception);
  if ((LocaleCompare(component,image_info->filename) != 0) &&
      (strchr(component,'%') == (char *) NULL))
    image_info->adjoin=MagickFalse;
}

static MagickBooleanType CopyImageToSeekableFile(ImageInfo *image_info,
  Image *image,char *component,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  if ((IsBlobSeekable(image) == MagickFalse) ||
      (IsBlobExempt(image) != MagickFalse))
    {
      /*
        Copy image to seekable temporary file.
      */
      *component='\0';
      status=ImageToFile(image,component,exception);
      if (CloseBlob(image) == MagickFalse)
        status=MagickFalse;
      if (status == MagickFalse)
        {
          (void) RelinquishUniqueFileResource(component);
          image=DestroyImage(image);
          return(MagickFalse);
        }
      SetImageInfoFile(image_info,(FILE *) NULL);
      (void) CopyMagickString(image->filename,component,MagickPathExtent);
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          (void) RelinquishUniqueFileResource(component);
          image=DestroyImage(image);
          return(MagickFalse);
        }
      (void) CopyMagickString(image_info->filename,component,
        MagickPathExtent);
      image_info->temporary=MagickTrue;
    }
  return(MagickTrue);
}

static void ResetUnsupportedEndian(ImageInfo *image_info,
  const MagickInfo *magick_info)
{
  if ((magick_info == (const MagickInfo *) NULL) ||
      (GetMagickEndianSupport(magick_info) == MagickFalse))
    image_info->endian=UndefinedEndian;
}

static MagickBooleanType SetImageMagickFromMagic(ImageInfo *image_info,
  const MagickInfo *magick_info,const MagicInfo *magic_info,
  ExceptionInfo *sans_exception)
{
  if ((magic_info != (const MagicInfo *) NULL) &&
      (GetMagicName(magic_info) != (char *) NULL))
    {
      /*
        Try to use magick_info that was determined earlier by the extension
      */
      if (IsExtensionMagickMatch(magick_info,magic_info) != MagickFalse)
        (void) CopyMagickString(image_info->magick,magick_info->name,
          MagickPathExtent);
      else
        {
          (void) CopyMagickString(image_info->magick,GetMagicName(
            magic_info),MagickPathExtent);
          magick_info=GetMagickInfo(image_info->magick,sans_exception);
        }
      ResetUnsupportedEndian(image_info,magick_info);
      sans_exception=DestroyExceptionInfo(sans_exception);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType SetImageMagickFromContent(ImageInfo *image_info,
  const MagickInfo *magick_info,char *component,ExceptionInfo *exception)
{
  const MagicInfo
    *magic_info;

  ExceptionInfo
    *sans_exception;

  Image
    *image;

  MagickBooleanType
    status;

  ssize_t
    count;

  unsigned char
    *magick;

  size_t
    magick_size;

  /*
    Determine the image format from the first few bytes of the file.
  */
  magick_size=GetMagicPatternExtent(exception);
  if (magick_size == 0)
    return(MagickFalse);
  image=AcquireImage(image_info,exception);
  (void) CopyMagickString(image->filename,image_info->filename,
    MagickPathExtent);
  sans_exception=AcquireExceptionInfo();
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  if (status == MagickFalse)
    {
      image=DestroyImage(image);
      return(MagickFalse);
    }
  if (CopyImageToSeekableFile(image_info,image,component,exception) ==
      MagickFalse)
    return(MagickFalse);
  magick=(unsigned char *) AcquireQuantumMemory(1,magick_size);
  if (magick == (unsigned char *) NULL)
    {
      (void) CloseBlob(image);
      image=DestroyImage(image);
      return(MagickFalse);
    }
  (void) memset(magick,0,magick_size);
  count=ReadBlob(image,magick_size,magick);
  (void) SeekBlob(image,-((MagickOffsetType) count),SEEK_CUR);
  (void) CloseBlob(image);
  image=DestroyImage(image);
  /*
    Check magic cache.
  */
  sans_exception=AcquireExceptionInfo();
  magic_info=GetMagicInfo(magick,(size_t) count,sans_exception);
  magick=(unsigned char *) RelinquishMagickMemory(magick);
  if (SetImageMagickFromMagic(image_info,magick_info,magic_info,
      sans_exception) == MagickFalse)
    return(MagickTrue);
  magick_info=GetMagickInfo(image_info->magick,sans_exception);
  ResetUnsupportedEndian(image_info,magick_info);
  sans_exception=DestroyExceptionInfo(sans_exception);
  return(MagickTrue);
}

static void DisableUnsupportedAdjoin(ImageInfo *image_info,
  const unsigned int frames,const char *magic,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  if ((image_info->adjoin == MagickFalse) || (frames == 0))
    return;
  /*
    Some image formats do not support multiple frames per file.
  */
  magick_info=GetMagickInfo(magic,exception);
  if (magick_info != (const MagickInfo *) NULL)
    if (GetMagickAdjoin(magick_info) == MagickFalse)
      image_info->adjoin=MagickFalse;
}

static void GetImageInfoExtension(const ImageInfo *image_info,char *component)
{
  char
    path[MagickPathExtent];

  *component='\0';
  if (*image_info->magick == '\0')
    GetPathComponent(image_info->filename,ExtensionPath,component);
  if (*component != '\0')
    {
      /*
        Base path sans any compression extension.
      */
      GetPathComponent(image_info->filename,BasePathSansCompressExtension,path);
      GetPathComponent(path,ExtensionPath,component);
    }
}

static const MagickInfo *GetImplicitFormat(ImageInfo *image_info,char *magic,
  ExceptionInfo *sans_exception)
{
  const MagickInfo
    *magick_info;

  (void) CopyMagickString(magic,image_info->magick,MagickPathExtent);
  magick_info=GetMagickInfo(magic,sans_exception);
  if ((magick_info != (const MagickInfo *) NULL) &&
      (magick_info->format_type == ExplicitFormatType))
    image_info->affirm=MagickTrue;
  return(magick_info);
}

static void SetImageInfoFilename(ImageInfo *image_info,
  const unsigned int frames,char *component)
{
  if (frames == 0)
    GetPathComponent(image_info->filename,CanonicalPath,component);
  else
    GetPathComponent(image_info->filename,SubcanonicalPath,component);
  (void) CopyMagickString(image_info->filename,component,MagickPathExtent);
}

MagickExport MagickBooleanType SetImageInfo(ImageInfo *image_info,
  const unsigned int frames,ExceptionInfo *exception)
{
  char
    component[MagickPathExtent],
    magic[MagickPathExtent];

  const MagickInfo
    *magick_info;

  ExceptionInfo
    *sans_exception;

  /*
    Look for 'image.format' in filename.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  ParseSubimageComponent(image_info);
  GetImageInfoExtension(image_info,component);
  image_info->affirm=MagickFalse;
  sans_exception=AcquireExceptionInfo();
  if ((*component != '\0') && (IsGlob(component) == MagickFalse))
    magick_info=SetImageInfoFromExtension(image_info,component,magic,
      sans_exception);
  /*
    Look for explicit 'format:image' in filename.
  */
  *magic='\0';
  GetPathComponent(image_info->filename,MagickPath,magic);
  if (*magic == '\0')
    {
      magick_info=GetImplicitFormat(image_info,magic,sans_exception);
      SetImageInfoFilename(image_info,frames,component);
    }
  else
    magick_info=SetUserSpecifiedFormat(image_info,magic,component,
      sans_exception,exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  ResetUnsupportedEndian(image_info,magick_info);
  TestMultipleImageSupport(image_info,frames,component,exception);
  DisableUnsupportedAdjoin(image_info,frames,magic,exception);
  if (image_info->affirm != MagickFalse)
    return(MagickTrue);
  if (frames == 0)
    return(SetImageMagickFromContent(image_info,magick_info,component,
      exception));
  return(MagickTrue);
}

static void ParseChromaticityPrimary(const char *option,PrimaryInfo *primary)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  flags=ParseGeometry(option,&geometry_info);
  if ((flags & RhoValue) != 0)
    primary->x=geometry_info.rho;
  primary->y=primary->x;
  if ((flags & SigmaValue) != 0)
    primary->y=geometry_info.sigma;
}

static void SetImageResolutionFromDensity(Image *image,const char *density)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  flags=ParseGeometry(density,&geometry_info);
  if ((flags & RhoValue) != 0)
    image->resolution.x=geometry_info.rho;
  image->resolution.y=image->resolution.x;
  if ((flags & SigmaValue) != 0)
    image->resolution.y=geometry_info.sigma;
}

static MagickStatusType ParsePageGeometryOption(const char *option,
  RectangleInfo *region)
{
  char
    *geometry;

  MagickStatusType
    flags;

  geometry=GetPageGeometry(option);
  flags=ParseAbsoluteGeometry(geometry,region);
  geometry=DestroyString(geometry);
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S y n c I m a g e S e t t i n g s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncImageSettings() syncs any image_info global options into per-image
%  attributes.
%
%  Note: in IMv6 free form 'options' were always mapped into 'artifacts', so
%  that operations and coders can find such settings.  In IMv7 if a desired
%  per-image artifact is not set, then it will directly look for a global
%  option as a fallback, as such this copy is no longer needed, only the
%  link set up.
%
%  The format of the SyncImageSettings method is:
%
%      MagickBooleanType SyncImageSettings(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *exception)
%      MagickBooleanType SyncImagesSettings(const ImageInfo *image_info,
%        Image *image,ExceptionInfo *exception)
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

MagickExport MagickBooleanType SyncImagesSettings(ImageInfo *image_info,
  Image *images,ExceptionInfo *exception)
{
  Image
    *image;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  image=images;
  for ( ; image != (Image *) NULL; image=GetNextImageInList(image))
    (void) SyncImageSettings(image_info,image,exception);
  (void) DeleteImageOption(image_info,"page");
  return(MagickTrue);
}

static void ConvertResolutionUnits(Image *image,const ResolutionType units)
{
  if (image->units != units)
    switch (image->units)
    {
      case PixelsPerInchResolution:
      {
        if (units == PixelsPerCentimeterResolution)
          {
            image->resolution.x/=2.54;
            image->resolution.y/=2.54;
          }
        break;
      }
      case PixelsPerCentimeterResolution:
      {
        if (units == PixelsPerInchResolution)
          {
            image->resolution.x=(double) ((size_t) (100.0*2.54*
              image->resolution.x+0.5))/100.0;
            image->resolution.y=(double) ((size_t) (100.0*2.54*
              image->resolution.y+0.5))/100.0;
          }
        break;
      }
      default:
        break;
    }
}

static void SyncColorOption(const ImageInfo *image_info,const char *key,
  PixelInfo *value,ExceptionInfo *exception)
{
  const char
    *option;

  option=GetImageOption(image_info,key);
  if (option != (const char *) NULL)
    (void) QueryColorCompliance(option,AllCompliance,value,exception);
}

static void SyncBooleanOption(const ImageInfo *image_info,const char *key,
  MagickBooleanType *value)
{
  const char
    *option;

  option=GetImageOption(image_info,key);
  if (option != (const char *) NULL)
    (*value)=(MagickBooleanType) ParseCommandOption(MagickBooleanOptions,
      MagickFalse,option);
}

static void SyncChromaticityOption(const ImageInfo *image_info,
  const char *key,PrimaryInfo *value)
{
  const char
    *option;

  option=GetImageOption(image_info,key);
  if (option != (const char *) NULL)
    ParseChromaticityPrimary(option,value);
}

static void SyncImageColorAndCodecSettings(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  const char
    *option;

  SyncColorOption(image_info,"background",&image->background_color,exception);
  SyncBooleanOption(image_info,"black-point-compensation",
    &image->black_point_compensation);
  SyncChromaticityOption(image_info,"blue-primary",
    &image->chromaticity.blue_primary);
  SyncColorOption(image_info,"bordercolor",&image->border_color,exception);
  /* FUTURE: do not sync compose to per-image compose setting here */
  option=GetImageOption(image_info,"compose");
  if (option != (const char *) NULL)
    image->compose=(CompositeOperator) ParseCommandOption(MagickComposeOptions,
      MagickFalse,option);
  /* -- */
  option=GetImageOption(image_info,"compress");
  if (option != (const char *) NULL)
    image->compression=(CompressionType) ParseCommandOption(
      MagickCompressOptions,MagickFalse,option);
  SyncBooleanOption(image_info,"debug",&image->debug);
}

static void SyncUnsignedLongOption(const ImageInfo *image_info,
  const char *key,size_t *value)
{
  const char
    *option;

  option=GetImageOption(image_info,key);
  if (option != (const char *) NULL)
    (*value)=StringToUnsignedLong(option);
}

static void SyncImageDepthAndFilterSettings(const ImageInfo *image_info,
  Image *image)
{
  const char
    *option;

  option=GetImageOption(image_info,"density");
  if (option != (const char *) NULL)
    SetImageResolutionFromDensity(image,option);
  SyncUnsignedLongOption(image_info,"depth",&image->depth);
  option=GetImageOption(image_info,"endian");
  if (option != (const char *) NULL)
    image->endian=(EndianType) ParseCommandOption(MagickEndianOptions,
      MagickFalse,option);
  option=GetImageOption(image_info,"filter");
  if (option != (const char *) NULL)
    image->filter=(FilterType) ParseCommandOption(MagickFilterOptions,
      MagickFalse,option);
  option=GetImageOption(image_info,"fuzz");
  if (option != (const char *) NULL)
    image->fuzz=StringToDoubleInterval(option,(double) QuantumRange+1.0);
  option=GetImageOption(image_info,"gravity");
  if (option != (const char *) NULL)
    image->gravity=(GravityType) ParseCommandOption(MagickGravityOptions,
      MagickFalse,option);
  SyncChromaticityOption(image_info,"green-primary",
    &image->chromaticity.green_primary);
}

static void SyncImageRenderingSettings(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  const char
    *option;

  option=GetImageOption(image_info,"intent");
  if (option != (const char *) NULL)
    image->rendering_intent=(RenderingIntent) ParseCommandOption(
      MagickIntentOptions,MagickFalse,option);
  option=GetImageOption(image_info,"intensity");
  if (option != (const char *) NULL)
    image->intensity=(PixelIntensityMethod) ParseCommandOption(
      MagickPixelIntensityOptions,MagickFalse,option);
  option=GetImageOption(image_info,"interlace");
  if (option != (const char *) NULL)
    image->interlace=(InterlaceType) ParseCommandOption(MagickInterlaceOptions,
      MagickFalse,option);
  option=GetImageOption(image_info,"interpolate");
  if (option != (const char *) NULL)
    image->interpolate=(PixelInterpolateMethod) ParseCommandOption(
      MagickInterpolateOptions,MagickFalse,option);
  option=GetImageOption(image_info,"loop");
  if (option != (const char *) NULL)
    image->iterations=StringToUnsignedLong(option);
  option=GetImageOption(image_info,"mattecolor");
  if (option != (const char *) NULL)
    (void) QueryColorCompliance(option,AllCompliance,&image->matte_color,
      exception);
  option=GetImageOption(image_info,"orient");
  if (option != (const char *) NULL)
    image->orientation=(OrientationType) ParseCommandOption(
      MagickOrientationOptions,MagickFalse,option);
}

static void SyncGeometryOption(const ImageInfo *image_info,
  const char *key,RectangleInfo *value)
{
  const char
    *option;

  option=GetImageOption(image_info,key);
  if (option != (const char *) NULL)
    (void) ParsePageGeometryOption(option,value);
}

static void SyncImagePageAndQualitySettings(const ImageInfo *image_info,
  Image *image)
{
  SyncGeometryOption(image_info,"page",&image->page);
  SyncUnsignedLongOption(image_info,"quality",&image->quality);
  SyncChromaticityOption(image_info,"red-primary",
    &image->chromaticity.red_primary);
  if (image_info->quality != UndefinedCompressionQuality)
    image->quality=image_info->quality;
  SyncUnsignedLongOption(image_info,"scene",&image->scene);
  SyncBooleanOption(image_info,"taint",&image->taint);
  SyncGeometryOption(image_info,"tile-offset",&image->tile_offset);
}

static void SyncImageUnitsSettings(const ImageInfo *image_info,Image *image)
{
  const char
    *option;

  ResolutionType
    units;

  option=GetImageOption(image_info,"units");
  units=image_info->units;
  if (option != (const char *) NULL)
    units=(ResolutionType) ParseCommandOption(MagickResolutionOptions,
      MagickFalse,option);
  if (units != UndefinedResolution)
    {
      ConvertResolutionUnits(image,units);
      image->units=units;
      option=GetImageOption(image_info,"density");
      if (option != (const char *) NULL)
        SetImageResolutionFromDensity(image,option);
    }
}

MagickExport MagickBooleanType SyncImageSettings(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  const char
    *option;

  /*
    Sync image options.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  SyncImageColorAndCodecSettings(image_info,image,exception);
  SyncImageDepthAndFilterSettings(image_info,image);
  SyncImageRenderingSettings(image_info,image,exception);
  SyncImagePageAndQualitySettings(image_info,image);
  option=GetImageOption(image_info,"transparent-color");
  if (option != (const char *) NULL)
    (void) QueryColorCompliance(option,AllCompliance,&image->transparent_color,
      exception);
  option=GetImageOption(image_info,"type");
  if (option != (const char *) NULL)
    image->type=(ImageType) ParseCommandOption(MagickTypeOptions,MagickFalse,
      option);
  SyncImageUnitsSettings(image_info,image);
  option=GetImageOption(image_info,"virtual-pixel");
  if (option != (const char *) NULL)
    (void) SetImageVirtualPixelMethod(image,(VirtualPixelMethod)
      ParseCommandOption(MagickVirtualPixelOptions,MagickFalse,option),
      exception);
  option=GetImageOption(image_info,"white-point");
  if (option != (const char *) NULL)
    ParseChromaticityPrimary(option,&image->chromaticity.white_point);
  /*
    Pointer to allow the lookup of pre-image artifact will fallback to a global
    option setting/define.  This saves a lot of duplication of global options
    into per-image artifacts, while ensuring only specifically set per-image
    artifacts are preserved when parenthesis ends.
  */
  if (image->image_info != (ImageInfo *) NULL)
    image->image_info=DestroyImageInfo(image->image_info);
  image->image_info=CloneImageInfo(image_info);
  return(MagickTrue);
}
