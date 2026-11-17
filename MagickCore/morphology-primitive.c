/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    M   M   OOO   RRRR   PPPP   H   H   OOO   L       OOO    GGGG  Y   Y     %
%    MM MM  O   O  R   R  P   P  H   H  O   O  L      O   O  G       Y Y      %
%    M M M  O   O  RRRR   PPPP   HHHHH  O   O  L      O   O  G  GG    Y       %
%    M   M  O   O  R R    P      H   H  O   O  L      O   O  G   G    Y       %
%    M   M   OOO   R  R   P      H   H   OOO   LLLLL   OOO    GGG     Y       %
%                                                                             %
%        PPPP   RRRR   IIIII  M   M  IIIII  TTTTT  IIIII  V   V  EEEEE        %
%        P   P  R   R    I    MM MM    I      T      I    V   V  E            %
%        PPPP   RRRR     I    M M M    I      T      I    V   V  EEE          %
%        P      R R      I    M   M    I      T      I     V V   E            %
%        P      R  R   IIIII  M   M  IIIII    T    IIIII    V    EEEEE        %
%                                                                             %
%                                                                             %
%                  MagickCore Morphology Application Methods                  %
%                                                                             %
%                              Software Design                                %
%                              Anthony Thyssen                                %
%                               January 2010                                  %
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
% Morphology is the application of various kernels, of any size or shape, to an
% image in various ways (typically binary, but not always).
%
% Convolution (weighted sum or average) is just one specific type of
% morphology. Just one that is very common for image blurring and sharpening
% effects.  Not only 2D Gaussian blurring, but also 2-pass 1D Blurring.
%
% This module provides not only a general morphology function, and the ability
% to apply more advanced or iterative morphologies, but also functions for the
% generation of many different types of kernel arrays from user supplied
% arguments. Prehaps even the generation of a kernel from a small image.
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/morphology.h"
#include "MagickCore/morphology-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/prepress.h"
#include "MagickCore/quantize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/registry.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/morphology-apply-private.h"

/*
  Typedef declarations.
*/
typedef struct _MorphologyPass
{
  MorphologyMethod
    method;

  const KernelInfo
    *kernel;

  double
    bias;
} MorphologyPass;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o r p h o l o g y A p p l y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MorphologyApply() applies a morphological method, multiple times using
%  a list of multiple kernels.  This is the method that should be called by
%  other 'operators' that internally use morphology operations as part of
%  their processing.
%
%  It is basically equivalent to as MorphologyImage() (see below) but without
%  any user controls.  This allows internal programs to use this method to
%  perform a specific task without possible interference by any API user
%  supplied settings.
%
%  It is MorphologyImage() task to extract any such user controls, and
%  pass them to this function for processing.
%
%  More specifically all given kernels should already be scaled, normalised,
%  and blended appropriately before being parred to this routine. The
%  appropriate bias, and compose (typically 'UndefinedComposeOp') given.
%
%  The format of the MorphologyApply method is:
%
%      Image *MorphologyApply(const Image *image,MorphologyMethod method,
%        const ssize_t iterations,const KernelInfo *kernel,
%        const CompositeMethod compose,const double bias,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the source image
%
%    o method: the morphology method to be applied.
%
%    o iterations: apply the operation this many times (or no change).
%                  A value of -1 means loop until no change found.
%                  How this is applied may depend on the morphology method.
%                  Typically this is a value of 1.
%
%    o channel: the channel type.
%
%    o kernel: An array of double representing the morphology kernel.
%
%    o compose: How to handle or merge multi-kernel results.
%          If 'UndefinedCompositeOp' use default for the Morphology method.
%          If 'NoCompositeOp' force image to be re-iterated by each kernel.
%          Otherwise merge the results using the compose method given.
%
%    o bias: Convolution Output Bias.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static double MinimizeChannelValue(const Quantum *magick_restrict pixels,
  const ssize_t i,double minimum)
{
  if ((double) pixels[i] < minimum)
    minimum=(double) pixels[i];
  return(minimum);
}

static double MaximizeChannelValue(const Quantum *magick_restrict pixels,
  const ssize_t i,double maximum)
{
  if ((double) pixels[i] > maximum)
    maximum=(double) pixels[i];
  return(maximum);
}

static double MinimizeKernelDistance(const MagickRealType *magick_restrict k,
  const Quantum *magick_restrict pixels,const ssize_t i,double pixel)
{
  if (!IsNaN(*k))
    {
      if (((double) pixels[i]+(*k)) < pixel)
        pixel=(double) pixels[i]+(*k);
    }
  return(pixel);
}

typedef struct _MorphologyKernelTap
{
  const MagickRealType
    *k;

  const Quantum
    *pixels;

  ssize_t
    i;
} MorphologyKernelTap;

static void UpdateHitAndMissExtrema(const MorphologyKernelTap *tap,
  double *minimum,double *maximum)
{
  if (*tap->k > 0.7)
    (*minimum)=MinimizeChannelValue(tap->pixels,tap->i,(*minimum));
  else
    if (*tap->k < 0.3)
      (*maximum)=MaximizeChannelValue(tap->pixels,tap->i,(*maximum));
}

typedef struct _MorphologyIntensityTap
{
  const Quantum
    *pixels;

  double
    intensity;

  ssize_t
    i;
} MorphologyIntensityTap;

static void SelectMinimumIntensityPixel(const MorphologyIntensityTap *tap,
  const Quantum *magick_restrict *quantum_pixels,double *pixel,double *minimum)
{
  if (tap->intensity < (*minimum))
    {
      (*quantum_pixels)=tap->pixels;
      (*pixel)=(double) tap->pixels[tap->i];
      (*minimum)=tap->intensity;
    }
}

static void SelectMaximumIntensityPixel(const MorphologyIntensityTap *tap,
  double *pixel,const Quantum *magick_restrict *quantum_pixels,double *maximum)
{
  if (tap->intensity > (*maximum))
    {
      (*pixel)=(double) tap->pixels[tap->i];
      (*quantum_pixels)=tap->pixels;
      (*maximum)=tap->intensity;
    }
}

static double ConvolveChannelValue(const MagickRealType *magick_restrict k,
  const Quantum *magick_restrict pixels,const ssize_t i,double pixel)
{
  if (!IsNaN(*k))
    pixel+=(*k)*(double) pixels[i];
  return(pixel);
}

typedef struct _MorphologyKernelWalk
{
  MorphologyMethod
    method;

  const Image
    *image;

  PixelTrait
    morphology_traits;

  const KernelInfo
    *kernel;

  const Quantum
    *magick_restrict p,
    *magick_restrict pixels;

  double
    bias,
    maximum,
    minimum;

  ssize_t
    center,
    i;
} MorphologyKernelWalk;

typedef enum
{
  ForwardKernelWalk,
  ReflectedKernelWalk
} MorphologyKernelWalkSide;

typedef double
  (*MorphologyExtremumMethod)(const Quantum *magick_restrict,const ssize_t,
    double);

static inline double WalkErodeOrDilateKernel(const MorphologyKernelWalk *walk,
  const MorphologyKernelWalkSide side,const MorphologyExtremumMethod method,
  double pixel)
{
  const Image
    *image = walk->image;

  const KernelInfo
    *kernel = walk->kernel;

  const Quantum
    *magick_restrict pixels = walk->pixels;

  const ssize_t
    i = walk->i;

  const MagickRealType
    *magick_restrict k;

  ssize_t
    u,
    v;

  k=((side == ForwardKernelWalk) ? kernel->values :
    (&kernel->values[kernel->width*kernel->height-1]));
  for (v=0; v < (ssize_t) kernel->height; v++)
  {
    for (u=0; u < (ssize_t) kernel->width; u++)
    {
      if (!IsNaN(*k) && ((side == ForwardKernelWalk) ? (*k >= 0.5) :
          (*k > 0.5)))
        {
          pixel=method(pixels,i,pixel);
        }
      ((side == ForwardKernelWalk) ? k++ : k--);
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(image->columns-1)*GetPixelChannels(image);
  }
  return(pixel);
}

typedef struct _MorphologyIntensityWalk
{
  MorphologyKernelWalkSide
    side;

  double
    *extremum;
} MorphologyIntensityWalk;

static inline void WalkIntensityKernel(const MorphologyKernelWalk *walk,
  const MorphologyIntensityWalk *select,
  const Quantum *magick_restrict *quantum_pixels,double *pixel)
{
  const MorphologyKernelWalkSide
    side = select->side;

  double
    *const extremum = select->extremum;

  const Image
    *image = walk->image;

  const KernelInfo
    *kernel = walk->kernel;

  const Quantum
    *magick_restrict pixels = walk->pixels;

  const ssize_t
    i = walk->i;

  double
    intensity;

  const MagickRealType
    *magick_restrict k;

  MorphologyIntensityTap
    intensity_tap;

  ssize_t
    u,
    v;

  k=((side == ForwardKernelWalk) ? kernel->values :
    (&kernel->values[kernel->width*kernel->height-1]));
  for (v=0; v < (ssize_t) kernel->height; v++)
  {
    for (u=0; u < (ssize_t) kernel->width; u++)
    {
      if (!IsNaN(*k) && (*k >= 0.5))
        {
          intensity=(double) GetPixelIntensity(image,pixels);
          intensity_tap.pixels=pixels;
          intensity_tap.intensity=intensity;
          intensity_tap.i=i;
          ((side == ForwardKernelWalk) ?
            SelectMinimumIntensityPixel(&intensity_tap,quantum_pixels,
              pixel,extremum) :
            SelectMaximumIntensityPixel(&intensity_tap,pixel,
              quantum_pixels,extremum));
        }
      ((side == ForwardKernelWalk) ? k++ : k--);
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(image->columns-1)*GetPixelChannels(image);
  }
}

static void WalkHitAndMissKernel(const MorphologyKernelWalk *walk,
  double *minimum,double *maximum)
{
  const Image
    *image = walk->image;

  const KernelInfo
    *kernel = walk->kernel;

  const Quantum
    *magick_restrict pixels = walk->pixels;

  const ssize_t
    i = walk->i;

  const MagickRealType
    *magick_restrict k;

  MorphologyKernelTap
    kernel_tap;

  ssize_t
    u,
    v;

  k=kernel->values;
  for (v=0; v < (ssize_t) kernel->height; v++)
  {
    for (u=0; u < (ssize_t) kernel->width; u++)
    {
      if (!IsNaN(*k))
        {
          kernel_tap.k=k;
          kernel_tap.pixels=pixels;
          kernel_tap.i=i;
          UpdateHitAndMissExtrema(&kernel_tap,minimum,maximum);
        }
      k++;
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(image->columns-1)*GetPixelChannels(image);
  }
}

typedef double
  (*MorphologyKernelMethod)(const MagickRealType *magick_restrict,
    const Quantum *magick_restrict,const ssize_t,double);

static inline double WalkReflectedKernel(const MorphologyKernelWalk *walk,
  const MorphologyKernelMethod method,const MagickRealType *magick_restrict k,
  double pixel)
{
  const Image
    *image = walk->image;

  const KernelInfo
    *kernel = walk->kernel;

  const Quantum
    *magick_restrict pixels = walk->pixels;

  const ssize_t
    i = walk->i;

  ssize_t
    u,
    v;

  for (v=0; v < (ssize_t) kernel->height; v++)
  {
    for (u=0; u < (ssize_t) kernel->width; u++)
    {
      pixel=method(k,pixels,i,pixel);
      k--;
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(image->columns-1)*GetPixelChannels(image);
  }
  return(pixel);
}

static void BlendReflectedKernel(const MorphologyKernelWalk *walk,
  const MagickRealType *magick_restrict k,double *pixel,double *gamma)
{
  const Image
    *image = walk->image;

  const KernelInfo
    *kernel = walk->kernel;

  const Quantum
    *magick_restrict pixels = walk->pixels;

  const ssize_t
    i = walk->i;

  double
    alpha;

  ssize_t
    u,
    v;

  for (v=0; v < (ssize_t) kernel->height; v++)
  {
    for (u=0; u < (ssize_t) kernel->width; u++)
    {
      if (!IsNaN(*k))
        {
          alpha=(double) (QuantumScale*(double)
            GetPixelAlpha(image,pixels));
          (*pixel)+=alpha*(*k)*(double) pixels[i];
          (*gamma)+=alpha*(*k);
        }
      k--;
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(image->columns-1)*GetPixelChannels(image);
  }
}

static double GetMorphologyInitialPixel(const MorphologyKernelWalk *walk)
{
  const MorphologyMethod
    method = walk->method;

  const Quantum
    *magick_restrict p = walk->p;

  const double
    bias = walk->bias;

  const ssize_t
    center = walk->center,
    i = walk->i;

  double
    pixel;

  switch (method)
  {
    case ConvolveMorphology:
    {
      pixel=bias;
      break;
    }
    case DilateMorphology:
    case ErodeIntensityMorphology:
    {
      pixel=0.0;
      break;
    }
    default:
    {
      pixel=(double) p[center+i];
      break;
    }
  }
  return(pixel);
}

static void ApplyMorphologyIntensity(const MorphologyKernelWalk *walk,
  const Quantum *magick_restrict *quantum_pixels,double *pixel)
{
  const MorphologyMethod
    method = walk->method;

  const KernelInfo
    *kernel = walk->kernel;

  double
    maximum = walk->maximum,
    minimum = walk->minimum;

  const MagickRealType
    *magick_restrict k;

  switch (method)
  {
    case ErodeIntensityMorphology:
    {
      const MorphologyIntensityWalk
        select = { ForwardKernelWalk, &minimum };

      /*
        Select (*pixel) with minimum intensity within kernel neighbourhood.

        The kernel is not reflected for this operation.
      */
      WalkIntensityKernel(walk,&select,quantum_pixels,pixel);
      break;
    }
    case DilateIntensityMorphology:
    {
      const MorphologyIntensityWalk
        select = { ReflectedKernelWalk, &maximum };

      /*
        Select (*pixel) with maximum intensity within kernel neighbourhood.

        The kernel is not reflected for this operation.
      */
      WalkIntensityKernel(walk,&select,quantum_pixels,pixel);
      break;
    }
    case IterativeDistanceMorphology:
    {
      /*
         Compute th iterative distance from black edge of a white image
         shape.  Essentially white values are decreased to the smallest
         'distance from edge' it can find.

         It works by adding kernel values to the neighbourhood, and
         select the minimum value found. The kernel is rotated before
         use, so kernel distances match resulting distances, when a user
         provided asymmetric kernel is applied.

         This code is nearly identical to True GrayScale Morphology but
         not quite.

         GreyDilate Kernel values added, maximum value found Kernel is
         rotated before use.

         GrayErode:  Kernel values subtracted and minimum value found No
         kernel rotation used.

         Note the Iterative Distance method is essentially a
         GrayErode, but with negative kernel values, and kernel rotation
         applied.
      */
      k=(&kernel->values[kernel->width*kernel->height-1]);
      (*pixel)=WalkReflectedKernel(walk,MinimizeKernelDistance,k,(*pixel));
      break;
    }
    case UndefinedMorphology:
    default:
      break;
  }
}

static void ApplyMorphologyHitAndMiss(const MorphologyKernelWalk *walk,
  const Quantum *magick_restrict *quantum_pixels,double *pixel)
{
  const MorphologyMethod
    method = walk->method;

  const Quantum
    *magick_restrict p = walk->p;

  const ssize_t
    center = walk->center,
    i = walk->i;

  double
    maximum = walk->maximum,
    minimum = walk->minimum;

  switch (method)
  {
    case HitAndMissMorphology:
    case ThinningMorphology:
    case ThickenMorphology:
    {
      /*
         Minimum of foreground (*pixel) minus maximum of background pixels.

         The kernel is not reflected for this operation, and consists
         of both foreground and background (*pixel) neighbourhoods, 0.0 for
         background, and 1.0 for foreground with either Nan or 0.5 values
         for don't care.

         This never produces a meaningless negative result.  Such results
         cause Thinning/Thicken to not work correctly when used against a
         greyscale image.
      */
      WalkHitAndMissKernel(walk,&minimum,&maximum);
      minimum-=maximum;
      if (minimum < 0.0)
        minimum=0.0;
      (*pixel)=minimum;
      if (method == ThinningMorphology)
        (*pixel)=(double) p[center+i]-minimum;
      else
        if (method == ThickenMorphology)
          (*pixel)=(double) p[center+i]+minimum;
      break;
    }
    default:
    {
      ApplyMorphologyIntensity(walk,quantum_pixels,pixel);
      break;
    }
  }
}

static void ApplyMorphologyMethod(const MorphologyKernelWalk *walk,
  double *pixel,double *gamma,const Quantum *magick_restrict *quantum_pixels)
{
  const MorphologyMethod
    method = walk->method;

  const Image
    *image = walk->image;

  const PixelTrait
    morphology_traits = walk->morphology_traits;

  const KernelInfo
    *kernel = walk->kernel;

  const MagickRealType
    *magick_restrict k;

  switch (method)
  {
    case ConvolveMorphology:
    {
      /*
         Weighted Average of pixels using reflected kernel

         For correct working of this operation for asymmetrical kernels,
         the kernel needs to be applied in its reflected form.  That is
         its values needs to be reversed.

         Correlation is actually the same as this but without reflecting
         the kernel, and thus 'lower-level' that Convolution.  However as
         Convolution is the more common method used, and it does not
         really cost us much in terms of processing to use a reflected
         kernel, so it is Convolution that is implemented.

         Correlation will have its kernel reflected before calling this
         function to do a Convolve.

         For more details of Correlation vs Convolution see
           http://www.cs.umd.edu/~djacobs/CMSC426/Convolution.pdf
      */
      k=(&kernel->values[kernel->width*kernel->height-1]);
      if (((image->alpha_trait & BlendPixelTrait) == 0) ||
          ((morphology_traits & BlendPixelTrait) == 0))
        {
          /*
            No alpha blending.
          */
          (*pixel)=WalkReflectedKernel(walk,ConvolveChannelValue,k,(*pixel));
          break;
        }
      /*
        Alpha blending.
      */
      (*gamma)=0.0;
      BlendReflectedKernel(walk,k,pixel,gamma);
      break;
    }
    case ErodeMorphology:
    {
      /*
        Minimum value within kernel neighbourhood.

        The kernel is not reflected for this operation.  In normal
        Greyscale Morphology, the kernel value should be added
        to the real value, this is currently not done, due to the
        nature of the boolean kernels being used.
      */
      (*pixel)=WalkErodeOrDilateKernel(walk,ForwardKernelWalk,
        MinimizeChannelValue,(*pixel));
      break;
    }
    case DilateMorphology:
    {
      /*
         Maximum value within kernel neighbourhood.

         For correct working of this operation for asymmetrical kernels,
         the kernel needs to be applied in its reflected form.  That is
         its values needs to be reversed.

         In normal Greyscale Morphology, the kernel value should be
         added to the real value, this is currently not done, due to the
         nature of the boolean kernels being used.
      */
      (*pixel)=WalkErodeOrDilateKernel(walk,ReflectedKernelWalk,
        MaximizeChannelValue,(*pixel));
      break;
    }
    default:
    {
      ApplyMorphologyHitAndMiss(walk,quantum_pixels,pixel);
      break;
    }
  }
}

typedef struct _MorphologyRow
{
  MorphologyMethod
    method;

  const Image
    *image;

  Image
    *morphology_image;

  const KernelInfo
    *kernel;

  double
    bias;

  size_t
    *changes;

  ssize_t
    center;

  int
    id;
} MorphologyRow;

static void MorphologyRowChannel(const MorphologyRow *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q,const ssize_t i)
{
  double
    gamma,
    maximum,
    minimum,
    pixel;

  PixelChannel
    channel;

  PixelTrait
    morphology_traits,
    traits;

  const Quantum
    *magick_restrict pixels,
    *magick_restrict quantum_pixels;

  MorphologyKernelWalk
    walk;

  channel=GetPixelChannelChannel(row->image,i);
  traits=GetPixelChannelTraits(row->image,channel);
  morphology_traits=GetPixelChannelTraits(row->morphology_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (morphology_traits == UndefinedPixelTrait))
    return;
  if ((traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(row->morphology_image,channel,p[row->center+i],q);
      return;
    }
  pixels=p;
  quantum_pixels=(const Quantum *) NULL;
  maximum=0.0;
  minimum=(double) QuantumRange;
  walk.method=row->method;
  walk.image=row->image;
  walk.morphology_traits=morphology_traits;
  walk.kernel=row->kernel;
  walk.p=p;
  walk.pixels=pixels;
  walk.bias=row->bias;
  walk.maximum=maximum;
  walk.minimum=minimum;
  walk.center=row->center;
  walk.i=i;
  pixel=GetMorphologyInitialPixel(&walk);
  gamma=1.0;
  ApplyMorphologyMethod(&walk,&pixel,&gamma,&quantum_pixels);
  if (quantum_pixels != (const Quantum *) NULL)
    {
      SetPixelChannel(row->morphology_image,channel,quantum_pixels[i],q);
      return;
    }
  gamma=MagickSafeReciprocal(gamma);
  SetPixelChannel(row->morphology_image,channel,ClampToQuantum(gamma*pixel),q);
  if (fabs(pixel-(double) p[row->center+i]) >= MagickEpsilon)
    row->changes[row->id]++;
}

typedef struct _MorphologyColumnWalk
{
  const Image
    *image;

  PixelTrait
    morphology_traits;

  const KernelInfo
    *kernel;

  const MagickRealType
    *magick_restrict k;

  const Quantum
    *magick_restrict pixels;

  ssize_t
    i;
} MorphologyColumnWalk;

static void BlendKernelColumn(const MorphologyColumnWalk *walk,
  double *pixel,double *gamma,size_t *count)
{
  const Image
    *image = walk->image;

  const KernelInfo
    *kernel = walk->kernel;

  const MagickRealType
    *magick_restrict k = walk->k;

  const Quantum
    *magick_restrict pixels = walk->pixels;

  const ssize_t
    i = walk->i;

  double
    alpha;

  ssize_t
    v;

  for (v=0; v < (ssize_t) kernel->height; v++)
  {
    if (!IsNaN(*k))
      {
        alpha=(double) (QuantumScale*(double)
          GetPixelAlpha(image,pixels));
        (*pixel)+=alpha*(*k)*(double) pixels[i];
        (*gamma)+=alpha*(*k);
        (*count)++;
      }
    k--;
    pixels+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ConvolveColumnChannelValue(const MorphologyColumnWalk *walk,
  double *pixel,double *gamma,size_t *count)
{
  const Image
    *image = walk->image;

  const PixelTrait
    morphology_traits = walk->morphology_traits;

  const KernelInfo
    *kernel = walk->kernel;

  const MagickRealType
    *magick_restrict k = walk->k;

  const Quantum
    *magick_restrict pixels = walk->pixels;

  const ssize_t
    i = walk->i;

  ssize_t
    v;

  if (((image->alpha_trait & BlendPixelTrait) == 0) ||
      ((morphology_traits & BlendPixelTrait) == 0))
    for (v=0; v < (ssize_t) kernel->height; v++)
    {
      if (!IsNaN(*k))
        {
          (*pixel)+=(*k)*(double) pixels[i];
          (*count)++;
        }
      k--;
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
  else
    {
      (*gamma)=0.0;
      BlendKernelColumn(walk,pixel,gamma,count);
    }
}

typedef struct _MorphologyColumn
{
  const Image
    *image;

  Image
    *morphology_image;

  const KernelInfo
    *kernel;

  double
    bias;

  size_t
    *changes;

  ssize_t
    center;

  int
    id;
} MorphologyColumn;

static void ConvolveMorphologyColumnChannel(const MorphologyColumn *column,
  const Quantum *magick_restrict p,Quantum *magick_restrict q,const ssize_t i)
{
  double
    gamma,
    pixel;

  PixelChannel
    channel;

  PixelTrait
    morphology_traits,
    traits;

  const MagickRealType
    *magick_restrict k;

  const Quantum
    *magick_restrict pixels;

  size_t
    count;

  MorphologyColumnWalk
    walk;

  channel=GetPixelChannelChannel(column->image,i);
  traits=GetPixelChannelTraits(column->image,channel);
  morphology_traits=GetPixelChannelTraits(column->morphology_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (morphology_traits == UndefinedPixelTrait))
    return;
  if ((traits & CopyPixelTrait) != 0)
    {
      SetPixelChannel(column->morphology_image,channel,p[column->center+i],q);
      return;
    }
  k=(&column->kernel->values[column->kernel->height-1]);
  pixels=p;
  pixel=column->bias;
  gamma=1.0;
  count=0;
  walk.image=column->image;
  walk.morphology_traits=morphology_traits;
  walk.kernel=column->kernel;
  walk.k=k;
  walk.pixels=pixels;
  walk.i=i;
  ConvolveColumnChannelValue(&walk,&pixel,&gamma,&count);
  if (fabs(pixel-(double) p[column->center+i]) >= MagickEpsilon)
    column->changes[column->id]++;
  gamma=MagickSafeReciprocal(gamma);
  if (count != 0)
    gamma*=(double) column->kernel->height/count;
  SetPixelChannel(column->morphology_image,channel,ClampToQuantum(gamma*
    pixel),q);
}

static void ConvolveMorphologyColumnPixels(const MorphologyColumn *column,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  const Image
    *image = column->image;

  Image
    *morphology_image = column->morphology_image;

  ssize_t
    r;

  for (r=0; r < (ssize_t) image->rows; r++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      ConvolveMorphologyColumnChannel(column,p,q,i);
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) GetPixelChannels(morphology_image);
  }
}

#define MorphologyTag  "Morphology/Image"

static void ReportMorphologyProgress(const Image *image,
  const MagickOffsetType progress,const size_t span,MagickBooleanType *status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,MorphologyTag,progress,span);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

typedef struct _MorphologyRowInput
{
  MorphologyMethod
    method;

  const Image
    *image;

  Image
    *morphology_image;

  const KernelInfo
    *kernel;

  double
    bias;

  CacheView
    *image_view,
    *morphology_view;

  OffsetInfo
    offset;

  size_t
    *changes,
    width;

  ExceptionInfo
    *exception;

  ssize_t
    y;

  int
    id;
} MorphologyRowInput;

static MagickBooleanType ProcessMorphologyRow(const MorphologyRowInput *input,
  MagickBooleanType *status)
{
  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  ssize_t
    center;

  MorphologyRow
    row;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(input->image_view,-input->offset.x,input->y-
    input->offset.y,input->width,input->kernel->height,input->exception);
  q=GetCacheViewAuthenticPixels(input->morphology_view,0,input->y,
    input->morphology_image->columns,1,input->exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  center=(ssize_t) ((ssize_t) GetPixelChannels(input->image)*(ssize_t)
    input->width*input->offset.y+(ssize_t) GetPixelChannels(input->image)*
    input->offset.x);
  row.method=input->method;
  row.image=input->image;
  row.morphology_image=input->morphology_image;
  row.kernel=input->kernel;
  row.bias=input->bias;
  row.changes=input->changes;
  row.center=center;
  row.id=input->id;
  for (x=0; x < (ssize_t) input->image->columns; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(input->image); i++)
    {
      MorphologyRowChannel(&row,p,q,i);
    }
    p+=(ptrdiff_t) GetPixelChannels(input->image);
    q+=(ptrdiff_t) GetPixelChannels(input->morphology_image);
  }
  if (SyncCacheViewAuthenticPixels(input->morphology_view,input->exception) ==
      MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

typedef struct _MorphologyColumnFetch
{
  const Image
    *image;

  Image
    *morphology_image;

  const KernelInfo
    *kernel;

  CacheView
    *image_view,
    *morphology_view;

  OffsetInfo
    offset;

  ExceptionInfo
    *exception;

  ssize_t
    x;
} MorphologyColumnFetch;

static MagickBooleanType FetchMorphologyColumn(
  const MorphologyColumnFetch *fetch,const Quantum *magick_restrict *p,
  Quantum *magick_restrict *q,MagickBooleanType *status)
{
  const Image
    *image = fetch->image;

  Image
    *morphology_image = fetch->morphology_image;

  const KernelInfo
    *kernel = fetch->kernel;

  CacheView
    *image_view = fetch->image_view,
    *morphology_view = fetch->morphology_view;

  const OffsetInfo
    offset = fetch->offset;

  ExceptionInfo
    *exception = fetch->exception;

  const ssize_t
    x = fetch->x;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  (*p)=GetCacheViewVirtualPixels(image_view,x,-offset.y,1,image->rows+
    kernel->height-1,exception);
  (*q)=GetCacheViewAuthenticPixels(morphology_view,x,0,1,
    morphology_image->rows,exception);
  if (((*p) == (const Quantum *) NULL) || ((*q) == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void SetMorphologyKernelOrigin(const MorphologyMethod method,
  const KernelInfo *kernel,OffsetInfo *offset,ExceptionInfo *exception)
{
  switch (method)
  {
    case ErodeMorphology:
    case ErodeIntensityMorphology:
    case HitAndMissMorphology:
    case ThinningMorphology:
    case ThickenMorphology:
    {
      /*
        Use kernel as is, not reflection required.
      */
      (*offset).x=kernel->x;
      (*offset).y=kernel->y;
      break;
    }
    default:
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "InvalidOption","`%s'","not a primitive morphology method");
      break;
    }
  }
}

static void SetMorphologyKernelOffset(const MorphologyMethod method,
  const KernelInfo *kernel,OffsetInfo *offset,ExceptionInfo *exception)
{
  switch (method)
  {
    case ConvolveMorphology:
    case DilateMorphology:
    case DilateIntensityMorphology:
    case IterativeDistanceMorphology:
    {
      /*
        Kernel needs to use a reflection about origin.
      */
      (*offset).x=(ssize_t) kernel->width-kernel->x-1;
      (*offset).y=(ssize_t) kernel->height-kernel->y-1;
      break;
    }
    default:
    {
      SetMorphologyKernelOrigin(method,kernel,offset,exception);
      break;
    }
  }
}

static void ResetMorphologyChanges(size_t *changes)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) GetOpenMPMaximumThreads(); j++)
    changes[j]=0;
}

static size_t *AcquireMorphologyChanges(void)
{
  size_t
    *changes;

  changes=(size_t *) AcquireQuantumMemory(GetOpenMPMaximumThreads(),
    sizeof(*changes));
  if (changes == (size_t *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  ResetMorphologyChanges(changes);
  return(changes);
}

typedef struct _MorphologyPrimitiveSetup
{
  MorphologyMethod
    method;

  const Image
    *image;

  Image
    *morphology_image;

  const KernelInfo
    *kernel;

  double
    bias;

  CacheView
    *image_view,
    *morphology_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  OffsetInfo
    offset;

  size_t
    changed,
    *changes,
    width;

  ExceptionInfo
    *exception;
} MorphologyPrimitiveSetup;

static size_t SumMorphologyChanges(size_t changed,const size_t *changes)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) GetOpenMPMaximumThreads(); j++)
    changed+=changes[j];
  return(changed);
}

static ssize_t MorphologyRows(const MorphologyPrimitiveSetup *setup)
{
  const MorphologyMethod
    method = setup->method;

  const Image
    *image = setup->image;

  Image
    *morphology_image = setup->morphology_image;

  const KernelInfo
    *kernel = setup->kernel;

  const double
    bias = setup->bias;

  CacheView
    *image_view = setup->image_view,
    *morphology_view = setup->morphology_view;

  MagickBooleanType
    status = setup->status;

  MagickOffsetType
    progress = setup->progress;

  const OffsetInfo
    offset = setup->offset;

  size_t
    changed = setup->changed,
    *changes = setup->changes;

  const size_t
    width = setup->width;

  ExceptionInfo
    *exception = setup->exception;

  ssize_t
    y;

  /*
    Normal handling of horizontal or rectangular kernels (row by row).
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,morphology_image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const int
      id = GetOpenMPThreadId();

    const MorphologyRowInput
      input = { method, image, morphology_image, kernel, bias, image_view,
        morphology_view, offset, changes, width, exception, y, id };

    if (ProcessMorphologyRow(&input,&status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportMorphologyProgress(image,progress,image->rows,&status);
      }
  }
  morphology_view=DestroyCacheView(morphology_view);
  image_view=DestroyCacheView(image_view);
  changed=SumMorphologyChanges(changed,changes);
  changes=(size_t *) RelinquishMagickMemory(changes);
  return(status ? (ssize_t) (changed/GetImageChannels(image)) : -1);
}

static ssize_t ConvolveMorphologyColumns(const MorphologyPrimitiveSetup *setup)
{
  const Image
    *image = setup->image;

  Image
    *morphology_image = setup->morphology_image;

  CacheView
    *image_view = setup->image_view,
    *morphology_view = setup->morphology_view;

  MagickBooleanType
    status = setup->status;

  MagickOffsetType
    progress = setup->progress;

  size_t
    changed = setup->changed,
    *changes = setup->changes;

  ssize_t
    j,
    x;

  /*
    Special handling (for speed) of vertical (blur) kernels.  This performs
    its handling in columns rather than in rows.  This is only done
    for convolve as it is the only method that generates very large 1-D
    vertical kernels (such as a 'BlurKernel')
 */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,morphology_image,image->columns,1)
#endif
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    const int
      id = GetOpenMPThreadId();

    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    ssize_t
      center;

    MorphologyColumn
      column;

    const MorphologyColumnFetch
      fetch = { image, morphology_image, setup->kernel, image_view,
        morphology_view, setup->offset, setup->exception, x };

    if (FetchMorphologyColumn(&fetch,&p,&q,&status) == MagickFalse)
      continue;
    center=(ssize_t) GetPixelChannels(image)*setup->offset.y;
    column.image=image;
    column.morphology_image=morphology_image;
    column.kernel=setup->kernel;
    column.bias=setup->bias;
    column.changes=changes;
    column.center=center;
    column.id=id;
    ConvolveMorphologyColumnPixels(&column,p,q);
    if (SyncCacheViewAuthenticPixels(morphology_view,setup->exception) ==
        MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportMorphologyProgress(image,progress,image->columns,&status);
      }
  }
  morphology_image->type=image->type;
  morphology_view=DestroyCacheView(morphology_view);
  image_view=DestroyCacheView(image_view);
  for (j=0; j < (ssize_t) GetOpenMPMaximumThreads(); j++)
    changed+=changes[j];
  changes=(size_t *) RelinquishMagickMemory(changes);
  return(status ? (ssize_t) (changed/GetImageChannels(image)) : 0);
}

static ssize_t MorphologyPrimitive(const Image *image,Image *morphology_image,
  const MorphologyPass *pass,ExceptionInfo *exception)
{
  const MorphologyMethod
    method = pass->method;

  const KernelInfo
    *kernel = pass->kernel;

  const double
    bias = pass->bias;

  CacheView
    *image_view,
    *morphology_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  MorphologyPrimitiveSetup
    setup;

  OffsetInfo
    offset;

  size_t
    changed,
    *changes,
    width;

  /*
    Some methods (including convolve) needs to use a reflected kernel.
    Adjust 'origin' offsets to loop though kernel as a reflection.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(morphology_image != (Image *) NULL);
  assert(morphology_image->signature == MagickCoreSignature);
  assert(kernel != (KernelInfo *) NULL);
  assert(kernel->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  morphology_view=AcquireAuthenticCacheView(morphology_image,exception);
  width=image->columns+kernel->width-1;
  offset.x=0;
  offset.y=0;
  SetMorphologyKernelOffset(method,kernel,&offset,exception);
  changed=0;
  changes=AcquireMorphologyChanges();
  setup.method=method;
  setup.image=image;
  setup.morphology_image=morphology_image;
  setup.kernel=kernel;
  setup.bias=bias;
  setup.image_view=image_view;
  setup.morphology_view=morphology_view;
  setup.status=status;
  setup.progress=progress;
  setup.offset=offset;
  setup.changed=changed;
  setup.changes=changes;
  setup.width=width;
  setup.exception=exception;
  if ((method == ConvolveMorphology) && (kernel->width == 1))
    return(ConvolveMorphologyColumns(&setup));
  return(MorphologyRows(&setup));
}

/*
  This is almost identical to the MorphologyPrimitive() function above, but
  applies the primitive directly to the actual image using two passes, once in
  each direction, with the results of the previous (and current) row being
  re-used.

  That is after each row is 'Sync'ed' into the image, the next row makes use of
  those values as part of the calculation of the next row.  It repeats, but
  going in the opposite (bottom-up) direction.

  Because of this 're-use of results' this function can not make use of multi-
  threaded, parallel processing.
*/
typedef struct _MorphologyDirectPixel
{
  Image
    *image;

  const KernelInfo
    *kernel;

  Quantum
    *q;

  ssize_t
    x,
    i;
} MorphologyDirectPixel;

static double MinimizeLeadingRowDistance(const MorphologyDirectPixel *row,
  const OffsetInfo offset,double pixel)
{
  Image
    *const image = row->image;

  const KernelInfo
    *const kernel = row->kernel;

  Quantum
    *const q = row->q;

  const ssize_t
    x = row->x,
    i = row->i;

  const MagickRealType
    *magick_restrict k;

  const Quantum
    *magick_restrict pixels;

  ssize_t
    u;

  k=(&kernel->values[(ssize_t) kernel->width*(kernel->y+1)-1]);
  pixels=q-offset.x*(ssize_t) GetPixelChannels(image);
  for (u=0; u < offset.x; u++)
  {
    if (!IsNaN(*k) && ((x+u-offset.x) >= 0))
      {
        if (((double) pixels[i]+(*k)) < pixel)
          pixel=(double) pixels[i]+(*k);
      }
    k--;
    pixels+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(pixel);
}

static double MinimizeTrailingRowDistance(const MorphologyDirectPixel *row,
  const OffsetInfo offset,const MagickRealType *magick_restrict k,double pixel)
{
  Image
    *const image = row->image;

  const KernelInfo
    *const kernel = row->kernel;

  Quantum
    *const q = row->q;

  const ssize_t
    x = row->x,
    i = row->i;

  const Quantum
    *magick_restrict pixels;

  ssize_t
    u;

  pixels=q;
  for (u=offset.x+1; u < (ssize_t) kernel->width; u++)
  {
    pixels+=(ptrdiff_t) GetPixelChannels(image);
    if (!IsNaN(*k) && ((x+u-offset.x) < (ssize_t) image->columns))
      {
        if (((double) pixels[i]+(*k)) < pixel)
          pixel=(double) pixels[i]+(*k);
      }
    k--;
  }
  return(pixel);
}

typedef struct _MorphologyKernelRows
{
  Image
    *image;

  const KernelInfo
    *kernel;

  ssize_t
    i;
} MorphologyKernelRows;

static void MinimizeKernelRowDistance(const MorphologyKernelRows *rows,
  double *pixel,const MagickRealType *magick_restrict *k,
  const Quantum *magick_restrict *pixels)
{
  Image
    *const image = rows->image;

  const KernelInfo
    *const kernel = rows->kernel;

  const ssize_t
    i = rows->i;

  ssize_t
    u;

  for (u=0; u < (ssize_t) kernel->width; u++)
  {
    *pixel=MinimizeKernelDistance(*k,*pixels,i,*pixel);
    (*k)--;
    *pixels+=(ptrdiff_t) GetPixelChannels(image);
  }
  *pixels+=(image->columns-1)*GetPixelChannels(image);
}

static double MinimizeTrailingRowsDistance(const MorphologyKernelRows *rows,
  const OffsetInfo offset,const Quantum *magick_restrict pixels,double pixel)
{
  Image
    *const image = rows->image;

  const KernelInfo
    *const kernel = rows->kernel;

  const ssize_t
    i = rows->i;

  const MagickRealType
    *magick_restrict k;

  ssize_t
    u,
    v;

  k=(&kernel->values[(ssize_t) kernel->width*(kernel->y+1)-1]);
  for (v=offset.y; v < (ssize_t) kernel->height; v++)
  {
    for (u=0; u < (ssize_t) kernel->width; u++)
    {
      pixel=MinimizeKernelDistance(k,pixels,i,pixel);
      k--;
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(image->columns-1)*GetPixelChannels(image);
  }
  return(pixel);
}

static OffsetInfo GetDirectKernelOffset(const MorphologyMethod method,
  const KernelInfo *kernel)
{
  OffsetInfo
    offset;

  switch(method)
  {
    case DistanceMorphology:
    case VoronoiMorphology:
    {
      /*
        Kernel reflected about origin.
      */
      offset.x=(ssize_t) kernel->width-kernel->x-1;
      offset.y=(ssize_t) kernel->height-kernel->y-1;
      break;
    }
    default:
    {
      offset.x=kernel->x;
      offset.y=kernel->y;
      break;
    }
  }
  return(offset);
}

typedef enum
{
  ForwardDirectScan,
  ReverseDirectScan
} DirectScanSide;

typedef struct _MorphologyDirectScan
{
  DirectScanSide
    side;

  Image
    *image;

  MorphologyMethod
    method;

  const KernelInfo
    *kernel;

  OffsetInfo
    offset;

  CacheView
    *image_view,
    *morphology_view;

  size_t
    width;

  ExceptionInfo
    *exception;
} MorphologyDirectScan;

static double ForwardDirectChannelValue(const MorphologyDirectScan *scan,
  const MorphologyDirectPixel *input,const Quantum *magick_restrict pixels,
  double pixel)
{
  Image
    *const image = input->image;

  const KernelInfo
    *const kernel = input->kernel;

  Quantum
    *const q = input->q;

  const ssize_t
    x = input->x,
    i = input->i;

  const MorphologyMethod
    method = scan->method;

  const OffsetInfo
    offset = scan->offset;

  const MagickRealType
    *magick_restrict k;

  MorphologyDirectPixel
    row;

  MorphologyKernelRows
    rows;

  ssize_t
    v;

  switch (method)
  {
    case DistanceMorphology:
    {
      k=(&kernel->values[kernel->width*kernel->height-1]);
      for (v=0; v <= offset.y; v++)
      {
        rows.image=image;
        rows.kernel=kernel;
        rows.i=i;
        MinimizeKernelRowDistance(&rows,&pixel,&k,&pixels);
      }
      row.image=image;
      row.kernel=kernel;
      row.q=q;
      row.x=x;
      row.i=i;
      pixel=MinimizeLeadingRowDistance(&row,offset,pixel);
      break;
    }
    case VoronoiMorphology:
    {
      k=(&kernel->values[kernel->width*kernel->height-1]);
      for (v=0; v < offset.y; v++)
      {
        rows.image=image;
        rows.kernel=kernel;
        rows.i=i;
        MinimizeKernelRowDistance(&rows,&pixel,&k,&pixels);
      }
      row.image=image;
      row.kernel=kernel;
      row.q=q;
      row.x=x;
      row.i=i;
      pixel=MinimizeLeadingRowDistance(&row,offset,pixel);
      break;
    }
    default:
      break;
  }
  return(pixel);
}

static double ReverseDirectChannelValue(const MorphologyDirectScan *scan,
  const MorphologyDirectPixel *input,const Quantum *magick_restrict pixels,
  double pixel)
{
  Image
    *const image = input->image;

  const KernelInfo
    *const kernel = input->kernel;

  Quantum
    *const q = input->q;

  const ssize_t
    x = input->x,
    i = input->i;

  const MorphologyMethod
    method = scan->method;

  const OffsetInfo
    offset = scan->offset;

  const MagickRealType
    *magick_restrict k;

  MorphologyDirectPixel
    row;

  MorphologyKernelRows
    rows;

  switch (method)
  {
    case DistanceMorphology:
    {
      rows.image=image;
      rows.kernel=kernel;
      rows.i=i;
      pixel=MinimizeTrailingRowsDistance(&rows,offset,pixels,pixel);
      k=(&kernel->values[(ssize_t) kernel->width*kernel->y+kernel->x-1]);
      row.image=image;
      row.kernel=kernel;
      row.q=q;
      row.x=x;
      row.i=i;
      pixel=MinimizeTrailingRowDistance(&row,offset,k,pixel);
      break;
    }
    case VoronoiMorphology:
    {
      rows.image=image;
      rows.kernel=kernel;
      rows.i=i;
      pixel=MinimizeTrailingRowsDistance(&rows,offset,pixels,pixel);
      k=(&kernel->values[(ssize_t) kernel->width*(kernel->y+1)-1]);
      row.image=image;
      row.kernel=kernel;
      row.q=q;
      row.x=x;
      row.i=i;
      pixel=MinimizeTrailingRowDistance(&row,offset,k,pixel);
      break;
    }
    default:
      break;
  }
  return(pixel);
}

static inline void MorphologyDirectChannel(const MorphologyDirectScan *scan,
  const MorphologyDirectPixel *input,const Quantum *magick_restrict p,
  size_t *changed)
{
  const DirectScanSide
    side = scan->side;

  Image
    *const image = input->image;

  Quantum
    *const q = input->q;

  const ssize_t
    i = input->i;

  double
    pixel;

  PixelChannel
    channel;

  PixelTrait
    traits;

  const Quantum
    *magick_restrict pixels;

  channel=GetPixelChannelChannel(image,i);
  traits=GetPixelChannelTraits(image,channel);
  if (traits == UndefinedPixelTrait)
    return;
  if ((traits & CopyPixelTrait) != 0)
    return;
  pixels=p;
  pixel=(double) QuantumRange;
  pixel=((side == ForwardDirectScan) ?
    ForwardDirectChannelValue(scan,input,pixels,pixel) :
    ReverseDirectChannelValue(scan,input,pixels,pixel));
  if (fabs(pixel-(double) q[i]) > MagickEpsilon)
    (*changed)++;
  q[i]=ClampToQuantum(pixel);
}

static inline void MorphologyDirectPixels(const MorphologyDirectScan *scan,
  const Quantum *magick_restrict p,Quantum *magick_restrict q,size_t *changed)
{
  const DirectScanSide
    side = scan->side;

  Image
    *const image = scan->image;

  const KernelInfo
    *const kernel = scan->kernel;

  ssize_t
    x;

  for (x=((side == ForwardDirectScan) ? 0 : (ssize_t) image->columns-1);
       ((side == ForwardDirectScan) ? x < (ssize_t) image->columns : x >= 0);
       ((side == ForwardDirectScan) ? x++ : x--))
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      const MorphologyDirectPixel
        input = { image, kernel, q, x, i };

      MorphologyDirectChannel(scan,&input,p,changed);
    }
    ((side == ForwardDirectScan) ? (p+=(ptrdiff_t) GetPixelChannels(image)) :
      (p-=(ptrdiff_t)GetPixelChannels(image)));
    ((side == ForwardDirectScan) ? (q+=(ptrdiff_t) GetPixelChannels(image)) :
      (q-=GetPixelChannels(image)));
  }
}

static inline MagickBooleanType MorphologyDirectScanRow(
  const MorphologyDirectScan *scan,const ssize_t y,size_t *changed,
  MagickBooleanType *status)
{
  const DirectScanSide
    side = scan->side;

  Image
    *const image = scan->image;

  const KernelInfo
    *const kernel = scan->kernel;

  const OffsetInfo
    offset = scan->offset;

  CacheView
    *const image_view = scan->image_view,
    *const morphology_view = scan->morphology_view;

  const size_t
    width = scan->width;

  ExceptionInfo
    *const exception = scan->exception;

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  if ((*status) == MagickFalse)
    return(MagickFalse);
  p=GetCacheViewVirtualPixels(image_view,-offset.x,((side ==
    ForwardDirectScan) ? y-offset.y : y),width,((side == ForwardDirectScan) ?
    (size_t) offset.y+1 : (size_t) kernel->y+1),exception);
  q=GetCacheViewAuthenticPixels(morphology_view,0,y,image->columns,1,
    exception);
  if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
    {
      (*status)=MagickFalse;
      return(MagickFalse);
    }
  if (side == ReverseDirectScan)
    {
      p+=(ptrdiff_t) (image->columns-1)*GetPixelChannels(image);
      q+=(ptrdiff_t) (image->columns-1)*GetPixelChannels(image);
    }
  MorphologyDirectPixels(scan,p,q,changed);
  if (SyncCacheViewAuthenticPixels(morphology_view,exception) == MagickFalse)
    (*status)=MagickFalse;
  return(MagickTrue);
}

static inline MagickOffsetType MorphologyDirectPass(
  const MorphologyDirectScan *scan,MagickOffsetType progress,size_t *changed,
  MagickBooleanType *status)
{
  const DirectScanSide
    side = scan->side;

  Image
    *const image = scan->image;

  ssize_t
    y;

  for (y=((side == ForwardDirectScan) ? 0 : (ssize_t) image->rows-1);
       ((side == ForwardDirectScan) ? y < (ssize_t) image->rows : y >= 0);
       ((side == ForwardDirectScan) ? y++ : y--))
  {
    if (MorphologyDirectScanRow(scan,y,changed,status) == MagickFalse)
      continue;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        ReportMorphologyProgress(image,progress,2*image->rows,status);
      }
  }
  return(progress);
}

static ssize_t MorphologyPrimitiveDirect(Image *image,
  const MorphologyMethod method,const KernelInfo *kernel,
  ExceptionInfo *exception)
{
  CacheView
    *morphology_view,
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  OffsetInfo
    offset;

  size_t
    width,
    changed;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(kernel != (KernelInfo *) NULL);
  assert(kernel->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  status=MagickTrue;
  changed=0;
  progress=0;
  offset=GetDirectKernelOffset(method,kernel);
  /*
    Two views into same image, do not thread.
  */
  image_view=AcquireVirtualCacheView(image,exception);
  morphology_view=AcquireAuthenticCacheView(image,exception);
  width=image->columns+kernel->width-1;
  {
    const MorphologyDirectScan
      scan = { ForwardDirectScan, image, method, kernel, offset, image_view,
        morphology_view, width, exception };

    /*
      Read virtual pixels, and authentic pixels, from the same image!  We read
      using virtual to get virtual pixel handling, but write back into the same
      image.

      Only top half of kernel is processed as we do a single pass downward
      through the image iterating the distance function as we go.
    */
    progress=MorphologyDirectPass(&scan,progress,&changed,&status);
  }
  morphology_view=DestroyCacheView(morphology_view);
  image_view=DestroyCacheView(image_view);
  /*
    Do the reverse pass through the image.
  */
  image_view=AcquireVirtualCacheView(image,exception);
  morphology_view=AcquireAuthenticCacheView(image,exception);
  {
    const MorphologyDirectScan
      scan = { ReverseDirectScan, image, method, kernel, offset, image_view,
        morphology_view, width, exception };

    /*
       Read virtual pixels, and authentic pixels, from the same image.  We
       read using virtual to get virtual pixel handling, but write back
       into the same image.

       Only the bottom half of the kernel is processed as we up the image.
    */
    progress=MorphologyDirectPass(&scan,progress,&changed,&status);
  }
  morphology_view=DestroyCacheView(morphology_view);
  image_view=DestroyCacheView(image_view);
  return(status ? (ssize_t) (changed/GetImageChannels(image)) : -1);
}

/*
  Apply a Morphology by calling one of the above low level primitive
  application functions.  This function handles any iteration loops,
  composition or re-iteration of results, and compound morphology methods that
  is based on multiple low-level (staged) morphology methods.

  Basically this provides the complex glue between the requested morphology
  method and raw low-level implementation (above).
*/
typedef struct _MorphologyStage
{
  MorphologyMethod
    method,
    primitive;

  size_t
    method_loop,
    stage_loop,
    stage_limit;
} MorphologyStage;

static void FormatMorphologyStage(char *v_info,const MorphologyStage *stage)
{
  const MorphologyMethod
    method = stage->method,
    primitive = stage->primitive;

  const size_t
    method_loop = stage->method_loop,
    stage_loop = stage->stage_loop,
    stage_limit = stage->stage_limit;

  if ( stage_limit > 1 )
    (void) FormatLocaleString(v_info,MagickPathExtent,"%s:%.17g.%.17g -> ",
     CommandOptionToMnemonic(MagickMorphologyOptions,method),(double)
     method_loop,(double) stage_loop);
  else if ( primitive != method )
    (void) FormatLocaleString(v_info, MagickPathExtent, "%s:%.17g -> ",
      CommandOptionToMnemonic(MagickMorphologyOptions, method),(double)
      method_loop);
  else
    v_info[0] = '\0';
}

typedef struct _MorphologyPassReport
{
  MorphologyMethod
    primitive;

  KernelInfo
    *this_kernel,
    *rflt_kernel;

  size_t
    method_loop,
    kernel_number,
    kernel_loop,
    count;

  ssize_t
    changed;
} MorphologyPassReport;

static void ReportMorphologyPass(const char *v_info,
  const MorphologyPassReport *report)
{
  const MorphologyMethod
    primitive = report->primitive;

  KernelInfo
    *const this_kernel = report->this_kernel,
    *const rflt_kernel = report->rflt_kernel;

  const size_t
    method_loop = report->method_loop,
    kernel_number = report->kernel_number,
    kernel_loop = report->kernel_loop,
    count = report->count;

  const ssize_t
    changed = report->changed;

  if ( kernel_loop > 1 )
    (void) FormatLocaleFile(stderr, "\n"); /* add end-of-line from previous */
  (void) (void) FormatLocaleFile(stderr,
    "%s%s%s:%.17g.%.17g #%.17g => Changed %.17g",
    v_info,CommandOptionToMnemonic(MagickMorphologyOptions,
    primitive),(this_kernel == rflt_kernel ) ? "*" : "",
    (double) (method_loop+kernel_loop-1),(double) kernel_number,
    (double) count,(double) changed);
}

static MagickBooleanType AcquireWorkImage(const Image *image,
  Image **work_image,ExceptionInfo *exception)
{
  if ( *work_image == (Image *) NULL )
    {
      *work_image=CloneImage(image,0,0,MagickTrue,exception);
      if (*work_image == (Image *) NULL)
        return(MagickFalse);
      if (SetImageStorageClass(*work_image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AcquireReflectedKernel(const MorphologyMethod method,
  const KernelInfo *kernel,KernelInfo **reflected_kernel)
{
  switch ( method ) {
    case CorrelateMorphology:
    case CloseMorphology:
    case CloseIntensityMorphology:
    case BottomHatMorphology:
    case SmoothMorphology:
      *reflected_kernel = CloneKernelInfo(kernel);
      if (*reflected_kernel == (KernelInfo *) NULL)
        return(MagickFalse);
      RotateKernelInfo(*reflected_kernel,180);
      break;
    default:
      break;
  }
  return(MagickTrue);
}

static void ReportKernelIteration(const MagickBooleanType verbose,
  const KernelInfo *this_kernel)
{
  if (verbose != MagickFalse) {
    if ( this_kernel->next != (KernelInfo *) NULL )
      (void) FormatLocaleFile(stderr, " (re-iterate)");
    else
      (void) FormatLocaleFile(stderr, " (done)");
  }
}

static void ReportSaveForCompose(const MagickBooleanType verbose)
{
  if (verbose != MagickFalse)
    (void) FormatLocaleFile(stderr, " (save for compose)");
}

typedef struct _MorphologyKernelCompose
{
  MagickBooleanType
    verbose;

  CompositeOperator
    rslt_compose;

  Image
    *rslt_image;

  const Image
    *image;

  ExceptionInfo
    *exception;
} MorphologyKernelCompose;

static Image *ComposeKernelResult(const MorphologyKernelCompose *kernel_compose,
  Image *curr_image)
{
  const MagickBooleanType
    verbose = kernel_compose->verbose;

  const CompositeOperator
    rslt_compose = kernel_compose->rslt_compose;

  Image
    *const rslt_image = kernel_compose->rslt_image;

  const Image
    *const image = kernel_compose->image;

  ExceptionInfo
    *const exception = kernel_compose->exception;

  if (verbose != MagickFalse)
    (void) FormatLocaleFile(stderr, " (compose \"%s\")",
      CommandOptionToMnemonic(MagickComposeOptions, rslt_compose) );
  (void) CompositeImage(rslt_image,curr_image,rslt_compose,MagickTrue,
    0,0,exception);
  curr_image = DestroyImage(curr_image);
  curr_image = (Image *) image;  /* continue with original image */
  return(curr_image);
}

typedef struct _MorphologyStageSelection
{
  MorphologyMethod
    method;

  KernelInfo
    *rflt_kernel;

  size_t
    stage_loop;
} MorphologyStageSelection;

static void SelectSmoothPrimitive(const MorphologyStageSelection *selection,
  KernelInfo **this_kernel,MorphologyMethod *primitive)
{
  switch ( selection->stage_loop ) {
    case 1: /* start an open method, which starts with Erode */
      (*primitive) = ErodeMorphology;
      break;
    case 2:  /* now Dilate the Erode */
      (*primitive) = DilateMorphology;
      break;
    case 3:  /* Reflect kernel a close */
      (*this_kernel) = selection->rflt_kernel; /* use the reflected kernel */
      (*primitive) = DilateMorphology;
      break;
    case 4:  /* Finish the Close */
      (*this_kernel) = selection->rflt_kernel; /* use the reflected kernel */
      (*primitive) = ErodeMorphology;
      break;
  }
}

static void SelectSmoothOrCorrelatePrimitive(
  const MorphologyStageSelection *selection,KernelInfo **this_kernel,
  MorphologyMethod *primitive)
{
  switch( selection->method ) {
    case SmoothMorphology:         /* open, close */
      SelectSmoothPrimitive(selection,this_kernel,primitive);
      break;
    case CorrelateMorphology:
      /* A Correlation is a Convolution with a reflected kernel.
      ** However a Convolution is a weighted sum using a reflected
      ** kernel.  It may seem strange to convert a Correlation into a
      ** Convolution as the Correlation is the simpler method, but
      ** Convolution is much more commonly used, and it makes sense to
      ** implement it directly so as to avoid the need to duplicate the
      ** kernel when it is not required (which is typically the
      ** default).
      */
      (*this_kernel) = selection->rflt_kernel; /* use the reflected kernel */
      (*primitive) = ConvolveMorphology;
      break;
    default:
      break;
  }
}

static void SelectOpenOrClosePrimitive(
  const MorphologyStageSelection *selection,KernelInfo **this_kernel,
  MorphologyMethod *primitive)
{
  switch( selection->method ) {
    case OpenMorphology:       /* erode then dilate */
    case TopHatMorphology:     /* open and image difference */
      (*primitive) = ErodeMorphology;
      if ( selection->stage_loop == 2 )
        (*primitive) = DilateMorphology;
      break;
    case CloseMorphology:      /* dilate, then erode */
    case BottomHatMorphology:  /* close and image difference */
      (*this_kernel) = selection->rflt_kernel; /* use the reflected kernel */
      (*primitive) = DilateMorphology;
      if ( selection->stage_loop == 2 )
        (*primitive) = ErodeMorphology;
      break;
    default:
      SelectSmoothOrCorrelatePrimitive(selection,&(*this_kernel),&(*primitive));
      break;
  }
}

static void SelectErodeOrDilatePrimitive(
  const MorphologyStageSelection *selection,KernelInfo **this_kernel,
  MorphologyMethod *primitive)
{
  switch( selection->method ) {
    case ErodeMorphology:      /* just erode */
    case EdgeInMorphology:     /* erode and image difference */
      (*primitive) = ErodeMorphology;
      break;
    case DilateMorphology:     /* just dilate */
    case EdgeOutMorphology:    /* dilate and image difference */
      (*primitive) = DilateMorphology;
      break;
    case OpenIntensityMorphology:
      (*primitive) = ErodeIntensityMorphology;
      if ( selection->stage_loop == 2 )
        (*primitive) = DilateIntensityMorphology;
      break;
    default:
      SelectOpenOrClosePrimitive(selection,&(*this_kernel),&(*primitive));
      break;
  }
}

static void SelectCloseIntensityPrimitive(
  const MorphologyStageSelection *selection,KernelInfo **this_kernel,
  MorphologyMethod *primitive)
{
  switch( selection->method ) {
    case CloseIntensityMorphology:
      (*this_kernel) = selection->rflt_kernel; /* use the reflected kernel */
      (*primitive) = DilateIntensityMorphology;
      if ( selection->stage_loop == 2 )
        (*primitive) = ErodeIntensityMorphology;
      break;
    default:
      SelectErodeOrDilatePrimitive(selection,&(*this_kernel),&(*primitive));
      break;
  }
}

typedef struct _MorphologyStages
{
  const Image
    *image;

  MorphologyMethod
    method;

  KernelInfo
    *norm_kernel,
    *rflt_kernel;

  double
    bias;

  MagickBooleanType
    verbose;

  size_t
    method_loop,
    kernel_number,
    stage_limit,
    kernel_limit;

  char
    *v_info;

  ExceptionInfo
    *exception;
} MorphologyStages;

typedef struct _MorphologyIterationTargets
{
  size_t
    *kernel_changed;

  ssize_t
    *changed;

  Image
    **work_image;

  size_t
    *count,
    *method_changed;

  Image
    **curr_image;
} MorphologyIterationTargets;

static void SwapMorphologyImages(const MorphologyStages *stages,
  Image **work_image,Image **curr_image)
{
  { Image *tmp = (*work_image);   /* swap images for iteration */
    (*work_image) = (*curr_image);
    (*curr_image) = tmp;
  }
  if ( (*work_image) == stages->image )
    (*work_image) = (Image *) NULL; /* replace input 'image' */
}

static MagickBooleanType IterateMorphologyKernel(
  const MorphologyStages *stages,const MorphologyMethod primitive,
  KernelInfo *this_kernel,const MorphologyIterationTargets *targets)
{
  size_t
    *const kernel_changed = targets->kernel_changed;

  ssize_t
    *const changed = targets->changed;

  Image
    **const work_image = targets->work_image;

  size_t
    *const count = targets->count,
    *const method_changed = targets->method_changed;

  Image
    **const curr_image = targets->curr_image;

  size_t
    kernel_loop;

  kernel_loop = 0;
  (*kernel_changed) = 0;
  (*changed) = 1;
  while ( kernel_loop < stages->kernel_limit && (*changed) > 0 ) {
    const MorphologyPass pass = { primitive, this_kernel, stages->bias };

    kernel_loop++;     /* the iteration of this kernel */

    /* Create a clone as the destination image, if not yet defined */
    if (AcquireWorkImage(stages->image,work_image,stages->exception) == MagickFalse)
      return(MagickFalse);

    /* APPLY THE MORPHOLOGICAL PRIMITIVE (curr -> work) */
    (*count)++;
    (*changed) = MorphologyPrimitive((*curr_image), (*work_image), &pass,
                 stages->exception);
    if (stages->verbose != MagickFalse) {
      MorphologyPassReport
        report;

      report.primitive=primitive;
      report.this_kernel=this_kernel;
      report.rflt_kernel=stages->rflt_kernel;
      report.method_loop=stages->method_loop;
      report.kernel_number=stages->kernel_number;
      report.kernel_loop=kernel_loop;
      report.count=(*count);
      report.changed=(*changed);
      ReportMorphologyPass(stages->v_info,&report);
    }
    if ( (*changed) < 0 )
      return(MagickFalse);
    (*kernel_changed) = (size_t) ((ssize_t) (*kernel_changed)+(*changed));
    (*method_changed) = (size_t) ((ssize_t) (*method_changed)+(*changed));

    /* prepare next loop */
    SwapMorphologyImages(stages,work_image,curr_image);

  } /* End Loop 4: Iterate the kernel with primitive */
  return(MagickTrue);
}

static void ReportStageChanges(const MorphologyStages *stages,
  const size_t kernel_changed,const ssize_t changed,const size_t stage_loop)
{
  if (stages->verbose != MagickFalse && kernel_changed != (size_t)changed)
    (void) FormatLocaleFile(stderr, "   Total %.17g",(double) kernel_changed);
  if (stages->verbose != MagickFalse && stage_loop < stages->stage_limit)
    (void) FormatLocaleFile(stderr, "\n"); /* add end-of-line before looping */
}

static void ReportMorphologyStage(const MorphologyStages *stages,
  const MorphologyMethod primitive,const size_t stage_loop)
{
  if (stages->verbose != MagickFalse) {
    MorphologyStage
      stage;

    stage.method=stages->method;
    stage.primitive=primitive;
    stage.method_loop=stages->method_loop;
    stage.stage_loop=stage_loop;
    stage.stage_limit=stages->stage_limit;
    FormatMorphologyStage(stages->v_info,&stage);
  }
}

typedef struct _MorphologyStageTargets
{
  KernelInfo
    **this_kernel;

  Image
    **save_image,
    **curr_image,
    **work_image;

  size_t
    *count,
    *method_changed;
} MorphologyStageTargets;

static MorphologyMethod SaveEdgeDilateImage(const MorphologyStages *stages,
  Image **save_image,Image **curr_image)
{
  MorphologyMethod
    primitive;

  (*save_image) = (*curr_image);      /* save the image difference */
  (*curr_image) = (Image *) stages->image;
  primitive = ErodeMorphology;
  return(primitive);
}

static MagickBooleanType ApplyMorphologyStages(
  const MorphologyStages *stages,const MorphologyStageTargets *targets)
{
  KernelInfo
    **const this_kernel = targets->this_kernel;

  Image
    **const save_image = targets->save_image,
    **const curr_image = targets->curr_image,
    **const work_image = targets->work_image;

  size_t
    *const count = targets->count,
    *const method_changed = targets->method_changed;

  MorphologyMethod
    primitive;

  size_t
    stage_loop,
    kernel_changed;

  ssize_t
    changed;

  const MorphologyIterationTargets
    iteration_targets = { &kernel_changed, &changed, work_image, count,
      method_changed, curr_image };

  stage_loop = 0;          /* the compound morphology stage number */
  while ( stage_loop < stages->stage_limit ) {
    MorphologyStageSelection
      selection;

    stage_loop++;   /* The stage of the compound morphology */

    /* Select primitive morphology for this stage of compound method */
    (*this_kernel) = stages->norm_kernel; /* default use unreflected kernel */
    primitive = stages->method;        /* Assume method is a primitive */
    selection.method=stages->method;
    selection.rflt_kernel=stages->rflt_kernel;
    selection.stage_loop=stage_loop;
    switch( stages->method ) {
      case EdgeMorphology:        /* dilate and erode difference */
        primitive = DilateMorphology;
        if ( stage_loop == 2 )
          primitive=SaveEdgeDilateImage(stages,save_image,curr_image);
        break;
      default:
        SelectCloseIntensityPrimitive(&selection,this_kernel,&primitive);
        break;
    }
    assert( (*this_kernel) != (KernelInfo *) NULL );

    /* Extra information for debugging compound operations */
    ReportMorphologyStage(stages,primitive,stage_loop);

    /* Loop 4: Iterate the kernel with primitive */
    if (IterateMorphologyKernel(stages,primitive,(*this_kernel),
        &iteration_targets) == MagickFalse)
      return(MagickFalse);

    ReportStageChanges(stages,kernel_changed,changed,stage_loop);

#if 0
    (void) FormatLocaleFile(stderr, "--E-- image=0x%lx\n", (unsigned long)image);
    (void) FormatLocaleFile(stderr, "      curr =0x%lx\n", (unsigned long)curr_image);
    (void) FormatLocaleFile(stderr, "      work =0x%lx\n", (unsigned long)work_image);
    (void) FormatLocaleFile(stderr, "      save =0x%lx\n", (unsigned long)save_image);
    (void) FormatLocaleFile(stderr, "      union=0x%lx\n", (unsigned long)rslt_image);
#endif

  } /* End Loop 3: Primitive (staging) Loop for Compound Methods */
  return(MagickTrue);
}

static void ReportCompoundDifference(const MagickBooleanType verbose,
  const MorphologyMethod method,const char *const format)
{
  if (verbose != MagickFalse)
    (void) FormatLocaleFile(stderr,
      format,CommandOptionToMnemonic(
      MagickMorphologyOptions, method) );
}

static size_t GetMorphologyKernelLimit(const Image *image,
  const ssize_t iterations)
{
  size_t
    kernel_limit;

  kernel_limit = (size_t) iterations;
  if ( iterations < 0 )  /* negative interactions = infinite (well almost) */
     kernel_limit = image->columns>image->rows ? image->columns : image->rows;
  return(kernel_limit);
}

static CompositeOperator ResolveMultiKernelCompose(
  const CompositeOperator compose,CompositeOperator rslt_compose)
{
  if ( compose != UndefinedCompositeOp )
    rslt_compose = compose;  /* override default composition for method */
  if ( rslt_compose == UndefinedCompositeOp )
    rslt_compose = NoCompositeOp; /* still not defined! Then re-iterate */
  return(rslt_compose);
}

typedef struct _MorphologyDirectApply
{
  const Image
    *image;

  MorphologyMethod
    method;

  const KernelInfo
    *kernel;

  MagickBooleanType
    verbose;

  ExceptionInfo
    *exception;
} MorphologyDirectApply;

static void ReportDirectMorphology(const MagickBooleanType verbose,
  const MorphologyMethod method,const ssize_t changed)
{
  if (verbose != MagickFalse)
    (void) (void) FormatLocaleFile(stderr,
      "%s:%.17g.%.17g #%.17g => Changed %.17g\n",
      CommandOptionToMnemonic(MagickMorphologyOptions, method),
      1.0,0.0,1.0, (double) changed);
}

static void PreserveVoronoiAlpha(const MorphologyMethod method,
  const Image *image,Image **rslt_image,ExceptionInfo *exception)
{
  if ( method == VoronoiMorphology ) {
    /* Preserve the alpha channel of input image - but turned it off */
    (void) SetImageAlphaChannel((*rslt_image), DeactivateAlphaChannel,
      exception);
    (void) CompositeImage((*rslt_image),image,CopyAlphaCompositeOp,
      MagickTrue,0,0,exception);
    (void) SetImageAlphaChannel((*rslt_image), DeactivateAlphaChannel,
      exception);
  }
}

static MagickBooleanType ApplyDirectMorphology(
  const MorphologyDirectApply *direct,Image **rslt_image)
{
  const Image
    *const image = direct->image;

  const MorphologyMethod
    method = direct->method;

  const KernelInfo
    *const kernel = direct->kernel;

  const MagickBooleanType
    verbose = direct->verbose;

  ExceptionInfo
    *const exception = direct->exception;

  ssize_t
    changed;        /* number pixels changed by last primitive operation */

  (*rslt_image)=CloneImage(image,0,0,MagickTrue,exception);
  if ((*rslt_image) == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass((*rslt_image),DirectClass,exception) == MagickFalse)
    return(MagickFalse);

  changed=MorphologyPrimitiveDirect((*rslt_image),method,kernel,exception);

  ReportDirectMorphology(verbose,method,changed);

  if ( changed < 0 )
    return(MagickFalse);

  PreserveVoronoiAlpha(method,image,rslt_image,exception);
  return(MagickTrue);
}

typedef struct _MorphologyCompoundFinish
{
  MorphologyMethod
    method;

  MagickBooleanType
    verbose;

  Image
    *curr_image;

  const Image
    *image;

  ExceptionInfo
    *exception;
} MorphologyCompoundFinish;

static Image *FinishCompoundMorphology(const MorphologyCompoundFinish *finish,
  Image *save_image)
{
  const MorphologyMethod
    method = finish->method;

  const MagickBooleanType
    verbose = finish->verbose;

  Image
    *const curr_image = finish->curr_image;

  const Image
    *const image = finish->image;

  ExceptionInfo
    *const exception = finish->exception;

  switch( method ) {
    case EdgeOutMorphology:
    case EdgeInMorphology:
    case TopHatMorphology:
    case BottomHatMorphology:
      ReportCompoundDifference(verbose,method,
        "\n%s: Difference with original image");
      (void) CompositeImage(curr_image,image,DifferenceCompositeOp,
        MagickTrue,0,0,exception);
      break;
    case EdgeMorphology:
      ReportCompoundDifference(verbose,method,
        "\n%s: Difference of Dilate and Erode");
      (void) CompositeImage(curr_image,save_image,DifferenceCompositeOp,
        MagickTrue,0,0,exception);
      save_image = DestroyImage(save_image); /* finished with save image */
      break;
    default:
      break;
  }
  return(save_image);
}

typedef struct _MorphologyKernelResult
{
  const KernelInfo
    *kernel;

  CompositeOperator
    rslt_compose;

  MagickBooleanType
    verbose;

  KernelInfo
    *this_kernel;

  const Image
    *image;

  ExceptionInfo
    *exception;
} MorphologyKernelResult;

static void SaveResultForCompose(const MagickBooleanType verbose,
  const Image *image,Image **rslt_image,Image **curr_image)
{
  ReportSaveForCompose(verbose);
  (*rslt_image) = (*curr_image);
  (*curr_image) = (Image *) image;  /* continue with original image */
}

static void CollectKernelResult(const MorphologyKernelResult *result,
  Image **rslt_image,Image **curr_image)
{
  const KernelInfo
    *const kernel = result->kernel;

  const CompositeOperator
    rslt_compose = result->rslt_compose;

  const MagickBooleanType
    verbose = result->verbose;

  KernelInfo
    *const this_kernel = result->this_kernel;

  const Image
    *const image = result->image;

  ExceptionInfo
    *const exception = result->exception;

  MorphologyKernelCompose
    kernel_compose;

  if ( kernel->next == (KernelInfo *) NULL )
    (*rslt_image) = (*curr_image);   /* just return the resulting image */
  else if ( rslt_compose == NoCompositeOp )
    { ReportKernelIteration(verbose,this_kernel);
      (*rslt_image) = (*curr_image); /* return result, and re-iterate */
    }
  else if ( (*rslt_image) == (Image *) NULL)
    SaveResultForCompose(verbose,image,rslt_image,curr_image);
  else
    { /* Add the new 'current' result to the composition
      **
      ** The removal of any 'Sync' channel flag in the Image Composition
      ** below ensures the mathematical compose method is applied in a
      ** purely mathematical way, and only to the selected channels.
      ** IE: Turn off SVG composition 'alpha blending'.
      */
      kernel_compose.verbose=verbose;
      kernel_compose.rslt_compose=rslt_compose;
      kernel_compose.rslt_image=(*rslt_image);
      kernel_compose.image=image;
      kernel_compose.exception=exception;
      (*curr_image)=ComposeKernelResult(&kernel_compose,(*curr_image));
    }
}

typedef struct _MorphologyMethodLoop
{
  const Image
    *image;

  MorphologyMethod
    method;

  const KernelInfo
    *kernel;

  KernelInfo
    *reflected_kernel;

  double
    bias;

  MagickBooleanType
    verbose;

  size_t
    method_limit,
    stage_limit,
    kernel_limit;

  char
    *v_info;

  ExceptionInfo
    *exception;

  CompositeOperator
    rslt_compose;
} MorphologyMethodLoop;

typedef struct _MorphologyApplyTargets
{
  Image
    **curr_image,
    **rslt_image,
    **save_image,
    **work_image;

  size_t
    *count;
} MorphologyApplyTargets;

static void FinishCompoundStages(const MorphologyMethodLoop *loop,
  Image **curr_image,Image **save_image)
{
  MorphologyCompoundFinish
    finish;

  finish.method=loop->method;
  finish.verbose=loop->verbose;
  finish.curr_image=(*curr_image);
  finish.image=loop->image;
  finish.exception=loop->exception;
  (*save_image)=FinishCompoundMorphology(&finish,(*save_image));
}

static void EndKernelReportLine(const MorphologyMethodLoop *loop)
{
  if (loop->verbose != MagickFalse)
    (void) FormatLocaleFile(stderr, "\n");
}

static KernelInfo *NextReflectedKernel(KernelInfo *rflt_kernel)
{
  if ( rflt_kernel != (KernelInfo *) NULL )
    rflt_kernel = rflt_kernel->next;
  return(rflt_kernel);
}

static MagickBooleanType ApplyMorphologyMethodLoop(
  const MorphologyMethodLoop *loop,const MorphologyApplyTargets *targets)
{
  Image
    **const curr_image = targets->curr_image,
    **const rslt_image = targets->rslt_image,
    **const save_image = targets->save_image,
    **const work_image = targets->work_image;

  size_t
    *const count = targets->count;

  KernelInfo
    *norm_kernel,      /* the current normal un-reflected kernel */
    *rflt_kernel,      /* the current reflected kernel (if needed) */
    *this_kernel;      /* the kernel being applied */

  size_t
    method_loop,    /* Loop 1: number of compound method iterations (norm 1) */
    kernel_number,  /* Loop 2: the kernel number being applied */
    method_changed; /* total count of changed over method iteration */

  const MorphologyStageTargets
    stage_targets = { &this_kernel, save_image, curr_image, work_image,
      count, &method_changed };

  /* Loop 1:  iterate the compound method */
  method_loop = 0;
  method_changed = 1;
  while ( method_loop < loop->method_limit && method_changed > 0 ) {
    method_loop++;
    method_changed = 0;

    /* Loop 2:  iterate over each kernel in a multi-kernel list */
    norm_kernel = (KernelInfo *) loop->kernel;
    this_kernel = (KernelInfo *) loop->kernel;
    rflt_kernel = loop->reflected_kernel;

    kernel_number = 0;
    while ( norm_kernel != NULL ) {
      const MorphologyStages
        stages = { loop->image, loop->method, norm_kernel, rflt_kernel,
          loop->bias, loop->verbose, method_loop, kernel_number,
          loop->stage_limit, loop->kernel_limit, loop->v_info,
          loop->exception };

      MorphologyKernelResult
        kernel_result;

      /* Loop 3: Compound Morphology Staging - Select Primitive to apply */
      if (ApplyMorphologyStages(&stages,&stage_targets) == MagickFalse)
        return(MagickFalse);

      /*  Final Post-processing for some Compound Methods
      **
      ** The removal of any 'Sync' channel flag in the Image Composition
      ** below ensures the mathematical compose method is applied in a
      ** purely mathematical way, and only to the selected channels.
      ** Turn off SVG composition 'alpha blending'.
      */
      FinishCompoundStages(loop,curr_image,save_image);

      /* multi-kernel handling:  re-iterate, or compose results */
      kernel_result.kernel=loop->kernel;
      kernel_result.rslt_compose=loop->rslt_compose;
      kernel_result.verbose=loop->verbose;
      kernel_result.this_kernel=this_kernel;
      kernel_result.image=loop->image;
      kernel_result.exception=loop->exception;
      CollectKernelResult(&kernel_result,rslt_image,curr_image);
      EndKernelReportLine(loop);

      /* loop to the next kernel in a multi-kernel list */
      norm_kernel = norm_kernel->next;
      rflt_kernel=NextReflectedKernel(rflt_kernel);
      kernel_number++;
    } /* End Loop 2: Loop over each kernel */

  } /* End Loop 1: compound method interaction */
  return(MagickTrue);
}

typedef struct _MorphologyApplyRelease
{
  const Image
    *image;

  Image
    *curr_image,
    *rslt_image,
    *work_image,
    *save_image;

  KernelInfo
    *reflected_kernel;
} MorphologyApplyRelease;

static void ReleaseCurrentImage(const Image *image,Image *curr_image,
  const Image *rslt_image)
{
  if ( curr_image == rslt_image || curr_image == image )
    curr_image = (Image *) NULL;
  if ( curr_image != (Image *) NULL )
    curr_image = DestroyImage(curr_image);
}

static void ReleaseMorphologyWork(Image *work_image,Image *save_image,
  KernelInfo *reflected_kernel)
{
  if ( work_image != (Image *) NULL )
    work_image = DestroyImage(work_image);
  if ( save_image != (Image *) NULL )
    save_image = DestroyImage(save_image);
  if ( reflected_kernel != (KernelInfo *) NULL )
    reflected_kernel = DestroyKernelInfo(reflected_kernel);
}

static void ReleaseMorphologyApply(const MorphologyApplyRelease *release)
{
  const Image
    *const image = release->image;

  Image
    *curr_image = release->curr_image,
    *const rslt_image = release->rslt_image,
    *work_image = release->work_image,
    *save_image = release->save_image;

  KernelInfo
    *reflected_kernel = release->reflected_kernel;

  ReleaseCurrentImage(image,curr_image,rslt_image);
  ReleaseMorphologyWork(work_image,save_image,reflected_kernel);
}

typedef struct _MorphologyIteratedLimits
{
  size_t
    *method_limit;

  MagickBooleanType
    *special;

  CompositeOperator
    *rslt_compose;

  size_t
    *kernel_limit;
} MorphologyIteratedLimits;

static void SetIteratedMorphologyLimits(const MorphologyMethod method,
  const MorphologyIteratedLimits *limits)
{
  size_t
    *const method_limit = limits->method_limit;

  MagickBooleanType
    *const special = limits->special;

  CompositeOperator
    *const rslt_compose = limits->rslt_compose;

  size_t
    *const kernel_limit = limits->kernel_limit;

  switch( method ) {
    case HitAndMissMorphology:
      (*rslt_compose) = LightenCompositeOp;  /* Union of multi-kernel results */
      magick_fallthrough;
    case ThinningMorphology:
    case ThickenMorphology:
      (*method_limit) = (*kernel_limit);  /* iterate the whole method */
      (*kernel_limit) = 1;             /* do not do kernel iteration  */
      break;
    case DistanceMorphology:
    case VoronoiMorphology:
      (*special) = MagickTrue;         /* use special direct primitive */
      break;
    default:
      break;
  }
}

typedef struct _MorphologyStageLimit
{
  MorphologyMethod
    method;

  size_t
    limit;
} MorphologyStageLimit;

static const MorphologyStageLimit
  MorphologyStageLimits[] =
  {
    { SmoothMorphology, 4 },  /* 4 primitive compound morphology */
    { OpenMorphology, 2 },    /* 2 primitive compound morphology */
    { OpenIntensityMorphology, 2 },
    { TopHatMorphology, 2 },
    { CloseMorphology, 2 },
    { CloseIntensityMorphology, 2 },
    { BottomHatMorphology, 2 },
    { EdgeMorphology, 2 }
  };

static const MorphologyStageLimit *FindMorphologyStageLimit(
  const MorphologyMethod method)
{
  size_t
    i;

  for (i=0; i < (sizeof(MorphologyStageLimits)/
       sizeof(*MorphologyStageLimits)); i++)
    if (MorphologyStageLimits[i].method == method)
      return(MorphologyStageLimits+i);
  return((const MorphologyStageLimit *) NULL);
}

static void DiscardMorphologyResult(Image **curr_image,Image **rslt_image)
{
  if ( (*curr_image) == (*rslt_image) )
    (*curr_image) = (Image *) NULL;
  if ( (*rslt_image) != (Image *) NULL )
    (*rslt_image) = DestroyImage((*rslt_image));
}

static size_t SelectMorphologyStageLimit(const MorphologyMethod method,
  size_t stage_limit,const MorphologyIteratedLimits limits)
{
  const MorphologyStageLimit
    *entry;

  entry=FindMorphologyStageLimit(method);
  if (entry != (const MorphologyStageLimit *) NULL)
    stage_limit=entry->limit;
  else
    SetIteratedMorphologyLimits(method,&limits);
  return(stage_limit);
}

typedef struct _MorphologyApplyStages
{
  const Image
    *image;

  MorphologyMethod
    method;

  const KernelInfo
    *kernel;

  CompositeOperator
    compose;

  double
    bias;

  MagickBooleanType
    special,
    verbose;

  size_t
    method_limit,
    stage_limit,
    kernel_limit;

  char
    *v_info;

  ExceptionInfo
    *exception;

  MorphologyApplyTargets
    targets;
} MorphologyApplyStages;

typedef struct _MorphologyStagesTargets
{
  Image
    **rslt_image;

  KernelInfo
    **reflected_kernel;
} MorphologyStagesTargets;

static MagickBooleanType ApplySpecialMorphology(
  const MorphologyApplyStages *stages,Image **rslt_image)
{
  const MorphologyDirectApply
    direct = { stages->image, stages->method, stages->kernel,
      stages->verbose, stages->exception };

  if (ApplyDirectMorphology(&direct,rslt_image) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType ApplyMorphologyMethodPrimitives(
  const MorphologyApplyStages *stages,CompositeOperator rslt_compose,
  const MorphologyStagesTargets *stage_targets)
{
  Image
    **const rslt_image = stage_targets->rslt_image;

  KernelInfo
    **const reflected_kernel = stage_targets->reflected_kernel;

  const MorphologyApplyTargets
    targets = stages->targets;

  /* Apply special methods with special requirements
  ** For example, single run only, or post-processing requirements
  */
  if ( stages->special != MagickFalse )
    return(ApplySpecialMorphology(stages,rslt_image));

  /* Handle user (caller) specified multi-kernel composition method */
  rslt_compose=ResolveMultiKernelCompose(stages->compose,rslt_compose);

  /* Some methods require a reflected kernel to use with primitives.
   * Create the reflected kernel for those methods. */
  if (AcquireReflectedKernel(stages->method,stages->kernel,reflected_kernel) ==
      MagickFalse)
    return(MagickFalse);

  /* Loops around more primitive morphology methods
  **  erose, dilate, open, close, smooth, edge, etc...
  */
  {
    const MorphologyMethodLoop
      loop = { stages->image, stages->method, stages->kernel,
        (*reflected_kernel), stages->bias, stages->verbose,
        stages->method_limit, stages->stage_limit, stages->kernel_limit,
        stages->v_info, stages->exception, rslt_compose };

    if (ApplyMorphologyMethodLoop(&loop,&targets) == MagickFalse)
      return(MagickFalse);
  }

  return(MagickTrue);
}

static void InitializeMorphologyLimits(const MorphologyMethod method,
  const CompositeOperator compose,const MorphologyIteratedLimits limits,
  size_t *stage_limit)
{
  size_t
    *const method_limit = limits.method_limit;

  MagickBooleanType
    *const special = limits.special;

  CompositeOperator
    *const rslt_compose = limits.rslt_compose;

  (*method_limit) = 1;       /* just do method once, unless otherwise set */
  (*stage_limit) = 1;        /* assume method is not a compound */
  (*special) = MagickFalse;   /* assume it is NOT a direct modify primitive */
  (*rslt_compose) = compose; /* and we are composing multi-kernels as given */
  (*stage_limit)=SelectMorphologyStageLimit(method,(*stage_limit),limits);
}

MagickPrivate Image *MorphologyApply(const Image *image,
  const MorphologyMethod method, const ssize_t iterations,
  const KernelInfo *kernel, const CompositeOperator compose,const double bias,
  ExceptionInfo *exception)
{
  CompositeOperator
    curr_compose;

  Image
    *curr_image,    /* Image we are working with or iterating */
    *work_image,    /* secondary image for primitive iteration */
    *save_image,    /* saved image - for 'edge' method only */
    *rslt_image;    /* resultant image - after multi-kernel handling */

  KernelInfo
    *reflected_kernel; /* A reflected copy of the kernel (if needed) */

  CompositeOperator
    rslt_compose;   /* multi-kernel compose method for results to use */

  MagickBooleanType
    special,        /* do we use a direct modify function? */
    verbose;        /* verbose output of results */

  size_t
    method_limit,   /*         maximum number of compound method iterations */
    stage_limit,    /*         how many primitives are in this compound */
    kernel_limit,   /*         number of times to iterate kernel */
    count;          /* total count of primitive steps applied */

  char
    v_info[MagickPathExtent];

  const MorphologyIteratedLimits
    limits = { &method_limit, &special, &rslt_compose, &kernel_limit };

  const MorphologyApplyTargets
    targets = { &curr_image, &rslt_image, &save_image, &work_image, &count };

  const MorphologyStagesTargets
    stage_targets = { &rslt_image, &reflected_kernel };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(kernel != (KernelInfo *) NULL);
  assert(kernel->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);

  count = 0;      /* number of low-level morphology primitives performed */
  if ( iterations == 0 )
    return((Image *) NULL);   /* null operation - nothing to do! */

  kernel_limit=GetMorphologyKernelLimit(image,iterations);

  verbose = IsStringTrue(GetImageArtifact(image,"debug"));

  /* initialise for cleanup */
  curr_image = (Image *) image;
  curr_compose = image->compose;
  (void) curr_compose;
  work_image = save_image = rslt_image = (Image *) NULL;
  reflected_kernel = (KernelInfo *) NULL;

  /* Initialize specific methods
   * + which loop should use the given iterations
   * + how many primitives make up the compound morphology
   * + multi-kernel compose method to use (by default)
   */
  InitializeMorphologyLimits(method,compose,limits,&stage_limit);

  {
    const MorphologyApplyStages
      stages = { image, method, kernel, compose, bias, special, verbose,
        method_limit, stage_limit, kernel_limit, v_info, exception, targets };

    if (ApplyMorphologyMethodPrimitives(&stages,rslt_compose,&stage_targets) ==
        MagickFalse)
      goto error_cleanup;
  }

  goto exit_cleanup;

  /* Yes goto's are bad, but it makes cleanup lot more efficient */
error_cleanup:
  DiscardMorphologyResult(&curr_image,&rslt_image);
exit_cleanup:
  {
    const MorphologyApplyRelease
      release = { image, curr_image, rslt_image, work_image, save_image,
        reflected_kernel };

    ReleaseMorphologyApply(&release);
  }
  return(rslt_image);
}
