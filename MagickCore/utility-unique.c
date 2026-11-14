/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               U   U  TTTTT  IIIII  L      IIIII  TTTTT  Y   Y               %
%               U   U    T      I    L        I      T     Y Y                %
%               U   U    T      I    L        I      T      Y                 %
%               U   U    T      I    L        I      T      Y                 %
%                UUU     T    IIIII  LLLLL  IIIII    T      Y                 %
%                                                                             %
%                  U   U  N   N  IIIII   QQQ   U   U  EEEEE                   %
%                  U   U  NN  N    I    Q   Q  U   U  E                       %
%                  U   U  N N N    I    Q   Q  U   U  EEE                     %
%                  U   U  N  NN    I    Q  QQ  U   U  E                       %
%                   UUU   N   N  IIIII   QQQQ   UUU   EEEEE                   %
%                                                                             %
%                                                                             %
%                       MagickCore Unique File Methods                        %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/property.h"
#include "MagickCore/blob.h"
#include "MagickCore/color.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/magick-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#if defined(MAGICKCORE_HAVE_PROCESS_H)
#include <process.h>
#endif
#if defined(MAGICKCORE_HAVE_MACH_O_DYLD_H)
#include <mach-o/dyld.h>
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e U n i q u e F i l e n a m e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireUniqueFilename() replaces the contents of path by a unique path name.
%
%  The format of the AcquireUniqueFilename method is:
%
%      MagickBooleanType AcquireUniqueFilename(char *path)
%
%  A description of each parameter follows.
%
%   o  path:  Specifies a pointer to an array of characters.  The unique path
%      name is returned in this array.
%
*/
MagickExport MagickBooleanType AcquireUniqueFilename(char *path)
{
  int
    file;

  file=AcquireUniqueFileResource(path);
  if (file == -1)
    return(MagickFalse);
  file=close_utf8(file)-1;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e U n i q u e S ym b o l i c L i n k                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireUniqueSymbolicLink() creates a unique symbolic link to the specified
%  source path and returns MagickTrue on success otherwise MagickFalse.  If the
%  symlink() method fails or is not available, a unique file name is generated
%  and the source file copied to it.  When you are finished with the file, use
%  RelinquishUniqueFileResource() to destroy it.
%
%  The format of the AcquireUniqueSymbolicLink method is:
%
%      MagickBooleanType AcquireUniqueSymbolicLink(const char *source,
%        char destination)
%
%  A description of each parameter follows.
%
%   o  source:  the source path.
%
%   o  destination:  the destination path.
%
*/

static MagickBooleanType CopyFileDescriptor(const int source_file,
  const int destination_file,const char *destination)
{
  MagickBooleanType
    status;

  size_t
    length,
    quantum;

  ssize_t
    count;

  struct stat
    attributes;

  unsigned char
    *buffer;

  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(source_file,&attributes) == 0) && (attributes.st_size > 0))
    quantum=(size_t) MagickMin(attributes.st_size,MagickMaxBufferExtent);
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      (void) close_utf8(source_file);
      (void) close_utf8(destination_file);
      (void) RelinquishUniqueFileResource(destination);
      return(MagickFalse);
    }
  status=MagickTrue;
  for (length=0; ; )
  {
    count=(ssize_t) read(source_file,buffer,quantum);
    if (count <= 0)
      break;
    length=(size_t) count;
    count=(ssize_t) write(destination_file,buffer,length);
    if ((size_t) count != length)
      {
        (void) RelinquishUniqueFileResource(destination);
        status=MagickFalse;
        break;
      }
  }
  (void) close_utf8(destination_file);
  (void) close_utf8(source_file);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  return(status);
}

static MagickBooleanType CopyToUniqueFile(const char *source,
  char *destination)
{
  int
    destination_file,
    source_file;

  destination_file=AcquireUniqueFileResource(destination);
  if (destination_file == -1)
    return(MagickFalse);
  source_file=open_utf8(source,O_RDONLY | O_CLOEXEC | O_BINARY,0);
  if (source_file == -1)
    {
      (void) close_utf8(destination_file);
      (void) RelinquishUniqueFileResource(destination);
      return(MagickFalse);
    }
  return(CopyFileDescriptor(source_file,destination_file,destination));
}

MagickExport MagickBooleanType AcquireUniqueSymbolicLink(const char *source,
  char *destination)
{
  static ssize_t
    passes = -1;

  assert(source != (const char *) NULL);
  assert(destination != (char *) NULL);
#if defined(MAGICKCORE_HAVE_SYMLINK)
  {
    MagickBooleanType
      status;

    /*
      Does policy permit symbolic links?
    */
    status=IsRightsAuthorizedByName(SystemPolicyDomain,"symlink",(PolicyRights)
      (ReadPolicyRights | WritePolicyRights),"follow");
    if (passes == -1)
      passes=GetShredPasses();
    if ((passes == 0) && (status != MagickFalse))
      {
        (void) AcquireUniqueFilename(destination);
        (void) RelinquishUniqueFileResource(destination);
        if (*source == *DirectorySeparator)
          {
            if (symlink(source,destination) == 0)
              return(MagickTrue);
          }
        else
          {
            char
              path[MagickPathExtent];

            *path='\0';
            if (getcwd(path,MagickPathExtent) == (char *) NULL)
              return(MagickFalse);
            (void) ConcatenateMagickString(path,DirectorySeparator,
              MagickPathExtent);
            (void) ConcatenateMagickString(path,source,MagickPathExtent);
            if (symlink(path,destination) == 0)
              return(MagickTrue);
          }
      }
  }
#endif
  /*
    Copy file from source to destination.
  */
  return(CopyToUniqueFile(source,destination));
}
