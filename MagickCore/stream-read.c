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
%                         RRRR   EEEEE   AAA   DDDD                           %
%                         R   R  E      A   A  D   D                          %
%                         RRRR   EEE    AAAAA  D   D                          %
%                         R R    E      A   A  D   D                          %
%                         R  R   EEEEE  A   A  DDDD                           %
%                                                                             %
%                                                                             %
%                       MagickCore Read Stream Methods                        %
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
#include "MagickCore/stream-read-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d S t r e a m                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadStream() makes the image pixels available to a user supplied callback
%  method immediately upon reading a scanline with the ReadImage() method.
%
%  The format of the ReadStream() method is:
%
%      Image *ReadStream(const ImageInfo *image_info,StreamHandler stream,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o stream: a callback method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *ReadStream(const ImageInfo *image_info,StreamHandler stream,
  ExceptionInfo *exception)
{
  CacheMethods
    cache_methods;

  Image
    *image;

  ImageInfo
    *read_info;

  /*
    Stream image pixels.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  read_info=CloneImageInfo(image_info);
  read_info->cache=AcquirePixelCache(0);
  GetPixelCacheMethods(&cache_methods);
  cache_methods.get_virtual_pixel_handler=GetVirtualPixelStream;
  cache_methods.get_virtual_pixels_handler=GetVirtualPixelsStream;
  cache_methods.get_virtual_metacontent_from_handler=
    GetVirtualMetacontentFromStream;
  cache_methods.get_authentic_pixels_handler=GetAuthenticPixelsStream;
  cache_methods.queue_authentic_pixels_handler=QueueAuthenticPixelsStream;
  cache_methods.sync_authentic_pixels_handler=SyncAuthenticPixelsStream;
  cache_methods.get_authentic_pixels_from_handler=GetAuthenticPixelsFromStream;
  cache_methods.get_authentic_metacontent_from_handler=
    GetAuthenticMetacontentFromStream;
  cache_methods.get_one_virtual_pixel_from_handler=GetOneVirtualPixelFromStream;
  cache_methods.get_one_authentic_pixel_from_handler=
    GetOneAuthenticPixelFromStream;
  cache_methods.destroy_pixel_handler=DestroyPixelStream;
  SetPixelCacheMethods(read_info->cache,&cache_methods);
  read_info->stream=stream;
  image=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  if (image != (Image *) NULL)
    {
      MagickBooleanType status = ResetPixelChannelMap(image,exception);
      if (status == MagickFalse)
        return(DestroyImage(image));
      ResetPixelCacheChannels(image);
    }
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c A u t h e n t i c P i x e l s S t r e a m                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncAuthenticPixelsStream() calls the user supplied callback method with
%  the latest stream of pixels.
%
%  The format of the SyncAuthenticPixelsStream method is:
%
%      MagickBooleanType SyncAuthenticPixelsStream(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType SyncAuthenticPixelsStream(Image *image,
  ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  size_t
    length;

  StreamHandler
    stream_handler;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickCoreSignature);
  stream_handler=GetBlobStreamHandler(image);
  if (stream_handler == (StreamHandler) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "NoStreamHandlerIsDefined","`%s'",image->filename);
      return(MagickFalse);
    }
  length=stream_handler(image,cache_info->pixels,(size_t) cache_info->columns);
  return(length == cache_info->columns ? MagickTrue : MagickFalse);
}
