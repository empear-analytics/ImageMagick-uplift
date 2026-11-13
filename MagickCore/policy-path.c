/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                  PPPP    OOO   L      IIIII   CCCC  Y   Y                   %
%                  P   P  O   O  L        I    C       Y Y                    %
%                  PPPP   O   O  L        I    C        Y                     %
%                  P      O   O  L        I    C        Y                     %
%                  P       OOO   LLLLL  IIIII   CCCC    Y                     %
%                                                                             %
%                         PPPP    AAA   TTTTT  H   H                          %
%                         P   P  A   A    T    H   H                          %
%                         PPPP   AAAAA    T    HHHHH                          %
%                         P      A   A    T    H   H                          %
%                         P      A   A    T    H   H                          %
%                                                                             %
%                                                                             %
%                       MagickCore Path Policy Methods                        %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/configure-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/linked-list-private.h"
#include "MagickCore/magick-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resource-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/stream-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#if defined(MAGICKCORE_XML_DELEGATE)
#  include <libxml/parser.h>
#  include <libxml/tree.h>
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I s P a t h A u t h o r i z e d                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPathAuthorized() determines if the specified path is authorized based on
%  the current policy settings.
%
%  The format of the IsPathAuthorized method is:
%
%      MagickBooleanType IsPathAuthorized(const PolicyRights rights, 
%        const char *path)
%
%  A description of each parameter follows.
%
%    o rights: The policy rights to check.
%
%    o path: The path to check.
%
*/

static inline MagickBooleanType IsPolicyPathSeparator(const char c)
{
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
  if ((c == '/') || (c == '\\'))
    return(MagickTrue);
#endif
  return((MagickBooleanType) (c == *DirectorySeparator));
}

static inline MagickBooleanType IsPolicyPathComponentCharacter(const char c)
{
  return((MagickBooleanType) ((c != '\0') &&
    (IsPolicyPathSeparator(c) == MagickFalse)));
}

static MagickBooleanType AppendPolicyPathRoot(char *partial,
  const size_t extent,ssize_t *offset,const char **p)
{
  if (IsPolicyPathSeparator(*(*p)) != MagickFalse)
    {
      /*
        Path starts with a directory separator, include it.
      */
      if (((*offset)+1) >= (ssize_t) extent)
        return(MagickFalse);
      partial[(*offset)++]=(*DirectorySeparator);
      (*p)++;
      partial[(*offset)]='\0';
    }
  return(MagickTrue);
}

static inline MagickBooleanType IsPathContainsSymlink(const char *path)
{
  char
    partial[MagickPathExtent];

  const char
    *p;

  ssize_t
    offset = 0;

  if (path == (const char *) NULL)
    return(MagickFalse);
  *partial='\0';
  p=path;
  if (AppendPolicyPathRoot(partial,sizeof(partial),&offset,&p) == MagickFalse)
    return(MagickFalse);
  while (*p != '\0')
  {
    char
      component[MagickPathExtent];

    ssize_t
      i = 0;

    /*
      Copy next component into a temporary buffer.
    */
    while ((IsPolicyPathComponentCharacter(*p) != MagickFalse) &&
           ((i+1) < (ssize_t) sizeof(component)))
      component[i++]=(*p++);
    component[i]='\0';
    if (i == 0)
      {
        /*
          skip repeated separators.
        */
        if (IsPolicyPathSeparator(*p) != MagickFalse)
          p++;
        continue;
      }
    if ((offset > 0) && (partial[offset-1] != *DirectorySeparator))
      {
        /*
          Append separator if needed.
        */
        if ((offset+1) >= (ssize_t) sizeof(partial))
          return MagickFalse;
        partial[offset++]=(*DirectorySeparator);
        partial[offset]='\0';
      }
    /*
      Append component.
    */
    if ((offset+i) >= (ssize_t) sizeof(partial))
      return(MagickFalse);
    (void) memcpy(partial+offset,component,i);
    offset+=i;
    partial[offset]='\0';
    /*
      Check whether this prefix is a symlink.
    */
    if ((*p != '\0') && (is_symlink_utf8(partial) != MagickFalse))
      return(MagickTrue);
    /*
      Skip separator.
    */
    if (IsPolicyPathSeparator(*p) != MagickFalse)
      p++;
  }
  return(MagickFalse);
}

MagickExport MagickBooleanType IsPathAuthorized(const PolicyRights rights,
  const char *path)
{
  MagickBooleanType symlink_follow_allowed = IsRightsAuthorizedByName(
    SystemPolicyDomain,"symlink",rights,"follow");
  MagickBooleanType status =
    (MagickBooleanType) ((IsRightsAuthorized(PathPolicyDomain,rights,path) != MagickFalse) &&
    ((symlink_follow_allowed != MagickFalse) ||
     (is_symlink_utf8(path) == MagickFalse)));
  if ((status != MagickFalse) && (symlink_follow_allowed == MagickFalse))
    if ((is_symlink_utf8(path) != MagickFalse) ||
        (IsPathContainsSymlink(path) != MagickFalse))
      status=MagickFalse;
  if (status != MagickFalse)
    status=IsFileResourceIdentityValid(path);
  return(status);
}
