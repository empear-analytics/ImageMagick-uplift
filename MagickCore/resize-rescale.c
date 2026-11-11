/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  RRRR   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                  R   R  E      SS       I       ZZ  E                       %
%                  RRRR   EEE     SSS     I     ZZZ   EEE                     %
%                  R R    E         SS    I    ZZ     E                       %
%                  R  R   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                                                                             %
%               RRRR   EEEEE  SSSSS   CCCC   AAA   L      EEEEE               %
%               R   R  E      SS     C      A   A  L      E                   %
%               RRRR   EEE     SSS   C      AAAAA  L      EEE                 %
%               R R    E         SS  C      A   A  L      E                   %
%               R  R   EEEEE  SSSSS   CCCC  A   A  LLLLL  EEEEE               %
%                                                                             %
%                                                                             %
%                      MagickCore Image Rescale Methods                       %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resize-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#if defined(MAGICKCORE_LQR_DELEGATE)
#include <lqr.h>
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t i v e R e s i z e I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpolativeResizeImage() resizes an image using the specified
%  interpolation method.
%
%  The format of the InterpolativeResizeImage method is:
%
%      Image *InterpolativeResizeImage(const Image *image,const size_t columns,
%        const size_t rows,const PixelInterpolateMethod method,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the resized image.
%
%    o rows: the number of rows in the resized image.
%
%    o method: the pixel interpolation method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsResizeSizeZero(const size_t columns,
  const size_t rows)
{
  return(((columns == 0) || (rows == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsResizeSizeUnchanged(const Image *image,
  const size_t columns,const size_t rows)
{
  return(((columns == image->columns) && (rows == image->rows)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsChannelTraitUndefined(
  const PixelTrait traits,const PixelTrait resize_traits)
{
  return(((traits == UndefinedPixelTrait) ||
    (resize_traits == UndefinedPixelTrait)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType SetDirectClassOrDestroy(Image *resize_image,
  ExceptionInfo *exception)
{
  if (SetImageStorageClass(resize_image,DirectClass,exception) == MagickFalse)
    {
      resize_image=DestroyImage(resize_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType SetResizeProgress(const Image *image,
  const char *tag,const MagickOffsetType progress)
{
  MagickBooleanType
    proceed,
    status;

  status=MagickTrue;
  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    status=MagickFalse;
  return(status);
}

static Image *FinishInterpolativeResize(CacheView *image_view,
  CacheView *resize_view,Image *resize_image,const MagickBooleanType status)
{
  resize_view=DestroyCacheView(resize_view);
  image_view=DestroyCacheView(image_view);
  if (status == MagickFalse)
    resize_image=DestroyImage(resize_image);
  return(resize_image);
}

static PointInfo GetInterpolativeScale(const Image *image,
  const Image *resize_image)
{
  PointInfo
    scale;

  scale.x=(double) image->columns/resize_image->columns;
  scale.y=(double) image->rows/resize_image->rows;
  return(scale);
}

typedef struct _InterpolativeResizeInfo
{
  const Image
    *image;

  CacheView
    *image_view,
    *resize_view;

  Image
    *resize_image;

  PixelInterpolateMethod
    method;

  PointInfo
    scale;

  ExceptionInfo
    *exception;
} InterpolativeResizeInfo;

static void InterpolateResizeRow(MagickBooleanType *status,
  const InterpolativeResizeInfo *resize,const double offset_y,Quantum *q)
{
  const Image
    *const image = resize->image;

  CacheView
    *const image_view = resize->image_view;

  Image
    *const resize_image = resize->resize_image;

  const PixelInterpolateMethod
    method = resize->method;

  const PointInfo
    scale = resize->scale;

  ExceptionInfo
    *const exception = resize->exception;

  PointInfo
    offset;

  ssize_t
    x;

  offset.y=offset_y;
  for (x=0; x < (ssize_t) resize_image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel
        channel;

      PixelTrait
        resize_traits,
        traits;

      channel=GetPixelChannelChannel(image,i);
      traits=GetPixelChannelTraits(image,channel);
      resize_traits=GetPixelChannelTraits(resize_image,channel);
      if (IsChannelTraitUndefined(traits,resize_traits) != MagickFalse)
        continue;
      offset.x=((double) x+0.5)*scale.x-0.5;
      (*status)=InterpolatePixelChannels(image,image_view,resize_image,method,
        offset.x,offset.y,q,exception);
      if ((*status) == MagickFalse)
        break;
    }
    q+=(ptrdiff_t) GetPixelChannels(resize_image);
  }
}

static void InterpolateResizeLine(MagickBooleanType *status,
  const InterpolativeResizeInfo *resize,const ssize_t y,Quantum *q)
{
  CacheView
    *const resize_view = resize->resize_view;

  const PointInfo
    scale = resize->scale;

  ExceptionInfo
    *const exception = resize->exception;

  InterpolateResizeRow(status,resize,((double) y+0.5)*scale.y-0.5,q);
  if (SyncCacheViewAuthenticPixels(resize_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType InterpolateResizeHead(
  const InterpolativeResizeInfo *resize,const ssize_t y,
  MagickBooleanType *status)
{
  CacheView
    *const resize_view = resize->resize_view;

  Image
    *const resize_image = resize->resize_image;

  ExceptionInfo
    *const exception = resize->exception;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=QueueCacheViewAuthenticPixels(resize_view,0,y,resize_image->columns,1,
    exception);
  if (q == (Quantum *) NULL)
    return(MagickFalse);
  InterpolateResizeLine(status,resize,y,q);
  return(MagickTrue);
}

static void ReportInterpolativeProgress(const Image *image,const char *tag,
  const MagickOffsetType progress,MagickBooleanType *status)
{
  if (SetResizeProgress(image,tag,progress) == MagickFalse)
    (*status)=MagickFalse;
}

static Image *InterpolateResizeRows(const Image *image,Image *resize_image,
  const PixelInterpolateMethod method,ExceptionInfo *exception)
{
#define InterpolativeResizeImageTag  "Resize/Image"

  CacheView
    *image_view,
    *resize_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PointInfo
    scale;

  ssize_t
    y;

  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  resize_view=AcquireAuthenticCacheView(resize_image,exception);
  scale=GetInterpolativeScale(image,resize_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,resize_image,resize_image->rows,1)
#endif
  for (y=0; y < (ssize_t) resize_image->rows; y++)
  {
    const InterpolativeResizeInfo
      resize = { image,image_view,resize_view,resize_image,method,scale,
        exception };

    if (InterpolateResizeHead(&resize,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportInterpolativeProgress(image,InterpolativeResizeImageTag,progress,
          &status);
      }
  }
  return(FinishInterpolativeResize(image_view,resize_view,resize_image,status));
}

MagickExport Image *InterpolativeResizeImage(const Image *image,
  const size_t columns,const size_t rows,const PixelInterpolateMethod method,
  ExceptionInfo *exception)
{
  Image
    *resize_image;

  /*
    Interpolatively resize image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsResizeSizeZero(columns,rows) != MagickFalse)
    ThrowImageException(ImageError,"NegativeOrZeroImageSize");
  if (IsResizeSizeUnchanged(image,columns,rows) != MagickFalse)
    return(CloneImage(image,0,0,MagickTrue,exception));
  resize_image=CloneImage(image,columns,rows,MagickTrue,exception);
  if (resize_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetDirectClassOrDestroy(resize_image,exception) == MagickFalse)
    return((Image *) NULL);
  return(InterpolateResizeRows(image,resize_image,method,exception));
}
#if defined(MAGICKCORE_LQR_DELEGATE)

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L i q u i d R e s c a l e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LiquidRescaleImage() rescales image with seam carving.
%
%  The format of the LiquidRescaleImage method is:
%
%      Image *LiquidRescaleImage(const Image *image,const size_t columns,
%        const size_t rows,const double delta_x,const double rigidity,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the rescaled image.
%
%    o rows: the number of rows in the rescaled image.
%
%    o delta_x: maximum seam transversal step (0 means straight seams).
%
%    o rigidity: introduce a bias for non-straight seams (typically 0).
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SetLiquidRescalePixelChannels(const Image *image,
  Image *rescale_image,gfloat *packet,Quantum *magick_restrict p)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel
      channel;

    PixelTrait
      rescale_traits,
      traits;

    channel=GetPixelChannelChannel(image,i);
    traits=GetPixelChannelTraits(image,channel);
    rescale_traits=GetPixelChannelTraits(rescale_image,channel);
    if (IsChannelTraitUndefined(traits,rescale_traits) != MagickFalse)
      continue;
    SetPixelChannel(rescale_image,channel,ClampToQuantum(QuantumRange*
      packet[i]),p);
  }
}

static gfloat *ImportLiquidRescaleRow(const Image *image,
  const Quantum *magick_restrict p,gfloat *q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      *q++=(gfloat) (QuantumScale*(double) p[i]);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void ExportLiquidRescalePixels(const Image *image,
  Image *rescale_image,LqrCarver *carver,ExceptionInfo *exception)
{
  CacheView
    *rescale_view;

  gfloat
    *packet;

  int
    x_offset,
    y_offset;

  rescale_view=AcquireAuthenticCacheView(rescale_image,exception);
  (void) lqr_carver_scan_reset(carver);
  while (lqr_carver_scan_ext(carver,&x_offset,&y_offset,(void **) &packet) != 0)
  {
    Quantum
      *magick_restrict p;

    p=QueueCacheViewAuthenticPixels(rescale_view,x_offset,y_offset,1,1,
      exception);
    if (p == (Quantum *) NULL)
      break;
    SetLiquidRescalePixelChannels(image,rescale_image,packet,p);
    if (SyncCacheViewAuthenticPixels(rescale_view,exception) == MagickFalse)
      break;
  }
  rescale_view=DestroyCacheView(rescale_view);
}

static void ImportLiquidRescalePixels(const Image *image,gfloat *pixels,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  gfloat
    *q;

  MagickBooleanType
    status;

  ssize_t
    y;

  status=MagickTrue;
  q=pixels;
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    q=ImportLiquidRescaleRow(image,p,q);
  }
  image_view=DestroyCacheView(image_view);
}

static inline MagickBooleanType IsTooSmallToCarve(const size_t columns,
  const size_t rows)
{
  return(((columns <= 2) || (rows <= 2)) ? MagickTrue : MagickFalse);
}

static Image *AcquireLiquidRescaledImage(const Image *image,
  LqrCarver *carver,MemoryInfo *pixel_info,ExceptionInfo *exception)
{
  Image
    *rescale_image;

  rescale_image=CloneImage(image,(size_t) lqr_carver_get_width(carver),
    (size_t) lqr_carver_get_height(carver),MagickTrue,exception);
  if (rescale_image == (Image *) NULL)
    {
      pixel_info=RelinquishVirtualMemory(pixel_info);
      return((Image *) NULL);
    }
  if (SetImageStorageClass(rescale_image,DirectClass,exception) == MagickFalse)
    {
      pixel_info=RelinquishVirtualMemory(pixel_info);
      rescale_image=DestroyImage(rescale_image);
      return((Image *) NULL);
    }
  ExportLiquidRescalePixels(image,rescale_image,carver,exception);
  pixel_info=RelinquishVirtualMemory(pixel_info);
  lqr_carver_destroy(carver);
  return(rescale_image);
}

MagickExport Image *LiquidRescaleImage(const Image *image,const size_t columns,
  const size_t rows,const double delta_x,const double rigidity,
  ExceptionInfo *exception)
{
#define LiquidRescaleImageTag  "Rescale/Image"

  gfloat
    *pixels;

  LqrCarver
    *carver;

  LqrRetVal
    lqr_status;

  MemoryInfo
    *pixel_info;

  /*
    Liquid rescale image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsResizeSizeZero(columns,rows) != MagickFalse)
    ThrowImageException(ImageError,"NegativeOrZeroImageSize");
  if (IsResizeSizeUnchanged(image,columns,rows) != MagickFalse)
    return(CloneImage(image,0,0,MagickTrue,exception));
  if (IsTooSmallToCarve(columns,rows) != MagickFalse)
    return(ResizeImage(image,columns,rows,image->filter,exception));
  pixel_info=AcquireVirtualMemory(image->columns,image->rows*MaxPixelChannels*
    sizeof(*pixels));
  if (pixel_info == (MemoryInfo *) NULL)
    return((Image *) NULL);
  pixels=(gfloat *) GetVirtualMemoryBlob(pixel_info);
  ImportLiquidRescalePixels(image,pixels,exception);
  carver=lqr_carver_new_ext(pixels,(int) image->columns,(int) image->rows,
    (int) GetPixelChannels(image),LQR_COLDEPTH_32F);
  if (carver == (LqrCarver *) NULL)
    {
      pixel_info=RelinquishVirtualMemory(pixel_info);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  lqr_carver_set_preserve_input_image(carver);
  lqr_status=lqr_carver_init(carver,(gint) delta_x,(gfloat) rigidity);
  lqr_status=lqr_carver_resize(carver,(gint) columns,(gint) rows);
  (void) lqr_status;
  return(AcquireLiquidRescaledImage(image,carver,pixel_info,exception));
}
#else
MagickExport Image *LiquidRescaleImage(const Image *image,
  const size_t magick_unused(columns),const size_t magick_unused(rows),
  const double magick_unused(delta_x),const double magick_unused(rigidity),
  ExceptionInfo *exception)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  magick_unreferenced(columns);
  magick_unreferenced(rows);
  magick_unreferenced(delta_x);
  magick_unreferenced(rigidity);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  (void) ThrowMagickException(exception,GetMagickModule(),MissingDelegateError,
    "DelegateLibrarySupportNotBuiltIn","'%s' (LQR)",image->filename);
  return((Image *) NULL);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M i n i f y I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MinifyImage() is a convenience method that scales an image proportionally to
%  half its size.
%
%  The format of the MinifyImage method is:
%
%      Image *MinifyImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *MinifyImage(const Image *image,ExceptionInfo *exception)
{
  Image
    *minify_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  minify_image=ResizeImage(image,image->columns/2,image->rows/2,SplineFilter,
    exception);
  return(minify_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s a m p l e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResampleImage() resize image in terms of its pixel size, so that when
%  displayed at the given resolution it will be the same size in terms of
%  real world units as the original image at the original resolution.
%
%  The format of the ResampleImage method is:
%
%      Image *ResampleImage(Image *image,const double x_resolution,
%        const double y_resolution,const FilterType filter,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image to be resized to fit the given resolution.
%
%    o x_resolution: the new image x resolution.
%
%    o y_resolution: the new image y resolution.
%
%    o filter: Image filter to use.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *ResampleImage(const Image *image,const double x_resolution,
  const double y_resolution,const FilterType filter,ExceptionInfo *exception)
{
#define ResampleImageTag  "Resample/Image"

  Image
    *resample_image;

  size_t
    height,
    width;

  /*
    Initialize sampled image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=(size_t) (x_resolution*image->columns/(image->resolution.x == 0.0 ?
    DefaultResolution : image->resolution.x)+0.5);
  height=(size_t) (y_resolution*image->rows/(image->resolution.y == 0.0 ?
    DefaultResolution : image->resolution.y)+0.5);
  resample_image=ResizeImage(image,width,height,filter,exception);
  if (resample_image != (Image *) NULL)
    {
      resample_image->resolution.x=x_resolution;
      resample_image->resolution.y=y_resolution;
    }
  return(resample_image);
}




/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T h u m b n a i l I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ThumbnailImage() changes the size of an image to the given dimensions and
%  removes any associated profiles.  The goal is to produce small low cost
%  thumbnail images suited for display on the Web.
%
%  The format of the ThumbnailImage method is:
%
%      Image *ThumbnailImage(const Image *image,const size_t columns,
%        const size_t rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the scaled image.
%
%    o rows: the number of rows in the scaled image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsURIUnreserved(const char *uri)
{
  return(((('a' <= *uri) && (*uri <= 'z')) ||
    (('A' <= *uri) && (*uri <= 'Z')) ||
    (('0' <= *uri) && (*uri <= '9')) || (strchr("/-_.~",*uri) != 0)) ?
    MagickTrue : MagickFalse);
}

static void url_encode(const char *uri,char *encode_uri)
{
  char
    *p;

  const char
    *hex = "0123456789ABCDEF";

  for (p=encode_uri; *uri != '\0'; uri++)
    if (IsURIUnreserved(uri) != MagickFalse)
      *p++=(*uri);
    else
      {
        *p++='%';
        *p++=hex[(*uri >> 4) & 0xF];
        *p++=hex[*uri & 0xF];
      }
  *p='\0';
}

static void SetThumbnailProperties(const Image *image,Image *thumbnail_image,
  ExceptionInfo *exception)
{
  char
    encode_uri[3*MagickPathExtent+1] = "/0";

  const char
    *mime_type;

  struct stat
    attributes;

  (void) DeleteImageProperty(thumbnail_image,"comment");
  url_encode(image->filename,encode_uri);
  if (*image->filename != '/')
    (void) FormatImageProperty(thumbnail_image,"Thumb::URI","./%s",encode_uri);
  else
    (void) FormatImageProperty(thumbnail_image,"Thumb::URI","file://%s",
      encode_uri);
  if (GetPathAttributes(image->filename,&attributes) != MagickFalse )
    (void) FormatImageProperty(thumbnail_image,"Thumb::MTime","%.17g",(double)
      attributes.st_mtime);
  (void) FormatImageProperty(thumbnail_image,"Thumb::Size","%.17g",
    (double) GetBlobSize(image));
  mime_type=GetImageProperty(image,"mime:type",exception);
  if (mime_type != (const char *) NULL)
    (void) SetImageProperty(thumbnail_image,"Thumb::Mimetype",mime_type,
      exception);
  (void) SetImageProperty(thumbnail_image,"software",MagickAuthoritativeURL,
    exception);
  (void) FormatImageProperty(thumbnail_image,"Thumb::Image::Width","%.17g",
    (double) image->magick_columns);
  (void) FormatImageProperty(thumbnail_image,"Thumb::Image::Height","%.17g",
    (double) image->magick_rows);
  (void) FormatImageProperty(thumbnail_image,"Thumb::Document::Pages","%.17g",
    (double) GetImageListLength(image));
}

static void StripThumbnailProfiles(Image *thumbnail_image)
{
  const char
    *name;

  for (name=GetNextImageProfile(thumbnail_image); name != (const char *) NULL; )
  {
    if ((LocaleCompare(name,"icc") != 0) && (LocaleCompare(name,"icm") != 0))
     {
       (void) DeleteImageProfile(thumbnail_image,name);
       ResetImageProfileIterator(thumbnail_image);
     }
    name=GetNextImageProfile(thumbnail_image);
  }
}

static Image *KeepReducedThumbnail(Image *clone_image,Image *thumbnail_image)
{
  if (thumbnail_image != (Image *) NULL)
    {
      clone_image=DestroyImage(clone_image);
      clone_image=thumbnail_image;
    }
  return(clone_image);
}

static inline MagickBooleanType IsReducibleBy(const ssize_t x_factor,
  const ssize_t y_factor,const ssize_t factor)
{
  return(((x_factor > factor) && (y_factor > factor)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsThumbnailResized(const Image *image,
  const size_t columns,const size_t rows)
{
  return(((columns != image->columns) || (rows != image->rows)) ?
    MagickTrue : MagickFalse);
}

MagickExport Image *ThumbnailImage(const Image *image,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
#define SampleFactor  5

  Image
    *thumbnail_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  thumbnail_image=CloneImage(image,0,0,MagickTrue,exception);
  if (thumbnail_image == (Image *) NULL)
    return(thumbnail_image);
  if (IsThumbnailResized(image,columns,rows) != MagickFalse)
    {
      Image
        *clone_image = thumbnail_image;

      ssize_t
        x_factor,
        y_factor;

      x_factor=(ssize_t) (image->columns*MagickSafeReciprocal((double) 
        columns));
      y_factor=(ssize_t) (image->rows*MagickSafeReciprocal((double) rows));
      if (IsReducibleBy(x_factor,y_factor,4) != MagickFalse)
        {
          thumbnail_image=SampleImage(clone_image,4*columns,4*rows,exception);
          clone_image=KeepReducedThumbnail(clone_image,thumbnail_image);
        }
      if (IsReducibleBy(x_factor,y_factor,2) != MagickFalse)
        {
          thumbnail_image=ResizeImage(clone_image,2*columns,2*rows,BoxFilter,
            exception);
          clone_image=KeepReducedThumbnail(clone_image,thumbnail_image);
        }
      thumbnail_image=ResizeImage(clone_image,columns,rows,image->filter ==
        UndefinedFilter ? LanczosSharpFilter : image->filter,exception);
      clone_image=DestroyImage(clone_image);
      if (thumbnail_image == (Image *) NULL)
        return(thumbnail_image);
    }
  (void) ParseAbsoluteGeometry("0x0+0+0",&thumbnail_image->page);
  thumbnail_image->depth=8;
  thumbnail_image->interlace=NoInterlace;
  /*
    Strip all profiles except color profiles.
  */
  ResetImageProfileIterator(thumbnail_image);
  StripThumbnailProfiles(thumbnail_image);
  SetThumbnailProperties(image,thumbnail_image,exception);
  return(thumbnail_image);
}
