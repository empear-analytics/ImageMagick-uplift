/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS  TTTTT  RRRR   EEEEE   AAA   M   M                   %
%                  SS       T    R   R  E      A   A  MM MM                   %
%                   SSS     T    RRRR   EEE    AAAAA  M M M                   %
%                     SS    T    R R    E      A   A  M   M                   %
%                  SSSSS    T    R  R   EEEEE  A   A  M   M                   %
%                                                                             %
%                                                                             %
%                       MagickCore Pixel Stream Methods                       %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 March 2000                                  %
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
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/policy.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/stream.h"
#include "MagickCore/stream-private.h"
#include "MagickCore/string_.h"

/*
  Typedef declarations.
*/
struct _StreamInfo
{
  const ImageInfo
    *image_info;

  const Image
    *image;

  Image
    *stream;

  QuantumInfo
    *quantum_info;

  char
    *map;

  StorageType
    storage_type;

  unsigned char
    *pixels;

  RectangleInfo
    extract_info;

  ssize_t
    y;

  ExceptionInfo
    *exception;

  const void
    *client_data;

  size_t
    signature;
};

/*
  Declare pixel cache interfaces.
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static MagickBooleanType
  StreamImagePixels(const StreamInfo *,const Image *,ExceptionInfo *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e S t r e a m I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireStreamInfo() allocates the StreamInfo structure.
%
%  The format of the AcquireStreamInfo method is:
%
%      StreamInfo *AcquireStreamInfo(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport StreamInfo *AcquireStreamInfo(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  StreamInfo
    *stream_info;

  stream_info=(StreamInfo *) AcquireCriticalMemory(sizeof(*stream_info));
  (void) memset(stream_info,0,sizeof(*stream_info));
  stream_info->pixels=(unsigned char *) MagickAssumeAligned(
    AcquireAlignedMemory(1,sizeof(*stream_info->pixels)));
  if (stream_info->pixels == (unsigned char *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  stream_info->map=ConstantString("RGB");
  stream_info->storage_type=CharPixel;
  stream_info->stream=AcquireImage(image_info,exception);
  stream_info->signature=MagickCoreSignature;
  return(stream_info);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y S t r e a m I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyStreamInfo() destroys memory associated with the StreamInfo
%  structure.
%
%  The format of the DestroyStreamInfo method is:
%
%      StreamInfo *DestroyStreamInfo(StreamInfo *stream_info)
%
%  A description of each parameter follows:
%
%    o stream_info: the stream info.
%
*/
MagickExport StreamInfo *DestroyStreamInfo(StreamInfo *stream_info)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (stream_info->map != (char *) NULL)
    stream_info->map=DestroyString(stream_info->map);
  if (stream_info->pixels != (unsigned char *) NULL)
    stream_info->pixels=(unsigned char *) RelinquishAlignedMemory(
      stream_info->pixels);
  if (stream_info->stream != (Image *) NULL)
    {
      (void) CloseBlob(stream_info->stream);
      stream_info->stream=DestroyImage(stream_info->stream);
    }
  if (stream_info->quantum_info != (QuantumInfo *) NULL)
    stream_info->quantum_info=DestroyQuantumInfo(stream_info->quantum_info);
  stream_info->signature=(~MagickCoreSignature);
  stream_info=(StreamInfo *) RelinquishMagickMemory(stream_info);
  return(stream_info);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t S t r e a m I n f o C l i e n t D a t a                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetStreamInfoClientData() gets the stream info client data.
%
%  The format of the GetStreamInfoClientData method is:
%
%      const void *GetStreamInfoClientData(StreamInfo *stream_info)
%
%  A description of each parameter follows:
%
%    o stream_info: the stream info.
%
*/
MagickPrivate const void *GetStreamInfoClientData(StreamInfo *stream_info)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickCoreSignature);
  return(stream_info->client_data);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p e n S t r e a m                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenStream() opens a stream for writing by the StreamImage() method.
%
%  The format of the OpenStream method is:
%
%       MagickBooleanType OpenStream(const ImageInfo *image_info,
%        StreamInfo *stream_info,const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o stream_info: the stream info.
%
%    o filename: the stream filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType OpenStream(const ImageInfo *image_info,
  StreamInfo *stream_info,const char *filename,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  (void) CopyMagickString(stream_info->stream->filename,filename,
    MagickPathExtent);
  status=OpenBlob(image_info,stream_info->stream,WriteBinaryBlobMode,exception);
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e s e t S t r e a m A n o n y m o u s M e m o r y                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetStreamAnonymousMemory() resets the anonymous_memory value.
%
%  The format of the ResetStreamAnonymousMemory method is:
%
%      void ResetStreamAnonymousMemory(void)
%
*/
MagickPrivate void ResetStreamAnonymousMemory(void)
{
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t S t r e a m I n f o C l i e n t D a t a                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStreamInfoClientData() sets the stream info client data.
%
%  The format of the SetStreamInfoClientData method is:
%
%      void SetStreamInfoClientData(StreamInfo *stream_info,
%        const void *client_data)
%
%  A description of each parameter follows:
%
%    o stream_info: the stream info.
%
%    o client_data: the client data.
%
*/
MagickPrivate void SetStreamInfoClientData(StreamInfo *stream_info,
  const void *client_data)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickCoreSignature);
  stream_info->client_data=client_data;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t S t r e a m I n f o M a p                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStreamInfoMap() sets the stream info map member.
%
%  The format of the SetStreamInfoMap method is:
%
%      void SetStreamInfoMap(StreamInfo *stream_info,const char *map)
%
%  A description of each parameter follows:
%
%    o stream_info: the stream info.
%
%    o map: the map.
%
*/
MagickExport void SetStreamInfoMap(StreamInfo *stream_info,const char *map)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickCoreSignature);
  (void) CloneString(&stream_info->map,map);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t S t r e a m I n f o S t o r a g e T y p e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStreamInfoStorageType() sets the stream info storage type member.
%
%  The format of the SetStreamInfoStorageType method is:
%
%      void SetStreamInfoStorageType(StreamInfo *stream_info,
%        const StorageType *storage_type)
%
%  A description of each parameter follows:
%
%    o stream_info: the stream info.
%
%    o storage_type: the storage type.
%
*/
MagickExport void SetStreamInfoStorageType(StreamInfo *stream_info,
  const StorageType storage_type)
{
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickCoreSignature);
  stream_info->storage_type=storage_type;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S t r e a m I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StreamImage() streams pixels from an image and writes them in a user
%  defined format and storage type (e.g. RGBA as 8-bit unsigned char).
%
%  The format of the StreamImage() method is:
%
%      Image *StreamImage(const ImageInfo *image_info,
%        StreamInfo *stream_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o stream_info: the stream info.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static MagickBooleanType PrepareStreamForWriting(StreamInfo *stream_info,
  const Image *image,const size_t length)
{
  if (image != stream_info->image)
    {
      ImageInfo
        *write_info;

      /*
        Prepare stream for writing.
      */
      (void) RelinquishAlignedMemory(stream_info->pixels);
      stream_info->pixels=(unsigned char *) AcquireAlignedMemory(1,length);
      if (stream_info->pixels == (unsigned char *) NULL)
        return(MagickFalse);
      (void) memset(stream_info->pixels,0,length);
      stream_info->image=image;
      write_info=CloneImageInfo(stream_info->image_info);
      (void) SetImageInfo(write_info,1,stream_info->exception);
      if (write_info->extract != (char *) NULL)
        (void) ParseAbsoluteGeometry(write_info->extract,
          &stream_info->extract_info);
      stream_info->y=0;
      write_info=DestroyImageInfo(write_info);
    }
  return(MagickTrue);
}

static size_t GetStreamPacketSize(const StreamInfo *stream_info)
{
  size_t
    packet_size;

  switch (stream_info->storage_type)
  {
    default: packet_size=sizeof(unsigned char); break;
    case DoublePixel: packet_size=sizeof(double); break;
    case FloatPixel: packet_size=sizeof(float); break;
    case LongPixel: packet_size=sizeof(unsigned int); break;
    case LongLongPixel: packet_size=sizeof(MagickSizeType); break;
    case QuantumPixel: packet_size=sizeof(Quantum); break;
    case ShortPixel: packet_size=sizeof(unsigned short); break;
  }
  return(packet_size);
}

static size_t WriteStreamImage(const Image *image,const void *pixels,
  const size_t columns)
{
  CacheInfo
    *cache_info;

  RectangleInfo
    extract_info;

  size_t
    length,
    packet_size;

  ssize_t
    count;

  StreamInfo
    *stream_info;

  (void) pixels;
  stream_info=(StreamInfo *) image->client_data;
  packet_size=GetStreamPacketSize(stream_info);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  packet_size*=strlen(stream_info->map);
  length=packet_size*cache_info->columns*cache_info->rows;
  if (PrepareStreamForWriting(stream_info,image,length) == MagickFalse)
    return(0);
  extract_info=stream_info->extract_info;
  if ((extract_info.width == 0) || (extract_info.height == 0))
    {
      /*
        Write all pixels to stream.
      */
      (void) StreamImagePixels(stream_info,image,stream_info->exception);
      count=WriteBlob(stream_info->stream,length,stream_info->pixels);
      stream_info->y++;
      return(count == 0 ? 0 : columns);
    }
  if ((stream_info->y < extract_info.y) ||
      (stream_info->y >= (extract_info.y+(ssize_t) extract_info.height)))
    {
      stream_info->y++;
      return(columns);
    }
  /*
    Write a portion of the pixel row to the stream.
  */
  (void) StreamImagePixels(stream_info,image,stream_info->exception);
  length=packet_size*extract_info.width;
  count=WriteBlob(stream_info->stream,length,stream_info->pixels+(ssize_t)
    packet_size*extract_info.x);
  stream_info->y++;
  return(count == 0 ? 0 : columns);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport Image *StreamImage(const ImageInfo *image_info,
  StreamInfo *stream_info,ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *read_info;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  read_info=CloneImageInfo(image_info);
  stream_info->image_info=image_info;
  if (stream_info->quantum_info == (QuantumInfo *) NULL)
    stream_info->quantum_info=AcquireQuantumInfo(image_info,(Image *) NULL);
  if (stream_info->quantum_info == (QuantumInfo *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      return((Image *) NULL);
    }
  stream_info->exception=exception;
  read_info->client_data=(void *) stream_info;
  image=ReadStream(read_info,&WriteStreamImage,exception);
  read_info=DestroyImageInfo(read_info);
  stream_info->quantum_info=DestroyQuantumInfo(stream_info->quantum_info);
  stream_info->quantum_info=AcquireQuantumInfo(image_info,image);
  if (stream_info->quantum_info == (QuantumInfo *) NULL)
    image=DestroyImage(image);
  return(image);
}

typedef struct _StreamRowInfo
{
  const Image
    *image;

  const QuantumInfo
    *quantum_info;

  const QuantumType
    *quantum_map;

  size_t
    length;
} StreamRowInfo;

typedef void
  (*StreamRowMethod)(const StreamRowInfo *,const Quantum *,void *);

static void StreamPixelQueue(const StreamRowInfo *row,
  const StreamRowMethod method,void *pixels)
{
  const Quantum
    *p;

  p=GetAuthenticPixelQueue(row->image);
  if (p == (const Quantum *) NULL)
    return;
  method(row,p,pixels);
}

static void StreamCharBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar((Quantum) 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar((Quantum) 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(ClampToQuantum(GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=0U;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=0U;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    *q++=GetPixelAlpha(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    *q++=(Quantum) 0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelAlpha(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    *q++=(Quantum) 0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(ClampToQuantum(
      GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(ClampToQuantum(
      GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ClampToQuantum(GetPixelIntensity(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelAlpha(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelIntensity(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelAlpha(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelAlpha(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelIntensity(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelAlpha(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned char *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelOpacity(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToChar(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void StreamCharQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned char *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      StreamCharAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

typedef struct _StreamDoubleChannel
{
  const Image
    *image;

  const QuantumInfo
    *quantum_info;

  QuantumType
    quantum;

  const Quantum
    *p;

  double
    *q;
} StreamDoubleChannel;

static void StreamDoubleAlphaIndexQuantum(const StreamDoubleChannel *channel)
{
  const Image
    *image = channel->image;

  const QuantumInfo
    *quantum_info = channel->quantum_info;

  const QuantumType
    quantum = channel->quantum;

  const Quantum
    *p = channel->p;

  double
    *q = channel->q;

  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelAlpha(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case OpacityQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelOpacity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=(double) ((QuantumScale*(double) GetPixelBlack(image,p))*
          quantum_info->scale+quantum_info->minimum);
      break;
    }
    case IndexQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelIntensity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
      *q=0;
  }
}

static void StreamDoubleQuantum(const Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum,const Quantum *p,
  double *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
    {
      StreamDoubleChannel
        channel;

      channel.image=image;
      channel.quantum_info=quantum_info;
      channel.quantum=quantum;
      channel.p=p;
      channel.q=q;
      StreamDoubleAlphaIndexQuantum(&channel);
      break;
    }
  }
}

typedef struct _StreamFloatChannel
{
  const Image
    *image;

  const QuantumInfo
    *quantum_info;

  QuantumType
    quantum;

  const Quantum
    *p;

  float
    *q;
} StreamFloatChannel;

static void StreamFloatAlphaIndexQuantum(const StreamFloatChannel *channel)
{
  const Image
    *image = channel->image;

  const QuantumInfo
    *quantum_info = channel->quantum_info;

  const QuantumType
    quantum = channel->quantum;

  const Quantum
    *p = channel->p;

  float
    *q = channel->q;

  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelAlpha(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case OpacityQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelOpacity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=(float) ((QuantumScale*(double) GetPixelBlack(image,p))*
          quantum_info->scale+quantum_info->minimum);
      break;
    }
    case IndexQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelIntensity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
      *q=0;
  }
}

static void StreamFloatQuantum(const Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum,const Quantum *p,
  float *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
    {
      StreamFloatChannel
        channel;

      channel.image=image;
      channel.quantum_info=quantum_info;
      channel.quantum=quantum;
      channel.p=p;
      channel.q=q;
      StreamFloatAlphaIndexQuantum(&channel);
      break;
    }
  }
}

static void StreamLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned int *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelOpacity(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToLong(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToLong(ClampToQuantum(GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void StreamLongQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned int *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      StreamLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamLongLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,MagickSizeType *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelOpacity(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToLongLong(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToLongLong(ClampToQuantum(
        GetPixelIntensity(image,p)));
      break;
    }
    default:
      *q=0;
  }
}

static void StreamLongLongQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,MagickSizeType *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      StreamLongLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamQuantumAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,Quantum *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=GetPixelAlpha(image,p);
      break;
    }
    case OpacityQuantum:
    {
      *q=GetPixelOpacity(image,p);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=GetPixelBlack(image,p);
      break;
    }
    case IndexQuantum:
    {
      *q=ClampToQuantum(GetPixelIntensity(image,p));
      break;
    }
    default:
      *q=(Quantum) 0;
  }
}

static void StreamQuantumQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,Quantum *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=GetPixelRed(image,p);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=GetPixelGreen(image,p);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=GetPixelBlue(image,p);
      break;
    }
    default:
    {
      StreamQuantumAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamShortAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned short *q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelOpacity(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToShort(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToShort(ClampToQuantum(
        GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void StreamShortQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned short *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      StreamShortAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamCharMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamCharQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  double
    *q = (double *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamDoubleQuantum(image,quantum_info,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  float
    *q = (float *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamFloatQuantum(image,quantum_info,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamLongQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamLongLongQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=(Quantum) 0;
      StreamQuantumQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamShortQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S t r e a m I m a g e P i x e l s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StreamImagePixels() extracts pixel data from an image and returns it in the
%  stream_info->pixels structure in the format as defined by
%  stream_info->quantum_info->map and stream_info->quantum_info->storage_type.
%
%  The format of the StreamImagePixels method is:
%
%      MagickBooleanType StreamImagePixels(const StreamInfo *stream_info,
%        const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o stream_info: the stream info.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void StreamCharPixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamCharBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamCharBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamCharBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamCharIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamCharRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamCharRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamCharRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamCharMapPixels,stream_info->pixels);
}

static void StreamDoublePixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  QuantumInfo
    *quantum_info;

  StreamRowInfo
    row;

  quantum_info=stream_info->quantum_info;
  row.image=image;
  row.quantum_info=quantum_info;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamDoubleRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamDoubleMapPixels,stream_info->pixels);
}

static void StreamFloatPixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  QuantumInfo
    *quantum_info;

  StreamRowInfo
    row;

  quantum_info=stream_info->quantum_info;
  row.image=image;
  row.quantum_info=quantum_info;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamFloatBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamFloatBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamFloatBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamFloatIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamFloatRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamFloatRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamFloatRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamFloatMapPixels,stream_info->pixels);
}

static void StreamLongPixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamLongBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamLongBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamLongBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamLongIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamLongRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamLongRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamLongRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamLongMapPixels,stream_info->pixels);
}

static void StreamLongLongPixels(const StreamInfo *stream_info,
  const Image *image,QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamLongLongRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamLongLongMapPixels,stream_info->pixels);
}

static void StreamQuantumPixels(const StreamInfo *stream_info,
  const Image *image,QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamQuantumRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamQuantumMapPixels,stream_info->pixels);
}

static void StreamShortPixels(const StreamInfo *stream_info,const Image *image,
  QuantumType *quantum_map,size_t length)
{
  StreamRowInfo
    row;

  row.image=image;
  row.quantum_map=quantum_map;
  row.length=length;
  if (LocaleCompare(stream_info->map,"BGR") == 0)
    {
      StreamPixelQueue(&row,StreamShortBGRPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRA") == 0)
    {
      StreamPixelQueue(&row,StreamShortBGRAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"BGRP") == 0)
    {
      StreamPixelQueue(&row,StreamShortBGRPPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"I") == 0)
    {
      StreamPixelQueue(&row,StreamShortIPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGB") == 0)
    {
      StreamPixelQueue(&row,StreamShortRGBPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBA") == 0)
    {
      StreamPixelQueue(&row,StreamShortRGBAPixels,stream_info->pixels);
      return;
    }
  if (LocaleCompare(stream_info->map,"RGBP") == 0)
    {
      StreamPixelQueue(&row,StreamShortRGBPPixels,stream_info->pixels);
      return;
    }
  StreamPixelQueue(&row,StreamShortMapPixels,stream_info->pixels);
}

typedef struct _StreamMapInfo
{
  char
    key;

  QuantumType
    quantum;

  MagickBooleanType
    separation;
} StreamMapInfo;

static const StreamMapInfo *GetStreamMapInfo(const char key)
{
  static const StreamMapInfo
    map_info[] =
    {
      { 'A', AlphaQuantum, MagickFalse },
      { 'a', AlphaQuantum, MagickFalse },
      { 'B', BlueQuantum, MagickFalse },
      { 'b', BlueQuantum, MagickFalse },
      { 'C', CyanQuantum, MagickTrue },
      { 'c', CyanQuantum, MagickTrue },
      { 'G', GreenQuantum, MagickFalse },
      { 'g', GreenQuantum, MagickFalse },
      { 'I', IndexQuantum, MagickFalse },
      { 'i', IndexQuantum, MagickFalse },
      { 'K', BlackQuantum, MagickTrue },
      { 'k', BlackQuantum, MagickTrue },
      { 'M', MagentaQuantum, MagickTrue },
      { 'm', MagentaQuantum, MagickTrue },
      { 'O', OpacityQuantum, MagickFalse },
      { 'o', OpacityQuantum, MagickFalse },
      { 'P', UndefinedQuantum, MagickFalse },
      { 'p', UndefinedQuantum, MagickFalse },
      { 'R', RedQuantum, MagickFalse },
      { 'r', RedQuantum, MagickFalse },
      { 'Y', YellowQuantum, MagickTrue },
      { 'y', YellowQuantum, MagickTrue }
    };

  ssize_t
    i;

  for (i=0; i < (ssize_t) (sizeof(map_info)/sizeof(*map_info)); i++)
    if (map_info[i].key == key)
      return(map_info+i);
  return((const StreamMapInfo *) NULL);
}

typedef struct _StreamStorageMethod
{
  StorageType
    storage_type;

  void
    (*stream_pixels)(const StreamInfo *,const Image *,QuantumType *,size_t);
} StreamStorageMethod;

static const StreamStorageMethod
  StreamStorageMethods[] =
  {
    { CharPixel, StreamCharPixels },
    { DoublePixel, StreamDoublePixels },
    { FloatPixel, StreamFloatPixels },
    { LongPixel, StreamLongPixels },
    { LongLongPixel, StreamLongLongPixels },
    { QuantumPixel, StreamQuantumPixels },
    { ShortPixel, StreamShortPixels }
  };

static const StreamStorageMethod *GetStreamStorageMethod(
  const StorageType storage_type)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) (sizeof(StreamStorageMethods)/
       sizeof(*StreamStorageMethods)); i++)
    if (StreamStorageMethods[i].storage_type == storage_type)
      return(StreamStorageMethods+i);
  return((const StreamStorageMethod *) NULL);
}

static inline MagickBooleanType IsStreamMapChannelAvailable(
  const StreamMapInfo *map_info,const Image *image)
{
  return(((map_info->separation == MagickFalse) ||
    (image->colorspace == CMYKColorspace)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType StreamImagePixels(const StreamInfo *stream_info,
  const Image *image,ExceptionInfo *exception)
{
  const StreamMapInfo
    *map_info;

  const StreamStorageMethod
    *storage_method;

  QuantumType
    *quantum_map;

  ssize_t
    i;

  size_t
    length;

  assert(stream_info != (StreamInfo *) NULL);
  assert(stream_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  length=strlen(stream_info->map);
  quantum_map=(QuantumType *) AcquireQuantumMemory(length,sizeof(*quantum_map));
  if (quantum_map == (QuantumType *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  (void) memset(quantum_map,0,length*sizeof(*quantum_map));
  for (i=0; i < (ssize_t) length; i++)
  {
    map_info=GetStreamMapInfo(stream_info->map[i]);
    if (map_info == (const StreamMapInfo *) NULL)
      {
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "UnrecognizedPixelMap","`%s'",stream_info->map);
        return(MagickFalse);
      }
    quantum_map[i]=map_info->quantum;
    if (IsStreamMapChannelAvailable(map_info,image) != MagickFalse)
      continue;
    quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
    (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
      "ColorSeparatedImageRequired","`%s'",stream_info->map);
    return(MagickFalse);
  }
  storage_method=GetStreamStorageMethod(stream_info->storage_type);
  if (storage_method != (const StreamStorageMethod *) NULL)
    storage_method->stream_pixels(stream_info,image,quantum_map,length);
  else
    {
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnrecognizedPixelMap","`%s'",stream_info->map);
    }
  quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
  return(MagickTrue);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e S t r e a m                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteStream() makes the image pixels available to a user supplied callback
%  method immediately upon writing pixel data with the WriteImage() method.
%
%  The format of the WriteStream() method is:
%
%      MagickBooleanType WriteStream(const ImageInfo *image_info,Image *,
%        StreamHandler stream,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o stream: A callback method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType WriteStream(const ImageInfo *image_info,
  Image *image,StreamHandler stream,ExceptionInfo *exception)
{
  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  write_info=CloneImageInfo(image_info);
  *write_info->magick='\0';
  write_info->stream=stream;
  status=WriteImage(write_info,image,exception);
  write_info=DestroyImageInfo(write_info);
  return(status);
}
