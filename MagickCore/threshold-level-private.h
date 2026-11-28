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

  MagickCore private level threshold methods.
*/
#ifndef MAGICKCORE_THRESHOLD_LEVEL_PRIVATE_H
#define MAGICKCORE_THRESHOLD_LEVEL_PRIVATE_H

#include "MagickCore/cache-view.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline PixelInfo GetThresholdLevels(const Image *image,
  const char *thresholds)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  PixelInfo
    threshold;

  GetPixelInfo(image,&threshold);
  flags=ParseGeometry(thresholds,&geometry_info);
  threshold.red=geometry_info.rho;
  threshold.green=geometry_info.rho;
  threshold.blue=geometry_info.rho;
  threshold.black=geometry_info.rho;
  threshold.alpha=100.0;
  if ((flags & SigmaValue) != 0)
    threshold.green=geometry_info.sigma;
  if ((flags & XiValue) != 0)
    threshold.blue=geometry_info.xi;
  if ((flags & PsiValue) != 0)
    threshold.alpha=geometry_info.psi;
  if (threshold.colorspace == CMYKColorspace)
    {
      if ((flags & PsiValue) != 0)
        threshold.black=geometry_info.psi;
      if ((flags & ChiValue) != 0)
        threshold.alpha=geometry_info.chi;
    }
  if ((flags & PercentValue) != 0)
    {
      threshold.red*=((MagickRealType) QuantumRange/100.0);
      threshold.green*=((MagickRealType) QuantumRange/100.0);
      threshold.blue*=((MagickRealType) QuantumRange/100.0);
      threshold.black*=((MagickRealType) QuantumRange/100.0);
      threshold.alpha*=((MagickRealType) QuantumRange/100.0);
    }
  return(threshold);
}

typedef void
  (*ThresholdChannelsMethod)(const Image *,const PixelInfo *,double,
    Quantum *magick_restrict);

static inline void ThresholdIntensity(const Image *image,
  const ThresholdChannelsMethod method,const PixelInfo *threshold,
  Quantum *magick_restrict q)
{
  double
    pixel;

  pixel=GetPixelIntensity(image,q);
  method(image,threshold,pixel,q);
}

typedef void
  (*ThresholdPixelMethod)(const Image *,const void *,Quantum *magick_restrict);

static inline void ThresholdPixels(const Image *image,
  const ThresholdPixelMethod method,const void *arguments,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    method(image,arguments,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef void
  (*ThresholdRowMethod)(const Image *,const void *,Quantum *magick_restrict);

typedef struct _ThresholdRows
{
  const char
    *tag;

  ThresholdRowMethod
    method;

  const void
    *arguments;
} ThresholdRows;

static inline MagickBooleanType ThresholdImageRows(Image *image,
  const ThresholdRows *rows,ExceptionInfo *exception)
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
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    const ThresholdLine
      line = { image,image_view,exception };

    if (GetThresholdRow(&line,y,&q,&status) == MagickFalse)
      continue;
    rows->method(image,rows->arguments,q);
    SyncThresholdRow(&line,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetThresholdProgress(image,rows->tag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
