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

  MagickCore private methods of utility.c: the directory listing helpers of
  ListFiles, IsPathDirectory to CheckListFilesArguments; included by that
  file only.
*/
#ifndef MAGICKCORE_UTILITY_LIST_PRIVATE_H
#define MAGICKCORE_UTILITY_LIST_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  I s P a t h D i r e c t o r y                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPathDirectory() returns -1 if the directory does not exist,  1 is returned
%  if the path represents a directory otherwise 0.
%
%  The format of the IsPathDirectory method is:
%
%      int IsPathDirectory(const char *path)
%
%  A description of each parameter follows.
%
%   o  path:  The directory path.
%
*/
static int IsPathDirectory(const char *path)
{
  MagickBooleanType
    status;

  struct stat
    attributes;

  if ((path == (const char *) NULL) || (*path == '\0'))
    return(MagickFalse);
  status=GetPathAttributes(path,&attributes);
  if (status == MagickFalse)
    return(-1);
  if (S_ISDIR(attributes.st_mode) == 0)
    return(0);
  return(1);
}

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int FileCompare(const void *x,const void *y)
{
  const char
    **p,
    **q;

  p=(const char **) x;
  q=(const char **) y;
  return(LocaleCompare(*p,*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

typedef struct _FileListTargets
{
  size_t
    *max_entries;

  char
    ***filelist;
} FileListTargets;

static MagickBooleanType ExtendFileList(const size_t *number_entries,
  const FileListTargets *targets)
{
  size_t
    *const max_entries = targets->max_entries;

  char
    ***const filelist = targets->filelist;

  if (*number_entries >= (*max_entries))
    {
      /*
        Extend the file list.
      */
      (*max_entries)<<=1;
      (*filelist)=(char **) ResizeQuantumMemory((*filelist),(size_t)
        (*max_entries),sizeof(*(*filelist)));
      if ((*filelist) == (char **) NULL)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static inline MagickBooleanType IsDotDirectoryEntry(const char *name)
{
  return(((LocaleCompare(name,".") == 0) ||
    (LocaleCompare(name,"..") == 0)) ? MagickTrue : MagickFalse);
}

typedef struct _FileListDirectoryTargets
{
  DIR
    **current_directory;

  size_t
    *max_entries;

  char
    ***filelist;

  struct dirent
    **buffer;
} FileListDirectoryTargets;

static MagickBooleanType OpenFileListDirectory(const char *directory,
  const FileListDirectoryTargets *targets)
{
  DIR
    **const current_directory = targets->current_directory;

  size_t
    *const max_entries = targets->max_entries;

  char
    ***const filelist = targets->filelist;

  struct dirent
    **const buffer = targets->buffer;

  (*current_directory)=opendir(directory);
  if ((*current_directory) == (DIR *) NULL)
    return(MagickFalse);
  /*
    Allocate filelist.
  */
  (*max_entries)=2048;
  (*filelist)=(char **) AcquireQuantumMemory((size_t) (*max_entries),
    sizeof(*(*filelist)));
  if ((*filelist) == (char **) NULL)
    {
      (void) closedir((*current_directory));
      return(MagickFalse);
    }
  /*
    Save the current and change to the new directory.
  */
  (*buffer)=(struct dirent *) AcquireMagickMemory(sizeof(*(*buffer))+
    FILENAME_MAX+1);
  if ((*buffer) == (struct dirent *) NULL)
    {
      (*filelist)=(char **) RelinquishMagickMemory((*filelist));
      (void) closedir((*current_directory));
      return(MagickFalse);
    }
  return(MagickTrue);
}

static char **SortFileList(char **filelist,const size_t *number_entries)
{
  if (filelist == (char **) NULL)
    return((char **) NULL);
  /*
    Sort filelist in ascending order.
  */
  qsort((void *) filelist,(size_t) *number_entries,sizeof(*filelist),
    FileCompare);
  return(filelist);
}

static void CheckListFilesArguments(const char *directory,
  const char *pattern,size_t *number_entries)
{
  magick_unreferenced(pattern);
  /*
    Open directory.
  */
  assert(directory != (const char *) NULL);
  assert(pattern != (const char *) NULL);
  assert(number_entries != (size_t *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",directory);
  *number_entries=0;
}

#endif
