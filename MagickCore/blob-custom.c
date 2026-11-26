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
%                   CCCC  U   U  SSSSS  TTTTT   OOO   M   M                   %
%                  C      U   U  SS       T    O   O  MM MM                   %
%                  C      U   U   SSS     T    O   O  M M M                   %
%                  C      U   U     SS    T    O   O  M   M                   %
%                   CCCC   UUU   SSSSS    T     OOO   M   M                   %
%                                                                             %
%                                                                             %
%                      MagickCore Custom Stream Methods                       %
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
+   A c q u i r e C u s t o m S t r e a m I n f o                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireCustomStreamInfo() allocates the CustomStreamInfo structure.
%
%  The format of the AcquireCustomStreamInfo method is:
%
%      CustomStreamInfo *AcquireCustomStreamInfo(ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport CustomStreamInfo *AcquireCustomStreamInfo(
  ExceptionInfo *magick_unused(exception))
{
  CustomStreamInfo
    *custom_stream;

  magick_unreferenced(exception);
  custom_stream=(CustomStreamInfo *) AcquireCriticalMemory(
    sizeof(*custom_stream));
  (void) memset(custom_stream,0,sizeof(*custom_stream));
  custom_stream->signature=MagickCoreSignature;
  return(custom_stream);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A t t a c h C u s t o m S t r e a m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AttachCustomStream() attaches a CustomStreamInfo to the BlobInfo structure.
%
%  The format of the AttachCustomStream method is:
%
%      void AttachCustomStream(BlobInfo *blob_info,
%        CustomStreamInfo *custom_stream)
%
%  A description of each parameter follows:
%
%    o blob_info: specifies a pointer to a BlobInfo structure.
%
%    o custom_stream: the custom stream info.
%
*/
MagickExport void AttachCustomStream(BlobInfo *blob_info,
  CustomStreamInfo *custom_stream)
{
  assert(blob_info != (BlobInfo *) NULL);
  assert(custom_stream != (CustomStreamInfo *) NULL);
  assert(custom_stream->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  blob_info->type=CustomStream;
  blob_info->custom_stream=custom_stream;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y C u s t o m S t r e a m I n f o                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyCustomStreamInfo() destroys memory associated with the
%  CustomStreamInfo structure.
%
%  The format of the DestroyCustomStreamInfo method is:
%
%      CustomStreamInfo *DestroyCustomStreamInfo(CustomStreamInfo *stream_info)
%
%  A description of each parameter follows:
%
%    o custom_stream: the custom stream info.
%
*/
MagickExport CustomStreamInfo *DestroyCustomStreamInfo(
  CustomStreamInfo *custom_stream)
{
  assert(custom_stream != (CustomStreamInfo *) NULL);
  assert(custom_stream->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  custom_stream->signature=(~MagickCoreSignature);
  custom_stream=(CustomStreamInfo *) RelinquishMagickMemory(custom_stream);
  return(custom_stream);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S e t C u s t o m S t r e a m D a t a                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCustomStreamData() sets the stream info data member.
%
%  The format of the SetCustomStreamData method is:
%
%      void SetCustomStreamData(CustomStreamInfo *custom_stream,void *)
%
%  A description of each parameter follows:
%
%    o custom_stream: the custom stream info.
%
%    o data: an object containing information about the custom stream.
%
*/
MagickExport void SetCustomStreamData(CustomStreamInfo *custom_stream,
  void *data)
{
  assert(custom_stream != (CustomStreamInfo *) NULL);
  assert(custom_stream->signature == MagickCoreSignature);
  custom_stream->data=data;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S e t C u s t o m S t r e a m R e a d e r                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCustomStreamReader() sets the stream info reader member.
%
%  The format of the SetCustomStreamReader method is:
%
%      void SetCustomStreamReader(CustomStreamInfo *custom_stream,
%        CustomStreamHandler reader)
%
%  A description of each parameter follows:
%
%    o custom_stream: the custom stream info.
%
%    o reader: a function to read from the stream.
%
*/
MagickExport void SetCustomStreamReader(CustomStreamInfo *custom_stream,
  CustomStreamHandler reader)
{
  assert(custom_stream != (CustomStreamInfo *) NULL);
  assert(custom_stream->signature == MagickCoreSignature);
  custom_stream->reader=reader;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S e t C u s t o m S t r e a m S e e k e r                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCustomStreamSeeker() sets the stream info seeker member.
%
%  The format of the SetCustomStreamReader method is:
%
%      void SetCustomStreamSeeker(CustomStreamInfo *custom_stream,
%        CustomStreamSeeker seeker)
%
%  A description of each parameter follows:
%
%    o custom_stream: the custom stream info.
%
%    o seeker: a function to seek in the custom stream.
%
*/
MagickExport void SetCustomStreamSeeker(CustomStreamInfo *custom_stream,
  CustomStreamSeeker seeker)
{
  assert(custom_stream != (CustomStreamInfo *) NULL);
  assert(custom_stream->signature == MagickCoreSignature);
  custom_stream->seeker=seeker;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S e t C u s t o m S t r e a m T e l l e r                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCustomStreamTeller() sets the stream info teller member.
%
%  The format of the SetCustomStreamTeller method is:
%
%      void SetCustomStreamTeller(CustomStreamInfo *custom_stream,
%        CustomStreamTeller *teller)
%
%  A description of each parameter follows:
%
%    o custom_stream: the custom stream info.
%
%    o teller: a function to set the position in the stream.
%
*/
MagickExport void SetCustomStreamTeller(CustomStreamInfo *custom_stream,
  CustomStreamTeller teller)
{
  assert(custom_stream != (CustomStreamInfo *) NULL);
  assert(custom_stream->signature == MagickCoreSignature);
  custom_stream->teller=teller;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S e t C u s t o m S t r e a m W r i t e r                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCustomStreamWriter() sets the stream info writer member.
%
%  The format of the SetCustomStreamWriter method is:
%
%      void SetCustomStreamWriter(CustomStreamInfo *custom_stream,
%        CustomStreamHandler *writer)
%
%  A description of each parameter follows:
%
%    o custom_stream: the custom stream info.
%
%    o writer: a function to write to the custom stream.
%
*/
MagickExport void SetCustomStreamWriter(CustomStreamInfo *custom_stream,
  CustomStreamHandler writer)
{
  assert(custom_stream != (CustomStreamInfo *) NULL);
  assert(custom_stream->signature == MagickCoreSignature);
  custom_stream->writer=writer;
}
