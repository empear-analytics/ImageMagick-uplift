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
%                         M   M   AAA   SSSSS  K   K                          %
%                         MM MM  A   A  SS     K  K                           %
%                         M M M  AAAAA   SSS   KKK                            %
%                         M   M  A   A     SS  K  K                           %
%                         M   M  A   A  SSSSS  K   K                          %
%                                                                             %
%                                                                             %
%                        MagickCore Image Mask Methods                        %
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

static void RemoveImageMaskChannel(Image *image,const PixelMask type)
{
  switch (type)
  {
    case ReadPixelMask:
    {
      image->channels=(ChannelType) ((unsigned int) image->channels &
        (unsigned int) ~ReadMaskChannel);
      break;
    }
    case WritePixelMask:
    {
      image->channels=(ChannelType) ((unsigned int) image->channels &
        (unsigned int) ~WriteMaskChannel);
      magick_fallthrough;
    }
    default:
    {
      image->channels=(ChannelType) ((unsigned int) image->channels &
        (unsigned int) ~CompositeMaskChannel);
      break;
    }
  }
}

static void SetMaskPixel(const Image *image,const PixelMask type,
  const Quantum pixel,Quantum *q)
{
  switch (type)
  {
    case ReadPixelMask:
    {
      SetPixelReadMask(image,pixel,q);
      break;
    }
    case WritePixelMask:
    {
      SetPixelWriteMask(image,pixel,q);
      break;
    }
    default:
    {
      SetPixelCompositeMask(image,pixel,q);
      break;
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e M a s k                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageMask() associates a mask with the image.  The mask must be the same
%  dimensions as the image.
%
%  The format of the SetImageMask method is:
%
%      MagickBooleanType SetImageMask(Image *image,const PixelMask type,
%        const Image *mask,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o type: the mask type, ReadPixelMask or WritePixelMask.
%
%    o mask: the image mask.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ChannelType GetMaskChannel(const PixelMask type)
{
  switch (type)
  {
    case ReadPixelMask: return(ReadMaskChannel);
    case WritePixelMask: return(WriteMaskChannel);
    default: return(CompositeMaskChannel);
  }
}

static void AddImageMaskChannel(Image *image,const PixelMask type)
{
  image->channels=(ChannelType) (image->channels | GetMaskChannel(type));
}

typedef struct _ImageMaskRow
{
  Image
    *image;

  const Image
    *mask;

  CacheView
    *image_view,
    *mask_view;

  PixelMask
    type;

  ExceptionInfo
    *exception;
} ImageMaskRow;

static inline MagickRealType GetMaskRowIntensity(const Image *mask,
  const Quantum *magick_restrict p,const ssize_t x,const ssize_t y)
{
  MagickRealType
    intensity;

  intensity=0.0;
  if ((x < (ssize_t) mask->columns) && (y < (ssize_t) mask->rows))
    intensity=GetPixelIntensity(mask,p);
  return(intensity);
}

static void SetImageMaskRow(const ImageMaskRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  const Image
    *mask = row->mask;

  CacheView
    *image_view = row->image_view,
    *mask_view = row->mask_view;

  const PixelMask
    type = row->type;

  ExceptionInfo
    *exception = row->exception;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  if ((*status) == MagickFalse)
    return;
  p=GetCacheViewVirtualPixels(mask_view,0,y,mask->columns,1,exception);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return;
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    MagickRealType
      intensity;

    intensity=GetMaskRowIntensity(mask,p,x,y);
    SetMaskPixel(image,type,ClampToQuantum(intensity),q);
    p+=(ptrdiff_t) GetPixelChannels(mask);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport MagickBooleanType SetImageMask(Image *image,const PixelMask type,
  const Image *mask,ExceptionInfo *exception)
{
  CacheView
    *mask_view,
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  /*
    Set image mask.
  */
  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickCoreSignature);
  image->mask_trait=UndefinedPixelTrait;
  if (mask == (const Image *) NULL)
    {
      RemoveImageMaskChannel(image,type);
      return(SyncImagePixelCache(image,exception));
    }
  AddImageMaskChannel(image,type);
  if (SyncImagePixelCache(image,exception) == MagickFalse)
    return(MagickFalse);
  status=MagickTrue;
  mask_view=AcquireVirtualCacheView(mask,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(mask,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const ImageMaskRow row = { image, mask, image_view, mask_view, type,
      exception };

    SetImageMaskRow(&row,y,&status);
  }
  mask_view=DestroyCacheView(mask_view);
  image_view=DestroyCacheView(image_view);
  image->mask_trait=UpdatePixelTrait;
  return(status);
}

static void RemoveRegionMaskChannel(Image *image,const PixelMask type)
{
  image->channels=(ChannelType) ((unsigned int) image->channels &
    (unsigned int) ~GetMaskChannel(type));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e R e g i o n M a s k                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageRegionMask() associates a mask with the image as defined by the
%  specified region.
%
%  The format of the SetImageRegionMask method is:
%
%      MagickBooleanType SetImageRegionMask(Image *image,const PixelMask type,
%        const RectangleInfo *region,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o type: the mask type, ReadPixelMask or WritePixelMask.
%
%    o geometry: the mask region.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsInsideRegion(const RectangleInfo *region,
  const ssize_t x,const ssize_t y)
{
  return((((x >= region->x) && (x < (region->x+(ssize_t) region->width))) &&
    ((y >= region->y) && (y < (region->y+(ssize_t) region->height)))) ?
    MagickTrue : MagickFalse);
}

typedef struct _RegionMaskRow
{
  Image
    *image;

  CacheView
    *image_view;

  const RectangleInfo
    *region;

  PixelMask
    type;

  ExceptionInfo
    *exception;
} RegionMaskRow;

static void SetRegionMaskRow(const RegionMaskRow *row,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  const RectangleInfo
    *region = row->region;

  const PixelMask
    type = row->type;

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
    Quantum
      pixel;

    pixel=QuantumRange;
    if (IsInsideRegion(region,x,y) != MagickFalse)
      pixel=(Quantum) 0;
    SetMaskPixel(image,type,pixel,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport MagickBooleanType SetImageRegionMask(Image *image,
  const PixelMask type,const RectangleInfo *region,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickBooleanType
    status;

  ssize_t
    y;

  /*
    Set image mask as defined by the region.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  image->mask_trait=UndefinedPixelTrait;
  if (region == (const RectangleInfo *) NULL)
    {
      RemoveRegionMaskChannel(image,type);
      return(SyncImagePixelCache(image,exception));
    }
  AddImageMaskChannel(image,type);
  if (SyncImagePixelCache(image,exception) == MagickFalse)
    return(MagickFalse);
  status=MagickTrue;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const RegionMaskRow row = { image, image_view, region, type, exception };

    SetRegionMaskRow(&row,y,&status);
  }
  image->mask_trait=UpdatePixelTrait;
  image_view=DestroyCacheView(image_view);
  return(status);
}
