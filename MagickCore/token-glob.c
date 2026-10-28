/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      TTTTT   OOO   K   K  EEEEE  N   N                      %
%                        T    O   O  K  K   E      NN  N                      %
%                        T    O   O  KKK    EEE    N N N                      %
%                        T    O   O  K  K   E      N  NN                      %
%                        T     OOO   K   K  EEEEE  N   N                      %
%                                                                             %
%                          GGGG  L       OOO   BBBB                           %
%                         G      L      O   O  B   B                          %
%                         G  GG  L      O   O  BBBB                           %
%                         G   G  L      O   O  B   B                          %
%                          GGG   LLLLL   OOO   BBBB                           %
%                                                                             %
%                                                                             %
%                      MagickCore Glob Expression Method                      %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                              January 1993                                   %
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
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"

static MagickBooleanType MatchCharacterClass(const char *p,const char *q,
  const int ecode)
{
  MagickBooleanType
    matched = MagickFalse;

  while (p < q)
  {
    const char
      *next;

    int
      code = GetUTFCode(p);

    size_t
      octets = GetUTFOctets(p);

    if (code == '\\')
      {
        p+=octets;
        code=GetUTFCode(p);
        octets=GetUTFOctets(p);
      }
    next=p+octets;
    if ((next < q) && (GetUTFCode(next) == '-'))
      {
        int
          ncode;

        next+=GetUTFOctets(next);
        ncode=GetUTFCode(next);
        if (ncode == '\\')
          {
            next+=GetUTFOctets(next);
            ncode=GetUTFCode(next);
          }
        if ((ecode >= code) && (ecode <= ncode))
          matched=MagickTrue;
        p=next+GetUTFOctets(next);
      }
    else
      {
        if (ecode == code)
          matched=MagickTrue;
        p+=octets;
      }
  }
  return(matched);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G l o b E x p r e s s i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GlobExpression() returns MagickTrue if the expression matches the pattern.
%
%  The format of the GlobExpression function is:
%
%      MagickBooleanType GlobExpression(const char *magick_restrict expression,
%        const char *magick_restrict pattern,
%        const MagickBooleanType case_insensitive)
%
%  A description of each parameter follows:
%
%    o expression: Specifies a pointer to a text string containing a file name.
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o case_insensitive: set to MagickTrue to ignore the case when matching
%      an expression.
%
*/

static MagickBooleanType GlobExpression_(const char *magick_restrict expression,
  const char *magick_restrict pattern,const MagickBooleanType case_insensitive,
  const size_t depth)
{
  if (depth >= MagickMaxRecursionDepth)
    {
      errno=EOVERFLOW;
      return(MagickFalse);
    }
  /*
    Empty pattern or single '*' always matches.
  */
  if (pattern == (const char *) NULL)
    return(MagickTrue);
  if (GetUTFCode(pattern) == 0)
    return(MagickTrue);
  if ((GetUTFCode(pattern) == '*') &&
      (GetUTFCode(pattern+GetUTFOctets(pattern)) == 0))
    return(MagickTrue);
  if ((strchr(pattern,'{') == NULL) &&
      (strchr(pattern,'*') == NULL) &&
      (strchr(pattern,'?') == NULL))
    {
      char
        path[MagickPathExtent]= { 0 };

      /*
        If no glob characters exist, ensure no subimage specifier.
      */
      GetPathComponent(pattern,SubimagePath,path);
      if (*path != '\0')
        return(MagickFalse);
    }
  while (GetUTFCode(pattern) != 0)
  {
    int
      ecode = GetUTFCode(expression),
      pcode = GetUTFCode(pattern);

    if ((ecode == 0) && (pcode != '*') && (pcode != '{'))
      break;
    switch (pcode)
    {
      case '*':
      {
        do
        {
          /*
            Skip consecutive '*'.
          */
          pattern+=GetUTFOctets(pattern);
        }
        while (GetUTFCode(pattern) == '*');
        while (1)
        {
          /*
            Try to match at each position.
          */
          if (GlobExpression_(expression,pattern,case_insensitive,depth+1) != MagickFalse)
            {
              /*
                Consume rest of expression and pattern.
              */
              while (GetUTFCode(expression) != 0)
                expression+=GetUTFOctets(expression);
              while (GetUTFCode(pattern) != 0)
                pattern+=GetUTFOctets(pattern);
              return(MagickTrue);
            }
            if (GetUTFCode(expression) == 0)
              break;
            expression+=GetUTFOctets(expression);
          }
        return(MagickFalse);
      }
      case '?':
      {
        if (ecode == 0)
          return(MagickFalse);
        pattern+=GetUTFOctets(pattern);
        expression+=GetUTFOctets(expression);
        break;
      }
      case '[':
      {
        const char
          *p = pattern+GetUTFOctets(pattern),
          *q = pattern+GetUTFOctets(pattern);

        MagickBooleanType
          matched = MagickFalse;

        if (ecode == 0)
          return(MagickFalse);
        while ((GetUTFCode(q) != 0) && (GetUTFCode(q) != ']'))
          q+=GetUTFOctets(q);
        if (GetUTFCode(q) == 0)
          return(MagickFalse);  /* malformed */
        matched=MatchCharacterClass(p,q,ecode);
        /*
          Skip consecutive '*'.
        */
        if (matched == MagickFalse)
          return(MagickFalse);
        pattern=q+GetUTFOctets(q);  /* skip ']' */
        expression+=GetUTFOctets(expression);
        break;
      }
      case '{':
      {
        char
          *a,
          *alternative;

        const char
          *p,
          *q;

        size_t
          remaining = MagickPathExtent;

        pattern+=GetUTFOctets(pattern);  /* Skip '{' */
        if (GetUTFCode(pattern) == 0)
          return(MagickFalse);
        /*
          End of brace expression: append remaining pattern.
        */
        p=pattern;
        while ((GetUTFCode(p) != 0) && (GetUTFCode(p) != '}'))
        {
#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__)
          if (GetUTFCode(p) == '\\')
            {
              p+=GetUTFOctets(p);
              if (GetUTFCode(p) == 0)
                break;
            }
#endif
          p+=GetUTFOctets(p);
        }
        if (GetUTFCode(p) != '}')
          return(MagickFalse);  /* malformed */
        q=p+GetUTFOctets(p);
        alternative=AcquireString(pattern);
        a=alternative;
        while (1)
        {
          int
            code = GetUTFCode(pattern);

          size_t
            octets;

          if ((code == 0) || (code == ',') || (code == '}'))
            {
              char
                *subpattern;

              MagickBooleanType
                match;

              /*
                Try alternative as a full sub-pattern.
              */
              *a='\0';
              subpattern=AcquireString(alternative);
              if (ConcatenateString(&subpattern,q) == MagickFalse)
                {
                  subpattern=DestroyString(subpattern);
                  alternative=DestroyString(alternative);
                  return(MagickFalse);
                }
              match=GlobExpression_(expression,subpattern,case_insensitive,
                depth+1);
              subpattern=DestroyString(subpattern);
              if (match != MagickFalse)
                {
                  /*
                    Consume rest of expression and pattern.
                  */
                  while (GetUTFCode(expression) != 0)
                    expression+=GetUTFOctets(expression);
                  pattern=q;
                  while (GetUTFCode(pattern) != 0)
                    pattern+=GetUTFOctets(pattern);
                  alternative=DestroyString(alternative);
                  return(MagickTrue);
                }
              /*
                Reset buffer for next alternative.
              */
              a=alternative;
              remaining=MagickPathExtent;
              if (code == ',')
                {
                  pattern+=GetUTFOctets(pattern);  /* skip ',' */
                  continue;
                }
              break;  /* '}' or end */
            }
          /*
            Copy UTF-8 sequence into alternative.
          */
          octets=GetUTFOctets(pattern);
          if ((octets == 0) || (octets >= remaining))
            break;
          (void) memcpy(a,pattern,octets);
          a+=octets;
          remaining-=octets;
          pattern+=octets;
        }
        alternative=DestroyString(alternative);
        return(MagickFalse);
      }
#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__)
      case '\\':
      {
        pattern+=GetUTFOctets(pattern);
        if (GetUTFCode(pattern) == 0)
          return(MagickFalse);
        magick_fallthrough;
      }
#endif
      default:
      {
        int
          ec = ecode,
          pc = pcode;

        if (ecode == 0)
          return(MagickFalse);
        if (case_insensitive != MagickFalse)
          {
            pc=LocaleToLowercase(pc);
            ec=LocaleToLowercase(ec);
          }
        if (pc != ec)
          return(MagickFalse);
        pattern+=GetUTFOctets(pattern);
        expression+=GetUTFOctets(expression);
        break;
      }
    }
  }
  while (GetUTFCode(pattern) == '*')
    pattern+=GetUTFOctets(pattern);
  return(((GetUTFCode(expression) == 0) &&
          (GetUTFCode(pattern) == 0)) ? MagickTrue : MagickFalse);
}

MagickExport MagickBooleanType GlobExpression(
  const char *magick_restrict expression,const char *magick_restrict pattern,
  const MagickBooleanType case_insensitive)
{
  return(GlobExpression_(expression,pattern,case_insensitive,0));
}
