/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private declarations of blob.c: its define block, the FileInfo
  union and the _BlobInfo and _CustomStreamInfo structures.
*/
#ifndef MAGICKCORE_BLOB_INFO_PRIVATE_H
#define MAGICKCORE_BLOB_INFO_PRIVATE_H

/*
  Define declarations.
*/
#define MagickMaxBlobExtent  (8*8192)
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
# define MAP_ANONYMOUS  MAP_ANON
#endif
#if !defined(MAP_FAILED)
#define MAP_FAILED  ((void *) -1)
#endif
#if defined(__OS2__)
#include <io.h>
#define _O_BINARY  O_BINARY
#endif
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
#  if !defined(fsync)
#    define fsync  _commit
#  endif
#  if !defined(mmap)
#    define MAGICKCORE_HAVE_MMAP 1
#    define mmap(address,length,protection,access,file,offset) \
  NTMapMemory(address,length,protection,access,file,offset)
#  endif
#  if !defined(munmap)
#    define munmap(address,length)  NTUnmapMemory(address,length)
#  endif
#  if !defined(pclose)
#    define pclose  _pclose
#  endif
#  if !defined(popen)
#    define popen  _popen
#  endif
#endif

/*
  Typedef declarations.
*/
typedef union FileInfo
{
  FILE
    *file;

#if defined(MAGICKCORE_ZLIB_DELEGATE)
  gzFile
    gzfile;
#endif

#if defined(MAGICKCORE_BZLIB_DELEGATE)
  BZFILE
    *bzfile;
#endif
} FileInfo;

struct _BlobInfo
{
  size_t
    length,
    extent,
    quantum;

  BlobMode
    mode;

  MagickBooleanType
    mapped,
    eof;

  int
    error,
    error_number;

  MagickOffsetType
    offset;

  MagickSizeType
    size;

  MagickBooleanType
    exempt,
    synchronize,
    temporary;

  int
    status;

  StreamType
    type;

  FileInfo
    file_info;

  struct stat
    properties;

  StreamHandler
    stream;

  CustomStreamInfo
    *custom_stream;

  unsigned char
    *data;

  MagickBooleanType
    debug;

  SemaphoreInfo
    *semaphore;

  ssize_t
    reference_count;

  size_t
    signature;
};

struct _CustomStreamInfo
{
  CustomStreamHandler
    reader,
    writer;

  CustomStreamSeeker
    seeker;

  CustomStreamTeller
    teller;

  void
    *data;

  size_t
    signature;
};

#endif
