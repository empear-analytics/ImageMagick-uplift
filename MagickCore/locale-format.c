/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  L       OOO    CCCC   AAA   L      EEEEE                   %
%                  L      O   O  C      A   A  L      E                       %
%                  L      O   O  C      AAAAA  L      EEE                     %
%                  L      O   O  C      A   A  L      E                       %
%                  LLLLL   OOO    CCCC  A   A  LLLLL  EEEEE                   %
%                                                                             %
%                  FFFFF   OOO   RRRR   M   M   AAA   TTTTT                   %
%                  F      O   O  R   R  MM MM  A   A    T                     %
%                  FFF    O   O  RRRR   M M M  AAAAA    T                     %
%                  F      O   O  R R    M   M  A   A    T                     %
%                  F       OOO   R  R   M   M  A   A    T                     %
%                                                                             %
%                                                                             %
%                      Formatted Output in the C Locale                       %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 2003                                   %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"

MagickExport ssize_t FormatLocaleFile(FILE *file,
  const char *magick_restrict format,...)
{
  ssize_t
    n;

  va_list
    operands;

  va_start(operands,format);
  n=FormatLocaleFileList(file,format,operands);
  va_end(operands);
  return(n);
}

MagickExport ssize_t FormatLocaleString(char *magick_restrict string,
  const size_t length,const char *magick_restrict format,...)
{
  ssize_t
    n;

  va_list
    operands;

  va_start(operands,format);
  n=FormatLocaleStringList(string,length,format,operands);
  va_end(operands);
  return(n);
}
