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
%                OOO   PPPP   TTTTT  IIIII   OOO   N   N  SSSSS               %
%               O   O  P   P    T      I    O   O  NN  N  SS                  %
%               O   O  PPPP     T      I    O   O  N N N   SSS                %
%               O   O  P        T      I    O   O  N  NN     SS               %
%                OOO   P        T    IIIII   OOO   N   N  SSSSS               %
%                                                                             %
%                                                                             %
%                      MagickCore Locale Option Methods                       %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t L o c a l e O p t i o n s                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLocaleOptions() returns any Magick configuration messages associated
%  with the specified filename.
%
%  The format of the GetLocaleOptions method is:
%
%      LinkedListInfo *GetLocaleOptions(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: the locale file tag.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport LinkedListInfo *GetLocaleOptions(const char *filename,
  ExceptionInfo *exception)
{
  char
    path[MagickPathExtent];

  const char
    *element;

  LinkedListInfo
    *messages,
    *paths;

  StringInfo
    *xml;

  assert(filename != (const char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  (void) CopyMagickString(path,filename,MagickPathExtent);
  /*
    Load XML from configuration files to linked-list.
  */
  messages=NewLinkedList(0);
  paths=GetConfigurePaths(filename,exception);
  if (paths != (LinkedListInfo *) NULL)
    {
      ResetLinkedListIterator(paths);
      element=(const char *) GetNextValueInLinkedList(paths);
      while (element != (const char *) NULL)
      {
        (void) FormatLocaleString(path,MagickPathExtent,"%s%s",element,
          filename);
        (void) LogMagickEvent(LocaleEvent,GetMagickModule(),
          "Searching for locale file: \"%s\"",path);
        xml=ConfigureFileToStringInfo(path);
        if (xml != (StringInfo *) NULL)
          (void) AppendValueToLinkedList(messages,xml);
        element=(const char *) GetNextValueInLinkedList(paths);
      }
      paths=DestroyLinkedList(paths,RelinquishMagickMemory);
    }
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
  {
    char
      *blob;

    blob=(char *) NTResourceToBlob(filename);
    if (blob != (char *) NULL)
      {
        xml=AcquireStringInfo(0);
        SetStringInfoLength(xml,strlen(blob)+1);
        SetStringInfoDatum(xml,(const unsigned char *) blob);
        blob=(char *) RelinquishMagickMemory(blob);
        SetStringInfoPath(xml,filename);
        (void) AppendValueToLinkedList(messages,xml);
      }
  }
#endif
  ResetLinkedListIterator(messages);
  return(messages);
}
