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

  MagickCore private delegate invocation methods.
*/
#ifndef MAGICKCORE_DELEGATE_INVOKE_PRIVATE_H
#define MAGICKCORE_DELEGATE_INVOKE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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

static void OpenInterpretDelegateProperties(const ImageInfo *image_info,
  const Image *image)
{
  assert(image == NULL || image->signature == MagickCoreSignature);
  assert(image_info == NULL || image_info->signature == MagickCoreSignature);
  if ((image != (Image *) NULL) && (IsEventLogging() != MagickFalse))
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  else
   if ((image_info != (ImageInfo *) NULL) && (IsEventLogging() != MagickFalse))
     (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s","no-image");
}

static MagickBooleanType DecodeDelegateSequence(const char **p,char **q,
  const MagickBooleanType number)
{
  if (DecodeDelegateCharacter(p,q) != MagickFalse)
    return(MagickTrue);
  return(DecodeDelegatePercent(p,q,number));
}

static MagickBooleanType IsDelegateDigit(const char *p)
{
  return(isdigit((int) ((unsigned char) *p)) ? MagickTrue : MagickFalse);
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

  OpenInterpretDelegateProperties(image_info,image);
  if (IsEmptyDelegateText(embed_text,&p,&returned) != MagickFalse)
    return(returned);
  /*
    Translate any embedded format characters.
  */
  interpret_text=AcquireString(embed_text);  /* new string with extra space */
  extent=MagickPathExtent;  /* allocated space in string */
  number=MagickFalse;  /* is last char a number? */
  for (q=interpret_text; *p!='\0';
    number=IsDelegateDigit(p),p++)
  {
    /*
      Interpret escape characters (e.g. Filename: %M).
    */
    *q='\0';
    ExtendInterpretText(MagickPathExtent);
    if (DecodeDelegateSequence(&p,&q,number) != MagickFalse)
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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
