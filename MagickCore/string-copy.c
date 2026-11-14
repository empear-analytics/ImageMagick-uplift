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
%                          CCCC   OOO   PPPP   Y   Y                          %
%                         C      O   O  P   P   Y Y                           %
%                         C      O   O  PPPP     Y                            %
%                         C      O   O  P        Y                            %
%                          CCCC   OOO   P        Y                            %
%                                                                             %
%                                                                             %
%                    Bounded String Copy and Concatenation                    %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n c a t e n a t e M a g i c k S t r i n g                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConcatenateMagickString() concatenates the source string to the destination
%  string.  The destination buffer is always null-terminated even if the
%  string must be truncated.
%
%  The format of the ConcatenateMagickString method is:
%
%      size_t ConcatenateMagickString(char *magick_restrict destination,
%        const char *magick_restrict source,const size_t length)
%
%  A description of each parameter follows:
%
%    o destination: the destination string.
%
%    o source: the source string.
%
%    o length: the length of the destination string.
%
*/
MagickExport size_t ConcatenateMagickString(char *magick_restrict destination,
  const char *magick_restrict source,const size_t length)
{
  char
    *magick_restrict q;

  const char
    *magick_restrict p;

  size_t
    count,
    i;

  assert(destination != (char *) NULL);
  assert(source != (const char *) NULL);
  assert(length >= 1);
  p=source;
  q=destination;
  i=length;
  while ((i-- != 0) && (*q != '\0'))
    q++;
  count=(size_t) (q-destination);
  i=length-count;
  if (i == 0)
    return(count+strlen(p));
  while (*p != '\0')
  {
    if (i != 1)
      {
        *q++=(*p);
        i--;
      }
    p++;
  }
  *q='\0';
  return(count+(size_t) (p-source));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o p y M a g i c k S t r i n g                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CopyMagickString() copies the source string to the destination string, with
%  out exceeding the given pre-declared length.
%
%  The destination buffer is always null-terminated even if the string must be
%  truncated.  The return value is the length of the string.
%
%  The format of the CopyMagickString method is:
%
%      size_t CopyMagickString(const char *magick_restrict destination,
%        char *magick_restrict source,const size_t length)
%
%  A description of each parameter follows:
%
%    o destination: the destination string.
%
%    o source: the source string.
%
%    o length: the length of the destination string.
%
*/
MagickExport size_t CopyMagickString(char *magick_restrict destination,
  const char *magick_restrict source,const size_t length)
{
  char
    *magick_restrict q;

  const char
    *magick_restrict p;

  size_t
    n;

  p=source;
  q=destination;
  for (n=length; n > 4; n-=4)
  {
    if (((*q++)=(*p++)) == '\0')
      return((size_t) (p-source-1));
    if (((*q++)=(*p++)) == '\0')
      return((size_t) (p-source-1));
    if (((*q++)=(*p++)) == '\0')
      return((size_t) (p-source-1));
    if (((*q++)=(*p++)) == '\0')
      return((size_t) (p-source-1));
  }
  if (length != 0)
    {
      while (--n != 0)
        if (((*q++)=(*p++)) == '\0')
          return((size_t) (p-source-1));
      *q='\0';
    }
  return((size_t) (p-source));
}
