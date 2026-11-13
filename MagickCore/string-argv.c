/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS  TTTTT  RRRR   IIIII  N   N   GGGG                   %
%                  SS       T    R   R    I    NN  N  G                       %
%                   SSS     T    RRRR     I    N N N  G  GG                   %
%                     SS    T    R R      I    N  NN  G   G                   %
%                  SSSSS    T    R  R   IIIII  N   N   GGG                    %
%                                                                             %
%                          AAA   RRRR    GGGG  V   V                          %
%                         A   A  R   R  G      V   V                          %
%                         AAAAA  RRRR   G  GG  V   V                          %
%                         A   A  R R    G   G   V V                           %
%                         A   A  R  R    GGG     V                            %
%                                                                             %
%                                                                             %
%                            String To Argv Method                            %
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

static const char *SkipArgvSpaces(const char *p)
{
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;
  return(p);
}

static const char *SkipArgvQuoted(const char *p,const int quote)
{
  for (p++; (*p != quote) && (*p != '\0'); p++) ;
  return(p);
}

static const char *SkipArgvWord(const char *p)
{
  while ((isspace((int) ((unsigned char) *p)) == 0) && (*p != '\0'))
    p++;
  return(p);
}

static void CountArgvArguments(const char *text,int *argc)
{
  const char
    *p;

  for (p=text; *p != '\0'; )
  {
    p=SkipArgvSpaces(p);
    if (*p == '\0')
      break;
    (*argc)++;
    if (*p == '"')
      p=SkipArgvQuoted(p,'"');
    if (*p == '\'')
      p=SkipArgvQuoted(p,'\'');
    p=SkipArgvWord(p);
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o A r g v                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToArgv() converts a text string into command line arguments.
%  The 'argv' array of arguments, is returned while the number of arguments
%  is returned via the provided integer variable pointer.
%
%  Simple 'word' tokenizer, which allows for each word to be optionally
%  quoted.  However it will not allow use of partial quotes, or escape
%  characters.
%
%  The format of the StringToArgv method is:
%
%      char **StringToArgv(const char *text,int *argc)
%
%  A description of each parameter follows:
%
%    o argv:  Method StringToArgv returns the string list unless an error
%      occurs, otherwise NULL.
%
%    o text:  Specifies the string to segment into a list.
%
%    o argc:  This integer pointer returns the number of arguments in the
%      list.
%
*/
MagickExport char **StringToArgv(const char *text,int *argc)
{
  char
    **argv;

  const char
    *p,
    *q;

  ssize_t
    i;

  *argc=0;
  if (text == (char *) NULL)
    return((char **) NULL);
  /*
    Determine the number of arguments.
  */
  CountArgvArguments(text,argc);
  (*argc)++;
  argv=(char **) AcquireQuantumMemory((size_t) *argc+1UL,sizeof(*argv));
  if (argv == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToConvertStringToARGV");
  /*
    Convert string to an ASCII list.
  */
  argv[0]=AcquireString("magick");
  p=text;
  for (i=1; i < (ssize_t) *argc; i++)
  {
    p=SkipArgvSpaces(p);
    q=p;
    if (*q == '"')
      {
        p++;
        q=SkipArgvQuoted(q,'"');
      }
    else
      if (*q == '\'')
        {
          p++;
          q=SkipArgvQuoted(q,'\'');
        }
      else
        q=SkipArgvWord(q);
    argv[i]=(char *) AcquireQuantumMemory((size_t) (q-p)+MagickPathExtent,
      sizeof(**argv));
    if (argv[i] == (char *) NULL)
      {
        for (i--; i >= 0; i--)
          argv[i]=DestroyString(argv[i]);
        argv=(char **) RelinquishMagickMemory(argv);
        ThrowFatalException(ResourceLimitFatalError,
          "UnableToConvertStringToARGV");
      }
    (void) memcpy(argv[i],p,(size_t) (q-p));
    argv[i][q-p]='\0';
    p=q;
    p=SkipArgvWord(p);
  }
  argv[i]=(char *) NULL;
  return(argv);
}
