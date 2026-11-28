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

  MagickCore private methods of opencl-initialize.c: the DOCTYPE skipper of
  the OpenCL device benchmark profile reader; included by
  opencl-initialize-benchmark-private.h only, inside its
  !MAGICKCORE_ZERO_CONFIGURATION_SUPPORT conditional.
*/
#ifndef MAGICKCORE_OPENCL_INITIALIZE_DOCTYPE_PRIVATE_H
#define MAGICKCORE_OPENCL_INITIALIZE_DOCTYPE_PRIVATE_H

static int EndDoctypeQuote(const char *q,int quote)
{
  if (*q == quote)
    quote=0;
  return(quote);
}

static inline MagickBooleanType IsDoctypeQuote(const char *q)
{
  return(((*q == '"') || (*q == '\'')) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsDoctypeEnd(const char *q,
  const int bracket_depth)
{
  return(((*q == '>') && (bracket_depth == 0)) ? MagickTrue : MagickFalse);
}

static const char *SkipOpenCLBenchmarkDoctype(const char *q)
{
  int
    bracket_depth = 0,
    quote = 0;

  /*
    DOCTYPE element.
  */
  for ( ; *q != '\0'; q++)
  {
    if (quote != 0)
      quote=EndDoctypeQuote(q,quote);
    else if (IsDoctypeQuote(q) != MagickFalse)
      quote=(*q);
    else if (*q == '[')
      bracket_depth++;
    else if (*q == ']')
      bracket_depth=CloseDoctypeBracket(bracket_depth);
    else if (IsDoctypeEnd(q,bracket_depth) != MagickFalse)
      {
        q++;   /* consume final '>' */
        break;
      }
  }
  return(q);
}

#endif
