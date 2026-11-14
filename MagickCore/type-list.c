/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                         TTTTT  Y   Y  PPPP   EEEEE                          %
%                           T     Y Y   P   P  E                              %
%                           T      Y    PPPP   EEE                            %
%                           T      Y    P      E                              %
%                           T      Y    P      EEEEE                          %
%                                                                             %
%                         L      IIIII  SSSSS  TTTTT                          %
%                         L        I    SS       T                            %
%                         L        I     SSS     T                            %
%                         L        I       SS    T                            %
%                         LLLLL  IIIII  SSSSS    T                            %
%                                                                             %
%                                                                             %
%                        MagickCore List Type Methods                         %
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
#include "MagickCore/configure.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-feature.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/type.h"
#include "MagickCore/type-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t T y p e I n f o                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListTypeInfo() lists the fonts to a file.
%
%  The format of the ListTypeInfo method is:
%
%      MagickBooleanType ListTypeInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsNewTypePath(const char *path,
  const TypeInfo **type_info,const ssize_t i)
{
  return((MagickBooleanType) (((path == (const char *) NULL) ||
    (LocaleCompare(path,type_info[i]->path) != 0)) &&
    (type_info[i]->path != (char *) NULL)));
}

static void PrintTypeInfo(FILE *file,const TypeInfo **type_info,
  const ssize_t i)
{
  const char
    *family,
    *glyphs,
    *metrics,
    *name,
    *stretch,
    *style;

  name="not defined";
  if (type_info[i]->name != (char *) NULL)
    name=type_info[i]->name;
  family="not defined";
  if (type_info[i]->family != (char *) NULL)
    family=type_info[i]->family;
  style=CommandOptionToMnemonic(MagickStyleOptions,type_info[i]->style);
  stretch=CommandOptionToMnemonic(MagickStretchOptions,type_info[i]->stretch);
  metrics="not defined";
  if (type_info[i]->metrics != (char *) NULL)
    metrics=type_info[i]->metrics;
  glyphs="not defined";
  if (type_info[i]->glyphs != (char *) NULL)
    glyphs=type_info[i]->glyphs;
  (void) FormatLocaleFile(file,"  Font: %s\n",name);
  (void) FormatLocaleFile(file,"    family: %s\n",family);
  (void) FormatLocaleFile(file,"    style: %s\n",style);
  (void) FormatLocaleFile(file,"    stretch: %s\n",stretch);
  (void) FormatLocaleFile(file,"    weight: %.17g\n",(double)
    type_info[i]->weight);
  (void) FormatLocaleFile(file,"    metrics: %s\n",metrics);
  (void) FormatLocaleFile(file,"    glyphs: %s\n",glyphs);
  (void) FormatLocaleFile(file,"    index: %d\n",(int)
    type_info[i]->face);
}

MagickExport MagickBooleanType ListTypeInfo(FILE *file,ExceptionInfo *exception)
{
  const char
    *path;

  const TypeInfo
    **type_info;

  ssize_t
    i;

  size_t
    number_fonts;

  if (file == (FILE *) NULL)
    file=stdout;
  number_fonts=0;
  type_info=GetTypeInfoList("*",&number_fonts,exception);
  if (type_info == (const TypeInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (ssize_t) number_fonts; i++)
  {
    if (type_info[i]->stealth != MagickFalse)
      continue;
    if (IsNewTypePath(path,type_info,i) != MagickFalse)
      (void) FormatLocaleFile(file,"\nPath: %s\n",type_info[i]->path);
    path=type_info[i]->path;
    PrintTypeInfo(file,type_info,i);
  }
  (void) fflush(file);
  type_info=(const TypeInfo **) RelinquishMagickMemory((void *) type_info);
  return(MagickTrue);
}
