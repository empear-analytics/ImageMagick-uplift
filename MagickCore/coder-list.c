/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                       CCCC   OOO   DDDD   EEEEE  RRRR                       %
%                      C      O   O  D   D  E      R   R                      %
%                      C      O   O  D   D  EEE    RRRR                       %
%                      C      O   O  D   D  E      R R                        %
%                       CCCC   OOO   DDDD   EEEEE  R  R                       %
%                                                                             %
%                         L      IIIII  SSSSS  TTTTT                          %
%                         L        I    SS       T                            %
%                         L        I     SSS     T                            %
%                         L        I       SS    T                            %
%                         LLLLL  IIIII  SSSSS    T                            %
%                                                                             %
%                                                                             %
%                           List Coder Information                            %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 May 2001                                    %
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
#include "MagickCore/coder.h"
#include "MagickCore/coder-private.h"
#include "MagickCore/configure.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/option.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "coders/coders.h"

static void PrintCoderPathHeader(FILE *file,const CoderInfo **coder_info,
  const ssize_t i)
{
  if (coder_info[i]->path != (char *) NULL)
    (void) FormatLocaleFile(file,"\nPath: %s\n\n",coder_info[i]->path);
  (void) FormatLocaleFile(file,"Magick          Coder\n");
  (void) FormatLocaleFile(file,
    "-------------------------------------------------"
    "------------------------------\n");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t C o d e r I n f o                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListCoderInfo() lists the coder info to a file.
%
%  The format of the ListCoderInfo coder is:
%
%      MagickBooleanType ListCoderInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ListCoderInfo(FILE *file,
  ExceptionInfo *exception)
{
  const char
    *path;

  const CoderInfo
    **coder_info;

  ssize_t
    i;

  size_t
    number_coders;

  ssize_t
    j;

  if (file == (const FILE *) NULL)
    file=stdout;
  coder_info=GetCoderInfoList("*",&number_coders,exception);
  if (coder_info == (const CoderInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (ssize_t) number_coders; i++)
  {
    if (coder_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,coder_info[i]->path) != 0))
      PrintCoderPathHeader(file,coder_info,i);
    path=coder_info[i]->path;
    (void) FormatLocaleFile(file,"%s",coder_info[i]->magick);
    for (j=(ssize_t) strlen(coder_info[i]->magick); j <= 15; j++)
      (void) FormatLocaleFile(file," ");
    if (coder_info[i]->name != (char *) NULL)
      (void) FormatLocaleFile(file,"%s",coder_info[i]->name);
    (void) FormatLocaleFile(file,"\n");
  }
  coder_info=(const CoderInfo **) RelinquishMagickMemory((void *) coder_info);
  (void) fflush(file);
  return(MagickTrue);
}
