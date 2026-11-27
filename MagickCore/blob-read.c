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
%                         RRRR   EEEEE   AAA   DDDD                           %
%                         R   R  E      A   A  D   D                          %
%                         RRRR   EEE    AAAAA  D   D                          %
%                         R R    E      A   A  D   D                          %
%                         R  R   EEEEE  A   A  DDDD                           %
%                                                                             %
%                                                                             %
%                        MagickCore Typed Blob Readers                        %
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
#include "MagickCore/blob-io-private.h"

static inline int ReadBlobFileByte(BlobInfo *magick_restrict blob_info)
{
  int
    c;

  c=getc(blob_info->file_info.file);
  if (c == EOF)
    {
      if (ferror(blob_info->file_info.file) != 0)
        ThrowBlobException(blob_info);
      return(EOF);
    }
  return(c);
}

static inline int ReadBlobStreamByte(BlobInfo *magick_restrict blob_info)
{
  int
    c;

  if (blob_info->offset >= (MagickOffsetType) blob_info->length)
    {
      blob_info->eof=MagickTrue;
      return(EOF);
    }
  c=(int) (*((unsigned char *) blob_info->data+blob_info->offset));
  blob_info->offset++;
  return(c);
}

static inline int ReadBlobDefaultByte(Image *image)
{
  int
    c;

  ssize_t
    count;

  unsigned char
    buffer[1];

  count=ReadBlob(image,1,buffer);
  if (count != 1)
    return(EOF);
  c=(int) *buffer;
  return(c);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b B y t e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobByte() reads a single byte from the image file and returns it.
%
%  The format of the ReadBlobByte method is:
%
%      int ReadBlobByte(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport int ReadBlobByte(Image *image)
{
  BlobInfo
    *magick_restrict blob_info;

  int
    c;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  blob_info=image->blob;
  switch (blob_info->type)
  {
    case StandardStream:
    case FileStream:
    case PipeStream:
    {
      c=ReadBlobFileByte(blob_info);
      break;
    }
    case BlobStream:
    {
      c=ReadBlobStreamByte(blob_info);
      break;
    }
    default:
    {
      c=ReadBlobDefaultByte(image);
      break;
    }
  }
  return(c);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b D o u b l e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobDouble() reads a double value as a 64-bit quantity in the byte-order
%  specified by the endian member of the image structure.
%
%  The format of the ReadBlobDouble method is:
%
%      double ReadBlobDouble(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport double ReadBlobDouble(Image *image)
{
  union
  {
    MagickSizeType
      unsigned_value;

    double
      double_value;
  } quantum;

  quantum.double_value=0.0;
  quantum.unsigned_value=ReadBlobLongLong(image);
  return(quantum.double_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b F l o a t                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobFloat() reads a float value as a 32-bit quantity in the byte-order
%  specified by the endian member of the image structure.
%
%  The format of the ReadBlobFloat method is:
%
%      float ReadBlobFloat(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport float ReadBlobFloat(Image *image)
{
  union
  {
    unsigned int
      unsigned_value;

    float
      float_value;
  } quantum;

  quantum.float_value=0.0;
  quantum.unsigned_value=ReadBlobLong(image);
  return(quantum.float_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L o n g                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLong() reads a unsigned int value as a 32-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the ReadBlobLong method is:
%
%      unsigned int ReadBlobLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned int ReadBlobLong(Image *image)
{
  const unsigned char
    *p;

  ssize_t
    count;

  unsigned char
    buffer[4];

  unsigned int
    value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,4,buffer,&count);
  if (count != 4)
    return(0UL);
  if (image->endian == LSBEndian)
    {
      value=(unsigned int) (*p++);
      value|=(unsigned int) (*p++) << 8;
      value|=(unsigned int) (*p++) << 16;
      value|=(unsigned int) (*p++) << 24;
      return(value);
    }
  value=(unsigned int) (*p++) << 24;
  value|=(unsigned int) (*p++) << 16;
  value|=(unsigned int) (*p++) << 8;
  value|=(unsigned int) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L o n g L o n g                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLongLong() reads a long long value as a 64-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the ReadBlobLongLong method is:
%
%      MagickSizeType ReadBlobLongLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport MagickSizeType ReadBlobLongLong(Image *image)
{
  MagickSizeType
    value;

  const unsigned char
    *p;

  ssize_t
    count;

  unsigned char
    buffer[8];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,8,buffer,&count);
  if (count != 8)
    return(MagickULLConstant(0));
  if (image->endian == LSBEndian)
    {
      value=(MagickSizeType) (*p++);
      value|=(MagickSizeType) (*p++) << 8;
      value|=(MagickSizeType) (*p++) << 16;
      value|=(MagickSizeType) (*p++) << 24;
      value|=(MagickSizeType) (*p++) << 32;
      value|=(MagickSizeType) (*p++) << 40;
      value|=(MagickSizeType) (*p++) << 48;
      value|=(MagickSizeType) (*p++) << 56;
      return(value);
    }
  value=(MagickSizeType) (*p++) << 56;
  value|=(MagickSizeType) (*p++) << 48;
  value|=(MagickSizeType) (*p++) << 40;
  value|=(MagickSizeType) (*p++) << 32;
  value|=(MagickSizeType) (*p++) << 24;
  value|=(MagickSizeType) (*p++) << 16;
  value|=(MagickSizeType) (*p++) << 8;
  value|=(MagickSizeType) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S h o r t                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobShort() reads a short value as a 16-bit quantity in the byte-order
%  specified by the endian member of the image structure.
%
%  The format of the ReadBlobShort method is:
%
%      unsigned short ReadBlobShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned short ReadBlobShort(Image *image)
{
  const unsigned char
    *p;

  unsigned short
    value;

  ssize_t
    count;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,2,buffer,&count);
  if (count != 2)
    return((unsigned short) 0U);
  if (image->endian == LSBEndian)
    {
      value=(unsigned short) (*p++);
      value|=(unsigned short) (*p++) << 8;
      return(value);
    }
  value=(unsigned short) ((unsigned short) (*p++) << 8);
  value|=(unsigned short) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B L o n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLSBLong() reads a unsigned int value as a 32-bit quantity in
%  least-significant byte first order.
%
%  The format of the ReadBlobLSBLong method is:
%
%      unsigned int ReadBlobLSBLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned int ReadBlobLSBLong(Image *image)
{
  const unsigned char
    *p;

  unsigned int
    value;

  ssize_t
    count;

  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,4,buffer,&count);
  if (count != 4)
    return(0U);
  value=(unsigned int) (*p++);
  value|=(unsigned int) (*p++) << 8;
  value|=(unsigned int) (*p++) << 16;
  value|=(unsigned int) (*p++) << 24;
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B S i g n e d L o n g                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLSBSignedLong() reads a signed int value as a 32-bit quantity in
%  least-significant byte first order.
%
%  The format of the ReadBlobLSBSignedLong method is:
%
%      signed int ReadBlobLSBSignedLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed int ReadBlobLSBSignedLong(Image *image)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobLSBLong(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B S h o r t                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLSBShort() reads a short value as a 16-bit quantity in
%  least-significant byte first order.
%
%  The format of the ReadBlobLSBShort method is:
%
%      unsigned short ReadBlobLSBShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned short ReadBlobLSBShort(Image *image)
{
  const unsigned char
    *p;

  unsigned short
    value;

  ssize_t
    count;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,2,buffer,&count);
  if (count != 2)
    return((unsigned short) 0U);
  value=(unsigned short) (*p++);
  value|=(unsigned short) (*p++) << 8;
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B S i g n e d S h o r t                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobLSBSignedShort() reads a signed short value as a 16-bit quantity in
%  least-significant byte-order.
%
%  The format of the ReadBlobLSBSignedShort method is:
%
%      signed short ReadBlobLSBSignedShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed short ReadBlobLSBSignedShort(Image *image)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobLSBShort(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B L o n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBLong() reads a unsigned int value as a 32-bit quantity in
%  most-significant byte first order.
%
%  The format of the ReadBlobMSBLong method is:
%
%      unsigned int ReadBlobMSBLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned int ReadBlobMSBLong(Image *image)
{
  const unsigned char
    *p;

  unsigned int
    value;

  ssize_t
    count;

  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,4,buffer,&count);
  if (count != 4)
    return(0UL);
  value=(unsigned int) (*p++) << 24;
  value|=(unsigned int) (*p++) << 16;
  value|=(unsigned int) (*p++) << 8;
  value|=(unsigned int) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B L o n g L o n g                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBLongLong() reads a unsigned long long value as a 64-bit quantity
%  in most-significant byte first order.
%
%  The format of the ReadBlobMSBLongLong method is:
%
%      unsigned int ReadBlobMSBLongLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport MagickSizeType ReadBlobMSBLongLong(Image *image)
{
  const unsigned char
    *p;

  MagickSizeType
    value;

  ssize_t
    count;

  unsigned char
    buffer[8];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,8,buffer,&count);
  if (count != 8)
    return(MagickULLConstant(0));
  value=(MagickSizeType) (*p++) << 56;
  value|=(MagickSizeType) (*p++) << 48;
  value|=(MagickSizeType) (*p++) << 40;
  value|=(MagickSizeType) (*p++) << 32;
  value|=(MagickSizeType) (*p++) << 24;
  value|=(MagickSizeType) (*p++) << 16;
  value|=(MagickSizeType) (*p++) << 8;
  value|=(MagickSizeType) (*p++);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B S h o r t                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBShort() reads a short value as a 16-bit quantity in
%  most-significant byte first order.
%
%  The format of the ReadBlobMSBShort method is:
%
%      unsigned short ReadBlobMSBShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport unsigned short ReadBlobMSBShort(Image *image)
{
  const unsigned char
    *p;

  unsigned short
    value;

  ssize_t
    count;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  *buffer='\0';
  p=(const unsigned char *) ReadBlobStream(image,2,buffer,&count);
  if (count != 2)
    return((unsigned short) 0U);
  value=(unsigned short) ((*p++) << 8);
  value|=(unsigned short) (*p++);
  return((unsigned short) (value & 0xffff));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B S i g n e d L o n g                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBSignedLong() reads a signed int value as a 32-bit quantity in
%  most-significant byte-order.
%
%  The format of the ReadBlobMSBSignedLong method is:
%
%      signed int ReadBlobMSBSignedLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed int ReadBlobMSBSignedLong(Image *image)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobMSBLong(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B S i g n e d S h o r t                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBSignedShort() reads a signed short value as a 16-bit quantity in
%  most-significant byte-order.
%
%  The format of the ReadBlobMSBSignedShort method is:
%
%      signed short ReadBlobMSBSignedShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed short ReadBlobMSBSignedShort(Image *image)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobMSBShort(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S i g n e d L o n g                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobSignedLong() reads a signed int value as a 32-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the ReadBlobSignedLong method is:
%
%      signed int ReadBlobSignedLong(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed int ReadBlobSignedLong(Image *image)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobLong(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S i g n e d S h o r t                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobSignedShort() reads a signed short value as a 16-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the ReadBlobSignedShort method is:
%
%      signed short ReadBlobSignedShort(Image *image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
*/
MagickExport signed short ReadBlobSignedShort(Image *image)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  quantum.unsigned_value=ReadBlobShort(image);
  return(quantum.signed_value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S t r e a m                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobStream() reads data from the blob or image file and returns it.  It
%  returns a pointer to the data buffer you supply or to the image memory
%  buffer if its supported (zero-copy). If length is zero, ReadBlobStream()
%  returns a count of zero and has no other results. If length is greater than
%  MAGICK_SSIZE_MAX, the result is unspecified.
%
%  The format of the ReadBlobStream method is:
%
%      const void *ReadBlobStream(Image *image,const size_t length,
%        void *magick_restrict data,ssize_t *count)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o length:  Specifies an integer representing the number of bytes to read
%      from the file.
%
%    o count: returns the number of bytes read.
%
%    o data:  Specifies an area to place the information requested from the
%      file.
%
*/
MagickExport magick_hot_spot const void *ReadBlobStream(Image *image,
  const size_t length,void *magick_restrict data,ssize_t *count)
{
  BlobInfo
    *magick_restrict blob_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  assert(count != (ssize_t *) NULL);
  blob_info=image->blob;
  if (blob_info->type != BlobStream)
    {
      assert(data != NULL);
      *count=ReadBlob(image,length,(unsigned char *) data);
      return(data);
    }
  if (blob_info->offset >= (MagickOffsetType) blob_info->length)
    {
      *count=0;
      blob_info->eof=MagickTrue;
      return(data);
    }
  data=blob_info->data+blob_info->offset;
  *count=(ssize_t) MagickMin((MagickOffsetType) length,(MagickOffsetType)
    blob_info->length-blob_info->offset);
  blob_info->offset+=(*count);
  if (*count != (ssize_t) length)
    blob_info->eof=MagickTrue;
  return(data);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d B l o b S t r i n g                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobString() reads characters from a blob or file until a newline
%  character is read or an end-of-file condition is encountered.
%
%  The format of the ReadBlobString method is:
%
%      char *ReadBlobString(Image *image,char *string)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o string: the address of a character buffer.
%
*/
static size_t ReadBlobLine(Image *image,BlobInfo *magick_restrict blob_info,
  char *string,size_t i)
{
  int
    c;

  do
  {
    c=ReadBlobByte(image);
    if (c == EOF)
      {
        blob_info->eof=MagickTrue;
        break;
      }
    string[i++]=(char) c;
    if (c == '\n')
      break;
  } while (i < (MaxTextExtent-2));
  string[i]='\0';
  return(i);
}

static MagickBooleanType ReadBlobFileString(
  BlobInfo *magick_restrict blob_info,char *string,size_t *i)
{
  char *p = fgets(string,MagickPathExtent,blob_info->file_info.file);
  if (p == (char *) NULL)
    {
      if (ferror(blob_info->file_info.file) != 0)
        ThrowBlobException(blob_info);
      return(MagickFalse);
    }
  (*i)=strlen(string);
  return(MagickTrue);
}

#if defined(MAGICKCORE_ZLIB_DELEGATE)
static MagickBooleanType ReadBlobGzipString(
  BlobInfo *magick_restrict blob_info,char *string,size_t *i)
{
  char *p = gzgets(blob_info->file_info.gzfile,string,MagickPathExtent);
  if (p == (char *) NULL)
    {
      int status = Z_OK;
      (void) gzerror(blob_info->file_info.gzfile,&status);
      if (status != Z_OK)
        ThrowBlobException(blob_info);
      return(MagickFalse);
    }
  (*i)=strlen(string);
  return(MagickTrue);
}
#endif

static void StripBlobStringNewline(char *string,const size_t i)
{
  /*
    Strip trailing newline.
  */
  if ((string[i] == '\r') || (string[i] == '\n'))
    string[i]='\0';
  if (i >= 1)
    if ((string[i-1] == '\r') || (string[i-1] == '\n'))
      string[i-1]='\0';
}

MagickExport char *ReadBlobString(Image *image,char *string)
{
  BlobInfo
    *magick_restrict blob_info;

  size_t
    i = 0;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  *string='\0';
  blob_info=image->blob;
  switch (blob_info->type)
  {
    case UndefinedStream:
      break;
    case StandardStream:
    case FileStream:
    {
      if (ReadBlobFileString(blob_info,string,&i) == MagickFalse)
        return((char *) NULL);
      break;
    }
    case ZipStream:
    {
#if defined(MAGICKCORE_ZLIB_DELEGATE)
      if (ReadBlobGzipString(blob_info,string,&i) == MagickFalse)
        return((char *) NULL);
      break;
#endif
    }
    default:
    {
      i=ReadBlobLine(image,blob_info,string,i);
      break;
    }
  }
  StripBlobStringNewline(string,i);
  if ((*string == '\0') && (blob_info->eof != MagickFalse))
    return((char *) NULL);
  return(string);
}
