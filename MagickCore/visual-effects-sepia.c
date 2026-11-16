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
%                      SSSSS  EEEEE  PPPP   IIIII   AAA                       %
%                      SS     E      P   P    I    A   A                      %
%                       SSS   EEE    PPPP     I    AAAAA                      %
%                         SS  E      P        I    A   A                      %
%                      SSSSS  EEEEE  P      IIIII  A   A                      %
%                                                                             %
%                                                                             %
%                        MagickCore Sepia Tone Method                         %
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
%     S e p i a T o n e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SepiaToneImage() applies a special effect to the image, similar to the
%  effect achieved in a photo darkroom by sepia toning.  Threshold ranges from
%  0 to QuantumRange and is a measure of the extent of the sepia toning.  A
%  threshold of 80% is a good starting point for a reasonable tone.
%
%  The format of the SepiaToneImage method is:
%
%      Image *SepiaToneImage(const Image *image,const double threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold: the tone threshold.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _SepiaTonePixelInfo
{
  const Image
    *image,
    *sepia_image;

  double
    threshold;
} SepiaTonePixelInfo;

static void SepiaTonePixel(const SepiaTonePixelInfo *pixel_info,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = pixel_info->image,
    *const sepia_image = pixel_info->sepia_image;

  const double
    threshold = pixel_info->threshold;

  double
    intensity,
    tone;

  intensity=GetPixelIntensity(image,p);
  tone=intensity > threshold ? (double) QuantumRange : intensity+
    (double) QuantumRange-threshold;
  SetPixelRed(sepia_image,ClampToQuantum(tone),q);
  tone=intensity > (7.0*threshold/6.0) ? (double) QuantumRange :
    intensity+(double) QuantumRange-7.0*threshold/6.0;
  SetPixelGreen(sepia_image,ClampToQuantum(tone),q);
  tone=intensity < (threshold/6.0) ? 0 : intensity-threshold/6.0;
  SetPixelBlue(sepia_image,ClampToQuantum(tone),q);
  tone=threshold/7.0;
  if ((double) GetPixelGreen(image,q) < tone)
    SetPixelGreen(sepia_image,ClampToQuantum(tone),q);
  if ((double) GetPixelBlue(image,q) < tone)
    SetPixelBlue(sepia_image,ClampToQuantum(tone),q);
  SetPixelAlpha(sepia_image,GetPixelAlpha(image,p),q);
}

typedef struct _SepiaToneRow
{
  const Image
    *image;

  Image
    *sepia_image;

  double
    threshold;
} SepiaToneRow;

static void SepiaToneImageRow(const SepiaToneRow *sepia_row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = sepia_row->image;

  Image
    *const sepia_image = sepia_row->sepia_image;

  const double
    threshold = sepia_row->threshold;

  const SepiaTonePixelInfo
    pixel_info = { image, sepia_image, threshold };

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    SepiaTonePixel(&pixel_info,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(sepia_image);
  }
}

static MagickBooleanType GetSepiaRows(const EffectRows *rows,
  const Quantum *magick_restrict *p,Quantum *magick_restrict *q,
  MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*p)=GetCacheViewVirtualPixels(rows->image_view,0,rows->y,
    rows->image->columns,1,rows->exception);
  (*q)=GetCacheViewAuthenticPixels(rows->effect_view,0,rows->y,
    rows->effect_image->columns,1,rows->exception);
  if (((*p) == (const Quantum *) NULL) || ((*q) == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickExport Image *SepiaToneImage(const Image *image,const double threshold,
  ExceptionInfo *exception)
{
#define SepiaToneImageTag  "SepiaTone/Image"

  CacheView
    *image_view,
    *sepia_view;

  Image
    *sepia_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  /*
    Initialize sepia-toned image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&sepia_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Tone each row of the image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  sepia_view=AcquireAuthenticCacheView(sepia_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,sepia_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const EffectRows
      rows = { image,sepia_image,image_view,sepia_view,exception,y };

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    SepiaToneRow
      sepia_row;

    if (GetSepiaRows(&rows,&p,&q,&status) == MagickFalse)
      continue;
    sepia_row.image=image;
    sepia_row.sepia_image=sepia_image;
    sepia_row.threshold=threshold;
    SepiaToneImageRow(&sepia_row,p,q);
    SyncEffectRow(sepia_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,SepiaToneImageTag,progress,&status);
      }
  }
  sepia_view=DestroyCacheView(sepia_view);
  image_view=DestroyCacheView(image_view);
  (void) NormalizeImage(sepia_image,exception);
  (void) ContrastImage(sepia_image,MagickTrue,exception);
  return(ReleaseFailedImage(sepia_image,status));
}
