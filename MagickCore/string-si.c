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

typedef struct _SiPrefixExponent
{
  int
    key;

  double
    value;
} SiPrefixExponent;

static const SiPrefixExponent
  SiPrefixExponents[] =
  {
    { 'q', (-30.0) },
    { 'r', (-27.0) },
    { 'y', (-24.0) },
    { 'z', (-21.0) },
    { 'a', (-18.0) },
    { 'f', (-15.0) },
    { 'p', (-12.0) },
    { 'n', (-9.0) },
    { 'u', (-6.0) },
    { 'm', (-3.0) },
    { 'c', (-2.0) },
    { 'd', (-1.0) },
    { 'h', 2.0 },
    { 'k', 3.0 },
    { 'K', 3.0 },
    { 'M', 6.0 },
    { 'G', 9.0 },
    { 'T', 12.0 },
    { 'P', 15.0 },
    { 'E', 18.0 },
    { 'Z', 21.0 },
    { 'Y', 24.0 },
    { 'R', 27.0 },
    { 'Q', 30.0 }
  };

static const SiPrefixExponent *FindSiPrefixExponent(const int key)
{
  size_t
    i;

  for (i=0; i < (sizeof(SiPrefixExponents)/sizeof(*SiPrefixExponents)); i++)
    if (SiPrefixExponents[i].key == key)
      return(SiPrefixExponents+i);
  return((const SiPrefixExponent *) NULL);
}

static double GetSiPrefixExponent(const char *q)
{
  const SiPrefixExponent
    *entry;

  double
    e;

  entry=FindSiPrefixExponent((int) ((unsigned char) *q));
  if (entry != (const SiPrefixExponent *) NULL)
    e=entry->value;
  else
    e=0.0;
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
static void ApplySiPrefix(double *value,char **q)
{
  double
    e;

  e=GetSiPrefixExponent(*q);
  if (e >= MagickEpsilon)
    {
      if ((*q)[1] == 'i')
        {
          *value*=pow(2.0,e/0.3);
          *q+=(ptrdiff_t) 2;
        }
      else
        {
          *value*=pow(10.0,e);
          (*q)++;
        }
    }
}

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
        ApplySiPrefix(&value,&q);
      if ((*q == 'B') || (*q == 'P'))
        q++;
    }
  if (sentinel != (char **) NULL)
    *sentinel=q;
  return(value);
}
