/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            PPPP    RRRR    OOO   PPPP   EEEEE  RRRR   TTTTT  Y   Y          %
%            P   P   R   R  O   O  P   P  E      R   R    T     Y Y           %
%            PPPP    RRRR   O   O  PPPP   EEE    RRRR     T      Y            %
%            P       R R    O   O  P      E      R R      T      Y            %
%            P       R  R    OOO   P      EEEEE  R  R     T      Y            %
%                                                                             %
%                                                                             %
%                         MagickCore Property Methods                         %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 March 2000                                  %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/compare.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/fx-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image.h"
#include "MagickCore/layer.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/signature.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#if defined(MAGICKCORE_LCMS_DELEGATE)
#if defined(MAGICKCORE_HAVE_LCMS2_LCMS2_H)
#include <lcms2/lcms2.h>
#elif defined(MAGICKCORE_HAVE_LCMS2_H)
#include "lcms2.h"
#elif defined(MAGICKCORE_HAVE_LCMS_LCMS_H)
#include <lcms/lcms.h>
#else
#include "lcms.h"
#endif
#endif

/*
  Define declarations.
*/
#if defined(MAGICKCORE_LCMS_DELEGATE)
#if defined(LCMS_VERSION) && (LCMS_VERSION < 2000)
#define cmsUInt32Number  DWORD
#endif
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e P r o p e r t i e s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageProperties() clones all the image properties to another image.
%
%  The format of the CloneImageProperties method is:
%
%      MagickBooleanType CloneImageProperties(Image *image,
%        const Image *clone_image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o clone_image: the clone image.
%
*/

typedef char
  *(*CloneKeyFunc)(const char *),
  *(*CloneValueFunc)(const char *);

static inline void *ClonePropertyKey(void *key)
{
  return((void *) ((CloneKeyFunc) ConstantString)((const char *) key));
}

static inline void *ClonePropertyValue(void *value)
{
  return((void *) ((CloneValueFunc) ConstantString)((const char *) value));
}

MagickExport MagickBooleanType CloneImageProperties(Image *image,
  const Image *clone_image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(clone_image != (const Image *) NULL);
  assert(clone_image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    {
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
        clone_image->filename);
    }
  (void) CopyMagickString(image->filename,clone_image->filename,
    MagickPathExtent);
  (void) CopyMagickString(image->magick_filename,clone_image->magick_filename,
    MagickPathExtent);
  image->compression=clone_image->compression;
  image->quality=clone_image->quality;
  image->depth=clone_image->depth;
  image->matte_color=clone_image->matte_color;
  image->background_color=clone_image->background_color;
  image->border_color=clone_image->border_color;
  image->transparent_color=clone_image->transparent_color;
  image->gamma=clone_image->gamma;
  image->chromaticity=clone_image->chromaticity;
  image->rendering_intent=clone_image->rendering_intent;
  image->black_point_compensation=clone_image->black_point_compensation;
  image->units=clone_image->units;
  image->montage=(char *) NULL;
  image->directory=(char *) NULL;
  (void) CloneString(&image->geometry,clone_image->geometry);
  image->offset=clone_image->offset;
  image->resolution.x=clone_image->resolution.x;
  image->resolution.y=clone_image->resolution.y;
  image->page=clone_image->page;
  image->tile_offset=clone_image->tile_offset;
  image->extract_info=clone_image->extract_info;
  image->filter=clone_image->filter;
  image->fuzz=clone_image->fuzz;
  image->intensity=clone_image->intensity;
  image->interlace=clone_image->interlace;
  image->interpolate=clone_image->interpolate;
  image->endian=clone_image->endian;
  image->gravity=clone_image->gravity;
  image->compose=clone_image->compose;
  image->orientation=clone_image->orientation;
  image->scene=clone_image->scene;
  image->dispose=clone_image->dispose;
  image->delay=clone_image->delay;
  image->ticks_per_second=clone_image->ticks_per_second;
  image->iterations=clone_image->iterations;
  image->total_colors=clone_image->total_colors;
  image->taint=clone_image->taint;
  image->progress_monitor=clone_image->progress_monitor;
  image->client_data=clone_image->client_data;
  image->start_loop=clone_image->start_loop;
  image->error=clone_image->error;
  image->signature=clone_image->signature;
  if (clone_image->properties != (void *) NULL)
    {
      if (image->properties != (void *) NULL)
        DestroyImageProperties(image);
      image->properties=CloneSplayTree((SplayTreeInfo *)
        clone_image->properties,ClonePropertyKey,ClonePropertyValue);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e f i n e I m a g e P r o p e r t y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DefineImageProperty() associates an assignment string of the form
%  "key=value" with an artifact or options. It is equivalent to
%  SetImageProperty().
%
%  The format of the DefineImageProperty method is:
%
%      MagickBooleanType DefineImageProperty(Image *image,const char *property,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o property: the image property.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType DefineImageProperty(Image *image,
  const char *property,ExceptionInfo *exception)
{
  char
    key[MagickPathExtent],
    value[MagickPathExtent];

  char
    *p;

  assert(image != (Image *) NULL);
  assert(property != (const char *) NULL);
  (void) CopyMagickString(key,property,MagickPathExtent-1);
  for (p=key; *p != '\0'; p++)
    if (*p == '=')
      break;
  *value='\0';
  if (*p == '=')
    (void) CopyMagickString(value,p+1,MagickPathExtent);
  *p='\0';
  return(SetImageProperty(image,key,value,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e I m a g e P r o p e r t y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteImageProperty() deletes an image property.
%
%  The format of the DeleteImageProperty method is:
%
%      MagickBooleanType DeleteImageProperty(Image *image,const char *property)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o property: the image property.
%
*/
static void CheckPropertyImage(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
}

MagickExport MagickBooleanType DeleteImageProperty(Image *image,
  const char *property)
{
  CheckPropertyImage(image);
  if (image->properties == (void *) NULL)
    return(MagickFalse);
  return(DeleteNodeFromSplayTree((SplayTreeInfo *) image->properties,property));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e P r o p e r t i e s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageProperties() destroys all properties and associated memory
%  attached to the given image.
%
%  The format of the DestroyDefines method is:
%
%      void DestroyImageProperties(Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void DestroyImageProperties(Image *image)
{
  CheckPropertyImage(image);
  if (image->properties != (void *) NULL)
    image->properties=(void *) DestroySplayTree((SplayTreeInfo *)
      image->properties);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t I m a g e P r o p e r t y                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatImageProperty() permits formatted property/value pairs to be saved as
%  an image property.
%
%  The format of the FormatImageProperty method is:
%
%      MagickBooleanType FormatImageProperty(Image *image,const char *property,
%        const char *format,...)
%
%  A description of each parameter follows.
%
%   o  image:  The image.
%
%   o  property:  The attribute property.
%
%   o  format:  A string describing the format to use to write the remaining
%      arguments.
%
*/
MagickExport MagickBooleanType FormatImageProperty(Image *image,
  const char *property,const char *format,...)
{
  char
    value[MagickPathExtent];

  ExceptionInfo
    *exception;

  MagickBooleanType
    status;

  ssize_t
    n;

  va_list
    operands;

  va_start(operands,format);
  n=FormatLocaleStringList(value,MagickPathExtent,format,operands);
  (void) n;
  va_end(operands);
  exception=AcquireExceptionInfo();
  status=SetImageProperty(image,property,value,exception);
  exception=DestroyExceptionInfo(exception);
  return(status);
}

#include "MagickCore/property-iptc-private.h"

#include "MagickCore/property-8bim-private.h"

#include "MagickCore/property-exif-private.h"

#include "MagickCore/property-icc-private.h"

#include "MagickCore/property-xmp-private.h"

#include "MagickCore/property-clippath-private.h"

#include "MagickCore/property-profile-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e P r o p e r t y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageProperty() gets a value associated with an image property.
%
%  This includes,  profile prefixes, such as "exif:", "iptc:" and "8bim:"
%  It does not handle non-profile prefixes, such as "fx:", "option:", or
%  "artifact:".
%
%  The returned string is stored as a prosperity of the same name for faster
%  lookup later. It should NOT be freed by the caller.
%
%  The format of the GetImageProperty method is:
%
%      const char *GetImageProperty(const Image *image,const char *key,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o key: the key.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport const char *GetImageProperty(const Image *image,
  const char *property,ExceptionInfo *exception)
{
  MagickBooleanType
    read_from_properties;

  const char
    *p,
    *returned;

  size_t
    property_length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((property == (const char *) NULL) || (*property == '\0'))
    return((const char *) NULL);
  read_from_properties=MagickTrue;
  property_length=strlen(property);
  if (IsWildcardProperty(property,property_length) != MagickFalse)
    read_from_properties=MagickFalse;
  if (GetStoredImageProperty(image,property,read_from_properties,&returned) !=
      MagickFalse)
    return(returned);
  GetProfileProperty(image,property,exception);
  if ((image->properties != (void *) NULL) &&
      (read_from_properties != MagickFalse))
    {
      p=(const char *) GetValueFromSplayTree((SplayTreeInfo *)
        image->properties,property);
      return(p);
    }
  return((const char *) NULL);
}

#include "MagickCore/property-magick-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k P r o p e r t y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickProperty() gets attributes or calculated values that is associated
%  with a fixed known property name, or single letter property. It may be
%  called if no image is defined (IMv7), in which case only global image_info
%  values are available:
%
%    \n   newline
%    \r   carriage return
%    <    less-than character.
%    >    greater-than character.
%    &    ampersand character.
%    %%   a percent sign
%    %b   file size of image read in
%    %c   comment meta-data property
%    %d   directory component of path
%    %e   filename extension or suffix
%    %f   filename (including suffix)
%    %g   layer canvas page geometry   (equivalent to "%Wx%H%X%Y")
%    %h   current image height in pixels
%    %i   image filename (note: becomes output filename for "info:")
%    %k   CALCULATED: number of unique colors
%    %l   label meta-data property
%    %m   image file format (file magic)
%    %n   number of images in current image sequence
%    %o   output filename  (used for delegates)
%    %p   index of image in current image list
%    %q   quantum depth (compile-time constant)
%    %r   image class and colorspace
%    %s   scene number (from input unless re-assigned)
%    %t   filename without directory or extension (suffix)
%    %u   unique temporary filename (used for delegates)
%    %w   current width in pixels
%    %x   x resolution (density)
%    %y   y resolution (density)
%    %z   image depth (as read in unless modified, image save depth)
%    %A   image transparency channel enabled (true/false)
%    %B   file size of image in bytes
%    %C   image compression type
%    %D   image GIF dispose method
%    %G   original image size (%wx%h; before any resizes)
%    %H   page (canvas) height
%    %M   Magick filename (original file exactly as given,  including read mods)
%    %O   page (canvas) offset ( = %X%Y )
%    %P   page (canvas) size ( = %Wx%H )
%    %Q   image compression quality ( 0 = default )
%    %S   ?? scenes ??
%    %T   image time delay (in centi-seconds)
%    %U   image resolution units
%    %W   page (canvas) width
%    %X   page (canvas) x offset (including sign)
%    %Y   page (canvas) y offset (including sign)
%    %Z   unique filename (used for delegates)
%    %@   CALCULATED: trim bounding box (without actually trimming)
%    %#   CALCULATED: 'signature' hash of image values
%
%  This routine only handles specifically known properties.  It does not
%  handle special prefixed properties, profiles, or expressions. Nor does
%  it return any free-form property strings.
%
%  The returned string is stored in a structure somewhere, and should not be
%  directly freed.  If the string was generated (common) the string will be
%  stored as as either as artifact or option 'magick-property'.  These may be
%  deleted (cleaned up) when no longer required, but neither artifact or
%  option is guaranteed to exist.
%
%  The format of the GetMagickProperty method is:
%
%      const char *GetMagickProperty(ImageInfo *image_info,Image *image,
%        const char *property,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info (optional)
%
%    o image: the image (optional)
%
%    o key: the key.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport const char *GetMagickProperty(ImageInfo *image_info,
  Image *image,const char *property,ExceptionInfo *exception)
{
  char
    value[MagickPathExtent];

  const char
    *string;

  MagickPropertyRequest
    request;

  assert(property != (const char *) NULL);
  assert(property[0] != '\0');
  assert(image != (Image *) NULL || image_info != (ImageInfo *) NULL );
  if (property[1] == '\0')  /* single letter property request */
    return(GetMagickPropertyLetter(image_info,image,*property,exception));
  LogMagickPropertyRequest(image_info,image);
  *value='\0';           /* formated string */
  string=(char *) NULL;  /* constant string reference */
  request.image_info=image_info;
  request.image=image;
  request.property=property;
  request.value=value;
  request.exception=exception;
  string=LookupMagickProperty(&request,string);
  switch (*property)
  {
    case 'p':
    {
#if defined(MAGICKCORE_LCMS_DELEGATE)
      if (IsICCProfileProperty(property) != MagickFalse)
        {
#if !defined(LCMS_VERSION) || (LCMS_VERSION < 2000)
#define cmsUInt32Number  DWORD
#endif

          const StringInfo
            *profile;

          cmsHPROFILE
            icc_profile;

          WarnNoImageReturn("\"%%[%s]\"",property);
          profile=GetImageProfile(image,property+8);
          if (profile == (StringInfo *) NULL)
            break;
          icc_profile=cmsOpenProfileFromMem(GetStringInfoDatum(profile),
            (cmsUInt32Number) GetStringInfoLength(profile));
          if (icc_profile != (cmsHPROFILE *) NULL)
            {
#if defined(LCMS_VERSION) && (LCMS_VERSION < 2000)
              string=cmsTakeProductName(icc_profile);
#else
              (void) cmsGetProfileInfoASCII(icc_profile,cmsInfoDescription,
                "en","US",value,MagickPathExtent);
#endif
              (void) cmsCloseProfile(icc_profile);
            }
      }
#endif
      break;
    }
  }
  return(SelectMagickPropertyValue(image_info,image,string,value));
}
#undef WarnNoImageReturn

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t I m a g e P r o p e r t y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextImageProperty() gets the next free-form string property name.
%
%  The format of the GetNextImageProperty method is:
%
%      char *GetNextImageProperty(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport const char *GetNextImageProperty(const Image *image)
{
  CheckPropertyImage(image);
  if (image->properties == (void *) NULL)
    return((const char *) NULL);
  return((const char *) GetNextKeyInSplayTree((SplayTreeInfo *) image->properties));
}

#include "MagickCore/property-interpret-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p r e t I m a g e P r o p e r t i e s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpretImageProperties() replaces any embedded formatting characters with
%  the appropriate image property and returns the interpreted text.
%
%  This searches for and replaces
%     \n \r \%          replaced by newline, return, and percent resp.
%     &lt; &gt; &amp;   replaced by '<', '>', '&' resp.
%     %%                replaced by percent
%
%     %x %[x]       where 'x' is a single letter prosperity, case sensitive).
%     %[type:name]  where 'type' a is special and known prefix.
%     %[name]       where 'name' is a specifically known attribute, calculated
%                   value, or a per-image property string name, or a per-image
%                   'artifact' (as generated from a global option).
%                   It may contain ':' as long as the prefix is not special.
%
%  Single letter % substitutions will only happen if the character before the
%  percent is NOT a number. But braced substitutions will always be performed.
%  This prevents the typical usage of percent in a interpreted geometry
%  argument from being substituted when the percent is a geometry flag.
%
%  If 'glob-expressions' ('*' or '?' characters) is used for 'name' it may be
%  used as a search pattern to print multiple lines of "name=value\n" pairs of
%  the associated set of properties.
%
%  The returned string must be freed using DestroyString() by the caller.
%
%  The format of the InterpretImageProperties method is:
%
%      char *InterpretImageProperties(ImageInfo *image_info,
%        Image *image,const char *embed_text,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info. (required)
%
%    o image: the image. (optional)
%
%    o embed_text: the address of a character string containing the embedded
%      formatting characters.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport char *InterpretImageProperties(ImageInfo *image_info,Image *image,
  const char *embed_text,ExceptionInfo *exception)
{
  char
    *interpret_text = (char *) NULL,
    *q,  /* current position in interpret_text */
    *returned;

  const char
    *p;  /* position in embed_text string being expanded */

  Image
    *property_image = (Image *) NULL;

  ImageInfo
    *property_info = (ImageInfo *) NULL;

  InterpretRequest
    request;

  MagickBooleanType
    status = MagickTrue;

  size_t
    extent;  /* allocated length of interpret_text */

  const InterpretText
    text = { &interpret_text, &q, &extent };

  LogInterpretImageProperties(image_info,image);
  if (embed_text == (const char *) NULL)
    return(ConstantString(""));
  p=SkipInterpretSpaces(embed_text);
  if (*p == '\0')
    return(ConstantString(""));
  if (ReadInterpretTextFile(p,&interpret_text,exception,&returned) !=
      MagickFalse)
    return(returned);
  /*
    Translate any embedded format characters.
  */
  if (image_info != (ImageInfo *) NULL)
    property_info=image_info;
  else
    property_info=CloneImageInfo(image_info);
  request.image_info=image_info;
  request.image=image;
  request.embed_text=embed_text;
  request.property_info=property_info;
  request.exception=exception;
  if (InterpretEmbeddedText(&request,&property_image,&text,&p) == MagickFalse)
    {
      status=MagickFalse;
      goto cleanup;
    }

cleanup:
  DestroyInterpretSources(image_info,image,property_info,property_image);
  return(FinishInterpretText(status,interpret_text));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e m o v e I m a g e P r o p e r t y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveImageProperty() removes a property from the image and returns its
%  value.
%
%  In this case the ConstantString() value returned should be freed by the
%  caller when finished.
%
%  The format of the RemoveImageProperty method is:
%
%      char *RemoveImageProperty(Image *image,const char *property)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o property: the image property.
%
*/
MagickExport char *RemoveImageProperty(Image *image,const char *property)
{
  char
    *value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->properties == (void *) NULL)
    return((char *) NULL);
  value=(char *) RemoveNodeFromSplayTree((SplayTreeInfo *) image->properties,
    property);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e P r o p e r t y I t e r a t o r                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImagePropertyIterator() resets the image properties iterator.  Use it
%  in conjunction with GetNextImageProperty() to iterate over all the values
%  associated with an image property.
%
%  The format of the ResetImagePropertyIterator method is:
%
%      void ResetImagePropertyIterator(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
*/
MagickExport void ResetImagePropertyIterator(const Image *image)
{
  CheckPropertyImage(image);
  if (image->properties == (void *) NULL)
    return;
  ResetSplayTreeIterator((SplayTreeInfo *) image->properties);
}

static const char
  *const ReadOnlyProperties[] =
  {
    "channels",
    "delay_units",
    "height",
    "kurtosis",
    "magick",
    "max",
    "mean",
    "min",
    "opaque",
    "size",
    "skewness",
    "scenes",
    "standard-deviation",
    "version",
    "width"
  };

static MagickBooleanType IsReadOnlyProperty(const char *property)
{
  size_t
    i;

  for (i=0; i < (sizeof(ReadOnlyProperties)/sizeof(*ReadOnlyProperties)); i++)
    if (LocaleCompare(ReadOnlyProperties[i],property) == 0)
      return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e P r o p e r t y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageProperty() saves the given string value either to specific known
%  attribute or to a freeform property string.
%
%  Attempting to set a property that is normally calculated will produce
%  an exception.
%
%  The format of the SetImageProperty method is:
%
%      MagickBooleanType SetImageProperty(Image *image,const char *property,
%        const char *value,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o property: the image property.
%
%    o values: the image property values.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void LowerImageDelay(Image *image,const GeometryInfo geometry_info)
{
  if (image->delay > (size_t) floor(geometry_info.rho+0.5))
    image->delay=(size_t) floor(geometry_info.rho+0.5);
}

static MagickBooleanType SetImageDelayProperty(Image *image,
  const char *value)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  flags=ParseGeometry(value,&geometry_info);
  if ((flags & GreaterValue) != 0)
    LowerImageDelay(image,geometry_info);
  else
    if ((flags & LessValue) != 0)
      {
        if ((double) image->delay < floor(geometry_info.rho+0.5))
          image->delay=(size_t) CastDoubleToSsizeT(floor(
            geometry_info.sigma+0.5));
      }
    else
      image->delay=(size_t) floor(geometry_info.rho+0.5);
  if ((flags & SigmaValue) != 0)
    image->ticks_per_second=CastDoubleToSsizeT(floor(
      geometry_info.sigma+0.5));
  return(MagickTrue);
}

static MagickBooleanType SetImageColorspaceProperty(Image *image,
  const char *value,ExceptionInfo *exception)
{
  ssize_t
    colorspace;

  colorspace=ParseCommandOption(MagickColorspaceOptions,MagickFalse,
    value);
  if (colorspace < 0)
    return(MagickFalse); /* FUTURE: value exception?? */
  return(SetImageColorspace(image,(ColorspaceType) colorspace,exception));
}

static MagickBooleanType SetImageDensityProperty(Image *image,
  const char *value,MagickStatusType *flags)
{
  GeometryInfo
    geometry_info;

  (*flags)=ParseGeometry(value,&geometry_info);
  if (((*flags) & RhoValue) != 0)
    image->resolution.x=geometry_info.rho;
  image->resolution.y=image->resolution.x;
  if (((*flags) & SigmaValue) != 0)
    image->resolution.y=geometry_info.sigma;
  return(MagickTrue);
}

typedef void
  (*SetImageMemberMethod)(Image *,const ssize_t);

typedef struct _EnumeratedPropertyInfo
{
  const char
    *property;

  CommandOption
    option;

  SetImageMemberMethod
    set_member;
} EnumeratedPropertyInfo;

static void SetImageComposeMember(Image *image,const ssize_t value)
{
  image->compose=(CompositeOperator) value;
}

static void SetImageCompressionMember(Image *image,const ssize_t value)
{
  image->compression=(CompressionType) value;
}

static void SetImageDisposeMember(Image *image,const ssize_t value)
{
  image->dispose=(DisposeType) value;
}

static void SetImageGravityMember(Image *image,const ssize_t value)
{
  image->gravity=(GravityType) value;
}

static void SetImageIntensityMember(Image *image,const ssize_t value)
{
  image->intensity=(PixelIntensityMethod) value;
}

static void SetImageRenderingIntentMember(Image *image,const ssize_t value)
{
  image->rendering_intent=(RenderingIntent) value;
}

static void SetImageInterpolateMember(Image *image,const ssize_t value)
{
  image->interpolate=(PixelInterpolateMethod) value;
}

static void SetImageTypeMember(Image *image,const ssize_t value)
{
  image->type=(ImageType) value;
}

static void SetImageUnitsMember(Image *image,const ssize_t value)
{
  image->units=(ResolutionType) value;
}

static const EnumeratedPropertyInfo
  EnumeratedProperties[] =
  {
    { "compose", MagickComposeOptions, SetImageComposeMember },
    { "compress", MagickCompressOptions, SetImageCompressionMember },
    { "dispose", MagickDisposeOptions, SetImageDisposeMember },
    { "gravity", MagickGravityOptions, SetImageGravityMember },
    { "intensity", MagickIntensityOptions, SetImageIntensityMember },
    { "intent", MagickIntentOptions, SetImageRenderingIntentMember },
    { "interpolate", MagickInterpolateOptions, SetImageInterpolateMember },
    { "rendering-intent", MagickIntentOptions, SetImageRenderingIntentMember },
    { "type", MagickTypeOptions, SetImageTypeMember },
    { "units", MagickResolutionOptions, SetImageUnitsMember }
  };

static MagickBooleanType SetEnumeratedProperty(Image *image,
  const char *property,const char *value,MagickBooleanType *result)
{
  ssize_t
    i,
    option;

  for (i=0; i < (ssize_t) (sizeof(EnumeratedProperties)/
       sizeof(*EnumeratedProperties)); i++)
    if (LocaleCompare(EnumeratedProperties[i].property,property) == 0)
      {
        option=ParseCommandOption(EnumeratedProperties[i].option,MagickFalse,
          value);
        if (option < 0)
          {
            (*result)=MagickFalse;
            return(MagickTrue);
          }
        EnumeratedProperties[i].set_member(image,option);
        (*result)=MagickTrue;
        return(MagickTrue);
      }
  return(MagickFalse);
}

typedef MagickBooleanType
  (*ImagePropertyHandler)(Image *,const char *,ExceptionInfo *);

typedef struct _ImagePropertyHandlerInfo
{
  const char
    *key;

  ImagePropertyHandler
    handler;
} ImagePropertyHandlerInfo;

static MagickBooleanType SetImageDelayHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  return(SetImageDelayProperty(image,value));
}

static MagickBooleanType SetImageDensityHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  MagickStatusType
    flags;

  magick_unreferenced(exception);
  return(SetImageDensityProperty(image,value,&flags));
}

static MagickBooleanType SetImageDepthHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  image->depth=StringToUnsignedLong(value);
  return(MagickTrue);
}

static MagickBooleanType SetImageGammaHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  image->gamma=StringToDouble(value,(char **) NULL);
  return(MagickTrue);
}

static MagickBooleanType SetImageLoopHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  magick_unreferenced(exception);
  image->iterations=StringToUnsignedLong(value);
  return(MagickTrue);
}

static MagickBooleanType SetImageGeometryMember(const char *value,
  RectangleInfo *member,MagickStatusType *flags)
{
  char
    *geometry;

  geometry=GetPageGeometry(value);
  (*flags)=ParseAbsoluteGeometry(geometry,member);
  geometry=DestroyString(geometry);
  return(MagickTrue);
}

static MagickBooleanType SetImagePageHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  MagickStatusType
    flags;

  magick_unreferenced(exception);
  return(SetImageGeometryMember(value,&image->page,&flags));
}

static MagickBooleanType SetImageTileOffsetHandler(Image *image,const char *value,
  ExceptionInfo *magick_unused(exception))
{
  MagickStatusType
    flags;

  magick_unreferenced(exception);
  return(SetImageGeometryMember(value,&image->tile_offset,&flags));
}

static const ImagePropertyHandlerInfo
  ImagePropertyHandlers[] =
  {
    { "colorspace", SetImageColorspaceProperty },
    { "delay", SetImageDelayHandler },
    { "density", SetImageDensityHandler },
    { "depth", SetImageDepthHandler },
    { "gamma", SetImageGammaHandler },
    { "loop", SetImageLoopHandler },
    { "page", SetImagePageHandler },
    { "tile-offset", SetImageTileOffsetHandler }
  };

static ImagePropertyHandler GetImagePropertyHandler(const char *property)
{
  size_t
    i;

  for (i=0; i < (sizeof(ImagePropertyHandlers)/sizeof(*ImagePropertyHandlers)); i++)
    if (LocaleCompare(ImagePropertyHandlers[i].key,property) == 0)
      return(ImagePropertyHandlers[i].handler);
  return((ImagePropertyHandler) NULL);
}

MagickExport MagickBooleanType SetImageProperty(Image *image,
  const char *property,const char *value,ExceptionInfo *exception)
{
  ImagePropertyHandler
    handler;

  MagickBooleanType
    status;

  size_t
    property_length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->properties == (void *) NULL)
    image->properties=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,RelinquishMagickMemory);  /* create splay-tree */
  if (value == (const char *) NULL)
    return(DeleteImageProperty(image,property));  /* delete if NULL */
  if (strlen(property) <= 1)
    {
      /*
        Do not 'set' single letter properties - read only shorthand.
      */
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "SetReadOnlyProperty","`%s'",property);
      return(MagickFalse);
    }
  property_length=strlen(property);
  if (IsWildcardProperty(property,property_length) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "SetReadOnlyProperty","`%s'",property);
      return(MagickFalse);
    }
  /*
    FUTURE: binary chars or quotes in key should produce a error
    Set attributes with known names or special prefixes
    return result is found, or break to set a free form property
  */
  if (SetEnumeratedProperty(image,property,value,&status) != MagickFalse)
    return(status);
  status=MagickTrue;
  if (IsReadOnlyProperty(property) != MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "SetReadOnlyProperty","`%s'",property);
      return(MagickFalse);
    }
  handler=GetImagePropertyHandler(property);
  if (handler != (ImagePropertyHandler) NULL)
    return(handler(image,value,exception));
  switch (*property)
  {
#include "MagickCore/property-set-8bim-private.h"
    case 'B':
    case 'b':
    {
      if (LocaleCompare("background",property) == 0)
        {
          (void) QueryColorCompliance(value,AllCompliance,
            &image->background_color,exception);
          /* check for FUTURE: value exception?? */
          /* also add user input to splay tree */
        }
      break; /* not an attribute, add as a property */
    }
#include "MagickCore/property-set-exif-fx-private.h"
    case 'I':
    case 'i':
    {
#include "MagickCore/property-set-iptc-private.h"
      break; /* not an attribute, add as a property */
    }
    case 'P':
    case 'p':
    {
#include "MagickCore/property-set-pixel-private.h"
      break; /* not an attribute, add as a property */
    }
#include "MagickCore/property-set-xmp-private.h"
  }
  /* Default: not an attribute, add as a property */
  status=AddValueToSplayTree((SplayTreeInfo *) image->properties,
    ConstantString(property),ConstantString(value));
  /* FUTURE: error if status is bad? */
  return(status);
}
