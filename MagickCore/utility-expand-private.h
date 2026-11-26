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

  MagickCore private methods of utility.c: the argument expansion helpers of
  ExpandFilenames, FormatExpandedPath to FinishArgumentVector; included by
  that file only.
*/
#ifndef MAGICKCORE_UTILITY_EXPAND_PRIVATE_H
#define MAGICKCORE_UTILITY_EXPAND_PRIVATE_H

static void FormatExpandedPath(char *file_path,const char *magick,
  const char *filename,const char *subimage)
{
  *file_path='\0';
  if (*magick != '\0')
    {
      (void) ConcatenateMagickString(file_path,magick,
        MagickPathExtent);
      (void) ConcatenateMagickString(file_path,":",MagickPathExtent);
    }
  (void) ConcatenateMagickString(file_path,filename,MagickPathExtent);
  if (*subimage != '\0')
    {
      (void) ConcatenateMagickString(file_path,"[",MagickPathExtent);
      (void) ConcatenateMagickString(file_path,subimage,
        MagickPathExtent);
      (void) ConcatenateMagickString(file_path,"]",MagickPathExtent);
    }
}

static char *AcquireCommandLine(char **vector,const ssize_t count)
{
  char
    *command_line;

  ssize_t
    i;

  command_line=AcquireString(vector[0]);
  for (i=1; i < count; i++)
  {
    (void) ConcatenateString(&command_line," {");
    (void) ConcatenateString(&command_line,vector[i]);
    (void) ConcatenateString(&command_line,"}");
  }
  return(command_line);
}

static inline void getcwd_utf8(char *path,size_t extent)
{
#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__)
  char
    *directory;

   directory=getcwd(path,extent);
   (void) directory;
#else
  wchar_t
    wide_path[MagickPathExtent];

  (void) _wgetcwd(wide_path,MagickPathExtent-1);
  (void) WideCharToMultiByte(CP_UTF8,0,wide_path,-1,path,(int) extent,NULL,NULL);
#endif
}

static inline MagickBooleanType IsTextMagick(const char *magick)
{
  return(((LocaleCompare(magick,"CAPTION") == 0) ||
    (LocaleCompare(magick,"LABEL") == 0) ||
    (LocaleCompare(magick,"PANGO") == 0) ||
    (LocaleCompare(magick,"VID") == 0)) ? MagickTrue : MagickFalse);
}

static char **DestroyFileList(char **filelist,const size_t number_files)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) number_files; j++)
    filelist[j]=DestroyString(filelist[j]);
  filelist=(char **) RelinquishMagickMemory(filelist);
  return(filelist);
}

static void DestroyOriginalArgument(char **vector,ssize_t *count,
  MagickBooleanType *destroy)
{
  if ((*destroy) != MagickFalse)
    {
      (*count)--;
      vector[(*count)]=DestroyString(vector[(*count)]);
      (*destroy)=MagickFalse;
    }
}

typedef struct _FileListParameters
{
  char
    **vector,
    **filelist;

  size_t
    number_files;

  ssize_t
    parameters;
} FileListParameters;

static void CopyFileListParameters(const FileListParameters *list,ssize_t *j,
  ssize_t *count)
{
  char
    **const vector = list->vector,
    **const filelist = list->filelist,
    *option;

  const size_t
    number_files = list->number_files;

  const ssize_t
    parameters = list->parameters;

  ssize_t
    k;

  for (k=0; k < parameters; k++)
  {
    (*j)++;
    if ((*j) == (ssize_t) number_files)
      break;
    option=filelist[(*j)];
    vector[(*count)++]=ConstantString(option);
  }
}

static void CheckFilenameLength(const char *filename)
{
  if (strlen(filename) >= (MagickPathExtent-1))
    ThrowFatalException(OptionFatalError,"FilenameTruncated");
}

typedef struct _ArgumentParameters
{
  int
    *number_arguments;

  char
    ***arguments,
    **vector;

  ssize_t
    parameters;
} ArgumentParameters;

static void CopyArgumentParameters(const ArgumentParameters *list,ssize_t *i,
  ssize_t *count)
{
  int
    *const number_arguments = list->number_arguments;

  char
    ***const arguments = list->arguments,
    **const vector = list->vector,
    *option;

  const ssize_t
    parameters = list->parameters;

  ssize_t
    j;

  for (j=0; j < parameters; j++)
  {
    (*i)++;
    if ((*i) == (ssize_t) *number_arguments)
      break;
    option=(*arguments)[(*i)];
    vector[(*count)++]=ConstantString(option);
  }
}

typedef struct _ArgumentFileTargets
{
  char
    ***filelist;

  size_t
    *number_files;
} ArgumentFileTargets;

static MagickBooleanType ReadFileList(const char *option,
  const ArgumentFileTargets *targets)
{
  char
    ***const filelist = targets->filelist;

  size_t
    *const number_files = targets->number_files;

  char
    *files;

  ExceptionInfo
    *exception;

  int
    length;

  ssize_t
    j;

  exception=AcquireExceptionInfo();
  files=FileToString(option,~0UL,exception);
  exception=DestroyExceptionInfo(exception);
  if (files == (char *) NULL)
    return(MagickFalse);
  (*filelist)=StringToArgv(files,&length);
  if ((*filelist) == (char **) NULL)
    return(MagickFalse);
  files=DestroyString(files);
  (*filelist)[0]=DestroyString((*filelist)[0]);
  for (j=0; j < (ssize_t) (length-1); j++)
    (*filelist)[j]=(*filelist)[j+1];
  (*number_files)=(size_t) length-1;
  return(MagickTrue);
}

static ssize_t FindNonDirectory(char **filelist,const size_t number_files)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) number_files; j++)
    if (IsPathDirectory(filelist[j]) <= 0)
      break;
  return(j);
}

static MagickBooleanType ParseExpandableArgument(const char *option,
  char *filename,char *magick)
{
  if ((*option == '"') || (*option == '\''))
    return(MagickFalse);
  GetPathComponent(option,TailPath,filename);
  GetPathComponent(option,MagickPath,magick);
  if (IsTextMagick(magick) != MagickFalse)
    return(MagickFalse);
  if ((IsGlob(filename) == MagickFalse) && (*option != '@'))
    return(MagickFalse);
  if (IsPathAccessible(option) != MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void ComposeListedFilename(char *filename,const char *path,
  char **filelist,const ssize_t j)
{
  (void) CopyMagickString(filename,path,MagickPathExtent);
  if (*path != '\0')
    (void) ConcatenateMagickString(filename,DirectorySeparator,
      MagickPathExtent);
  if (filelist[j] != (char *) NULL)
    (void) ConcatenateMagickString(filename,filelist[j],MagickPathExtent);
  filelist[j]=DestroyString(filelist[j]);
}

typedef struct _FileListTransfer
{
  char
    **vector,
    **filelist;

  size_t
    number_files;

  const char
    *path,
    *magick,
    *subimage;
} FileListTransfer;

static void TransferFileList(const FileListTransfer *transfer,char *filename,
  ssize_t *count,MagickBooleanType *destroy)
{
  char
    **const vector = transfer->vector,
    **const filelist = transfer->filelist,
    *option;

  const size_t
    number_files = transfer->number_files;

  const char
    *const path = transfer->path,
    *const magick = transfer->magick,
    *const subimage = transfer->subimage;

  ssize_t
    j,
    parameters;

  for (j=0; j < (ssize_t) number_files; j++)
  {
    option=filelist[j];
    parameters=ParseCommandOption(MagickCommandOptions,MagickFalse,option);
    if (parameters > 0)
      {
        FileListParameters
          list;

        /*
          Do not expand command option parameters.
        */
        vector[(*count)++]=ConstantString(option);
        list.vector=vector;
        list.filelist=filelist;
        list.number_files=number_files;
        list.parameters=parameters;
        CopyFileListParameters(&list,&j,&(*count));
        continue;
      }
    ComposeListedFilename(filename,path,filelist,j);
    CheckFilenameLength(filename);
    if (IsPathDirectory(filename) <= 0)
      {
        char
          file_path[MagickPathExtent];

        FormatExpandedPath(file_path,magick,filename,subimage);
        CheckFilenameLength(file_path);
        DestroyOriginalArgument(vector,&(*count),&(*destroy));
        vector[(*count)++]=ConstantString(file_path);
      }
  }
}

typedef struct _WildcardArgument
{
  char
    *option,
    *filename,
    *path,
    *subimage,
    *home_directory;
} WildcardArgument;

static void ListWildcardFiles(const WildcardArgument *wildcard,
  char ***filelist,size_t *number_files)
{
  char
    *const option = wildcard->option,
    *const filename = wildcard->filename,
    *const path = wildcard->path,
    *const subimage = wildcard->subimage,
    *const home_directory = wildcard->home_directory;

  /*
    Generate file list from wildcard filename (e.g. *.jpg).
  */
  GetPathComponent(option,HeadPath,path);
  GetPathComponent(option,SubimagePath,subimage);
  ExpandFilename(path);
  if (*home_directory == '\0')
    getcwd_utf8(home_directory,MagickPathExtent-1);
  (*filelist)=ListFiles(*path == '\0' ? home_directory : path,filename,
    number_files);
}

static MagickBooleanType ListArgumentFiles(const WildcardArgument *wildcard,
  char ***filelist,size_t *number_files)
{
  char
    *const option = wildcard->option;

  if (*option != '@')
    ListWildcardFiles(wildcard,filelist,number_files);
  else
    {
      ArgumentFileTargets
        targets;

      /*
        Generate file list from file list (e.g. @filelist.txt).
      */
      targets.filelist=filelist;
      targets.number_files=number_files;
      if (ReadFileList(option,&targets) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType HasFileListFiles(char ***filelist,
  const size_t number_files)
{
  ssize_t
    j;

  if ((*filelist) == (char **) NULL)
    return(MagickFalse);
  j=FindNonDirectory((*filelist),number_files);
  if (j == (ssize_t) number_files)
    {
      (*filelist)=DestroyFileList((*filelist),number_files);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void LogExpandedCommandLine(char **vector,const ssize_t count)
{
  if (IsEventLogging() != MagickFalse)
    {
      char
        *command_line;

      command_line=AcquireCommandLine(vector,count);
      (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
        "Command line: %s",command_line);
      command_line=DestroyString(command_line);
    }
}

static char **AcquireArgumentVector(const int *number_arguments,
  char ***arguments)
{
  char
    **vector;

  magick_unreferenced(arguments);
  /*
    Allocate argument vector.
  */
  assert(number_arguments != (int *) NULL);
  assert(arguments != (char ***) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  vector=(char **) AcquireQuantumMemory((size_t) (*number_arguments+1),
    sizeof(*vector));
  if (vector == (char **) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  return(vector);
}

static MagickBooleanType CollectArgumentFiles(
  const WildcardArgument *wildcard,char *magick,
  const ArgumentFileTargets *targets)
{
  char
    *const option = wildcard->option,
    *const filename = wildcard->filename;

  char
    ***const filelist = targets->filelist;

  size_t
    *const number_files = targets->number_files;

  if (ParseExpandableArgument(option,filename,magick) == MagickFalse)
    return(MagickFalse);
  if (ListArgumentFiles(wildcard,filelist,number_files) == MagickFalse)
    return(MagickFalse);
  if (HasFileListFiles(filelist,*number_files) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void FinishArgumentVector(int *number_arguments,char ***arguments,
  char **vector,const ssize_t count)
{
  vector[count]=(char *) NULL;
  LogExpandedCommandLine(vector,count);
  *number_arguments=(int) count;
  *arguments=vector;
}

#endif
