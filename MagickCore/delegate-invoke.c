/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%           DDDD   EEEEE  L      EEEEE   GGGG   AAA   TTTTT  EEEEE            %
%           D   D  E      L      E      G      A   A    T    E                %
%           D   D  EEE    L      EEE    G  GG  AAAAA    T    EEE              %
%           D   D  E      L      E      G   G  A   A    T    E                %
%           DDDD   EEEEE  LLLLL  EEEEE   GGG   A   A    T    EEEEE            %
%                                                                             %
%                  IIIII  N   N  V   V   OOO   K   K  EEEEE                   %
%                    I    NN  N  V   V  O   O  K  K   E                       %
%                    I    N N N  V   V  O   O  KKK    EEE                     %
%                    I    N  NN   V V   O   O  K  K   E                       %
%                  IIIII  N   N    V     OOO   K   K  EEEEE                   %
%                                                                             %
%                                                                             %
%               Delegate Command Interpretation and Invocation                %
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
%  The Delegates methods associate a set of commands with a particular
%  image format.  ImageMagick uses delegates for formats it does not handle
%  directly.
%
%  Thanks to Bob Friesenhahn for the initial inspiration and design of the
%  delegates methods.
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/delegate-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/linked-list-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/property.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature.h"
#include "MagickCore/string_.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e C o m m a n d                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateCommand() replaces any embedded formatting characters with the
%  appropriate image attribute and returns the resulting command.
%
%  The format of the GetDelegateCommand method is:
%
%      char *GetDelegateCommand(const ImageInfo *image_info,Image *image,
%        const char *decode,const char *encode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o command: Method GetDelegateCommand returns the command associated
%      with specified delegate tag.
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o decode: Specifies the decode delegate we are searching for as a
%      character string.
%
%    o encode: Specifies the encode delegate we are searching for as a
%      character string.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsImageInfoLetter(const char letter)
{
  return(((letter != '\0') && (strchr("aouS",letter) != (char *) NULL)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsImageLetter(const char letter)
{
  return(((letter != '\0') &&
    (strchr("bdefghimpqrstwxyzACDFGHIMOPQTUWXY~@#",letter) != (char *) NULL)) ?
    MagickTrue : MagickFalse);
}

static const char *GetListProperty(const Image *image,const char letter,
  char *value)
{
  switch (letter)
  {
    case 'n': /* Number of images in the list.  */
    {
      if (image != (Image *) NULL)
        (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
          GetImageListLength(image));
      return(value);
    }
    case '%': /* percent escaped */
      return("%");
    default:
      return(value);
  }
}

static const char *GetMnemonicProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'r': /* Image storage class, colorspace, and alpha enabled.  */
    {
      ColorspaceType
        colorspace;

      colorspace=image->colorspace;
      (void) FormatLocaleString(value,MagickPathExtent,"%s %s %s",
        CommandOptionToMnemonic(MagickClassOptions,(ssize_t)
        image->storage_class),CommandOptionToMnemonic(MagickColorspaceOptions,
        (ssize_t) colorspace),image->alpha_trait != UndefinedPixelTrait ?
        "Alpha" : "");
      return(value);
    }
    case 'A': /* Image alpha channel  */
      return(CommandOptionToMnemonic(MagickPixelTraitOptions,(ssize_t)
        image->alpha_trait));
    case 'C': /* Image compression method.  */
      return(CommandOptionToMnemonic(MagickCompressOptions,
        (ssize_t) image->compression));
    case 'D': /* Image dispose method.  */
      return(CommandOptionToMnemonic(MagickDisposeOptions,
        (ssize_t) image->dispose));
    case 'U': /* Image resolution units. */
      return(CommandOptionToMnemonic(MagickResolutionOptions,
        (ssize_t) image->units));
    case '#':
    {
      /*
        Image signature.
      */
      (void) SignatureImage(image,exception);
      return(GetImageProperty(image,"signature",exception));
    }
    default:
      return(GetListProperty(image,letter,value));
  }
}

static const char *GetQualityProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case '~': /* BPG image compression quality */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (100-(image->quality == 0 ? 42 : image->quality))/2);
      return(value);
    }
    case 'Q': /* image compression quality */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (image->quality == 0 ? 92 : image->quality));
      return(value);
    }
    default:
      return(GetMnemonicProperty(image,letter,value,exception));
  }
}

static double GetResolutionValue(const double resolution,
  const ResolutionType units)
{
  return(fabs(resolution) > MagickEpsilon ? resolution :
    units == PixelsPerCentimeterResolution ? DefaultResolution/2.54 :
    DefaultResolution);
}

static const char *GetMeasureProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'b':  /* image size read in - in bytes */
    {
      (void) FormatMagickSize(image->extent,MagickFalse,"B",MagickPathExtent,
        value);
      if (image->extent == 0)
        (void) FormatMagickSize(GetBlobSize(image),MagickFalse,"B",
          MagickPathExtent,value);
      return(value);
    }
    case 'x': /* Image horizontal resolution (with units) */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",
        GetResolutionValue(image->resolution.x,image->units));
      return(value);
    }
    case 'y': /* Image vertical resolution (with units) */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",
        GetResolutionValue(image->resolution.y,image->units));
      return(value);
    }
    default:
      return(GetQualityProperty(image,letter,value,exception));
  }
}

static const char *GetCounterProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'p': /* Image index in current image list */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        GetImageIndexInList(image));
      return(value);
    }
    case 'q': /* Quantum depth of image in memory */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        MAGICKCORE_QUANTUM_DEPTH);
      return(value);
    }
    case 's': /* Image scene number */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->scene);
      return(value);
    }
    case 'z': /* Image depth as read in */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",
        (double) image->depth);
      return(value);
    }
    case 'I': /* image iterations for animations */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->iterations);
      return(value);
    }
    case 'T': /* image time delay for animations */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->delay);
      return(value);
    }
    default:
      return(GetMeasureProperty(image,letter,value,exception));
  }
}

static const char *GetPageProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'H': /* layer canvas height */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",
        (double) image->page.height);
      return(value);
    }
    case 'W': /* layer canvas width */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        image->page.width);
      return(value);
    }
    case 'X': /* layer canvas X offset */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%+.20g",(double)
        image->page.x);
      return(value);
    }
    case 'Y': /* layer canvas Y offset */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%+.20g",(double)
        image->page.y);
      return(value);
    }
    default:
      return(GetCounterProperty(image,letter,value,exception));
  }
}

static const char *GetSizeProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'h': /* Image height (current) */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (image->rows != 0 ? image->rows : image->magick_rows));
      return(value);
    }
    case 'w': /* Image width (current) */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (image->columns != 0 ? image->columns : image->magick_columns));
      return(value);
    }
    case 'G': /* Image size as geometry = "%wx%h" */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17gx%.17g",
        (double) image->magick_columns,(double) image->magick_rows);
      return(value);
    }
    case 'P': /* layer canvas page size = "%Wx%H" */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%.17gx%.17g",
        (double) image->page.width,(double) image->page.height);
      return(value);
    }
    default:
      return(GetPageProperty(image,letter,value,exception));
  }
}

static const char *GetGeometryProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'g': /* Image geometry, canvas and offset  %Wx%H+%X+%Y */
    {
      (void) FormatLocaleString(value,MagickPathExtent,
        "%.17gx%.17g%+.20g%+.20g",(double) image->page.width,(double)
        image->page.height,(double) image->page.x,(double) image->page.y);
      return(value);
    }
    case 'O': /* layer canvas offset with sign = "+%X+%Y" */
    {
      (void) FormatLocaleString(value,MagickPathExtent,"%+ld%+ld",(long)
        image->page.x,(long) image->page.y);
      return(value);
    }
    case '@': /* Trim bounding box, without actually trimming! */
    {
      RectangleInfo
        page;

      page=GetImageBoundingBox(image,exception);
      (void) FormatLocaleString(value,MagickPathExtent,
        "%.17gx%.17g%+.20g%+.20g",(double) page.width,(double) page.height,
        (double) page.x,(double) page.y);
      return(value);
    }
    default:
      return(GetSizeProperty(image,letter,value,exception));
  }
}

static const char *GetNameProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'i': /* Filename last used for an image (read or write) */
      return(image->filename);
    case 'm': /* Image format (file magick) */
      return(image->magick);
    case 'F':
    {
      /*
        Magick filename - filename given incl. coder & read mods.
      */
      (void) CopyMagickString(value,image->magick_filename,MagickPathExtent);
      return(value);
    }
    case 'M': /* Magick filename - filename given incl. coder & read mods */
      return(image->magick_filename);
    default:
      return(GetGeometryProperty(image,letter,value,exception));
  }
}

static const char *GetPathProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'd':  /* Directory component of filename */
    {
      GetPathComponent(image->magick_filename,HeadPath,value);
      return(value);
    }
    case 'e': /* Filename extension (suffix) of image file */
    {
      GetPathComponent(image->magick_filename,ExtensionPath,value);
      return(value);
    }
    case 'f': /* Filename without directory component */
    {
      GetPathComponent(image->magick_filename,TailPath,value);
      return(value);
    }
    case 't': /* Base filename without directory or extension */
    {
      GetPathComponent(image->magick_filename,BasePath,value);
      return(value);
    }
    default:
      return(GetNameProperty(image,letter,value,exception));
  }
}

static const char *GetImageInfoProperty(const ImageInfo *image_info,
  const char letter,char *value)
{
  switch (letter)
  {
    case 'a': /* authentication passphrase */
      return(GetImageOption(image_info,"authenticate"));
    case 'o': /* Output Filename */
      return(image_info->filename);
    case 'u': /* Unique filename */
      return(image_info->unique);
    default:
    {
      /* Number of scenes in image list.  */
      (void) FormatLocaleString(value,MagickPathExtent,"%.17g",(double)
        (image_info->number_scenes == 0 ? 2147483647 :
         image_info->number_scenes));
      return(value);
    }
  }
}

static char *GetMagickPropertyLetter(ImageInfo *image_info,Image *image,
  const char letter,ExceptionInfo *exception)
{
  char
    value[MagickPathExtent];

  const char
    *string;

  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
    if ((image_info != (ImageInfo *) NULL) && (IsEventLogging() != MagickFalse))
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no-images");
  /*
    Get properties that are directly defined by images.
  */
  *value='\0';           /* formatted string */
  string=(const char *) value;
  if ((IsImageInfoLetter(letter) != MagickFalse) &&
      (image_info == (ImageInfo *) NULL))
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "NoImageInfoForProperty","\"%%%c\"",letter);
  else
    if ((IsImageLetter(letter) != MagickFalse) && (image == (Image *) NULL))
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "NoImageForProperty","\"%%%c\"",letter);
    else
      if (IsImageInfoLetter(letter) != MagickFalse)
        string=GetImageInfoProperty(image_info,letter,value);
      else
        string=GetPathProperty(image,letter,value,exception);
  return(SanitizeDelegateString(string));
}

static inline MagickBooleanType IsPropertyEnd(const char *p)
{
  return(((*p == '\0') || (*p == '\'') || (*p == '"')) ? MagickTrue :
    MagickFalse);
}

static void DecodeDelegateEscape(const char **p,char **q)
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

static void DecodeDelegateEntity(const char **p,char **q)
{
  if (LocaleNCompare("&lt;",*p,4) == 0)
    {
      *(*q)++='<';
      *p+=(ptrdiff_t) 3;
    }
  else
    if (LocaleNCompare("&gt;",*p,4) == 0)
      {
        *(*q)++='>';
        *p+=(ptrdiff_t) 3;
      }
    else
      if (LocaleNCompare("&amp;",*p,5) == 0)
        {
          *(*q)++='&';
          *p+=(ptrdiff_t) 4;
        }
      else
        *(*q)++=(**p);
}

#define ExtendInterpretText(string_length) \
{ \
  size_t length=(string_length); \
  if ((size_t) (q-interpret_text+(ssize_t) length+1) >= extent) \
    { \
      extent+=length; \
      interpret_text=(char *) ResizeQuantumMemory(interpret_text,extent+ \
        MagickPathExtent,sizeof(*interpret_text)); \
      if (interpret_text == (char *) NULL) \
        return((char *) NULL); \
      q=interpret_text+strlen(interpret_text); \
   } \
}

#define AppendKeyValue2Text(key,value)\
{ \
  size_t length=strlen(key)+strlen(value)+2; \
  if ((size_t) (q-interpret_text+length+1) >= extent) \
    { \
      extent+=length; \
      interpret_text=(char *) ResizeQuantumMemory(interpret_text,extent+ \
        MagickPathExtent,sizeof(*interpret_text)); \
      if (interpret_text == (char *) NULL) \
        return((char *) NULL); \
      q=interpret_text+strlen(interpret_text); \
     } \
   q+=(ptrdiff_t) FormatLocaleString(q,extent,"%s=%s\n",(key),(value)); \
}

#define AppendString2Text(string) \
{ \
  size_t length=strlen((string)); \
  if ((size_t) (q-interpret_text+(ssize_t) length+1) >= extent) \
    { \
      extent+=length; \
      interpret_text=(char *) ResizeQuantumMemory(interpret_text,extent+ \
        MagickPathExtent,sizeof(*interpret_text)); \
      if (interpret_text == (char *) NULL) \
        return((char *) NULL); \
      q=interpret_text+strlen(interpret_text); \
    } \
  (void) CopyMagickString(q,(string),extent); \
  q+=(ptrdiff_t) length; \
}

static MagickBooleanType DecodeDelegatePercent(const char **p,char **q,
  const MagickBooleanType number)
{
  /*
    Doubled percent - or percent at end of string - and a percent that follows
    a number are literal.
  */
  (*p)++; /* advance beyond the percent */
  if (IsPropertyEnd(*p) != MagickFalse)
    (*p)--;
  if (**p == '%')
    {
      *(*q)++='%';
      return(MagickTrue);
    }
  if (number != MagickFalse)
    {
      /*
        But only if not preceded by a number!
      */
      *(*q)++='%'; /* do NOT substitute the percent */
      (*p)--;      /* back up one */
      return(MagickTrue);
    }
  return(MagickFalse);
}

static const char *SkipDelegateSpace(const char *p)
{
  while ((isspace((int) ((unsigned char) *p)) != 0) && (*p != '\0'))
    p++;
  return(p);
}

static MagickBooleanType DecodeDelegateCharacter(const char **p,char **q)
{
  switch (*(*p))
  {
    case '\\':
    {
      DecodeDelegateEscape(p,q);
      return(MagickTrue);
    }
    case '&':
    {
      DecodeDelegateEntity(p,q);
      return(MagickTrue);
    }
    case '%':
      break;  /* continue to next set of handlers */
    default:
    {
      *(*q)++=(*(*p));  /* any thing else is 'as normal' */
      return(MagickTrue);
    }
  }
  return(MagickFalse);
}

static MagickBooleanType IsEmptyDelegateText(const char *embed_text,
  const char **p,char **result)
{
  if (embed_text == (const char *) NULL)
    {
      (*result)=ConstantString("");
      return(MagickTrue);
    }
  (*p)=SkipDelegateSpace(embed_text);
  if (*(*p) == '\0')
    {
      (*result)=ConstantString("");
      return(MagickTrue);
    }
  return(MagickFalse);
}

static char *InterpretDelegateProperties(ImageInfo *image_info,
  Image *image,const char *embed_text,ExceptionInfo *exception)
{
  char
    *interpret_text,
    *returned,
    *string;

  char
    *q;  /* current position in interpret_text */

  const char
    *p;  /* position in embed_text string being expanded */

  size_t
    extent;  /* allocated length of interpret_text */

  MagickBooleanType
    number;

  assert(image == NULL || image->signature == MagickCoreSignature);
  assert(image_info == NULL || image_info->signature == MagickCoreSignature);
  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
   if ((image_info != (ImageInfo *) NULL) && (IsEventLogging() != MagickFalse))
     (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no-image");
  if (IsEmptyDelegateText(embed_text,&p,&returned) != MagickFalse)
    return(returned);
  /*
    Translate any embedded format characters.
  */
  interpret_text=AcquireString(embed_text);  /* new string with extra space */
  extent=MagickPathExtent;  /* allocated space in string */
  number=MagickFalse;  /* is last char a number? */
  for (q=interpret_text; *p!='\0';
    number=isdigit((int) ((unsigned char) *p)) ? MagickTrue : MagickFalse,p++)
  {
    /*
      Interpret escape characters (e.g. Filename: %M).
    */
    *q='\0';
    ExtendInterpretText(MagickPathExtent);
    if (DecodeDelegateCharacter(&p,&q) != MagickFalse)
      continue;
    if (DecodeDelegatePercent(&p,&q,number) != MagickFalse)
      continue;
    string=GetMagickPropertyLetter(image_info,image,*p,exception);
    if (string != (char *) NULL)
      {
        AppendString2Text(string);
        string=DestroyString(string);
        continue;
      }
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "UnknownImageProperty","\"%%%c\"",*p);
  }
  *q='\0';
  return(interpret_text);
}

MagickExport char *GetDelegateCommand(const ImageInfo *image_info,Image *image,
  const char *decode,const char *encode,ExceptionInfo *exception)
{
  char
    *command,
    **commands;

  const DelegateInfo
    *delegate_info;

  ssize_t
    i;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  delegate_info=GetDelegateInfo(decode,encode,exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
        "NoTagFound","`%s'",decode ? decode : encode);
      return((char *) NULL);
    }
  commands=StringToList(delegate_info->commands);
  if (commands == (char **) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",decode ? decode :
        encode);
      return((char *) NULL);
    }
  command=InterpretDelegateProperties((ImageInfo *) image_info,image,
    commands[0],exception);
  if (command == (char *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),ResourceLimitError,
      "MemoryAllocationFailed","`%s'",commands[0]);
  /*
    Relinquish resources.
  */
  for (i=0; commands[i] != (char *) NULL; i++)
    commands[i]=DestroyString(commands[i]);
  commands=(char **) RelinquishMagickMemory(commands);
  return(command);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n v o k e D e l e g a t e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InvokeDelegate replaces any embedded formatting characters with the
%  appropriate image attribute and executes the resulting command.  MagickFalse
%  is returned if the commands execute with success otherwise MagickTrue.
%
%  The format of the InvokeDelegate method is:
%
%      MagickBooleanType InvokeDelegate(ImageInfo *image_info,Image *image,
%        const char *decode,const char *encode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the imageInfo.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType AuthorizeDelegateCopy(const char *source,
  const char *destination,const int destination_file,ExceptionInfo *exception)
{
  if (IsPathAuthorized(WritePolicyRights,destination) == MagickFalse)
    ThrowPolicyException(destination,MagickFalse);
  if (IsPathAuthorized(ReadPolicyRights,source) == MagickFalse)
    {
      (void) close_utf8(destination_file);
      ThrowPolicyException(source,MagickFalse);
    }
  return(MagickTrue);
}

static inline MagickBooleanType IsKeptDestination(
  const MagickBooleanType overwrite,const char *destination,
  struct stat *attributes)
{
  return(((overwrite == MagickFalse) &&
    (GetPathAttributes(destination,attributes) != MagickFalse)) ? MagickTrue :
    MagickFalse);
}

static size_t GetDelegateCopyQuantum(const int source_file)
{
  size_t
    quantum;

  struct stat
    attributes = { 0 };

  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(source_file,&attributes) == 0) && (attributes.st_size > 0))
    quantum=MagickMin((size_t) attributes.st_size,MagickMaxBufferExtent);
  return(quantum);
}

typedef struct _DelegateFileCopy
{
  int
    source_file,
    destination_file;

  unsigned char
    *buffer;

  size_t
    quantum;
} DelegateFileCopy;

static void CopyDelegateFileData(const DelegateFileCopy *copy,
  MagickBooleanType *status)
{
  const int
    source_file = copy->source_file,
    destination_file = copy->destination_file;

  unsigned char
    *const buffer = copy->buffer;

  const size_t
    quantum = copy->quantum;

  ssize_t
    count = 0;

  while ((count=read(source_file,buffer,quantum)) > 0)
  {
    ssize_t written = write(destination_file,buffer,(size_t) count);
    if (written != count)
      {
        *status=MagickFalse;
        break;
      }
  }
  if (count < 0)
    *status=MagickFalse;
}

static MagickBooleanType CopyDelegateFile(const char *source,
  const char *destination,const MagickBooleanType overwrite,
  ExceptionInfo *exception)
{
  DelegateFileCopy
    copy;

  int
    destination_file = -1,
    source_file = -1;

  MagickBooleanType
    status = MagickTrue;

  size_t
    quantum = 0;

  struct stat
    attributes = { 0 };

  unsigned char
    *buffer;

  /*
    Copy source file to destination.
  */
  assert(source != (const char *) NULL);
  assert(destination != (char *) NULL);
  if (IsKeptDestination(overwrite,destination,&attributes) != MagickFalse)
    return(MagickTrue);
  if (IsPathAuthorized(WritePolicyRights,destination) == MagickFalse)
    ThrowPolicyException(source,MagickFalse);
  destination_file=open_utf8(destination,O_WRONLY | O_CLOEXEC | O_BINARY | O_CREAT |
    O_NOFOLLOW,S_MODE);
  if (destination_file == -1)
    return(MagickFalse);
  if (AuthorizeDelegateCopy(source,destination,destination_file,exception) ==
      MagickFalse)
    return(MagickFalse);
  source_file=open_utf8(source,O_RDONLY | O_CLOEXEC | O_BINARY,0);
  if (source_file == -1)
    {
      (void) close_utf8(destination_file);
      return(MagickFalse);
    }
  quantum=GetDelegateCopyQuantum(source_file);
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      (void) close_utf8(source_file);
      (void) close_utf8(destination_file);
      return(MagickFalse);
    }
  copy.source_file=source_file;
  copy.destination_file=destination_file;
  copy.buffer=buffer;
  copy.quantum=quantum;
  CopyDelegateFileData(&copy,&status);
  (void) close_utf8(destination_file);
  (void) close_utf8(source_file);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  return(status);
}

static inline MagickBooleanType IsTwoStepDelegate(
  const DelegateInfo *delegate_info,const char *decode,const char *encode)
{
  return(((delegate_info->mode != 0) && (((decode != (const char *) NULL) &&
    (delegate_info->encode != (char *) NULL)) ||
    ((encode != (const char *) NULL) &&
    (delegate_info->decode != (char *) NULL)))) ? MagickTrue : MagickFalse);
}

static void WaitForDelegateInput(const Image *image)
{
  ssize_t
    count;

  /*
    Wait for input file to 'disappear', or maximum 2 seconds.
  */
  count=20;
  while ((count-- > 0) && (access_utf8(image->filename,F_OK) == 0))
    (void) MagickDelay(100);  /* sleep 0.1 seconds */
}

static void RelinquishTemporaryInput(const Image *image,
  const MagickBooleanType temporary)
{
  if (temporary != MagickFalse)
    (void) RelinquishUniqueFileResource(image->filename);
}

typedef struct _DelegateInvocation
{
  ImageInfo
    *image_info;

  Image
    *image;

  const char
    *decode,
    *encode;

  ExceptionInfo
    *exception;

  const DelegateInfo
    *delegate_info;

  MagickBooleanType
    temporary;
} DelegateInvocation;

static MagickBooleanType WriteDelegateImages(
  const DelegateInvocation *invocation,ImageInfo **clone_info)
{
  ImageInfo
    *const image_info = invocation->image_info;

  Image
    *const image = invocation->image;

  const char
    *const decode = invocation->decode,
    *const encode = invocation->encode;

  ExceptionInfo
    *const exception = invocation->exception;

  const DelegateInfo
    *const delegate_info = invocation->delegate_info;

  const MagickBooleanType
    temporary = invocation->temporary;

  Image
    *p;

  MagickBooleanType
    status;

  for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    (void) FormatLocaleString(p->filename,MagickPathExtent,"%s:%s",
      delegate_info->decode,(*clone_info)->filename);
    status=WriteImage(*clone_info,p,exception);
    if (status == MagickFalse)
      {
        (void) RelinquishUniqueFileResource(image_info->unique);
        if (temporary != MagickFalse)
          (void) RelinquishUniqueFileResource(image->filename);
        *clone_info=DestroyImageInfo(*clone_info);
        (void) ThrowMagickException(exception,GetMagickModule(),
          DelegateError,"DelegateFailed","`%s'",decode ? decode : encode);
        return(MagickFalse);
      }
    if ((*clone_info)->adjoin != MagickFalse)
      break;
  }
  return(MagickTrue);
}

static MagickBooleanType ConvertDelegateInput(
  const DelegateInvocation *request)
{
  ImageInfo
    *const image_info = request->image_info;

  Image
    *const image = request->image;

  const char
    *const decode = request->decode,
    *const encode = request->encode;

  ExceptionInfo
    *const exception = request->exception;

  const DelegateInfo
    *const delegate_info = request->delegate_info;

  const MagickBooleanType
    temporary = request->temporary;

  char
    *magick;

  DelegateInvocation
    invocation;

  ImageInfo
    *clone_info;

  /*
    Delegate requires a particular image format.
  */
  if (AcquireUniqueFilename(image_info->unique) == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,
        "UnableToCreateTemporaryFile",image_info->unique);
      return(MagickFalse);
    }
  magick=InterpretImageProperties(image_info,image,decode != (char *) NULL ?
    delegate_info->encode : delegate_info->decode,exception);
  if (magick == (char *) NULL)
    {
      (void) RelinquishUniqueFileResource(image_info->unique);
      RelinquishTemporaryInput(image,temporary);
      (void) ThrowMagickException(exception,GetMagickModule(),
        DelegateError,"DelegateFailed","`%s'",decode ? decode : encode);
      return(MagickFalse);
    }
  LocaleUpper(magick);
  clone_info=CloneImageInfo(image_info);
  (void) CopyMagickString((char *) clone_info->magick,magick,
    MagickPathExtent);
  if (LocaleCompare(magick,"NULL") != 0)
    (void) CopyMagickString(image->magick,magick,MagickPathExtent);
  magick=DestroyString(magick);
  (void) FormatLocaleString(clone_info->filename,MagickPathExtent,"%s:",
    delegate_info->decode);
  (void) SetImageInfo(clone_info,(unsigned int) GetImageListLength(image),
    exception);
  (void) CopyMagickString(clone_info->filename,image_info->filename,
    MagickPathExtent);
  (void) CopyMagickString(image_info->filename,image->filename,
    MagickPathExtent);
  invocation.image_info=image_info;
  invocation.image=image;
  invocation.decode=decode;
  invocation.encode=encode;
  invocation.exception=exception;
  invocation.delegate_info=delegate_info;
  invocation.temporary=temporary;
  if (WriteDelegateImages(&invocation,&clone_info) == MagickFalse)
    return(MagickFalse);
  (void) RelinquishUniqueFileResource(image_info->unique);
  clone_info=DestroyImageInfo(clone_info);
  return(MagickTrue);
}

static void RestoreDelegateInput(const Image *image,
  const char *input_filename,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (IsPathAccessible(input_filename) != MagickFalse)
    {
      if (CopyDelegateFile(image->filename,input_filename,MagickFalse,exception) == MagickFalse)
        {
          (void) RelinquishUniqueFileResource(input_filename);
          *status=MagickFalse;
        }
    }
}

static MagickBooleanType LinkDelegateInput(Image *image,
  const char *input_filename,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (IsPathAccessible(input_filename) != MagickFalse)
    {
      *status=AcquireUniqueSymbolicLink(input_filename,image->filename);
      if (*status == MagickFalse)
        {
          ThrowFileException(exception,FileOpenError,
            "UnableToCreateTemporaryFile",input_filename);
          return(MagickFalse);
        }
    }
  return(MagickTrue);
}

typedef struct _DelegateExecution
{
  const DelegateInfo
    *delegate_info;

  ImageInfo
    *image_info;

  Image
    *image;

  ExceptionInfo
    *exception;

  const char
    *input_filename,
    *output_filename,
    *text;
} DelegateExecution;

static void ExecuteDelegateCommand(const DelegateExecution *execution,
  MagickBooleanType *status,char **command)
{
  const DelegateInfo
    *const delegate_info = execution->delegate_info;

  ImageInfo
    *const image_info = execution->image_info;

  Image
    *const image = execution->image;

  ExceptionInfo
    *const exception = execution->exception;

  if (*command != (char *) NULL)
    {
      /*
        Execute delegate.
      */
      if (ExternalDelegateCommand(delegate_info->spawn,image_info->verbose,*command,(char *) NULL,exception) != 0)
        *status=MagickFalse;
      if (delegate_info->spawn != MagickFalse)
        WaitForDelegateInput(image);
      *command=DestroyString(*command);
    }
}

static MagickBooleanType CopyDelegateOutput(const ImageInfo *image_info,
  const char *input_filename,const char *output_filename,
  ExceptionInfo *exception)
{
  if ((strcmp(input_filename,output_filename) != 0) &&
      (CopyDelegateFile(image_info->filename,output_filename,MagickTrue,exception) == MagickFalse))
    {
      (void) RelinquishUniqueFileResource(output_filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void RelinquishDelegateFiles(const ImageInfo *image_info,
  const Image *image)
{
  if (image_info->temporary != MagickFalse)
    (void) RelinquishUniqueFileResource(image_info->filename);
  (void) RelinquishUniqueFileResource(image_info->unique);
  (void) RelinquishUniqueFileResource(image_info->filename);
  (void) RelinquishUniqueFileResource(image->filename);
}

static MagickBooleanType RunDelegateStep(const DelegateExecution *execution,
  MagickBooleanType *status)
{
  ImageInfo
    *const image_info = execution->image_info;

  Image
    *const image = execution->image;

  ExceptionInfo
    *const exception = execution->exception;

  char
    *command;

  (void) AcquireUniqueSymbolicLink(execution->output_filename,
    image_info->filename);
  if (AcquireUniqueFilename(image_info->unique) == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,
        "UnableToCreateTemporaryFile",image_info->unique);
      return(MagickFalse);
    }
  if (LinkDelegateInput(image,execution->input_filename,exception,status) ==
      MagickFalse)
    return(MagickFalse);
  *status=MagickTrue;
  command=InterpretDelegateProperties(image_info,image,execution->text,
    exception);
  ExecuteDelegateCommand(execution,status,&command);
  RestoreDelegateInput(image,execution->input_filename,exception,status);
  if (CopyDelegateOutput(image_info,execution->input_filename,
      execution->output_filename,exception) == MagickFalse)
    *status=MagickFalse;
  RelinquishDelegateFiles(image_info,image);
  return(MagickTrue);
}

static MagickBooleanType AcquireDelegateOutputFilename(ImageInfo *image_info,
  const Image *image,const MagickBooleanType temporary,
  ExceptionInfo *exception)
{
  if (*image_info->filename != '\0')
    return(MagickTrue);
  if (AcquireUniqueFilename(image_info->filename) == MagickFalse)
    {
      RelinquishTemporaryInput(image,temporary);
      ThrowFileException(exception,FileOpenError,
        "UnableToCreateTemporaryFile",image_info->filename);
      return(MagickFalse);
    }
  image_info->temporary=MagickTrue;
  return(MagickTrue);
}

static inline MagickBooleanType IsDelegateNameDenied(const char *name)
{
  return(((name != (const char *) NULL) &&
    (IsRightsAuthorized(DelegatePolicyDomain,ExecutePolicyRights,name) ==
     MagickFalse)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType AuthorizeDelegateNames(const char *decode,
  const char *encode,ExceptionInfo *exception)
{
  if (IsDelegateNameDenied(decode) != MagickFalse)
    ThrowPolicyException(decode,MagickFalse);
  if (IsDelegateNameDenied(encode) != MagickFalse)
    ThrowPolicyException(encode,MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType AcquireDelegateInputFilename(Image *image,
  const MagickBooleanType temporary,ExceptionInfo *exception)
{
  if ((temporary != MagickFalse) && (AcquireUniqueFilename(image->filename) ==
      MagickFalse))
    {
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static char **DestroyDelegateCommands(char **commands,ssize_t i)
{
  for ( ; commands[i] != (char *) NULL; i++)
    commands[i]=DestroyString(commands[i]);
  return((char **) RelinquishMagickMemory(commands));
}

MagickExport MagickBooleanType InvokeDelegate(ImageInfo *image_info,
  Image *image,const char *decode,const char *encode,ExceptionInfo *exception)
{
  char
    **commands,
    input_filename[MagickPathExtent],
    output_filename[MagickPathExtent];

  const DelegateInfo
    *delegate_info;

  MagickBooleanType
    status,
    temporary;

  ssize_t
    i;

  /*
    Get delegate.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (AuthorizeDelegateNames(decode,encode,exception) == MagickFalse)
    return(MagickFalse);
  temporary=(*image->filename == '\0') ? MagickTrue : MagickFalse;
  if (AcquireDelegateInputFilename(image,temporary,exception) == MagickFalse)
    return(MagickFalse);
  delegate_info=GetDelegateInfo(decode,encode,exception);
  if (delegate_info == (DelegateInfo *) NULL)
    {
      RelinquishTemporaryInput(image,temporary);
      (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
        "NoTagFound","`%s'",decode ? decode : encode);
      return(MagickFalse);
    }
  if (AcquireDelegateOutputFilename(image_info,image,temporary,exception) ==
      MagickFalse)
    return(MagickFalse);
  if (IsTwoStepDelegate(delegate_info,decode,encode) != MagickFalse)
    {
      const DelegateInvocation
        request = { image_info, image, decode, encode, exception,
          delegate_info, temporary };

      if (ConvertDelegateInput(&request) == MagickFalse)
        return(MagickFalse);
    }
  /*
    Invoke delegate.
  */
  commands=StringToList(delegate_info->commands);
  if (commands == (char **) NULL)
    {
      RelinquishTemporaryInput(image,temporary);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",
        decode ? decode : encode);
      return(MagickFalse);
    }
  status=MagickTrue;
  (void) CopyMagickString(output_filename,image_info->filename,
    MagickPathExtent);
  (void) CopyMagickString(input_filename,image->filename,MagickPathExtent);
  for (i=0; commands[i] != (char *) NULL; i++)
  {
    const DelegateExecution
      execution = { delegate_info, image_info, image, exception,
        input_filename, output_filename, commands[i] };

    if (RunDelegateStep(&execution,&status) == MagickFalse)
      break;
    if (status == MagickFalse)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
          "DelegateFailed","`%s'",commands[i]);
        break;
      }
    commands[i]=DestroyString(commands[i]);
  }
  (void) CopyMagickString(image_info->filename,output_filename,
    MagickPathExtent);
  (void) CopyMagickString(image->filename,input_filename,MagickPathExtent);
  /*
    Relinquish resources.
  */
  commands=DestroyDelegateCommands(commands,i);
  RelinquishTemporaryInput(image,temporary);
  return(status);
}
