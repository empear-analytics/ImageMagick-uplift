/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS  TTTTT  RRRR   IIIII  N   N   GGGG                   %
%                  SS       T    R   R    I    NN  N  G                       %
%                   SSS     T    RRRR     I    N N N  G  GG                   %
%                     SS    T    R R      I    N  NN  G   G                   %
%                  SSSSS    T    R  R   IIIII  N   N   GGG                    %
%                                                                             %
%                         IIIII  N   N  FFFFF   OOO                           %
%                           I    NN  N  F      O   O                          %
%                           I    N N N  FFF    O   O                          %
%                           I    N  NN  F      O   O                          %
%                         IIIII  N   N  F       OOO                           %
%                                                                             %
%                                                                             %
%                             StringInfo Methods                              %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               August 2003                                   %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the license.  You may  %
%  obtain a copy of the license at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  unless required by applicable law or agreed to in writing, software        %
%  distributed under the license is distributed on an "as is" basis,          %
%  without warranties or conditions of any kind, either express or implied.   %
%  See the license for the specific language governing permissions and        %
%  limitations under the license.                                             %
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
#include "MagickCore/blob-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/property.h"
#include "MagickCore/policy.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resource-private.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/utility-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e S t r i n g I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireStringInfo() allocates the StringInfo structure.
%
%  The format of the AcquireStringInfo method is:
%
%      StringInfo *AcquireStringInfo(const size_t length)
%
%  A description of each parameter follows:
%
%    o length: the string length.
%
*/

static StringInfo *AcquireStringInfoContainer(void)
{
  StringInfo
    *string_info;

  string_info=(StringInfo *) AcquireCriticalMemory(sizeof(*string_info));
  (void) memset(string_info,0,sizeof(*string_info));
  string_info->signature=MagickCoreSignature;
  return(string_info);
}

MagickExport StringInfo *AcquireStringInfo(const size_t length)
{
  StringInfo
    *string_info;

  string_info=AcquireStringInfoContainer();
  string_info->length=length;
  if (~string_info->length >= (MagickPathExtent-1))
    string_info->datum=(unsigned char *) AcquireQuantumMemory(
      string_info->length+MagickPathExtent,sizeof(*string_info->datum));
  if (string_info->datum == (unsigned char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) memset(string_info->datum,0,(length+MagickPathExtent)*
    sizeof(*string_info->datum));
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B l o b T o S t r i n g I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlobToStringInfo() returns the contents of a blob as a StringInfo structure
%  with MagickPathExtent extra space.
%
%  The format of the BlobToStringInfo method is:
%
%      StringInfo *BlobToStringInfo(const void *blob,const size_t length)
%
%  A description of each parameter follows:
%
%    o blob: the blob.
%
%    o length: the length of the blob.
%
*/
MagickExport StringInfo *BlobToStringInfo(const void *blob,const size_t length)
{
  StringInfo
    *string_info;

  if (~length < MagickPathExtent)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  string_info=AcquireStringInfoContainer();
  string_info->length=length;
  string_info->datum=(unsigned char *) AcquireQuantumMemory(length+
    MagickPathExtent,sizeof(*string_info->datum));
  if (string_info->datum == (unsigned char *) NULL)
    {
      string_info=DestroyStringInfo(string_info);
      return((StringInfo *) NULL);
    }
  if (blob != (const void *) NULL)
    (void) memcpy(string_info->datum,blob,length);
  else
    (void) memset(string_info->datum,0,length*sizeof(*string_info->datum));
  (void) memset(string_info->datum+length,0,MagickPathExtent*
    sizeof(*string_info->datum));
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e S t r i n g I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneStringInfo() clones a copy of the StringInfo structure.
%
%  The format of the CloneStringInfo method is:
%
%      StringInfo *CloneStringInfo(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport StringInfo *CloneStringInfo(const StringInfo *string_info)
{
  StringInfo
    *clone_info;

  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  clone_info=AcquireStringInfo(string_info->length);
  (void) CloneString(&clone_info->path,string_info->path);
  (void) CloneString(&clone_info->name,string_info->name);
  if (string_info->length != 0)
    (void) memcpy(clone_info->datum,string_info->datum,string_info->length+1);
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p a r e S t r i n g I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompareStringInfo() compares the two datums target and source.  It returns
%  an integer less than, equal to, or greater than zero if target is found,
%  respectively, to be less than, to match, or be greater than source.
%
%  The format of the CompareStringInfo method is:
%
%      int CompareStringInfo(const StringInfo *target,const StringInfo *source)
%
%  A description of each parameter follows:
%
%    o target: the target string.
%
%    o source: the source string.
%
*/

MagickExport int CompareStringInfo(const StringInfo *target,
  const StringInfo *source)
{
  int
    status;

  assert(target != (StringInfo *) NULL);
  assert(target->signature == MagickCoreSignature);
  assert(source != (StringInfo *) NULL);
  assert(source->signature == MagickCoreSignature);
  status=memcmp(target->datum,source->datum,MagickMin(target->length,
    source->length));
  if (status != 0)
    return(status);
  if (target->length == source->length)
    return(0);
  return(target->length < source->length ? -1 : 1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n c a t e n a t e S t r i n g I n f o                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConcatenateStringInfo() concatenates the source string to the destination
%  string.
%
%  The format of the ConcatenateStringInfo method is:
%
%      void ConcatenateStringInfo(StringInfo *string_info,
%        const StringInfo *source)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
%    o source: the source string.
%
*/
MagickExport void ConcatenateStringInfo(StringInfo *string_info,
  const StringInfo *source)
{
  size_t
    length;

  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  assert(source != (const StringInfo *) NULL);
  length=string_info->length;
  if (~length < source->length)
    ThrowFatalException(ResourceLimitFatalError,"UnableToConcatenateString");
  length+=source->length;
  if (~length < MagickPathExtent)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  if (string_info->datum == (unsigned char *) NULL)
    string_info->datum=(unsigned char *) AcquireQuantumMemory(length+
      MagickPathExtent,sizeof(*string_info->datum));
  else
    string_info->datum=(unsigned char *) ResizeQuantumMemory(
      string_info->datum,OverAllocateMemory(length+MagickPathExtent),
      sizeof(*string_info->datum));
  if (string_info->datum == (unsigned char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) memcpy(string_info->datum+string_info->length,source->datum,
    source->length);
  string_info->length=length;
}

static MagickBooleanType ReadFileIntoBuffer(const int file,char *string,
  const size_t length)
{
  size_t
    i;

  ssize_t
    count;

  void
    *map;

  map=MapBlob(file,ReadMode,0,length);
  if (map != (void *) NULL)
    {
      (void) memcpy(string,map,length);
      (void) UnmapBlob(map,length);
      return(MagickTrue);
    }
  (void) lseek(file,0,SEEK_SET);
  for (i=0; i < length; i+=(size_t) count)
  {
    count=MagickRead(file,string+i,(size_t) MagickMin(length-i,(size_t)
      MagickMaxBufferExtent));
    if (count <= 0)
      break;
  }
  return(i < length ? MagickFalse : MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n f i g u r e F i l e T o S t r i n g I n f o                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConfigureFileToStringInfo() returns the contents of a configure file as a
%  string.
%
%  The format of the ConfigureFileToStringInfo method is:
%
%      StringInfo *ConfigureFileToStringInfo(const char *filename)
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: the filename.
%
*/
MagickExport StringInfo *ConfigureFileToStringInfo(const char *filename)
{
  char
    *string;

  int
    file;

  MagickOffsetType
    offset;

  size_t
    length;

  StringInfo
    *string_info;

  assert(filename != (const char *) NULL);
  file=open_utf8(filename,O_RDONLY | O_CLOEXEC | O_BINARY,0);
  if (file == -1)
    return((StringInfo *) NULL);
  offset=(MagickOffsetType) lseek(file,0,SEEK_END);
  if ((offset < 0) || (offset != (MagickOffsetType) ((ssize_t) offset)))
    {
      file=close_utf8(file)-1;
      return((StringInfo *) NULL);
    }
  length=(size_t) offset;
  string=(char *) NULL;
  if (~length >= (MagickPathExtent-1))
    string=(char *) AcquireQuantumMemory(length+MagickPathExtent,
      sizeof(*string));
  if (string == (char *) NULL)
    {
      file=close_utf8(file)-1;
      return((StringInfo *) NULL);
    }
  if (ReadFileIntoBuffer(file,string,length) == MagickFalse)
    {
      file=close_utf8(file)-1;
      string=DestroyString(string);
      return((StringInfo *) NULL);
    }
  string[length]='\0';
  file=close_utf8(file)-1;
  string_info=AcquireStringInfoContainer();
  string_info->path=ConstantString(filename);
  string_info->length=length;
  string_info->datum=(unsigned char *) string;
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S t r i n g I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyStringInfo() destroys memory associated with the StringInfo structure.
%
%  The format of the DestroyStringInfo method is:
%
%      StringInfo *DestroyStringInfo(StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport StringInfo *DestroyStringInfo(StringInfo *string_info)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  if (string_info->datum != (unsigned char *) NULL)
    string_info->datum=(unsigned char *) RelinquishMagickMemory(
      string_info->datum);
  if (string_info->name != (char *) NULL)
    string_info->name=DestroyString(string_info->name);
  if (string_info->path != (char *) NULL)
    string_info->path=DestroyString(string_info->path);
  string_info->signature=(~MagickCoreSignature);
  string_info=(StringInfo *) RelinquishMagickMemory(string_info);
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e T o S t r i n g I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToStringInfo() returns the contents of a file as a string.
%
%  The format of the FileToStringInfo method is:
%
%      StringInfo *FileToStringInfo(const char *filename,const size_t extent,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: the filename.
%
%    o extent: Maximum length of the string.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport StringInfo *FileToStringInfo(const char *filename,
  const size_t extent,ExceptionInfo *exception)
{
  StringInfo
    *string_info;

  assert(filename != (const char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  string_info=AcquireStringInfoContainer();
  string_info->path=ConstantString(filename);
  string_info->datum=(unsigned char *) FileToBlob(filename,extent,
    &string_info->length,exception);
  if (string_info->datum == (unsigned char *) NULL)
    {
      string_info=DestroyStringInfo(string_info);
      return((StringInfo *) NULL);
    }
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t S t r i n g I n f o D a t u m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetStringInfoDatum() returns the datum associated with the string.
%
%  The format of the GetStringInfoDatum method is:
%
%      unsigned char *GetStringInfoDatum(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport unsigned char *GetStringInfoDatum(const StringInfo *string_info)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  return(string_info->datum);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t S t r i n g I n f o L e n g t h                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetStringInfoLength() returns the string length.
%
%  The format of the GetStringInfoLength method is:
%
%      size_t GetStringInfoLength(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport size_t GetStringInfoLength(const StringInfo *string_info)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  return(string_info->length);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t S t r i n g I n f o N a m e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetStringInfoName() returns the name associated with the string.
%
%  The format of the GetStringInfoName method is:
%
%      const char *GetStringInfoName(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport const char *GetStringInfoName(const StringInfo *string_info)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  return(string_info->name);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t S t r i n g I n f o P a t h                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetStringInfoPath() returns the path associated with the string.
%
%  The format of the GetStringInfoPath method is:
%
%      const char *GetStringInfoPath(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport const char *GetStringInfoPath(const StringInfo *string_info)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  return(string_info->path);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t S t r i n g I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetStringInfo() reset the string to all null bytes.
%
%  The format of the ResetStringInfo method is:
%
%      void ResetStringInfo(StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport void ResetStringInfo(StringInfo *string_info)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  (void) memset(string_info->datum,0,string_info->length);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t S t r i n g I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStringInfo() copies the source string to the destination string.
%
%  The format of the SetStringInfo method is:
%
%      void SetStringInfo(StringInfo *string_info,const StringInfo *source)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
%    o source: the source string.
%
*/
MagickExport void SetStringInfo(StringInfo *string_info,
  const StringInfo *source)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  assert(source != (StringInfo *) NULL);
  assert(source->signature == MagickCoreSignature);
  if (string_info->length == 0)
    return;
  (void) memset(string_info->datum,0,string_info->length);
  (void) memcpy(string_info->datum,source->datum,MagickMin(string_info->length,
    source->length));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t S t r i n g I n f o D a t u m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStringInfoDatum() copies bytes from the source string for the length of
%  the destination string.
%
%  The format of the SetStringInfoDatum method is:
%
%      void SetStringInfoDatum(StringInfo *string_info,
%        const unsigned char *source)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
%    o source: the source string.
%
*/
MagickExport void SetStringInfoDatum(StringInfo *string_info,
  const unsigned char *source)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  if (string_info->length != 0)
    (void) memcpy(string_info->datum,source,string_info->length);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t S t r i n g I n f o L e n g t h                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStringInfoLength() set the string length to the specified value.
%
%  The format of the SetStringInfoLength method is:
%
%      void SetStringInfoLength(StringInfo *string_info,const size_t length)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
%    o length: the string length.
%
*/
MagickExport void SetStringInfoLength(StringInfo *string_info,
  const size_t length)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  if (string_info->length == length)
    return;
  if (~length < MagickPathExtent)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  string_info->length=length;
  if (string_info->datum == (unsigned char *) NULL)
    string_info->datum=(unsigned char *) AcquireQuantumMemory(length+
      MagickPathExtent,sizeof(*string_info->datum));
  else
    string_info->datum=(unsigned char *) ResizeQuantumMemory(string_info->datum,
      length+MagickPathExtent,sizeof(*string_info->datum));
  if (string_info->datum == (unsigned char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t S t r i n g I n f o N a m e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStringInfoName() sets the name associated with the string.
%
%  The format of the SetStringInfoName method is:
%
%      void SetStringInfoName(StringInfo *string_info,const char *name)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
%    o name: the name.
%
*/
MagickExport void SetStringInfoName(StringInfo *string_info,const char *name)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  assert(name != (const char *) NULL);
  string_info->name=ConstantString(name);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t S t r i n g I n f o P a t h                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStringInfoPath() sets the path associated with the string.
%
%  The format of the SetStringInfoPath method is:
%
%      void SetStringInfoPath(StringInfo *string_info,const char *path)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
%    o path: the path.
%
*/
MagickExport void SetStringInfoPath(StringInfo *string_info,const char *path)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  assert(path != (const char *) NULL);
  string_info->path=ConstantString(path);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S p l i t S t r i n g I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SplitStringInfo() splits a string into two and returns it.
%
%  The format of the SplitStringInfo method is:
%
%      StringInfo *SplitStringInfo(StringInfo *string_info,const size_t offset)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport StringInfo *SplitStringInfo(StringInfo *string_info,
  const size_t offset)
{
  StringInfo
    *split_info;

  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickCoreSignature);
  if (offset > string_info->length)
    return((StringInfo *) NULL);
  split_info=AcquireStringInfo(offset);
  SetStringInfo(split_info,string_info);
  (void) memmove(string_info->datum,string_info->datum+offset,
    string_info->length-offset+MagickPathExtent);
  SetStringInfoLength(string_info,string_info->length-offset);
  return(split_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i n g I n f o T o D i g e s t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringInfoToDigest() converts a string info string to a hex digest.
%
%  The format of the StringInfoToString method is:
%
%      char *StringInfoToDigest(const StringInfo *signature)
%
%  A description of each parameter follows:
%
%    o string_info: the string.
%
*/
MagickExport char *StringInfoToDigest(const StringInfo *signature)
{
  char
    *digest;

  SignatureInfo
    *signature_info;

  signature_info=AcquireSignatureInfo();
  UpdateSignature(signature_info,signature);
  FinalizeSignature(signature_info);
  digest=StringInfoToHexString(GetSignatureDigest(signature_info));
  signature_info=DestroySignatureInfo(signature_info);
  return(digest);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i n g I n f o T o H e x S t r i n g                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringInfoToHexString() converts a string info string to a C string.
%
%  The format of the StringInfoToHexString method is:
%
%      char *StringInfoToHexString(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string.
%
*/
MagickExport char *StringInfoToHexString(const StringInfo *string_info)
{
  char
    *string;

  const unsigned char
    *p;

  ssize_t
    i;

  unsigned char
    *q;

  size_t
    length;

  unsigned char
    hex_digits[16];

  length=string_info->length;
  if (~length < MagickPathExtent)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
  string=(char *) AcquireQuantumMemory(length+MagickPathExtent,2*
    sizeof(*string));
  if (string == (char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
  hex_digits[0]='0';
  hex_digits[1]='1';
  hex_digits[2]='2';
  hex_digits[3]='3';
  hex_digits[4]='4';
  hex_digits[5]='5';
  hex_digits[6]='6';
  hex_digits[7]='7';
  hex_digits[8]='8';
  hex_digits[9]='9';
  hex_digits[10]='a';
  hex_digits[11]='b';
  hex_digits[12]='c';
  hex_digits[13]='d';
  hex_digits[14]='e';
  hex_digits[15]='f';
  p=string_info->datum;
  q=(unsigned char *) string;
  for (i=0; i < (ssize_t) string_info->length; i++)
  {
    *q++=hex_digits[(*p >> 4) & 0x0f];
    *q++=hex_digits[*p & 0x0f];
    p++;
  }
  *q='\0';
  return(string);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i n g I n f o T o S t r i n g                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringInfoToString() converts a string info string to a C string.
%
%  The format of the StringInfoToString method is:
%
%      char *StringInfoToString(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string.
%
*/
MagickExport char *StringInfoToString(const StringInfo *string_info)
{
  char
    *string;

  size_t
    length;

  string=(char *) NULL;
  length=string_info->length;
  if (~length >= (MagickPathExtent-1))
    string=(char *) AcquireQuantumMemory(length+MagickPathExtent,
      sizeof(*string));
  if (string == (char *) NULL)
    return((char *) NULL);
  (void) memcpy(string,(char *) string_info->datum,length*sizeof(*string));
  string[length]='\0';
  return(string);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i n g T o S t r i n g I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToStringInfo() converts a string to a StringInfo type.
%
%  The format of the StringToStringInfo method is:
%
%      StringInfo *StringToStringInfo(const char *string)
%
%  A description of each parameter follows:
%
%    o string:  The string.
%
*/
MagickExport StringInfo *StringToStringInfo(const char *string)
{
  StringInfo
    *string_info;

  assert(string != (const char *) NULL);
  string_info=AcquireStringInfo(strlen(string));
  SetStringInfoDatum(string_info,(const unsigned char *) string);
  return(string_info);
}
