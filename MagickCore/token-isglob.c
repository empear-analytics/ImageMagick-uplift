/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      TTTTT   OOO   K   K  EEEEE  N   N                      %
%                        T    O   O  K  K   E      NN  N                      %
%                        T    O   O  KKK    EEE    N N N                      %
%                        T    O   O  K  K   E      N  NN                      %
%                        T     OOO   K   K  EEEEE  N   N                      %
%                                                                             %
%                  IIIII  SSSSS   GGGG  L       OOO   BBBB                    %
%                    I    SS     G      L      O   O  B   B                   %
%                    I     SSS   G  GG  L      O   O  BBBB                    %
%                    I       SS  G   G  L      O   O  B   B                   %
%                  IIIII  SSSSS   GGG   LLLLL   OOO   BBBB                    %
%                                                                             %
%                                                                             %
%                       Test a Path for Glob Characters                       %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                              January 1993                                   %
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
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     I s G l o b                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGlob() returns MagickTrue if the path specification contains a globbing
%  pattern.
%
%  The format of the IsGlob method is:
%
%      MagickBooleanType IsGlob(const char *geometry)
%
%  A description of each parameter follows:
%
%    o path: the path.
%
*/
MagickPrivate MagickBooleanType IsGlob(const char *path)
{
  MagickBooleanType
    status = MagickFalse;

  const char
    *p;

  if (IsPathAccessible(path) != MagickFalse)
    return(MagickFalse);
  for (p=path; *p != '\0'; p++)
  {
    switch (*p)
    {
      case '*':
      case '?':
      case '{':
      case '}':
      case '[':
      case ']':
      {
        status=MagickTrue;
        break;
      }
      default:
        break;
    }
  }
  return(status);
}
