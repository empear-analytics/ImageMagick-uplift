/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  M   M  EEEEE  M   M   OOO   RRRR   Y   Y                   %
%                  MM MM  E      MM MM  O   O  R   R   Y Y                    %
%                  M M M  EEE    M M M  O   O  RRRR     Y                     %
%                  M   M  E      M   M  O   O  R R      Y                     %
%                  M   M  EEEEE  M   M   OOO   R  R     Y                     %
%                                                                             %
%                QQQ   U   U   AAA   N   N  TTTTT  U   U  M   M               %
%               Q   Q  U   U  A   A  NN  N    T    U   U  MM MM               %
%               Q   Q  U   U  AAAAA  N N N    T    U   U  M M M               %
%               Q  QQ  U   U  A   A  N  NN    T    U   U  M   M               %
%                QQQQ   UUU   A   A  N   N    T     UUU   M   M               %
%                                                                             %
%                                                                             %
%                 Quantum Memory and Memory Block Operations                  %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1998                                   %
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
%  We provide these memory allocators:
%
%    AcquireCriticalMemory(): allocate a small memory request with
%      AcquireMagickMemory(), however, on fail throw a fatal exception and exit.
%      Free the memory reserve with RelinquishMagickMemory().
%    AcquireAlignedMemory(): allocate a small memory request that is aligned
%      on a cache line.  On fail, return NULL for possible recovery.
%      Free the memory reserve with RelinquishMagickMemory().
%    AcquireMagickMemory()/ResizeMagickMemory(): allocate a small to medium
%      memory request, typically with malloc()/realloc(). On fail, return NULL
%      for possible recovery.  Free the memory reserve with
%      RelinquishMagickMemory().
%    AcquireQuantumMemory()/ResizeQuantumMemory(): allocate a small to medium
%      memory request.  This is a secure memory allocator as it accepts two
%      parameters, count and quantum, to ensure the request does not overflow.
%      It also check to ensure the request does not exceed the maximum memory
%      per the security policy.  Free the memory reserve with
%      RelinquishMagickMemory().
%    AcquireVirtualMemory(): allocate a large memory request either in heap,
%      memory-mapped, or memory-mapped on disk depending on whether heap
%      allocation fails or if the request exceeds the maximum memory policy.
%      Free the memory reserve with RelinquishVirtualMemory().
%    ResetMagickMemory(): fills the bytes of the memory area with a constant
%      byte.
%    
%  In addition, we provide hooks for your own memory constructor/destructors.
%  You can also utilize our internal custom allocator as follows: Segregate
%  our memory requirements from any program that calls our API.  This should
%  help reduce the risk of others changing our program state or causing memory
%  corruption.
%
%  Our custom memory allocation manager implements a best-fit allocation policy
%  using segregated free lists.  It uses a linear distribution of size classes
%  for lower sizes and a power of two distribution of size classes at higher
%  sizes.  It is based on the paper, "Fast Memory Allocation using Lazy Fits."
%  written by Yoo C. Chung.
%
%  By default, C's standard library is used (e.g. malloc);  use the
%  custom memory allocator by defining MAGICKCORE_ANONYMOUS_MEMORY_SUPPORT
%  to allocate memory with private anonymous mapping rather than from the
%  heap.
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
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/utility-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e Q u a n t u m M e m o r y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireQuantumMemory() returns a pointer to a block of memory at least
%  count * quantum bytes suitably aligned for any use.
%
%  The format of the AcquireQuantumMemory method is:
%
%      void *AcquireQuantumMemory(const size_t count,const size_t quantum)
%
%  A description of each parameter follows:
%
%    o count: the number of objects to allocate contiguously.
%
%    o quantum: the size (in bytes) of each object.
%
*/
MagickExport void *AcquireQuantumMemory(const size_t count,const size_t quantum)
{
  size_t
    size;

  if ((HeapOverflowSanityCheckGetSize(count,quantum,&size) != MagickFalse) ||
      (size > GetMaxMemoryRequest()))
    {
      errno=ENOMEM;
      return(NULL);
    }
  return(AcquireMagickMemory(size));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o p y M a g i c k M e m o r y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CopyMagickMemory() copies size bytes from memory area source to the
%  destination.  Copying between objects that overlap will take place
%  correctly.  It returns destination.
%
%  The format of the CopyMagickMemory method is:
%
%      void *CopyMagickMemory(void *magick_restrict destination,
%        const void *magick_restrict source,const size_t size)
%
%  A description of each parameter follows:
%
%    o destination: the destination.
%
%    o source: the source.
%
%    o size: the size of the memory in bytes to allocate.
%
*/
MagickExport void *CopyMagickMemory(void *magick_restrict destination,
  const void *magick_restrict source,const size_t size)
{
  const unsigned char
    *p;

  unsigned char
    *q;

  assert(destination != (void *) NULL);
  assert(source != (const void *) NULL);
  p=(const unsigned char *) source;
  q=(unsigned char *) destination;
  if (((q+size) < p) || (q > (p+size)))
    switch (size)
    {
      default: return(memcpy(destination,source,size));
      case 8: *q++=(*p++); magick_fallthrough;
      case 7: *q++=(*p++); magick_fallthrough;
      case 6: *q++=(*p++); magick_fallthrough;
      case 5: *q++=(*p++); magick_fallthrough;
      case 4: *q++=(*p++); magick_fallthrough;
      case 3: *q++=(*p++); magick_fallthrough;
      case 2: *q++=(*p++); magick_fallthrough;
      case 1: *q++=(*p++); magick_fallthrough;
      case 0: return(destination);
    }
  return(memmove(destination,source,size));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s i z e Q u a n t u m M e m o r y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResizeQuantumMemory() changes the size of the memory and returns a pointer
%  to the (possibly moved) block.  The contents will be unchanged up to the
%  lesser of the new and old sizes.
%
%  The format of the ResizeQuantumMemory method is:
%
%      void *ResizeQuantumMemory(void *memory,const size_t count,
%        const size_t quantum)
%
%  A description of each parameter follows:
%
%    o memory: A pointer to a memory allocation.
%
%    o count: the number of objects to allocate contiguously.
%
%    o quantum: the size (in bytes) of each object.
%
*/
MagickExport void *ResizeQuantumMemory(void *memory,const size_t count,
  const size_t quantum)
{
  size_t
    size;

  if ((HeapOverflowSanityCheckGetSize(count,quantum,&size) != MagickFalse) ||
      (size > GetMaxMemoryRequest()))
    {
      errno=ENOMEM;
      memory=RelinquishMagickMemory(memory);
      return(NULL);
    }
  return(ResizeMagickMemory(memory,size));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S h r e d M a g i c k M e m o r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShredMagickMemory() overwrites the specified memory buffer with random data.
%  The overwrite is optional and is only required to help keep the contents of
%  the memory buffer private.
%
%  The format of the ShredMagickMemory method is:
%
%      MagickBooleanType ShredMagickMemory(void *memory,const size_t length)
%
%  A description of each parameter follows.
%
%    o memory:  Specifies the memory buffer.
%
%    o length:  Specifies the length of the memory buffer.
%
*/
static MagickBooleanType OverwriteMagickMemory(void *memory,
  const size_t length,const ssize_t passes)
{
  RandomInfo
    *random_info;

  size_t
    quantum;

  ssize_t
    i;

  StringInfo
    *key;

  /*
    Overwrite the memory buffer with random data.
  */
  quantum=(size_t) MagickMin(length,MagickMinBufferExtent);
  random_info=AcquireRandomInfo();
  key=GetRandomKey(random_info,quantum);
  for (i=0; i < passes; i++)
  {
    size_t
      j;

    unsigned char
      *p = (unsigned char *) memory;

    for (j=0; j < length; j+=quantum)
    {
      if (i != 0)
        SetRandomKey(random_info,quantum,GetStringInfoDatum(key));
      (void) memcpy(p,GetStringInfoDatum(key),(size_t)
        MagickMin(quantum,length-j));
      p+=(ptrdiff_t) quantum;
    }
    if (j < length)
      break;
  }
  key=DestroyStringInfo(key);
  random_info=DestroyRandomInfo(random_info);
  return(i < passes ? MagickFalse : MagickTrue);
}

MagickPrivate MagickBooleanType ShredMagickMemory(void *memory,
  const size_t length)
{
  static ssize_t
    passes = -1;

  if ((memory == NULL) || (length == 0))
    return(MagickFalse);
  if (passes == -1)
    passes=GetShredPasses();
  if (passes == 0)
    return(MagickTrue);
  return(OverwriteMagickMemory(memory,length,passes));
}
