/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                         L      IIIII  SSSSS  TTTTT                          %
%                         L        I    SS       T                            %
%                         L        I     SSS     T                            %
%                         L        I       SS    T                            %
%                         LLLLL  IIIII  SSSSS    T                            %
%                                                                             %
%                  SSSSS   CCCC  EEEEE  N   N  EEEEE  SSSSS                   %
%                  SS     C      E      NN  N  E      SS                      %
%                   SSS   C      EEE    N N N  EEE     SSS                    %
%                     SS  C      E      N  NN  E         SS                   %
%                  SSSSS   CCCC  EEEEE  N   N  EEEEE  SSSSS                   %
%                                                                             %
%                                                                             %
%                    Clone and Delete Images by Scene List                    %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                               December 2002                                 %
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
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"

static ssize_t GetFrameStep(const char *artifact)
{
  ssize_t
    step;

  step=1;
  if (artifact != (const char *) NULL)
    {
      step=(ssize_t) StringToLong(artifact);
      if (step == 0)
        step=1;
    }
  return(step);
}

static ssize_t ClampSceneIndex(ssize_t first,const size_t length)
{
  if (first < 0)
    first+=(ssize_t) length;
  else
    if (first > (ssize_t) length)
      first=(ssize_t) length;
  return(first);
}

static MagickBooleanType CloneSceneToList(const Image *images,
  const ssize_t first,Image **clone_images,ExceptionInfo *exception)
{
  const Image
    *next;

  Image
    *image;

  MagickBooleanType
    match;

  ssize_t
    i;

  match=MagickFalse;
  i=0;
  for (next=images; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    if (i == (ssize_t) first)
      {
        image=CloneImage(next,0,0,MagickTrue,exception);
        if (image == (Image *) NULL)
          break;
        AppendImageToList(clone_images,image);
        match=MagickTrue;
      }
    i++;
  }
  return(match);
}

static ssize_t ParseCloneSceneEnd(char **p,const ssize_t first,
  const size_t length)
{
  ssize_t
    last;

  last=first;
  while (isspace((int) ((unsigned char) **p)) != 0)
    (*p)++;
  if (**p == '-')
    {
      last=(ssize_t) strtol((*p)+1,p,10);
      last=ClampSceneIndex(last,length);
    }
  return(last);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImages() clones one or more images from an image sequence, using a
%  comma separated list of image numbers or ranges.
%
%  The numbers start at 0 for the first image in the list, while negative
%  numbers refer to images starting counting from the end of the range. Images
%  may be referred to multiple times to clone them multiple times. Images
%  referred beyond the available number of images in list are ignored.
%
%  Images referenced may be reversed, and results in a clone of those images
%  also being made with a reversed order.
%
%  The format of the CloneImages method is:
%
%      Image *CloneImages(const Image *images,const char *scenes,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image sequence.
%
%    o scenes: This character string specifies which scenes to clone
%      (e.g. 1,3-5,7-3,2).
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *CloneImages(const Image *images,const char *scenes,
  ExceptionInfo *exception)
{
  char
    *p;

  const char
    *artifact;

  Image
    *clone_images;

  size_t
    length;

  ssize_t
    first,
    last,
    offset,
    step;

  assert(images != (const Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(scenes != (char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  clone_images=NewImageList();
  images=GetFirstImageInList(images);
  artifact=GetImageArtifact(images,"frames:step");
  length=GetImageListLength(images);
  for (p=(char *) scenes; *p != '\0';)
  {
    MagickBooleanType
      match;

    while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == ','))
      p++;
    first=(ssize_t) strtol(p,&p,10);
    offset=first;
    first=ClampSceneIndex(first,length);
    first%=(ssize_t) (length << 1);
    last=ParseCloneSceneEnd(&p,first,length);
    last%=(ssize_t) (length << 1);
    match=MagickFalse;
    step=GetFrameStep(artifact);
    step=(ssize_t) (first > last ? -step : step);
    for ( ; step > 0 ? (last-first) >= 0 : (last-first) <= 0; first+=step)
    {
      if (CloneSceneToList(images,first,&clone_images,exception) !=
          MagickFalse)
        match=MagickTrue;
      if (match == MagickFalse)
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidImageIndex","%g `%s'",(double) offset,images->filename);
    }
  }
  return(GetFirstImageInList(clone_images));
}

static void DeleteMarkedImages(Image **images,
  const MagickBooleanType *delete_list,const size_t length)
{
  Image
    *image;

  ssize_t
    i;

  image=(*images);
  for (i=0; i < (ssize_t) length; i++)
  {
    *images=image;
    image=GetNextImageInList(image);
    if (delete_list[i] != MagickFalse)
      DeleteImageFromList(images);
  }
}

static void MarkImagesForDeletion(MagickBooleanType *delete_list,
  const long first,const long last,const size_t length)
{
  ssize_t
    i;

  for (i=(ssize_t) first; i <= (ssize_t) last; i++)
    if ((i >= 0) && (i < (ssize_t) length))
      delete_list[i]=MagickTrue;
}

static MagickBooleanType ParseSceneRangeEnd(char **p,long *last,
  const size_t length)
{
  char
    *q;

  if (**p != '-')
    return(MagickTrue);
  *last=strtol((*p)+1,&q,10);
  if (((*p)+1) == q)
    return(MagickFalse);
  *p=q;
  if (*last < 0)
    *last+=(long) length;
  return(MagickTrue);
}

static void MarkScenesForDeletion(MagickBooleanType *delete_list,
  const char *scenes,const size_t length)
{
  char
    *p;

  long
    first,
    last;

  for (p=(char *) scenes; *p != '\0'; )
  {
    char
      *q;

    while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == ','))
      p++;
    first=strtol(p,&q,10);
    if (p == q)
      break;
    p=q;
    if (first < 0)
      first+=(long) length;
    last=first;
    while (isspace((int) ((unsigned char) *p)) != 0)
      p++;
    if (ParseSceneRangeEnd(&p,&last,length) == MagickFalse)
      break;
    if (first > last)
      continue;
    MarkImagesForDeletion(delete_list,first,last,length);
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e I m a g e s                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteImages() deletes one or more images from an image sequence, using a
%  comma separated list of image numbers or ranges.
%
%  The numbers start at 0 for the first image, while negative numbers refer to
%  images starting counting from the end of the range. Images may be referred to
%  multiple times without problems. Image referred beyond the available number
%  of images in list are ignored.
%
%  If the referenced images are in the reverse order, that range will be
%  completely ignored, unlike CloneImages().
%
%  The format of the DeleteImages method is:
%
%      void DeleteImages(Image **images,const char *scenes,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: the image sequence.
%
%    o scenes: This character string specifies which scenes to delete
%      (e.g. 1,3-5,-2-6,2).
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport void DeleteImages(Image **images,const char *scenes,
  ExceptionInfo *exception)
{
  MagickBooleanType
    *delete_list;

  ssize_t
    i;

  size_t
    length;

  assert(images != (Image **) NULL);
  assert((*images)->signature == MagickCoreSignature);
  assert(scenes != (char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      (*images)->filename);
  *images=GetFirstImageInList(*images);
  length=GetImageListLength(*images);
  delete_list=(MagickBooleanType *) AcquireQuantumMemory(length,
    sizeof(*delete_list));
  if (delete_list == (MagickBooleanType *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",(*images)->filename);
      return;
    }
  for (i=0; i < (ssize_t) length; i++)
    delete_list[i]=MagickFalse;
  /*
    Note which images will be deleted, avoid duplicates.
  */
  MarkScenesForDeletion(delete_list,scenes,length);
  /*
    Delete images marked for deletion, once only.
  */
  DeleteMarkedImages(images,delete_list,length);
  (void) RelinquishMagickMemory(delete_list);
  *images=GetFirstImageInList(*images);
}
