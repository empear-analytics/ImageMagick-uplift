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

  MagickCore private methods of morphology-primitive.c: the kernel walks, the
  row and column drivers and MorphologyPrimitive, from the MorphologyPass
  typedef to MorphologyPrimitive; included by that file only.
*/
#ifndef MAGICKCORE_MORPHOLOGY_PRIMITIVE_WALK_PRIVATE_H
#define MAGICKCORE_MORPHOLOGY_PRIMITIVE_WALK_PRIVATE_H

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

static magick_attribute((__always_inline__)) inline void
  SelectMinimumIntensityPixel(const MorphologyIntensityTap *tap,
  const Quantum *magick_restrict *quantum_pixels,double *pixel,double *minimum)
{
  if (tap->intensity < (*minimum))
    {
      (*quantum_pixels)=tap->pixels;
      (*pixel)=(double) tap->pixels[tap->i];
      (*minimum)=tap->intensity;
    }
}

static magick_attribute((__always_inline__)) inline void
  SelectMaximumIntensityPixel(const MorphologyIntensityTap *tap,
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

static magick_attribute((__always_inline__)) inline void
  WalkIntensityKernel(const MorphologyKernelWalk *walk,
  const MorphologyIntensityWalk *select,
  const Quantum *magick_restrict *quantum_pixels,double *pixel)
{
  const MorphologyKernelWalkSide
    side = select->side;

  double
    *const extremum = select->extremum,
    extremum_value = *select->extremum,
    pixel_value = *pixel;

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

  const Quantum
    *magick_restrict selected_pixels = *quantum_pixels;

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
            SelectMinimumIntensityPixel(&intensity_tap,&selected_pixels,
              &pixel_value,&extremum_value) :
            SelectMaximumIntensityPixel(&intensity_tap,&pixel_value,
              &selected_pixels,&extremum_value));
        }
      ((side == ForwardKernelWalk) ? k++ : k--);
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(image->columns-1)*GetPixelChannels(image);
  }
  (*quantum_pixels)=selected_pixels;
  (*pixel)=pixel_value;
  (*extremum)=extremum_value;
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

static ssize_t FinishMorphologyRows(const MorphologyPrimitiveSetup *setup,
  const MagickBooleanType status)
{
  const Image
    *image = setup->image;

  CacheView
    *image_view = setup->image_view,
    *morphology_view = setup->morphology_view;

  size_t
    changed = setup->changed,
    *changes = setup->changes;

  morphology_view=DestroyCacheView(morphology_view);
  image_view=DestroyCacheView(image_view);
  changed=SumMorphologyChanges(changed,changes);
  changes=(size_t *) RelinquishMagickMemory(changes);
  return(status ? (ssize_t) (changed/GetImageChannels(image)) : -1);
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
  return(FinishMorphologyRows(setup,status));
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

#endif
