/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS   TTTTT  RRRR   IIIII  N   N   GGGG                  %
%                  SS        T    R   R    I    NN  N  G                      %
%                   SSS      T    RRRR     I    N N N  G GGG                  %
%                     SS     T    R R      I    N  NN  G   G                  %
%                  SSSSS     T    R  R   IIIII  N   N   GGGG                  %
%                                                                             %
%                                                                             %
%                        MagickCore String Methods                            %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               August 2003                                   %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the license.  You may  %
%  obtain a copy of the license at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  unless required by applicable law or agreed to in writing, software        %
%  distributed under the license is distributed on an "as is" basis,          %
%  without warranties or conditions of any kind, either express or implied.   %
%  See the license for the specific language governing permissions and        %
%  limitations under the license.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/property.h"
#include "MagickCore/policy.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resource-private.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/utility-private.h"

/*
  Define declarations.
*/
#define CharsPerLine  0x14

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e S t r i n g                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireString() returns an new extended string, containing a clone of the
%  given string.
%
%  An extended string is the string length, plus an extra MagickPathExtent space
%  to allow for the string to be actively worked on.
%
%  The returned string should be freed using DestroyString().
%
%  The format of the AcquireString method is:
%
%      char *AcquireString(const char *source)
%
%  A description of each parameter follows:
%
%    o source: A character string.
%
*/
MagickExport char *AcquireString(const char *source)
{
  char
    *destination;

  size_t
    length;

  length=0;
  if (source != (char *) NULL)
    length+=strlen(source);
  if (~length < MagickPathExtent)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
  destination=(char *) AcquireQuantumMemory(length+MagickPathExtent,
    sizeof(*destination));
  if (destination == (char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
  if (source != (char *) NULL)
    (void) memcpy(destination,source,length*sizeof(*destination));
  destination[length]='\0';
  return(destination);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e S t r i n g                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneString() replaces or frees the destination string to make it
%  a clone of the input string plus MagickPathExtent more space so the string
%  may be worked on.
%
%  If source is a NULL pointer the destination string will be freed and set to
%  a NULL pointer.  A pointer to the stored in the destination is also returned.
%
%  When finished the non-NULL string should be freed using DestroyString()
%  or using CloneString() with a NULL pointed for the source.
%
%  The format of the CloneString method is:
%
%      char *CloneString(char **destination,const char *source)
%
%  A description of each parameter follows:
%
%    o destination:  A pointer to a character string.
%
%    o source: A character string.
%
*/
MagickExport char *CloneString(char **destination,const char *source)
{
  size_t
    length;

  assert(destination != (char **) NULL);
  if (source == (const char *) NULL)
    {
      if (*destination != (char *) NULL)
        *destination=DestroyString(*destination);
      return(*destination);
    }
  if (*destination == (char *) NULL)
    {
      *destination=AcquireString(source);
      return(*destination);
    }
  length=strlen(source);
  if (~length < MagickPathExtent)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
  *destination=(char *) ResizeQuantumMemory(*destination,length+
    MagickPathExtent,sizeof(**destination));
  if (*destination == (char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
  if (length != 0)
    (void) memcpy(*destination,source,length*sizeof(**destination));
  (*destination)[length]='\0';
  return(*destination);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n c a t e n a t e S t r i n g                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConcatenateString() appends a copy of string source, including the
%  terminating null character, to the end of string destination.
%
%  The format of the ConcatenateString method is:
%
%      MagickBooleanType ConcatenateString(char **magick_restrict destination,
%        const char *magick_restrict source)
%
%  A description of each parameter follows:
%
%    o destination:  A pointer to a character string.
%
%    o source: A character string.
%
*/
MagickExport MagickBooleanType ConcatenateString(
  char **magick_restrict destination,const char *magick_restrict source)
{
  size_t
    destination_length,
    length,
    source_length;

  assert(destination != (char **) NULL);
  if (source == (const char *) NULL)
    return(MagickTrue);
  if (*destination == (char *) NULL)
    {
      *destination=AcquireString(source);
      return(MagickTrue);
    }
  destination_length=strlen(*destination);
  source_length=strlen(source);
  length=destination_length;
  if (~length < source_length)
    ThrowFatalException(ResourceLimitFatalError,"UnableToConcatenateString");
  length+=source_length;
  if (~length < MagickPathExtent)
    ThrowFatalException(ResourceLimitFatalError,"UnableToConcatenateString");
  *destination=(char *) ResizeQuantumMemory(*destination,
    OverAllocateMemory(length+MagickPathExtent),sizeof(**destination));
  if (*destination == (char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToConcatenateString");
  if (source_length != 0)
    (void) memcpy((*destination)+destination_length,source,source_length);
  (*destination)[length]='\0';
  return(MagickTrue);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n s t a n t S t r i n g                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConstantString() allocates exactly the needed memory for a string and
%  copies the source string to that memory location.  A NULL string pointer
%  will allocate an empty string containing just the NUL character.
%
%  When finished the string should be freed using DestroyString()
%
%  The format of the ConstantString method is:
%
%      char *ConstantString(const char *source)
%
%  A description of each parameter follows:
%
%    o source: A character string.
%
*/
MagickExport char *ConstantString(const char *source)
{
  char
    *destination;

  size_t
    length;

  length=0;
  if (source != (char *) NULL)
    length+=strlen(source);
  destination=(char *) NULL;
  if (~length >= 1UL)
    destination=(char *) AcquireQuantumMemory(length+1UL,sizeof(*destination));
  if (destination == (char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
  if (source != (char *) NULL)
    (void) memcpy(destination,source,length*sizeof(*destination));
  destination[length]='\0';
  return(destination);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S t r i n g                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyString() destroys memory associated with a string.
%
%  The format of the DestroyString method is:
%
%      char *DestroyString(char *string)
%
%  A description of each parameter follows:
%
%    o string: the string.
%
*/
MagickExport char *DestroyString(char *string)
{
  return((char *) RelinquishMagickMemory(string));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S t r i n g L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyStringList() zeros memory associated with a string list.
%
%  The format of the DestroyStringList method is:
%
%      char **DestroyStringList(char **list)
%
%  A description of each parameter follows:
%
%    o list: the string list.
%
*/
MagickExport char **DestroyStringList(char **list)
{
  ssize_t
    i;

  assert(list != (char **) NULL);
  for (i=0; list[i] != (char *) NULL; i++)
    list[i]=DestroyString(list[i]);
  list=(char **) RelinquishMagickMemory(list);
  return(list);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E s c a p e S t r i n g                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EscapeString() allocates memory for a backslash-escaped version of a
%  source text string, copies the escaped version of the text to that
%  memory location while adding backslash characters, and returns the
%  escaped string.
%
%  The format of the EscapeString method is:
%
%      char *EscapeString(const char *source,const char escape)
%
%  A description of each parameter follows:
%
%    o allocate_string:  Method EscapeString returns the escaped string.
%
%    o source: A character string.
%
%    o escape: the quoted string termination character to escape (e.g. '"').
%
*/
static inline MagickBooleanType NeedsEscape(const char c,const char escape)
{
  return((c == '\\') || (c == escape) ? MagickTrue : MagickFalse);
}

static void CopyEscapedString(char *destination,const char *source,
  const char escape)
{
  char
    *q;

  const char
    *p;

  q=destination;
  for (p=source; *p != '\0'; p++)
  {
    if (NeedsEscape(*p,escape) != MagickFalse)
      *q++='\\';
    *q++=(*p);
  }
  *q='\0';
}

MagickExport char *EscapeString(const char *source,const char escape)
{
  char
    *destination;

  const char
    *p;

  size_t
    length;

  assert(source != (const char *) NULL);
  length=0;
  for (p=source; *p != '\0'; p++)
  {
    if (NeedsEscape(*p,escape) != MagickFalse)
      {
        if (~length < 1)
          ThrowFatalException(ResourceLimitFatalError,"UnableToEscapeString");
        length++;
      }
    length++;
  }
  destination=(char *) NULL;
  if (~length >= (MagickPathExtent-1))
    destination=(char *) AcquireQuantumMemory(length+MagickPathExtent,
      sizeof(*destination));
  if (destination == (char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToEscapeString");
  *destination='\0';
  CopyEscapedString(destination,source,escape);
  return(destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e T o S t r i n g                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToString() returns the contents of a file as a string.
%
%  The format of the FileToString method is:
%
%      char *FileToString(const char *filename,const size_t extent,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: the filename.
%
%    o extent: Maximum length of the string.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport char *FileToString(const char *filename,const size_t extent,
  ExceptionInfo *exception)
{
  const char
    *p;

  size_t
    length;

  assert(filename != (const char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  p=filename;
  if ((*filename == '@') && (strlen(filename) > 1))
    {
      MagickBooleanType
        status;

      status=IsRightsAuthorized(PathPolicyDomain,ReadPolicyRights,filename);
      if (status == MagickFalse)
        ThrowPolicyException(filename,(char *) NULL);
      p=filename+1;
    }
  return((char *) FileToBlob(p,extent,&length,exception));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t M a g i c k S i z e                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatMagickSize() converts a size to a human readable format, for example,
%  14k, 234m, 2.7g, or 3.0t.  Scaling is done by repetitively dividing by
%  1000.
%
%  The format of the FormatMagickSize method is:
%
%      ssize_t FormatMagickSize(const MagickSizeType size,const char *suffix,
%        const size_t length,char *format)
%
%  A description of each parameter follows:
%
%    o size:  convert this size to a human readable format.
%
%    o bi:  use power of two rather than power of ten.
%
%    o suffix:  append suffix, typically B or P.
%
%    o length: the maximum length of the string.
%
%    o format:  human readable format.
%
*/
MagickExport ssize_t FormatMagickSize(const MagickSizeType size,
  const MagickBooleanType bi,const char *suffix,const size_t length,
  char *format)
{
  const char
    **units;

  double
    bytes,
    extent;

  ssize_t
    i;

  ssize_t
    count;

  static const char
    *bi_units[] =
    {
      "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi", "Ri", "Qi", (char *) NULL
    },
    *traditional_units[] =
    {
      "", "K", "M", "G", "T", "P", "E", "Z", "Y", "R", "Q", (char *) NULL
    };

  bytes=1000.0;
  units=traditional_units;
  if (bi != MagickFalse)
    {
      bytes=1024.0;
      units=bi_units;
    }
  extent=(double) size;
  (void) FormatLocaleString(format,MagickFormatExtent,"%.*g",
    GetMagickPrecision(),extent);
  if (strstr(format,"e+") == (char *) NULL)
    {
      if (suffix == (const char *) NULL)
        count=FormatLocaleString(format,length,"%.17g%s",extent,units[0]);
      else
        count=FormatLocaleString(format,length,"%.17g%s%s",extent,units[0],
          suffix);
      return(count);
    }
  for (i=0; (extent >= bytes) && (units[i+1] != (const char *) NULL); i++)
    extent/=bytes;
  if (suffix == (const char *) NULL)
    count=FormatLocaleString(format,length,"%.*g%s",GetMagickPrecision(),
      extent,units[i]);
  else
    count=FormatLocaleString(format,length,"%.*g%s%s",GetMagickPrecision(),
      extent,units[i],suffix);
  return(count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t E n v i r o n m e n t V a l u e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetEnvironmentValue() returns the environment string that matches the
%  specified name.
%
%  The format of the GetEnvironmentValue method is:
%
%      char *GetEnvironmentValue(const char *name)
%
%  A description of each parameter follows:
%
%    o name: the environment name.
%
*/
MagickExport char *GetEnvironmentValue(const char *name)
{
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
  return(NTGetEnvironmentValue(name));
#else
  const char
    *environment;

  environment=getenv(name);
  if (environment == (const char *) NULL)
    return((char *) NULL);
  return(ConstantString(environment));
#endif
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s S t r i n g T r u e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsStringTrue() returns MagickTrue if the value is "true", "on", "yes" or
%  "1". Any other string or undefined returns MagickFalse.
%
%  Typically this is used to look at strings (options or artifacts) which
%  has a default value of "false", when not defined.
%
%  The format of the IsStringTrue method is:
%
%      MagickBooleanType IsStringTrue(const char *value)
%
%  A description of each parameter follows:
%
%    o value: Specifies a pointer to a character array.
%
*/
MagickExport MagickBooleanType IsStringTrue(const char *value)
{
  if (value == (const char *) NULL)
    return(MagickFalse);
  if (LocaleCompare(value,"true") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"on") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"yes") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"1") == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s S t r i n g F a l s e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsStringFalse() returns MagickTrue if the value is "false", "off", "no" or
%  "0". Any other string or undefined returns MagickFalse.
%
%  Typically this is used to look at strings (options or artifacts) which
%  has a default value of "true", when it has not been defined.
%
%  The format of the IsStringFalse method is:
%
%      MagickBooleanType IsStringFalse(const char *value)
%
%  A description of each parameter follows:
%
%    o value: Specifies a pointer to a character array.
%
*/
MagickExport MagickBooleanType IsStringFalse(const char *value)
{
  if (value == (const char *) NULL)
    return(MagickFalse);
  if (LocaleCompare(value,"false") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"off") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"no") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"0") == 0)
    return(MagickTrue);
  return(MagickFalse);
}

static void PadStringInfoHexLine(FILE *file,size_t j)
{
  /*
    Padding.
  */
  for ( ; j < CharsPerLine; j++)
  {
    (void) fputc(' ',file);
    (void) fputc(' ',file);
    if (((j+1) % 0x04) == 0)
      (void) fputc(' ',file);
  }
}

static void PrintStringInfoAsciiLine(FILE *file,
  const StringInfo *string_info,const unsigned char *p,const size_t i)
{
  size_t
    j;

  /*
    ASCII section.
  */
  for (j=0; j < MagickMin(string_info->length-i,CharsPerLine); j++)
  {
    unsigned char c = p[i+j];
    if (isprint((int) c) != 0)
      (void) fputc(c,file);
    else
      (void) fputc('-',file);
  }
}

static void PrintStringInfoHexLine(FILE *file,
  const StringInfo *string_info,const unsigned char *p,const size_t i)
{
  size_t
    j;

  (void) FormatLocaleFile(file,"0x%08lx: ",(unsigned long) i);
  for (j = 0; j < MagickMin(string_info->length-i, CharsPerLine); j++)
  {
    (void) FormatLocaleFile(file,"%02lx",(unsigned long) (p[i+j]) & 0xff);
    if (((j+1) % 0x04) == 0)
      (void) fputc(' ',file);
  }
  PadStringInfoHexLine(file,j);
  (void) fputc(' ',file);
  PrintStringInfoAsciiLine(file,string_info,p,i);
  (void) fputc('\n',file);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P r i n t S t r i n g I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PrintStringInfo() prints the string.
%
%  The format of the PrintStringInfo method is:
%
%      void PrintStringInfo(FILE *file,const char *id,
%        const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o file: the file, typically stdout.
%
%    o id: the string id.
%
%    o string_info: the string info.
%
*/
static size_t FindUnprintableChar(const StringInfo *string_info,
  const unsigned char *p)
{
  size_t
    i;

  for (i=0; i < string_info->length; i++)
    if ((p[i] < 32) && (isspace((int)p[i]) == 0))
      break;
  return(i);
}

MagickExport void PrintStringInfo(FILE *file,const char *id,
  const StringInfo *string_info)
{
  const unsigned char
    *p;

  size_t
    i;

  /*
    Check if string is printable.
  */
  assert(id != (const char *) NULL);
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  p=(const unsigned char *) string_info->datum;
  i=FindUnprintableChar(string_info,p);
  (void) FormatLocaleFile(file,"%s(%.17g):\n",id,(double) string_info->length);
  if (i == string_info->length)
   {
     for (i = 0; i < string_info->length; i++)
       (void) fputc(p[i],file);
     (void) fputc('\n',file);
     return;
   }
  /*
    Convert string to a HEX list.
  */
  for (i=0; i < string_info->length; i+=CharsPerLine)
    PrintStringInfoHexLine(file,string_info,p,i);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S a n t i z e S t r i n g                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SanitizeString() returns a new string with all characters removed except
%  letters, digits and !#$%&'*+-=?^_`{|}~@.[].
%
%  Free the sanitized string with DestroyString().
%
%  The format of the SanitizeString method is:
%
%      char *SanitizeString(const char *source)
%
%  A description of each parameter follows:
%
%    o source: A character string.
%
*/
MagickExport char *SanitizeString(const char *source)
{
  char
    *sanitize_source;

  const char
    *q;

  char
    *p;

  static char
    allowlist[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 "
      "$-_.+!*'(),{}|\\^~[]`\"><#%;/?:@&=";

  sanitize_source=AcquireString(source);
  p=sanitize_source;
  q=sanitize_source+strlen(sanitize_source);
  for (p+=strspn(p,allowlist); p != q; p+=(ptrdiff_t) strspn(p,allowlist))
    *p='_';
  return(sanitize_source);
}



static const char *SkipValueSeparator(const char *p)
{
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;  /* skip spaces */
  if (*p == ',')
    p++;  /* skip comma */
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;  /* and more spaces */
  return(p);
}

static ssize_t CountDoubleValues(const char *string)
{
  char
    *q;

  const char
    *p;

  ssize_t
    count;

  count=0;
  p=string;
  while (*p != '\0')
  {
    (void) StringToDouble(p,&q);  /* get value - ignores leading space */
    if (p == q)
      return(-1);  /* no value found */
    p=q;
    count++;  /* increment value count */
    p=SkipValueSeparator(p);
  }
  return(count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i n g T o A r r a y O f D o u b l e s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToArrayOfDoubles() converts a string of space or comma separated
%  numbers into array of floating point numbers (doubles). Any number that
%  fails to parse properly will produce a syntax error. As will two commas
%  without a  number between them.  However a final comma at the end will
%  not be regarded as an error so as to simplify automatic list generation.
%
%  A NULL value is returned on syntax or memory errors.
%
%  Use RelinquishMagickMemory() to free returned array when finished.
%
%  The format of the StringToArrayOfDoubles method is:
%
%     double *StringToArrayOfDoubles(const char *string,size_t *count,
%       ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o string: the string containing the comma/space separated values.
%
%    o count: returns number of arguments in returned array
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport double *StringToArrayOfDoubles(const char *string,ssize_t *count,
  ExceptionInfo *exception)
{
  char
    *q;

  const char
    *p;

  double
    *array;

  ssize_t
    i;

  /*
    Determine count of values, and check syntax.
  */
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  *count=0;
  if (string == (char *) NULL)
    return((double *) NULL);  /* no value found */
  i=CountDoubleValues(string);
  if (i < 0)
    return((double *) NULL);  /* no value found */
  /*
    Allocate floating point argument list.
  */
  *count=i;
  array=(double *) AcquireQuantumMemory((size_t) i,sizeof(*array));
  if (array == (double *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return((double *) NULL);
    }
  /*
    Fill in the floating point values.
  */
  i=0;
  p=string;
  while ((*p != '\0') && (i < *count))
  {
    array[i++]=StringToDouble(p,&q);
    p=q;
    while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == ','))
      p++;
  }
  return(array);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S t r i n g T o k e n                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToken() looks for any one of given delimiters and splits the string
%  into two separate strings by replacing the delimiter character found with a
%  null character.
%
%  The given string pointer is changed to point to the string following the
%  delimiter character found, or NULL.  A pointer to the start of the
%  string is returned, representing the token before the delimiter.
%
%  StringToken() is similar to the strtok() C library method, but with
%  multiple delimiter characters rather than a delimiter string.
%
%  The format of the StringToken method is:
%
%      char *StringToken(const char *delimiters,char **string)
%
%  A description of each parameter follows:
%
%    o delimiters: one or more delimiters.
%
%    o string: return the first token in the string.  If none is found, return
%      NULL.
%
*/
static MagickBooleanType IsDelimiter(const int c,const char *delimiters)
{
  const char
    *r;

  int
    d;

  r=delimiters;
  do
  {
    d=(*r++);
    if (c == d)
      return(MagickTrue);
  } while (d != '\0');
  return(MagickFalse);
}

MagickExport char *StringToken(const char *delimiters,char **string)
{
  char
    *q;

  char
    *p;

  int
    c;

  p=(*string);
  if (p == (char *) NULL)
    return((char *) NULL);
  q=p;
  for ( ; ; )
  {
    c=(*p++);
    if (IsDelimiter(c,delimiters) != MagickFalse)
      {
        if (c == '\0')
          p=(char *) NULL;
        else
          p[-1]='\0';
        *string=p;
        return(q);
      }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o L i s t                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToList() converts a text string into a list by segmenting the text
%  string at each carriage return discovered.  The list is converted to HEX
%  characters if any control characters are discovered within the text string.
%
%  The format of the StringToList method is:
%
%      char **StringToList(const char *text)
%
%  A description of each parameter follows:
%
%    o text:  Specifies the string to segment into a list.
%
*/
MagickExport char **StringToList(const char *text)
{
  return(StringToStrings(text,(size_t *) NULL));
}

static size_t CountTextLines(const char *text)
{
  const char
    *p;

  size_t
    lines;

  lines=1;
  for (p=text; *p != '\0'; p++)
    if (*p == '\n')
      lines++;
  return(lines);
}

static char **DestroyTextList(char **textlist,const ssize_t count)
{
  ssize_t
    i;

  for (i=0; i < count; i++)
    textlist[i]=DestroyString(textlist[i]);
  textlist=(char **) RelinquishMagickMemory(textlist);
  return((char **) NULL);
}

static char **AsciiTextList(const char *text,size_t *lines)
{
  char
    **textlist;

  const char
    *p,
    *q;

  ssize_t
    i;

  /*
    Convert string to an ASCII list.
  */
  *lines=CountTextLines(text);
  textlist=(char **) AcquireQuantumMemory((size_t) *lines+1UL,
    sizeof(*textlist));
  if (textlist == (char **) NULL)
    return((char **) NULL);
  p=text;
  for (i=0; i < (ssize_t) *lines; i++)
  {
    for (q=p; *q != '\0'; q++)
      if ((*q == '\r') || (*q == '\n'))
        break;
    textlist[i]=(char *) AcquireQuantumMemory((size_t) (q-p)+1,
      sizeof(**textlist));
    if (textlist[i] == (char *) NULL)
      return(DestroyTextList(textlist,i));
    (void) memcpy(textlist[i],p,(size_t) (q-p));
    textlist[i][q-p]='\0';
    if (*q == '\r')
      q++;
    p=q+1;
  }
  return(textlist);
}

static char *PadHexTextLine(char *q,ssize_t j)
{
  for ( ; j <= CharsPerLine; j++)
  {
    *q++=' ';
    *q++=' ';
    if ((j % 0x04) == 0)
      *q++=' ';
  }
  return(q);
}

static char *FormatHexDigits(char *q,const char *p,const size_t length)
{
  char
    hex_string[MagickPathExtent];

  ssize_t
    j;

  for (j=1; j <= (ssize_t) MagickMin(length,CharsPerLine); j++)
  {
    (void) FormatLocaleString(hex_string,MagickPathExtent,"%02x",*(p+j));
    (void) CopyMagickString(q,hex_string,MagickPathExtent);
    q+=(ptrdiff_t) 2;
    if ((j % 0x04) == 0)
      *q++=' ';
  }
  q=PadHexTextLine(q,j);
  return(q);
}

static char *FormatHexTextLine(char *q,const char **text)
{
  const char
    *p;

  size_t
    length;

  ssize_t
    j;

  p=(*text);
  length=strlen(p);
  q=FormatHexDigits(q,p,length);
  *q++=' ';
  for (j=1; j <= (ssize_t) MagickMin(length,CharsPerLine); j++)
  {
    if (isprint((int) ((unsigned char) *p)) != 0)
      *q++=(*p);
    else
      *q++='-';
    p++;
  }
  *q='\0';
  *text=p;
  return(q);
}

static char **HexTextList(const char *text,size_t *lines)
{
  char
    **textlist,
    *q,
    *resized;

  const char
    *p;

  ssize_t
    i;

  /*
    Convert string to a HEX list.
  */
  *lines=(size_t) (strlen(text)/CharsPerLine)+1;
  textlist=(char **) AcquireQuantumMemory((size_t) *lines+1UL,
    sizeof(*textlist));
  if (textlist == (char **) NULL)
    return((char **) NULL);
  p=text;
  for (i=0; i < (ssize_t) *lines; i++)
  {
    textlist[i]=(char *) AcquireQuantumMemory(2UL*MagickPathExtent,
      sizeof(**textlist));
    if (textlist[i] == (char *) NULL)
      return(DestroyTextList(textlist,i));
    (void) FormatLocaleString(textlist[i],MagickPathExtent,"0x%08lx: ",
      (long) (CharsPerLine*i));
    q=FormatHexTextLine(textlist[i]+strlen(textlist[i]),&p);
    resized=(char *) ResizeQuantumMemory(textlist[i],(size_t) (q-textlist[i]+
      1),sizeof(**textlist));
    if (resized == (char *) NULL)
      {
        textlist[i]=(char *) NULL;
        return(DestroyTextList(textlist,i));
      }
    textlist[i]=resized;
  }
  return(textlist);
}

static MagickBooleanType IsPlainText(const char *text)
{
  const char
    *p;

  for (p=text; *p != '\0'; p++)
    if (((int) ((unsigned char) *p) < 32) &&
        (isspace((int) ((unsigned char) *p)) == 0))
      break;
  return(*p == '\0' ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o S t r i n g s                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToStrings() converts a text string into a list by segmenting the text
%  string at each carriage return discovered.  The list is converted to HEX
%  characters if any control characters are discovered within the text string.
%
%  The format of the StringToList method is:
%
%      char **StringToList(const char *text,size_t *lines)
%
%  A description of each parameter follows:
%
%    o text:  Specifies the string to segment into a list.
%
%    o count: Return value for the number of items in the list.
%
*/
MagickExport char **StringToStrings(const char *text,size_t *count)
{
  char
    **textlist;

  size_t
    lines;

  if (text == (char *) NULL)
    {
      if (count != (size_t *) NULL)
        *count=0;
      return((char **) NULL);
    }
  if (IsPlainText(text) != MagickFalse)
    textlist=AsciiTextList(text,&lines);
  else
    textlist=HexTextList(text,&lines);
  if (textlist == (char **) NULL)
    return((char **) NULL);
  if (count != (size_t *) NULL)
    *count=lines;
  textlist[lines]=(char *) NULL;
  return(textlist);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i p M a g i c k S t r i n g                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StripMagickString() strips any whitespace or quotes from the beginning and
%  end of a string of characters. It returns the stripped string length.
%
%  The format of the StripMagickString method is:
%
%      size_t StripMagickString(char *message)
%
%  A description of each parameter follows:
%
%    o message: Specifies an array of characters.
%
*/

MagickExport void StripString(char *message)
{
  (void) StripMagickString(message);
}

static char *SkipLeadingSpaceAndQuote(char *message)
{
  char
    *p;

  p=message;
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;
  if ((*p == '\'') || (*p == '"'))
    p++;
  return(p);
}

static char *TrimTrailingSpaceAndQuote(char *message,const char *p)
{
  char
    *q;

  q=message+strlen(message)-1;
  while ((isspace((int) ((unsigned char) *q)) != 0) && (q > p))
    q--;
  if (q > p)
    if ((*q == '\'') || (*q == '"'))
      q--;
  return(q);
}

MagickExport size_t StripMagickString(char *message)
{
  char
    *p,
    *q;

  assert(message != (char *) NULL);
  if (*message == '\0')
    return(0);
  if (strlen(message) == 1)
    return(1);
  p=SkipLeadingSpaceAndQuote(message);
  q=TrimTrailingSpaceAndQuote(message,p);
  (void) memmove(message,p,(size_t) (q-p+1));
  message[q-p+1]='\0';
  for (p=message; *p != '\0'; p++)
    if (*p == '\n')
      *p=' ';
  return((size_t) (q-p+1));
}

static void ReplaceMatch(char *p,const char *replace,
  const size_t search_extent,const size_t replace_extent)
{
  if (search_extent != replace_extent)
    (void) memmove(p+replace_extent,p+search_extent,
      strlen(p+search_extent)+1);
  (void) memcpy(p,replace,replace_extent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S u b s t i t u t e S t r i n g                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SubstituteString() performs string substitution on a string, replacing the
%  string with the substituted version. Buffer must be allocated from the heap.
%  If the string is matched and status, MagickTrue is returned otherwise
%  MagickFalse.
%
%  The format of the SubstituteString method is:
%
%      MagickBooleanType SubstituteString(char **string,const char *search,
%        const char *replace)
%
%  A description of each parameter follows:
%
%    o string: the string to perform replacements on;  replaced with new
%      allocation if a replacement is made.
%
%    o search: search for this string.
%
%    o replace: replace any matches with this string.
%
*/
MagickExport MagickBooleanType SubstituteString(char **string,
  const char *search,const char *replace)
{
  MagickBooleanType
    status;

  char
    *p;

  size_t
    extent,
    replace_extent,
    search_extent;

  ssize_t
    offset;

  status=MagickFalse;
  search_extent=0,
  replace_extent=0;
  for (p=strchr(*string,*search); p != (char *) NULL; p=strchr(p+1,*search))
  {
    if (search_extent == 0)
      search_extent=strlen(search);
    if (strncmp(p,search,search_extent) != 0)
      continue;
    /*
      We found a match.
    */
    status=MagickTrue;
    if (replace_extent == 0)
      replace_extent=strlen(replace);
    if (replace_extent > search_extent)
      {
        /*
          Make room for the replacement string.
        */
        offset=(ssize_t) (p-(*string));
        extent=strlen(*string)+replace_extent-search_extent+1;
        *string=(char *) ResizeQuantumMemory(*string,
          OverAllocateMemory(extent+MagickPathExtent),sizeof(*p));
        if (*string == (char *) NULL)
          ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
        p=(*string)+offset;
      }
    /*
      Replace string.
    */
    ReplaceMatch(p,replace,search_extent,replace_extent);
    p+=(ptrdiff_t) replace_extent-1;
  }
  return(status);
}
