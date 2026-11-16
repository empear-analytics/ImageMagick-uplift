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

  MagickCore private blur and sharpen methods.
*/
#ifndef MAGICKCORE_EFFECT_BLUR_ADAPTIVE_FILTERS_PRIVATE_H
#define MAGICKCORE_EFFECT_BLUR_ADAPTIVE_FILTERS_PRIVATE_H

#include "MagickCore/image.h"
#include "MagickCore/monitor.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define MagickSigma  (fabs(sigma) < MagickEpsilon ? MagickEpsilon : sigma)

static inline void SetBlurProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static inline Image *ReleaseFailedImage(Image *image,const MagickBooleanType status)
{
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
