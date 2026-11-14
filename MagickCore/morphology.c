/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    M   M    OOO    RRRR   PPPP   H   H   OOO   L       OOO    GGGG  Y   Y   %
%    MM MM   O   O   R   R  P   P  H   H  O   O  L      O   O  G       Y Y    %
%    M M M   O   O   RRRR   PPPP   HHHHH  O   O  L      O   O  G GGG    Y     %
%    M   M   O   O   R R    P      H   H  O   O  L      O   O  G   G    Y     %
%    M   M    OOO    R  R   P      H   H   OOO   LLLLL   OOO    GGG     Y     %
%                                                                             %
%                                                                             %
%                        MagickCore Morphology Methods                        %
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


/* Integer Factorial Function - for a Binomial kernel */

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     D e s t r o y K e r n e l I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyKernelInfo() frees the memory used by a Convolution/Morphology
%  kernel.
%
%  The format of the DestroyKernelInfo method is:
%
%      KernelInfo *DestroyKernelInfo(KernelInfo *kernel)
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel to be destroyed
%
*/
MagickExport KernelInfo *DestroyKernelInfo(KernelInfo *kernel)
{
  assert(kernel != (KernelInfo *) NULL);
  if (kernel->next != (KernelInfo *) NULL)
    kernel->next=DestroyKernelInfo(kernel->next);
  kernel->values=(MagickRealType *) RelinquishAlignedMemory(kernel->values);
  kernel=(KernelInfo *) RelinquishMagickMemory(kernel);
  return(kernel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     E x p a n d M i r r o r K e r n e l I n f o                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExpandMirrorKernelInfo() takes a single kernel, and expands it into a
%  sequence of 90-degree rotated kernels but providing a reflected 180
%  rotation, before the -/+ 90-degree rotations.
%
%  This special rotation order produces a better, more symmetrical thinning of
%  objects.
%
%  The format of the ExpandMirrorKernelInfo method is:
%
%      void ExpandMirrorKernelInfo(KernelInfo *kernel)
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel
%
% This function is only internal to this module, as it is not finalized,
% especially with regard to non-orthogonal angles, and rotation of larger
% 2D kernels.
*/

#if 0
static void FlopKernelInfo(KernelInfo *kernel)
    { /* Do a Flop by reversing each row. */
      size_t
        y;
      ssize_t
        x,r;
      double
        *k,t;

      for ( y=0, k=kernel->values; y < kernel->height; y++, k+=kernel->width)
        for ( x=0, r=kernel->width-1; x<kernel->width/2; x++, r--)
          t=k[x],  k[x]=k[r],  k[r]=t;

      kernel->x = kernel->width - kernel->x - 1;
      angle = fmod(angle+180.0, 360.0);
    }
#endif



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o r p h o l o g y I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MorphologyImage() applies a user supplied kernel to the image according to
%  the given morphology method.
%
%  This function applies any and all user defined settings before calling
%  the above internal function MorphologyApply().
%
%  User defined settings include...
%    * Output Bias for Convolution and correlation ("-define convolve:bias=??")
%    * Kernel Scale/normalize settings            ("-define convolve:scale=??")
%      This can also includes the addition of a scaled unity kernel.
%    * Show Kernel being applied            ("-define morphology:showKernel=1")
%
%  Other operators that do not want user supplied options interfering,
%  especially "convolve:bias" and "morphology:showKernel" should use
%  MorphologyApply() directly.
%
%  The format of the MorphologyImage method is:
%
%      Image *MorphologyImage(const Image *image,MorphologyMethod method,
%        const ssize_t iterations,KernelInfo *kernel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o method: the morphology method to be applied.
%
%    o iterations: apply the operation this many times (or no change).
%                  A value of -1 means loop until no change found.
%                  How this is applied may depend on the morphology method.
%                  Typically this is a value of 1.
%
%    o kernel: An array of double representing the morphology kernel.
%              Warning: kernel may be normalized for the Convolve method.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static CompositeOperator GetMorphologyCompose(const Image *image,
  CompositeOperator compose,ExceptionInfo *exception)
{
  const char
    *artifact;

  ssize_t
    parse;

  artifact = GetImageArtifact(image,"morphology:compose");
  if ( artifact != (const char *) NULL) {
    parse=ParseCommandOption(MagickComposeOptions,
      MagickFalse,artifact);
    if ( parse < 0 )
      (void) ThrowMagickException(exception,GetMagickModule(),
           OptionWarning,"UnrecognizedComposeOperator","'%s' '%s'",
           "morphology:compose",artifact);
    else
      compose=(CompositeOperator)parse;
  }
  return(compose);
}

static double GetConvolveBias(const Image *image,double bias,
  ExceptionInfo *exception)
{
  const char
    *artifact;

  artifact = GetImageArtifact(image,"convolve:bias");
  if ( artifact != (const char *) NULL) {
    if (IsGeometry(artifact) == MagickFalse)
      (void) ThrowMagickException(exception,GetMagickModule(),
           OptionWarning,"InvalidSetting","'%s' '%s'",
           "convolve:bias",artifact);
    else
      bias=StringToDoubleInterval(artifact,(double) QuantumRange+1.0);
  }
  return(bias);
}

static MagickBooleanType ScaleConvolveKernel(const char *artifact,
  const KernelInfo *kernel,KernelInfo **curr_kernel,ExceptionInfo *exception)
{
  if (IsGeometry(artifact) == MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),
         OptionWarning,"InvalidSetting","'%s' '%s'",
         "convolve:scale",artifact);
  else {
    if ( *curr_kernel == kernel )
      *curr_kernel = CloneKernelInfo(kernel);
    if (*curr_kernel == (KernelInfo *) NULL)
      return(MagickFalse);
    ScaleGeometryKernelInfo(*curr_kernel, artifact);
  }
  return(MagickTrue);
}

MagickExport Image *MorphologyImage(const Image *image,
  const MorphologyMethod method,const ssize_t iterations,
  const KernelInfo *kernel,ExceptionInfo *exception)
{
  const char
    *artifact;

  CompositeOperator
    compose;

  double
    bias;

  Image
    *morphology_image;

  KernelInfo
    *curr_kernel;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  curr_kernel = (KernelInfo *) kernel;
  bias=0.0;
  compose = UndefinedCompositeOp;  /* use default for method */

  /* Apply Convolve/Correlate Normalization and Scaling Factors.
   * This is done BEFORE the ShowKernelInfo() function is called so that
   * users can see the results of the 'option:convolve:scale' option.
   */
  if ( method == ConvolveMorphology || method == CorrelateMorphology ) {
      /* Get the bias value as it will be needed */
      bias=GetConvolveBias(image,bias,exception);

      /* Scale kernel according to user wishes */
      artifact = GetImageArtifact(image,"convolve:scale");
      if ( artifact != (const char *) NULL ) {
        if (ScaleConvolveKernel(artifact,kernel,&curr_kernel,exception) == MagickFalse)
          return((Image *) NULL);
      }
    }

  /* display the (normalized) kernel via stderr */
  artifact=GetImageArtifact(image,"morphology:showKernel");
  if (IsStringTrue(artifact) != MagickFalse)
    ShowKernelInfo(curr_kernel);

  /* Override the default handling of multi-kernel morphology results
   * If 'Undefined' use the default method
   * If 'None' (default for 'Convolve') re-iterate previous result
   * Otherwise merge resulting images using compose method given.
   * Default for 'HitAndMiss' is 'Lighten'.
   */
  compose=GetMorphologyCompose(image,compose,exception);
  /* Apply the Morphology */
  morphology_image = MorphologyApply(image,method,iterations,
    curr_kernel,compose,bias,exception);

  /* Cleanup and Exit */
  if ( curr_kernel != kernel )
    curr_kernel=DestroyKernelInfo(curr_kernel);
  return(morphology_image);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S c a l e G e o m e t r y K e r n e l I n f o                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ScaleGeometryKernelInfo() takes a geometry argument string, typically
%  provided as a  "-set option:convolve:scale {geometry}" user setting,
%  and modifies the kernel according to the parsed arguments of that setting.
%
%  The first argument (and any normalization flags) are passed to
%  ScaleKernelInfo() to scale/normalize the kernel.  The second argument
%  is then passed to UnityAddKernelInfo() to add a scaled unity kernel
%  into the scaled/normalized kernel.
%
%  The format of the ScaleGeometryKernelInfo method is:
%
%      void ScaleGeometryKernelInfo(KernelInfo *kernel,
%        const double scaling_factor,const MagickStatusType normalize_flags)
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel to modify
%
%    o geometry:
%             The geometry string to parse, typically from the user provided
%             "-set option:convolve:scale {geometry}" setting.
%
*/
MagickExport void ScaleGeometryKernelInfo (KernelInfo *kernel,
  const char *geometry)
{
  MagickStatusType
    flags;

  GeometryInfo
    args;

  SetGeometryInfo(&args);
  flags = ParseGeometry(geometry, &args);

#if 0
  /* For Debugging Geometry Input */
  (void) FormatLocaleFile(stderr, "Geometry = 0x%04X : %lg x %lg %+lg %+lg\n",
       flags, args.rho, args.sigma, args.xi, args.psi );
#endif

  if ( (flags & PercentValue) != 0 )      /* Handle Percentage flag*/
    args.rho *= 0.01,  args.sigma *= 0.01;

  if ( (flags & RhoValue) == 0 )          /* Set Defaults for missing args */
    args.rho = 1.0;
  if ( (flags & SigmaValue) == 0 )
    args.sigma = 0.0;

  /* Scale/Normalize the input kernel */
  ScaleKernelInfo(kernel, args.rho, (GeometryFlags) flags);

  /* Add Unity Kernel, for blending with original */
  if ( (flags & SigmaValue) != 0 )
    UnityAddKernelInfo(kernel, args.sigma);

  return;
}
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S c a l e K e r n e l I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ScaleKernelInfo() scales the given kernel list by the given amount, with or
%  without normalization of the sum of the kernel values (as per given flags).
%
%  By default (no flags given) the values within the kernel is scaled
%  directly using given scaling factor without change.
%
%  If either of the two 'normalize_flags' are given the kernel will first be
%  normalized and then further scaled by the scaling factor value given.
%
%  Kernel normalization ('normalize_flags' given) is designed to ensure that
%  any use of the kernel scaling factor with 'Convolve' or 'Correlate'
%  morphology methods will fall into -1.0 to +1.0 range.  Note that for
%  non-HDRI versions of IM this may cause images to have any negative results
%  clipped, unless some 'bias' is used.
%
%  More specifically.  Kernels which only contain positive values (such as a
%  'Gaussian' kernel) will be scaled so that those values sum to +1.0,
%  ensuring a 0.0 to +1.0 output range for non-HDRI images.
%
%  For Kernels that contain some negative values, (such as 'Sharpen' kernels)
%  the kernel will be scaled by the absolute of the sum of kernel values, so
%  that it will generally fall within the +/- 1.0 range.
%
%  For kernels whose values sum to zero, (such as 'Laplacian' kernels) kernel
%  will be scaled by just the sum of the positive values, so that its output
%  range will again fall into the  +/- 1.0 range.
%
%  For special kernels designed for locating shapes using 'Correlate', (often
%  only containing +1 and -1 values, representing foreground/background
%  matching) a special normalization method is provided to scale the positive
%  values separately to those of the negative values, so the kernel will be
%  forced to become a zero-sum kernel better suited to such searches.
%
%  WARNING: Correct normalization of the kernel assumes that the '*_range'
%  attributes within the kernel structure have been correctly set during the
%  kernels creation.
%
%  NOTE: The values used for 'normalize_flags' have been selected specifically
%  to match the use of geometry options, so that '!' means NormalizeValue, '^'
%  means CorrelateNormalizeValue.  All other GeometryFlags values are ignored.
%
%  The format of the ScaleKernelInfo method is:
%
%      void ScaleKernelInfo(KernelInfo *kernel, const double scaling_factor,
%               const MagickStatusType normalize_flags )
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel
%
%    o scaling_factor:
%             multiply all values (after normalization) by this factor if not
%             zero.  If the kernel is normalized regardless of any flags.
%
%    o normalize_flags:
%             GeometryFlags defining normalization method to use.
%             specifically: NormalizeValue, CorrelateNormalizeValue,
%                           and/or PercentValue
%
*/
static double GetKernelNormalizeScale(const KernelInfo *kernel)
{
  double
    pos_scale;

  if ( fabs(kernel->positive_range + kernel->negative_range) >= MagickEpsilon )
    /* non-zero-summing kernel (generally positive) */
    pos_scale = fabs(kernel->positive_range + kernel->negative_range);
  else
    /* zero-summing kernel */
    pos_scale = kernel->positive_range;
  return(pos_scale);
}

static void ScaleKernelValues(KernelInfo *kernel,const double pos_scale,
  const double neg_scale)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) (kernel->width*kernel->height); i++)
    if (!IsNaN(kernel->values[i]))
      kernel->values[i] *= (kernel->values[i] >= 0) ? pos_scale : neg_scale;
}

static void ApplyKernelScaling(KernelInfo *kernel,
  const double scaling_factor,const double pos_scale,const double neg_scale)
{
  ScaleKernelValues(kernel,pos_scale,neg_scale);

  /* convolution output range */
  kernel->positive_range *= pos_scale;
  kernel->negative_range *= neg_scale;
  /* maximum and minimum values in kernel */
  kernel->maximum *= (kernel->maximum >= 0.0) ? pos_scale : neg_scale;
  kernel->minimum *= (kernel->minimum >= 0.0) ? pos_scale : neg_scale;

  /* swap kernel settings if user's scaling factor is negative */
  if ( scaling_factor < MagickEpsilon ) {
    double t;
    t = kernel->positive_range;
    kernel->positive_range = kernel->negative_range;
    kernel->negative_range = t;
    t = kernel->maximum;
    kernel->maximum = kernel->minimum;
    kernel->minimum = 1;
  }

  return;
}

MagickExport void ScaleKernelInfo(KernelInfo *kernel,
  const double scaling_factor,const GeometryFlags normalize_flags)
{
  double
    pos_scale,
    neg_scale;

  /* do the other kernels in a multi-kernel list first */
  if ( kernel->next != (KernelInfo *) NULL)
    ScaleKernelInfo(kernel->next, scaling_factor, normalize_flags);

  /* Normalization of Kernel */
  pos_scale = 1.0;
  if ( (normalize_flags&NormalizeValue) != 0 ) {
    pos_scale = GetKernelNormalizeScale(kernel);
  }
  /* Force kernel into a normalized zero-summing kernel */
  if ( (normalize_flags&CorrelateNormalizeValue) != 0 ) {
    pos_scale = ( fabs(kernel->positive_range) >= MagickEpsilon )
                 ? kernel->positive_range : 1.0;
    neg_scale = ( fabs(kernel->negative_range) >= MagickEpsilon )
                 ? -kernel->negative_range : 1.0;
  }
  else
    neg_scale = pos_scale;

  /* finalize scaling_factor for positive and negative components */
  pos_scale = scaling_factor/pos_scale;
  neg_scale = scaling_factor/neg_scale;

  ApplyKernelScaling(kernel,scaling_factor,pos_scale,neg_scale);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S h o w K e r n e l I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShowKernelInfo() outputs the details of the given kernel definition to
%  standard error, generally due to a users 'morphology:showKernel' option
%  request.
%
%  The format of the ShowKernel method is:
%
%      void ShowKernelInfo(const KernelInfo *kernel)
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel
%
*/
static void ShowKernelValues(const KernelInfo *k)
{
  size_t
    i, u, v;

  for (i=v=0; v < k->height; v++) {
    (void) FormatLocaleFile(stderr, "%2lu:", (unsigned long) v );
    for (u=0; u < k->width; u++, i++)
      if (IsNaN(k->values[i]))
        (void) FormatLocaleFile(stderr," %*s", GetMagickPrecision()+3, "nan");
      else
        (void) FormatLocaleFile(stderr," %*.*lg", GetMagickPrecision()+3,
            GetMagickPrecision(), (double) k->values[i]);
    (void) FormatLocaleFile(stderr,"\n");
  }
}

static void ShowKernelSum(const KernelInfo *k)
{
  if ( fabs(k->positive_range+k->negative_range) < MagickEpsilon )
    (void) FormatLocaleFile(stderr, " (Zero-Summing)\n");
  else if ( fabs(k->positive_range+k->negative_range-1.0) < MagickEpsilon )
    (void) FormatLocaleFile(stderr, " (Normalized)\n");
  else
    (void) FormatLocaleFile(stderr, " (Sum %.*lg)\n",
        GetMagickPrecision(), k->positive_range+k->negative_range);
}

MagickPrivate void ShowKernelInfo(const KernelInfo *kernel)
{
  const KernelInfo
    *k;

  size_t
    c;

  for (c=0, k=kernel;  k != (KernelInfo *) NULL;  c++, k=k->next ) {

    (void) FormatLocaleFile(stderr, "Kernel");
    if ( kernel->next != (KernelInfo *) NULL )
      (void) FormatLocaleFile(stderr, " #%lu", (unsigned long) c );
    (void) FormatLocaleFile(stderr, " \"%s",
          CommandOptionToMnemonic(MagickKernelOptions, k->type) );
    if ( fabs(k->angle) >= MagickEpsilon )
      (void) FormatLocaleFile(stderr, "@%lg", k->angle);
    (void) FormatLocaleFile(stderr, "\" of size %lux%lu%+ld%+ld",(unsigned long)
      k->width,(unsigned long) k->height,(long) k->x,(long) k->y);
    (void) FormatLocaleFile(stderr,
          " with values from %.*lg to %.*lg\n",
          GetMagickPrecision(), k->minimum,
          GetMagickPrecision(), k->maximum);
    (void) FormatLocaleFile(stderr, "Forming a output range from %.*lg to %.*lg",
          GetMagickPrecision(), k->negative_range,
          GetMagickPrecision(), k->positive_range);
    ShowKernelSum(k);
    ShowKernelValues(k);
  }
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     Z e r o K e r n e l N a n s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ZeroKernelNans() replaces any special 'nan' value that may be present in
%  the kernel with a zero value.  This is typically done when the kernel will
%  be used in special hardware (GPU) convolution processors, to simply
%  matters.
%
%  The format of the ZeroKernelNans method is:
%
%      void ZeroKernelNans (KernelInfo *kernel)
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel
%
*/
MagickPrivate void ZeroKernelNans(KernelInfo *kernel)
{
  size_t
    i;

  /* do the other kernels in a multi-kernel list first */
  if (kernel->next != (KernelInfo *) NULL)
    ZeroKernelNans(kernel->next);

  for (i=0; i < (kernel->width*kernel->height); i++)
    if (IsNaN(kernel->values[i]))
      kernel->values[i]=0.0;

  return;
}
