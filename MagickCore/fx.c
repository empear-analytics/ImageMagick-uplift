/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                                 FFFFF  X   X                                %
%                                 F       X X                                 %
%                                 FFF      X                                  %
%                                 F       X X                                 %
%                                 F      X   X                                %
%                                                                             %
%                                                                             %
%                   MagickCore Image Special Effects Methods                  %
%                                                                             %
%                               Software Design                               %
%                             snibgo (Alan Gibson)                            %
%                                 January 2022                                %
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
#include "MagickCore/fx.h"
#include "MagickCore/fx-private.h"
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
#include "MagickCore/policy.h"
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
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/fx-execute-private.h"
#include "MagickCore/fx-translate-private.h"


FxInfo *AcquireFxInfo (const Image * images, const char * expression, ExceptionInfo *exception)
{
  return AcquireFxInfoPrivate (images, expression, MagickFalse, exception);
}

FxInfo *DestroyFxInfo (FxInfo * pfx)
{
  assert (pfx != NULL);
  assert (pfx->image != NULL);
  assert (pfx->Images != NULL);
  assert (pfx->Imgs != NULL);
  assert (pfx->fxrts != NULL);

  DestroyFxRuntimes (pfx);
  pfx->fxrts = (fxRtT *) RelinquishMagickMemory (pfx->fxrts);

  DestroyRPN (pfx);

  pfx->expression = DestroyString (pfx->expression);
  pfx->pex = NULL;

  (void) DeInitFx (pfx);

  pfx = (FxInfo*) RelinquishMagickMemory(pfx);

  return NULL;
}

static void SetFxProgress (const Image *image, const char *tag,
  const MagickOffsetType progress, MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed = SetImageProgress (image, tag, progress, image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

/* Following is substitute for FxImage().
*/
static Image *FinishFxImage (FxInfo * pfx, MagickBooleanType status,
  Image *fx_image)
{
  /* Before destroying the user symbol values, dump them to stderr.
  */
  if (pfx->DebugOpt && pfx->usedUserSymbols)
    DumpUserSymbolValues (pfx);

  if ((status == MagickFalse) || (pfx->exception->severity >= ErrorException))
    fx_image=DestroyImage(fx_image);

  pfx=DestroyFxInfo(pfx);

  return(fx_image);
}

#define FxImageTag  "FxNew/Image"

typedef struct _FxImageInputs
{
  const Image
    *image;

  Image
    *fx_image;

  FxInfo
    *pfx;
} FxImageInputs;

static Image *ApplyFxImageRows (const FxImageInputs *fx_in)
{
  const Image
    *const image = fx_in->image;

  Image
    *const fx_image = fx_in->fx_image;

  FxInfo
    *const pfx = fx_in->pfx;

  CacheView
    *fx_view,
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  image_view = AcquireVirtualCacheView (image, pfx->exception);
  fx_view = AcquireAuthenticCacheView (fx_image, pfx->exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic) shared(progress,status) \
    magick_number_threads(image,fx_image,fx_image->rows, \
      pfx->ContainsDebug ? 0 : 1)
#endif
  for (y=0; y < (ssize_t) fx_image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    const FxRowInputs
      row = { image, fx_image, image_view, fx_view, pfx, id };

    if (ApplyFxToRow (&row, y, &status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetFxProgress (image, FxImageTag, progress, &status);
      }
  }

  fx_view = DestroyCacheView (fx_view);
  image_view = DestroyCacheView (image_view);

  return(FinishFxImage (pfx, status, fx_image));
}

MagickExport Image *FxImage(const Image *image,const char *expression,
  ExceptionInfo *exception)
{
  Image
    *fx_image;

  FxInfo
    *pfx;

  FxImageInputs
    fx_in;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (expression == (const char *) NULL)
    return(CloneImage(image,0,0,MagickTrue,exception));
  fx_image=CloneImage(image,0,0,MagickTrue,exception);
  if (!fx_image) return NULL;
  if (SetImageStorageClass(fx_image,DirectClass,exception) == MagickFalse) {
    fx_image=DestroyImage(fx_image);
    return NULL;
  }

  pfx = AcquireFxInfoPrivate (image, expression, MagickTrue, exception);

  if (!pfx) {
    fx_image=DestroyImage(fx_image);
    return NULL;
  }

  assert (pfx->image != NULL);
  assert (pfx->Images != NULL);
  assert (pfx->Imgs != NULL);
  assert (pfx->fxrts != NULL);

  fx_in.image=image;
  fx_in.fx_image=fx_image;
  fx_in.pfx=pfx;
  return(ApplyFxImageRows (&fx_in));
}
