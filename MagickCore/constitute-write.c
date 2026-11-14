/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     CCCC   OOO   N   N  SSSSS  TTTTT  IIIII  TTTTT  U   U  TTTTT  EEEEE     %
%    C      O   O  NN  N  SS       T      I      T    U   U    T    E         %
%    C      O   O  N N N   SSS     T      I      T    U   U    T    EEE       %
%    C      O   O  N  NN     SS    T      I      T    U   U    T    E         %
%     CCCC   OOO   N   N  SSSSS    T    IIIII    T     UUU     T    EEEEE     %
%                                                                             %
%                      W   W  RRRR   IIIII  TTTTT  EEEEE                      %
%                      W   W  R   R    I      T    E                          %
%                      W W W  RRRR     I      T    EEE                        %
%                      WW WW  R R      I      T    E                          %
%                      W   W  R  R   IIIII    T    EEEEE                      %
%                                                                             %
%                                                                             %
%                     MagickCore Image List Write Methods                     %
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
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/client.h"
#include "MagickCore/coder-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/constitute-private.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/identify.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/policy.h"
#include "MagickCore/profile.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"

static void SyncImageSceneNumbers(Image *images)
{
  Image
    *p;

  p=images;
  for ( ; GetNextImageInList(p) != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (p->scene >= GetNextImageInList(p)->scene)
      {
        ssize_t
          i;

        /*
          Generate consistent scene numbers.
        */
        i=(ssize_t) images->scene;
        for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
          p->scene=(size_t) i++;
        break;
      }
  }
}

static void PrepareWriteImagesInfo(ImageInfo *write_info,Image *images,
  const char *filename)
{
  ExceptionInfo
    *sans_exception;

  Image
    *p;

  if (filename != (const char *) NULL)
    for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
      (void) CopyMagickString(p->filename,filename,MagickPathExtent);
  (void) CopyMagickString(write_info->filename,images->filename,
    MagickPathExtent);
  sans_exception=AcquireExceptionInfo();
  (void) SetImageInfo(write_info,(unsigned int) GetImageListLength(images),
    sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  if (*write_info->magick == '\0')
    (void) CopyMagickString(write_info->magick,images->magick,MagickPathExtent);
  SyncImageSceneNumbers(images);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e I m a g e s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteImages() writes an image sequence into one or more files.  While
%  WriteImage() can write an image sequence, it is limited to writing
%  the sequence into a single file using a format which supports multiple
%  frames.   WriteImages(), however, does not have this limitation, instead it
%  generates multiple output files if necessary (or when requested).  When
%  ImageInfo's adjoin flag is set to MagickFalse, the file name is expected
%  to include a printf-style formatting string for the frame number (e.g.
%  "image%02d.png").
%
%  The format of the WriteImages method is:
%
%      MagickBooleanType WriteImages(const ImageInfo *image_info,Image *images,
%        const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o images: the image list.
%
%    o filename: the image filename.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SyncWriteBlobStream(ImageInfo *write_info,Image *p,
  Image *images)
{
  if (write_info->blob != (void *) NULL)
    SyncBlobStream(write_info,p,images);
}

static void RestoreWriteProgressMonitor(Image *p,
  const MagickSizeType number_images,MagickProgressMonitor progress_monitor)
{
  if (number_images != 1)
    (void) SetImageProgressMonitor(p,progress_monitor,p->client_data);
}

MagickExport MagickBooleanType WriteImages(const ImageInfo *image_info,
  Image *images,const char *filename,ExceptionInfo *exception)
{
#define WriteImageTag  "Write/Image"

  ImageInfo
    *write_info;

  MagickBooleanType
    proceed;

  MagickOffsetType
    progress;

  MagickProgressMonitor
    progress_monitor;

  MagickSizeType
    number_images;

  MagickStatusType
    status;

  Image
    *p;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  write_info=CloneImageInfo(image_info);
  *write_info->magick='\0';
  images=GetFirstImageInList(images);
  if (images == (Image *) NULL)
    return(MagickFalse);
  PrepareWriteImagesInfo(write_info,images,filename);
  /*
    Write images.
  */
  status=MagickTrue;
  progress_monitor=(MagickProgressMonitor) NULL;
  progress=0;
  number_images=GetImageListLength(images);
  for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (number_images != 1)
      progress_monitor=SetImageProgressMonitor(p,(MagickProgressMonitor) NULL,
        p->client_data);
    status&=(MagickStatusType) WriteImage(write_info,p,exception);
    SyncWriteBlobStream(write_info,p,images);
    RestoreWriteProgressMonitor(p,number_images,progress_monitor);
    if (write_info->adjoin != MagickFalse)
      break;
    if (number_images != 1)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(p,WriteImageTag,progress,number_images);
        if (proceed == MagickFalse)
          break;
      }
  }
  write_info=DestroyImageInfo(write_info);
  return((MagickBooleanType) (status != 0));
}
