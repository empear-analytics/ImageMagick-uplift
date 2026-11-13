/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            CCCC   OOO   M   M  PPPP   RRRR   EEEEE  SSSSS  SSSSS            %
%           C      O   O  MM MM  P   P  R   R  E      SS     SS               %
%           C      O   O  M M M  PPPP   RRRR   EEE     SSS    SSS             %
%           C      O   O  M   M  P      R R    E         SS     SS            %
%            CCCC   OOO   M   M  P      R  R   EEEEE  SSSSS  SSSSS            %
%                                                                             %
%                             L      ZZZZZ  W   W                             %
%                             L         ZZ  W   W                             %
%                             L       ZZZ   W W W                             %
%                             L      ZZ     WW WW                             %
%                             LLLLL  ZZZZZ  W   W                             %
%                                                                             %
%                                                                             %
%                             LZW Image Encoding                              %
%                                                                             %
%                           Software Design                                   %
%                                Cristy                                       %
%                              May  1993                                      %
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
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#if defined(MAGICKCORE_ZLIB_DELEGATE)
#include "zlib.h"
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L Z W E n c o d e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LZWEncodeImage() compresses an image via LZW-coding specific to Postscript
%  Level II or Portable Document Format.
%
%  The format of the LZWEncodeImage method is:
%
%      MagickBooleanType LZWEncodeImage(Image *image,const size_t length,
%        unsigned char *magick_restrict pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length:  A value that specifies the number of pixels to compress.
%
%    o pixels: the address of an unsigned array of characters containing the
%      pixels to compress.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _TableType
{
  ssize_t
    prefix,
    suffix,
    next;
} TableType;

static void ResetLZWTable(TableType *table)
{
  ssize_t
    index;

  for (index=0; index < 256; index++)
  {
    table[index].prefix=(-1);
    table[index].suffix=index;
    table[index].next=(-1);
  }
}

static ssize_t FindLZWString(const TableType *table,size_t *last_code,
  const unsigned char pixel)
{
  ssize_t
    index;

  index=(ssize_t) (*last_code);
  while (index != -1)
    if ((table[index].prefix != (ssize_t) (*last_code)) ||
        (table[index].suffix != (ssize_t) pixel))
      index=table[index].next;
    else
      {
        (*last_code)=(size_t) index;
        break;
      }
  return(index);
}

static void AddLZWString(TableType *table,const size_t last_code,
  const unsigned char pixel,const size_t next_index)
{
  table[next_index].prefix=(ssize_t) last_code;
  table[next_index].suffix=(ssize_t) pixel;
  table[next_index].next=table[last_code].next;
  table[last_code].next=(ssize_t) next_index;
}

#define LZWClr  256UL  /* Clear Table Marker */
#define LZWEod  257UL  /* End of Data marker */
#define OutputCode(code) \
{ \
    accumulator+=code << (32-code_width-number_bits); \
    number_bits+=code_width; \
    while (number_bits >= 8) \
    { \
      (void) WriteBlobByte(image,(unsigned char) (accumulator >> 24)); \
      accumulator=accumulator << 8; \
      number_bits-=8; \
    } \
}

static void CheckLZWEncodeArguments(const Image *image,
  const unsigned char *magick_restrict pixels,const ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(pixels != (unsigned char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
}

MagickExport MagickBooleanType LZWEncodeImage(Image *image,const size_t length,
  unsigned char *magick_restrict pixels,ExceptionInfo *exception)
{
  ssize_t
    i;

  size_t
    accumulator,
    number_bits,
    code_width,
    last_code,
    next_index;

  ssize_t
    index;

  TableType
    *table;

  /*
    Allocate string table.
  */
  CheckLZWEncodeArguments(image,pixels,exception);
  table=(TableType *) AcquireQuantumMemory(1UL << 12,sizeof(*table));
  if (table == (TableType *) NULL)
    ThrowBinaryException(ResourceLimitWarning,"MemoryAllocationFailed",
      image->filename);
  /*
    Initialize variables.
  */
  accumulator=0;
  code_width=9;
  number_bits=0;
  last_code=0;
  OutputCode(LZWClr);
  ResetLZWTable(table);
  next_index=LZWEod+1;
  code_width=9;
  last_code=(size_t) pixels[0];
  for (i=1; i < (ssize_t) length; i++)
  {
    /*
      Find string.
    */
    index=FindLZWString(table,&last_code,pixels[i]);
    if (last_code == (size_t) index)
      continue;
    /*
      Add string.
    */
    OutputCode(last_code);
    AddLZWString(table,last_code,pixels[i],next_index);
    next_index++;
    /*
      Did we just move up to next bit width?
    */
    if ((next_index >> code_width) != 0)
      {
        code_width++;
        if (code_width > 12)
          {
            /*
              Did we overflow the max bit width?
            */
            code_width--;
            OutputCode(LZWClr);
            ResetLZWTable(table);
            next_index=LZWEod+1;
            code_width=9;
          }
        }
      last_code=(size_t) pixels[i];
  }
  /*
    Flush tables.
  */
  OutputCode(last_code);
  OutputCode(LZWEod);
  if (number_bits != 0)
    (void) WriteBlobByte(image,(unsigned char) (accumulator >> 24));
  table=(TableType *) RelinquishMagickMemory(table);
  return(MagickTrue);
}
