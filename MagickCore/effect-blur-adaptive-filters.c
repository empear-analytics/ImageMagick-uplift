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
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R   R                          %
%                         BBBB   L      U   U  RRRR                           %
%                         B   B  L      U   U  R R                            %
%                         BBBB   LLLLL   UUU   R  R                           %
%                                                                             %
%            AAA   DDDD    AAA   PPPP   TTTTT  IIIII  V   V  EEEEE            %
%           A   A  D   D  A   A  P   P    T      I    V   V  E                %
%           AAAAA  D   D  AAAAA  PPPP     T      I    V   V  EEE              %
%           A   A  D   D  A   A  P        T      I     V V   E                %
%           A   A  DDDD   A   A  P        T    IIIII    V    EEEEE            %
%                                                                             %
%               FFFFF  IIIII  L      TTTTT  EEEEE  RRRR   SSSSS               %
%               F        I    L        T    E      R   R  SS                  %
%               FFF      I    L        T    EEE    RRRR    SSS                %
%               F        I    L        T    E      R R       SS               %
%               F      IIIII  LLLLL    T    EEEEE  R  R   SSSSS               %
%                                                                             %
%                                                                             %
%                  MagickCore Image Blur and Sharpen Methods                  %
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
#include "MagickCore/effect-blur-adaptive-filters-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E m b o s s I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EmbossImage() returns a grayscale image with a three-dimensional effect.
%  We convolve the image with a Gaussian operator of the given radius and
%  standard deviation (sigma).  For reasonable results, radius should be
%  larger than sigma.  Use a radius of 0 and Emboss() selects a suitable
%  radius for you.
%
%  The format of the EmbossImage method is:
%
%      Image *EmbossImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void NormalizeKernelValues(KernelInfo *kernel_info)
{
  double
    gamma,
    normalize;

  ssize_t
    i;

  normalize=0.0;
  for (i=0; i < (ssize_t) (kernel_info->width*kernel_info->height); i++)
    normalize+=kernel_info->values[i];
  gamma=MagickSafeReciprocal(normalize);
  for (i=0; i < (ssize_t) (kernel_info->width*kernel_info->height); i++)
    kernel_info->values[i]*=gamma;
}

static void SetEmbossKernelValues(KernelInfo *kernel_info,const double sigma)
{
  ssize_t
    i;

  ssize_t
    j,
    k,
    u,
    v;

  j=(ssize_t) (kernel_info->width-1)/2;
  k=j;
  i=0;
  for (v=(-j); v <= j; v++)
  {
    for (u=(-j); u <= j; u++)
    {
      kernel_info->values[i]=(MagickRealType) (((u < 0) || (v < 0) ? -8.0 :
        8.0)*exp(-((double) u*u+v*v)/(2.0*MagickSigma*MagickSigma))/
        (2.0*MagickPI*MagickSigma*MagickSigma));
      if (u != k)
        kernel_info->values[i]=0.0;
      i++;
    }
    k--;
  }
}

MagickExport Image *EmbossImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *emboss_image;

  KernelInfo
    *kernel_info;

  size_t
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel_info=AcquireKernelInfo((const char *) NULL,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  kernel_info->width=width;
  kernel_info->height=width;
  kernel_info->x=(ssize_t) (width-1)/2;
  kernel_info->y=(ssize_t) (width-1)/2;
  kernel_info->values=(MagickRealType *) MagickAssumeAligned(
    AcquireAlignedMemory(kernel_info->width,kernel_info->width*
    sizeof(*kernel_info->values)));
  if (kernel_info->values == (MagickRealType *) NULL)
    {
      kernel_info=DestroyKernelInfo(kernel_info);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  SetEmbossKernelValues(kernel_info,sigma);
  NormalizeKernelValues(kernel_info);
  emboss_image=ConvolveImage(image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  if (emboss_image != (Image *) NULL)
    (void) EqualizeImage(emboss_image,exception);
  return(emboss_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o t i o n B l u r I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MotionBlurImage() simulates motion blur.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and MotionBlurImage() selects a suitable radius for you.
%  Angle gives the angle of the blurring motion.
%
%  Andrew Protano contributed this effect.
%
%  The format of the MotionBlurImage method is:
%
%    Image *MotionBlurImage(const Image *image,const double radius,
%      const double sigma,const double angle,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting
%      the center pixel.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o angle: Apply the effect along this angle.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickRealType *GetMotionBlurKernel(const size_t width,
  const double sigma)
{
  MagickRealType
    *kernel,
    normalize;

  ssize_t
    i;

  /*
   Generate a 1-D convolution kernel.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  kernel=(MagickRealType *) MagickAssumeAligned(AcquireAlignedMemory((size_t)
    width,sizeof(*kernel)));
  if (kernel == (MagickRealType *) NULL)
    return(kernel);
  normalize=0.0;
  for (i=0; i < (ssize_t) width; i++)
  {
    kernel[i]=(MagickRealType) (exp((-((double) i*i)/(double) (2.0*MagickSigma*
      MagickSigma)))/(MagickSQ2PI*MagickSigma));
    normalize+=kernel[i];
  }
  for (i=0; i < (ssize_t) width; i++)
    kernel[i]/=normalize;
  return(kernel);
}

typedef struct _MotionSum
{
  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ssize_t
    x,
    y,
    i;

  ExceptionInfo
    *exception;
} MotionSum;

static MagickBooleanType SumMotionPixels(const MotionSum *sum,double *pixel,
  MagickRealType *magick_restrict *k)
{
  CacheView
    *const motion_view = sum->motion_view;

  OffsetInfo
    *const offset = sum->offset;

  const size_t
    width = sum->width;

  const ssize_t
    x = sum->x,
    y = sum->y,
    i = sum->i;

  ExceptionInfo
    *const exception = sum->exception;

  const Quantum
    *magick_restrict r;

  MagickBooleanType
    status;

  ssize_t
    j;

  status=MagickTrue;
  for (j=0; j < (ssize_t) width; j++)
  {
    r=GetCacheViewVirtualPixels(motion_view,x+offset[j].x,y+
      offset[j].y,1,1,exception);
    if (r == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    *pixel+=(**k)*(double) r[i];
    (*k)++;
  }
  return(status);
}

typedef struct _MotionBlendedSum
{
  const Image
    *image;

  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ssize_t
    x,
    y,
    i;

  ExceptionInfo
    *exception;
} MotionBlendedSum;

static MagickBooleanType SumMotionBlendedPixels(const MotionBlendedSum *sum,
  double *pixel,double *gamma,MagickRealType *magick_restrict *k)
{
  const Quantum
    *magick_restrict r;

  double
    alpha = 0.0;

  MagickBooleanType
    status;

  ssize_t
    j;

  status=MagickTrue;
  for (j=0; j < (ssize_t) sum->width; j++)
  {
    r=GetCacheViewVirtualPixels(sum->motion_view,sum->x+sum->offset[j].x,
      sum->y+sum->offset[j].y,1,1,sum->exception);
    if (r == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    alpha=QuantumScale*(double) GetPixelAlpha(sum->image,r);
    *pixel+=(**k)*alpha*(double) r[sum->i];
    *gamma+=(**k)*alpha;
    (*k)++;
  }
  return(status);
}

typedef struct _MotionChannel
{
  const Image
    *image;

  Image
    *blur_image;

  const Quantum
    *p;

  MagickRealType
    *kernel;

  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ssize_t
    x,
    y;

  ExceptionInfo
    *exception;
} MotionChannel;

static MagickBooleanType BlurMotionChannel(const MotionChannel *motion,
  const ssize_t i,Quantum *magick_restrict q)
{
  double
    gamma = 0.0,
    pixel;

  MagickBooleanType
    status;

  MotionBlendedSum
    blended_sum;

  MotionSum
    sum;

  PixelChannel
    channel;

  PixelTrait
    blur_traits,
    traits;

  MagickRealType
    *magick_restrict k;

  status=MagickTrue;
  channel=GetPixelChannelChannel(motion->image,i);
  traits=GetPixelChannelTraits(motion->image,channel);
  blur_traits=GetPixelChannelTraits(motion->blur_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (blur_traits == UndefinedPixelTrait))
    return(status);
  if ((blur_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(motion->blur_image,channel,motion->p[i],q);
      return(status);
    }
  k=motion->kernel;
  pixel=0.0;
  if ((blur_traits & BlendPixelTrait) == 0)
    {
      sum.motion_view=motion->motion_view;
      sum.offset=motion->offset;
      sum.width=motion->width;
      sum.x=motion->x;
      sum.y=motion->y;
      sum.i=i;
      sum.exception=motion->exception;
      if (SumMotionPixels(&sum,&pixel,&k) == MagickFalse)
        status=MagickFalse;
      SetPixelChannel(motion->blur_image,channel,ClampToQuantum(pixel),q);
      return(status);
    }
  blended_sum.image=motion->image;
  blended_sum.motion_view=motion->motion_view;
  blended_sum.offset=motion->offset;
  blended_sum.width=motion->width;
  blended_sum.x=motion->x;
  blended_sum.y=motion->y;
  blended_sum.i=i;
  blended_sum.exception=motion->exception;
  if (SumMotionBlendedPixels(&blended_sum,&pixel,&gamma,&k) == MagickFalse)
    status=MagickFalse;
  gamma=MagickSafeReciprocal(gamma);
  SetPixelChannel(motion->blur_image,channel,ClampToQuantum(gamma*pixel),q);
  return(status);
}

static MagickBooleanType BlurMotionPixel(const MotionChannel *pixel_motion,
  Quantum *magick_restrict q)
{
  MagickBooleanType
    status;

  ssize_t
    i;

  status=MagickTrue;
  for (i=0; i < (ssize_t) GetPixelChannels(pixel_motion->image); i++)
  {
    MotionChannel
      motion;

    motion.image=pixel_motion->image;
    motion.blur_image=pixel_motion->blur_image;
    motion.p=pixel_motion->p;
    motion.kernel=pixel_motion->kernel;
    motion.motion_view=pixel_motion->motion_view;
    motion.offset=pixel_motion->offset;
    motion.width=pixel_motion->width;
    motion.x=pixel_motion->x;
    motion.y=pixel_motion->y;
    motion.exception=pixel_motion->exception;
    if (BlurMotionChannel(&motion,i,q) == MagickFalse)
      status=MagickFalse;
  }
  return(status);
}

typedef struct _MotionRow
{
  const Image
    *image;

  Image
    *blur_image;

  MagickRealType
    *kernel;

  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} MotionRow;

static MagickBooleanType BlurMotionRow(const MotionRow *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *const image = row->image;

  Image
    *const blur_image = row->blur_image;

  MagickRealType
    *const kernel = row->kernel;

  CacheView
    *const motion_view = row->motion_view;

  OffsetInfo
    *const offset = row->offset;

  const size_t
    width = row->width;

  const ssize_t
    y = row->y;

  ExceptionInfo
    *const exception = row->exception;

  MagickBooleanType
    status;

  ssize_t
    x;

  status=MagickTrue;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    MotionChannel
      pixel_motion;

    pixel_motion.image=image;
    pixel_motion.blur_image=blur_image;
    pixel_motion.p=p;
    pixel_motion.kernel=kernel;
    pixel_motion.motion_view=motion_view;
    pixel_motion.offset=offset;
    pixel_motion.width=width;
    pixel_motion.x=x;
    pixel_motion.y=y;
    pixel_motion.exception=exception;
    if (BlurMotionPixel(&pixel_motion,q) == MagickFalse)
      status=MagickFalse;
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(blur_image);
  }
  return(status);
}

static MagickBooleanType BlurMotionLine(const MotionRow *row,
  CacheView *image_view,CacheView *blur_view,MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,0,row->y,row->image->columns,1,
    row->exception);
  q=QueueCacheViewAuthenticPixels(blur_view,0,row->y,row->blur_image->columns,
    1,row->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  if (BlurMotionRow(row,p,q) == MagickFalse)
    (*status)=MagickFalse;
  if (SyncCacheViewAuthenticPixels(blur_view,row->exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static void FillMotionOffsets(OffsetInfo *offset,const size_t width,
  const double angle)
{
  PointInfo
    point;

  ssize_t
    w;

  point.x=(double) width*sin(DegreesToRadians(angle));
  point.y=(double) width*cos(DegreesToRadians(angle));
  for (w=0; w < (ssize_t) width; w++)
  {
    offset[w].x=CastDoubleToSsizeT(ceil((double) (w*point.y)/
      hypot(point.x,point.y)-0.5));
    offset[w].y=CastDoubleToSsizeT(ceil((double) (w*point.x)/
      hypot(point.x,point.y)-0.5));
  }
}

static void RelinquishMotionTables(MagickRealType *kernel,OffsetInfo *offset)
{
  kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
  offset=(OffsetInfo *) RelinquishMagickMemory(offset);
}

#define BlurImageTag  "Blur/Image"

typedef struct _MotionBlur
{
  const Image
    *image;

  Image
    *blur_image;

  MagickRealType
    *kernel;

  CacheView
    *motion_view;

  OffsetInfo
    *offset;

  size_t
    width;

  ExceptionInfo
    *exception;

  CacheView
    *image_view,
    *blur_view;
} MotionBlur;

static MagickBooleanType BlurMotionRows(const MotionBlur *motion,
  MagickBooleanType status,MagickOffsetType progress)
{
  const Image
    *const image = motion->image;

  Image
    *const blur_image = motion->blur_image;

  MagickRealType
    *const kernel = motion->kernel;

  CacheView
    *const motion_view = motion->motion_view;

  OffsetInfo
    *const offset = motion->offset;

  const size_t
    width = motion->width;

  ExceptionInfo
    *const exception = motion->exception;

  CacheView
    *const image_view = motion->image_view,
    *const blur_view = motion->blur_view;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,blur_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const MotionRow
      row = { image, blur_image, kernel, motion_view, offset, width, y,
        exception };

    if (BlurMotionLine(&row,image_view,blur_view,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetBlurProgress(image,BlurImageTag,progress,&status);
      }
  }
  return(status);
}

typedef struct _MotionBlurTail
{
  const Image
    *image;

  Image
    *blur_image;

  MagickRealType
    *kernel;

  OffsetInfo
    *offset;

  size_t
    width;

  ExceptionInfo
    *exception;
} MotionBlurTail;

static Image *BlurMotionImage(const MotionBlurTail *tail)
{
  const Image
    *const image = tail->image;

  Image
    *const blur_image = tail->blur_image;

  MagickRealType
    *const kernel = tail->kernel;

  OffsetInfo
    *const offset = tail->offset;

  const size_t
    width = tail->width;

  ExceptionInfo
    *const exception = tail->exception;

  CacheView
    *blur_view,
    *image_view,
    *motion_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MotionBlur
    motion;

  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  motion_view=AcquireVirtualCacheView(image,exception);
  blur_view=AcquireAuthenticCacheView(blur_image,exception);
  motion.image=image;
  motion.blur_image=blur_image;
  motion.kernel=kernel;
  motion.motion_view=motion_view;
  motion.offset=offset;
  motion.width=width;
  motion.exception=exception;
  motion.image_view=image_view;
  motion.blur_view=blur_view;
  status=BlurMotionRows(&motion,status,progress);
  blur_view=DestroyCacheView(blur_view);
  motion_view=DestroyCacheView(motion_view);
  image_view=DestroyCacheView(image_view);
  RelinquishMotionTables(kernel,offset);
  return(ReleaseFailedImage(blur_image,status));
}

MagickExport Image *MotionBlurImage(const Image *image,const double radius,
  const double sigma,const double angle,ExceptionInfo *exception)
{
  Image
    *blur_image;

  MagickRealType
    *kernel;

  MotionBlurTail
    tail;

  OffsetInfo
    *offset;

  size_t
    width;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel=GetMotionBlurKernel(width,sigma);
  if (kernel == (MagickRealType *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  offset=(OffsetInfo *) AcquireQuantumMemory(width,sizeof(*offset));
  if (offset == (OffsetInfo *) NULL)
    {
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  FillMotionOffsets(offset,width,angle);
  /*
    Motion blur image.
  */
#if defined(MAGICKCORE_OPENCL_SUPPORT)
  blur_image=AccelerateMotionBlurImage(image,kernel,width,offset,exception);
  if (blur_image != (Image *) NULL)
    {
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      offset=(OffsetInfo *) RelinquishMagickMemory(offset);
      return(blur_image);
    }
#endif
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    {
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      offset=(OffsetInfo *) RelinquishMagickMemory(offset);
      return((Image *) NULL);
    }
  if (SetImageStorageClass(blur_image,DirectClass,exception) == MagickFalse)
    {
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      offset=(OffsetInfo *) RelinquishMagickMemory(offset);
      blur_image=DestroyImage(blur_image);
      return((Image *) NULL);
    }
  tail.image=image;
  tail.blur_image=blur_image;
  tail.kernel=kernel;
  tail.offset=offset;
  tail.width=width;
  tail.exception=exception;
  return(BlurMotionImage(&tail));
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h a r p e n I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SharpenImage() sharpens the image.  We convolve the image with a Gaussian
%  operator of the given radius and standard deviation (sigma).  For
%  reasonable results, radius should be larger than sigma.  Use a radius of 0
%  and SharpenImage() selects a suitable radius for you.
%
%  Using a separable kernel would be faster, but the negative weights cancel
%  out on the corners of the kernel producing often undesirable ringing in the
%  filtered result; this can be avoided by using a 2D gaussian shaped image
%  sharpening kernel instead.
%
%  The format of the SharpenImage method is:
%
%    Image *SharpenImage(const Image *image,const double radius,
%      const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: the standard deviation of the Laplacian, in pixels.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *SharpenImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  double
    normalize;

  Image
    *sharp_image;

  KernelInfo
    *kernel_info;

  ssize_t
    i;

  size_t
    width;

  ssize_t
    j,
    u,
    v;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel_info=AcquireKernelInfo((const char *) NULL,exception);
  if (kernel_info == (KernelInfo *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  (void) memset(kernel_info,0,sizeof(*kernel_info));
  kernel_info->width=width;
  kernel_info->height=width;
  kernel_info->x=(ssize_t) (width-1)/2;
  kernel_info->y=(ssize_t) (width-1)/2;
  kernel_info->signature=MagickCoreSignature;
  kernel_info->values=(MagickRealType *) MagickAssumeAligned(
    AcquireAlignedMemory(kernel_info->width,kernel_info->height*
    sizeof(*kernel_info->values)));
  if (kernel_info->values == (MagickRealType *) NULL)
    {
      kernel_info=DestroyKernelInfo(kernel_info);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  normalize=0.0;
  j=(ssize_t) (kernel_info->width-1)/2;
  i=0;
  for (v=(-j); v <= j; v++)
  {
    for (u=(-j); u <= j; u++)
    {
      kernel_info->values[i]=(MagickRealType) (-exp(-((double) u*u+v*v)/(2.0*
        MagickSigma*MagickSigma))/(2.0*MagickPI*MagickSigma*MagickSigma));
      normalize+=kernel_info->values[i];
      i++;
    }
  }
  kernel_info->values[i/2]=(double) ((-2.0)*normalize);
  NormalizeKernelValues(kernel_info);
  sharp_image=ConvolveImage(image,kernel_info,exception);
  kernel_info=DestroyKernelInfo(kernel_info);
  return(sharp_image);
}
