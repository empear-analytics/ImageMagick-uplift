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

  MagickCore private image visual effects methods.
*/
#ifndef MAGICKCORE_VISUAL_EFFECTS_PIXEL_METHODS_PRIVATE_H
#define MAGICKCORE_VISUAL_EFFECTS_PIXEL_METHODS_PRIVATE_H

#include "MagickCore/cache-view.h"
#include "MagickCore/image.h"
#include "MagickCore/monitor.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsPixelChannelPairUndefined(
  const PixelTrait traits,const PixelTrait other_traits)
{
  return(((traits == UndefinedPixelTrait) ||
    (other_traits == UndefinedPixelTrait)) ? MagickTrue : MagickFalse);
}

typedef struct _EffectRows
{
  const Image
    *image,
    *effect_image;

  CacheView
    *image_view,
    *effect_view;

  ExceptionInfo
    *exception;

  ssize_t
    y;
} EffectRows;

static inline MagickBooleanType GetEffectRows(const EffectRows *rows,
  const Quantum *magick_restrict *p,Quantum *magick_restrict *q,
  MagickBooleanType *status)
{
  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*p)=GetCacheViewVirtualPixels(rows->image_view,0,rows->y,
    rows->image->columns,1,rows->exception);
  (*q)=QueueCacheViewAuthenticPixels(rows->effect_view,0,rows->y,
    rows->effect_image->columns,1,rows->exception);
  if (((*p) == (const Quantum *) NULL) || ((*q) == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static inline void SyncEffectRow(CacheView *view,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

static inline void SetEffectProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static inline MagickBooleanType CloneEffectImage(const Image *image,
  Image **effect_image,ExceptionInfo *exception)
{
  *effect_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*effect_image == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(*effect_image,DirectClass,exception) == MagickFalse)
    {
      *effect_image=DestroyImage(*effect_image);
      return(MagickFalse);
    }
  return(MagickTrue);
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
