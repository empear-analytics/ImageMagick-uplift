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

  MagickCore private methods of property.c: the helpers of
  InterpretImageProperties, from IsNonEmptyImage to the escape and percent
  expansion; included by that file only, where the run stood.
*/
#ifndef MAGICKCORE_PROPERTY_INTERPRET_PRIVATE_H
#define MAGICKCORE_PROPERTY_INTERPRET_PRIVATE_H

static inline MagickBooleanType IsNonEmptyImage(const Image *image)
{
  return((MagickBooleanType) ((image != (Image *) NULL) &&
    (image->columns != 0) && (image->rows != 0)));
}

static inline MagickBooleanType IsEscapeEnd(const char *p)
{
  return((MagickBooleanType) ((*p == '\0') || (*p == '\'') || (*p == '"')));
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

static inline MagickBooleanType IsEscapedPatternCharacter(const char *p)
{
  return((MagickBooleanType) ((*p == '\\') && (*(p+1) != '\0')));
}

static void ScanBracedPattern(char *pattern,ssize_t *offset,ssize_t *depth,
  const char **p)
{
  for ((*offset)=0; (*offset) < (MagickPathExtent-1L) && (*(*p) != '\0'); )
  {
    if (IsEscapedPatternCharacter(*p) != MagickFalse)
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
  const FxChannelPosition
    red = { RedPixelChannel, 0, 0 },
    green = { GreenPixelChannel, 0, 0 },
    blue = { BluePixelChannel, 0, 0 },
    black = { BlackPixelChannel, 0, 0 },
    alpha = { AlphaPixelChannel, 0, 0 };

  double
    value;

  MagickStatusType
    fx_status;

  fx_status=FxEvaluateChannelExpression(fx_info,&red,&value,exception);
  pixel->red=(double) QuantumRange*value;
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,&green,
    &value,exception);
  pixel->green=(double) QuantumRange*value;
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,&blue,
    &value,exception);
  pixel->blue=(double) QuantumRange*value;
  if (image->colorspace == CMYKColorspace)
    {
      fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,
        &black,&value,exception);
      pixel->black=(double) QuantumRange*value;
    }
  fx_status&=(MagickStatusType) FxEvaluateChannelExpression(fx_info,&alpha,
    &value,exception);
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
  const FxChannelPosition
    composite = { CompositePixelChannel, 0, 0 };

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
  fx_status=FxEvaluateChannelExpression(fx_info,&composite,&value,exception);
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

static void ApplyPixelCompliance(Image *property_image,const PixelInfo *pixel,
  char *name,ExceptionInfo *exception)
{
  const char
    *string;

  string=GetImageArtifact(property_image,"pixel:compliance");
  if (string != (char *) NULL)
    {
      ComplianceType compliance=(ComplianceType) ParseCommandOption(
        MagickComplianceOptions,MagickFalse,string);
      (void) QueryColorname(property_image,pixel,compliance,name,
        exception);
    }
}

static MagickBooleanType AppendPixelValue(const InterpretText *text,
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
      ApplyPixelCompliance(property_image,&pixel,name,exception);
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

static void ClearMagickPropertyHints(Image *property_image,
  ImageInfo *property_info)
{
  (void) DeleteImageArtifact(property_image,"magick-property");
  (void) DeleteImageOption(property_info,"magick-property");
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
      ClearMagickPropertyHints(property_image,property_info);
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

typedef struct _InterpretLetterInputs
{
  ImageInfo
    *property_info;

  Image
    *image,
    *property_image;

  ExceptionInfo
    *exception;

  MagickBooleanType
    number;
} InterpretLetterInputs;

static MagickBooleanType InterpretLetterEscape(
  const InterpretLetterInputs *inputs,const InterpretText *text,
  const char **p)
{
  ImageInfo
    *const property_info = inputs->property_info;

  Image
    *const image = inputs->image,
    *const property_image = inputs->property_image;

  ExceptionInfo
    *const exception = inputs->exception;

  const MagickBooleanType
    number = inputs->number;

  char
    **const q = text->q;

  const char
    *string;

  if (number != MagickFalse)
    {
      /*
        But only if not preceded by a number!
      */
      *(*q)++='%'; /* do NOT substitute the percent */
      (*p)--;      /* back up one */
      return(MagickTrue);
    }
  string=GetMagickPropertyLetter(property_info,image,*(*p),exception);
  if (string != (char *) NULL)
    {
      if (AppendInterpretString(text,(string)) == MagickFalse)
        return(MagickFalse);
      (void) DeleteImageArtifact(property_image,"magick-property");
      (void) DeleteImageOption(property_info,"magick-property");
      return(MagickTrue);
    }
  (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
    "UnknownImageProperty","\"%%%c\"",*(*p));
  return(MagickTrue);
}

static MagickBooleanType InterpretEscapeCharacter(
  const InterpretLetterInputs *inputs,const InterpretText *text,
  const char **p)
{
  char
    **const q = text->q;

  /*
    Look for the various escapes, (and handle other specials)
  */
  *(*q)='\0';
  if (ExtendInterpretBuffer(text,(MagickPathExtent)) == MagickFalse)
    return(MagickFalse);
  if (InterpretCharacter(q,p) != MagickFalse)
    return(MagickTrue);
  (*p)++; /* advance beyond the percent */
  /*
    Doubled Percent - or percent at end of string.
  */
  if (IsEscapeEnd(*p) != MagickFalse)
    (*p)--;
  if (*(*p) == '%')
    {
      *(*q)++='%';
      return(MagickTrue);
    }
  /*
    Single letter escapes %c.
  */
  if (*(*p) != '[')
    return(InterpretLetterEscape(inputs,text,p));
  {
    const InterpretSources
      sources = { inputs->property_info, inputs->property_image,
        inputs->exception };

    return(InterpretBracedEscape(&sources,text,p));
  }
}

static inline MagickBooleanType IsLoggedInterpretImage(const Image *image)
{
  return(((image != (Image *) NULL) && (IsEventLogging() != MagickFalse)) ?
    MagickTrue : MagickFalse);
}

static void LogInterpretImageProperties(const ImageInfo *image_info,
  const Image *image)
{
  if (IsLoggedInterpretImage(image) != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
    if ((image_info != (ImageInfo *) NULL) && (IsEventLogging() != MagickFalse))
      (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
        image_info->filename);
    else
      if (IsEventLogging() != MagickFalse)
        (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no image");
}

static void DestroyInterpretSources(const ImageInfo *image_info,
  const Image *image,ImageInfo *property_info,Image *property_image)
{
  if ((property_image != image) && (property_image != (Image *) NULL))
    property_image=DestroyImage(property_image);
  if ((property_info != image_info) && (property_info != (ImageInfo *) NULL))
    property_info=DestroyImageInfo(property_info);
}

static MagickBooleanType AcquireInterpretImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception,Image **property_image)
{
  if (IsNonEmptyImage(image) != MagickFalse)
    (*property_image)=image;
  else
    {
      (*property_image)=AcquireImage(image_info,exception);
      if ((*property_image) == (Image *) NULL)
        return(MagickFalse);
      (void) SetImageExtent((*property_image),1,1,exception);
      (void) SetImageBackgroundColor((*property_image),exception);
    }
  return(MagickTrue);
}

static const char *SkipInterpretSpaces(const char *embed_text)
{
  const char
    *p;

  p=embed_text;
  while ((isspace((int) ((unsigned char) *p)) != 0) && (*p != '\0'))
    p++;
  return(p);
}

static char *FinishInterpretText(const MagickBooleanType status,
  char *interpret_text)
{
  if (status == MagickFalse)
    {
      if (interpret_text != (char *) NULL)
        interpret_text=DestroyString(interpret_text);
      return((char *) NULL);
    }
  return(interpret_text);
}

typedef struct _InterpretRequest
{
  ImageInfo
    *image_info;

  Image
    *image;

  const char
    *embed_text;

  ImageInfo
    *property_info;

  ExceptionInfo
    *exception;
} InterpretRequest;

static MagickBooleanType InterpretEmbeddedText(const InterpretRequest *request,
  Image **property_image,const InterpretText *text,const char **p)
{
  ImageInfo
    *const image_info = request->image_info;

  Image
    *const image = request->image;

  const char
    *const embed_text = request->embed_text;

  ImageInfo
    *const property_info = request->property_info;

  ExceptionInfo
    *const exception = request->exception;

  char
    **const interpret_text = text->interpret_text,
    **const q = text->q;

  size_t
    *const extent = text->extent;

  MagickBooleanType
    number;

  if (AcquireInterpretImage(image_info,image,exception,property_image) ==
      MagickFalse)
    return(MagickFalse);
  (*interpret_text)=AcquireString(embed_text); /* new string with extra space */
  if ((*interpret_text) == (char *) NULL)
    return(MagickFalse);
  (*extent)=MagickPathExtent;                  /* allocated space in string */
  number=MagickFalse;                       /* is last char a number? */
  for ((*q)=(*interpret_text); *(*p)!='\0'; number=isdigit((int) ((unsigned char) *(*p))) ? MagickTrue : MagickFalse,(*p)++)
  {
    InterpretLetterInputs
      inputs;

    inputs.property_info=property_info;
    inputs.image=image;
    inputs.property_image=(*property_image);
    inputs.exception=exception;
    inputs.number=number;
    if (InterpretEscapeCharacter(&inputs,text,p) == MagickFalse)
      return(MagickFalse);
  }
  *(*q)='\0';
  return(MagickTrue);
}

#endif
