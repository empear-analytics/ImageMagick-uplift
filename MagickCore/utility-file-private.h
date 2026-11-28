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

  MagickCore private file utility methods: the path and file helpers and the
  UTF-8 file wrappers access_utf8 to is_symlink_utf8, MagickRead,
  MagickReadDirectory and MagickWrite. Included by utility-private.h only.
*/
#ifndef MAGICKCORE_UTILITY_FILE_PRIVATE_H
#define MAGICKCORE_UTILITY_FILE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define MagickPathTemplate "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  /* min 6 X's */

extern MagickPrivate char
  **GetPathComponents(const char *,size_t *),
  **ListFiles(const char *,const char *,size_t *);

extern MagickPrivate MagickBooleanType
  GetExecutionPath(char *,const size_t),
  ShredFile(const char *);

extern MagickPrivate ssize_t
  GetMagickPageSize(void);

extern MagickPrivate void
  ChopPathComponents(char *,const size_t),
  ExpandFilename(char *);

static inline int access_utf8(const char *path,int mode)
{
  if (path == (const char *) NULL)
    return(-1);
#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__)
  return(access(path,mode));
#else
  return(NTAccessWide(path,mode));
#endif
}

#if defined(MAGICKCORE_WINDOWS_SUPPORT) && !defined(__CYGWIN__)
#define close_utf8 _close
#else
#define close_utf8 close
#endif

static inline FILE *fopen_utf8(const char *path,const char *mode)
{
#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__)
  return(fopen(path,mode));
#else
  return(NTOpenFileWide(path,mode));
#endif
}

static inline MagickBooleanType is_symlink_utf8(const char *path)
{
#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__)
#if defined(MAGICKCORE_POSIX_SUPPORT)
  struct stat
    status;

  if (lstat(path,&status) == -1)
    return(MagickFalse);
  return(S_ISLNK(status.st_mode) != 0 ? MagickTrue : MagickFalse);
#else
  return(MagickFalse);
#endif
#else
 return(NTIsSymlinkWide(path));
#endif
}

static inline ssize_t MagickRead(int fd,void *buffer,size_t extent)
{
  unsigned char *p = (unsigned char *) buffer;
  size_t offset = 0;
  while (offset < extent)
  {
    ssize_t count = read(fd,p+offset,extent-offset);
    if (count < 0)
      {
        if (errno == EINTR)
          continue;
        return(-1);
      }
    if (count == 0)
      break;
    offset+=(size_t) count;
  }
  return((ssize_t) offset);
}

static inline int MagickReadDirectory(DIR *directory,struct dirent *entry,
  struct dirent **result)
{
  (void) entry;
  errno=0;
  *result=readdir(directory);
  return(errno);
}

static inline ssize_t MagickWrite(int fd,const void *buffer,size_t extent)
{
  const unsigned char *p = (const unsigned char *) buffer;
  size_t offset = 0;
  while (offset < extent)
  {
    ssize_t count = write(fd,p+offset,extent-offset);
    if (count < 0) {
      if (errno == EINTR)
        continue;
      return(-1);
    }
    if (count == 0)
      break;
    offset+=(size_t) count;
  }
  return((ssize_t) offset);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
