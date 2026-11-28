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

  MagickCore private image level methods.
*/
#ifndef MAGICKCORE_ENHANCE_LEVEL_PRIVATE_H
#define MAGICKCORE_ENHANCE_LEVEL_PRIVATE_H

#include "MagickCore/cache-view.h"
#include "MagickCore/image.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/enhance-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef void
  (*ToneRowMethod)(const Image *,const void *,Quantum *);

typedef struct _ToneRows
{
  const char
    *tag;

  ToneRowMethod
    method;

  const void
    *arguments;
} ToneRows;

static inline MagickBooleanType ToneImageRows(Image *image,const ToneRows *rows,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    const ToneLine
      line = { image,image_view,exception };

    if (GetToneRow(&line,y,&q,&status) == MagickFalse)
      continue;
    rows->method(image,rows->arguments,q);
    SyncToneRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetToneProgress(image,rows->tag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

static inline double gamma_pow(const double value,const double gamma)
{
  return(value < 0.0 ? value : pow(value,gamma));
}

typedef struct _LevelPoints
{
  double
    black_point,
    white_point,
    gamma;
} LevelPoints;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
