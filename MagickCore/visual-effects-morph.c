/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  V   V  IIIII  SSSSS  U   U   AAA   L                       %
%                  V   V    I    SS     U   U  A   A  L                       %
%                  V   V    I     SSS   U   U  AAAAA  L                       %
%                   V V     I       SS  U   U  A   A  L                       %
%                    V    IIIII  SSSSS   UUU   A   A  LLLLL                   %
%                                                                             %
%               EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT  SSSSS               %
%               E      F      F      E      C        T    SS                  %
%               EEE    FFF    FFF    EEE    C        T     SSS                %
%               E      F      F      E      C        T       SS               %
%               EEEEE  F      F      EEEEE   CCCC    T    SSSSS               %
%                                                                             %
%                      M   M   OOO   RRRR   PPPP   H   H                      %
%                      MM MM  O   O  R   R  P   P  H   H                      %
%                      M M M  O   O  RRRR   PPPP   HHHHH                      %
%                      M   M  O   O  R R    P      H   H                      %
%                      M   M   OOO   R  R   P      H   H                      %
%                                                                             %
%                                                                             %
%                           MagickCore Morph Method                           %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
%                                                                             %
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
#include "MagickCore/accelerate-private.h"
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/visual-effects.h"
#include "MagickCore/visual-effects-pixel-methods-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o r p h I m a g e s                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The MorphImages() method requires a minimum of two images.  The first
%  image is transformed into the second by a number of intervening images
%  as specified by frames.
%
%  The format of the MorphImage method is:
%
%      Image *MorphImages(const Image *image,const size_t number_frames,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o number_frames:  Define the number of in-between image to generate.
%      The more in-between frames, the smoother the morph.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _MorphPixel
{
  const Image
    *morph_image,
    *morph_images;

  double
    alpha,
    beta;
} MorphPixel;

static void MorphPixelChannels(const MorphPixel *morph_pixel,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const morph_image = morph_pixel->morph_image,
    *const morph_images = morph_pixel->morph_images;

  const double
    alpha = morph_pixel->alpha,
    beta = morph_pixel->beta;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(morph_image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(morph_image,i);
    PixelTrait traits = GetPixelChannelTraits(morph_image,channel);
    PixelTrait morph_traits=GetPixelChannelTraits(morph_images,channel);
    if (IsPixelChannelPairUndefined(traits,morph_traits) != MagickFalse)
      continue;
    if ((morph_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(morph_image,channel,p[i],q);
        continue;
      }
    SetPixelChannel(morph_image,channel,ClampToQuantum(alpha*(double)
      GetPixelChannel(morph_images,channel,q)+beta*(double) p[i]),q);
  }
}

#define MorphImageTag  "Morph/Image"

static MagickBooleanType ReportMorphProgress(const Image *image,
  const MagickOffsetType scene,MagickBooleanType status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,MorphImageTag,scene,
        GetImageListLength(image));
      if (proceed == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

static void ReportMorphFrameProgress(const Image *image,const ssize_t n,
  const size_t number_frames,MagickBooleanType *status)
{
  if (image->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(image,MorphImageTag,(MagickOffsetType) n,
        number_frames);
      if (proceed == MagickFalse)
        (*status)=MagickFalse;
    }
}

typedef struct _MorphFrameRowInfo
{
  CacheView
    *image_view;

  Image
    *morph_image;

  ExceptionInfo
    *exception;

  CacheView
    *morph_view;

  Image
    *morph_images;

  double
    alpha;

  double
    beta;
} MorphFrameRowInfo;

static void MorphFrameRow(MagickBooleanType *status,
  const MorphFrameRowInfo *info,const ssize_t y)
{
  CacheView
    *const image_view = info->image_view;

  Image
    *const morph_image = info->morph_image;

  ExceptionInfo
    *const exception = info->exception;

  CacheView
    *const morph_view = info->morph_view;

  Image
    *const morph_images = info->morph_images;

  const double
    alpha = info->alpha;

  const double
    beta = info->beta;

  const MorphPixel
    morph_pixel = { morph_image, morph_images, alpha, beta };

  MagickBooleanType
    sync;

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(image_view,0,y,morph_image->columns,1,
    exception);
  q=GetCacheViewAuthenticPixels(morph_view,0,y,morph_images->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) morph_images->columns; x++)
  {
    MorphPixelChannels(&morph_pixel,p,q);
    p+=(ptrdiff_t) GetPixelChannels(morph_image);
    q+=(ptrdiff_t) GetPixelChannels(morph_images);
  }
  sync=SyncCacheViewAuthenticPixels(morph_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _MorphSingleInfo
{
  const Image
    *image;

  size_t
    number_frames;

  ExceptionInfo
    *exception;
} MorphSingleInfo;

static Image *MorphSingleImage(const MorphSingleInfo *single,
  Image *morph_images,MagickBooleanType *status)
{
  const Image
    *const image = single->image;

  const size_t
    number_frames = single->number_frames;

  ExceptionInfo
    *const exception = single->exception;

  Image
    *morph_image;

  ssize_t
    n;

  /*
    Morph single image.
  */
  for (n=1; n < (ssize_t) number_frames; n++)
  {
    morph_image=CloneImage(image,0,0,MagickTrue,exception);
    if (morph_image == (Image *) NULL)
      {
        morph_images=DestroyImageList(morph_images);
        return((Image *) NULL);
      }
    AppendImageToList(&morph_images,morph_image);
    ReportMorphFrameProgress(image,n,number_frames,status);
  }
  return(GetFirstImageInList(morph_images));
}

typedef struct _MorphFrameInfo
{
  Image
    *morph_images;

  double
    alpha,
    beta;

  ExceptionInfo
    *exception;
} MorphFrameInfo;

static MagickBooleanType MorphFrame(const MorphFrameInfo *frame,
  Image *morph_image,MagickBooleanType status)
{
  Image
    *const morph_images = frame->morph_images;

  const double
    alpha = frame->alpha,
    beta = frame->beta;

  ExceptionInfo
    *const exception = frame->exception;

  CacheView
    *image_view,
    *morph_view;

  ssize_t
    y;

  image_view=AcquireVirtualCacheView(morph_image,exception);
  morph_view=AcquireAuthenticCacheView(morph_images,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(morph_image,morph_image,morph_image->rows,2)
#endif
  for (y=0; y < (ssize_t) morph_images->rows; y++)
  {
    const MorphFrameRowInfo
      info = { image_view, morph_image, exception, morph_view,
        morph_images, alpha, beta };

    MorphFrameRow(&status,&info,y);
  }
  morph_view=DestroyCacheView(morph_view);
  image_view=DestroyCacheView(image_view);
  morph_image=DestroyImage(morph_image);
  return(status);
}

typedef struct _MorphSequenceTargets
{
  Image
    **morph_images;

  MagickBooleanType
    *status;

  ssize_t
    *n;
} MorphSequenceTargets;

static MagickBooleanType MorphSequenceFrames(const Image *next,
  const size_t number_frames,ExceptionInfo *exception,
  const MorphSequenceTargets *targets)
{
  Image
    **const morph_images = targets->morph_images;

  MagickBooleanType
    *const status = targets->status;

  ssize_t
    *const n = targets->n;

  double
    alpha,
    beta;

  Image
    *morph_image;

  for ((*n)=0; (*n) < (ssize_t) number_frames; (*n)++)
  {
    MorphFrameInfo
      frame;

    beta=(double) ((*n)+1.0)/(double) (number_frames+1.0);
    alpha=1.0-beta;
    morph_image=ResizeImage(next,(size_t) (alpha*next->columns+beta*
      GetNextImageInList(next)->columns+0.5),(size_t) (alpha*next->rows+beta*
      GetNextImageInList(next)->rows+0.5),next->filter,exception);
    if (morph_image == (Image *) NULL)
      {
        (*morph_images)=DestroyImageList((*morph_images));
        return(MagickFalse);
      }
    (*status)=SetImageStorageClass(morph_image,DirectClass,exception);
    if ((*status) == MagickFalse)
      {
        morph_image=DestroyImage(morph_image);
        return(MagickFalse);
      }
    AppendImageToList(morph_images,morph_image);
    (*morph_images)=GetLastImageInList((*morph_images));
    morph_image=ResizeImage(GetNextImageInList(next),(*morph_images)->columns,
      (*morph_images)->rows,GetNextImageInList(next)->filter,exception);
    if (morph_image == (Image *) NULL)
      {
        (*morph_images)=DestroyImageList((*morph_images));
        return(MagickFalse);
      }
    frame.morph_images=(*morph_images);
    frame.alpha=alpha;
    frame.beta=beta;
    frame.exception=exception;
    (*status)=MorphFrame(&frame,morph_image,(*status));
  }
  return(MagickTrue);
}

static MagickBooleanType MorphImageSequence(const Image *image,
  const size_t number_frames,Image **morph_images,ExceptionInfo *exception)
{
  Image
    *morph_image;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  const Image
    *next;

  ssize_t
    n;

  status=MagickTrue;
  scene=0;
  next=image;
  for ( ; GetNextImageInList(next) != (Image *) NULL; next=GetNextImageInList(next))
  {
    MorphSequenceTargets
      targets;

    targets.morph_images=morph_images;
    targets.status=(&status);
    targets.n=(&n);
    if (MorphSequenceFrames(next,number_frames,exception,&targets) ==
        MagickFalse)
      return(MagickFalse);
    if (n < (ssize_t) number_frames)
      break;
    /*
      Clone last frame in sequence.
    */
    morph_image=CloneImage(GetNextImageInList(next),0,0,MagickTrue,exception);
    if (morph_image == (Image *) NULL)
      {
        (*morph_images)=DestroyImageList((*morph_images));
        return(MagickFalse);
      }
    AppendImageToList(morph_images,morph_image);
    (*morph_images)=GetLastImageInList((*morph_images));
    status=ReportMorphProgress(image,scene,status);
    scene++;
  }
  if (GetNextImageInList(next) != (Image *) NULL)
    {
      (*morph_images)=DestroyImageList((*morph_images));
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickExport Image *MorphImages(const Image *image,const size_t number_frames,
  ExceptionInfo *exception)
{
  Image
    *morph_images;

  MagickBooleanType
    status;

  /*
    Clone first frame in sequence.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  morph_images=CloneImage(image,0,0,MagickTrue,exception);
  if (morph_images == (Image *) NULL)
    return((Image *) NULL);
  if (GetNextImageInList(image) == (Image *) NULL)
    {
      const MorphSingleInfo
        single = { image, number_frames, exception };

      return(MorphSingleImage(&single,morph_images,&status));
    }
  /*
    Morph image sequence.
  */
  if (MorphImageSequence(image,number_frames,&morph_images,exception) ==
      MagickFalse)
    return((Image *) NULL);
  return(GetFirstImageInList(morph_images));
}
