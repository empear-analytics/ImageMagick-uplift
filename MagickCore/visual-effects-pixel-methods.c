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
%                      PPPP   IIIII  X   X  EEEEE  L                          %
%                      P   P    I     X X   E      L                          %
%                      PPPP     I      X    EEE    L                          %
%                      P        I     X X   E      L                          %
%                      P      IIIII  X   X  EEEEE  LLLLL                      %
%                                                                             %
%               M   M  EEEEE  TTTTT  H   H   OOO   DDDD   SSSSS               %
%               MM MM  E        T    H   H  O   O  D   D  SS                  %
%               M M M  EEE      T    HHHHH  O   O  D   D   SSS                %
%               M   M  E        T    H   H  O   O  D   D     SS               %
%               M   M  EEEEE    T    H   H   OOO   DDDD   SSSSS               %
%                                                                             %
%                                                                             %
%                   MagickCore Pixel Visual Effects Methods                   %
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
%     I m p l o d e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImplodeImage() creates a new image that is a copy of an existing
%  one with the image pixels "implode" by the specified percentage.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ImplodeImage method is:
%
%      Image *ImplodeImage(const Image *image,const double amount,
%        const PixelInterpolateMethod method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o implode_image: Method ImplodeImage returns a pointer to the image
%      after it is implode.  A null image is returned if there is a memory
%      shortage.
%
%    o image: the image.
%
%    o amount:  Define the extent of the implosion.
%
%    o method: the pixel interpolation method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _WarpInfo
{
  const Image
    *image,
    *canvas_image,
    *warp_image;

  CacheView
    *interpolate_view;

  ExceptionInfo
    *exception;

  PixelInterpolateMethod
    method;

  PointInfo
    center,
    scale;

  double
    amount,
    radius;

  ssize_t
    y;
} WarpInfo;

typedef struct _WarpSite
{
  PointInfo
    delta;

  double
    distance;
} WarpSite;

static void CopyPixelChannels(const Image *source,
  const Image *destination,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(source); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(source,i);
    PixelTrait traits = GetPixelChannelTraits(source,channel);
    PixelTrait destination_traits = GetPixelChannelTraits(destination,
      channel);
    if (IsPixelChannelPairUndefined(traits,destination_traits) != MagickFalse)
      continue;
    SetPixelChannel(destination,channel,p[i],q);
  }
}

static MagickBooleanType ImplodePixel(const WarpInfo *warp,
  const WarpSite *site,Quantum *magick_restrict q,MagickBooleanType *status)
{
  double
    factor;

  PointInfo
    offset;

  /*
    Implode the pixel.
  */
  factor=1.0;
  if (site->distance > 0.0)
    factor=pow(sin(MagickPI*sqrt(site->distance)*
      MagickSafeReciprocal(warp->radius)/2.0),-warp->amount);
  offset.x=factor*site->delta.x*MagickSafeReciprocal(warp->scale.x)+
    warp->center.x;
  offset.y=factor*site->delta.y*MagickSafeReciprocal(warp->scale.y)+
    warp->center.y;
  if ((IsValidPixelOffset((ssize_t) offset.x,warp->image->columns) !=
       MagickFalse) &&
      (IsValidPixelOffset((ssize_t) offset.y,warp->image->rows) !=
       MagickFalse))
    (*status)=InterpolatePixelChannels(warp->canvas_image,
      warp->interpolate_view,warp->warp_image,warp->method,offset.x,offset.y,q,
      warp->exception);
  if ((*status) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void ImplodeRow(const WarpInfo *warp,const Quantum *magick_restrict p,
  Quantum *magick_restrict q,MagickBooleanType *status)
{
  ssize_t
    x;

  WarpSite
    site;

  site.delta.y=warp->scale.y*((double) warp->y-warp->center.y);
  for (x=0; x < (ssize_t) warp->canvas_image->columns; x++)
  {
    /*
      Determine if the pixel is within an ellipse.
    */
    site.delta.x=warp->scale.x*((double) x-warp->center.x);
    site.distance=site.delta.x*site.delta.x+site.delta.y*site.delta.y;
    if (site.distance >= (warp->radius*warp->radius))
      CopyPixelChannels(warp->canvas_image,warp->warp_image,p,q);
    else
      if (ImplodePixel(warp,&site,q,status) == MagickFalse)
        break;
    p+=(ptrdiff_t) GetPixelChannels(warp->canvas_image);
    q+=(ptrdiff_t) GetPixelChannels(warp->warp_image);
  }
}

static MagickBooleanType CloneWarpImage(Image *canvas_image,
  Image **warp_image,ExceptionInfo *exception)
{
  *warp_image=CloneImage(canvas_image,0,0,MagickTrue,exception);
  if (*warp_image == (Image *) NULL)
    {
      canvas_image=DestroyImage(canvas_image);
      return(MagickFalse);
    }
  if (SetImageStorageClass(*warp_image,DirectClass,exception) == MagickFalse)
    {
      canvas_image=DestroyImage(canvas_image);
      *warp_image=DestroyImage(*warp_image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void DestroyWarpResources(CacheView *warp_view,
  CacheView *interpolate_view,CacheView *canvas_view,Image *canvas_image)
{
  warp_view=DestroyCacheView(warp_view);
  interpolate_view=DestroyCacheView(interpolate_view);
  canvas_view=DestroyCacheView(canvas_view);
  canvas_image=DestroyImage(canvas_image);
}

static void SetImplodeCanvasAlpha(Image *canvas_image,
  ExceptionInfo *exception)
{
  if ((canvas_image->alpha_trait == UndefinedPixelTrait) &&
      (canvas_image->background_color.alpha != (double) OpaqueAlpha))
    (void) SetImageAlphaChannel(canvas_image,OpaqueAlphaChannel,exception);
}

static MagickBooleanType CloneImplodeCanvas(const Image *image,
  Image **canvas_image,ExceptionInfo *exception)
{
  *canvas_image=CloneImage(image,0,0,MagickTrue,exception);
  if (*canvas_image == (Image *) NULL)
    return(MagickFalse);
  SetImplodeCanvasAlpha(*canvas_image,exception);
  return(MagickTrue);
}

static void GetImplodeGeometry(const Image *canvas_image,PointInfo *scale,
  PointInfo *center,double *radius)
{
  (*scale).x=1.0;
  (*scale).y=1.0;
  (*center).x=0.5*canvas_image->columns;
  (*center).y=0.5*canvas_image->rows;
  *radius=(*center).x;
  if (canvas_image->columns > canvas_image->rows)
    (*scale).y=(double) canvas_image->columns*MagickSafeReciprocal((double)
      canvas_image->rows);
  else
    if (canvas_image->columns < canvas_image->rows)
      {
        (*scale).x=(double) canvas_image->rows*MagickSafeReciprocal((double)
          canvas_image->columns);
        *radius=(*center).y;
      }
}

MagickExport Image *ImplodeImage(const Image *image,const double amount,
  const PixelInterpolateMethod method,ExceptionInfo *exception)
{
#define ImplodeImageTag  "Implode/Image"

  CacheView
    *canvas_view,
    *implode_view,
    *interpolate_view;

  double
    radius;

  Image
    *canvas_image,
    *implode_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PointInfo
    center,
    scale;

  ssize_t
    y;

  /*
    Initialize implode image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneImplodeCanvas(image,&canvas_image,exception) == MagickFalse)
    return((Image *) NULL);
  if (CloneWarpImage(canvas_image,&implode_image,exception) == MagickFalse)
    return((Image *) NULL);
  /*
    Compute scaling factor.
  */
  GetImplodeGeometry(canvas_image,&scale,&center,&radius);
  /*
    Implode image.
  */
  status=MagickTrue;
  progress=0;
  canvas_view=AcquireVirtualCacheView(canvas_image,exception);
  interpolate_view=AcquireVirtualCacheView(canvas_image,exception);
  implode_view=AcquireAuthenticCacheView(implode_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(canvas_image,implode_image,canvas_image->rows,1)
#endif
  for (y=0; y < (ssize_t) canvas_image->rows; y++)
  {
    const EffectRows
      rows = { canvas_image,implode_image,canvas_view,implode_view,exception,y };

    const WarpInfo
      warp = { image,canvas_image,implode_image,interpolate_view,exception,
        method,center,scale,amount,radius,y };

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    if (GetEffectRows(&rows,&p,&q,&status) == MagickFalse)
      continue;
    ImplodeRow(&warp,p,q,&status);
    SyncEffectRow(implode_view,exception,&status);
    if (canvas_image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(canvas_image,ImplodeImageTag,progress,&status);
      }
  }
  DestroyWarpResources(implode_view,interpolate_view,canvas_view,canvas_image);
  return(ReleaseFailedImage(implode_image,status));
}




/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S o l a r i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SolarizeImage() applies a special effect to the image, similar to the effect
%  achieved in a photo darkroom by selectively exposing areas of photo
%  sensitive paper to light.  Threshold ranges from 0 to QuantumRange and is a
%  measure of the extent of the solarization.
%
%  The format of the SolarizeImage method is:
%
%      MagickBooleanType SolarizeImage(Image *image,const double threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o threshold:  Define the extent of the solarization.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void SolarizePixelChannels(const Image *image,const double threshold,
  Quantum *magick_restrict q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) == 0)
      continue;
    if ((double) q[i] > threshold)
      q[i]=QuantumRange-q[i];
  }
}

static void SolarizeColormap(Image *image,const double threshold)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) image->colors; i++)
  {
    if ((double) image->colormap[i].red > threshold)
      image->colormap[i].red=(double) QuantumRange-image->colormap[i].red;
    if ((double) image->colormap[i].green > threshold)
      image->colormap[i].green=(double) QuantumRange-
        image->colormap[i].green;
    if ((double) image->colormap[i].blue > threshold)
      image->colormap[i].blue=(double) QuantumRange-image->colormap[i].blue;
  }
}

typedef struct _SolarizeRow
{
  Image
    *image;

  CacheView
    *image_view;

  double
    threshold;

  ExceptionInfo
    *exception;
} SolarizeRow;

static MagickBooleanType SolarizeImageRow(const SolarizeRow *solarize_row,
  const ssize_t y,MagickBooleanType *status)
{
  Image
    *const image = solarize_row->image;

  CacheView
    *const image_view = solarize_row->image_view;

  const double
    threshold = solarize_row->threshold;

  ExceptionInfo
    *const exception = solarize_row->exception;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
  if (q == (Quantum *) NULL)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    SolarizePixelChannels(image,threshold,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static MagickBooleanType SolarizePseudoClassImage(Image *image,
  const double threshold,ExceptionInfo *exception)
{
  SolarizeColormap(image,threshold);
  return(SyncImage(image,exception));
}

static void ConformSolarizeColorspace(Image *image,ExceptionInfo *exception)
{
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
}

MagickExport MagickBooleanType SolarizeImage(Image *image,
  const double threshold,ExceptionInfo *exception)
{
#define SolarizeImageTag  "Solarize/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  ConformSolarizeColorspace(image,exception);
  if (image->storage_class == PseudoClass)
    {
      /*
        Solarize colormap.
      */
      return(SolarizePseudoClassImage(image,threshold,exception));
    }
  /*
    Solarize image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,2)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    SolarizeRow
      solarize_row;

    solarize_row.image=image;
    solarize_row.image_view=image_view;
    solarize_row.threshold=threshold;
    solarize_row.exception=exception;
    if (SolarizeImageRow(&solarize_row,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,SolarizeImageTag,progress,&status);
      }
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S w i r l I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SwirlImage() swirls the pixels about the center of the image, where
%  degrees indicates the sweep of the arc through which each pixel is moved.
%  You get a more dramatic effect as the degrees move from 1 to 360.
%
%  The format of the SwirlImage method is:
%
%      Image *SwirlImage(const Image *image,double degrees,
%        const PixelInterpolateMethod method,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o degrees: Define the tightness of the swirling effect.
%
%    o method: the pixel interpolation method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType SwirlPixel(const WarpInfo *warp,
  const WarpSite *site,Quantum *magick_restrict q,MagickBooleanType *status)
{
  double
    cosine,
    factor,
    sine;

  /*
    Swirl the pixel.
  */
  factor=1.0-sqrt((double) site->distance)/warp->radius;
  sine=sin((double) (warp->amount*factor*factor));
  cosine=cos((double) (warp->amount*factor*factor));
  (*status)=InterpolatePixelChannels(warp->canvas_image,warp->interpolate_view,
    warp->warp_image,warp->method,((cosine*site->delta.x-sine*site->delta.y)/
    warp->scale.x+warp->center.x),(double) ((sine*site->delta.x+cosine*
    site->delta.y)/warp->scale.y+warp->center.y),q,warp->exception);
  if ((*status) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static PointInfo GetSwirlScale(const Image *canvas_image)
{
  PointInfo
    scale;

  scale.x=1.0;
  scale.y=1.0;
  if (canvas_image->columns > canvas_image->rows)
    scale.y=(double) canvas_image->columns/(double) canvas_image->rows;
  else
    if (canvas_image->columns < canvas_image->rows)
      scale.x=(double) canvas_image->rows/(double) canvas_image->columns;
  return(scale);
}

static void GetSwirlGeometry(const Image *canvas_image,PointInfo *center,
  double *radius,PointInfo *scale)
{
  (*center).x=(double) canvas_image->columns/2.0;
  (*center).y=(double) canvas_image->rows/2.0;
  *radius=MagickMax((*center).x,(*center).y);
  *scale=GetSwirlScale(canvas_image);
}

static void SetSwirlImageAlpha(Image *swirl_image,ExceptionInfo *exception)
{
  if (swirl_image->background_color.alpha_trait != UndefinedPixelTrait)
    (void) SetImageAlphaChannel(swirl_image,OnAlphaChannel,exception);
}

typedef struct _SwirlRowInput
{
  EffectRows
    rows;

  WarpInfo
    warp;

  Image
    *canvas_image,
    *swirl_image;

  CacheView
    *swirl_view;

  ExceptionInfo
    *exception;

  PointInfo
    center,
    scale;

  double
    radius;

  ssize_t
    y;
} SwirlRowInput;

static MagickBooleanType SwirlImageRow(const SwirlRowInput *input,
  MagickBooleanType *status)
{
  const EffectRows
    rows = input->rows;

  const WarpInfo
    warp = input->warp;

  Image
    *const canvas_image = input->canvas_image,
    *const swirl_image = input->swirl_image;

  CacheView
    *const swirl_view = input->swirl_view;

  ExceptionInfo
    *const exception = input->exception;

  const PointInfo
    center = input->center,
    scale = input->scale;

  const double
    radius = input->radius;

  const ssize_t
    y = input->y;

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  WarpSite
    site;

  if (GetEffectRows(&rows,&p,&q,status) == MagickFalse)
    return(MagickFalse);
  site.delta.y=scale.y*(double) (y-center.y);
  for (x=0; x < (ssize_t) canvas_image->columns; x++)
  {
    /*
      Determine if the pixel is within an ellipse.
    */
    site.delta.x=scale.x*(double) (x-center.x);
    site.distance=site.delta.x*site.delta.x+site.delta.y*site.delta.y;
    if (site.distance >= (radius*radius))
      CopyPixelChannels(canvas_image,swirl_image,p,q);
    else
      if (SwirlPixel(&warp,&site,q,status) == MagickFalse)
        break;
    p+=(ptrdiff_t) GetPixelChannels(canvas_image);
    q+=(ptrdiff_t) GetPixelChannels(swirl_image);
  }
  SyncEffectRow(swirl_view,exception,status);
  return(MagickTrue);
}

MagickExport Image *SwirlImage(const Image *image,double degrees,
  const PixelInterpolateMethod method,ExceptionInfo *exception)
{
#define SwirlImageTag  "Swirl/Image"

  CacheView
    *canvas_view,
    *interpolate_view,
    *swirl_view;

  double
    radius;

  Image
    *canvas_image,
    *swirl_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PointInfo
    center,
    scale;

  ssize_t
    y;

  /*
    Initialize swirl image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  canvas_image=CloneImage(image,0,0,MagickTrue,exception);
  if (canvas_image == (Image *) NULL)
    return((Image *) NULL);
  if (CloneWarpImage(canvas_image,&swirl_image,exception) == MagickFalse)
    return((Image *) NULL);
  SetSwirlImageAlpha(swirl_image,exception);
  /*
    Compute scaling factor.
  */
  GetSwirlGeometry(canvas_image,&center,&radius,&scale);
  degrees=(double) DegreesToRadians(degrees);
  /*
    Swirl image.
  */
  status=MagickTrue;
  progress=0;
  canvas_view=AcquireVirtualCacheView(canvas_image,exception);
  interpolate_view=AcquireVirtualCacheView(image,exception);
  swirl_view=AcquireAuthenticCacheView(swirl_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(canvas_image,swirl_image,canvas_image->rows,1)
#endif
  for (y=0; y < (ssize_t) canvas_image->rows; y++)
  {
    const EffectRows
      rows = { canvas_image,swirl_image,canvas_view,swirl_view,exception,y };

    const WarpInfo
      warp = { image,canvas_image,swirl_image,interpolate_view,exception,
        method,center,scale,degrees,radius,y };

    const SwirlRowInput
      input = { rows, warp, canvas_image, swirl_image, swirl_view, exception,
        center, scale, radius, y };

    if (SwirlImageRow(&input,&status) == MagickFalse)
      continue;
    if (canvas_image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(canvas_image,SwirlImageTag,progress,&status);
      }
  }
  DestroyWarpResources(swirl_view,interpolate_view,canvas_view,canvas_image);
  return(ReleaseFailedImage(swirl_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     T i n t I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TintImage() applies a color vector to each pixel in the image.  The length
%  of the vector is 0 for black and white and at its maximum for the midtones.
%  The vector weighting function is f(x)=(1-(4.0*((x-0.5)*(x-0.5))))
%
%  The format of the TintImage method is:
%
%      Image *TintImage(const Image *image,const char *blend,
%        const PixelInfo *tint,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o blend: A color value used for tinting.
%
%    o tint: A color value used for tinting.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static PixelInfo ParseTintColorVector(const Image *image,const char *blend)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  PixelInfo
    color_vector;

  GetPixelInfo(image,&color_vector);
  flags=ParseGeometry(blend,&geometry_info);
  color_vector.red=geometry_info.rho;
  color_vector.green=geometry_info.rho;
  color_vector.blue=geometry_info.rho;
  color_vector.alpha=(MagickRealType) OpaqueAlpha;
  if ((flags & SigmaValue) != 0)
    color_vector.green=geometry_info.sigma;
  if ((flags & XiValue) != 0)
    color_vector.blue=geometry_info.xi;
  if ((flags & PsiValue) != 0)
    color_vector.alpha=geometry_info.psi;
  if (image->colorspace == CMYKColorspace)
    {
      color_vector.black=geometry_info.rho;
      if ((flags & PsiValue) != 0)
        color_vector.black=geometry_info.psi;
      if ((flags & ChiValue) != 0)
        color_vector.alpha=geometry_info.chi;
    }
  return(color_vector);
}

typedef struct _TintPixelInfo
{
  const Image
    *image,
    *tint_image;

  const PixelInfo
    *color_vector;
} TintPixelInfo;

static void TintPixel(const TintPixelInfo *pixel_info,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = pixel_info->image,
    *const tint_image = pixel_info->tint_image;

  const PixelInfo
    *const color_vector = pixel_info->color_vector;

  PixelInfo
    pixel;

  double
    weight;

  GetPixelInfo(image,&pixel);
  weight=QuantumScale*(double) GetPixelRed(image,p)-0.5;
  pixel.red=(MagickRealType) GetPixelRed(image,p)+color_vector->red*
    (1.0-(4.0*(weight*weight)));
  weight=QuantumScale*(double) GetPixelGreen(image,p)-0.5;
  pixel.green=(MagickRealType) GetPixelGreen(image,p)+color_vector->green*
    (1.0-(4.0*(weight*weight)));
  weight=QuantumScale*(double) GetPixelBlue(image,p)-0.5;
  pixel.blue=(MagickRealType) GetPixelBlue(image,p)+color_vector->blue*
    (1.0-(4.0*(weight*weight)));
  weight=QuantumScale*(double) GetPixelBlack(image,p)-0.5;
  pixel.black=(MagickRealType) GetPixelBlack(image,p)+color_vector->black*
    (1.0-(4.0*(weight*weight)));
  pixel.alpha=(MagickRealType) GetPixelAlpha(image,p);
  SetPixelViaPixelInfo(tint_image,&pixel,q);
}

static void SetTintColorspace(const Image *image,const PixelInfo *tint,
  Image *tint_image,ExceptionInfo *exception)
{
  if ((IsGrayColorspace(image->colorspace) != MagickFalse) &&
      (IsPixelInfoGray(tint) == MagickFalse))
    (void) SetImageColorspace(tint_image,sRGBColorspace,exception);
}

static PixelInfo ScaleTintColorVector(PixelInfo color_vector,
  const PixelInfo *tint)
{
  double
    intensity;

  intensity=(double) GetPixelInfoIntensity((const Image *) NULL,tint);
  color_vector.red=(double) (color_vector.red*tint->red/100.0-intensity);
  color_vector.green=(double) (color_vector.green*tint->green/100.0-intensity);
  color_vector.blue=(double) (color_vector.blue*tint->blue/100.0-intensity);
  color_vector.black=(double) (color_vector.black*tint->black/100.0-intensity);
  color_vector.alpha=(double) (color_vector.alpha*tint->alpha/100.0-intensity);
  return(color_vector);
}

typedef struct _TintRow
{
  const Image
    *image;

  Image
    *tint_image;

  PixelInfo
    color_vector;
} TintRow;

static void TintImageRow(const TintRow *tint_row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = tint_row->image;

  Image
    *const tint_image = tint_row->tint_image;

  const PixelInfo
    color_vector = tint_row->color_vector;

  const TintPixelInfo
    pixel_info = { image, tint_image, (&color_vector) };

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    TintPixel(&pixel_info,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(tint_image);
  }
}

MagickExport Image *TintImage(const Image *image,const char *blend,
  const PixelInfo *tint,ExceptionInfo *exception)
{
#define TintImageTag  "Tint/Image"

  CacheView
    *image_view,
    *tint_view;

  Image
    *tint_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelInfo
    color_vector;

  ssize_t
    y;

  /*
    Allocate tint image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (CloneEffectImage(image,&tint_image,exception) == MagickFalse)
    return((Image *) NULL);
  SetTintColorspace(image,tint,tint_image,exception);
  if (blend == (const char *) NULL)
    return(tint_image);
  /*
    Determine RGB values of the color.
  */
  color_vector=ParseTintColorVector(image,blend);
  color_vector=ScaleTintColorVector(color_vector,tint);
  /*
    Tint image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  tint_view=AcquireAuthenticCacheView(tint_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,tint_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const EffectRows
      rows = { image,tint_image,image_view,tint_view,exception,y };

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    TintRow
      tint_row;

    if (GetEffectRows(&rows,&p,&q,&status) == MagickFalse)
      continue;
    tint_row.image=image;
    tint_row.tint_image=tint_image;
    tint_row.color_vector=color_vector;
    TintImageRow(&tint_row,p,q);
    SyncEffectRow(tint_view,exception,&status);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetEffectProgress(image,TintImageTag,progress,&status);
      }
  }
  tint_view=DestroyCacheView(tint_view);
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(tint_image,status));
}

