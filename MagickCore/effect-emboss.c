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
%                  EEEEE  M   M  BBBB    OOO   SSSSS  SSSSS                   %
%                  E      MM MM  B   B  O   O  SS     SS                      %
%                  EEE    M M M  BBBB   O   O   SSS    SSS                    %
%                  E      M   M  B   B  O   O     SS     SS                   %
%                  EEEEE  M   M  BBBB    OOO   SSSSS  SSSSS                   %
%                                                                             %
%                                                                             %
%                    MagickCore Emboss and Sharpen Methods                    %
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
