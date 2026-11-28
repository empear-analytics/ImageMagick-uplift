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

  MagickCore private token methods.
*/
#ifndef MAGICKCORE_TOKEN_PRIVATE_H
#define MAGICKCORE_TOKEN_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef EILSEQ
  #define EILSEQ  ENOENT
#endif

#define MaxMultibyteCodes  6

extern MagickPrivate MagickBooleanType
  IsGlob(const char *) magick_attribute((__pure__));

typedef struct
{
  int
    code_mask,
    code_value,
    utf_mask,
    utf_value;
} UTFInfo;

static UTFInfo
  utf_info[MaxMultibyteCodes] =
  {
    { 0x80, 0x00, 0x000007f, 0x0000000 },  /* 1 byte sequence */
    { 0xE0, 0xC0, 0x00007ff, 0x0000080 },  /* 2 byte sequence */
    { 0xF0, 0xE0, 0x000ffff, 0x0000800 },  /* 3 byte sequence */
    { 0xF8, 0xF0, 0x01fffff, 0x0010000 },  /* 4 byte sequence */
    { 0xFC, 0xF8, 0x03fffff, 0x0200000 },  /* 5 byte sequence */
    { 0xFE, 0xFC, 0x7ffffff, 0x4000000 },  /* 6 byte sequence */
  };

#include "MagickCore/token-utf8-private.h"

static inline int GetNextUTFCode(const char *magick_restrict text,
  unsigned int *magick_restrict octets)
{
  int
    code;

  ssize_t
    i;

  int
    c,
    unicode;

  *octets=1;
  if (text == (const char *) NULL)
    {
      errno=EINVAL;
      return(-1);
    }
  code=(int) (*text++) & 0xff;
  unicode=code;
  for (i=0; i < MaxMultibyteCodes; i++)
  {
    if ((code & utf_info[i].code_mask) == utf_info[i].code_value)
      {
        unicode&=utf_info[i].utf_mask;
        if (unicode < utf_info[i].utf_value)
          break;
        *octets=(unsigned int) (i+1);
        return(unicode);
      }
    c=(int) (*text++ ^ 0x80) & 0xff;
    if ((c & 0xc0) != 0)
      break;
    if (unicode > 0x10FFFF)
      break;
    unicode=(unicode << 6) | c;
  }
  errno=EILSEQ;
  return(-1);
}

static inline int GetUTFCode(const char *magick_restrict text)
{
  unsigned int
    octets;

  return(GetNextUTFCode(text,&octets));
}

static inline unsigned int GetUTFOctets(const char *magick_restrict text)
{
  unsigned int
    octets;

  (void) GetNextUTFCode(text,&octets);
  return(octets);
}

static inline MagickBooleanType IsNonBreakingUTFSpace(const int code)
{
  if (code == 0x00a0)
    return(MagickTrue);
  return(MagickFalse);
}

static const int
  UTFSpaceCodes[] =
  {
    0x0020,
    0x0085,
    0x00a0,
    0x1680,
    0x180e,
    0x2028,
    0x2029,
    0x202f,
    0x205f,
    0x3000
  };

static inline MagickBooleanType IsUTFSpace(const int code)
{
  size_t
    i;

  if ((code >= 0x0009) && (code <= 0x000d))
    return(MagickTrue);
  for (i=0; i < (sizeof(UTFSpaceCodes)/sizeof(*UTFSpaceCodes)); i++)
    if (UTFSpaceCodes[i] == code)
      return(MagickTrue);
  if ((code >= 0x2000) && (code <= 0x200a))
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsUTFValid(int code)
{
  int
    mask;

  mask=(int) 0x7fffffff;
  if ((code & ~mask) == 0)
    return(MagickTrue);
  if (!((code < 0xd800) || (code > 0xdfff)))
    return(MagickTrue);
  if (code == 0xfffe)
    return(MagickTrue);
  if (code == 0xffff)
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsUTFAscii(int code)
{
  int
    mask;

  mask=(int) 0x7f;
  if ((code & ~mask) != 0)
    return(MagickFalse);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
