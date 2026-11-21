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

  MagickCore private image filename methods.
*/
#ifndef MAGICKCORE_IMAGE_FILENAME_PRIVATE_H
#define MAGICKCORE_IMAGE_FILENAME_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsUnknownFormatSpecifier(const char specifier)
{
  return((MagickBooleanType) ((specifier != 'd') && (specifier != 'x') &&
    (specifier != 'o')));
}

static inline MagickBooleanType AreDigitsFrom(const char *start,
  const size_t length,size_t i)
{
  for ( ; i < (length-1); i++)
    if (isdigit((int) ((unsigned char) start[i])) == 0)
      return(MagickFalse);
  return(MagickTrue);
}

static inline MagickBooleanType IsValidDigitSpecifier(const char *start,
  const size_t length)
{
  size_t
    i = 0;

  if (*start == '0')
    {
      if ((length >= 3) && (start[1] == '0'))
        return(MagickFalse);
      i=1;
    }
  return(AreDigitsFrom(start,length,i));
}

static inline MagickBooleanType IsValidFormatSpecifier(const char *start,
  const char *end)
{
  char
    specifier = end[-1];

  size_t
    length = (size_t) (end-start);

  /*
    Is this a valid format specifier?
  */
  if (IsUnknownFormatSpecifier(specifier) != MagickFalse)
    return(MagickFalse);
  if ((length == 1) && (*start == specifier))
    return(MagickTrue);
  if (length >= 2)
    return(IsValidDigitSpecifier(start,length));
  return(MagickFalse);
}

static inline MagickBooleanType IsIntegerSpecifierAt(const char *cursor)
{
  return((MagickBooleanType) ((*cursor == 'd') || (*cursor == 'o') ||
    (*cursor == 'x')));
}

static const char *GetFilenameOption(const ImageInfo *image_info,
  Image *image,const char *pattern,ExceptionInfo *exception)
{
  const char
    *option = (const char *) NULL;

  if (image != NULL)
    {
      option=GetImageProperty(image,pattern,exception);
      if (option == (const char *) NULL)
        option=GetImageArtifact(image,pattern);
    }
  if ((option == (const char *) NULL) &&
      (image_info != (ImageInfo *) NULL))
    option=GetImageOption(image_info,pattern);
  return(option);
}

typedef struct _FilenameExpansion
{
  const char
    *start;

  char
    *filename;

  const ImageInfo
    *image_info;

  Image
    *image;

  ExceptionInfo
    *exception;

  const char
    *specifier_start;

  int
    value;
} FilenameExpansion;

typedef struct _FilenameTargets
{
  char
    **p;

  const char
    **cursor;
} FilenameTargets;

static MagickBooleanType ExpandFilenameOption(
  const FilenameExpansion *expansion,const FilenameTargets *targets)
{
  char
    *const filename = expansion->filename;

  const ImageInfo
    *const image_info = expansion->image_info;

  Image
    *const image = expansion->image;

  ExceptionInfo
    *const exception = expansion->exception;

  char
    **const p = targets->p;

  const char
    **const cursor = targets->cursor;

  char
    pattern[MagickPathExtent];

  const char
    *end = strchr((*cursor),']'),
    *option = (const char *) NULL;

  size_t
    extent,
    option_length;

  if (end == (const char *) NULL)
    return(MagickTrue);
  extent=(size_t) (end-(*cursor)-1);
  if (extent >= sizeof(pattern))
    return(MagickTrue);
  (void) CopyMagickString(pattern,(*cursor)+1,extent+1);
  pattern[extent]='\0';
  option=GetFilenameOption(image_info,image,pattern,exception);
  if (option == (const char *) NULL)
    return(MagickTrue);
  option_length=strlen(option);
  if (((*p)-filename+(ssize_t) option_length) >= MagickPathExtent)
    return(MagickFalse);
  (void) CopyMagickString((*p),option,(size_t) (MagickPathExtent-
    ((*p)-filename)));
  (*p)+=option_length;
  (*cursor)=end+1;
  return(MagickTrue);
}

static MagickBooleanType ExpandFilenameTail(
  const FilenameExpansion *expansion,const FilenameTargets *targets)
{
  const char
    *const start = expansion->start;

  char
    *const filename = expansion->filename;

  char
    **const p = targets->p;

  const char
    **const cursor = targets->cursor;

  if (*(*cursor) == '[')
    return(ExpandFilenameOption(expansion,targets));
  /*
    Invalid or unsupported specifier — treat as literal.
  */
  (*cursor)=start;
  if (((*p)-filename+1) >= MagickPathExtent)
    return(MagickFalse);
  *(*p)++=(*(*cursor)++);
  return(MagickTrue);
}

static MagickBooleanType FormatFilenameSpecifier(
  const FilenameExpansion *expansion,const FilenameTargets *targets)
{
  char
    *const filename = expansion->filename;

  const char
    *const specifier_start = expansion->specifier_start;

  const int
    value = expansion->value;

  char
    **const p = targets->p;

  const char
    **const cursor = targets->cursor;

  char
    pattern[MagickPathExtent];

  char
    format_specifier[MagickPathExtent];

  size_t
    length = (size_t) ((*cursor)-specifier_start),
    pattern_length;

  ssize_t
    count;

  (void) snprintf(format_specifier,sizeof(format_specifier),
    "%%%.*s%c",(int) length,specifier_start,*(*cursor));
  count=FormatLocaleString(pattern,sizeof(pattern),format_specifier,
    value);
  pattern_length=strlen(pattern);
  if ((count <= 0) || ((size_t) count != pattern_length))
    return(MagickFalse);
  if (((*p)-filename+(ssize_t) pattern_length) >= MagickPathExtent)
    return(MagickFalse);
  (void) CopyMagickString((*p),pattern,(size_t) (MagickPathExtent-
    ((*p)-filename)));
  (*p)+=pattern_length;
  (*cursor)++;
  return(MagickTrue);
}

static MagickBooleanType ExpandFilenameSpecifier(
  const FilenameExpansion *expansion,const FilenameTargets *targets)
{
  const char
    *const start = expansion->start;

  const char
    *const specifier_start = expansion->specifier_start;

  char
    **const p = targets->p;

  const char
    **const cursor = targets->cursor;

  if (IsIntegerSpecifierAt((*cursor)) != MagickFalse)
    {
      const char
        *specifier_end = (*cursor)+1;

      if (IsValidFormatSpecifier(specifier_start,specifier_end) != MagickFalse)
        return(FormatFilenameSpecifier(expansion,targets));
      else
        {
          /*
            Invalid specifier — treat as literal.
          */
          (*cursor)=start;
          *(*p)++=(*(*cursor)++);
          return(MagickTrue);
        }
    }
  if (ExpandFilenameTail(expansion,targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType CopyLiteralFilename(const char *format,
  char *filename,ExceptionInfo *exception,size_t *result)
{
  char
    *literal;

  literal=(char *) GetImageRegistry(StringRegistryType,"filename:literal",
    exception);
  if (IsStringTrue(literal) != MagickFalse)
    {
      if (literal != (char *) NULL)
        literal=DestroyString(literal);
      (void) CopyMagickString(filename,format,MagickPathExtent);
      (*result)=strlen(filename);
      return(MagickTrue);
    }
  if (literal != (char *) NULL)
    literal=DestroyString(literal);
  return(MagickFalse);
}

static MagickBooleanType CopyFilenameCharacter(char **p,
  const char **cursor,const char **start)
{
  if (*(*cursor) != '%')
    {
      *(*p)++=(*(*cursor)++);
      return(MagickTrue);
    }
  (*start)=(*cursor)++;  /* Skip '%' */
  if (*(*cursor) == '%')
    {
      *(*p)++='%';
      (*cursor)++;
      return(MagickTrue);
    }
  return(MagickFalse);
}

static const char *SkipFilenameDigits(const char *cursor)
{
  while (isdigit((int) ((unsigned char) *cursor)) != 0)
    cursor++;
  return(cursor);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
