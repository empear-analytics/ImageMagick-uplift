/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private delegate property letter and escape methods.
*/
#ifndef MAGICKCORE_DELEGATE_PROPERTY_PRIVATE_H
#define MAGICKCORE_DELEGATE_PROPERTY_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsImageInfoLetter(const char letter)
{
  return((MagickBooleanType) ((letter != '\0') && (strchr("aouS",letter)
    != (char *) NULL)));
}

static inline MagickBooleanType IsImageLetter(const char letter)
{
  return((MagickBooleanType) ((letter != '\0') &&
    (strchr("bdefghimpqrstwxyzACDFGHIMOPQTUWXY~@#",letter) != (char *) NULL)));
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

static void FormatClassColorspaceAlpha(const Image *image,char *value)
{
  ColorspaceType
    colorspace;

  colorspace=image->colorspace;
  (void) FormatLocaleString(value,MagickPathExtent,"%s %s %s",
    CommandOptionToMnemonic(MagickClassOptions,(ssize_t)
    image->storage_class),CommandOptionToMnemonic(MagickColorspaceOptions,
    (ssize_t) colorspace),image->alpha_trait != UndefinedPixelTrait ?
    "Alpha" : "");
}

static const char *GetMnemonicProperty(Image *image,const char letter,
  char *value,ExceptionInfo *exception)
{
  switch (letter)
  {
    case 'r': /* Image storage class, colorspace, and alpha enabled.  */
    {
      FormatClassColorspaceAlpha(image,value);
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

static void OpenMagickPropertyLetter(const ImageInfo *image_info,
  const Image *image)
{
  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
    if ((image_info != (ImageInfo *) NULL) && (IsEventLogging() != MagickFalse))
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no-images");
}

static char *GetMagickPropertyLetter(ImageInfo *image_info,Image *image,
  const char letter,ExceptionInfo *exception)
{
  char
    value[MagickPathExtent];

  const char
    *string;

  OpenMagickPropertyLetter(image_info,image);
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
  return((MagickBooleanType) ((*p == '\0') || (*p == '\'') || (*p == '"')));
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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
