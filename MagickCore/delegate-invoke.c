/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%           DDDD   EEEEE  L      EEEEE   GGGG   AAA   TTTTT  EEEEE            %
%           D   D  E      L      E      G      A   A    T    E                %
%           D   D  EEE    L      EEE    G  GG  AAAAA    T    EEE              %
%           D   D  E      L      E      G   G  A   A    T    E                %
%           DDDD   EEEEE  LLLLL  EEEEE   GGG   A   A    T    EEEEE            %
%                                                                             %
%                  IIIII  N   N  V   V   OOO   K   K  EEEEE                   %
%                    I    NN  N  V   V  O   O  K  K   E                       %
%                    I    N N N  V   V  O   O  KKK    EEE                     %
%                    I    N  NN   V V   O   O  K  K   E                       %
%                  IIIII  N   N    V     OOO   K   K  EEEEE                   %
%                                                                             %
%                                                                             %
%               Delegate Command Interpretation and Invocation                %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               October 1998                                  %
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
%  The Delegates methods associate a set of commands with a particular
%  image format.  ImageMagick uses delegates for formats it does not handle
%  directly.
%
%  Thanks to Bob Friesenhahn for the initial inspiration and design of the
%  delegates methods.
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/client.h"
#include "MagickCore/configure.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/delegate-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/linked-list-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/policy.h"
#include "MagickCore/policy-private.h"
#include "MagickCore/property.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature.h"
#include "MagickCore/string_.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xml-tree.h"
#include "MagickCore/xml-tree-private.h"
#include "MagickCore/delegate-property-private.h"
#include "MagickCore/delegate-invoke-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e C o m m a n d                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateCommand() replaces any embedded formatting characters with the
%  appropriate image attribute and returns the resulting command.
%
%  The format of the GetDelegateCommand method is:
%
%      char *GetDelegateCommand(const ImageInfo *image_info,Image *image,
%        const char *decode,const char *encode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o command: Method GetDelegateCommand returns the command associated
%      with specified delegate tag.
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o decode: Specifies the decode delegate we are searching for as a
%      character string.
%
%    o encode: Specifies the encode delegate we are searching for as a
%      character string.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport char *GetDelegateCommand(const ImageInfo *image_info,Image *image,
  const char *decode,const char *encode,ExceptionInfo *exception)
{
  char
    *command,
    **commands;

  const DelegateInfo
    *delegate_info;

  ssize_t
    i;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  delegate_info=GetDelegateInfo(decode,encode,exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
        "NoTagFound","`%s'",decode ? decode : encode);
      return((char *) NULL);
    }
  commands=StringToList(delegate_info->commands);
  if (commands == (char **) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",decode ? decode :
        encode);
      return((char *) NULL);
    }
  command=InterpretDelegateProperties((ImageInfo *) image_info,image,
    commands[0],exception);
  if (command == (char *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),ResourceLimitError,
      "MemoryAllocationFailed","`%s'",commands[0]);
  /*
    Relinquish resources.
  */
  for (i=0; commands[i] != (char *) NULL; i++)
    commands[i]=DestroyString(commands[i]);
  commands=(char **) RelinquishMagickMemory(commands);
  return(command);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n v o k e D e l e g a t e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InvokeDelegate replaces any embedded formatting characters with the
%  appropriate image attribute and executes the resulting command.  MagickFalse
%  is returned if the commands execute with success otherwise MagickTrue.
%
%  The format of the InvokeDelegate method is:
%
%      MagickBooleanType InvokeDelegate(ImageInfo *image_info,Image *image,
%        const char *decode,const char *encode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the imageInfo.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType AuthorizeDelegateCopy(const char *source,
  const char *destination,const int destination_file,ExceptionInfo *exception)
{
  if (IsPathAuthorized(WritePolicyRights,destination) == MagickFalse)
    ThrowPolicyException(destination,MagickFalse);
  if (IsPathAuthorized(ReadPolicyRights,source) == MagickFalse)
    {
      (void) close_utf8(destination_file);
      ThrowPolicyException(source,MagickFalse);
    }
  return(MagickTrue);
}

static inline MagickBooleanType IsKeptDestination(
  const MagickBooleanType overwrite,const char *destination,
  struct stat *attributes)
{
  return((MagickBooleanType) ((overwrite == MagickFalse) &&
    (GetPathAttributes(destination,attributes) != MagickFalse)));
}

static size_t GetDelegateCopyQuantum(const int source_file)
{
  size_t
    quantum;

  struct stat
    attributes = { 0 };

  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(source_file,&attributes) == 0) && (attributes.st_size > 0))
    quantum=MagickMin((size_t) attributes.st_size,MagickMaxBufferExtent);
  return(quantum);
}

typedef struct _DelegateFileCopy
{
  int
    source_file,
    destination_file;

  unsigned char
    *buffer;

  size_t
    quantum;
} DelegateFileCopy;

static void CopyDelegateFileData(const DelegateFileCopy *copy,
  MagickBooleanType *status)
{
  const int
    source_file = copy->source_file,
    destination_file = copy->destination_file;

  unsigned char
    *const buffer = copy->buffer;

  const size_t
    quantum = copy->quantum;

  ssize_t
    count = 0;

  while ((count=read(source_file,buffer,quantum)) > 0)
  {
    ssize_t written = write(destination_file,buffer,(size_t) count);
    if (written != count)
      {
        *status=MagickFalse;
        break;
      }
  }
  if (count < 0)
    *status=MagickFalse;
}

static MagickBooleanType CopyDelegateFile(const char *source,
  const char *destination,const MagickBooleanType overwrite,
  ExceptionInfo *exception)
{
  DelegateFileCopy
    copy;

  int
    destination_file = -1,
    source_file = -1;

  MagickBooleanType
    status = MagickTrue;

  size_t
    quantum = 0;

  struct stat
    attributes = { 0 };

  unsigned char
    *buffer;

  /*
    Copy source file to destination.
  */
  assert(source != (const char *) NULL);
  assert(destination != (char *) NULL);
  if (IsKeptDestination(overwrite,destination,&attributes) != MagickFalse)
    return(MagickTrue);
  if (IsPathAuthorized(WritePolicyRights,destination) == MagickFalse)
    ThrowPolicyException(source,MagickFalse);
  destination_file=open_utf8(destination,O_WRONLY | O_CLOEXEC | O_BINARY | O_CREAT |
    O_NOFOLLOW,S_MODE);
  if (destination_file == -1)
    return(MagickFalse);
  if (AuthorizeDelegateCopy(source,destination,destination_file,exception) ==
      MagickFalse)
    return(MagickFalse);
  source_file=open_utf8(source,O_RDONLY | O_CLOEXEC | O_BINARY,0);
  if (source_file == -1)
    {
      (void) close_utf8(destination_file);
      return(MagickFalse);
    }
  quantum=GetDelegateCopyQuantum(source_file);
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      (void) close_utf8(source_file);
      (void) close_utf8(destination_file);
      return(MagickFalse);
    }
  copy.source_file=source_file;
  copy.destination_file=destination_file;
  copy.buffer=buffer;
  copy.quantum=quantum;
  CopyDelegateFileData(&copy,&status);
  (void) close_utf8(destination_file);
  (void) close_utf8(source_file);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  return(status);
}

static inline MagickBooleanType IsTwoStepDelegate(
  const DelegateInfo *delegate_info,const char *decode,const char *encode)
{
  return((MagickBooleanType) ((delegate_info->mode != 0) &&
    (((decode != (const char *) NULL) &&
    (delegate_info->encode != (char *) NULL)) ||
    ((encode != (const char *) NULL) &&
    (delegate_info->decode != (char *) NULL)))));
}

static void WaitForDelegateInput(const Image *image)
{
  ssize_t
    count;

  /*
    Wait for input file to 'disappear', or maximum 2 seconds.
  */
  count=20;
  while ((count-- > 0) && (access_utf8(image->filename,F_OK) == 0))
    (void) MagickDelay(100);  /* sleep 0.1 seconds */
}

static void RelinquishTemporaryInput(const Image *image,
  const MagickBooleanType temporary)
{
  if (temporary != MagickFalse)
    (void) RelinquishUniqueFileResource(image->filename);
}

typedef struct _DelegateInvocation
{
  ImageInfo
    *image_info;

  Image
    *image;

  const char
    *decode,
    *encode;

  ExceptionInfo
    *exception;

  const DelegateInfo
    *delegate_info;

  MagickBooleanType
    temporary;
} DelegateInvocation;

static MagickBooleanType WriteDelegateImages(
  const DelegateInvocation *invocation,ImageInfo **clone_info)
{
  ImageInfo
    *const image_info = invocation->image_info;

  Image
    *const image = invocation->image;

  const char
    *const decode = invocation->decode,
    *const encode = invocation->encode;

  ExceptionInfo
    *const exception = invocation->exception;

  const DelegateInfo
    *const delegate_info = invocation->delegate_info;

  const MagickBooleanType
    temporary = invocation->temporary;

  Image
    *p;

  MagickBooleanType
    status;

  for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    (void) FormatLocaleString(p->filename,MagickPathExtent,"%s:%s",
      delegate_info->decode,(*clone_info)->filename);
    status=WriteImage(*clone_info,p,exception);
    if (status == MagickFalse)
      {
        (void) RelinquishUniqueFileResource(image_info->unique);
        if (temporary != MagickFalse)
          (void) RelinquishUniqueFileResource(image->filename);
        *clone_info=DestroyImageInfo(*clone_info);
        (void) ThrowMagickException(exception,GetMagickModule(),
          DelegateError,"DelegateFailed","`%s'",decode ? decode : encode);
        return(MagickFalse);
      }
    if ((*clone_info)->adjoin != MagickFalse)
      break;
  }
  return(MagickTrue);
}

static MagickBooleanType ConvertDelegateInput(
  const DelegateInvocation *request)
{
  ImageInfo
    *const image_info = request->image_info;

  Image
    *const image = request->image;

  const char
    *const decode = request->decode,
    *const encode = request->encode;

  ExceptionInfo
    *const exception = request->exception;

  const DelegateInfo
    *const delegate_info = request->delegate_info;

  const MagickBooleanType
    temporary = request->temporary;

  char
    *magick;

  DelegateInvocation
    invocation;

  ImageInfo
    *clone_info;

  /*
    Delegate requires a particular image format.
  */
  if (AcquireUniqueFilename(image_info->unique) == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,
        "UnableToCreateTemporaryFile",image_info->unique);
      return(MagickFalse);
    }
  magick=InterpretImageProperties(image_info,image,decode != (char *) NULL ?
    delegate_info->encode : delegate_info->decode,exception);
  if (magick == (char *) NULL)
    {
      (void) RelinquishUniqueFileResource(image_info->unique);
      RelinquishTemporaryInput(image,temporary);
      (void) ThrowMagickException(exception,GetMagickModule(),
        DelegateError,"DelegateFailed","`%s'",decode ? decode : encode);
      return(MagickFalse);
    }
  LocaleUpper(magick);
  clone_info=CloneImageInfo(image_info);
  (void) CopyMagickString((char *) clone_info->magick,magick,
    MagickPathExtent);
  if (LocaleCompare(magick,"NULL") != 0)
    (void) CopyMagickString(image->magick,magick,MagickPathExtent);
  magick=DestroyString(magick);
  (void) FormatLocaleString(clone_info->filename,MagickPathExtent,"%s:",
    delegate_info->decode);
  (void) SetImageInfo(clone_info,(unsigned int) GetImageListLength(image),
    exception);
  (void) CopyMagickString(clone_info->filename,image_info->filename,
    MagickPathExtent);
  (void) CopyMagickString(image_info->filename,image->filename,
    MagickPathExtent);
  invocation.image_info=image_info;
  invocation.image=image;
  invocation.decode=decode;
  invocation.encode=encode;
  invocation.exception=exception;
  invocation.delegate_info=delegate_info;
  invocation.temporary=temporary;
  if (WriteDelegateImages(&invocation,&clone_info) == MagickFalse)
    return(MagickFalse);
  (void) RelinquishUniqueFileResource(image_info->unique);
  clone_info=DestroyImageInfo(clone_info);
  return(MagickTrue);
}

static void RestoreDelegateInput(const Image *image,
  const char *input_filename,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (IsPathAccessible(input_filename) != MagickFalse)
    {
      if (CopyDelegateFile(image->filename,input_filename,MagickFalse,exception) == MagickFalse)
        {
          (void) RelinquishUniqueFileResource(input_filename);
          *status=MagickFalse;
        }
    }
}

static MagickBooleanType LinkDelegateInput(Image *image,
  const char *input_filename,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (IsPathAccessible(input_filename) != MagickFalse)
    {
      *status=AcquireUniqueSymbolicLink(input_filename,image->filename);
      if (*status == MagickFalse)
        {
          ThrowFileException(exception,FileOpenError,
            "UnableToCreateTemporaryFile",input_filename);
          return(MagickFalse);
        }
    }
  return(MagickTrue);
}

typedef struct _DelegateExecution
{
  const DelegateInfo
    *delegate_info;

  ImageInfo
    *image_info;

  Image
    *image;

  ExceptionInfo
    *exception;

  const char
    *input_filename,
    *output_filename,
    *text;
} DelegateExecution;

static void ExecuteDelegateCommand(const DelegateExecution *execution,
  MagickBooleanType *status,char **command)
{
  const DelegateInfo
    *const delegate_info = execution->delegate_info;

  ImageInfo
    *const image_info = execution->image_info;

  Image
    *const image = execution->image;

  ExceptionInfo
    *const exception = execution->exception;

  if (*command != (char *) NULL)
    {
      /*
        Execute delegate.
      */
      if (ExternalDelegateCommand(delegate_info->spawn,image_info->verbose,*command,(char *) NULL,exception) != 0)
        *status=MagickFalse;
      if (delegate_info->spawn != MagickFalse)
        WaitForDelegateInput(image);
      *command=DestroyString(*command);
    }
}

static MagickBooleanType CopyDelegateOutput(const ImageInfo *image_info,
  const char *input_filename,const char *output_filename,
  ExceptionInfo *exception)
{
  if ((strcmp(input_filename,output_filename) != 0) &&
      (CopyDelegateFile(image_info->filename,output_filename,MagickTrue,exception) == MagickFalse))
    {
      (void) RelinquishUniqueFileResource(output_filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void RelinquishDelegateFiles(const ImageInfo *image_info,
  const Image *image)
{
  if (image_info->temporary != MagickFalse)
    (void) RelinquishUniqueFileResource(image_info->filename);
  (void) RelinquishUniqueFileResource(image_info->unique);
  (void) RelinquishUniqueFileResource(image_info->filename);
  (void) RelinquishUniqueFileResource(image->filename);
}

static MagickBooleanType RunDelegateStep(const DelegateExecution *execution,
  MagickBooleanType *status)
{
  ImageInfo
    *const image_info = execution->image_info;

  Image
    *const image = execution->image;

  ExceptionInfo
    *const exception = execution->exception;

  char
    *command;

  (void) AcquireUniqueSymbolicLink(execution->output_filename,
    image_info->filename);
  if (AcquireUniqueFilename(image_info->unique) == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,
        "UnableToCreateTemporaryFile",image_info->unique);
      return(MagickFalse);
    }
  if (LinkDelegateInput(image,execution->input_filename,exception,status) ==
      MagickFalse)
    return(MagickFalse);
  *status=MagickTrue;
  command=InterpretDelegateProperties(image_info,image,execution->text,
    exception);
  ExecuteDelegateCommand(execution,status,&command);
  RestoreDelegateInput(image,execution->input_filename,exception,status);
  if (CopyDelegateOutput(image_info,execution->input_filename,
      execution->output_filename,exception) == MagickFalse)
    *status=MagickFalse;
  RelinquishDelegateFiles(image_info,image);
  return(MagickTrue);
}

static MagickBooleanType AcquireDelegateOutputFilename(ImageInfo *image_info,
  const Image *image,const MagickBooleanType temporary,
  ExceptionInfo *exception)
{
  if (*image_info->filename != '\0')
    return(MagickTrue);
  if (AcquireUniqueFilename(image_info->filename) == MagickFalse)
    {
      RelinquishTemporaryInput(image,temporary);
      ThrowFileException(exception,FileOpenError,
        "UnableToCreateTemporaryFile",image_info->filename);
      return(MagickFalse);
    }
  image_info->temporary=MagickTrue;
  return(MagickTrue);
}

static inline MagickBooleanType IsDelegateNameDenied(const char *name)
{
  return((MagickBooleanType) ((name != (const char *) NULL) &&
    (IsRightsAuthorized(DelegatePolicyDomain,ExecutePolicyRights,name) ==
     MagickFalse)));
}

static MagickBooleanType AuthorizeDelegateNames(const char *decode,
  const char *encode,ExceptionInfo *exception)
{
  if (IsDelegateNameDenied(decode) != MagickFalse)
    ThrowPolicyException(decode,MagickFalse);
  if (IsDelegateNameDenied(encode) != MagickFalse)
    ThrowPolicyException(encode,MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType AcquireDelegateInputFilename(Image *image,
  const MagickBooleanType temporary,ExceptionInfo *exception)
{
  if ((temporary != MagickFalse) && (AcquireUniqueFilename(image->filename) ==
      MagickFalse))
    {
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static char **DestroyDelegateCommands(char **commands,ssize_t i)
{
  for ( ; commands[i] != (char *) NULL; i++)
    commands[i]=DestroyString(commands[i]);
  return((char **) RelinquishMagickMemory(commands));
}

typedef struct _DelegateCommandRun
{
  const DelegateInfo
    *delegate_info;

  ImageInfo
    *image_info;

  Image
    *image;

  ExceptionInfo
    *exception;

  char
    **commands;
} DelegateCommandRun;

static void RunDelegateCommands(const DelegateCommandRun *run,
  MagickBooleanType *status,ssize_t *i)
{
  const DelegateInfo
    *const delegate_info = run->delegate_info;

  ImageInfo
    *const image_info = run->image_info;

  Image
    *const image = run->image;

  ExceptionInfo
    *const exception = run->exception;

  char
    **const commands = run->commands;

  char
    input_filename[MagickPathExtent],
    output_filename[MagickPathExtent];

  *status=MagickTrue;
  (void) CopyMagickString(output_filename,image_info->filename,
    MagickPathExtent);
  (void) CopyMagickString(input_filename,image->filename,MagickPathExtent);
  for (*i=0; commands[*i] != (char *) NULL; (*i)++)
  {
    const DelegateExecution
      execution = { delegate_info, image_info, image, exception,
        input_filename, output_filename, commands[*i] };

    if (RunDelegateStep(&execution,status) == MagickFalse)
      break;
    if (*status == MagickFalse)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
          "DelegateFailed","`%s'",commands[*i]);
        break;
      }
    commands[*i]=DestroyString(commands[*i]);
  }
  (void) CopyMagickString(image_info->filename,output_filename,
    MagickPathExtent);
  (void) CopyMagickString(image->filename,input_filename,MagickPathExtent);
}

static const char *DelegateTag(const char *decode,const char *encode)
{
  return(decode ? decode : encode);
}

static void OpenInvokeDelegate(const ImageInfo *image_info,
  const Image *image)
{
  magick_unreferenced(image_info);
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
}

MagickExport MagickBooleanType InvokeDelegate(ImageInfo *image_info,
  Image *image,const char *decode,const char *encode,ExceptionInfo *exception)
{
  char
    **commands;

  const DelegateInfo
    *delegate_info;

  DelegateCommandRun
    run;

  MagickBooleanType
    status,
    temporary;

  ssize_t
    i;

  /*
    Get delegate.
  */
  OpenInvokeDelegate(image_info,image);
  if (AuthorizeDelegateNames(decode,encode,exception) == MagickFalse)
    return(MagickFalse);
  temporary=(MagickBooleanType) (*image->filename == '\0');
  if (AcquireDelegateInputFilename(image,temporary,exception) == MagickFalse)
    return(MagickFalse);
  delegate_info=GetDelegateInfo(decode,encode,exception);
  if (delegate_info == (DelegateInfo *) NULL)
    {
      RelinquishTemporaryInput(image,temporary);
      (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
        "NoTagFound","`%s'",DelegateTag(decode,encode));
      return(MagickFalse);
    }
  if (AcquireDelegateOutputFilename(image_info,image,temporary,exception) ==
      MagickFalse)
    return(MagickFalse);
  if (IsTwoStepDelegate(delegate_info,decode,encode) != MagickFalse)
    {
      const DelegateInvocation
        request = { image_info, image, decode, encode, exception,
          delegate_info, temporary };

      if (ConvertDelegateInput(&request) == MagickFalse)
        return(MagickFalse);
    }
  /*
    Invoke delegate.
  */
  commands=StringToList(delegate_info->commands);
  if (commands == (char **) NULL)
    {
      RelinquishTemporaryInput(image,temporary);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",
        DelegateTag(decode,encode));
      return(MagickFalse);
    }
  run.delegate_info=delegate_info;
  run.image_info=image_info;
  run.image=image;
  run.exception=exception;
  run.commands=commands;
  RunDelegateCommands(&run,&status,&i);
  /*
    Relinquish resources.
  */
  commands=DestroyDelegateCommands(commands,i);
  RelinquishTemporaryInput(image,temporary);
  return(status);
}
