/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                             X   X  M   M  L                                 %
%                              X X   MM MM  L                                 %
%                               X    M M M  L                                 %
%                              X X   M   M  L                                 %
%                             X   X  M   M  LLLLL                             %
%                                                                             %
%                         TTTTT  RRRR   EEEEE  EEEEE                          %
%                           T    R   R  E      E                              %
%                           T    RRRR   EEE    EEE                            %
%                           T    R R    E      E                              %
%                           T    R  R   EEEEE  EEEEE                          %
%                                                                             %
%                         FFFFF  IIIII  L      EEEEE                          %
%                         F        I    L      E                              %
%                         FFF      I    L      EEE                            %
%                         F        I    L      E                              %
%                         F      IIIII  LLLLL  EEEEE                          %
%                                                                             %
%                                                                             %
%                       Read an XML File into a String                        %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               December 2004                                 %
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
%  This module implements the standard handy xml-tree methods for storing and
%  retrieving nodes and attributes from an XML string.
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e T o X M L                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToXML() returns the contents of a file as a XML string.
%
%  The format of the FileToXML method is:
%
%      char *FileToXML(const char *filename,const size_t extent)
%
%  A description of each parameter follows:
%
%    o filename: the filename.
%
%    o extent: Maximum length of the string.
%
*/
static inline MagickBooleanType IsUnseekableXMLFile(const int file,
  const MagickOffsetType offset)
{
  return(((file == fileno(stdin)) || (offset < 0) ||
    (offset != (MagickOffsetType) ((ssize_t) offset))) ? MagickTrue :
    MagickFalse);
}

static MagickBooleanType ReadXMLFileContent(int file,char *xml,
  const size_t length)
{
  size_t
    i;

  ssize_t
    count;

  void
    *map;

  map=MapBlob(file,ReadMode,0,length);
  if (map != (char *) NULL)
    {
      (void) memcpy(xml,map,length);
      (void) UnmapBlob(map,length);
    }
  else
    {
      (void) lseek(file,0,SEEK_SET);
      for (i=0; i < length; i+=(size_t) count)
      {
        count=MagickRead(file,xml+i,(size_t) MagickMin(length-i,(size_t)
          MagickMaxBufferExtent));
        if (count <= 0)
          break;
      }
      if (i < length)
        {
          file=close_utf8(file)-1;
          xml=(char *) RelinquishMagickMemory(xml);
          return(MagickFalse);
        }
    }
  return(MagickTrue);
}

MagickPrivate char *FileToXML(const char *filename,const size_t extent)
{
  char
    *xml;

  int
    file;

  MagickOffsetType
    offset;

  size_t
    i,
    length;

  ssize_t
    count;

  assert(filename != (const char *) NULL);
  length=0;
  file=fileno(stdin);
  if (LocaleCompare(filename,"-") != 0)
    file=open_utf8(filename,O_RDONLY | O_CLOEXEC | O_BINARY,0);
  if (file == -1)
    return((char *) NULL);
  offset=(MagickOffsetType) lseek(file,0,SEEK_END);
  count=0;
  if (IsUnseekableXMLFile(file,offset) != MagickFalse)
    {
      size_t
        quantum;

      struct stat
        file_stats;

      /*
        Stream is not seekable.
      */
      offset=(MagickOffsetType) lseek(file,0,SEEK_SET);
      quantum=(size_t) MagickMaxBufferExtent;
      if ((fstat(file,&file_stats) == 0) && (file_stats.st_size > 0))
        quantum=(size_t) MagickMin(file_stats.st_size,MagickMaxBufferExtent);
      xml=(char *) AcquireQuantumMemory(quantum,sizeof(*xml));
      for (i=0; xml != (char *) NULL; i+=(size_t) count)
      {
        count=MagickRead(file,xml+i,quantum);
        if (count <= 0)
          break;
        if (~((size_t) i) < (quantum+1))
          {
            xml=(char *) RelinquishMagickMemory(xml);
            break;
          }
        xml=(char *) ResizeQuantumMemory(xml,i+quantum+1,sizeof(*xml));
        if ((i+(size_t) count) >= extent)
          break;
      }
      if (LocaleCompare(filename,"-") != 0)
        file=close_utf8(file);
      if (xml == (char *) NULL)
        return((char *) NULL);
      if (file == -1)
        {
          xml=(char *) RelinquishMagickMemory(xml);
          return((char *) NULL);
        }
      length=MagickMin(i+(size_t) count,extent);
      xml[length]='\0';
      return(xml);
    }
  length=(size_t) MagickMin(offset,(MagickOffsetType) extent);
  xml=(char *) NULL;
  if (~length >= (MagickPathExtent-1))
    xml=(char *) AcquireQuantumMemory(length+MagickPathExtent,sizeof(*xml));
  if (xml == (char *) NULL)
    {
      file=close_utf8(file);
      return((char *) NULL);
    }
  if (ReadXMLFileContent(file,xml,length) == MagickFalse)
    return((char *) NULL);
  xml[length]='\0';
  if (LocaleCompare(filename,"-") != 0)
    file=close_utf8(file);
  if (file == -1)
    xml=(char *) RelinquishMagickMemory(xml);
  return(xml);
}
