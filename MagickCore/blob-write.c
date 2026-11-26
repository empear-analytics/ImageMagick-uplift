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
%                      W   W  RRRR   IIIII  TTTTT  EEEEE                      %
%                      W   W  R   R    I      T    E                          %
%                      W W W  RRRR     I      T    EEE                        %
%                      WW WW  R R      I      T    E                          %
%                      W   W  R  R   IIIII    T    EEEEE                      %
%                                                                             %
%                                                                             %
%                        MagickCore Typed Blob Writers                        %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b B y t e                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobByte() write an integer to a blob.  It returns the number of bytes
%  written (either 0 or 1);
%
%  The format of the WriteBlobByte method is:
%
%      ssize_t WriteBlobByte(Image *image,const unsigned char value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobByte(Image *image,const unsigned char value)
{
  BlobInfo
    *magick_restrict blob_info;

  ssize_t
    count;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  blob_info=image->blob;
  count=0;
  switch (blob_info->type)
  {
    case StandardStream:
    case FileStream:
    case PipeStream:
    {
      int
        c;

      c=putc((int) value,blob_info->file_info.file);
      if (c == EOF)
        {
          if (ferror(blob_info->file_info.file) != 0)
            ThrowBlobException(blob_info);
          break;
        }
      count++;
      break;
    }
    default:
    {
      count=WriteBlobStream(image,1,&value);
      break;
    }
  }
  return(count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b F l o a t                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobFloat() writes a float value as a 32-bit quantity in the byte-order
%  specified by the endian member of the image structure.
%
%  The format of the WriteBlobFloat method is:
%
%      ssize_t WriteBlobFloat(Image *image,const float value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobFloat(Image *image,const float value)
{
  union
  {
    unsigned int
      unsigned_value;

    float
      float_value;
  } quantum;

  quantum.unsigned_value=0U;
  quantum.float_value=value;
  return(WriteBlobLong(image,quantum.unsigned_value));
}

static ssize_t WriteBlobLSBLongBytes(Image *image,const unsigned int value)
{
  unsigned char
    buffer[4];

  buffer[0]=(unsigned char) value;
  buffer[1]=(unsigned char) (value >> 8);
  buffer[2]=(unsigned char) (value >> 16);
  buffer[3]=(unsigned char) (value >> 24);
  return(WriteBlobStream(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L o n g                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLong() writes a unsigned int value as a 32-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the WriteBlobLong method is:
%
%      ssize_t WriteBlobLong(Image *image,const unsigned int value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLong(Image *image,const unsigned int value)
{
  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->endian == LSBEndian)
    {
      return(WriteBlobLSBLongBytes(image,value));
    }
  buffer[0]=(unsigned char) (value >> 24);
  buffer[1]=(unsigned char) (value >> 16);
  buffer[2]=(unsigned char) (value >> 8);
  buffer[3]=(unsigned char) value;
  return(WriteBlobStream(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L o n g L o n g                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobMSBLongLong() writes a long long value as a 64-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the WriteBlobLongLong method is:
%
%      ssize_t WriteBlobLongLong(Image *image,const MagickSizeType value)
%
%  A description of each parameter follows.
%
%    o value:  Specifies the value to write.
%
%    o image: the image.
%
*/
MagickExport ssize_t WriteBlobLongLong(Image *image,const MagickSizeType value)
{
  unsigned char
    buffer[8];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->endian == LSBEndian)
    {
      buffer[0]=(unsigned char) value;
      buffer[1]=(unsigned char) (value >> 8);
      buffer[2]=(unsigned char) (value >> 16);
      buffer[3]=(unsigned char) (value >> 24);
      buffer[4]=(unsigned char) (value >> 32);
      buffer[5]=(unsigned char) (value >> 40);
      buffer[6]=(unsigned char) (value >> 48);
      buffer[7]=(unsigned char) (value >> 56);
      return(WriteBlobStream(image,8,buffer));
    }
  buffer[0]=(unsigned char) (value >> 56);
  buffer[1]=(unsigned char) (value >> 48);
  buffer[2]=(unsigned char) (value >> 40);
  buffer[3]=(unsigned char) (value >> 32);
  buffer[4]=(unsigned char) (value >> 24);
  buffer[5]=(unsigned char) (value >> 16);
  buffer[6]=(unsigned char) (value >> 8);
  buffer[7]=(unsigned char) value;
  return(WriteBlobStream(image,8,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b S h o r t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobShort() writes a short value as a 16-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the WriteBlobShort method is:
%
%      ssize_t WriteBlobShort(Image *image,const unsigned short value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value:  Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobShort(Image *image,const unsigned short value)
{
  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->endian == LSBEndian)
    {
      buffer[0]=(unsigned char) value;
      buffer[1]=(unsigned char) (value >> 8);
      return(WriteBlobStream(image,2,buffer));
    }
  buffer[0]=(unsigned char) (value >> 8);
  buffer[1]=(unsigned char) value;
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b S i g n e d L o n g                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobSignedLong() writes a signed value as a 32-bit quantity in the
%  byte-order specified by the endian member of the image structure.
%
%  The format of the WriteBlobSignedLong method is:
%
%      ssize_t WriteBlobSignedLong(Image *image,const signed int value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobSignedLong(Image *image,const signed int value)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  quantum.signed_value=value;
  if (image->endian == LSBEndian)
    {
      return(WriteBlobLSBLongBytes(image,quantum.unsigned_value));
    }
  buffer[0]=(unsigned char) (quantum.unsigned_value >> 24);
  buffer[1]=(unsigned char) (quantum.unsigned_value >> 16);
  buffer[2]=(unsigned char) (quantum.unsigned_value >> 8);
  buffer[3]=(unsigned char) quantum.unsigned_value;
  return(WriteBlobStream(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L S B L o n g                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLSBLong() writes a unsigned int value as a 32-bit quantity in
%  least-significant byte first order.
%
%  The format of the WriteBlobLSBLong method is:
%
%      ssize_t WriteBlobLSBLong(Image *image,const unsigned int value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLSBLong(Image *image,const unsigned int value)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  return(WriteBlobLSBLongBytes(image,value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b L S B S h o r t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLSBShort() writes a unsigned short value as a 16-bit quantity in
%  least-significant byte first order.
%
%  The format of the WriteBlobLSBShort method is:
%
%      ssize_t WriteBlobLSBShort(Image *image,const unsigned short value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value:  Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLSBShort(Image *image,const unsigned short value)
{
  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  buffer[0]=(unsigned char) value;
  buffer[1]=(unsigned char) (value >> 8);
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L S B S i g n e d L o n g                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLSBSignedLong() writes a signed value as a 32-bit quantity in
%  least-significant byte first order.
%
%  The format of the WriteBlobLSBSignedLong method is:
%
%      ssize_t WriteBlobLSBSignedLong(Image *image,const signed int value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value: Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLSBSignedLong(Image *image,const signed int value)
{
  union
  {
    unsigned int
      unsigned_value;

    signed int
      signed_value;
  } quantum;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  quantum.signed_value=value;
  return(WriteBlobLSBLongBytes(image,quantum.unsigned_value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b L S B S i g n e d S h o r t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobLSBSignedShort() writes a signed short value as a 16-bit quantity
%  in least-significant byte first order.
%
%  The format of the WriteBlobLSBSignedShort method is:
%
%      ssize_t WriteBlobLSBSignedShort(Image *image,const signed short value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value:  Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobLSBSignedShort(Image *image,
  const signed short value)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  quantum.signed_value=value;
  buffer[0]=(unsigned char) quantum.unsigned_value;
  buffer[1]=(unsigned char) (quantum.unsigned_value >> 8);
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b M S B L o n g                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobMSBLong() writes a unsigned int value as a 32-bit quantity in
%  most-significant byte first order.
%
%  The format of the WriteBlobMSBLong method is:
%
%      ssize_t WriteBlobMSBLong(Image *image,const unsigned int value)
%
%  A description of each parameter follows.
%
%    o value:  Specifies the value to write.
%
%    o image: the image.
%
*/
MagickExport ssize_t WriteBlobMSBLong(Image *image,const unsigned int value)
{
  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  buffer[0]=(unsigned char) (value >> 24);
  buffer[1]=(unsigned char) (value >> 16);
  buffer[2]=(unsigned char) (value >> 8);
  buffer[3]=(unsigned char) value;
  return(WriteBlobStream(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b M S B S i g n e d S h o r t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobMSBSignedShort() writes a signed short value as a 16-bit quantity
%  in most-significant byte first order.
%
%  The format of the WriteBlobMSBSignedShort method is:
%
%      ssize_t WriteBlobMSBSignedShort(Image *image,const signed short value)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o value:  Specifies the value to write.
%
*/
MagickExport ssize_t WriteBlobMSBSignedShort(Image *image,
  const signed short value)
{
  union
  {
    unsigned short
      unsigned_value;

    signed short
      signed_value;
  } quantum;

  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  quantum.signed_value=value;
  buffer[0]=(unsigned char) (quantum.unsigned_value >> 8);
  buffer[1]=(unsigned char) quantum.unsigned_value;
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b M S B S h o r t                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobMSBShort() writes a unsigned short value as a 16-bit quantity in
%  most-significant byte first order.
%
%  The format of the WriteBlobMSBShort method is:
%
%      ssize_t WriteBlobMSBShort(Image *image,const unsigned short value)
%
%  A description of each parameter follows.
%
%   o  value:  Specifies the value to write.
%
%   o  file:  Specifies the file to write the data to.
%
*/
MagickExport ssize_t WriteBlobMSBShort(Image *image,const unsigned short value)
{
  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  buffer[0]=(unsigned char) (value >> 8);
  buffer[1]=(unsigned char) value;
  return(WriteBlobStream(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b S t r i n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlobString() write a string to a blob.  It returns the number of
%  characters written.
%
%  The format of the WriteBlobString method is:
%
%      ssize_t WriteBlobString(Image *image,const char *string)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o string: Specifies the string to write.
%
*/
MagickExport ssize_t WriteBlobString(Image *image,const char *string)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(string != (const char *) NULL);
  return(WriteBlobStream(image,strlen(string),(const unsigned char *) string));
}
