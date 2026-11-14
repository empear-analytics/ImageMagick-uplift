/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                         FFFFF  IIIII  L      L                              %
%                         F        I    L      L                              %
%                         FFF      I    L      L                              %
%                         F        I    L      L                              %
%                         F      IIIII  LLLLL  LLLLL                          %
%                                                                             %
%                                                                             %
%                        MagickCore Image Fill Methods                        %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/compress.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/display.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magic.h"
#include "MagickCore/magick.h"
#include "MagickCore/magick-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/module.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/profile.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/random_.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/timer.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/xwindow-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N e w M a g i c k I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewMagickImage() creates a blank image canvas of the specified size and
%  background color.
%
%  The format of the NewMagickImage method is:
%
%      Image *NewMagickImage(const ImageInfo *image_info,const size_t width,
%        const size_t height,const PixelInfo *background,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o width: the image width.
%
%    o height: the image height.
%
%    o background: the image color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void FillPixelRow(const Image *image,const PixelInfo *color,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    SetPixelViaPixelInfo(image,color,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport Image *NewMagickImage(const ImageInfo *image_info,
  const size_t width,const size_t height,const PixelInfo *background,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  Image
    *image;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(background != (const PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  image=AcquireImage(image_info,exception);
  image->columns=width;
  image->rows=height;
  image->colorspace=background->colorspace;
  image->alpha_trait=background->alpha_trait;
  image->fuzz=background->fuzz;
  image->depth=background->depth;
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    FillPixelRow(image,background,q);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}

typedef struct _ResetRow
{
  Image
    *image;

  CacheView
    *image_view;

  ExceptionInfo
    *exception;
} ResetRow;

static void ResetImagePixelsRow(const ResetRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  ExceptionInfo
    *exception = row->exception;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    (void) memset(q,0,GetPixelChannels(image)*sizeof(Quantum));
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e P i x e l s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImagePixels() reset the image pixels, that is, all the pixel components
%  are zeroed.
%
%  The format of the SetImage method is:
%
%      MagickBooleanType ResetImagePixels(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType ResetImagePixels(Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  size_t
    length;

  ssize_t
    y;

  void
    *pixels;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  pixels=AcquirePixelCachePixels(image,&length,exception);
  if (pixels != (void *) NULL)
    {
      /*
        Reset in-core image pixels.
      */
      (void) memset(pixels,0,length);
      return(MagickTrue);
    }
  /*
    Reset image pixels.
  */
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const ResetRow row = { image, image_view, exception };

    ResetImagePixelsRow(&row,y,&status);
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

typedef struct _AlphaRow
{
  Image
    *image;

  CacheView
    *image_view;

  Quantum
    alpha;

  ExceptionInfo
    *exception;
} AlphaRow;

static void SetImageAlphaRow(const AlphaRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  const Quantum
    alpha = row->alpha;

  ExceptionInfo
    *exception = row->exception;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelWriteMask(image,q) > (QuantumRange/2))
      SetPixelAlpha(image,alpha,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S e t I m a g e A l p h a                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageAlpha() sets the alpha levels of the image.
%
%  The format of the SetImageAlpha method is:
%
%      MagickBooleanType SetImageAlpha(Image *image,const Quantum alpha,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o alpha: the level of transparency: 0 is fully transparent and QuantumRange
%      is fully opaque.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageAlpha(Image *image,const Quantum alpha,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  PixelTrait
    original_mask;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  image->alpha_trait=BlendPixelTrait;
  /* Disable mask to make sure that all pixels are changed */
  original_mask=image->mask_trait;
  image->mask_trait=UndefinedPixelTrait;
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const AlphaRow row = { image, image_view, alpha, exception };

    SetImageAlphaRow(&row,y,&status);
  }
  image_view=DestroyCacheView(image_view);
  image->mask_trait=original_mask;
  return(status);
}

typedef struct _BackgroundRow
{
  Image
    *image;

  CacheView
    *image_view;

  PixelInfo
    background;

  ExceptionInfo
    *exception;
} BackgroundRow;

static void SetImageBackgroundRow(const BackgroundRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  const PixelInfo
    background = row->background;

  ExceptionInfo
    *exception = row->exception;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    SetPixelViaPixelInfo(image,&background,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e B a c k g r o u n d C o l o r                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageBackgroundColor() initializes the image pixels to the image
%  background color.  The background color is defined by the background_color
%  member of the image structure.
%
%  The format of the SetImage method is:
%
%      MagickBooleanType SetImageBackgroundColor(Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageBackgroundColor(Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  PixelInfo
    background;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((image->background_color.alpha_trait != UndefinedPixelTrait) &&
      ((image->alpha_trait & BlendPixelTrait) == 0))
    (void) SetImageAlphaChannel(image,ActivateAlphaChannel,exception);
  ConformPixelInfo(image,&image->background_color,&background,exception);
  /*
    Set image background color.
  */
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const BackgroundRow row = { image, image_view, background, exception };

    SetImageBackgroundRow(&row,y,&status);
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e C o l o r                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageColor() set the entire image canvas to the specified color.
%
%  The format of the SetImageColor method is:
%
%      MagickBooleanType SetImageColor(Image *image,const PixelInfo *color,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o background: the image color.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType SetImageColor(Image *image,
  const PixelInfo *color,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(color != (const PixelInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  image->colorspace=color->colorspace;
  image->alpha_trait=color->alpha_trait;
  image->fuzz=color->fuzz;
  image->depth=color->depth;
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      SetPixelViaPixelInfo(image,color,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}
