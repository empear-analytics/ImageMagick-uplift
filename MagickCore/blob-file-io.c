/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                         BBBB   L       OOO   BBBB                           %
%                         B   B  L      O   O  B   B                          %
%                         BBBB   L      O   O  BBBB                           %
%                         B   B  L      O   O  B   B                          %
%                         BBBB   LLLLL   OOO   BBBB                           %
%                                                                             %
%                         FFFFF  IIIII  L      EEEEE                          %
%                         F        I    L      E                              %
%                         FFF      I    L      EEE                            %
%                         F        I    L      E                              %
%                         F      IIIII  LLLLL  EEEEE                          %
%                                                                             %
%                                IIIII   OOO                                  %
%                                  I    O   O                                 %
%                                  I    O   O                                 %
%                                  I    O   O                                 %
%                                IIIII   OOO                                  %
%                                                                             %
%                                                                             %
%                    MagickCore Blob and File Copy Methods                    %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1999                                   %
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
#ifdef __VMS
#include  <types.h>
#include  <mman.h>
#endif
#include "MagickCore/studio.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/client.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#if defined(MAGICKCORE_ZLIB_DELEGATE)
#include "zlib.h"
#endif
#if defined(MAGICKCORE_BZLIB_DELEGATE)
#include "bzlib.h"
#endif

#include "MagickCore/blob-info-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   B l o b T o F i l e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlobToFile() writes a blob to a file.  It returns MagickFalse if an error
%  occurs otherwise MagickTrue.
%
%  The format of the BlobToFile method is:
%
%       MagickBooleanType BlobToFile(char *filename,const void *blob,
%         const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: Write the blob to this file. The filename buffer length must
%      be a minimum of MagickPathExtent characters.
%
%    o blob: the address of a blob.
%
%    o length: This length in bytes of the blob.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType BlobToFile(char *filename,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  int
    file;

  size_t
    i;

  ssize_t
    count;

  assert(filename != (const char *) NULL);
  assert(blob != (const void *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  if (*filename == '\0')
    file=AcquireUniqueFileResource(filename);
  else
    file=open_utf8(filename,O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL | O_BINARY,P_MODE);
  if (file == -1)
    {
      ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
      return(MagickFalse);
    }
  for (i=0; i < length; i+=(size_t) count)
  {
    count=MagickWrite(file,(const char *) blob+i,MagickMin(length-i,(size_t)
      MagickMaxBufferExtent));
    if (count <= 0)
      break;
  }
  file=close_utf8(file);
  if ((file == -1) || (i < length))
    {
      ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e T o B l o b                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToBlob() returns the contents of a file as a buffer terminated with
%  the '\0' character.  The length of the buffer (not including the extra
%  terminating '\0' character) is returned via the 'length' parameter.  Free
%  the buffer with RelinquishMagickMemory().
%
%  The format of the FileToBlob method is:
%
%      void *FileToBlob(const char *filename,const size_t extent,
%        size_t *length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o blob:  FileToBlob() returns the contents of a file as a blob.  If
%      an error occurs NULL is returned.
%
%    o filename: the filename.
%
%    o extent:  The maximum length of the blob.
%
%    o length: On return, this reflects the actual length of the blob.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsUnseekableBlobFile(const int file,
  const MagickOffsetType offset)
{
  return(((file == fileno(stdin)) || (offset < 0) ||
    (offset != (MagickOffsetType) ((ssize_t) offset))) ? MagickTrue :
    MagickFalse);
}

MagickExport void *FileToBlob(const char *filename,const size_t extent,
  size_t *length,ExceptionInfo *exception)
{
  int
    file;

  MagickBooleanType
    status;

  MagickOffsetType
    offset;

  size_t
    i;

  ssize_t
    count;

  struct stat
    attributes;

  unsigned char
    *blob;

  void
    *map;

  assert(filename != (const char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  *length=0;
  if (IsPathAuthorized(ReadPolicyRights,filename) == MagickFalse)
    ThrowPolicyException(filename,NULL);
  file=fileno(stdin);
  if (LocaleCompare(filename,"-") != 0)
    {
      int
        flags = O_RDONLY | O_CLOEXEC | O_BINARY;

      status=GetPathAttributes(filename,&attributes);
      if ((status == MagickFalse) || (S_ISDIR(attributes.st_mode) != 0))
        {
          ThrowFileException(exception,BlobError,"UnableToReadBlob",filename);
          return(NULL);
        }
      file=open_utf8(filename,flags,0);
    }
  if (file == -1)
    {
      ThrowFileException(exception,BlobError,"UnableToOpenFile",filename);
      return(NULL);
    }
  if (IsPathAuthorized(ReadPolicyRights,filename) == MagickFalse)
    {
      file=close_utf8(file)-1;
      ThrowPolicyException(filename,NULL);
    }
  offset=(MagickOffsetType) lseek(file,0,SEEK_END);
  count=0;
  if (IsUnseekableBlobFile(file,offset) != MagickFalse)
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
      blob=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*blob));
      for (i=0; blob != (unsigned char *) NULL; i+=(size_t) count)
      {
        count=MagickRead(file,blob+i,quantum);
        if (count <= 0)
          break;
        if (~i < ((size_t) count+quantum+1))
          {
            blob=(unsigned char *) RelinquishMagickMemory(blob);
            break;
          }
        blob=(unsigned char *) ResizeQuantumMemory(blob,i+(size_t) count+
          quantum+1,sizeof(*blob));
        if ((i+(size_t) count) >= extent)
          break;
      }
      if (LocaleCompare(filename,"-") != 0)
        file=close_utf8(file);
      if (blob == (unsigned char *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
          return(NULL);
        }
      if ((count < 0) || (file == -1))
        {
          blob=(unsigned char *) RelinquishMagickMemory(blob);
          ThrowFileException(exception,BlobError,"UnableToReadBlob",filename);
          return(NULL);
        }
      *length=(size_t) MagickMin(i+(size_t) count,extent);
      blob[*length]='\0';
      return(blob);
    }
  *length=(size_t) MagickMin(offset,(MagickOffsetType)
    MagickMin(extent,(size_t) MAGICK_SSIZE_MAX));
  blob=(unsigned char *) NULL;
  if (~(*length) >= (MagickPathExtent-1))
    blob=(unsigned char *) AcquireQuantumMemory(*length+MagickPathExtent,
      sizeof(*blob));
  if (blob == (unsigned char *) NULL)
    {
      file=close_utf8(file);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
      return(NULL);
    }
  map=MapBlob(file,ReadMode,0,*length);
  if (map != (unsigned char *) NULL)
    {
      (void) memcpy(blob,map,*length);
      (void) UnmapBlob(map,*length);
    }
  else
    {
      (void) lseek(file,0,SEEK_SET);
      for (i=0; i < *length; i+=(size_t) count)
      {
        count=MagickRead(file,blob+i,(size_t) MagickMin(*length-i,(size_t)
          MagickMaxBufferExtent));
        if (count <= 0)
          break;
      }
      if (i < *length)
        {
          file=close_utf8(file)-1;
          blob=(unsigned char *) RelinquishMagickMemory(blob);
          ThrowFileException(exception,BlobError,"UnableToReadBlob",filename);
          return(NULL);
        }
    }
  blob[*length]='\0';
  if (LocaleCompare(filename,"-") != 0)
    file=close_utf8(file);
  if (file == -1)
    {
      blob=(unsigned char *) RelinquishMagickMemory(blob);
      ThrowFileException(exception,BlobError,"UnableToReadBlob",filename);
    }
  return(blob);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m a g e T o F i l e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImageToFile() writes an image to a file.  It returns MagickFalse if an error
%  occurs otherwise MagickTrue.
%
%  The format of the ImageToFile method is:
%
%       MagickBooleanType ImageToFile(Image *image,char *filename,
%         ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o filename: Write the image to this file.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ImageToFile(Image *image,char *filename,
  ExceptionInfo *exception)
{
  int
    file;

  const unsigned char
    *p;

  size_t
    i;

  size_t
    length,
    quantum;

  ssize_t
    count;

  struct stat
    file_stats;

  unsigned char
    *buffer;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  assert(filename != (const char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  if (*filename == '\0')
    file=AcquireUniqueFileResource(filename);
  else
    if (LocaleCompare(filename,"-") == 0)
      file=fileno(stdout);
    else
      file=open_utf8(filename,O_RDWR | O_CLOEXEC | O_CREAT | O_EXCL | O_BINARY,P_MODE);
  if (file == -1)
    {
      ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
      return(MagickFalse);
    }
  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(file,&file_stats) == 0) && (file_stats.st_size > 0))
    quantum=(size_t) MagickMin(file_stats.st_size,MagickMaxBufferExtent);
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      file=close_utf8(file)-1;
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationError","`%s'",filename);
      return(MagickFalse);
    }
  length=0;
  p=(const unsigned char *) ReadBlobStream(image,quantum,buffer,&count);
  for (i=0; count > 0; )
  {
    length=(size_t) count;
    for (i=0; i < length; i+=(size_t) count)
    {
      count=MagickWrite(file,p+i,(size_t) (length-i));
      if (count <= 0)
        break;
    }
    if (i < length)
      break;
    p=(const unsigned char *) ReadBlobStream(image,quantum,buffer,&count);
  }
  if (LocaleCompare(filename,"-") != 0)
    file=close_utf8(file);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  if ((file == -1) || (i < length))
    {
      if (file != -1)
        file=close_utf8(file);
      ThrowFileException(exception,BlobError,"UnableToWriteBlob",filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}
