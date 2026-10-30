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
%                                SSSSS  IIIII                                 %
%                                SS       I                                   %
%                                 SSS     I                                   %
%                                   SS    I                                   %
%                                SSSSS  IIIII                                 %
%                                                                             %
%                                                                             %
%                           SI Prefix Value Method                            %
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

static double GetSiPrefixExponent(const char *q)
{
  double
    e;

  switch ((int) ((unsigned char) *q))
  {
    case 'q': e=(-30.0); break;
    case 'r': e=(-27.0); break;
    case 'y': e=(-24.0); break;
    case 'z': e=(-21.0); break;
    case 'a': e=(-18.0); break;
    case 'f': e=(-15.0); break;
    case 'p': e=(-12.0); break;
    case 'n': e=(-9.0); break;
    case 'u': e=(-6.0); break;
    case 'm': e=(-3.0); break;
    case 'c': e=(-2.0); break;
    case 'd': e=(-1.0); break;
    case 'h': e=2.0; break;
    case 'k': e=3.0; break;
    case 'K': e=3.0; break;
    case 'M': e=6.0; break;
    case 'G': e=9.0; break;
    case 'T': e=12.0; break;
    case 'P': e=15.0; break;
    case 'E': e=18.0; break;
    case 'Z': e=21.0; break;
    case 'Y': e=24.0; break;
    case 'R': e=27.0; break;
    case 'Q': e=30.0; break;
    default: e=0.0; break;
  }
  return(e);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n t e r p r e t S i P r e f i x V a l u e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpretSiPrefixValue() converts the initial portion of the string to a
%  double representation.  It also recognizes SI prefixes (e.g. B, KB, MiB,
%  etc.).
%
%  The format of the InterpretSiPrefixValue method is:
%
%      double InterpretSiPrefixValue(const char *value,char **sentinel)
%
%  A description of each parameter follows:
%
%    o value: the string value.
%
%    o sentinel:  if sentinel is not NULL, return a pointer to the character
%      after the last character used in the conversion.
%
*/
MagickExport double InterpretSiPrefixValue(const char *magick_restrict string,
  char **magick_restrict sentinel)
{
  char
    *q;

  double
    value;

  value=InterpretLocaleValue(string,&q);
  if (q != string)
    {
      if ((*q >= 'E') && (*q <= 'z'))
        {
          double
            e;

          e=GetSiPrefixExponent(q);
          if (e >= MagickEpsilon)
            {
              if (q[1] == 'i')
                {
                  value*=pow(2.0,e/0.3);
                  q+=(ptrdiff_t) 2;
                }
              else
                {
                  value*=pow(10.0,e);
                  q++;
                }
            }
        }
      if ((*q == 'B') || (*q == 'P'))
        q++;
    }
  if (sentinel != (char **) NULL)
    *sentinel=q;
  return(value);
}
