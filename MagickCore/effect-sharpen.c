/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT                   %
%                  E      F      F      E      C        T                     %
%                  EEE    FFF    FFF    EEE    C        T                     %
%                  E      F      F      E      C        T                     %
%                  EEEEE  F      F      EEEEE   CCCC    T                     %
%                                                                             %
%               SSSSS  H   H   AAA   RRRR   PPPP   EEEEE  N   N               %
%               SS     H   H  A   A  R   R  P   P  E      NN  N               %
%                SSS   HHHHH  AAAAA  RRRR   PPPP   EEE    N N N               %
%                  SS  H   H  A   A  R R    P      E      N  NN               %
%               SSSSS  H   H  A   A  R  R   P      EEEEE  N   N               %
%                                                                             %
%                                                                             %
%                 MagickCore Image Shade and Sharpen Methods                  %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
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
#include "MagickCore/blob.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/fx.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/matrix.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/segment.h"
#include "MagickCore/shear.h"
#include "MagickCore/signature-private.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/threshold.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/effect-private.h"

/*
  Typedef declarations.
*/
typedef struct _ShadeImages
{
  const Image
    *linear_image,
    *shade_image;

  MagickBooleanType
    gray;
} ShadeImages;

typedef struct _UnsharpPixels
{
  const Image
    *image,
    *unsharp_image;

  double
    quantum_threshold,
    gain;
} UnsharpPixels;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h a d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShadeImage() shines a distant light on an image to create a
%  three-dimensional effect. You control the positioning of the light with
%  azimuth and elevation; azimuth is measured in degrees off the x axis
%  and elevation is measured in pixels above the Z axis.
%
%  The format of the ShadeImage method is:
%
%      Image *ShadeImage(const Image *image,const MagickBooleanType gray,
%        const double azimuth,const double elevation,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o gray: A value other than zero shades the intensity of each pixel.
%
%    o azimuth, elevation:  Define the light source direction.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline double GetSlopedSurfaceShade(const PrimaryInfo *normal,
  const PrimaryInfo *light)
{
  double
    distance,
    normal_distance,
    shade;

  shade=0.0;
  distance=normal->x*light->x+normal->y*light->y+normal->z*light->z;
  if (distance > MagickEpsilon)
    {
      normal_distance=normal->x*normal->x+normal->y*normal->y+
        normal->z*normal->z;
      if (normal_distance > (MagickEpsilon*MagickEpsilon))
        shade=distance/sqrt((double) normal_distance);
    }
  return(shade);
}

static inline double GetSurfaceShade(const PrimaryInfo *normal,
  const PrimaryInfo *light)
{
  double
    shade;

  if ((fabs(normal->x) <= MagickEpsilon) &&
      (fabs(normal->y) <= MagickEpsilon))
    shade=light->z;
  else
    shade=GetSlopedSurfaceShade(normal,light);
  return(shade);
}

typedef struct _ShadeChannel
{
  const Image
    *linear_image,
    *shade_image;

  const Quantum
    *center;

  MagickBooleanType
    gray;

  double
    shade;
} ShadeChannel;

static inline void ShadePixelChannel(const ShadeChannel *shade_channel,
  const ssize_t i,Quantum *magick_restrict q)
{
  const Image
    *const linear_image = shade_channel->linear_image,
    *const shade_image = shade_channel->shade_image;

  const Quantum
    *const center = shade_channel->center;

  const MagickBooleanType
    gray = shade_channel->gray;

  const double
    shade = shade_channel->shade;

  PixelChannel
    channel;

  PixelTrait
    shade_traits,
    traits;

  channel=GetPixelChannelChannel(linear_image,i);
  traits=GetPixelChannelTraits(linear_image,channel);
  shade_traits=GetPixelChannelTraits(shade_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (shade_traits == UndefinedPixelTrait))
    return;
  if ((shade_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(shade_image,channel,center[i],q);
      return;
    }
  if ((traits & UpdatePixelTrait) == 0)
    {
      SetPixelChannel(shade_image,channel,center[i],q);
      return;
    }
  if (gray != MagickFalse)
    {
      SetPixelChannel(shade_image,channel,ClampToQuantum(shade),q);
      return;
    }
  SetPixelChannel(shade_image,channel,ClampToQuantum(QuantumScale*
    shade*(double) center[i]),q);
}

static inline void ShadePixelChannels(const ShadeImages *shading,
  const Quantum *magick_restrict center,Quantum *magick_restrict q,
  const double shade)
{
  const Image
    *const linear_image = shading->linear_image,
    *const shade_image = shading->shade_image;

  const MagickBooleanType
    gray = shading->gray;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(linear_image); i++)
  {
    ShadeChannel
      shade_channel;

    shade_channel.linear_image=linear_image;
    shade_channel.shade_image=shade_image;
    shade_channel.center=center;
    shade_channel.gray=gray;
    shade_channel.shade=shade;
    ShadePixelChannel(&shade_channel,i,q);
  }
}

#define GetShadeIntensity(image,pixel) \
  ClampPixel(GetPixelIntensity((image),(pixel)))
#define ShadeImageTag  "Shade/Image"

typedef struct _ShadeRow
{
  const Image
    *linear_image,
    *shade_image;

  PrimaryInfo
    light;

  ShadeImages
    shading;

  CacheView
    *image_view,
    *shade_view;

  ExceptionInfo
    *exception;

  ssize_t
    y;
} ShadeRow;

static void ShadeRowPixels(const ShadeRow *row,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  const Image
    *const linear_image = row->linear_image,
    *const shade_image = row->shade_image;

  const PrimaryInfo
    light = row->light;

  const ShadeImages
    shading = row->shading;

  const Quantum
    *magick_restrict center,
    *magick_restrict post,
    *magick_restrict pre;

  double
    shade;

  PrimaryInfo
    normal;

  ssize_t
    x;

  normal.z=2.0*(double) QuantumRange;  /* constant Z of surface normal */
  for (x=0; x < (ssize_t) linear_image->columns; x++)
  {
    /*
      Determine the surface normal and compute shading.
    */
    pre=p+GetPixelChannels(linear_image);
    center=pre+(linear_image->columns+2)*GetPixelChannels(linear_image);
    post=center+(linear_image->columns+2)*GetPixelChannels(linear_image);
    normal.x=(double) (
      GetShadeIntensity(linear_image,pre-GetPixelChannels(linear_image))+
      GetShadeIntensity(linear_image,center-GetPixelChannels(linear_image))+
      GetShadeIntensity(linear_image,post-GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,pre+GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,center+GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,post+GetPixelChannels(linear_image)));
    normal.y=(double) (
      GetShadeIntensity(linear_image,post-GetPixelChannels(linear_image))+
      GetShadeIntensity(linear_image,post)+
      GetShadeIntensity(linear_image,post+GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,pre-GetPixelChannels(linear_image))-
      GetShadeIntensity(linear_image,pre)-
      GetShadeIntensity(linear_image,pre+GetPixelChannels(linear_image)));
    shade=GetSurfaceShade(&normal,&light);
    ShadePixelChannels(&shading,center,q,shade);
    p+=(ptrdiff_t) GetPixelChannels(linear_image);
    q+=(ptrdiff_t) GetPixelChannels(shade_image);
  }
}

typedef struct _ShadeImageTargets
{
  Image
    **linear_image,
    **shade_image;
} ShadeImageTargets;

static MagickBooleanType AcquireShadeImages(const Image *image,
  ExceptionInfo *exception,const ShadeImageTargets *targets)
{
  Image
    **const linear_image = targets->linear_image,
    **const shade_image = targets->shade_image;

  (*linear_image)=CloneImage(image,0,0,MagickTrue,exception);
  (*shade_image)=CloneImage(image,0,0,MagickTrue,exception);
  if (((*linear_image) == (Image *) NULL) || ((*shade_image) == (Image *) NULL))
    {
      if ((*linear_image) != (Image *) NULL)
        (*linear_image)=DestroyImage((*linear_image));
      if ((*shade_image) != (Image *) NULL)
        (*shade_image)=DestroyImage((*shade_image));
      return(MagickFalse);
    }
  if (SetImageStorageClass((*shade_image),DirectClass,exception) == MagickFalse)
    {
      (*linear_image)=DestroyImage((*linear_image));
      (*shade_image)=DestroyImage((*shade_image));
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void SyncSharpenRow(MagickBooleanType *status,CacheView *view,
  ExceptionInfo *exception)
{
  if (SyncCacheViewAuthenticPixels(view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

static MagickBooleanType ShadeHeadRow(MagickBooleanType *status,
  const ShadeRow *row)
{
  const Image
    *const linear_image = row->linear_image,
    *const shade_image = row->shade_image;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(row->image_view,-1,row->y-1,
    linear_image->columns+2,3,row->exception);
  q=QueueCacheViewAuthenticPixels(row->shade_view,0,row->y,shade_image->columns,
    1,row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  ShadeRowPixels(row,p,q);
  return(MagickTrue);
}

static PrimaryInfo GetShadeLight(const double azimuth,const double elevation)
{
  PrimaryInfo
    light;

  light.x=(double) QuantumRange*cos(DegreesToRadians(azimuth))*
    cos(DegreesToRadians(elevation));
  light.y=(double) QuantumRange*sin(DegreesToRadians(azimuth))*
    cos(DegreesToRadians(elevation));
  light.z=(double) QuantumRange*sin(DegreesToRadians(elevation));
  return(light);
}

MagickExport Image *ShadeImage(const Image *image,const MagickBooleanType gray,
  const double azimuth,const double elevation,ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *shade_view;

  Image
    *linear_image,
    *shade_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PrimaryInfo
    light;

  ShadeImageTargets
    targets;

  ssize_t
    y;

  /*
    Initialize shaded image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  targets.linear_image=(&linear_image);
  targets.shade_image=(&shade_image);
  if (AcquireShadeImages(image,exception,&targets) == MagickFalse)
    return((Image *) NULL);
  /*
    Compute the light vector.
  */
  light=GetShadeLight(azimuth,elevation);
  /*
    Shade image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(linear_image,exception);
  shade_view=AcquireAuthenticCacheView(shade_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(linear_image,shade_image,linear_image->rows,1)
#endif
  for (y=0; y < (ssize_t) linear_image->rows; y++)
  {
    const ShadeImages
      shading = { linear_image, shade_image, gray };

    const ShadeRow
      row = { linear_image,shade_image,light,shading,image_view,shade_view,
        exception,y };

    if (ShadeHeadRow(&status,&row) == MagickFalse)
      continue;
    SyncSharpenRow(&status,shade_view,exception);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportRowProgress(image,ShadeImageTag,progress,&status);
      }
  }
  shade_view=DestroyCacheView(shade_view);
  image_view=DestroyCacheView(image_view);
  linear_image=DestroyImage(linear_image);
  return(ReleaseFailedImage(shade_image,status));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     U n s h a r p M a s k I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnsharpMaskImage() sharpens one or more image channels.  We convolve the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and UnsharpMaskImage() selects a suitable radius for you.
%
%  The format of the UnsharpMaskImage method is:
%
%    Image *UnsharpMaskImage(const Image *image,const double radius,
%      const double sigma,const double amount,const double threshold,
%      ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o gain: the percentage of the difference between the original and the
%      blur image that is added back into the original.
%
%    o threshold: the threshold in pixels needed to apply the difference gain.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline void UnsharpPixelChannels(const UnsharpPixels *unsharp,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = unsharp->image,
    *const unsharp_image = unsharp->unsharp_image;

  const double
    quantum_threshold = unsharp->quantum_threshold,
    gain = unsharp->gain;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    double
      pixel;

    PixelChannel
      channel;

    PixelTrait
      traits,
      unsharp_traits;

    channel=GetPixelChannelChannel(image,i);
    traits=GetPixelChannelTraits(image,channel);
    unsharp_traits=GetPixelChannelTraits(unsharp_image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (unsharp_traits == UndefinedPixelTrait))
      continue;
    if ((unsharp_traits & CopyPixelTrait) != 0)
      {
        SetPixelChannel(unsharp_image,channel,p[i],q);
        continue;
      }
    pixel=(double) p[i]-(double) GetPixelChannel(unsharp_image,channel,q);
    if (fabs(2.0*pixel) < quantum_threshold)
      pixel=(double) p[i];
    else
      pixel=(double) p[i]+gain*pixel;
    SetPixelChannel(unsharp_image,channel,ClampToQuantum(pixel),q);
  }
}

static void UnsharpRow(const UnsharpPixels *unsharp,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = unsharp->image,
    *const unsharp_image = unsharp->unsharp_image;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    UnsharpPixelChannels(unsharp,p,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(unsharp_image);
  }
}

typedef struct _UnsharpHead
{
  UnsharpPixels
    unsharp;

  CacheView
    *image_view,
    *unsharp_view;

  ExceptionInfo
    *exception;
} UnsharpHead;

static MagickBooleanType UnsharpHeadRow(const UnsharpHead *head,const ssize_t y,
  MagickBooleanType *status)
{
  CacheView
    *const image_view = head->image_view,
    *const unsharp_view = head->unsharp_view;

  const Image
    *const image = head->unsharp.image,
    *const unsharp_image = head->unsharp.unsharp_image;

  const Quantum
    *magick_restrict p;

  const UnsharpPixels
    unsharp = head->unsharp;

  ExceptionInfo
    *const exception = head->exception;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
  q=GetCacheViewAuthenticPixels(unsharp_view,0,y,unsharp_image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  UnsharpRow(&unsharp,p,q);
  if (SyncCacheViewAuthenticPixels(unsharp_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

MagickExport Image *UnsharpMaskImage(const Image *image,const double radius,
  const double sigma,const double gain,const double threshold,
  ExceptionInfo *exception)
{
#define SharpenImageTag  "Sharpen/Image"

  CacheView
    *image_view,
    *unsharp_view;

  Image
    *unsharp_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  double
    quantum_threshold;

  ssize_t
    y;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
/* This kernel appears to be broken.
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  unsharp_image=AccelerateUnsharpMaskImage(image,radius,sigma,gain,threshold,
    exception);
  if (unsharp_image != (Image *) NULL)
    return(unsharp_image);
#endif
*/
  unsharp_image=BlurImage(image,radius,sigma,exception);
  if (unsharp_image == (Image *) NULL)
    return((Image *) NULL);
  quantum_threshold=(double) QuantumRange*threshold;
  /*
    Unsharp-mask image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  unsharp_view=AcquireAuthenticCacheView(unsharp_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,unsharp_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const UnsharpHead
      head = { { image, unsharp_image, quantum_threshold, gain }, image_view,
        unsharp_view, exception };

    if (UnsharpHeadRow(&head,y,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportRowProgress(image,SharpenImageTag,progress,&status);
      }
  }
  unsharp_image->type=image->type;
  unsharp_view=DestroyCacheView(unsharp_view);
  image_view=DestroyCacheView(image_view);
  return(ReleaseFailedImage(unsharp_image,status));
}
