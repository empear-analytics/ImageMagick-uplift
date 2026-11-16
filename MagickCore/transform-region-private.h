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

  MagickCore private image transform methods.
*/
#ifndef MAGICKCORE_TRANSFORM_REGION_PRIVATE_H
#define MAGICKCORE_TRANSFORM_REGION_PRIVATE_H

#include "MagickCore/cache-view.h"
#include "MagickCore/image.h"
#include "MagickCore/locale_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void SetTransformProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static inline MagickBooleanType IsPixelRowMissing(const Quantum *p,
  const Quantum *q)
{
  return(((p == (const Quantum *) NULL) || (q == (Quantum *) NULL)) ?
    MagickTrue : MagickFalse);
}

static inline void CopyPixelChannelsTo(const Image *image,const Image *destination,
  const Quantum *p,Quantum *q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    PixelTrait destination_traits=GetPixelChannelTraits(destination,channel);
    if ((traits == UndefinedPixelTrait) ||
        (destination_traits == UndefinedPixelTrait))
      continue;
    SetPixelChannel(destination,channel,p[i],q);
  }
}

static inline Image *FinishChopImage(const Image *image,Image *chop_image,
  CacheView *image_view,const MagickBooleanType status)
{
  image_view=DestroyCacheView(image_view);
  chop_image->type=image->type;
  if (status == MagickFalse)
    chop_image=DestroyImage(chop_image);
  return(chop_image);
}

typedef struct _TransformLine
{
  const Image
    *image;

  Image
    *transform_image;

  CacheView
    *image_view,
    *transform_view;

  ExceptionInfo
    *exception;
} TransformLine;

static inline void CopyPixelRow(const Image *image,const Image *destination,
  const Quantum *p,Quantum *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) destination->columns; x++)
  {
    CopyPixelChannelsTo(image,destination,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(destination);
  }
}

static inline void AppendTransformProperty(const Image *image,
  Image *transform_image,const char *format,ExceptionInfo *exception)
{
  char
    transform[MagickPathExtent];

  (void) FormatLocaleString(transform,MagickPathExtent,format,
    (double) image->columns,(double) image->rows);
  AppendImageProfileProperty(transform_image,"hdrgm","hdrgm:Transform",
    transform,exception);
}

typedef struct _TransformFinish
{
  const Image
    *image;

  MagickBooleanType
    status;

  ExceptionInfo
    *exception;

  const char
    *format;
} TransformFinish;

static inline Image *FinishTransformImage(const TransformFinish *finish,
  Image *transform_image)
{
  const Image
    *const image = finish->image;

  const MagickBooleanType
    status = finish->status;

  ExceptionInfo
    *const exception = finish->exception;

  const char
    *const format = finish->format;

  if (status != MagickFalse)
    AppendTransformProperty(image,transform_image,format,exception);
  if (status == MagickFalse)
    transform_image=DestroyImage(transform_image);
  return(transform_image);
}

static inline void SyncTransformView(CacheView *view,ExceptionInfo *exception,
  MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
