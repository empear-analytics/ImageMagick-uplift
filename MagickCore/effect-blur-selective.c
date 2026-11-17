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
%        SSSSS  EEEEE  L      EEEEE   CCCC  TTTTT  IIIII  V   V  EEEEE        %
%        SS     E      L      E      C        T      I    V   V  E            %
%         SSS   EEE    L      EEE    C        T      I    V   V  EEE          %
%           SS  E      L      E      C        T      I     V V   E            %
%        SSSSS  EEEEE  LLLLL  EEEEE   CCCC    T    IIIII    V    EEEEE        %
%                                                                             %
%                                                                             %
%                      MagickCore Selective Blur Method                       %
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
%     S e l e c t i v e B l u r I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SelectiveBlurImage() selectively blur pixels within a contrast threshold.
%  It is similar to the unsharpen mask that sharpens everything with contrast
%  above a certain threshold.
%
%  The format of the SelectiveBlurImage method is:
%
%      Image *SelectiveBlurImage(const Image *image,const double radius,
%        const double sigma,const double threshold,ExceptionInfo *exception)
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
%    o threshold: only pixels within this contrast threshold are included
%      in the blur operation.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsSelectiveRowMissing(const Quantum *p,
  const Quantum *l,const Quantum *q)
{
  return(((p == (const Quantum *) NULL) || (l == (const Quantum *) NULL) ||
    (q == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

typedef struct _SelectiveTap
{
  double
    contrast,
    threshold;

  const MagickRealType
    *k;

  const Quantum
    *pixels;

  ssize_t
    i;
} SelectiveTap;

static void AddSelectiveTap(const SelectiveTap *tap,double *pixel,
  double *gamma)
{
  const double
    contrast = tap->contrast,
    threshold = tap->threshold;

  const MagickRealType
    *magick_restrict const k = tap->k;

  const Quantum
    *magick_restrict const pixels = tap->pixels;

  const ssize_t
    i = tap->i;

  if (fabs(contrast) < threshold)
    {
      *pixel+=(*k)*(double) pixels[i];
      *gamma+=(*k);
    }
}

typedef struct _SelectiveSum
{
  const Image
    *image;

  Image
    *luminance_image;

  size_t
    width;

  double
    intensity,
    threshold;

  ssize_t
    i;
} SelectiveSum;

typedef struct _SelectiveSumTargets
{
  double
    *pixel,
    *gamma;

  const MagickRealType
    *magick_restrict *k;

  const Quantum
    *magick_restrict *pixels,
    *magick_restrict *luminance_pixels;
} SelectiveSumTargets;

static void SumSelectivePixels(const SelectiveSum *sum,
  const SelectiveSumTargets *targets)
{
  double
    *const pixel = targets->pixel,
    *const gamma = targets->gamma;

  const MagickRealType
    *magick_restrict *const k = targets->k;

  const Quantum
    *magick_restrict *const pixels = targets->pixels,
    *magick_restrict *const luminance_pixels = targets->luminance_pixels;

  double
    contrast;

  ssize_t
    u,
    v;

  for (v=0; v < (ssize_t) sum->width; v++)
  {
    for (u=0; u < (ssize_t) sum->width; u++)
    {
      SelectiveTap
        tap;

      contrast=GetPixelIntensity(sum->luminance_image,*luminance_pixels)-
        sum->intensity;
      tap.contrast=contrast;
      tap.threshold=sum->threshold;
      tap.k=*k;
      tap.pixels=*pixels;
      tap.i=sum->i;
      AddSelectiveTap(&tap,pixel,gamma);
      (*k)++;
      *pixels+=(ptrdiff_t) GetPixelChannels(sum->image);
      *luminance_pixels+=(ptrdiff_t) GetPixelChannels(sum->luminance_image);
    }
    *pixels+=(ptrdiff_t) GetPixelChannels(sum->image)*sum->image->columns;
    *luminance_pixels+=(ptrdiff_t) GetPixelChannels(sum->luminance_image)*
      sum->luminance_image->columns;
  }
}

static void SumSelectiveBlendedPixels(const SelectiveSum *sum,
  const SelectiveSumTargets *targets)
{
  double
    alpha,
    contrast;

  ssize_t
    u,
    v;

  for (v=0; v < (ssize_t) sum->width; v++)
  {
    for (u=0; u < (ssize_t) sum->width; u++)
    {
      contrast=GetPixelIntensity(sum->image,*targets->pixels)-sum->intensity;
      if (fabs(contrast) < sum->threshold)
        {
          alpha=QuantumScale*(double) GetPixelAlpha(sum->image,
            *targets->pixels);
          *targets->pixel+=(**targets->k)*alpha*(double)
            (*targets->pixels)[sum->i];
          *targets->gamma+=(**targets->k)*alpha;
        }
      (*targets->k)++;
      *targets->pixels+=(ptrdiff_t) GetPixelChannels(sum->image);
      *targets->luminance_pixels+=(ptrdiff_t) GetPixelChannels(
        sum->luminance_image);
    }
    *targets->pixels+=(ptrdiff_t) GetPixelChannels(sum->image)*
      sum->image->columns;
    *targets->luminance_pixels+=(ptrdiff_t) GetPixelChannels(
      sum->luminance_image)*sum->luminance_image->columns;
  }
}

typedef struct _SelectiveChannelSum
{
  const Image
    *image;

  Image
    *blur_image,
    *luminance_image;

  MagickRealType
    *kernel;

  const Quantum
    *p,
    *l;

  ssize_t
    center;

  double
    intensity,
    threshold;

  size_t
    width;

  PixelChannel
    channel;

  PixelTrait
    blur_traits;
} SelectiveChannelSum;

static void BlurSelectiveChannelSum(const SelectiveChannelSum *blur,
  const ssize_t i,Quantum *magick_restrict q)
{
  double
    gamma,
    pixel;

  const MagickRealType
    *magick_restrict k;

  const Quantum
    *magick_restrict luminance_pixels,
    *magick_restrict pixels;

  SelectiveSum
    sum;

  SelectiveSumTargets
    targets;

  k=blur->kernel;
  pixel=0.0;
  pixels=blur->p;
  luminance_pixels=blur->l;
  gamma=0.0;
  if ((blur->blur_traits & BlendPixelTrait) == 0)
    {
      sum.image=blur->image;
      sum.luminance_image=blur->luminance_image;
      sum.width=blur->width;
      sum.intensity=blur->intensity;
      sum.threshold=blur->threshold;
      sum.i=i;
      targets.pixel=(&pixel);
      targets.gamma=(&gamma);
      targets.k=(&k);
      targets.pixels=(&pixels);
      targets.luminance_pixels=(&luminance_pixels);
      SumSelectivePixels(&sum,&targets);
      if (fabs((double) gamma) < MagickEpsilon)
        {
          SetPixelChannel(blur->blur_image,blur->channel,
            blur->p[blur->center+i],q);
          return;
        }
      gamma=MagickSafeReciprocal(gamma);
      SetPixelChannel(blur->blur_image,blur->channel,ClampToQuantum(
        gamma*pixel),q);
      return;
    }
  sum.image=blur->image;
  sum.luminance_image=blur->luminance_image;
  sum.width=blur->width;
  sum.intensity=blur->intensity;
  sum.threshold=blur->threshold;
  sum.i=i;
  targets.pixel=(&pixel);
  targets.gamma=(&gamma);
  targets.k=(&k);
  targets.pixels=(&pixels);
  targets.luminance_pixels=(&luminance_pixels);
  SumSelectiveBlendedPixels(&sum,&targets);
  if (fabs((double) gamma) < MagickEpsilon)
    {
      SetPixelChannel(blur->blur_image,blur->channel,blur->p[blur->center+i],
        q);
      return;
    }
  gamma=MagickSafeReciprocal(gamma);
  SetPixelChannel(blur->blur_image,blur->channel,ClampToQuantum(gamma*pixel),
    q);
}

typedef struct _SelectiveChannel
{
  const Image
    *image;

  Image
    *blur_image,
    *luminance_image;

  MagickRealType
    *kernel;

  const Quantum
    *p,
    *l;

  ssize_t
    center;

  double
    intensity,
    threshold;

  size_t
    width;
} SelectiveChannel;

static void BlurSelectiveChannel(const SelectiveChannel *selective,
  const ssize_t i,Quantum *magick_restrict q)
{
  PixelChannel
    channel;

  PixelTrait
    blur_traits,
    traits;

  SelectiveChannelSum
    blur;

  channel=GetPixelChannelChannel(selective->image,i);
  traits=GetPixelChannelTraits(selective->image,channel);
  blur_traits=GetPixelChannelTraits(selective->blur_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (blur_traits == UndefinedPixelTrait))
    return;
  if ((blur_traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(selective->blur_image,channel,
        selective->p[selective->center+i],q);
      return;
    }
  blur.image=selective->image;
  blur.blur_image=selective->blur_image;
  blur.luminance_image=selective->luminance_image;
  blur.kernel=selective->kernel;
  blur.p=selective->p;
  blur.l=selective->l;
  blur.center=selective->center;
  blur.intensity=selective->intensity;
  blur.threshold=selective->threshold;
  blur.width=selective->width;
  blur.channel=channel;
  blur.blur_traits=blur_traits;
  BlurSelectiveChannelSum(&blur,i,q);
}

static void BuildSelectiveKernel(MagickRealType *kernel,const size_t width,
  const double sigma)
{
  ssize_t
    i,
    j,
    v;

  j=(ssize_t) (width-1)/2;
  i=0;
  for (v=(-j); v <= j; v++)
  {
    ssize_t
      u;

    for (u=(-j); u <= j; u++)
      kernel[i++]=(MagickRealType) (exp(-((double) u*u+v*v)/(2.0*MagickSigma*
        MagickSigma))/(2.0*MagickPI*MagickSigma*MagickSigma));
  }
}

static void LogSelectiveKernel(MagickRealType *kernel,const size_t width)
{
  char
    format[MagickPathExtent],
    *message;

  const MagickRealType
    *k;

  ssize_t
    u,
    v;

  (void) LogMagickEvent(TransformEvent,GetMagickModule(),
    "  SelectiveBlurImage with %.17gx%.17g kernel:",(double) width,(double)
    width);
  message=AcquireString("");
  k=kernel;
  for (v=0; v < (ssize_t) width; v++)
  {
    *message='\0';
    (void) FormatLocaleString(format,MagickPathExtent,"%.17g: ",(double) v);
    (void) ConcatenateString(&message,format);
    for (u=0; u < (ssize_t) width; u++)
    {
      (void) FormatLocaleString(format,MagickPathExtent,"%+f ",(double)
        *k++);
      (void) ConcatenateString(&message,format);
    }
    (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
  }
  message=DestroyString(message);
}

typedef struct _SelectiveRow
{
  const Image
    *image;

  Image
    *blur_image;

  Image
    *luminance_image;

  CacheView
    *blur_view,
    *image_view,
    *luminance_view;

  MagickRealType
    *kernel;

  ssize_t
    center,
    y;

  double
    threshold;

  size_t
    width;

  ExceptionInfo
    *exception;
} SelectiveRow;

static void BlurSelectiveRow(const SelectiveRow *row,
  const Quantum *magick_restrict p,const Quantum *magick_restrict l,
  Quantum *magick_restrict q)
{
  ssize_t
    x;

  for (x=0; x < (ssize_t) row->image->columns; x++)
  {
    double
      intensity;

    ssize_t
      i;

    intensity=GetPixelIntensity(row->image,p+row->center);
    for (i=0; i < (ssize_t) GetPixelChannels(row->image); i++)
    {
      SelectiveChannel
        selective;

      selective.image=row->image;
      selective.blur_image=row->blur_image;
      selective.luminance_image=row->luminance_image;
      selective.kernel=row->kernel;
      selective.p=p;
      selective.l=l;
      selective.center=row->center;
      selective.intensity=intensity;
      selective.threshold=row->threshold;
      selective.width=row->width;
      BlurSelectiveChannel(&selective,i,q);
    }
    p+=(ptrdiff_t) GetPixelChannels(row->image);
    l+=(ptrdiff_t) GetPixelChannels(row->luminance_image);
    q+=(ptrdiff_t) GetPixelChannels(row->blur_image);
  }
}

static MagickBooleanType BlurSelectiveLine(const SelectiveRow *row,
  MagickBooleanType *status)
{
  MagickBooleanType
    sync;

  const Quantum
    *magick_restrict l,
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(row->image_view,-((ssize_t) (row->width-1)/2L),
    row->y-(ssize_t) ((row->width-1)/2L),row->image->columns+row->width,
    row->width,row->exception);
  l=GetCacheViewVirtualPixels(row->luminance_view,-((ssize_t) (row->width-1)/
    2L),row->y-(ssize_t) ((row->width-1)/2L),row->luminance_image->columns+
    row->width,row->width,row->exception);
  q=QueueCacheViewAuthenticPixels(row->blur_view,0,row->y,
    row->blur_image->columns,1,row->exception);
  if (IsSelectiveRowMissing(p,l,q) != MagickFalse)
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  BlurSelectiveRow(row,p,l,q);
  sync=SyncCacheViewAuthenticPixels(row->blur_view,row->exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static MagickBooleanType AcquireSelectiveLuminance(const Image *image,
  Image **luminance_image,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  (*luminance_image)=CloneImage(image,0,0,MagickTrue,exception);
  if ((*luminance_image) == (Image *) NULL)
    return(MagickFalse);
  status=TransformImageColorspace((*luminance_image),GRAYColorspace,exception);
  if (status == MagickFalse)
    {
      (*luminance_image)=DestroyImage((*luminance_image));
      return(MagickFalse);
    }
  return(MagickTrue);
}

#define SelectiveBlurImageTag  "SelectiveBlur/Image"

typedef struct _SelectiveBlur
{
  const Image
    *image;

  Image
    *blur_image,
    *luminance_image;

  MagickRealType
    *kernel;

  double
    threshold;

  size_t
    width;

  ExceptionInfo
    *exception;
} SelectiveBlur;

static Image *BlurSelectiveRows(const SelectiveBlur *selective)
{
  const Image
    *const image = selective->image;

  Image
    *const blur_image = selective->blur_image;

  Image
    *luminance_image = selective->luminance_image;

  MagickRealType
    *kernel = selective->kernel;

  const double
    threshold = selective->threshold;

  const size_t
    width = selective->width;

  ExceptionInfo
    *const exception = selective->exception;

  CacheView
    *blur_view,
    *image_view,
    *luminance_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    center,
    y;

  status=MagickTrue;
  progress=0;
  center=(ssize_t) (GetPixelChannels(image)*(image->columns+width)*
    ((width-1)/2L)+GetPixelChannels(image)*((width-1)/2L));
  image_view=AcquireVirtualCacheView(image,exception);
  luminance_view=AcquireVirtualCacheView(luminance_image,exception);
  blur_view=AcquireAuthenticCacheView(blur_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,blur_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const SelectiveRow
      row = { image, blur_image, luminance_image, blur_view, image_view,
        luminance_view, kernel, center, y, threshold, width, exception };

    if (BlurSelectiveLine(&row,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetBlurProgress(image,SelectiveBlurImageTag,progress,&status);
      }
  }
  blur_image->type=image->type;
  blur_view=DestroyCacheView(blur_view);
  luminance_view=DestroyCacheView(luminance_view);
  image_view=DestroyCacheView(image_view);
  luminance_image=DestroyImage(luminance_image);
  kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
  return(ReleaseFailedImage(blur_image,status));
}

MagickExport Image *SelectiveBlurImage(const Image *image,const double radius,
  const double sigma,const double threshold,ExceptionInfo *exception)
{
  Image
    *blur_image,
    *luminance_image;

  MagickRealType
    *kernel;

  SelectiveBlur
    selective;

  size_t
    width;

  /*
    Initialize blur image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel=(MagickRealType *) MagickAssumeAligned(AcquireAlignedMemory((size_t)
    width,width*sizeof(*kernel)));
  if (kernel == (MagickRealType *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  BuildSelectiveKernel(kernel,width,sigma);
  if (image->debug != MagickFalse)
    LogSelectiveKernel(kernel,width);
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(blur_image,DirectClass,exception) == MagickFalse)
    {
      blur_image=DestroyImage(blur_image);
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      return((Image *) NULL);
    }
  if (AcquireSelectiveLuminance(image,&luminance_image,exception) ==
      MagickFalse)
    {
      blur_image=DestroyImage(blur_image);
      kernel=(MagickRealType *) RelinquishAlignedMemory(kernel);
      return((Image *) NULL);
    }
  /*
    Threshold blur image.
  */
  selective.image=image;
  selective.blur_image=blur_image;
  selective.luminance_image=luminance_image;
  selective.kernel=kernel;
  selective.threshold=threshold;
  selective.width=width;
  selective.exception=exception;
  return(BlurSelectiveRows(&selective));
}
