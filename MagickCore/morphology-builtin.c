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
%               BBBB   U   U  IIIII  L      TTTTT  IIIII  N   N               %
%               B   B  U   U    I    L        T      I    NN  N               %
%               BBBB   U   U    I    L        T      I    N N N               %
%               B   B  U   U    I    L        T      I    N  NN               %
%               BBBB    UUU   IIIII  LLLLL    T    IIIII  N   N               %
%                                                                             %
%                                                                             %
%                MagickCore Built-In Morphology Kernel Methods                %
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
#include "MagickCore/morphology-apply-kernel-private.h"
#include "MagickCore/morphology-builtin-shape-private.h"

static KernelInfo *AcquireChebyshevKernel(KernelInfo *kernel,
  const GeometryInfo *args)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  if (args->rho < 1.0)
    kernel->width = kernel->height = 3;  /* default radius = 1 */
  else
    kernel->width = kernel->height = CastDoubleToSizeT(args->rho)*2+1;
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;

  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      kernel->positive_range += ( kernel->values[i] =
          args->sigma*MagickMax(fabs((double)u),fabs((double)v)) );
  kernel->maximum = kernel->values[0];
  return(kernel);
}

static void SetGaussianKernelValues(KernelInfo *kernel,const double sigma)
{
  double
    A, B;

  ssize_t
    i,
    u,
    v;

  if ( sigma > MagickEpsilon )
    { A = 1.0/(2.0*sigma*sigma);  /* simplify loop expressions */
      B = (double) (1.0/(Magick2PI*sigma*sigma));
      for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
        for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
            kernel->values[i] = exp(-((double)(u*u+v*v))*A)*B;
    }
  else /* limiting case - a unity (normalized Dirac) kernel */
    { (void) memset(kernel->values,0, (size_t)
        kernel->width*kernel->height*sizeof(*kernel->values));
      kernel->values[kernel->x+kernel->y*(ssize_t) kernel->width] = 1.0;
    }
}

static void SubtractGaussianKernelValues(KernelInfo *kernel,
  const double sigma2)
{
  double
    sigma,
    A, B;

  ssize_t
    i,
    u,
    v;

  if ( sigma2 > MagickEpsilon )
    { sigma = sigma2;                /* simplify loop expressions */
      A = 1.0/(2.0*sigma*sigma);
      B = (double) (1.0/(Magick2PI*sigma*sigma));
      for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
        for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
          kernel->values[i] -= exp(-((double)(u*u+v*v))*A)*B;
    }
  else /* limiting case - a unity (normalized Dirac) kernel */
    kernel->values[kernel->x+kernel->y*(ssize_t) kernel->width] -= 1.0;
}

static void SetLoGKernelLoop(KernelInfo *kernel,const double A,
  const double B)
{
  double
    R;

  ssize_t
    i,
    u,
    v;

  for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      { R = ((double)(u*u+v*v))*A;
        kernel->values[i] = (1-R)*exp(-R)*B;
      }
}

static void SetLoGKernelValues(KernelInfo *kernel,const double sigma)
{
  double
    A, B;

  A = 1.0/(2.0*sigma*sigma);  /* simplify loop expressions */
  B = (double) (1.0/(MagickPI*sigma*sigma*sigma*sigma));
  SetLoGKernelLoop(kernel,A,B);
}

static void SetUnityKernelValues(KernelInfo *kernel)
{
  (void) memset(kernel->values,0, (size_t)
    kernel->width*kernel->height*sizeof(*kernel->values));
  kernel->values[kernel->x+kernel->y*(ssize_t) kernel->width] = 1.0;
}

static void SetLoGOrUnityKernelValues(KernelInfo *kernel,const double sigma)
{
  if ( sigma > MagickEpsilon )
    SetLoGKernelValues(kernel,sigma);
  else /* special case - generate a unity kernel */
    SetUnityKernelValues(kernel);
}

static MagickBooleanType IsGaussianSigmaWidest(const KernelInfoType type,
  const double sigma,const double sigma2)
{
  return(((type != DoGKernel) || (sigma >= sigma2)) ? MagickTrue :
    MagickFalse);
}

static void SetGaussianFamilyValues(KernelInfo *kernel,
  const KernelInfoType type,const double sigma,const double sigma2)
{
  if ( type == GaussianKernel || type == DoGKernel )
    { /* Calculate a Gaussian,  OR positive half of a DoG */
      SetGaussianKernelValues(kernel,sigma);
    }

  if ( type == DoGKernel )
    { /* Subtract a Negative Gaussian for "Difference of Gaussian" */
      SubtractGaussianKernelValues(kernel,sigma2);
    }

  if ( type == LoGKernel )
    { /* Calculate a Laplacian of a Gaussian - Or Mexican Hat */
      SetLoGOrUnityKernelValues(kernel,sigma);
    }
}

static void NormalizeGaussianKernel(KernelInfo *kernel)
{
  CalcKernelMetaData(kernel);  /* the other kernel meta-data */
  ScaleKernelInfo(kernel, 1.0, CorrelateNormalizeValue);
}

static KernelInfo *AcquireGaussianKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args)
{
  double
    sigma = fabs(args->sigma),
    sigma2 = fabs(args->xi);

  if ( args->rho >= 1.0 )
    kernel->width = CastDoubleToSizeT(args->rho)*2+1;
  else if (IsGaussianSigmaWidest(type,sigma,sigma2) != MagickFalse)
    kernel->width = GetOptimalKernelWidth2D(args->rho,sigma);
  else
    kernel->width = GetOptimalKernelWidth2D(args->rho,sigma2);
  kernel->height = kernel->width;
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;
  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  /* WARNING: The following generates a 'sampled gaussian' kernel.
   * What we really want is a 'discrete gaussian' kernel.
   *
   * How to do this is I don't know, but appears to be basied on the
   * Error Function 'erf()' (integral of a gaussian)
   */

  SetGaussianFamilyValues(kernel,type,sigma,sigma2);

  /* Note the above kernels may have been 'clipped' by a user defined
  ** radius, producing a smaller (darker) kernel.  Also for very small
  ** sigma's (> 0.1) the central value becomes larger than one, and thus
  ** producing a very bright kernel.
  **
  ** Normalization will still be needed.
  */

  /* Normalize the 2D Gaussian Kernel
  **
  ** NB: a CorrelateNormalize performs a normal Normalize if
  ** there are no negative values.
  */
  NormalizeGaussianKernel(kernel);

  return(kernel);
}

static KernelInfo *AcquireNamedEdgeKernel(const KernelInfoType type,
  const GeometryInfo *args,const char *kernel_array)
{
  KernelInfo
    *kernel;

  kernel=ParseKernelArray(kernel_array);
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  RotateKernelInfo(kernel, args->rho);
  return(kernel);
}

typedef struct _KernelRadius
{
  double
    minimum;

  size_t
    default_width;
} KernelRadius;

static void SetKernelRadius(KernelInfo *kernel,const GeometryInfo *args,
  const KernelRadius *radius)
{
  const double
    minimum = radius->minimum;

  const size_t
    default_width = radius->default_width;

  if (args->rho < minimum)
    kernel->width = kernel->height = default_width;
  else
    kernel->width = kernel->height = CastDoubleToSizeT(args->rho)*2+1;
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;
}

typedef enum
{
  PlusKernelShape,
  CrossKernelShape
} PlusKernelSide;

static void SetPlusOrCrossKernelValues(KernelInfo *kernel,
  const GeometryInfo *args,const double nan,const PlusKernelSide side)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      kernel->values[i] = ((side == PlusKernelShape) ? (u == 0 || v == 0) :
        (u == v || u == -v)) ? args->sigma : nan;
}

static inline KernelInfo *AcquirePlusOrCrossKernel(KernelInfo *kernel,
  const GeometryInfo *args,const double nan,const PlusKernelSide side)
{
  const KernelRadius
    radius = { 1.0, 5 };  /* default radius 2 */

  SetKernelRadius(kernel,args,&radius);

  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  /* set all kernel values along axises to given scale */
  SetPlusOrCrossKernelValues(kernel,args,nan,side);
  kernel->minimum = kernel->maximum = args->sigma;   /* a flat shape */
  kernel->positive_range = args->sigma*(kernel->width*2.0 - 1.0);
  return(kernel);
}

typedef enum
{
  ManhattanKernelDistance,
  EuclideanKernelDistance
} DistanceKernelSide;

static void SetManhattanOrEuclideanValues(KernelInfo *kernel,
  const GeometryInfo *args,const DistanceKernelSide side)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      kernel->positive_range += ( kernel->values[i] =
        ((side == ManhattanKernelDistance) ?
        args->sigma*(labs((long) u)+labs((long) v)) :
        args->sigma*sqrt((double) (u*u+v*v))) );
}

static inline KernelInfo *AcquireManhattanOrEuclideanKernel(KernelInfo *kernel,
  const GeometryInfo *args,const DistanceKernelSide side)
{
  const KernelRadius
    radius = { 1.0, 3 };  /* default radius = 1 */

  SetKernelRadius(kernel,args,&radius);

  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  SetManhattanOrEuclideanValues(kernel,args,side);
  kernel->maximum = kernel->values[0];
  return(kernel);
}

static KernelInfo *AcquireOctagonalKernel(KernelInfo *kernel,
  const GeometryInfo *args)
{
  const KernelRadius
    radius = { 2.0, 5 };  /* default/minimum radius = 2 */

  ssize_t
    i;

  ssize_t
    u,
    v;

  SetKernelRadius(kernel,args,&radius);

  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      {
        double
          r1 = MagickMax(fabs((double)u),fabs((double)v)),
          r2 = floor((double)(labs((long)u)+labs((long)v)+1)/1.5);
        kernel->positive_range += kernel->values[i] =
                  args->sigma*MagickMax(r1,r2);
      }
  kernel->maximum = kernel->values[0];
  return(kernel);
}

static KernelInfo *AcquireCornersKernel(const KernelInfoType type,
  ExceptionInfo *exception)
{
  KernelInfo
    *kernel;

  kernel=AcquireKernelInfo("ThinSE:87",exception);
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  ExpandRotateKernelInfo(kernel, 90.0); /* Expand 90 degree rotations */
  return(kernel);
}

static KernelInfo *AcquireDistanceKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args)
{
  switch(type) {
      /*
        Distance Measuring Kernels
      */
      case ChebyshevKernel:
        return(AcquireChebyshevKernel(kernel,args));
      case ManhattanKernel:
        return(AcquireManhattanOrEuclideanKernel(kernel,args,
          ManhattanKernelDistance));
      case OctagonalKernel:
        return(AcquireOctagonalKernel(kernel,args));
    case EuclideanKernel:
      return(AcquireManhattanOrEuclideanKernel(kernel,args,
        EuclideanKernelDistance));
    default:
      {
        /* No-Op Kernel - Basically just a single pixel on its own */
        kernel=ParseKernelArray("1:1");
        if (kernel == (KernelInfo *) NULL)
          return(kernel);
        kernel->type = UndefinedKernel;
        break;
      }
      break;
  }
  return(kernel);
}

static KernelInfo *AcquireLineOrThinningKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,ExceptionInfo *exception)
{
  switch(type) {
      case LineJunctionsKernel:
        return(AcquireLineJunctionsKernel(kernel,type,args,exception));
      case RidgesKernel:
        return(AcquireRidgesKernel(kernel,type,args));
      case ConvexHullKernel:
        return(AcquireConvexHullKernel(kernel,type));
      case SkeletonKernel:
        return(AcquireSkeletonKernel(kernel,type,args,exception));
      case ThinSEKernel:
        return(AcquireThinSEKernel(kernel,type,args));
    default:
      return(AcquireDistanceKernel(kernel,type,args));
  }
}

typedef struct _BuiltInKernelInputs
{
  const GeometryInfo
    *args;

  double
    nan;

  ExceptionInfo
    *exception;
} BuiltInKernelInputs;

static KernelInfo *AcquireHitAndMissKernel(KernelInfo *kernel,
  const KernelInfoType type,const BuiltInKernelInputs *inputs)
{
  const GeometryInfo
    *args = inputs->args;

  const double
    nan = inputs->nan;

  ExceptionInfo
    *exception = inputs->exception;

  switch(type) {
      /*
        HitAndMiss Kernels
      */
      case RingKernel:
      case PeaksKernel:
        return(AcquireRingKernel(kernel,type,args,nan));
      case EdgesKernel:
        return(AcquireEdgesKernel(kernel,type,exception));
      case CornersKernel:
        return(AcquireCornersKernel(type,exception));
      case DiagonalsKernel:
        return(AcquireDiagonalsKernel(kernel,type,args));
      case LineEndsKernel:
        return(AcquireLineEndsKernel(kernel,type,args,exception));
    default:
      return(AcquireLineOrThinningKernel(kernel,type,args,exception));
  }
}

static KernelInfo *AcquireCrossShapedKernel(KernelInfo *kernel,
  const KernelInfoType type,const BuiltInKernelInputs *inputs)
{
  const GeometryInfo
    *args = inputs->args;

  const double
    nan = inputs->nan;

  switch(type) {
      case PlusKernel:
        return(AcquirePlusOrCrossKernel(kernel,args,nan,PlusKernelShape));
      case CrossKernel:
        return(AcquirePlusOrCrossKernel(kernel,args,nan,CrossKernelShape));
    default:
      return(AcquireHitAndMissKernel(kernel,type,inputs));
  }
}

static KernelInfo *AcquireAreaShapedKernel(KernelInfo *kernel,
  const KernelInfoType type,const BuiltInKernelInputs *inputs)
{
  const GeometryInfo
    *args = inputs->args;

  const double
    nan = inputs->nan;

  switch(type) {
    /*
      Boolean or Shaped Kernels
    */
    case DiamondKernel:
      return(AcquireDiamondKernel(kernel,args,nan));
    case SquareKernel:
    case RectangleKernel:
      return(AcquireRectangleKernel(kernel,type,args));
      case OctagonKernel:
        return(AcquireOctagonKernel(kernel,args,nan));
      case DiskKernel:
        return(AcquireDiskKernel(kernel,args,nan));
    default:
      return(AcquireCrossShapedKernel(kernel,type,inputs));
  }
}

static KernelInfo *AcquireEdgeDetectionKernel(KernelInfo *kernel,
  const KernelInfoType type,const BuiltInKernelInputs *inputs)
{
  const GeometryInfo
    *args = inputs->args;

  ExceptionInfo
    *exception = inputs->exception;

  switch(type) {
    case SobelKernel: /* Simple Sobel Kernel */
      return(AcquireNamedEdgeKernel(type,args,"3: 1,0,-1  2,0,-2  1,0,-1"));
    case RobertsKernel:
      return(AcquireNamedEdgeKernel(type,args,"3: 0,0,0  1,-1,0  0,0,0"));
    case PrewittKernel:
      return(AcquireNamedEdgeKernel(type,args,"3: 1,0,-1  1,0,-1  1,0,-1"));
    case CompassKernel:
      return(AcquireNamedEdgeKernel(type,args,"3: 1,1,-1  1,-2,-1  1,1,-1"));
    case KirschKernel:
      return(AcquireNamedEdgeKernel(type,args,"3: 5,-3,-3  5,0,-3  5,-3,-3"));
    case FreiChenKernel:
      /* Direction is set to be left to right positive */
      /* http://www.math.tau.ac.il/~turkel/notes/edge_detectors.pdf -- RIGHT? */
      /* http://ltswww.epfl.ch/~courstiv/exos_labos/sol3.pdf -- WRONG? */
      return(AcquireFreiChenKernel(kernel,type,args,exception));

    default:
      return(AcquireAreaShapedKernel(kernel,type,inputs));
  }
}

static void AddRankedBlurGaussian(KernelInfo *kernel,const ssize_t v,
  const double alpha,const double beta)
{
  ssize_t
    u;

  for ( u=-v; u <= v; u++) {
    kernel->values[(u+v)/KernelRank] +=
                    exp(-((double)(u*u))*alpha)*beta;
  }
}

static void SetBlurKernelWidth(KernelInfo *kernel,const GeometryInfo *args,
  const double sigma)
{
  if ( args->rho >= 1.0 )
    kernel->width = CastDoubleToSizeT(args->rho)*2+1;
  else
    kernel->width = GetOptimalKernelWidth1D(args->rho,sigma);
}

static void SetBlurKernelGeometry(KernelInfo *kernel)
{
  kernel->height = 1;
  kernel->x = (ssize_t) (kernel->width-1)/2;
  kernel->y = 0;
  kernel->negative_range = kernel->positive_range = 0.0;
}

static KernelInfo *AcquireBlurKernel(KernelInfo *kernel,
  const GeometryInfo *args)
{
  ssize_t
    v;

  double
    sigma = fabs(args->sigma),
    alpha, beta;

  SetBlurKernelWidth(kernel,args,sigma);
  SetBlurKernelGeometry(kernel);
  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

#if 1
#define KernelRank 3
  /* Formula derived from GetBlurKernel() in "effect.c" (plus bug fix).
  ** It generates a gaussian 3 times the width, and compresses it into
  ** the expected range.  This produces a closer normalization of the
  ** resulting kernel, especially for very low sigma values.
  ** As such while wierd it is prefered.
  **
  ** I am told this method originally came from Photoshop.
  **
  ** A properly normalized curve is generated (apart from edge clipping)
  ** even though we later normalize the result (for edge clipping)
  ** to allow the correct generation of a "Difference of Blurs".
  */

  /* initialize */
  v = (ssize_t) (kernel->width*KernelRank-1)/2; /* start/end points to fit range */
  (void) memset(kernel->values,0, (size_t)
    kernel->width*kernel->height*sizeof(*kernel->values));
  /* Calculate a Positive 1D Gaussian */
  if ( sigma > MagickEpsilon )
    { sigma *= KernelRank;               /* simplify loop expressions */
      alpha = 1.0/(2.0*sigma*sigma);
      beta= (double) (1.0/(MagickSQ2PI*sigma ));
      AddRankedBlurGaussian(kernel,v,alpha,beta);
    }
  else /* special case - generate a unity kernel */
    kernel->values[kernel->x+kernel->y*(ssize_t) kernel->width] = 1.0;
#else
  /* Direct calculation without curve averaging
     This is equivalent to a KernelRank of 1 */

  /* Calculate a Positive Gaussian */
  if ( sigma > MagickEpsilon )
    { alpha = 1.0/(2.0*sigma*sigma);    /* simplify loop expressions */
      beta = 1.0/(MagickSQ2PI*sigma);
      for ( i=0, u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
        kernel->values[i] = exp(-((double)(u*u))*alpha)*beta;
    }
  else /* special case - generate a unity kernel */
    { (void) memset(kernel->values,0, (size_t)
        kernel->width*kernel->height*sizeof(*kernel->values));
      kernel->values[kernel->x+kernel->y*kernel->width] = 1.0;
    }
#endif
  /* Note the above kernel may have been 'clipped' by a user defined
  ** radius, producing a smaller (darker) kernel.  Also for very small
  ** sigma's (> 0.1) the central value becomes larger than one, as a
  ** result of not generating a actual 'discrete' kernel, and thus
  ** producing a very bright 'impulse'.
  **
  ** Because of these two factors Normalization is required!
  */

  /* Normalize the 1D Gaussian Kernel
  **
  ** NB: a CorrelateNormalize performs a normal Normalize if
  ** there are no negative values.
  */
  CalcKernelMetaData(kernel);  /* the other kernel meta-data */
  ScaleKernelInfo(kernel, 1.0, CorrelateNormalizeValue);

  /* rotate the 1D kernel by given angle */
  RotateKernelInfo(kernel, args->xi );
  return(kernel);
}

static KernelInfo *AcquireBlurOrNamedKernel(KernelInfo *kernel,
  const KernelInfoType type,const BuiltInKernelInputs *inputs)
{
  const GeometryInfo
    *args = inputs->args;

  switch(type) {
    case BlurKernel:
      return(AcquireBlurKernel(kernel,args));
    case CometKernel:
      return(AcquireCometKernel(kernel,args));
    case BinomialKernel:
      return(AcquireBinomialKernel(kernel,args));

    /*
      Convolution Kernels - Well Known Named Constant Kernels
    */
    case LaplacianKernel:
      return(AcquireLaplacianKernel(kernel,type,args));
    default:
      return(AcquireEdgeDetectionKernel(kernel,type,inputs));
  }
}

static KernelInfo *AcquireKernelOfType(KernelInfo *kernel,
  const KernelInfoType type,const BuiltInKernelInputs *builtin)
{
  BuiltInKernelInputs
    inputs;

  const GeometryInfo
    *args = builtin->args;

  const double
    nan = builtin->nan;

  ExceptionInfo
    *exception = builtin->exception;

  switch(type) {
    /*
      Convolution Kernels
    */
    case UnityKernel:
      return(AcquireUnityKernel(kernel,args));
    case GaussianKernel:
    case DoGKernel:
    case LoGKernel:
      return(AcquireGaussianKernel(kernel,type,args));
    default:
      inputs.args=args;
      inputs.nan=nan;
      inputs.exception=exception;
      return(AcquireBlurOrNamedKernel(kernel,type,&inputs));
  }
}

static const KernelInfoType
  KernelsBuiltWithoutBase[] =
  {
    UndefinedKernel,    /* These should not call this function */
    UserDefinedKernel,
    LaplacianKernel,   /* Named Discrete Convolution Kernels */
    SobelKernel,       /* these are defined using other kernels */
    RobertsKernel,
    PrewittKernel,
    CompassKernel,
    KirschKernel,
    FreiChenKernel,
    EdgesKernel,       /* Hit and Miss kernels */
    CornersKernel,
    DiagonalsKernel,
    LineEndsKernel,
    LineJunctionsKernel,
    RidgesKernel,
    ConvexHullKernel,
    SkeletonKernel,
    ThinSEKernel
  };

static MagickBooleanType IsKernelBuiltWithoutBase(const KernelInfoType type)
{
  size_t
    i;

  for (i=0; i < (sizeof(KernelsBuiltWithoutBase)/
       sizeof(*KernelsBuiltWithoutBase)); i++)
    if (KernelsBuiltWithoutBase[i] == type)
      return(MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType IsBaseKernelNeeded(const KernelInfoType type)
{
  if (IsKernelBuiltWithoutBase(type) != MagickFalse)
    return(MagickFalse); /* A pre-generated kernel is not needed */
  switch(type) {
#if 0
    /* set to 1 to do a compile-time check that we haven't missed anything */
    case UnityKernel:
    case GaussianKernel:
    case DoGKernel:
    case LoGKernel:
    case BlurKernel:
    case CometKernel:
    case BinomialKernel:
    case DiamondKernel:
    case SquareKernel:
    case RectangleKernel:
    case OctagonKernel:
    case DiskKernel:
    case PlusKernel:
    case CrossKernel:
    case RingKernel:
    case PeaksKernel:
    case ChebyshevKernel:
    case ManhattanKernel:
    case OctagonalKernel:
    case EuclideanKernel:
#else
    default:
#endif
      return(MagickTrue);
  }
  return(MagickFalse);
}

static void InitializeBaseKernel(KernelInfo *kernel,const KernelInfoType type)
{
  (void) memset(kernel,0,sizeof(*kernel));
  kernel->minimum = kernel->maximum = kernel->angle = 0.0;
  kernel->negative_range = kernel->positive_range = 0.0;
  kernel->type = type;
  kernel->next = (KernelInfo *) NULL;
  kernel->signature=MagickCoreSignature;
}

MagickExport KernelInfo *AcquireKernelBuiltIn(const KernelInfoType type,
  const GeometryInfo *args,ExceptionInfo *exception)
{
  BuiltInKernelInputs
    builtin;

  KernelInfo
    *kernel;

  double
    nan = sqrt(-1.0);  /* Special Value : Not A Number */

  /* Generate a new empty kernel if needed */
  kernel=(KernelInfo *) NULL;
  switch(type) {
    case UndefinedKernel:    /* These should not call this function */
    case UserDefinedKernel:
      (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
        "InvalidOption","`%s'","Should not call this function");
      return((KernelInfo *) NULL);
    default:
      break;
  }
  if (IsBaseKernelNeeded(type) != MagickFalse)
    {
      /* Generate the base Kernel Structure */
      kernel=(KernelInfo *) AcquireMagickMemory(sizeof(*kernel));
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      InitializeBaseKernel(kernel,type);
    }

  builtin.args=args;
  builtin.nan=nan;
  builtin.exception=exception;
  return(AcquireKernelOfType(kernel,type,&builtin));
}
