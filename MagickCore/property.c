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

static char
  *TracePSClippath(const unsigned char *,size_t),
  *TraceSVGClippath(const unsigned char *,size_t,const size_t,
    const size_t);

static char *AppendIPTCDataset(char *attribute,const StringInfo *profile,
  const unsigned char *p,const ssize_t i)
{
  char
    *message = (char *) NULL;

  size_t declared = ((size_t) p[3] << 8) | (size_t) p[4];
  size_t remaining = GetStringInfoLength(profile)-(i+5);
  size_t length = MagickMin(declared,remaining);
  if (~length >= 1)
    message=(char *) AcquireQuantumMemory(length+1UL,sizeof(*message));
  if (message != (char *) NULL)
    {
      /*
        Copy only the clamped length.
      */
      (void) memcpy(message,p+5,length);
      message[length]='\0';
      (void) ConcatenateString(&attribute,message);
      (void) ConcatenateString(&attribute,";");
      message=DestroyString(message);
    }
  return(attribute);
}

static void GetIPTCProperty(const Image *image,const char *key,
  ExceptionInfo *exception)
{
  char
    *attribute;

  const StringInfo
    *profile;

  long
    count,
    dataset,
    record;

  ssize_t
    i;

  profile=GetImageProfile(image,"iptc");
  if (profile == (StringInfo *) NULL)
    profile=GetImageProfile(image,"8bim");
  if (profile == (StringInfo *) NULL)
    return;
  count=MagickSscanf(key,"IPTC:%ld:%ld",&dataset,&record);
  if (count != 2)
    return;
  attribute=(char *) NULL;
  for (i=0; i < (ssize_t) GetStringInfoLength(profile)-5; )
  {
    const unsigned char *p = GetStringInfoDatum(profile)+i;
  
    if (p[0] != 0x1c)  /* Look for IPTC marker */
      {
        i++;
        continue;
      }
    /*
      Dataset and record.
    */
    if (((long) p[1] == dataset) && ((long) p[2] == record))
      attribute=AppendIPTCDataset(attribute,profile,p,i);
    /*
      Advance past this record header + data.
    */
    i+=(((size_t) p[3] << 8) | (size_t) p[4])+5;
  }
  if ((attribute == (char *) NULL) || (*attribute == ';'))
    {
      if (attribute != (char *) NULL)
        attribute=DestroyString(attribute);
      return;
    }
  attribute[strlen(attribute)-1]='\0';
  (void) SetImageProperty((Image *) image,key,(const char *) attribute,
    exception);
  attribute=DestroyString(attribute);
}

static inline int ReadPropertyByte(const unsigned char **p,size_t *length)
{
  int
    c;

  if (*length < 1)
    return(EOF);
  c=(int) (*(*p)++);
  (*length)--;
  return(c);
}

static inline void ReadPropertyBuffer(const unsigned char **p,size_t *length,
  unsigned char *buffer,const ssize_t count)
{
  int
    c;

  ssize_t
    i;

  for (i=0; i < count; i++)
  {
    c=(int) (*(*p)++);
    (*length)--;
    buffer[i]=(unsigned char) c;
  }
}

static inline signed int ReadPropertyMSBLong(const unsigned char **p,
  size_t *length)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  unsigned char
    buffer[4];

  unsigned int
    value;

  if (*length < 4)
    return(-1);
  ReadPropertyBuffer(p,length,buffer,4);
  value=(unsigned int) buffer[0] << 24;
  value|=(unsigned int) buffer[1] << 16;
  value|=(unsigned int) buffer[2] << 8;
  value|=(unsigned int) buffer[3];
  quantum.unsigned_value=value & 0xffffffff;
  return(quantum.signed_value);
}

static inline signed short ReadPropertyMSBShort(const unsigned char **p,
  size_t *length)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  unsigned char
    buffer[2];

  unsigned short
    value;

  if (*length < 2)
    return(-1);
  ReadPropertyBuffer(p,length,buffer,2);
  value=(unsigned short) buffer[0] << 8;
  value|=(unsigned short) buffer[1];
  quantum.unsigned_value=value & 0xffff;
  return(quantum.signed_value);
}

static inline MagickBooleanType IsInvalid8BIMFieldCount(const ssize_t count)
{
  return(((count != 2) && (count != 3) && (count != 4)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsUnrequestedResource(const char *resource,
  const char *macroman_resource,const char *name)
{
  return(((resource == (char *) NULL) || (macroman_resource == (char *) NULL) ||
    ((LocaleCompare(name,resource) != 0) &&
    (LocaleCompare(name,macroman_resource) != 0))) ? MagickTrue : MagickFalse);
}

static char *Trace8BIMClippath(const Image *image,const char *format,
  char *attribute,const ssize_t count)
{
  char
    *path;

  if (LocaleCompare(format,"svg") == 0)
    path=TraceSVGClippath((unsigned char *) attribute,(size_t) count,
      image->columns,image->rows);
  else
    path=TracePSClippath((unsigned char *) attribute,(size_t) count);
  return(path);
}

static void Read8BIMResourceBytes(char **resource,const unsigned char **info,
  size_t *length,const ssize_t count)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) count; i++)
    (*resource)[i]=(char) ReadPropertyByte(&(*info),&(*length));
  (*resource)[count]='\0';
}

static void Read8BIMResourceName(char **resource,const unsigned char **info,
  size_t *length,const ssize_t count)
{
  if ((count != 0) && ((size_t) count <= (*length)))
    {
      if (~((size_t) count) >= (MagickPathExtent-1))
        *resource=(char *) AcquireQuantumMemory((size_t) count+
          MagickPathExtent,sizeof(*(*resource)));
      if (*resource != (char *) NULL)
        Read8BIMResourceBytes(resource,info,length,count);
    }
}

typedef struct _AttributeInfo8BIM
{
  const Image
    *image;

  const char
    *key;

  char
    *format;

  ssize_t
    count,
    id;

  ExceptionInfo
    *exception;
} AttributeInfo8BIM;

static void Set8BIMAttribute(const AttributeInfo8BIM *attribute_info,
  char *attribute)
{
  const Image
    *const image = attribute_info->image;

  const char
    *const key = attribute_info->key;

  char
    *const format = attribute_info->format;

  const ssize_t
    count = attribute_info->count,
    id = attribute_info->id;

  ExceptionInfo
    *const exception = attribute_info->exception;

  if ((id <= 1999) || (id >= 2999))
    (void) SetImageProperty((Image *) image,key,(const char *) attribute,
      exception);
  else
    {
      char
        *path;

      path=Trace8BIMClippath(image,format,attribute,count);
      (void) SetImageProperty((Image *) image,key,(const char *) path,
        exception);
      path=DestroyString(path);
    }
}

static inline MagickBooleanType Is8BIMResourceName(const char *name)
{
  return(((*name != '\0') && (*name != '#')) ? MagickTrue : MagickFalse);
}

static void Get8BIMProperty(const Image *image,const char *key,
  ExceptionInfo *exception)
{
  char
    *attribute,
    format[MagickPathExtent],
    *macroman_resource = (char *) NULL,
    name[MagickPathExtent],
    *resource = (char *) NULL;

  const StringInfo
    *profile;

  const unsigned char
    *info;

  long
    start,
    stop;

  MagickBooleanType
    status;

  size_t
    length;

  ssize_t
    count,
    id,
    sub_number;

  /*
    There are no newlines in path names, so it's safe as terminator.
  */
  profile=GetImageProfile(image,"8bim");
  if (profile == (StringInfo *) NULL)
    return;
  count=(ssize_t) MagickSscanf(key,"8BIM:%ld,%ld:%1024[^\n]\n%1024[^\n]",
    &start,&stop,name,format);
  if (IsInvalid8BIMFieldCount(count) != MagickFalse)
    return;
  if (count < 4)
    (void) CopyMagickString(format,"SVG",MagickPathExtent);
  if (count < 3)
    *name='\0';
  sub_number=1;
  if (*name == '#')
    sub_number=(ssize_t) StringToLong(&name[1]);
  sub_number=MagickMax(sub_number,1L);
  status=MagickFalse;
  length=GetStringInfoLength(profile);
  info=GetStringInfoDatum(profile);
  while ((length > 0) && (status == MagickFalse))
  {
    if (ReadPropertyByte(&info,&length) != (unsigned char) '8')
      continue;
    if (ReadPropertyByte(&info,&length) != (unsigned char) 'B')
      continue;
    if (ReadPropertyByte(&info,&length) != (unsigned char) 'I')
      continue;
    if (ReadPropertyByte(&info,&length) != (unsigned char) 'M')
      continue;
    id=(ssize_t) ReadPropertyMSBShort(&info,&length);
    if (id < (ssize_t) start)
      continue;
    if (id > (ssize_t) stop)
      continue;
    if (macroman_resource != (char *) NULL)
      macroman_resource=DestroyString(macroman_resource);
    if (resource != (char *) NULL)
      resource=DestroyString(resource);
    count=(ssize_t) ReadPropertyByte(&info,&length);
    Read8BIMResourceName(&resource,&info,&length,count);
    if ((count & 0x01) == 0)
      (void) ReadPropertyByte(&info,&length);
    count=(ssize_t) ReadPropertyMSBLong(&info,&length);
    if ((count < 0) || ((size_t) count > length))
      {
        length=0;
        continue;
      }
    if (resource != (char *) NULL)
      macroman_resource=(char *) ConvertMacRomanToUTF8((unsigned char *)
        resource);
    if ((Is8BIMResourceName(name) != MagickFalse) &&
        (IsUnrequestedResource(resource,macroman_resource,name) != MagickFalse))
      {
        /*
          No name match, scroll forward and try next.
        */
        info+=count;
        length=(size_t) ((ssize_t) length-MagickMin(count,(ssize_t) length));
        continue;
      }
    if ((*name == '#') && (sub_number != 1))
      {
        /*
          No numbered match, scroll forward and try next.
        */
        sub_number--;
        info+=count;
        length=(size_t) ((ssize_t) length-MagickMin(count,(ssize_t) length));
        continue;
      }
    /*
      We have the resource of interest.
    */
    attribute=(char *) NULL;
    if (~((size_t) count) >= (MagickPathExtent-1))
      attribute=(char *) AcquireQuantumMemory((size_t) count+MagickPathExtent,
        sizeof(*attribute));
    if (attribute != (char *) NULL)
      {
        AttributeInfo8BIM
          attribute_info;

        (void) memcpy(attribute,(char *) info,(size_t) count);
        attribute[count]='\0';
        info+=count;
        length=(size_t) ((ssize_t) length-MagickMin(count,(ssize_t) length));
        attribute_info.image=image;
        attribute_info.key=key;
        attribute_info.format=format;
        attribute_info.count=count;
        attribute_info.id=id;
        attribute_info.exception=exception;
        Set8BIMAttribute(&attribute_info,attribute);
        attribute=DestroyString(attribute);
        status=MagickTrue;
      }
  }
  if (macroman_resource != (char *) NULL)
    macroman_resource=DestroyString(macroman_resource);
  if (resource != (char *) NULL)
    resource=DestroyString(resource);
}

static inline signed int ReadPropertySignedLong(const EndianType endian,
  const unsigned char *buffer)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  unsigned int
    value;

  if (endian == LSBEndian)
    {
      value=(unsigned int) buffer[3] << 24;
      value|=(unsigned int) buffer[2] << 16;
      value|=(unsigned int) buffer[1] << 8;
      value|=(unsigned int) buffer[0];
      quantum.unsigned_value=value & 0xffffffff;
      return(quantum.signed_value);
    }
  value=(unsigned int) buffer[0] << 24;
  value|=(unsigned int) buffer[1] << 16;
  value|=(unsigned int) buffer[2] << 8;
  value|=(unsigned int) buffer[3];
  quantum.unsigned_value=value & 0xffffffff;
  return(quantum.signed_value);
}

static inline unsigned int ReadPropertyUnsignedLong(const EndianType endian,
  const unsigned char *buffer)
{
  unsigned int
    value;

  if (endian == LSBEndian)
    {
      value=(unsigned int) buffer[3] << 24;
      value|=(unsigned int) buffer[2] << 16;
      value|=(unsigned int) buffer[1] << 8;
      value|=(unsigned int) buffer[0];
      return(value & 0xffffffff);
    }
  value=(unsigned int) buffer[0] << 24;
  value|=(unsigned int) buffer[1] << 16;
  value|=(unsigned int) buffer[2] << 8;
  value|=(unsigned int) buffer[3];
  return(value & 0xffffffff);
}

static inline signed short ReadPropertySignedShort(const EndianType endian,
  const unsigned char *buffer)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  unsigned short
    value;

  if (endian == LSBEndian)
    {
      value=(unsigned short) buffer[1] << 8;
      value|=(unsigned short) buffer[0];
      quantum.unsigned_value=value & 0xffff;
      return(quantum.signed_value);
    }
  value=(unsigned short) buffer[0] << 8;
  value|=(unsigned short) buffer[1];
  quantum.unsigned_value=value & 0xffff;
  return(quantum.signed_value);
}

static inline unsigned short ReadPropertyUnsignedShort(const EndianType endian,
  const unsigned char *buffer)
{
  unsigned short
    value;

  if (endian == LSBEndian)
    {
      value=(unsigned short) buffer[1] << 8;
      value|=(unsigned short) buffer[0];
      return(value & 0xffff);
    }
  value=(unsigned short) buffer[0] << 8;
  value|=(unsigned short) buffer[1];
  return(value & 0xffff);
}

#define MaxDirectoryStack  16
#define EXIF_DELIMITER  "\n"
#define EXIF_NUM_FORMATS  12
#define EXIF_FMT_BYTE  1
#define EXIF_FMT_STRING  2
#define EXIF_FMT_USHORT  3
#define EXIF_FMT_ULONG  4
#define EXIF_FMT_URATIONAL  5
#define EXIF_FMT_SBYTE  6
#define EXIF_FMT_UNDEFINED  7
#define EXIF_FMT_SSHORT  8
#define EXIF_FMT_SLONG  9
#define EXIF_FMT_SRATIONAL  10
#define EXIF_FMT_SINGLE  11
#define EXIF_FMT_DOUBLE  12
#define GPS_LATITUDE  0x10002
#define GPS_LONGITUDE  0x10004
#define GPS_TIMESTAMP  0x10007
#define TAG_EXIF_OFFSET  0x8769
#define TAG_GPS_OFFSET  0x8825
#define TAG_INTEROP_OFFSET  0xa005

#define EXIFGPSFractions(format,arg1,arg2,arg3,arg4,arg5,arg6) \
{ \
   size_t \
     extent = 0; \
 \
   ssize_t \
     component = 0; \
 \
   for ( ; component < components; component++) \
   { \
     if (component != 0) \
       extent+=(size_t) FormatLocaleString(buffer+extent,MagickPathExtent- \
         extent,", "); \
     extent+=(size_t) FormatLocaleString(buffer+extent,MagickPathExtent- \
       extent,format,(arg1),(arg2),(arg3),(arg4),(arg5),(arg6)); \
     if (extent >= (MagickPathExtent-1)) \
       extent=MagickPathExtent-1; \
   } \
   buffer[extent]='\0'; \
   value=AcquireString(buffer); \
}

#define EXIFMultipleValues(format,arg) \
{ \
   size_t \
     extent = 0; \
 \
   ssize_t \
     component = 0; \
 \
   for ( ; component < components; component++) \
   { \
     if (component != 0) \
       extent+=(size_t) FormatLocaleString(buffer+extent,MagickPathExtent- \
         extent,", "); \
     extent+=(size_t) FormatLocaleString(buffer+extent,MagickPathExtent- \
       extent,format,arg); \
     if (extent >= (MagickPathExtent-1)) \
       extent=MagickPathExtent-1; \
   } \
   buffer[extent]='\0'; \
   value=AcquireString(buffer); \
}

#define EXIFMultipleFractions(format,arg1,arg2) \
{ \
   size_t \
     extent = 0; \
 \
   ssize_t \
     component = 0; \
 \
   for ( ; component < components; component++) \
   { \
     if (component != 0) \
       extent+=(size_t) FormatLocaleString(buffer+extent,MagickPathExtent-\
         extent,", "); \
     extent+=(size_t) FormatLocaleString(buffer+extent,MagickPathExtent- \
       extent,format,(arg1),(arg2)); \
     if (extent >= (MagickPathExtent-1)) \
       extent=MagickPathExtent-1; \
   } \
   buffer[extent]='\0'; \
   value=AcquireString(buffer); \
}

static inline MagickBooleanType IsGPSFractionTag(const size_t tag_value)
{
  return(((tag_value == GPS_LATITUDE) || (tag_value == GPS_LONGITUDE) ||
    (tag_value == GPS_TIMESTAMP)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsSubdirectoryTag(const size_t tag_value)
{
  return(((tag_value == TAG_EXIF_OFFSET) ||
    (tag_value == TAG_INTEROP_OFFSET) || (tag_value == TAG_GPS_OFFSET)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNextIFDOffset(const ssize_t tag_offset1,
  const size_t length,const ssize_t level)
{
  return(((tag_offset1 != 0) && ((size_t) tag_offset1 < length) &&
    (level < (MaxDirectoryStack-2))) ? MagickTrue : MagickFalse);
}

typedef struct _DirectoryInfo
{
  const unsigned char
    *directory;

  size_t
    entry;

  ssize_t
    offset;
} DirectoryInfo;

typedef struct _TagInfo
{
  size_t
    tag;

  const char
    description[36];
} TagInfo;

static const char *GetEXIFTagDescription(const TagInfo *EXIFTag,
  const size_t tag_value)
{
  const char
    *description;

  ssize_t
    i;

  description="unknown";
  for (i=0; ; i++)
  {
    if (EXIFTag[i].tag == 0)
      break;
    if (EXIFTag[i].tag == tag_value)
      {
        description=EXIFTag[i].description;
        break;
      }
  }
  return(description);
}

static size_t GetEXIFTagByName(const TagInfo *EXIFTag,const char *property,
  size_t tag)
{
  ssize_t
    i;

  for (i=0; ; i++)
  {
    if (EXIFTag[i].tag == 0)
      break;
    if (LocaleCompare(EXIFTag[i].description,property) == 0)
      {
        tag=(size_t) EXIFTag[i].tag;
        break;
      }
  }
  return(tag);
}

static char *AcquireEXIFByteValue(const unsigned char *p,
  const ssize_t number_bytes)
{
  char
    *value;

  ssize_t
    i;

  value=(char *) NULL;
  if (~((size_t) number_bytes) >= 1)
    value=(char *) AcquireQuantumMemory((size_t) number_bytes+1UL,
      sizeof(*value));
  if (value != (char *) NULL)
    {
      for (i=0; i < (ssize_t) number_bytes; i++)
      {
        value[i]='.';
        if (isprint((int) p[i]) != 0)
          value[i]=(char) p[i];
      }
      value[i]='\0';
    }
  return(value);
}

static char *AcquireEXIFStringValue(const unsigned char *p,
  const ssize_t number_bytes)
{
  char
    *value;

  ssize_t
    i;

  value=(char *) NULL;
  if (~((size_t) number_bytes) >= 1)
    value=(char *) AcquireQuantumMemory((size_t) number_bytes+1UL,
      sizeof(*value));
  if (value != (char *) NULL)
    {
      for (i=0; i < (ssize_t) number_bytes; i++)
      {
        value[i]='.';
        if ((isprint((int) p[i]) != 0) || (p[i] == '\0'))
          value[i]=(char) p[i];
      }
      value[i]='\0';
    }
  return(value);
}

static void FormatEXIFTagIdKey(char *key,const size_t tag_value)
{
  if (tag_value < 0x10000)
    (void) FormatLocaleString(key,MagickPathExtent,"#%04lx",
      (unsigned long) tag_value);
  else
    if (tag_value < 0x20000)
      (void) FormatLocaleString(key,MagickPathExtent,"@%04lx",
        (unsigned long) (tag_value & 0xffff));
    else
      (void) FormatLocaleString(key,MagickPathExtent,"unknown");
}

static int
  tag_bytes[] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};

static MagickBooleanType ReadEXIFMarker(const unsigned char **exif,
  size_t *length,MagickBooleanType *broke)
{
  (*broke)=MagickFalse;
  if (ReadPropertyByte(exif,length) != 0x45)
    return(MagickFalse);
  if (ReadPropertyByte(exif,length) != 0x78)
    return(MagickFalse);
  if (ReadPropertyByte(exif,length) != 0x69)
    return(MagickFalse);
  if (ReadPropertyByte(exif,length) != 0x66)
    return(MagickFalse);
  if (ReadPropertyByte(exif,length) != 0x00)
    return(MagickFalse);
  if (ReadPropertyByte(exif,length) != 0x00)
    return(MagickFalse);
  (*broke)=MagickTrue;
  return(MagickTrue);
}

static void SkipToEXIFHeader(const unsigned char **exif,size_t *length)
{
  MagickBooleanType
    broke = MagickFalse;

  while ((*length) != 0)
  {
    if (ReadEXIFMarker(exif,length,&broke) == MagickFalse)
      continue;
    if (broke != MagickFalse)
      break;
  }
}

static MagickBooleanType ReadEXIFHeader(const unsigned char **exif,
  size_t *length,EndianType *endian)
{
  ssize_t
    id;

  SkipToEXIFHeader(exif,length);
  if ((*length) < 16)
    return(MagickFalse);
  id=(ssize_t) ReadPropertySignedShort(LSBEndian,(*exif));
  (*endian)=LSBEndian;
  if (id == 0x4949)
    (*endian)=LSBEndian;
  else
    if (id == 0x4D4D)
      (*endian)=MSBEndian;
    else
      return(MagickFalse);
  if (ReadPropertyUnsignedShort((*endian),(*exif)+2) != 0x002a)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType ParseEXIFHexTag(const char **property,size_t *tag)
{
  int
    c;

  size_t
    n;

  ssize_t
    i;

  n=strlen((*property));
  if (n != 4)
    return(MagickFalse);
  /*
    Parse tag specification as a hex number.
  */
  n/=4;
  do
  {
    for (i=(ssize_t) n-1L; i >= 0; i--)
    {
      c=(*(*property)++);
      (*tag)<<=4;
      if ((c >= '0') && (c <= '9'))
        (*tag)|=(size_t) (c-'0');
      else
        if ((c >= 'A') && (c <= 'F'))
          (*tag)|=(size_t) (c-('A'-10));
        else
          if ((c >= 'a') && (c <= 'f'))
            (*tag)|=(size_t) (c-('a'-10));
          else
            return(MagickFalse);
    }
  } while (*(*property) != '\0');
  return(MagickTrue);
}

static MagickBooleanType ParseEXIFTagRequest(const TagInfo *EXIFTag,
  const char **property,ssize_t *all,size_t *tag)
{
  (*all)=0;
  (*tag)=(~0UL);
  switch (*((*property)+5))
  {
    case '*':
    {
      /*
        Caller has asked for all the tags in the EXIF data.
      */
      (*tag)=0;
      (*all)=1; /* return the data in description=value format */
      break;
    }
    case '!':
    {
      (*tag)=0;
      (*all)=2; /* return the data in tagid=value format */
      break;
    }
    case '#':
    case '@':
    {
      /*
        Check for a hex based tag specification first.
      */
      (*tag)=(*((*property)+5) == '@') ? 1UL : 0UL;
      (*property)+=6;
      if (ParseEXIFHexTag(property,tag) == MagickFalse)
        return(MagickFalse);
      break;
    }
    default:
    {
      /*
        Try to match the text with a tag name instead.
      */
      (*tag)=GetEXIFTagByName(EXIFTag,(*property),(*tag));
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _EXIFWalkInfo
{
  const Image
    *image;

  const char
    *property;

  const TagInfo
    *EXIFTag;

  ssize_t
    all;

  size_t
    tag;

  EndianType
    endian;

  const unsigned char
    *exif;

  size_t
    length;

  const unsigned char
    *directory;

  ssize_t
    tag_offset;

  SplayTreeInfo
    *exif_resources;

  ExceptionInfo
    *exception;
} EXIFWalkInfo;

typedef struct _EXIFWalkTargets
{
  ssize_t
    *level;

  size_t
    *entry;

  DirectoryInfo
    *directory_stack;
} EXIFWalkTargets;

typedef struct _EXIFTagInfo
{
  size_t
    tag_value,
    format;

  ssize_t
    components,
    number_bytes;

  unsigned char
    *p;
} EXIFTagInfo;

static char *AcquireEXIFTagValue(const EXIFWalkInfo *walk,
  const EXIFTagInfo *exif_tag)
{
  const EndianType
    endian = walk->endian;

  const unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  const size_t
    tag_value = exif_tag->tag_value,
    format = exif_tag->format;

  ssize_t
    components = exif_tag->components;

  const ssize_t
    number_bytes = exif_tag->number_bytes;

  unsigned char
    *const p = exif_tag->p;

  char
    buffer[6*sizeof(double)+MagickPathExtent],
    *value;

  value=(char *) NULL;
  *buffer='\0';
  switch (format)
  {
    case EXIF_FMT_BYTE:
    {
      value=AcquireEXIFByteValue(p,number_bytes);
      break;
    }
    case EXIF_FMT_SBYTE:
    {
      EXIFMultipleValues("%.17g",(double) (*(signed char *) p));
      break;
    }
    case EXIF_FMT_SSHORT:
    {
      EXIFMultipleValues("%hd",ReadPropertySignedShort(endian,p));
      break;
    }
    case EXIF_FMT_USHORT:
    {
      EXIFMultipleValues("%hu",ReadPropertyUnsignedShort(endian,p));
      break;
    }
    case EXIF_FMT_ULONG:
    {
      EXIFMultipleValues("%.17g",(double)
        ReadPropertyUnsignedLong(endian,p));
      break;
    }
    case EXIF_FMT_SLONG:
    {
      EXIFMultipleValues("%.17g",(double)
        ReadPropertySignedLong(endian,p));
      break;
    }
    case EXIF_FMT_URATIONAL:
    {
      if (IsGPSFractionTag(tag_value) != MagickFalse)
        {
          if (number_bytes < 24)
            break;  /* reads three rationals */
          components=1;
          EXIFGPSFractions("%.17g/%.17g,%.17g/%.17g,%.17g/%.17g",
            (double) ReadPropertyUnsignedLong(endian,p),
            (double) ReadPropertyUnsignedLong(endian,p+4),
            (double) ReadPropertyUnsignedLong(endian,p+8),
            (double) ReadPropertyUnsignedLong(endian,p+12),
            (double) ReadPropertyUnsignedLong(endian,p+16),
            (double) ReadPropertyUnsignedLong(endian,p+20));
          break;
        }
      EXIFMultipleFractions("%.17g/%.17g",(double)
        ReadPropertyUnsignedLong(endian,p),(double)
        ReadPropertyUnsignedLong(endian,p+4));
      break;
    }
    case EXIF_FMT_SRATIONAL:
    {
      EXIFMultipleFractions("%.17g/%.17g",(double)
        ReadPropertySignedLong(endian,p),(double)
        ReadPropertySignedLong(endian,p+4));
      break;
    }
    case EXIF_FMT_SINGLE:
    {
      EXIFMultipleValues("%.17g",(double)
        ReadPropertySignedLong(endian,p));
      break;
    }
    case EXIF_FMT_DOUBLE:
    {
      EXIFMultipleValues("%.17g",(double)
        ReadPropertySignedLong(endian,p));
      break;
    }
    case EXIF_FMT_STRING:
    case EXIF_FMT_UNDEFINED:
    default:
    {
      if ((p < exif) || (p > (exif+length-number_bytes)))
        break;
      value=AcquireEXIFStringValue(p,number_bytes);
      break;
    }
  }
  return(value);
}

static void SetEXIFTagProperty(const EXIFWalkInfo *walk,
  const size_t tag_value,const ssize_t level,char *value)
{
  const Image
    *const image = walk->image;

  const char
    *const property = walk->property;

  const TagInfo
    *const EXIFTag = walk->EXIFTag;

  const ssize_t
    all = walk->all;

  ExceptionInfo
    *const exception = walk->exception;

  char
    *key;

  key=AcquireString(property);
  switch (all)
  {
    case 1:
    {
      const char
        *description;

      description=GetEXIFTagDescription(EXIFTag,tag_value);
      (void) FormatLocaleString(key,MagickPathExtent,"%s",
        description);
      if (level == 2)
        (void) SubstituteString(&key,"exif:","exif:Thumbnail.");
      break;
    }
    case 2:
    {
      FormatEXIFTagIdKey(key,tag_value);
      break;
    }
    default:
    {
      if (level == 2)
        (void) SubstituteString(&key,"exif:","exif:Thumbnail.");
    }
  }
  if ((image->properties == (void *) NULL) ||
      (GetValueFromSplayTree((SplayTreeInfo *) image->properties,key) == (const void *) NULL))
    (void) SetImageProperty((Image *) image,key,value,exception);
  value=DestroyString(value);
  key=DestroyString(key);
}

static MagickBooleanType GetEXIFTagProperty(const EXIFWalkInfo *walk,
  const EXIFTagInfo *exif_tag,const ssize_t level)
{
  const ssize_t
    all = walk->all;

  const size_t
    tag = walk->tag;

  const unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  const size_t
    tag_value = exif_tag->tag_value,
    format = exif_tag->format;

  unsigned char
    *const p = exif_tag->p;

  if ((all != 0) || (tag == (size_t) tag_value))
    {
      char
        *value;

      if ((p < exif) || (p > (exif+length-tag_bytes[format])))
        return(MagickFalse);
      value=AcquireEXIFTagValue(walk,exif_tag);
      if (value != (char *) NULL)
        SetEXIFTagProperty(walk,tag_value,level,value);
    }
  return(MagickTrue);
}

static MagickBooleanType PushEXIFDirectory(const EXIFWalkInfo *walk,
  const EXIFTagInfo *exif_tag,const size_t number_entries,
  const EXIFWalkTargets *targets)
{
  const EndianType
    endian = walk->endian;

  const unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  const unsigned char
    *const directory = walk->directory;

  const ssize_t
    tag_offset = walk->tag_offset;

  const size_t
    tag_value = exif_tag->tag_value;

  unsigned char
    *const p = exif_tag->p;

  ssize_t
    *const level = targets->level;

  size_t
    *const entry = targets->entry;

  DirectoryInfo
    *const directory_stack = targets->directory_stack;

  ssize_t
    i,
    tag_offset1;

  if (IsSubdirectoryTag(tag_value) == MagickFalse)
    return(MagickFalse);
  tag_offset1=(ssize_t) ReadPropertySignedLong(endian,p);
  if (((size_t) tag_offset1 < length) &&
      ((*level) < (MaxDirectoryStack-2)))
    {
      ssize_t
        tag_offset2;

      tag_offset2=(ssize_t) ((tag_value == TAG_GPS_OFFSET) ? 0x10000 :
        0);
      directory_stack[(*level)].directory=directory;
      (*entry)++;
      directory_stack[(*level)].entry=(*entry);
      directory_stack[(*level)].offset=tag_offset;
      (*level)++;
      /*
        Check for duplicate tag.
      */
      for (i=0; i < (*level); i++)
        if (directory_stack[i].directory == (exif+tag_offset1))
          break;
      if (i < (*level))
        return(MagickTrue);  /* duplicate tag */
      directory_stack[(*level)].directory=exif+tag_offset1;
      directory_stack[(*level)].offset=tag_offset2;
      directory_stack[(*level)].entry=0;
      (*level)++;
      if ((directory+2+(12*number_entries)+4) > (exif+length))
        return(MagickTrue);
      tag_offset1=(ssize_t) ReadPropertySignedLong(endian,directory+
        2+(12*number_entries));
      if (IsNextIFDOffset(tag_offset1,length,(*level)) != MagickFalse)
        {
          directory_stack[(*level)].directory=exif+tag_offset1;
          directory_stack[(*level)].entry=0;
          directory_stack[(*level)].offset=tag_offset2;
          (*level)++;
        }
    }
  return(MagickTrue);
}

typedef struct _EXIFEntryTargets
{
  unsigned char
    **q;

  size_t
    *tag_value,
    *format;

  ssize_t
    *components,
    *number_bytes;
} EXIFEntryTargets;

static MagickBooleanType ReadEXIFEntry(const EXIFWalkInfo *walk,
  const size_t entry,const EXIFEntryTargets *targets)
{
  const EndianType
    endian = walk->endian;

  const unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  const unsigned char
    *const directory = walk->directory;

  const ssize_t
    tag_offset = walk->tag_offset;

  SplayTreeInfo
    *const exif_resources = walk->exif_resources;

  unsigned char
    **const q = targets->q;

  size_t
    *const tag_value = targets->tag_value,
    *const format = targets->format;

  ssize_t
    *const components = targets->components,
    *const number_bytes = targets->number_bytes;

  (*q)=(unsigned char *) (directory+(12*entry)+2);
  if ((*q) > (exif+length-12))
    return(MagickFalse);  /* corrupt EXIF */
  if (GetValueFromSplayTree(exif_resources,(*q)) == (*q))
    return(MagickFalse);
  (void) AddValueToSplayTree(exif_resources,(*q),(*q));
  (*tag_value)=(size_t) (ReadPropertyUnsignedShort(endian,(*q))+(ssize_t)
    tag_offset);
  (*format)=(size_t) ReadPropertyUnsignedShort(endian,(*q)+2);
  if ((*format) >= (sizeof(tag_bytes)/sizeof(*tag_bytes)))
    return(MagickFalse);
  if ((*format) == 0)
    return(MagickFalse);  /* corrupt EXIF */
  (*components)=(ssize_t) ReadPropertySignedLong(endian,(*q)+4);
  if ((*components) < 0)
    return(MagickFalse);  /* corrupt EXIF */
  (*number_bytes)=(*components)*(ssize_t) tag_bytes[(*format)];
  if ((*number_bytes) < (*components))
    return(MagickFalse);  /* prevent overflow */
  return(MagickTrue);
}

static MagickBooleanType LocateEXIFEntryValue(const EXIFWalkInfo *walk,
  unsigned char *q,const ssize_t number_bytes,unsigned char **p)
{
  const EndianType
    endian = walk->endian;

  const unsigned char
    *const exif = walk->exif;

  const size_t
    length = walk->length;

  if (number_bytes <= 4)
    (*p)=q+8;
  else
    {
      ssize_t
        dir_offset;

      /*
        The directory entry contains an offset.
      */
      dir_offset=(ssize_t) ReadPropertySignedLong(endian,q+8);
      if ((dir_offset < 0) || (size_t) dir_offset >= length)
        return(MagickFalse);
      if (((size_t) dir_offset+(size_t) number_bytes) < (size_t) dir_offset)
        return(MagickFalse);  /* prevent overflow */
      if ((size_t) (dir_offset+(ssize_t) number_bytes) > length)
        return(MagickFalse);
      (*p)=(unsigned char *) (exif+dir_offset);
    }
  return(MagickTrue);
}

static MagickBooleanType GetEXIFEntry(const EXIFWalkInfo *walk,
  const size_t number_entries,const EXIFWalkTargets *targets)
{
  ssize_t
    *const level = targets->level;

  size_t
    *const entry = targets->entry;

  size_t
    tag_value;

  EXIFTagInfo
    exif_tag;

  size_t
    format;

  ssize_t
    components,
    number_bytes;

  unsigned char
    *p,
    *q;

  const EXIFEntryTargets
    entry_targets = { &q, &tag_value, &format, &components, &number_bytes };

  if (ReadEXIFEntry(walk,(*entry),&entry_targets) == MagickFalse)
    return(MagickTrue);
  if (LocateEXIFEntryValue(walk,q,number_bytes,&p) == MagickFalse)
    return(MagickFalse);
  exif_tag.tag_value=tag_value;
  exif_tag.format=format;
  exif_tag.components=components;
  exif_tag.number_bytes=number_bytes;
  exif_tag.p=p;
  if (GetEXIFTagProperty(walk,&exif_tag,(*level)) == MagickFalse)
    return(MagickTrue);
  if (PushEXIFDirectory(walk,&exif_tag,number_entries,targets) !=
      MagickFalse)
    return(MagickTrue);
  return(MagickFalse);
}

static void GetEXIFDirectory(const EXIFWalkInfo *walk,
  const EXIFWalkTargets *targets)
{
  const EndianType
    endian = walk->endian;

  const unsigned char
    *const directory = walk->directory;

  size_t
    *const entry = targets->entry;

  size_t
    number_entries;

  /*
    Determine how many entries there are in the current IFD.
  */
  number_entries=(size_t) ReadPropertyUnsignedShort(endian,directory);
  for ( ; (*entry) < number_entries; (*entry)++)
  {
    if (GetEXIFEntry(walk,number_entries,targets) != MagickFalse)
      break;
  }
}

static void GetEXIFProperty(const Image *image,const char *property,
  ExceptionInfo *exception)
{
  static const TagInfo
    EXIFTag[] =
    {
      {  0x001, "exif:InteroperabilityIndex" },
      {  0x002, "exif:InteroperabilityVersion" },
      {  0x100, "exif:ImageWidth" },
      {  0x101, "exif:ImageLength" },
      {  0x102, "exif:BitsPerSample" },
      {  0x103, "exif:Compression" },
      {  0x106, "exif:PhotometricInterpretation" },
      {  0x10a, "exif:FillOrder" },
      {  0x10d, "exif:DocumentName" },
      {  0x10e, "exif:ImageDescription" },
      {  0x10f, "exif:Make" },
      {  0x110, "exif:Model" },
      {  0x111, "exif:StripOffsets" },
      {  0x112, "exif:Orientation" },
      {  0x115, "exif:SamplesPerPixel" },
      {  0x116, "exif:RowsPerStrip" },
      {  0x117, "exif:StripByteCounts" },
      {  0x11a, "exif:XResolution" },
      {  0x11b, "exif:YResolution" },
      {  0x11c, "exif:PlanarConfiguration" },
      {  0x11d, "exif:PageName" },
      {  0x11e, "exif:XPosition" },
      {  0x11f, "exif:YPosition" },
      {  0x118, "exif:MinSampleValue" },
      {  0x119, "exif:MaxSampleValue" },
      {  0x120, "exif:FreeOffsets" },
      {  0x121, "exif:FreeByteCounts" },
      {  0x122, "exif:GrayResponseUnit" },
      {  0x123, "exif:GrayResponseCurve" },
      {  0x124, "exif:T4Options" },
      {  0x125, "exif:T6Options" },
      {  0x128, "exif:ResolutionUnit" },
      {  0x12d, "exif:TransferFunction" },
      {  0x131, "exif:Software" },
      {  0x132, "exif:DateTime" },
      {  0x13b, "exif:Artist" },
      {  0x13e, "exif:WhitePoint" },
      {  0x13f, "exif:PrimaryChromaticities" },
      {  0x140, "exif:ColorMap" },
      {  0x141, "exif:HalfToneHints" },
      {  0x142, "exif:TileWidth" },
      {  0x143, "exif:TileLength" },
      {  0x144, "exif:TileOffsets" },
      {  0x145, "exif:TileByteCounts" },
      {  0x14a, "exif:SubIFD" },
      {  0x14c, "exif:InkSet" },
      {  0x14d, "exif:InkNames" },
      {  0x14e, "exif:NumberOfInks" },
      {  0x150, "exif:DotRange" },
      {  0x151, "exif:TargetPrinter" },
      {  0x152, "exif:ExtraSample" },
      {  0x153, "exif:SampleFormat" },
      {  0x154, "exif:SMinSampleValue" },
      {  0x155, "exif:SMaxSampleValue" },
      {  0x156, "exif:TransferRange" },
      {  0x157, "exif:ClipPath" },
      {  0x158, "exif:XClipPathUnits" },
      {  0x159, "exif:YClipPathUnits" },
      {  0x15a, "exif:Indexed" },
      {  0x15b, "exif:JPEGTables" },
      {  0x15f, "exif:OPIProxy" },
      {  0x200, "exif:JPEGProc" },
      {  0x201, "exif:JPEGInterchangeFormat" },
      {  0x202, "exif:JPEGInterchangeFormatLength" },
      {  0x203, "exif:JPEGRestartInterval" },
      {  0x205, "exif:JPEGLosslessPredictors" },
      {  0x206, "exif:JPEGPointTransforms" },
      {  0x207, "exif:JPEGQTables" },
      {  0x208, "exif:JPEGDCTables" },
      {  0x209, "exif:JPEGACTables" },
      {  0x211, "exif:YCbCrCoefficients" },
      {  0x212, "exif:YCbCrSubSampling" },
      {  0x213, "exif:YCbCrPositioning" },
      {  0x214, "exif:ReferenceBlackWhite" },
      {  0x2bc, "exif:ExtensibleMetadataPlatform" },
      {  0x301, "exif:Gamma" },
      {  0x302, "exif:ICCProfileDescriptor" },
      {  0x303, "exif:SRGBRenderingIntent" },
      {  0x320, "exif:ImageTitle" },
      {  0x5001, "exif:ResolutionXUnit" },
      {  0x5002, "exif:ResolutionYUnit" },
      {  0x5003, "exif:ResolutionXLengthUnit" },
      {  0x5004, "exif:ResolutionYLengthUnit" },
      {  0x5005, "exif:PrintFlags" },
      {  0x5006, "exif:PrintFlagsVersion" },
      {  0x5007, "exif:PrintFlagsCrop" },
      {  0x5008, "exif:PrintFlagsBleedWidth" },
      {  0x5009, "exif:PrintFlagsBleedWidthScale" },
      {  0x500A, "exif:HalftoneLPI" },
      {  0x500B, "exif:HalftoneLPIUnit" },
      {  0x500C, "exif:HalftoneDegree" },
      {  0x500D, "exif:HalftoneShape" },
      {  0x500E, "exif:HalftoneMisc" },
      {  0x500F, "exif:HalftoneScreen" },
      {  0x5010, "exif:JPEGQuality" },
      {  0x5011, "exif:GridSize" },
      {  0x5012, "exif:ThumbnailFormat" },
      {  0x5013, "exif:ThumbnailWidth" },
      {  0x5014, "exif:ThumbnailHeight" },
      {  0x5015, "exif:ThumbnailColorDepth" },
      {  0x5016, "exif:ThumbnailPlanes" },
      {  0x5017, "exif:ThumbnailRawBytes" },
      {  0x5018, "exif:ThumbnailSize" },
      {  0x5019, "exif:ThumbnailCompressedSize" },
      {  0x501a, "exif:ColorTransferFunction" },
      {  0x501b, "exif:ThumbnailData" },
      {  0x5020, "exif:ThumbnailImageWidth" },
      {  0x5021, "exif:ThumbnailImageHeight" },
      {  0x5022, "exif:ThumbnailBitsPerSample" },
      {  0x5023, "exif:ThumbnailCompression" },
      {  0x5024, "exif:ThumbnailPhotometricInterp" },
      {  0x5025, "exif:ThumbnailImageDescription" },
      {  0x5026, "exif:ThumbnailEquipMake" },
      {  0x5027, "exif:ThumbnailEquipModel" },
      {  0x5028, "exif:ThumbnailStripOffsets" },
      {  0x5029, "exif:ThumbnailOrientation" },
      {  0x502a, "exif:ThumbnailSamplesPerPixel" },
      {  0x502b, "exif:ThumbnailRowsPerStrip" },
      {  0x502c, "exif:ThumbnailStripBytesCount" },
      {  0x502d, "exif:ThumbnailResolutionX" },
      {  0x502e, "exif:ThumbnailResolutionY" },
      {  0x502f, "exif:ThumbnailPlanarConfig" },
      {  0x5030, "exif:ThumbnailResolutionUnit" },
      {  0x5031, "exif:ThumbnailTransferFunction" },
      {  0x5032, "exif:ThumbnailSoftwareUsed" },
      {  0x5033, "exif:ThumbnailDateTime" },
      {  0x5034, "exif:ThumbnailArtist" },
      {  0x5035, "exif:ThumbnailWhitePoint" },
      {  0x5036, "exif:ThumbnailPrimaryChromaticities" },
      {  0x5037, "exif:ThumbnailYCbCrCoefficients" },
      {  0x5038, "exif:ThumbnailYCbCrSubsampling" },
      {  0x5039, "exif:ThumbnailYCbCrPositioning" },
      {  0x503A, "exif:ThumbnailRefBlackWhite" },
      {  0x503B, "exif:ThumbnailCopyRight" },
      {  0x5090, "exif:LuminanceTable" },
      {  0x5091, "exif:ChrominanceTable" },
      {  0x5100, "exif:FrameDelay" },
      {  0x5101, "exif:LoopCount" },
      {  0x5110, "exif:PixelUnit" },
      {  0x5111, "exif:PixelPerUnitX" },
      {  0x5112, "exif:PixelPerUnitY" },
      {  0x5113, "exif:PaletteHistogram" },
      {  0x1000, "exif:RelatedImageFileFormat" },
      {  0x1001, "exif:RelatedImageLength" },
      {  0x1002, "exif:RelatedImageWidth" },
      {  0x800d, "exif:ImageID" },
      {  0x80e3, "exif:Matteing" },
      {  0x80e4, "exif:DataType" },
      {  0x80e5, "exif:ImageDepth" },
      {  0x80e6, "exif:TileDepth" },
      {  0x828d, "exif:CFARepeatPatternDim" },
      {  0x828e, "exif:CFAPattern2" },
      {  0x828f, "exif:BatteryLevel" },
      {  0x8298, "exif:Copyright" },
      {  0x829a, "exif:ExposureTime" },
      {  0x829d, "exif:FNumber" },
      {  0x83bb, "exif:IPTC/NAA" },
      {  0x84e3, "exif:IT8RasterPadding" },
      {  0x84e5, "exif:IT8ColorTable" },
      {  0x8649, "exif:ImageResourceInformation" },
      {  0x8769, "exif:ExifOffset" },  /* specs as "Exif IFD Pointer"? */
      {  0x8773, "exif:InterColorProfile" },
      {  0x8822, "exif:ExposureProgram" },
      {  0x8824, "exif:SpectralSensitivity" },
      {  0x8825, "exif:GPSInfo" }, /* specs as "GPSInfo IFD Pointer"? */
      {  0x8827, "exif:PhotographicSensitivity" },
      {  0x8828, "exif:OECF" },
      {  0x8829, "exif:Interlace" },
      {  0x882a, "exif:TimeZoneOffset" },
      {  0x882b, "exif:SelfTimerMode" },
      {  0x8830, "exif:SensitivityType" },
      {  0x8831, "exif:StandardOutputSensitivity" },
      {  0x8832, "exif:RecommendedExposureIndex" },
      {  0x8833, "exif:ISOSpeed" },
      {  0x8834, "exif:ISOSpeedLatitudeyyy" },
      {  0x8835, "exif:ISOSpeedLatitudezzz" },
      {  0x9000, "exif:ExifVersion" },
      {  0x9003, "exif:DateTimeOriginal" },
      {  0x9004, "exif:DateTimeDigitized" },
      {  0x9010, "exif:OffsetTime" },
      {  0x9011, "exif:OffsetTimeOriginal" },
      {  0x9012, "exif:OffsetTimeDigitized" },
      {  0x9101, "exif:ComponentsConfiguration" },
      {  0x9102, "exif:CompressedBitsPerPixel" },
      {  0x9201, "exif:ShutterSpeedValue" },
      {  0x9202, "exif:ApertureValue" },
      {  0x9203, "exif:BrightnessValue" },
      {  0x9204, "exif:ExposureBiasValue" },
      {  0x9205, "exif:MaxApertureValue" },
      {  0x9206, "exif:SubjectDistance" },
      {  0x9207, "exif:MeteringMode" },
      {  0x9208, "exif:LightSource" },
      {  0x9209, "exif:Flash" },
      {  0x920a, "exif:FocalLength" },
      {  0x920b, "exif:FlashEnergy" },
      {  0x920c, "exif:SpatialFrequencyResponse" },
      {  0x920d, "exif:Noise" },
      {  0x9214, "exif:SubjectArea" },
      {  0x9290, "exif:SubSecTime" },
      {  0x9291, "exif:SubSecTimeOriginal" },
      {  0x9292, "exif:SubSecTimeDigitized" },
      {  0x9211, "exif:ImageNumber" },
      {  0x9212, "exif:SecurityClassification" },
      {  0x9213, "exif:ImageHistory" },
      {  0x9214, "exif:SubjectArea" },
      {  0x9215, "exif:ExposureIndex" },
      {  0x9216, "exif:TIFF-EPStandardID" },
      {  0x927c, "exif:MakerNote" },
      {  0x9286, "exif:UserComment" },
      {  0x9290, "exif:SubSecTime" },
      {  0x9291, "exif:SubSecTimeOriginal" },
      {  0x9292, "exif:SubSecTimeDigitized" },
      {  0x9400, "exif:Temperature" },
      {  0x9401, "exif:Humidity" },
      {  0x9402, "exif:Pressure" },
      {  0x9403, "exif:WaterDepth" },
      {  0x9404, "exif:Acceleration" },
      {  0x9405, "exif:CameraElevationAngle" },
      {  0x9C9b, "exif:WinXP-Title" },
      {  0x9C9c, "exif:WinXP-Comments" },
      {  0x9C9d, "exif:WinXP-Author" },
      {  0x9C9e, "exif:WinXP-Keywords" },
      {  0x9C9f, "exif:WinXP-Subject" },
      {  0xa000, "exif:FlashPixVersion" },
      {  0xa001, "exif:ColorSpace" },
      {  0xa002, "exif:PixelXDimension" },
      {  0xa003, "exif:PixelYDimension" },
      {  0xa004, "exif:RelatedSoundFile" },
      {  0xa005, "exif:InteroperabilityOffset" },
      {  0xa20b, "exif:FlashEnergy" },
      {  0xa20c, "exif:SpatialFrequencyResponse" },
      {  0xa20d, "exif:Noise" },
      {  0xa20e, "exif:FocalPlaneXResolution" },
      {  0xa20f, "exif:FocalPlaneYResolution" },
      {  0xa210, "exif:FocalPlaneResolutionUnit" },
      {  0xa214, "exif:SubjectLocation" },
      {  0xa215, "exif:ExposureIndex" },
      {  0xa216, "exif:TIFF/EPStandardID" },
      {  0xa217, "exif:SensingMethod" },
      {  0xa300, "exif:FileSource" },
      {  0xa301, "exif:SceneType" },
      {  0xa302, "exif:CFAPattern" },
      {  0xa401, "exif:CustomRendered" },
      {  0xa402, "exif:ExposureMode" },
      {  0xa403, "exif:WhiteBalance" },
      {  0xa404, "exif:DigitalZoomRatio" },
      {  0xa405, "exif:FocalLengthIn35mmFilm" },
      {  0xa406, "exif:SceneCaptureType" },
      {  0xa407, "exif:GainControl" },
      {  0xa408, "exif:Contrast" },
      {  0xa409, "exif:Saturation" },
      {  0xa40a, "exif:Sharpness" },
      {  0xa40b, "exif:DeviceSettingDescription" },
      {  0xa40c, "exif:SubjectDistanceRange" },
      {  0xa420, "exif:ImageUniqueID" },
      {  0xa430, "exif:CameraOwnerName" },
      {  0xa431, "exif:BodySerialNumber" },
      {  0xa432, "exif:LensSpecification" },
      {  0xa433, "exif:LensMake" },
      {  0xa434, "exif:LensModel" },
      {  0xa435, "exif:LensSerialNumber" },
      {  0xc4a5, "exif:PrintImageMatching" },
      {  0xa500, "exif:Gamma" },
      {  0xc640, "exif:CR2Slice" },
      { 0x10000, "exif:GPSVersionID" },
      { 0x10001, "exif:GPSLatitudeRef" },
      { 0x10002, "exif:GPSLatitude" },
      { 0x10003, "exif:GPSLongitudeRef" },
      { 0x10004, "exif:GPSLongitude" },
      { 0x10005, "exif:GPSAltitudeRef" },
      { 0x10006, "exif:GPSAltitude" },
      { 0x10007, "exif:GPSTimeStamp" },
      { 0x10008, "exif:GPSSatellites" },
      { 0x10009, "exif:GPSStatus" },
      { 0x1000a, "exif:GPSMeasureMode" },
      { 0x1000b, "exif:GPSDop" },
      { 0x1000c, "exif:GPSSpeedRef" },
      { 0x1000d, "exif:GPSSpeed" },
      { 0x1000e, "exif:GPSTrackRef" },
      { 0x1000f, "exif:GPSTrack" },
      { 0x10010, "exif:GPSImgDirectionRef" },
      { 0x10011, "exif:GPSImgDirection" },
      { 0x10012, "exif:GPSMapDatum" },
      { 0x10013, "exif:GPSDestLatitudeRef" },
      { 0x10014, "exif:GPSDestLatitude" },
      { 0x10015, "exif:GPSDestLongitudeRef" },
      { 0x10016, "exif:GPSDestLongitude" },
      { 0x10017, "exif:GPSDestBearingRef" },
      { 0x10018, "exif:GPSDestBearing" },
      { 0x10019, "exif:GPSDestDistanceRef" },
      { 0x1001a, "exif:GPSDestDistance" },
      { 0x1001b, "exif:GPSProcessingMethod" },
      { 0x1001c, "exif:GPSAreaInformation" },
      { 0x1001d, "exif:GPSDateStamp" },
      { 0x1001e, "exif:GPSDifferential" },
      { 0x1001f, "exif:GPSHPositioningError" },
      { 0x00000, "" }
    };  /* https://cipa.jp/std/documents/download_e.html?DC-008-Translation-2019-E */

  const StringInfo
    *profile;

  const unsigned char
    *directory,
    *exif;

  DirectoryInfo
    directory_stack[MaxDirectoryStack] = { { 0, 0, 0 } };

  EndianType
    endian;

  EXIFWalkInfo
    walk;

  size_t
    entry,
    length,
    tag;

  SplayTreeInfo
    *exif_resources;

  ssize_t
    all,
    level,
    offset,
    tag_offset;

  const EXIFWalkTargets
    targets = { &level, &entry, directory_stack };

  /*
    If EXIF data exists, then try to parse the request for a tag.
  */
  profile=GetImageProfile(image,"exif");
  if (profile == (const StringInfo *) NULL)
    return;
  if ((property == (const char *) NULL) || (*property == '\0'))
    return;
  while (isspace((int) ((unsigned char) *property)) != 0)
    property++;
  if (strlen(property) <= 5)
    return;
  if (ParseEXIFTagRequest(EXIFTag,&property,&all,&tag) == MagickFalse)
    return;
  if (tag == (~0UL))
    return;
  length=GetStringInfoLength(profile);
  if (length < 6)
    return;
  exif=GetStringInfoDatum(profile);
  if (ReadEXIFHeader(&exif,&length,&endian) == MagickFalse)
    return;
  /*
    This the offset to the first IFD.
  */
  offset=(ssize_t) ReadPropertySignedLong(endian,exif+4);
  if ((offset < 0) || (size_t) offset >= length)
    return;
  /*
    Set the pointer to the first IFD and follow it were it leads.
  */
  directory=exif+offset;
  level=0;
  entry=0;
  tag_offset=0;
  exif_resources=NewSplayTree((int (*)(const void *,const void *)) NULL,
    (void *(*)(void *)) NULL,(void *(*)(void *)) NULL);
  do
  {
    /*
      If there is anything on the stack then pop it off.
    */
    if (level > 0)
      {
        level--;
        directory=directory_stack[level].directory;
        entry=directory_stack[level].entry;
        tag_offset=directory_stack[level].offset;
      }
    if ((directory < exif) || (directory > (exif+length-2)))
      break;
    walk.image=image;
    walk.property=property;
    walk.EXIFTag=EXIFTag;
    walk.all=all;
    walk.tag=tag;
    walk.endian=endian;
    walk.exif=exif;
    walk.length=length;
    walk.directory=directory;
    walk.tag_offset=tag_offset;
    walk.exif_resources=exif_resources;
    walk.exception=exception;
    GetEXIFDirectory(&walk,&targets);
  } while (level > 0);
  exif_resources=DestroySplayTree(exif_resources);
}

#if defined(MAGICKCORE_LCMS_DELEGATE)
#if !defined(LCMS_VERSION) || (LCMS_VERSION >= 2000)
typedef struct _ICCInfoProperty
{
  cmsInfoType
    type;

  const char
    *key;
} ICCInfoProperty;

static const ICCInfoProperty ICCInfoProperties[] =
{
  { cmsInfoDescription, "icc:description" },
  { cmsInfoManufacturer, "icc:manufacturer" },
  { cmsInfoModel, "icc:model" },
  { cmsInfoCopyright, "icc:copyright" }
};

static void SetICCInfoProperty(const Image *image,cmsHPROFILE icc_profile,
  const ICCInfoProperty *entry,ExceptionInfo *exception)
{
  StringInfo
    *info;

  unsigned int
    extent;

  info=AcquireStringInfo(0);
  extent=cmsGetProfileInfoASCII(icc_profile,entry->type,"en","US",NULL,0);
  if (extent != 0)
    {
      SetStringInfoLength(info,extent+1);
      extent=cmsGetProfileInfoASCII(icc_profile,entry->type,"en","US",
        (char *) GetStringInfoDatum(info),extent);
      if (extent != 0)
        (void) SetImageProperty((Image *) image,entry->key,
          (char *) GetStringInfoDatum(info),exception);
    }
  info=DestroyStringInfo(info);
}
#endif

static void GetICCProperty(const Image *image,ExceptionInfo *exception)
{

  const StringInfo
    *profile;

  cmsHPROFILE
    icc_profile;

  /*
    Return ICC profile property.
  */
  profile=GetImageProfile(image,"icc");
  if (profile == (StringInfo *) NULL)
    profile=GetImageProfile(image,"icm");
  if (profile == (StringInfo *) NULL)
    return;
  if (GetStringInfoLength(profile) < 128)
    return;  /* minimum ICC profile length */
  icc_profile=cmsOpenProfileFromMem(GetStringInfoDatum(profile),
    (cmsUInt32Number) GetStringInfoLength(profile));
  if (icc_profile != (cmsHPROFILE *) NULL)
    {
#if defined(LCMS_VERSION) && (LCMS_VERSION < 2000)
      const char
        *name;

      name=cmsTakeProductName(icc_profile);
      if (name != (const char *) NULL)
        (void) SetImageProperty((Image *) image,"icc:name",name,exception);
#else
      ssize_t
        i;

      for (i=0; i < (ssize_t) (sizeof(ICCInfoProperties)/
           sizeof(*ICCInfoProperties)); i++)
        SetICCInfoProperty(image,icc_profile,ICCInfoProperties+i,exception);
#endif
      (void) cmsCloseProfile(icc_profile);
    }
}
#endif

static MagickBooleanType SkipXMPValue(const char *value)
{
  if (value == (const char*) NULL)
    return(MagickTrue);
  while (*value != '\0')
  {
    if (isspace((int) ((unsigned char) *value)) == 0)
      return(MagickFalse);
    value++;
  }
  return(MagickTrue);
}

static inline MagickBooleanType IsNotNamespaceWildcard(
  const char *xmp_namespace,const size_t xmp_namespace_length)
{
  return(((xmp_namespace_length <= 2) ||
    (*(xmp_namespace+(xmp_namespace_length-2)) != ':') ||
    (*(xmp_namespace+(xmp_namespace_length-1)) != '*')) ? MagickTrue :
    MagickFalse);
}

static void AddXMPPropertyValue(const Image *image,XMLTreeInfo *node,
  const char *content)
{
  char
    *xmp_namespace;

  size_t
    xmp_namespace_length;

  xmp_namespace=ConstantString(GetXMLTreeTag(node));
  (void) SubstituteString(&xmp_namespace,"exif:","xmp:");
  xmp_namespace_length=strlen(xmp_namespace);
  if (IsNotNamespaceWildcard(xmp_namespace,xmp_namespace_length) !=
      MagickFalse)
    (void) AddValueToSplayTree((SplayTreeInfo *) image->properties,
     ConstantString(xmp_namespace),ConstantString(content));
  xmp_namespace=DestroyString(xmp_namespace);
}

static void AddXMPChildValues(const Image *image,XMLTreeInfo *node,
  XMLTreeInfo *child)
{
  const char
    *content;

  while (child != (XMLTreeInfo *) NULL)
  {
    content=GetXMLTreeContent(child);
    if (SkipXMPValue(content) == MagickFalse)
      AddXMPPropertyValue(image,node,content);
    child=GetXMLTreeSibling(child);
  }
}

static void AddXMPDescriptionValues(const Image *image,
  XMLTreeInfo *description)
{
  const char
    *content;

  XMLTreeInfo
    *child,
    *node;

  node=GetXMLTreeChild(description,(const char *) NULL);
  while (node != (XMLTreeInfo *) NULL)
  {
    child=GetXMLTreeChild(node,(const char *) NULL);
    content=GetXMLTreeContent(node);
    if ((child == (XMLTreeInfo *) NULL) &&
        (SkipXMPValue(content) == MagickFalse))
      AddXMPPropertyValue(image,node,content);
    AddXMPChildValues(image,node,child);
    node=GetXMLTreeSibling(node);
  }
}

static void AddXMPRDFValues(const Image *image,XMLTreeInfo *rdf)
{
  XMLTreeInfo
    *description;

  if (image->properties == (void *) NULL)
    ((Image *) image)->properties=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,RelinquishMagickMemory);
  description=GetXMLTreeChild(rdf,"rdf:Description");
  while (description != (XMLTreeInfo *) NULL)
  {
    AddXMPDescriptionValues(image,description);
    description=GetNextXMLTreeTag(description);
  }
}

static void GetXMPProperty(const Image *image,const char *property)
{
  char
    *xmp_profile;

  const StringInfo
    *profile;

  ExceptionInfo
    *exception;

  const char
    *p;

  XMLTreeInfo
    *rdf,
    *xmp;

  profile=GetImageProfile(image,"xmp");
  if (profile == (StringInfo *) NULL)
    return;
  if (GetStringInfoLength(profile) < 17)
    return;
  if ((property == (const char *) NULL) || (*property == '\0'))
    return;
  xmp_profile=StringInfoToString(profile);
  if (xmp_profile == (char *) NULL)
    return;
  for (p=xmp_profile; *p != '\0'; p++)
    if ((*p == '<') && (*(p+1) == 'x'))
      break;
  exception=AcquireExceptionInfo();
  xmp=NewXMLTree((char *) p,exception);
  xmp_profile=DestroyString(xmp_profile);
  exception=DestroyExceptionInfo(exception);
  if (xmp == (XMLTreeInfo *) NULL)
    return;
  rdf=GetXMLTreeChild(xmp,"rdf:RDF");
  if (rdf != (XMLTreeInfo *) NULL)
    AddXMPRDFValues(image,rdf);
  xmp=DestroyXMLTree(xmp);
}

static inline MagickBooleanType IsPointCorner(const PointInfo *last,
  const PointInfo *point)
{
  return(((last[1].x == last[2].x) && (last[1].y == last[2].y) &&
    (point[0].x == point[1].x) && (point[0].y == point[1].y)) ? MagickTrue :
    MagickFalse);
}

static void FormatPSClippathClose(char *message,const PointInfo *last,
  const PointInfo *first)
{
  if (IsPointCorner(last,first) != MagickFalse)
    (void) FormatLocaleString(message,MagickPathExtent,
      "  %g %g l z\n",first[1].x,first[1].y);
  else
    if ((last[1].x == last[2].x) && (last[1].y == last[2].y))
      (void) FormatLocaleString(message,MagickPathExtent,
        "  %g %g %g %g v z\n",first[0].x,first[0].y,
        first[1].x,first[1].y);
    else
      if ((first[0].x == first[1].x) && (first[0].y == first[1].y))
        (void) FormatLocaleString(message,MagickPathExtent,
          "  %g %g %g %g y z\n",last[2].x,last[2].y,
          first[1].x,first[1].y);
      else
        (void) FormatLocaleString(message,MagickPathExtent,
          "  %g %g %g %g %g %g c z\n",last[2].x,
          last[2].y,first[0].x,first[0].y,first[1].x,first[1].y);
}

static void FormatPSClippathKnot(char *message,PointInfo *last,
  const PointInfo *point)
{
  ssize_t
    i;

  /*
    Handle special cases when Bezier curves are used to describe
    corners and straight lines.
  */
  if (IsPointCorner(last,point) != MagickFalse)
    (void) FormatLocaleString(message,MagickPathExtent,
      "  %g %g l\n",point[1].x,point[1].y);
  else
    if ((last[1].x == last[2].x) && (last[1].y == last[2].y))
      (void) FormatLocaleString(message,MagickPathExtent,
        "  %g %g %g %g v\n",point[0].x,point[0].y,
        point[1].x,point[1].y);
    else
      if ((point[0].x == point[1].x) && (point[0].y == point[1].y))
        (void) FormatLocaleString(message,MagickPathExtent,
          "  %g %g %g %g y\n",last[2].x,last[2].y,
          point[1].x,point[1].y);
      else
        (void) FormatLocaleString(message,MagickPathExtent,
          "  %g %g %g %g %g %g c\n",last[2].x,
          last[2].y,point[0].x,point[0].y,point[1].x,point[1].y);
  for (i=0; i < 3; i++)
    last[i]=point[i];
}

static void StartPSClippathSubpath(char *message,PointInfo *first,
  PointInfo *last,const PointInfo *point)
{
  ssize_t
    i;

  (void) FormatLocaleString(message,MagickPathExtent,"  %g %g m\n",
    point[1].x,point[1].y);
  for (i=0; i < 3; i++)
  {
    first[i]=point[i];
    last[i]=point[i];
  }
}

static char *TracePSClippath(const unsigned char *blob,size_t length)
{
  char
    *path,
    *message;

  MagickBooleanType
    in_subpath;

  PointInfo
    first[3],
    last[3],
    point[3];

  ssize_t
    i,
    x;

  ssize_t
    knot_count,
    selector,
    y;

  path=AcquireString((char *) NULL);
  if (path == (char *) NULL)
    return((char *) NULL);
  message=AcquireString((char *) NULL);
  (void) FormatLocaleString(message,MagickPathExtent,"/ClipImage\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,"{\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /c {curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /l {lineto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /m {moveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /v {currentpoint 6 2 roll curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /y {2 copy curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,
    "  /z {closepath} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,"  newpath\n");
  (void) ConcatenateString(&path,message);
  /*
    The clipping path format is defined in "Adobe Photoshop File Formats
    Specification" version 6.0 downloadable from adobe.com.
  */
  (void) memset(point,0,sizeof(point));
  (void) memset(first,0,sizeof(first));
  (void) memset(last,0,sizeof(last));
  knot_count=0;
  in_subpath=MagickFalse;
  while (length > 0)
  {
    selector=(ssize_t) ReadPropertyMSBShort(&blob,&length);
    switch (selector)
    {
      case 0:
      case 3:
      {
        if (knot_count != 0)
          {
            blob+=24;
            length=(size_t) ((ssize_t) length-MagickMin(24,(ssize_t) length));
            break;
          }
        /*
          Expected subpath length record.
        */
        knot_count=(ssize_t) ReadPropertyMSBShort(&blob,&length);
        blob+=22;
        length=(size_t) ((ssize_t) length-MagickMin(22,(ssize_t) length));
        break;
      }
      case 1:
      case 2:
      case 4:
      case 5:
      {
        if (knot_count == 0)
          {
            /*
              Unexpected subpath knot
            */
            blob+=24;
            length=(size_t) ((ssize_t) length-MagickMin(24,(ssize_t) length));
            break;
          }
        /*
          Add sub-path knot
        */
        for (i=0; i < 3; i++)
        {
          y=(ssize_t) ReadPropertyMSBLong(&blob,&length);
          x=(ssize_t) ReadPropertyMSBLong(&blob,&length);
          point[i].x=(double) x/4096.0/4096.0;
          point[i].y=1.0-(double) y/4096.0/4096.0;
        }
        if (in_subpath == MagickFalse)
          StartPSClippathSubpath(message,first,last,point);
        else
          FormatPSClippathKnot(message,last,point);
        (void) ConcatenateString(&path,message);
        in_subpath=MagickTrue;
        knot_count--;
        /*
          Close the subpath if there are no more knots.
        */
        if (knot_count == 0)
          {
            /*
              Same special handling as above except we compare to the
              first point in the path and close the path.
            */
            FormatPSClippathClose(message,last,first);
            (void) ConcatenateString(&path,message);
            in_subpath=MagickFalse;
          }
        break;
      }
      case 6:
      case 7:
      case 8:
      default:
      {
        blob+=24;
        length=(size_t) ((ssize_t) length-MagickMin(24,(ssize_t) length));
        break;
      }
    }
  }
  /*
    Returns an empty PS path if the path has no knots.
  */
  (void) FormatLocaleString(message,MagickPathExtent,"  eoclip\n");
  (void) ConcatenateString(&path,message);
  (void) FormatLocaleString(message,MagickPathExtent,"} bind def");
  (void) ConcatenateString(&path,message);
  message=DestroyString(message);
  return(path);
}

static inline MagickBooleanType IsBezierCorner(const PointInfo *last,
  const PointInfo *point)
{
  return((((last+1)->x == (last+2)->x) && ((last+1)->y == (last+2)->y) &&
    (point->x == (point+1)->x) && (point->y == (point+1)->y)) ? MagickTrue :
    MagickFalse);
}

static inline void TraceBezierCurve(char *message,PointInfo *last,
  PointInfo *point)
{
  /*
    Handle special cases when Bezier curves are used to describe
    corners and straight lines.
  */
  if (IsBezierCorner(last,point) != MagickFalse)
    (void) FormatLocaleString(message,MagickPathExtent,
      "L %g %g\n",point[1].x,point[1].y);
  else
    (void) FormatLocaleString(message,MagickPathExtent,"C %g %g %g %g %g %g\n",
      (last+2)->x,(last+2)->y,point->x,point->y,(point+1)->x,(point+1)->y);
}

static void CopyClippathKnot(PointInfo *first,PointInfo *last,
  const PointInfo *point)
{
  ssize_t
    i;

  for (i=0; i < 3; i++)
  {
    first[i]=point[i];
    last[i]=point[i];
  }
}

static void TraceSVGClippathKnot(char *message,PointInfo *last,
  PointInfo *point)
{
  ssize_t
    i;

  TraceBezierCurve(message,last,point);
  for (i=0; i < 3; i++)
    last[i]=point[i];
}

static char *TraceSVGClippath(const unsigned char *blob,size_t length,
  const size_t columns,const size_t rows)
{
  char
    *path,
    *message;

  MagickBooleanType
    in_subpath;

  PointInfo
    first[3],
    last[3],
    point[3];

  ssize_t
    i;

  ssize_t
    knot_count,
    selector,
    x,
    y;

  path=AcquireString((char *) NULL);
  if (path == (char *) NULL)
    return((char *) NULL);
  message=AcquireString((char *) NULL);
  (void) FormatLocaleString(message,MagickPathExtent,(
    "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
    "<svg xmlns=\"http://www.w3.org/2000/svg\""
    " width=\"%.17g\" height=\"%.17g\">\n"
    "<g>\n"
    "<path fill-rule=\"evenodd\" style=\"fill:#000000;stroke:#000000;"
    "stroke-width:0;shape-rendering:crispEdges\" d=\"\n"),(double) columns,
    (double) rows);
  (void) ConcatenateString(&path,message);
  (void) memset(point,0,sizeof(point));
  (void) memset(first,0,sizeof(first));
  (void) memset(last,0,sizeof(last));
  knot_count=0;
  in_subpath=MagickFalse;
  while (length != 0)
  {
    selector=(ssize_t) ReadPropertyMSBShort(&blob,&length);
    switch (selector)
    {
      case 0:
      case 3:
      {
        if (knot_count != 0)
          {
            blob+=24;
            length=(size_t) ((ssize_t) length-MagickMin(24,(ssize_t) length));
            break;
          }
        /*
          Expected subpath length record.
        */
        knot_count=(ssize_t) ReadPropertyMSBShort(&blob,&length);
        blob+=22;
        length=(size_t) ((ssize_t) length-MagickMin(22,(ssize_t) length));
        break;
      }
      case 1:
      case 2:
      case 4:
      case 5:
      {
        if (knot_count == 0)
          {
            /*
              Unexpected subpath knot.
            */
            blob+=24;
            length=(size_t) ((ssize_t) length-MagickMin(24,(ssize_t) length));
            break;
          }
        /*
          Add sub-path knot
        */
        for (i=0; i < 3; i++)
        {
          y=(ssize_t) ReadPropertyMSBLong(&blob,&length);
          x=(ssize_t) ReadPropertyMSBLong(&blob,&length);
          point[i].x=(double) x*columns/4096.0/4096.0;
          point[i].y=(double) y*rows/4096.0/4096.0;
        }
        if (in_subpath == MagickFalse)
          {
            (void) FormatLocaleString(message,MagickPathExtent,"M %g %g\n",
              point[1].x,point[1].y);
            CopyClippathKnot(first,last,point);
          }
        else
          TraceSVGClippathKnot(message,last,point);
        (void) ConcatenateString(&path,message);
        in_subpath=MagickTrue;
        knot_count--;
        /*
          Close the subpath if there are no more knots.
        */
        if (knot_count == 0)
          {
            TraceBezierCurve(message,last,first);
            (void) ConcatenateString(&path,message);
            in_subpath=MagickFalse;
          }
        break;
      }
      case 6:
      case 7:
      case 8:
      default:
      {
        blob+=24;
        length=(size_t) ((ssize_t) length-MagickMin(24,(ssize_t) length));
        break;
      }
    }
  }
  /*
    Return an empty SVG image if the path does not have knots.
  */
  (void) ConcatenateString(&path,"\"/>\n</g>\n</svg>\n");
  message=DestroyString(message);
  return(path);
}

static void GetProfileProperty(const Image *image,const char *property,
  ExceptionInfo *exception)
{
  switch (*property)
  {
    case '8':
    {
      if (LocaleNCompare("8bim:",property,5) == 0)
        {
          Get8BIMProperty(image,property,exception);
          break;
        }
      break;
    }
    case 'E':
    case 'e':
    {
      if (LocaleNCompare("exif:",property,5) == 0)
        {
          GetEXIFProperty(image,property,exception);
          break;
        }
      break;
    }
    case 'I':
    case 'i':
    {
      if ((LocaleNCompare("icc:",property,4) == 0) ||
          (LocaleNCompare("icm:",property,4) == 0))
        {
#if defined(MAGICKCORE_LCMS_DELEGATE)
          GetICCProperty(image,exception);
#endif
          break;
        }
      if (LocaleNCompare("iptc:",property,5) == 0)
        {
          GetIPTCProperty(image,property,exception);
          break;
        }
      break;
    }
    case 'X':
    case 'x':
    {
      if (LocaleNCompare("xmp:",property,4) == 0)
        {
          GetXMPProperty(image,property);
          break;
        }
      break;
    }
    default:
      break;
  }
}

static inline MagickBooleanType IsWildcardProperty(const char *property,
  const size_t property_length)
{
  return(((property_length > 2) && (*(property+(property_length-2)) == ':') &&
    (*(property+(property_length-1)) == '*')) ? MagickTrue : MagickFalse);
}

MagickExport const char *GetImageProperty(const Image *image,
  const char *property,ExceptionInfo *exception)
{
  MagickBooleanType
    read_from_properties;

  const char
    *p;

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
  if (read_from_properties != MagickFalse)
    {
      p=(const char *) NULL;
      if (image->properties != (void *) NULL)
        {
          p=(const char *) GetValueFromSplayTree((SplayTreeInfo *)
            image->properties,property);
          if (p != (const char *) NULL)
            return(p);
        }
      if (strchr(property,':') == (char *) NULL)
        return(p);
    }
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
static const char *CloneMagickPropertyValue(ImageInfo *image_info,
  Image *image,const char *value)
{
  if (*value != '\0')
    {
      /*
        Create a cloned copy of result.
      */
      if (image != (Image *) NULL)
        {
          (void) SetImageArtifact(image,"magick-property",value);
          return(GetImageArtifact(image,"magick-property"));
        }
      else
        {
          (void) SetImageOption(image_info,"magick-property",value);
          return(GetImageOption(image_info,"magick-property"));
        }
    }
  return((char *) NULL);
}

static const char *GetMagickPropertyLetter(ImageInfo *image_info,
  Image *image,const char letter,ExceptionInfo *exception)
{
#define WarnNoImageReturn(format,arg) \
  if (image == (Image *) NULL ) { \
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning, \
      "NoImageForProperty",format,arg); \
    return((const char *) NULL); \
  }
#define WarnNoImageInfoReturn(format,arg) \
  if (image_info == (ImageInfo *) NULL ) { \
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning, \
      "NoImageInfoForProperty",format,arg); \
    return((const char *) NULL); \
  }

  char
    value[MagickPathExtent];  /* formatted string to store as an artifact */

  const char
    *string;     /* return a string already stored somewhere */

  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
    if ((image_info != (ImageInfo *) NULL) &&
        (IsEventLogging() != MagickFalse))
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no-images");
  *value='\0';           /* formatted string */
  string=(char *) NULL;  /* constant string reference */
  /*
    Get properties that are directly defined by images.
  */
  switch (letter)
  {
    case 'b':  /* image size read in - in bytes */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatMagickSize(image->extent,MagickFalse,"B",MagickPathExtent,
        value);
      if (image->extent == 0)
        (void) FormatMagickSize(GetBlobSize(image),MagickFalse,"B",
          MagickPathExtent,value);
      break;
    }
    case 'c':  /* image comment property - empty string by default */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=GetImageProperty(image,"comment",exception);
      if ( string == (const char *) NULL )
        string="";
      break;
    }
    case 'd':  /* Directory component of filename */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      GetPathComponent(image->magick_filename,HeadPath,value);
      if (*value == '\0')
        string="";
      break;
    }
    case 'e': /* Filename extension (suffix) of image file */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      GetPathComponent(image->magick_filename,ExtensionPath,value);
      if (*value == '\0')
        string="";
      break;
    }
    case 'f': /* Filename without directory component */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      GetPathComponent(image->magick_filename,TailPath,value);
      if (*value == '\0')
        string="";
      break;
    }
    case 'g': /* Image geometry, canvas and offset  %Wx%H+%X+%Y */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,
        "%.17gx%.17g%+.20g%+.20g",(double) image->page.width,(double)
        image->page.height,(double) image->page.x,(double) image->page.y);
      break;
    }
    case 'h': /* Image height (current) */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (image->rows != 0 ? image->rows : image->magick_rows));
      break;
    }
    case 'i': /* Filename last used for an image (read or write) */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=image->filename;
      break;
    }
    case 'k': /* Number of unique colors  */
    {
      /*
        FUTURE: ensure this does not generate the formatted comment!
      */
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        GetNumberColors(image,(FILE *) NULL,exception));
      break;
    }
    case 'l': /* Image label property - empty string by default */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=GetImageProperty(image,"label",exception);
      if (string == (const char *) NULL)
        string="";
      break;
    }
    case 'm': /* Image format (file magick) */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=image->magick;
      break;
    }
    case 'n': /* Number of images in the list.  */
    {
      if ( image != (Image *) NULL )
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
          GetImageListLength(image));
      else
        string="0";    /* no images or scenes */
      break;
    }
    case 'o': /* Output Filename - for delegate use only */
    {
      WarnNoImageInfoReturn("\"%%%c\"",letter);
      string=image_info->filename;
      break;
    }
    case 'p': /* Image index in current image list */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        GetImageIndexInList(image));
      break;
    }
    case 'q': /* Quantum depth of image in memory */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        MAGICKCORE_QUANTUM_DEPTH);
      break;
    }
    case 'r': /* Image storage class, colorspace, and alpha enabled.  */
    {
      ColorspaceType
        colorspace;

      WarnNoImageReturn("\"%%%c\"",letter);
      colorspace=image->colorspace;
      (void) FormatLocaleString(value,MagickPathExtent,"%s %s %s",
        CommandOptionToMnemonic(MagickClassOptions,(ssize_t)
        image->storage_class),CommandOptionToMnemonic(MagickColorspaceOptions,
        (ssize_t) colorspace),image->alpha_trait != UndefinedPixelTrait ?
        "Alpha" : "");
      break;
    }
    case 's': /* Image scene number */
    {
#if 0  /* this seems non-sensical -- simplifying */
      if (image_info->number_scenes != 0)
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
          image_info->scene);
      else if (image != (Image *) NULL)
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
          image->scene);
      else
          string="0";
#else
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->scene);
#endif
      break;
    }
    case 't': /* Base filename without directory or extension */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      GetPathComponent(image->magick_filename,BasePath,value);
      if (*value == '\0')
        string="";
      break;
    }
    case 'u': /* Unique filename */
    {
      WarnNoImageInfoReturn("\"%%%c\"",letter);
      string=image_info->unique;
      break;
    }
    case 'w': /* Image width (current) */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (image->columns != 0 ? image->columns : image->magick_columns));
      break;
    }
    case 'x': /* Image horizontal resolution (with units) */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",
        fabs(image->resolution.x) > MagickEpsilon ? image->resolution.x :
        image->units == PixelsPerCentimeterResolution ? DefaultResolution/2.54 :
        DefaultResolution);
      break;
    }
    case 'y': /* Image vertical resolution (with units) */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",
        fabs(image->resolution.y) > MagickEpsilon ? image->resolution.y :
        image->units == PixelsPerCentimeterResolution ? DefaultResolution/2.54 :
        DefaultResolution);
      break;
    }
    case 'z': /* Image depth as read in */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->depth);
      break;
    }
    case 'A': /* Image alpha channel  */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=CommandOptionToMnemonic(MagickPixelTraitOptions,(ssize_t)
        image->alpha_trait);
      break;
    }
    case 'B':  /* image size read in - in bytes */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->extent);
      if (image->extent == 0)
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
          GetBlobSize(image));
      break;
    }
    case 'C': /* Image compression method.  */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=CommandOptionToMnemonic(MagickCompressOptions,(ssize_t)
        image->compression);
      break;
    }
    case 'D': /* Image dispose method.  */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=CommandOptionToMnemonic(MagickDisposeOptions,(ssize_t)
        image->dispose);
      break;
    }
    case 'G': /* Image size as geometry = "%wx%h" */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17gx%.17g",(double)
        image->magick_columns,(double) image->magick_rows);
      break;
    }
    case 'H': /* layer canvas height */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->page.height);
      break;
    }
    case 'M': /* Magick filename - filename given incl. coder & read mods */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=image->magick_filename;
      break;
    }
    case 'N': /* Number of images in the list.  */
    {
      if ((image != (Image *) NULL) && (image->next == (Image *) NULL))
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g\n",(double)
          GetImageListLength(image));
      else
        string="";
      break;
    }
    case 'O': /* layer canvas offset with sign = "+%X+%Y" */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%+ld%+ld",(long)
        image->page.x,(long) image->page.y);
      break;
    }
    case 'P': /* layer canvas page size = "%Wx%H" */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17gx%.17g",(double)
        image->page.width,(double) image->page.height);
      break;
    }
    case 'Q': /* image compression quality */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (image->quality == 0 ? 92 : image->quality));
      break;
    }
    case 'S': /* Number of scenes in image list.  */
    {
      WarnNoImageInfoReturn("\"%%%c\"",letter);
#if 0 /* What is this number? -- it makes no sense - simplifying */
      if (image_info->number_scenes == 0)
         string="2147483647";
      else if ( image != (Image *) NULL )
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
                image_info->scene+image_info->number_scenes);
      else
        string="0";
#else
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (image_info->number_scenes == 0 ? 2147483647 :
         image_info->number_scenes));
#endif
      break;
    }
    case 'T': /* image time delay for animations */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->delay);
      break;
    }
    case 'U': /* Image resolution units. */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      string=CommandOptionToMnemonic(MagickResolutionOptions,(ssize_t)
        image->units);
      break;
    }
    case 'W': /* layer canvas width */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->page.width);
      break;
    }
    case 'X': /* layer canvas X offset */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%+.20g",(double)
        image->page.x);
      break;
    }
    case 'Y': /* layer canvas Y offset */
    {
      WarnNoImageReturn("\"%%%c\"",letter);
      (void) FormatLocaleString(value,MagickPathExtent,"%+.20g",(double)
        image->page.y);
      break;
    }
    case '%': /* percent escaped */
    {
      string="%";
      break;
    }
    case '@': /* Trim bounding box, without actually Trimming! */
    {
      RectangleInfo
        page;

      WarnNoImageReturn("\"%%%c\"",letter);
      page=GetImageBoundingBox(image,exception);
      (void) FormatLocaleString(value,MagickPathExtent,
        "%.17gx%.17g%+.20g%+.20g",(double) page.width,(double) page.height,
        (double) page.x,(double)page.y);
      break;
    }
    case '#':
    {
      /*
        Image signature.
      */
      WarnNoImageReturn("\"%%%c\"",letter);
      if ((image->columns != 0) && (image->rows != 0))
        (void) SignatureImage(image,exception);
      string=GetImageProperty(image,"signature",exception);
      break;
    }
  }
  if (string != (char *) NULL)
    return(string);
  return(CloneMagickPropertyValue(image_info,image,value));
}

static const char *GetPathComponentProperty(const Image *image,
  const PathType type,char *value,const char *string)
{
  GetPathComponent(image->magick_filename,type,value);
  if (*value == '\0')
    string="";
  return(string);
}

static void GetProfilesProperty(const Image *image,char *value)
{
  const char
    *name;

  ResetImageProfileIterator(image);
  name=GetNextImageProfile(image);
  if (name != (char *) NULL)
    {
      (void) CopyMagickString(value,name,MagickPathExtent);
      name=GetNextImageProfile(image);
      while (name != (char *) NULL)
      {
        ConcatenateMagickString(value,",",MagickPathExtent);
        ConcatenateMagickString(value,name,MagickPathExtent);
        name=GetNextImageProfile(image);
      }
    }
}

static MagickBooleanType IsTopHullPoint(const PointInfo *point,
  const PointInfo *extreme)
{
  if (point->y < extreme->y)
    return(MagickTrue);
  if (point->y != extreme->y)
    return(MagickFalse);
  return((MagickBooleanType) (point->x < extreme->x));
}

static MagickBooleanType IsRightHullPoint(const PointInfo *point,
  const PointInfo *extreme)
{
  if (point->x > extreme->x)
    return(MagickTrue);
  if (point->x != extreme->x)
    return(MagickFalse);
  return((MagickBooleanType) (point->y < extreme->y));
}

static MagickBooleanType IsBottomHullPoint(const PointInfo *point,
  const PointInfo *extreme)
{
  if (point->y > extreme->y)
    return(MagickTrue);
  if (point->y != extreme->y)
    return(MagickFalse);
  return((MagickBooleanType) (point->x > extreme->x));
}

static MagickBooleanType IsLeftHullPoint(const PointInfo *point,
  const PointInfo *extreme)
{
  if (point->x < extreme->x)
    return(MagickTrue);
  if (point->x != extreme->x)
    return(MagickFalse);
  return((MagickBooleanType) (point->y > extreme->y));
}

typedef MagickBooleanType
  (*HullPointMethod)(const PointInfo *,const PointInfo *);

static PointInfo GetHullExtremePoint(const PointInfo *convex_hull,
  const size_t number_points,const HullPointMethod method)
{
  PointInfo
    extreme;

  ssize_t
    n;

  extreme=convex_hull[0];
  for (n=0; n < (ssize_t) number_points; n++)
    if (method(convex_hull+n,&extreme) != MagickFalse)
      extreme=convex_hull[n];
  return(extreme);
}

static const char *GetConvexHullExtremePoints(Image *image,char *value,
  const char *string,ExceptionInfo *exception)
{
  char
    *points;

  PointInfo
    extreme,
    *convex_hull;

  size_t
    number_points;

  convex_hull=GetImageConvexHull(image,&number_points,exception);
  if (convex_hull == (PointInfo *) NULL)
    return(string);
  points=AcquireString("");
  extreme=GetHullExtremePoint(convex_hull,number_points,  /* top */
    IsTopHullPoint);
  (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
    extreme.x,extreme.y);
  (void) ConcatenateString(&points,value);
  extreme=GetHullExtremePoint(convex_hull,number_points,  /* right */
    IsRightHullPoint);
  (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
    extreme.x,extreme.y);
  (void) ConcatenateString(&points,value);
  extreme=GetHullExtremePoint(convex_hull,number_points,  /* bottom */
    IsBottomHullPoint);
  (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
    extreme.x,extreme.y);
  (void) ConcatenateString(&points,value);
  extreme=GetHullExtremePoint(convex_hull,number_points,  /* left */
    IsLeftHullPoint);
  (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
    extreme.x,extreme.y);
  (void) ConcatenateString(&points,value);
  convex_hull=(PointInfo *) RelinquishMagickMemory(convex_hull);
  (void) SetImageProperty(image,"convex-hull:extreme-points",points,
    exception);
  points=DestroyString(points);
  string=GetImageProperty(image,"convex-hull:extreme-points",exception);
  return(string);
}

typedef enum
{
  ConvexHullPointList,
  MinimumBoundingBoxPointList
} PointListSide;

typedef struct _PointListInputs
{
  Image
    *image;

  char
    *value;

  const char
    *string,
    *property;

  ExceptionInfo
    *exception;
} PointListInputs;

static inline const char *GetPointListProperty(const PointListInputs *inputs,
  const PointListSide side)
{
  char
    *const value = inputs->value;

  const char
    *const property = inputs->property,
    *string = inputs->string;

  ExceptionInfo
    *const exception = inputs->exception;

  Image
    *const image = inputs->image;

  char
    *points;

  PointInfo
    *point_list;

  ssize_t
    n;

  size_t
    number_points;

  point_list=((side == ConvexHullPointList) ? GetImageConvexHull(image,
    &number_points,exception) : GetImageMinimumBoundingBox(image,
    &number_points,exception));
  if (point_list == (PointInfo *) NULL)
    return(string);
  points=AcquireString("");
  for (n=0; n < (ssize_t) number_points; n++)
  {
    (void) FormatLocaleString(value,MagickPathExtent,"%g,%g ",
      point_list[n].x,point_list[n].y);
    (void) ConcatenateString(&points,value);
  }
  point_list=(PointInfo *) RelinquishMagickMemory(point_list);
  (void) SetImageProperty(image,((side == ConvexHullPointList) ?
    "convex-hull" : "minimum-bounding-box"),points,exception);
  points=DestroyString(points);
  string=GetImageProperty(image,((side == ConvexHullPointList) ?
    "convex-hull" : property),exception);
  return(string);
}

static void FormatPaperSizeProperty(const char *property,char *value)
{
  char
    *papersize;

  *value='\0';
  papersize=GetPageGeometry(property+10);
  if (papersize != (const char *) NULL)
    {
      RectangleInfo
        page = { 0, 0, 0, 0 };

      (void) ParseAbsoluteGeometry(papersize,&page);
      (void) FormatLocaleString(value,MagickPathExtent,"%.17gx%.17g",
        (double) page.width,(double) page.height);
      papersize=DestroyString(papersize);
    }
}

MagickExport const char *GetMagickProperty(ImageInfo *image_info,
  Image *image,const char *property,ExceptionInfo *exception)
{
  char
    value[MagickPathExtent];

  const char
    *string;

  assert(property != (const char *) NULL);
  assert(property[0] != '\0');
  assert(image != (Image *) NULL || image_info != (ImageInfo *) NULL );
  if (property[1] == '\0')  /* single letter property request */
    return(GetMagickPropertyLetter(image_info,image,*property,exception));
  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
    if ((image_info != (ImageInfo *) NULL) &&
        (IsEventLogging() != MagickFalse))
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no-images");
  *value='\0';           /* formated string */
  string=(char *) NULL;  /* constant string reference */
  switch (*property)
  {
    case 'b':
    {
      if (LocaleCompare("basename",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=GetPathComponentProperty(image,BasePath,value,string);
          break;
        }
      if (LocaleCompare("bit-depth",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
            GetImageDepth(image,exception));
          break;
        }
      if (LocaleCompare("bounding-box",property) == 0)
        {
          RectangleInfo
            geometry;

          WarnNoImageReturn("\"%%[%s]\"",property);
          geometry=GetImageBoundingBox(image,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%g,%g %g,%g",
            (double) geometry.x,(double) geometry.y,
            (double) geometry.x+geometry.width,
            (double) geometry.y+geometry.height);
          break;
        }
      break;
    }
    case 'c':
    {
      if (LocaleCompare("channels",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%s%s %g.%g",
            CommandOptionToMnemonic(MagickColorspaceOptions,(ssize_t)
            image->colorspace),image->alpha_trait != UndefinedPixelTrait ?
            "a" : " ",(double) image->number_channels,(double)
            image->number_meta_channels);
          LocaleLower(value);
          break;
        }
      if (LocaleCompare("colors",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          image->colors=GetNumberColors(image,(FILE *) NULL,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
            image->colors);
          break;
        }
      if (LocaleCompare("colorspace",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=CommandOptionToMnemonic(MagickColorspaceOptions,(ssize_t)
            image->colorspace);
          break;
        }
      if (LocaleCompare("compose",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=CommandOptionToMnemonic(MagickComposeOptions,(ssize_t)
            image->compose);
          break;
        }
      if (LocaleCompare("compression",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=CommandOptionToMnemonic(MagickCompressOptions,(ssize_t)
            image->compression);
          break;
        }
      if (LocaleCompare("convex-hull",property) == 0)
        {
          const PointListInputs
            inputs = { image, value, string, property, exception };

          WarnNoImageReturn("\"%%[%s]\"",property);
          string=GetPointListProperty(&inputs,ConvexHullPointList);
          break;
        }
      if (LocaleCompare("convex-hull:extreme-points",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=GetConvexHullExtremePoints(image,value,string,exception);
          break;
        }
      if (LocaleCompare("copyright",property) == 0)
        {
          (void) CopyMagickString(value,GetMagickCopyright(),MagickPathExtent);
          break;
        }
      break;
    }
    case 'd':
    {
      if (LocaleCompare("depth",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
            image->depth);
          break;
        }
      if (LocaleCompare("directory",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=GetPathComponentProperty(image,HeadPath,value,string);
          break;
        }
      break;
    }
    case 'e':
    {
      if (LocaleCompare("entropy",property) == 0)
        {
          double
            entropy;

          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) GetImageEntropy(image,&entropy,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),entropy);
          break;
        }
      if (LocaleCompare("extension",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=GetPathComponentProperty(image,ExtensionPath,value,string);
          break;
        }
      break;
    }
    case 'g':
    {
      if (LocaleCompare("gamma",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),image->gamma);
          break;
        }
      break;
    }
    case 'h':
    {
      if (LocaleCompare("height",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.17g",
            image->magick_rows != 0 ? (double) image->magick_rows : 256.0);
          break;
        }
      break;
    }
    case 'i':
    {
      if (LocaleCompare("input",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=image->filename;
          break;
        }
      if (LocaleCompare("interlace",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=CommandOptionToMnemonic(MagickInterlaceOptions,(ssize_t)
            image->interlace);
          break;
        }
      break;
    }
    case 'k':
    {
      if (LocaleCompare("kurtosis",property) == 0)
        {
          double
            kurtosis,
            skewness;

          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) GetImageKurtosis(image,&kurtosis,&skewness,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),kurtosis);
          break;
        }
      break;
    }
    case 'm':
    {
      if (LocaleCompare("magick",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=image->magick;
          break;
        }
      if ((LocaleCompare("maxima",property) == 0) ||
          (LocaleCompare("max",property) == 0))
        {
          double
            maximum,
            minimum;

          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) GetImageRange(image,&minimum,&maximum,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),maximum);
          break;
        }
      if (LocaleCompare("mean",property) == 0)
        {
          double
            mean,
            standard_deviation;

          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) GetImageMean(image,&mean,&standard_deviation,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),mean);
          break;
        }
      if (LocaleCompare("median",property) == 0)
        {
          double
            median;

          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) GetImageMedian(image,&median,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),median);
          break;
        }
      if (LocaleCompare("mime:type",property) == 0)
        {
          const MagickInfo
            *magick_info;

          magick_info=GetMagickInfo(image->magick,exception);
          if ((magick_info != (const MagickInfo *) NULL) &&
              (GetMagickMimeType(magick_info) != (const char *) NULL))
            (void) CopyMagickString(value,GetMagickMimeType(magick_info),
              MagickPathExtent);
          break;
        }
      if ((LocaleCompare("minima",property) == 0) ||
          (LocaleCompare("min",property) == 0))
        {
          double
            maximum,
            minimum;

          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) GetImageRange(image,&minimum,&maximum,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),minimum);
          break;
        }
      if (LocaleNCompare("minimum-bounding-box",property,20) == 0)
        {
          const PointListInputs
            inputs = { image, value, string, property, exception };

          WarnNoImageReturn("\"%%[%s]\"",property);
          string=GetPointListProperty(&inputs,MinimumBoundingBoxPointList);
          break;
        }
      break;
    }
    case 'o':
    {
      if (LocaleCompare("opaque",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=CommandOptionToMnemonic(MagickBooleanOptions,(ssize_t)
            IsImageOpaque(image,exception));
          break;
        }
      if (LocaleCompare("orientation",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=CommandOptionToMnemonic(MagickOrientationOptions,(ssize_t)
            image->orientation);
          break;
        }
      if (LocaleCompare("output",property) == 0)
        {
          WarnNoImageInfoReturn("\"%%[%s]\"",property);
          (void) CopyMagickString(value,image_info->filename,MagickPathExtent);
          break;
        }
      break;
    }
    case 'p':
    {
      if (LocaleCompare("page",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.17gx%.17g",
            (double) image->page.width,(double) image->page.height);
          break;
        }
      if (LocaleNCompare("papersize:",property,10) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          FormatPaperSizeProperty(property,value);
          break;
        }
#if defined(MAGICKCORE_LCMS_DELEGATE)
      if (LocaleCompare("profile:icc",property) == 0 ||
          LocaleCompare("profile:icm",property) == 0)
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
      if (LocaleCompare("printsize.x",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),(double) MagickSafeReciprocal(
              image->resolution.x)*image->columns);
          break;
        }
      if (LocaleCompare("printsize.y",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),(double) MagickSafeReciprocal(
              image->resolution.y)*image->rows);
          break;
        }
      if (LocaleCompare("profiles",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          GetProfilesProperty(image,value);
          break;
        }
      break;
    }
    case 'q':
    {
      if (LocaleCompare("quality",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
            image->quality);
          break;
        }
      break;
    }
    case 'r':
    {
      if (LocaleCompare("resolution.x",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%g",
            image->resolution.x);
          break;
        }
      if (LocaleCompare("resolution.y",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%g",
            image->resolution.y);
          break;
        }
      break;
    }
    case 's':
    {
      if (LocaleCompare("scene",property) == 0)
        {
          WarnNoImageInfoReturn("\"%%[%s]\"",property);
          if (image_info->number_scenes != 0)
            (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
              image_info->scene);
          else {
            WarnNoImageReturn("\"%%[%s]\"",property);
            (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
              image->scene);
          }
          break;
        }
      if (LocaleCompare("scenes",property) == 0)
        {
          /* FUTURE: equivalent to %n? */
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
            GetImageListLength(image));
          break;
        }
      if (LocaleCompare("size",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatMagickSize(GetBlobSize(image),MagickFalse,"B",
            MagickPathExtent,value);
          break;
        }
      if (LocaleCompare("skewness",property) == 0)
        {
          double
            kurtosis,
            skewness;

          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) GetImageKurtosis(image,&kurtosis,&skewness,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),skewness);
          break;
        }
      if (LocaleCompare("standard-deviation",property) == 0)
        {
          double
            mean,
            standard_deviation;

          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) GetImageMean(image,&mean,&standard_deviation,exception);
          (void) FormatLocaleString(value,MagickPathExtent,"%.*g",
            GetMagickPrecision(),standard_deviation);
          break;
        }
      break;
    }
    case 't':
    {
      if (LocaleCompare("type",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=CommandOptionToMnemonic(MagickTypeOptions,(ssize_t)
            IdentifyImageType(image,exception));
          break;
        }
       break;
    }
    case 'u':
    {
      if (LocaleCompare("unique",property) == 0)
        {
          WarnNoImageInfoReturn("\"%%[%s]\"",property);
          string=image_info->unique;
          break;
        }
      if (LocaleCompare("units",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          string=CommandOptionToMnemonic(MagickResolutionOptions,(ssize_t)
            image->units);
          break;
        }
      break;
    }
    case 'v':
    {
      if (LocaleCompare("version",property) == 0)
        {
          string=GetMagickVersion((size_t *) NULL);
          break;
        }
      break;
    }
    case 'w':
    {
      if (LocaleCompare("width",property) == 0)
        {
          WarnNoImageReturn("\"%%[%s]\"",property);
          (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
            (image->magick_columns != 0 ? image->magick_columns : 256));
          break;
        }
      break;
    }
  }
  if (string != (char *) NULL)
    return(string);
  return(CloneMagickPropertyValue(image_info,image,value));
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
static inline MagickBooleanType IsNonEmptyImage(const Image *image)
{
  return(((image != (Image *) NULL) && (image->columns != 0) &&
    (image->rows != 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEscapeEnd(const char *p)
{
  return(((*p == '\0') || (*p == '\'') || (*p == '"')) ? MagickTrue :
    MagickFalse);
}

static void TruncatePatternForMessage(char *pattern,const ssize_t offset)
{
  if (offset >= 64)
    {
      pattern[61]='.';  /* truncate string for error message */
      pattern[62]='.';
      pattern[63]='.';
      pattern[64]='\0';
    }
}

static void ScanBracedPattern(char *pattern,ssize_t *offset,ssize_t *depth,
  const char **p)
{
  for ((*offset)=0; (*offset) < (MagickPathExtent-1L) && (*(*p) != '\0'); )
  {
    if ((*(*p) == '\\') && (*((*p)+1) != '\0'))
      {
        /*
          Skip escaped braces within braced pattern.
        */
        pattern[(*offset)++]=(*(*p)++);
        pattern[(*offset)++]=(*(*p)++);
        continue;
      }
    if (*(*p) == '[')
      (*depth)++;
    if (*(*p) == ']')
      (*depth)--;
    if ((*depth) <= 0)
      break;
    pattern[(*offset)++]=(*(*p)++);
  }
  pattern[(*offset)]='\0';
}

static MagickStatusType EvaluatePixelExpression(FxInfo *fx_info,
  const Image *image,PixelInfo *pixel,ExceptionInfo *exception)
{
  double
    value;

  MagickStatusType
    fx_status;

  fx_status=FxEvaluateChannelExpression(fx_info,RedPixelChannel,0,0,
    &value,exception);
  pixel->red=(double) QuantumRange*value;
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,
    GreenPixelChannel,0,0,&value,exception);
  pixel->green=(double) QuantumRange*value;
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,
    BluePixelChannel,0,0,&value,exception);
  pixel->blue=(double) QuantumRange*value;
  if (image->colorspace == CMYKColorspace)
    {
      fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,
        BlackPixelChannel,0,0,&value,exception);
      pixel->black=(double) QuantumRange*value;
    }
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,
    AlphaPixelChannel,0,0,&value,exception);
  pixel->alpha=(double) QuantumRange*value;
  return(fx_status);
}

static void InterpretXMLEntity(char **q,const char **p)
{
  if (LocaleNCompare("&lt;",(*p),4) == 0)
    {
      *(*q)++='<';
      (*p)+=(ptrdiff_t) 3;
    }
  else
    if (LocaleNCompare("&gt;",(*p),4) == 0)
      {
        *(*q)++='>';
        (*p)+=(ptrdiff_t) 3;
      }
    else
      if (LocaleNCompare("&amp;",(*p),5) == 0)
        {
          *(*q)++='&';
          (*p)+=(ptrdiff_t) 4;
        }
      else
        *(*q)++=(*(*p));
}

typedef struct _InterpretText
{
  char
    **interpret_text,
    **q;

  size_t
    *extent;
} InterpretText;

static MagickBooleanType ExtendInterpretBuffer(const InterpretText *text,
  const size_t string_length)
{
  size_t
    length = (string_length);

  if ((size_t) ((*text->q)-(*text->interpret_text)+(ssize_t) length+1) >=
      (*text->extent))
    {
      (*text->extent)+=length;
      (*text->interpret_text)=(char *) ResizeQuantumMemory(
        (*text->interpret_text),(*text->extent)+MagickPathExtent,
        sizeof(*(*text->interpret_text)));
      if ((*text->interpret_text) == (char *) NULL)
        return(MagickFalse);
      (*text->q)=(*text->interpret_text)+strlen((*text->interpret_text));
    }
  return(MagickTrue);
}

static MagickBooleanType AppendInterpretKeyValue(const InterpretText *text,
  const char *key,const char *value)
{
  size_t
    length = strlen(key)+strlen(value)+2;

  if (ExtendInterpretBuffer(text,length) == MagickFalse)
    return(MagickFalse);
  (*text->q)+=(ptrdiff_t) FormatLocaleString((*text->q),(*text->extent),
    "%s=%s\n",(key),(value));
  return(MagickTrue);
}

static MagickBooleanType AppendInterpretString(const InterpretText *text,
  const char *string)
{
  size_t
    length = strlen((string));

  if (ExtendInterpretBuffer(text,length) == MagickFalse)
    return(MagickFalse);
  (void) CopyMagickString((*text->q),(string),(*text->extent));
  (*text->q)+=(ptrdiff_t) length;
  return(MagickTrue);
}

static void InterpretBackslash(char **q,const char **p)
{
  switch (*(*p+1))
  {
    case '\0':
      return;
    case 'r':  /* convert to RETURN */
    {
      *(*q)++='\r';
      (*p)++;
      return;
    }
    case 'n':  /* convert to NEWLINE */
    {
      *(*q)++='\n';
      (*p)++;
      return;
    }
    case '\n':  /* EOL removal UNIX,MacOSX */
    {
      (*p)++;
      return;
    }
    case '\r':  /* EOL removal DOS,Windows */
    {
      (*p)++;
      if (**p == '\n') /* return-newline EOL */
        (*p)++;
      return;
    }
    default:
    {
      (*p)++;
      *(*q)++=(**p);
    }
  }
}

static MagickBooleanType InterpretCharacter(char **q,const char **p)
{
  switch (**p)
  {
    case '\\':
    {
      InterpretBackslash(q,p);
      return(MagickTrue);
    }
    case '&':
    {
      InterpretXMLEntity(q,p);
      return(MagickTrue);
    }
    case '%':
      break;  /* continue to next set of handlers */
    default:
    {
      *(*q)++=(**p);  /* any thing else is 'as normal' */
      return(MagickTrue);
    }
  }
  return(MagickFalse);
}

static MagickBooleanType AppendFxValue(const InterpretText *text,
  Image *property_image,const char *pattern,ExceptionInfo *exception)
{
  double
    value;

  FxInfo
    *fx_info;

  MagickBooleanType
    fx_status;

  /*
    FX - value calculator.
  */
  fx_info=AcquireFxInfo(property_image,pattern+3,exception);
  if (fx_info == (FxInfo *) NULL)
    return(MagickTrue);
  fx_status=FxEvaluateChannelExpression(fx_info,CompositePixelChannel,
    0,0,&value,exception);
  fx_info=DestroyFxInfo(fx_info);
  if (fx_status != MagickFalse)
    {
      char
        result[MagickPathExtent];

      (void) FormatLocaleString(result,MagickPathExtent,"%.*g",
        GetMagickPrecision(),(double) value);
      if (AppendInterpretString(text,result) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AppendHexValue(const InterpretText *text,
  Image *property_image,const char *pattern,ExceptionInfo *exception)
{
  FxInfo
    *fx_info;

  MagickStatusType
    fx_status;

  PixelInfo
    pixel;

  /*
    Pixel - color value calculator.
  */
  GetPixelInfo(property_image,&pixel);
  fx_info=AcquireFxInfo(property_image,pattern+4,exception);
  if (fx_info == (FxInfo *) NULL)
    return(MagickTrue);
  fx_status=EvaluatePixelExpression(fx_info,property_image,&pixel,
    exception);
  fx_info=DestroyFxInfo(fx_info);
  if (fx_status != MagickFalse)
    {
      char
        hex[MagickPathExtent];

      GetColorTuple(&pixel,MagickTrue,hex);
      if (AppendInterpretString(text,hex+1) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AppendPixelValue(const InterpretText *text,
  Image *property_image,const char *pattern,ExceptionInfo *exception)
{
  const char
    *string;

  FxInfo
    *fx_info;

  MagickStatusType
    fx_status;

  PixelInfo
    pixel;

  /*
    Pixel - color value calculator.
  */
  GetPixelInfo(property_image,&pixel);
  fx_info=AcquireFxInfo(property_image,pattern+6,exception);
  if (fx_info == (FxInfo *) NULL)
    return(MagickTrue);
  fx_status=EvaluatePixelExpression(fx_info,property_image,&pixel,
    exception);
  fx_info=DestroyFxInfo(fx_info);
  if (fx_status != MagickFalse)
    {
      char
        name[MagickPathExtent];

      GetColorTuple(&pixel,MagickFalse,name);
      string=GetImageArtifact(property_image,"pixel:compliance");
      if (string != (char *) NULL)
        {
          ComplianceType compliance=(ComplianceType) ParseCommandOption(
            MagickComplianceOptions,MagickFalse,string);
          (void) QueryColorname(property_image,&pixel,compliance,name,
            exception);
        }
      if (AppendInterpretString(text,name) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _InterpretSources
{
  ImageInfo
    *property_info;

  Image
    *property_image;

  ExceptionInfo
    *exception;
} InterpretSources;

typedef enum
{
  OptionGlobSide,
  ArtifactGlobSide
} GlobSide;

static MagickBooleanType AppendGlobbedValues(const InterpretSources *sources,
  const InterpretText *text,const char *glob,const GlobSide side)
{
  const char
    *key,
    *string;

  Image
    *const property_image = sources->property_image;

  ImageInfo
    *const property_info = sources->property_info;

  ((side == OptionGlobSide) ? ResetImageOptionIterator(property_info) :
    ResetImageArtifactIterator(property_image));
  while ((key=((side == OptionGlobSide) ? GetNextImageOption(property_info) :
         GetNextImageArtifact(property_image))) != (const char *) NULL)
    if (GlobExpression(key,glob,MagickTrue) != MagickFalse)
      {
        string=((side == OptionGlobSide) ? GetImageOption(property_info,key) :
          GetImageArtifact(property_image,key));
        if ((string != (const char *) NULL) &&
            (AppendInterpretKeyValue(text,key,string) == MagickFalse))
          return(MagickFalse);
      }
  return(MagickTrue);
}

static MagickBooleanType AppendGlobbedProperties(const InterpretText *text,
  Image *property_image,const char *pattern,ExceptionInfo *exception)
{
  const char
    *key,
    *string;

  ResetImagePropertyIterator(property_image);
  while ((key=GetNextImageProperty(property_image)) != (const char *) NULL)
    if (GlobExpression(key,pattern,MagickTrue) != MagickFalse)
      {
        string=GetImageProperty(property_image,key,exception);
        if ((string != (const char *) NULL) &&
            (AppendInterpretKeyValue(text,key,string) == MagickFalse))
          return(MagickFalse);
      }
  return(MagickTrue);
}

static MagickBooleanType WarnUnknownProperty(ExceptionInfo *exception,
  char *pattern,const ssize_t offset)
{
  TruncatePatternForMessage(pattern,offset);
  (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
    "UnknownImageProperty","\"%%[%s]\"",pattern);
  return(MagickTrue);
}

typedef struct _BracedPattern
{
  char
    *pattern;

  ssize_t
    offset;
} BracedPattern;

static MagickBooleanType AppendOptionOrArtifact(
  const InterpretSources *sources,const InterpretText *text,
  const BracedPattern *braced,const GlobSide side)
{
  char
    *const pattern = braced->pattern;

  const char
    *string;

  const ssize_t
    offset = braced->offset;

  ExceptionInfo
    *const exception = sources->exception;

  Image
    *const property_image = sources->property_image;

  ImageInfo
    *const property_info = sources->property_info;

  /*
    Option - direct global option lookup (with globbing).
    Artifact - direct image artifact lookup (with glob).
  */
  if (IsGlob(((side == OptionGlobSide) ? pattern+7 : pattern+9)) != MagickFalse)
    return(AppendGlobbedValues(sources,text,((side == OptionGlobSide) ?
      pattern+7 : pattern+9),side));
  string=((side == OptionGlobSide) ? GetImageOption(property_info,pattern+7) :
    GetImageArtifact(property_image,pattern+9));
  if (string == (char *) NULL)
    return(WarnUnknownProperty(exception,pattern,offset));
  return(AppendInterpretString(text,string));
}

static MagickBooleanType AppendPrefixedProperty(
  const InterpretSources *sources,const InterpretText *text,
  const BracedPattern *braced)
{
  char
    *const pattern = braced->pattern;

  const char
    *string;

  const ssize_t
    offset = braced->offset;

  ExceptionInfo
    *const exception = sources->exception;

  Image
    *const property_image = sources->property_image;

  /*
    Property - direct image property lookup (with glob).
  */
  if (IsGlob(pattern+9) != MagickFalse)
    return(AppendGlobbedProperties(text,property_image,pattern,
      exception));
  string=GetImageProperty(property_image,pattern+9,exception);
  if (string == (char *) NULL)
    return(WarnUnknownProperty(exception,pattern,offset));
  return(AppendInterpretString(text,string));
}

static MagickBooleanType AppendUnprefixedValue(
  const InterpretSources *sources,const InterpretText *text,
  const BracedPattern *braced)
{
  char
    *const pattern = braced->pattern;

  const char
    *string;

  const ssize_t
    offset = braced->offset;

  ExceptionInfo
    *const exception = sources->exception;

  Image
    *const property_image = sources->property_image;

  ImageInfo
    *const property_info = sources->property_info;

  /*
    Properties without special prefix.
  */
  string=GetImageProperty(property_image,pattern,exception);
  if (string != (const char *) NULL)
    {
      if (AppendInterpretString(text,string) == MagickFalse)
        return(MagickFalse);
      (void) DeleteImageArtifact(property_image,"magick-property");
      (void) DeleteImageOption(property_info,"magick-property");
      return(MagickTrue);
    }
  if (IsGlob(pattern) != MagickFalse)
    return(AppendGlobbedProperties(text,property_image,pattern,exception));
  /*
    Known property or attribute.
  */
  string=GetMagickProperty(property_info,property_image,pattern,exception);
  if (string != (const char *) NULL)
    return(AppendInterpretString(text,string));
  /*
    Per-image artifact.
  */
  string=GetImageArtifact(property_image,pattern);
  if (string != (char *) NULL)
    return(AppendInterpretString(text,string));
  /*
    Direct option lookup.
  */
  string=GetImageOption(property_info,pattern);
  if (string != (char *) NULL)
    return(AppendInterpretString(text,string));
  return(WarnUnknownProperty(exception,pattern,offset));
}

static MagickBooleanType AppendBracedValue(
  const InterpretSources *sources,const InterpretText *text,
  const BracedPattern *braced)
{
  char
    *const pattern = braced->pattern;

  ExceptionInfo
    *const exception = sources->exception;

  Image
    *const property_image = sources->property_image;

  /*
    Special Lookup Prefixes %[prefix:...].
  */
  if (LocaleNCompare("fx:",pattern,3) == 0)
    return(AppendFxValue(text,property_image,pattern,exception));
  if (LocaleNCompare("hex:",pattern,4) == 0)
    return(AppendHexValue(text,property_image,pattern,exception));
  if (LocaleNCompare("pixel:",pattern,6) == 0)
    return(AppendPixelValue(text,property_image,pattern,exception));
  if (LocaleNCompare("option:",pattern,7) == 0)
    return(AppendOptionOrArtifact(sources,text,braced,OptionGlobSide));
  if (LocaleNCompare("artifact:",pattern,9) == 0)
    return(AppendOptionOrArtifact(sources,text,braced,ArtifactGlobSide));
  if (LocaleNCompare("property:",pattern,9) == 0)
    return(AppendPrefixedProperty(sources,text,braced));
  return(AppendUnprefixedValue(sources,text,braced));
}

static MagickBooleanType InterpretBracedEscape(
  const InterpretSources *sources,const InterpretText *text,const char **p)
{
  char
    pattern[2*MagickPathExtent] = "\0";

  BracedPattern
    braced;

  ssize_t
    depth,
    offset;

  ExceptionInfo
    *const exception = sources->exception;

  /*
    Braced Percent Escape %[...].
  */
  (*p)++;  /* advance p to just inside the opening brace */
  depth=1;
  if (**p == ']')
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "UnknownImageProperty","\"%%[]\"");
      return(MagickFalse);
    }
  ScanBracedPattern(pattern,&offset,&depth,p);
  if (depth != 0)
    {
      /*
        Check for unmatched final ']' for "%[...]".
      */
      TruncatePatternForMessage(pattern,offset);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnbalancedBraces","\"%%[%s\"",pattern);
      return(MagickFalse);
    }
  braced.pattern=pattern;
  braced.offset=offset;
  return(AppendBracedValue(sources,text,&braced));
}

static MagickBooleanType ReadInterpretTextFile(const char *p,
  char **interpret_text,ExceptionInfo *exception,char **result)
{
  if ((*p == '@') && (IsPathAccessible(p+1) != MagickFalse))
    {
      /*
        Handle a '@' replace string from file.
      */
      (*interpret_text)=FileToString(p,~0UL,exception);
      if ((*interpret_text) != (char *) NULL)
        {
          (*result)=(*interpret_text);
          return(MagickTrue);
        }
    }
  return(MagickFalse);
}

MagickExport char *InterpretImageProperties(ImageInfo *image_info,Image *image,
  const char *embed_text,ExceptionInfo *exception)
{
#define ExtendInterpretText(string_length) \
{ \
  if (ExtendInterpretBuffer(&text,(string_length)) == MagickFalse) \
    { \
      status=MagickFalse; \
      goto cleanup; \
    } \
}

#define AppendString2Text(string) \
{ \
  if (AppendInterpretString(&text,(string)) == MagickFalse) \
    { \
      status=MagickFalse; \
      goto cleanup; \
    } \
}

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

  MagickBooleanType
    number,
    status = MagickTrue;

  size_t
    extent;  /* allocated length of interpret_text */

  const InterpretText
    text = { &interpret_text, &q, &extent };

  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
    if ((image_info != (ImageInfo *) NULL) && (IsEventLogging() != MagickFalse))
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
        image_info->filename);
    else
      if (IsEventLogging() != MagickFalse)
        (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no image");
  if (embed_text == (const char *) NULL)
    return(ConstantString(""));
  p=embed_text;
  while ((isspace((int) ((unsigned char) *p)) != 0) && (*p != '\0'))
    p++;
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
  if (IsNonEmptyImage(image) != MagickFalse)
    property_image=image;
  else
    {
      property_image=AcquireImage(image_info,exception);
      if (property_image == (Image *) NULL)
        {
          status=MagickFalse;
          goto cleanup;
        }
      (void) SetImageExtent(property_image,1,1,exception);
      (void) SetImageBackgroundColor(property_image,exception);
    }
  interpret_text=AcquireString(embed_text); /* new string with extra space */
  if (interpret_text == (char *) NULL)
    {
      status=MagickFalse;
      goto cleanup;
    }
  extent=MagickPathExtent;                  /* allocated space in string */
  number=MagickFalse;                       /* is last char a number? */
  for (q=interpret_text; *p!='\0'; number=isdigit((int) ((unsigned char) *p)) ? MagickTrue : MagickFalse,p++)
  {
    /*
      Look for the various escapes, (and handle other specials)
    */
    *q='\0';
    ExtendInterpretText(MagickPathExtent);
    if (InterpretCharacter(&q,&p) != MagickFalse)
      continue;
    p++; /* advance beyond the percent */
    /*
      Doubled Percent - or percent at end of string.
    */
    if (IsEscapeEnd(p) != MagickFalse)
      p--;
    if (*p == '%')
      {
        *q++='%';
        continue;
      }
    /*
      Single letter escapes %c.
    */
    if (*p != '[')
      {
        const char
          *string;

        if (number != MagickFalse)
          {
            /*
              But only if not preceded by a number!
            */
            *q++='%'; /* do NOT substitute the percent */
            p--;      /* back up one */
            continue;
          }
        string=GetMagickPropertyLetter(property_info,image,*p,exception);
        if (string != (char *) NULL)
          {
            AppendString2Text(string);
            (void) DeleteImageArtifact(property_image,"magick-property");
            (void) DeleteImageOption(property_info,"magick-property");
            continue;
          }
        (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
          "UnknownImageProperty","\"%%%c\"",*p);
        continue;
      }
    {
      const InterpretSources
        sources = { property_info, property_image, exception };

      if (InterpretBracedEscape(&sources,&text,&p) == MagickFalse)
        {
          status=MagickFalse;
          goto cleanup;
        }
    }
  }
  *q='\0';

cleanup:
  if ((property_image != image) && (property_image != (Image *) NULL))
    property_image=DestroyImage(property_image);
  if ((property_info != image_info) && (property_info != (ImageInfo *) NULL))
    property_info=DestroyImageInfo(property_info);
  if (status == MagickFalse)
    {
      if (interpret_text != (char *) NULL)
        interpret_text=DestroyString(interpret_text);
      return((char *) NULL);
    }
  return(interpret_text);
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
static inline MagickBooleanType IsReadOnlyMProperty(const char *property)
{
  return(((LocaleCompare("magick",property) == 0) ||
    (LocaleCompare("max",property) == 0) ||
    (LocaleCompare("mean",property) == 0) ||
    (LocaleCompare("min",property) == 0) ||
    (LocaleCompare("min",property) == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsReadOnlySProperty(const char *property)
{
  return(((LocaleCompare("size",property) == 0) ||
    (LocaleCompare("skewness",property) == 0) ||
    (LocaleCompare("scenes",property) == 0) ||
    (LocaleCompare("standard-deviation",property) == 0)) ? MagickTrue :
    MagickFalse);
}

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

MagickExport MagickBooleanType SetImageProperty(Image *image,
  const char *property,const char *value,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  MagickStatusType
    flags;

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
  switch (*property)
  {
#if 0  /* Percent escape's sets values with this prefix: for later use
          Throwing an exception causes this setting to fail */
    case '8':
    {
      if (LocaleNCompare("8bim:",property,5) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break;
    }
#endif
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
    case 'C':
    case 'c':
    {
      if (LocaleCompare("channels",property) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      if (LocaleCompare("colorspace",property) == 0)
        {
          return(SetImageColorspaceProperty(image,value,exception));
        }
      break; /* not an attribute, add as a property */
    }
    case 'D':
    case 'd':
    {
      if (LocaleCompare("delay",property) == 0)
        {
          return(SetImageDelayProperty(image,value));
        }
      if (LocaleCompare("delay_units",property) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      if (LocaleCompare("density",property) == 0)
        {
          return(SetImageDensityProperty(image,value,&flags));
        }
      if (LocaleCompare("depth",property) == 0)
        {
          image->depth=StringToUnsignedLong(value);
          return(MagickTrue);
        }
      break; /* not an attribute, add as a property */
    }
#if 0  /* Percent escape's sets values with this prefix: for later use
          Throwing an exception causes this setting to fail */
    case 'E':
    case 'e':
    {
      if (LocaleNCompare("exif:",property,5) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    }
    case 'F':
    case 'f':
    {
      if (LocaleNCompare("fx:",property,3) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    }
#endif
    case 'G':
    case 'g':
    {
      if (LocaleCompare("gamma",property) == 0)
        {
          image->gamma=StringToDouble(value,(char **) NULL);
          return(MagickTrue);
        }
      break; /* not an attribute, add as a property */
    }
    case 'H':
    case 'h':
    {
      if (LocaleCompare("height",property) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    }
    case 'I':
    case 'i':
    {
#if 0  /* Percent escape's sets values with this prefix: for later use
          Throwing an exception causes this setting to fail */
      if (LocaleNCompare("iptc:",property,5) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
#endif
      break; /* not an attribute, add as a property */
    }
    case 'K':
    case 'k':
      if (LocaleCompare("kurtosis",property) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    case 'L':
    case 'l':
    {
      if (LocaleCompare("loop",property) == 0)
        {
          image->iterations=StringToUnsignedLong(value);
          return(MagickTrue);
        }
      break; /* not an attribute, add as a property */
    }
    case 'M':
    case 'm':
      if (IsReadOnlyMProperty(property) != MagickFalse)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
             "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    case 'O':
    case 'o':
      if (LocaleCompare("opaque",property) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    case 'P':
    case 'p':
    {
      if (LocaleCompare("page",property) == 0)
        {
          char
            *geometry;

          geometry=GetPageGeometry(value);
          flags=ParseAbsoluteGeometry(geometry,&image->page);
          geometry=DestroyString(geometry);
          return(MagickTrue);
        }
#if 0  /* Percent escape's sets values with this prefix: for later use
          Throwing an exception causes this setting to fail */
      if (LocaleNCompare("pixel:",property,6) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
#endif
      break; /* not an attribute, add as a property */
    }
    case 'R':
    case 'r':
    {
      break; /* not an attribute, add as a property */
    }
    case 'S':
    case 's':
      if (IsReadOnlySProperty(property) != MagickFalse)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    case 'T':
    case 't':
    {
      if (LocaleCompare("tile-offset",property) == 0)
        {
          char
            *geometry;

          geometry=GetPageGeometry(value);
          flags=ParseAbsoluteGeometry(geometry,&image->tile_offset);
          geometry=DestroyString(geometry);
          return(MagickTrue);
        }
      break; /* not an attribute, add as a property */
    }
    case 'U':
    case 'u':
    {
      break; /* not an attribute, add as a property */
    }
    case 'V':
    case 'v':
    {
      if (LocaleCompare("version",property) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    }
    case 'W':
    case 'w':
    {
      if (LocaleCompare("width",property) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    }
#if 0  /* Percent escape's sets values with this prefix: for later use
          Throwing an exception causes this setting to fail */
    case 'X':
    case 'x':
    {
      if (LocaleNCompare("xmp:",property,4) == 0)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
            "SetReadOnlyProperty","`%s'",property);
          return(MagickFalse);
        }
      break; /* not an attribute, add as a property */
    }
#endif
  }
  /* Default: not an attribute, add as a property */
  status=AddValueToSplayTree((SplayTreeInfo *) image->properties,
    ConstantString(property),ConstantString(value));
  /* FUTURE: error if status is bad? */
  return(status);
}
