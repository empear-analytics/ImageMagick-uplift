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

  MagickCore private methods of utility.c: the Base64 decoding helpers,
  DecodeBase64Character to CheckBase64DecodeArguments; included by that file
  only.
*/
#ifndef MAGICKCORE_UTILITY_BASE64_PRIVATE_H
#define MAGICKCORE_UTILITY_BASE64_PRIVATE_H

static void DecodeBase64Character(unsigned char *decode,const char *q,
  size_t *i,int *state)
{
  switch (*state)
  {
    case 0:
    {
      decode[(*i)]=(unsigned char)((q-Base64) << 2);
      (*state)++;
      break;
    }
    case 1:
    {
      decode[(*i)++]|=(unsigned char)((q-Base64) >> 4);
      decode[(*i)]=(unsigned char)(((q-Base64) & 0x0f) << 4);
      (*state)++;
      break;
    }
    case 2:
    {
      decode[(*i)++]|=(unsigned char)((q-Base64) >> 2);
      decode[(*i)]=(unsigned char)(((q-Base64) & 0x03) << 6);
      (*state)++;
      break;
    }
    case 3:
    {
      decode[(*i)++]|=(unsigned char)(q-Base64);
      (*state)=0;
      break;
    }
  }
}

static MagickBooleanType VerifyBase64Tail(const char *p,
  unsigned char *decode,const size_t i)
{
  for ( ; *p != '\0'; p++)
    if (isspace((int) ((unsigned char) *p)) == 0)
      {
        decode=(unsigned char *) RelinquishMagickMemory(decode);
        return(MagickFalse);
      }
  if ((int) decode[i] != 0)
    {
      decode=(unsigned char *) RelinquishMagickMemory(decode);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static const char *SkipBase64Whitespace(const char *p)
{
  for ( ; *p != '\0'; p++)
    if (isspace((int) ((unsigned char) *p)) == 0)
      break;
  return(p);
}

static MagickBooleanType VerifyBase64Padding(const char *p,const int state,
  unsigned char *decode,const size_t i)
{
  p++;
  switch (state)
  {
    case 0:
    case 1:
    {
      /*
        Unrecognized '=' character.
      */
      decode=(unsigned char *) RelinquishMagickMemory(decode);
      return(MagickFalse);
    }
    case 2:
    {
      p=SkipBase64Whitespace(p);
      if (*p != '=')
        {
          decode=(unsigned char *) RelinquishMagickMemory(decode);
          return(MagickFalse);
        }
      p++;
      magick_fallthrough;
    }
    case 3:
    {
      if (VerifyBase64Tail(p,decode,i) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType VerifyBase64Terminal(const char *p,const int state,
  unsigned char *decode,const size_t i)
{
  if (*p != '=')
    {
      if (state != 0)
        {
          decode=(unsigned char *) RelinquishMagickMemory(decode);
          return(MagickFalse);
        }
    }
  else if (VerifyBase64Padding(p,state,decode,i) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void CheckBase64DecodeArguments(const char *source,
  const size_t *length)
{
  magick_unreferenced(source);
  magick_unreferenced(length);
  assert(source != (char *) NULL);
  assert(length != (size_t *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
}

#endif
