/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        EEEEE  X   X   CCCC  EEEEE  PPPP   TTTTT  IIIII   OOO   N   N        %
%        E       X X   C      E      P   P    T      I    O   O  NN  N        %
%        EEE      X    C      EEE    PPPP     T      I    O   O  N N N        %
%        E       X X   C      E      P        T      I    O   O  N  NN        %
%        EEEEE  X   X   CCCC  EEEEE  P        T    IIIII   OOO   N   N        %
%                                                                             %
%                      TTTTT  H   H  RRRR    OOO   W   W                      %
%                        T    H   H  R   R  O   O  W   W                      %
%                        T    HHHHH  RRRR   O   O  W W W                      %
%                        T    H   H  R R    O   O  WW WW                      %
%                        T    H   H  R  R    OOO   W   W                      %
%                                                                             %
%                                                                             %
%                    Throw an Exception from a Format List                    %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                                July 1993                                    %
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
#include "MagickCore/client.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T h r o w M a g i c k E x c e p t i o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ThrowMagickException logs an exception as determined by the log
%  configuration file.  If an error occurs, MagickFalse is returned
%  otherwise MagickTrue.
%
%  The format of the ThrowMagickException method is:
%
%      MagickBooleanType ThrowFileException(ExceptionInfo *exception,
%        const char *module,const char *function,const size_t line,
%        const ExceptionType severity,const char *tag,const char *format,...)
%
%  A description of each parameter follows:
%
%    o exception: the exception info.
%
%    o filename: the source module filename.
%
%    o function: the function name.
%
%    o line: the line number of the source module.
%
%    o severity: Specifies the numeric error category.
%
%    o tag: the locale tag.
%
%    o format: the output format.
%
*/

MagickExport MagickBooleanType ThrowMagickExceptionList(
  ExceptionInfo *exception,const char *module,const char *function,
  const size_t line,const ExceptionType severity,const char *tag,
  const char *format,va_list operands)
{
  char
    message[MagickPathExtent],
    path[MagickPathExtent],
    reason[MagickPathExtent];

  const char
    *locale,
    *type;

  int
    n;

  MagickBooleanType
    status;

  size_t
    length;

  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  locale=GetLocaleExceptionMessage(severity,tag);
  (void) CopyMagickString(reason,locale,MagickPathExtent);
  (void) ConcatenateMagickString(reason," ",MagickPathExtent);
  length=strlen(reason);
  n=vsnprintf(reason+length,MagickPathExtent-length,format,operands);
  if (n < 0)
    reason[MagickPathExtent-1]='\0';
  status=LogMagickEvent(ExceptionEvent,module,function,line,"%s",reason);
  GetPathComponent(module,TailPath,path);
  type="undefined";
  if ((severity >= WarningException) && (severity < ErrorException))
    type="warning";
  if ((severity >= ErrorException) && (severity < FatalErrorException))
    type="error";
  if (severity >= FatalErrorException)
    type="fatal";
  (void) FormatLocaleString(message,MagickPathExtent,"%s @ %s/%s/%s/%.17g",
    reason,type,path,function,(double) line);
  (void) ThrowException(exception,severity,message,(char *) NULL);
  return(status);
}
