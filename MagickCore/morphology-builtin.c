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

static inline size_t fact(size_t n)
{
  size_t f,l;
  for(f=1, l=2; l <= n; f=f*l, l++);
  return(f);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c q u i r e K e r n e l B u i l t I n                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireKernelBuiltIn() returned one of the 'named' built-in types of
%  kernels used for special purposes such as gaussian blurring, skeleton
%  pruning, and edge distance determination.
%
%  They take a KernelType, and a set of geometry style arguments, which were
%  typically decoded from a user supplied string, or from a more complex
%  Morphology Method that was requested.
%
%  The format of the AcquireKernelBuiltIn method is:
%
%      KernelInfo *AcquireKernelBuiltIn(const KernelInfoType type,
%           const GeometryInfo args)
%
%  A description of each parameter follows:
%
%    o type: the pre-defined type of kernel wanted
%
%    o args: arguments defining or modifying the kernel
%
%  Convolution Kernels
%
%    Unity
%       The a No-Op or Scaling single element kernel.
%
%    Gaussian:{radius},{sigma}
%       Generate a two-dimensional gaussian kernel, as used by -gaussian.
%       The sigma for the curve is required.  The resulting kernel is
%       normalized,
%
%       If 'sigma' is zero, you get a single pixel on a field of zeros.
%
%       NOTE: that the 'radius' is optional, but if provided can limit (clip)
%       the final size of the resulting kernel to a square 2*radius+1 in size.
%       The radius should be at least 2 times that of the sigma value, or
%       sever clipping and aliasing may result.  If not given or set to 0 the
%       radius will be determined so as to produce the best minimal error
%       result, which is usually much larger than is normally needed.
%
%    LoG:{radius},{sigma}
%        "Laplacian of a Gaussian" or "Mexican Hat" Kernel.
%        The supposed ideal edge detection, zero-summing kernel.
%
%        An alternative to this kernel is to use a "DoG" with a sigma ratio of
%        approx 1.6 (according to wikipedia).
%
%    DoG:{radius},{sigma1},{sigma2}
%        "Difference of Gaussians" Kernel.
%        As "Gaussian" but with a gaussian produced by 'sigma2' subtracted
%        from the gaussian produced by 'sigma1'. Typically sigma2 > sigma1.
%        The result is a zero-summing kernel.
%
%    Blur:{radius},{sigma}[,{angle}]
%       Generates a 1 dimensional or linear gaussian blur, at the angle given
%       (current restricted to orthogonal angles).  If a 'radius' is given the
%       kernel is clipped to a width of 2*radius+1.  Kernel can be rotated
%       by a 90 degree angle.
%
%       If 'sigma' is zero, you get a single pixel on a field of zeros.
%
%       Note that two convolutions with two "Blur" kernels perpendicular to
%       each other, is equivalent to a far larger "Gaussian" kernel with the
%       same sigma value, However it is much faster to apply. This is how the
%       "-blur" operator actually works.
%
%    Comet:{width},{sigma},{angle}
%       Blur in one direction only, much like how a bright object leaves
%       a comet like trail.  The Kernel is actually half a gaussian curve,
%       Adding two such blurs in opposite directions produces a Blur Kernel.
%       Angle can be rotated in multiples of 90 degrees.
%
%       Note that the first argument is the width of the kernel and not the
%       radius of the kernel.
%
%    Binomial:[{radius}]
%       Generate a discrete kernel using a 2 dimensional Pascal's Triangle
%       of values. Used for special forma of image filters.
%
%    # Still to be implemented...
%    #
%    # Filter2D
%    # Filter1D
%    #    Set kernel values using a resize filter, and given scale (sigma)
%    #    Cylindrical or Linear.   Is this possible with an image?
%    #
%
%  Named Constant Convolution Kernels
%
%  All these are unscaled, zero-summing kernels by default. As such for
%  non-HDRI version of ImageMagick some form of normalization, user scaling,
%  and biasing the results is recommended, to prevent the resulting image
%  being 'clipped'.
%
%  The 3x3 kernels (most of these) can be circularly rotated in multiples of
%  45 degrees to generate the 8 angled variants of each of the kernels.
%
%    Laplacian:{type}
%      Discrete Laplacian Kernels, (without normalization)
%        Type 0 :  3x3 with center:8 surrounded by -1  (8 neighbourhood)
%        Type 1 :  3x3 with center:4 edge:-1 corner:0 (4 neighbourhood)
%        Type 2 :  3x3 with center:4 edge:1 corner:-2
%        Type 3 :  3x3 with center:4 edge:-2 corner:1
%        Type 5 :  5x5 laplacian
%        Type 7 :  7x7 laplacian
%        Type 15 : 5x5 LoG (sigma approx 1.4)
%        Type 19 : 9x9 LoG (sigma approx 1.4)
%
%    Sobel:{angle}
%      Sobel 'Edge' convolution kernel (3x3)
%          | -1, 0, 1 |
%          | -2, 0,-2 |
%          | -1, 0, 1 |
%
%    Roberts:{angle}
%      Roberts convolution kernel (3x3)
%          |  0, 0, 0 |
%          | -1, 1, 0 |
%          |  0, 0, 0 |
%
%    Prewitt:{angle}
%      Prewitt Edge convolution kernel (3x3)
%          | -1, 0, 1 |
%          | -1, 0, 1 |
%          | -1, 0, 1 |
%
%    Compass:{angle}
%      Prewitt's "Compass" convolution kernel (3x3)
%          | -1, 1, 1 |
%          | -1,-2, 1 |
%          | -1, 1, 1 |
%
%    Kirsch:{angle}
%      Kirsch's "Compass" convolution kernel (3x3)
%          | -3,-3, 5 |
%          | -3, 0, 5 |
%          | -3,-3, 5 |
%
%    FreiChen:{angle}
%      Frei-Chen Edge Detector is based on a kernel that is similar to
%      the Sobel Kernel, but is designed to be isotropic. That is it takes
%      into account the distance of the diagonal in the kernel.
%
%          |   1,     0,   -1     |
%          | sqrt(2), 0, -sqrt(2) |
%          |   1,     0,   -1     |
%
%    FreiChen:{type},{angle}
%
%      Frei-Chen Pre-weighted kernels...
%
%        Type 0:  default un-normalized version shown above.
%
%        Type 1: Orthogonal Kernel (same as type 11 below)
%          |   1,     0,   -1     |
%          | sqrt(2), 0, -sqrt(2) | / 2*sqrt(2)
%          |   1,     0,   -1     |
%
%        Type 2: Diagonal form of Kernel...
%          |   1,     sqrt(2),    0     |
%          | sqrt(2),   0,     -sqrt(2) | / 2*sqrt(2)
%          |   0,    -sqrt(2)    -1     |
%
%      However this kernel is als at the heart of the FreiChen Edge Detection
%      Process which uses a set of 9 specially weighted kernel.  These 9
%      kernels not be normalized, but directly applied to the image. The
%      results is then added together, to produce the intensity of an edge in
%      a specific direction.  The square root of the pixel value can then be
%      taken as the cosine of the edge, and at least 2 such runs at 90 degrees
%      from each other, both the direction and the strength of the edge can be
%      determined.
%
%        Type 10: All 9 of the following pre-weighted kernels...
%
%        Type 11: |   1,     0,   -1     |
%                 | sqrt(2), 0, -sqrt(2) | / 2*sqrt(2)
%                 |   1,     0,   -1     |
%
%        Type 12: | 1, sqrt(2), 1 |
%                 | 0,   0,     0 | / 2*sqrt(2)
%                 | 1, sqrt(2), 1 |
%
%        Type 13: | sqrt(2), -1,    0     |
%                 |  -1,      0,    1     | / 2*sqrt(2)
%                 |   0,      1, -sqrt(2) |
%
%        Type 14: |    0,     1, -sqrt(2) |
%                 |   -1,     0,     1    | / 2*sqrt(2)
%                 | sqrt(2), -1,     0    |
%
%        Type 15: | 0, -1, 0 |
%                 | 1,  0, 1 | / 2
%                 | 0, -1, 0 |
%
%        Type 16: |  1, 0, -1 |
%                 |  0, 0,  0 | / 2
%                 | -1, 0,  1 |
%
%        Type 17: |  1, -2,  1 |
%                 | -2,  4, -2 | / 6
%                 | -1, -2,  1 |
%
%        Type 18: | -2, 1, -2 |
%                 |  1, 4,  1 | / 6
%                 | -2, 1, -2 |
%
%        Type 19: | 1, 1, 1 |
%                 | 1, 1, 1 | / 3
%                 | 1, 1, 1 |
%
%      The first 4 are for edge detection, the next 4 are for line detection
%      and the last is to add a average component to the results.
%
%      Using a special type of '-1' will return all 9 pre-weighted kernels
%      as a multi-kernel list, so that you can use them directly (without
%      normalization) with the special "-set option:morphology:compose Plus"
%      setting to apply the full FreiChen Edge Detection Technique.
%
%      If 'type' is large it will be taken to be an actual rotation angle for
%      the default FreiChen (type 0) kernel.  As such  FreiChen:45  will look
%      like a  Sobel:45  but with 'sqrt(2)' instead of '2' values.
%
%      WARNING: The above was layed out as per
%          http://www.math.tau.ac.il/~turkel/notes/edge_detectors.pdf
%      But rotated 90 degrees so direction is from left rather than the top.
%      I have yet to find any secondary confirmation of the above. The only
%      other source found was actual source code at
%          http://ltswww.epfl.ch/~courstiv/exos_labos/sol3.pdf
%      Neither paper defines the kernels in a way that looks logical or
%      correct when taken as a whole.
%
%  Boolean Kernels
%
%    Diamond:[{radius}[,{scale}]]
%       Generate a diamond shaped kernel with given radius to the points.
%       Kernel size will again be radius*2+1 square and defaults to radius 1,
%       generating a 3x3 kernel that is slightly larger than a square.
%
%    Square:[{radius}[,{scale}]]
%       Generate a square shaped kernel of size radius*2+1, and defaulting
%       to a 3x3 (radius 1).
%
%    Octagon:[{radius}[,{scale}]]
%       Generate octagonal shaped kernel of given radius and constant scale.
%       Default radius is 3 producing a 7x7 kernel. A radius of 1 will result
%       in "Diamond" kernel.
%
%    Disk:[{radius}[,{scale}]]
%       Generate a binary disk, thresholded at the radius given, the radius
%       may be a float-point value. Final Kernel size is floor(radius)*2+1
%       square. A radius of 5.3 is the default.
%
%       NOTE: That a low radii Disk kernels produce the same results as
%       many of the previously defined kernels, but differ greatly at larger
%       radii.  Here is a table of equivalences...
%          "Disk:1"    => "Diamond", "Octagon:1", or "Cross:1"
%          "Disk:1.5"  => "Square"
%          "Disk:2"    => "Diamond:2"
%          "Disk:2.5"  => "Octagon"
%          "Disk:2.9"  => "Square:2"
%          "Disk:3.5"  => "Octagon:3"
%          "Disk:4.5"  => "Octagon:4"
%          "Disk:5.4"  => "Octagon:5"
%          "Disk:6.4"  => "Octagon:6"
%       All other Disk shapes are unique to this kernel, but because a "Disk"
%       is more circular when using a larger radius, using a larger radius is
%       preferred over iterating the morphological operation.
%
%    Rectangle:{geometry}
%       Simply generate a rectangle of 1's with the size given. You can also
%       specify the location of the 'control point', otherwise the closest
%       pixel to the center of the rectangle is selected.
%
%       Properly centered and odd sized rectangles work the best.
%
%  Symbol Dilation Kernels
%
%    These kernel is not a good general morphological kernel, but is used
%    more for highlighting and marking any single pixels in an image using,
%    a "Dilate" method as appropriate.
%
%    For the same reasons iterating these kernels does not produce the
%    same result as using a larger radius for the symbol.
%
%    Plus:[{radius}[,{scale}]]
%    Cross:[{radius}[,{scale}]]
%       Generate a kernel in the shape of a 'plus' or a 'cross' with
%       a each arm the length of the given radius (default 2).
%
%       NOTE: "plus:1" is equivalent to a "Diamond" kernel.
%
%    Ring:{radius1},{radius2}[,{scale}]
%       A ring of the values given that falls between the two radii.
%       Defaults to a ring of approximately 3 radius in a 7x7 kernel.
%       This is the 'edge' pixels of the default "Disk" kernel,
%       More specifically, "Ring" -> "Ring:2.5,3.5,1.0"
%
%  Hit and Miss Kernels
%
%    Peak:radius1,radius2
%       Find any peak larger than the pixels the fall between the two radii.
%       The default ring of pixels is as per "Ring".
%    Edges
%       Find flat orthogonal edges of a binary shape
%    Corners
%       Find 90 degree corners of a binary shape
%    Diagonals:type
%       A special kernel to thin the 'outside' of diagonals
%    LineEnds:type
%       Find end points of lines (for pruning a skeleton)
%       Two types of lines ends (default to both) can be searched for
%         Type 0: All line ends
%         Type 1: single kernel for 4-connected line ends
%         Type 2: single kernel for simple line ends
%    LineJunctions
%       Find three line junctions (within a skeleton)
%         Type 0: all line junctions
%         Type 1: Y Junction kernel
%         Type 2: Diagonal T Junction kernel
%         Type 3: Orthogonal T Junction kernel
%         Type 4: Diagonal X Junction kernel
%         Type 5: Orthogonal + Junction kernel
%    Ridges:type
%       Find single pixel ridges or thin lines
%         Type 1: Fine single pixel thick lines and ridges
%         Type 2: Find two pixel thick lines and ridges
%    ConvexHull
%       Octagonal Thickening Kernel, to generate convex hulls of 45 degrees
%    Skeleton:type
%       Traditional skeleton generating kernels.
%         Type 1: Traditional Skeleton kernel (4 connected skeleton)
%         Type 2: HIPR2 Skeleton kernel (8 connected skeleton)
%         Type 3: Thinning skeleton based on a research paper by
%                 Dan S. Bloomberg (Default Type)
%    ThinSE:type
%       A huge variety of Thinning Kernels designed to preserve connectivity.
%       many other kernel sets use these kernels as source definitions.
%       Type numbers are 41-49, 81-89, 481, and 482 which are based on
%       the super and sub notations used in the source research paper.
%
%  Distance Measuring Kernels
%
%    Different types of distance measuring methods, which are used with the
%    a 'Distance' morphology method for generating a gradient based on
%    distance from an edge of a binary shape, though there is a technique
%    for handling a anti-aliased shape.
%
%    See the 'Distance' Morphological Method, for information of how it is
%    applied.
%
%    Chebyshev:[{radius}][x{scale}[%!]]
%       Chebyshev Distance (also known as Tchebychev or Chessboard distance)
%       is a value of one to any neighbour, orthogonal or diagonal. One why
%       of thinking of it is the number of squares a 'King' or 'Queen' in
%       chess needs to traverse reach any other position on a chess board.
%       It results in a 'square' like distance function, but one where
%       diagonals are given a value that is closer than expected.
%
%    Manhattan:[{radius}][x{scale}[%!]]
%       Manhattan Distance (also known as Rectilinear, City Block, or the Taxi
%       Cab distance metric), it is the distance needed when you can only
%       travel in horizontal or vertical directions only.  It is the
%       distance a 'Rook' in chess would have to travel, and results in a
%       diamond like distances, where diagonals are further than expected.
%
%    Octagonal:[{radius}][x{scale}[%!]]
%       An interleaving of Manhattan and Chebyshev metrics producing an
%       increasing octagonally shaped distance.  Distances matches those of
%       the "Octagon" shaped kernel of the same radius.  The minimum radius
%       and default is 2, producing a 5x5 kernel.
%
%    Euclidean:[{radius}][x{scale}[%!]]
%       Euclidean distance is the 'direct' or 'as the crow flys' distance.
%       However by default the kernel size only has a radius of 1, which
%       limits the distance to 'Knight' like moves, with only orthogonal and
%       diagonal measurements being correct.  As such for the default kernel
%       you will get octagonal like distance function.
%
%       However using a larger radius such as "Euclidean:4" you will get a
%       much smoother distance gradient from the edge of the shape. Especially
%       if the image is pre-processed to include any anti-aliasing pixels.
%       Of course a larger kernel is slower to use, and not always needed.
%
%    The first three Distance Measuring Kernels will only generate distances
%    of exact multiples of {scale} in binary images. As such you can use a
%    scale of 1 without loosing any information.  However you also need some
%    scaling when handling non-binary anti-aliased shapes.
%
%    The "Euclidean" Distance Kernel however does generate a non-integer
%    fractional results, and as such scaling is vital even for binary shapes.
%
*/

static inline MagickBooleanType IsKernelOriginOutside(const GeometryInfo *args,
  const KernelInfo *kernel)
{
  return(((args->xi < 0.0) || (args->xi  >= (double) kernel->width) ||
    (args->psi < 0.0) || (args->psi >= (double) kernel->height)) ?
    MagickTrue : MagickFalse);
}

static void SetDiamondKernelValues(KernelInfo *kernel,const GeometryInfo *args,
  const double nan)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      if ( (labs((long) u)+labs((long) v)) <= (long) kernel->x)
        kernel->positive_range += kernel->values[i] = args->sigma;
      else
        kernel->values[i] = nan;
}

static KernelInfo *AcquireDiamondKernel(KernelInfo *kernel,
  const GeometryInfo *args,const double nan)
{
  if (args->rho < 1.0)
    kernel->width = kernel->height = 3;  /* default radius = 1 */
  else
    kernel->width = kernel->height = CastDoubleToSizeT(args->rho)*2+1;
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;

  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  /* set all kernel values within diamond area to scale given */
  SetDiamondKernelValues(kernel,args,nan);
  kernel->minimum = kernel->maximum = args->sigma;   /* a flat shape */
  return(kernel);
}

static KernelInfo *AcquireUnityKernel(KernelInfo *kernel,
  const GeometryInfo *args)
{
  kernel->height = kernel->width = (size_t) 1;
  kernel->x = kernel->y = (ssize_t) 0;
  kernel->values=(MagickRealType *) MagickAssumeAligned(
    AcquireAlignedMemory(1,sizeof(*kernel->values)));
  if (kernel->values == (MagickRealType *) NULL)
    return(DestroyKernelInfo(kernel));
  kernel->maximum = kernel->values[0] = args->rho;
  return(kernel);
}

static void SetCometKernelWidth(KernelInfo *kernel,const GeometryInfo *args,
  const double sigma)
{
  if ( args->rho < 1.0 )
    kernel->width = (GetOptimalKernelWidth1D(args->rho,sigma)-1)/2+1;
  else
    kernel->width = CastDoubleToSizeT(args->rho);
}

static void SetCometUnityKernel(KernelInfo *kernel)
{
  (void) memset(kernel->values,0, (size_t)
    kernel->width*kernel->height*sizeof(*kernel->values));
  kernel->values[kernel->x+kernel->y*(ssize_t) kernel->width] = 1.0;
  kernel->positive_range = 1.0;
}

static void SetCometKernelRange(KernelInfo *kernel)
{
  kernel->minimum = 0.0;
  kernel->maximum = kernel->values[0];
  kernel->negative_range = 0.0;
}

static KernelInfo *AcquireCometKernel(KernelInfo *kernel,
  const GeometryInfo *args)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  double
    sigma = fabs(args->sigma),
    A;

  SetCometKernelWidth(kernel,args,sigma);
  kernel->x = kernel->y = 0;
  kernel->height = 1;
  kernel->negative_range = kernel->positive_range = 0.0;
  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  /* A comet blur is half a 1D gaussian curve, so that the object is
  ** blurred in one direction only.  This may not be quite the right
  ** curve to use so may change in the future. The function must be
  ** normalised after generation, which also resolves any clipping.
  **
  ** As we are normalizing and not subtracting gaussians,
  ** there is no need for a divisor in the gaussian formula
  **
  ** It is less complex
  */
  if ( sigma > MagickEpsilon )
    {
#if 1
#define KernelRank 3
      v = (ssize_t) kernel->width*KernelRank; /* start/end points */
      (void) memset(kernel->values,0, (size_t)
        kernel->width*sizeof(*kernel->values));
      sigma *= KernelRank;            /* simplify the loop expression */
      A = 1.0/(2.0*sigma*sigma);
      /* B = 1.0/(MagickSQ2PI*sigma); */
      for ( u=0; u < v; u++) {
        kernel->values[u/KernelRank] +=
            exp(-((double)(u*u))*A);
        /*  exp(-((double)(i*i))/2.0*sigma*sigma)/(MagickSQ2PI*sigma); */
      }
      for (i=0; i < (ssize_t) kernel->width; i++)
        kernel->positive_range += kernel->values[i];
#else
      A = 1.0/(2.0*sigma*sigma);     /* simplify the loop expression */
      /* B = 1.0/(MagickSQ2PI*sigma); */
      for ( i=0; i < (ssize_t) kernel->width; i++)
        kernel->positive_range +=
          kernel->values[i] = exp(-((double)(i*i))*A);
          /* exp(-((double)(i*i))/2.0*sigma*sigma)/(MagickSQ2PI*sigma); */
#endif
    }
  else /* special case - generate a unity kernel */
    SetCometUnityKernel(kernel);

  SetCometKernelRange(kernel);

  ScaleKernelInfo(kernel, 1.0, NormalizeValue); /* Normalize */
  RotateKernelInfo(kernel, args->xi); /* Rotate by angle */
  return(kernel);
}

static void SetBinomialKernelValues(KernelInfo *kernel,const size_t order_f)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  for ( i=0, v=0; v < (ssize_t)kernel->height; v++)
    {
      size_t
        alpha = order_f / ( fact((size_t) v) * fact(kernel->height-(size_t) v-1) );

      for ( u=0; u < (ssize_t)kernel->width; u++, i++)
        kernel->positive_range += kernel->values[i] = (double)
          (alpha * order_f / ( fact((size_t) u) * fact(kernel->height-(size_t) u-1) ));
    }
}

static KernelInfo *AcquireBinomialKernel(KernelInfo *kernel,
  const GeometryInfo *args)
{
  const size_t
    max_order = (sizeof(size_t) > 4) ? 20 : 12;

  size_t
    order_f;

  if (args->rho < 1.0)
    kernel->width = kernel->height = 3;  /* default radius = 1 */
  else
    kernel->width = kernel->height = CastDoubleToSizeT(args->rho)*2+1;
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;

    /* Check if kernel order (width-1) would overflow fact() */
  if ((kernel->width-1) > max_order)
    return(DestroyKernelInfo(kernel));

  order_f = fact(kernel->width-1);

  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  /* set all kernel values within diamond area to scale given */
  SetBinomialKernelValues(kernel,order_f);
  kernel->minimum = 1.0;
  kernel->maximum = kernel->values[kernel->x+kernel->y*(ssize_t) kernel->width];
  kernel->negative_range = 0.0;
  return(kernel);
}

typedef struct _KeyedKernelArray
{
  int
    key;

  const char
    *definition;
} KeyedKernelArray;

static const KeyedKernelArray
  LaplacianKernelArrays[] =
  {
    { 1, "3: 0,-1,0  -1,4,-1  0,-1,0" }, /* laplacian diamond filter */
    { 2, "3: -2,1,-2  1,4,1  -2,1,-2" },
    { 3, "3: 1,-2,1  -2,4,-2  1,-2,1" },
    /* a 5x5 laplacian */
    { 5,
      "5: -4,-1,0,-1,-4  -1,2,3,2,-1  0,3,4,3,0  -1,2,3,2,-1  -4,-1,0,-1,-4" },
    /* a 7x7 laplacian */
    { 7,
      "7:-10,-5,-2,-1,-2,-5,-10 -5,0,3,4,3,0,-5 -2,3,6,7,6,3,-2 -1,4,7,8,7,4,-1 -2,3,6,7,6,3,-2 -5,0,3,4,3,0,-5 -10,-5,-2,-1,-2,-5,-10" },
    /* a 5x5 LoG (sigma approx 1.4) */
    { 15,
      "5: 0,0,-1,0,0  0,-1,-2,-1,0  -1,-2,16,-2,-1  0,-1,-2,-1,0  0,0,-1,0,0" },
    /* a 9x9 LoG (sigma approx 1.4) */
    /* http://www.cscjournals.org/csc/manuscript/Journals/IJIP/volume3/Issue1/IJIP-15.pdf */
    { 19,
      "9: 0,-1,-1,-2,-2,-2,-1,-1,0  -1,-2,-4,-5,-5,-5,-4,-2,-1  -1,-4,-5,-3,-0,-3,-5,-4,-1  -2,-5,-3,12,24,12,-3,-5,-2  -2,-5,-0,24,40,24,-0,-5,-2  -2,-5,-3,12,24,12,-3,-5,-2  -1,-4,-5,-3,-0,-3,-5,-4,-1  -1,-2,-4,-5,-5,-5,-4,-2,-1  0,-1,-1,-2,-2,-2,-1,-1,0" },
    /* laplacian square filter -- default */
    { 0, "3: -1,-1,-1  -1,8,-1  -1,-1,-1" }
  };

static size_t FindLaplacianKernelArray(const int key)
{
  size_t
    i;

  for (i=0; i < ((sizeof(LaplacianKernelArrays)/
       sizeof(*LaplacianKernelArrays))-1); i++)
    if (LaplacianKernelArrays[i].key == key)
      break;
  return(i);
}

static KernelInfo *AcquireLaplacianKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args)
{
  const int
    key = (int) args->rho;

  size_t
    i;

  i=FindLaplacianKernelArray(key);
  kernel=ParseKernelArray(LaplacianKernelArrays[i].definition);
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  return(kernel);
}

static void RotateOutOfBoundsFreiChenKernel(KernelInfo *kernel,
  const GeometryInfo *args)
{
  if ( args->rho > 30.0 || args->rho < -30.0 )
    /* Rotate by out of bounds 'type' */
    RotateKernelInfo(kernel, args->rho);
}

static KernelInfo *RotateFreiChenKernel(KernelInfo *kernel,
  const GeometryInfo *args)
{
  if ( fabs(args->sigma) >= MagickEpsilon )
    /* Rotate by correctly supplied 'angle' */
    RotateKernelInfo(kernel, args->sigma);
  else
    RotateOutOfBoundsFreiChenKernel(kernel,args);
  return(kernel);
}

static KernelInfo *AcquireFreiChenEdgeOrSetKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,ExceptionInfo *exception)
{
  switch ( (int) args->rho ) {
    default:
    case 0:
      kernel=ParseKernelArray("3: 1,0,-1  2,0,-2  1,0,-1");
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      kernel->values[3] = +(MagickRealType) MagickSQ2;
      kernel->values[5] = -(MagickRealType) MagickSQ2;
      CalcKernelMetaData(kernel);     /* recalculate meta-data */
      break;
    case 2:
      kernel=ParseKernelArray("3: 1,2,0  2,0,-2  0,-2,-1");
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      kernel->values[1] = kernel->values[3]= +(MagickRealType) MagickSQ2;
      kernel->values[5] = kernel->values[7]= -(MagickRealType) MagickSQ2;
      CalcKernelMetaData(kernel);     /* recalculate meta-data */
      ScaleKernelInfo(kernel, (double) (1.0/2.0*MagickSQ2), NoValue);
      break;
    case 10:
    {
      kernel=AcquireKernelInfo("FreiChen:11;FreiChen:12;FreiChen:13;FreiChen:14;FreiChen:15;FreiChen:16;FreiChen:17;FreiChen:18;FreiChen:19",exception);
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      break;
    }
  }
  return(RotateFreiChenKernel(kernel,args));
}

static KernelInfo *AcquireFreiChenEdgeBasisKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,ExceptionInfo *exception)
{
  switch ( (int) args->rho ) {
    default:
      return(AcquireFreiChenEdgeOrSetKernel(kernel,type,args,exception));
    case 1:
    case 11:
      kernel=ParseKernelArray("3: 1,0,-1  2,0,-2  1,0,-1");
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      kernel->values[3] = +(MagickRealType) MagickSQ2;
      kernel->values[5] = -(MagickRealType) MagickSQ2;
      CalcKernelMetaData(kernel);     /* recalculate meta-data */
      ScaleKernelInfo(kernel, (double) (1.0/2.0*MagickSQ2), NoValue);
      break;
    case 12:
      kernel=ParseKernelArray("3: 1,2,1  0,0,0  1,2,1");
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      kernel->values[1] = +(MagickRealType) MagickSQ2;
      kernel->values[7] = +(MagickRealType) MagickSQ2;
      CalcKernelMetaData(kernel);
      ScaleKernelInfo(kernel, (double) (1.0/2.0*MagickSQ2), NoValue);
      break;
  }
  return(RotateFreiChenKernel(kernel,args));
}

typedef struct _FreiChenScaledKernel
{
  int
    key;

  const char
    *definition;

  double
    scale;
} FreiChenScaledKernel;

static const FreiChenScaledKernel
  FreiChenScaledKernels[] =
  {
    { 15, "3: 0,-1,0  1,0,1  0,-1,0", 1.0/2.0 },
    { 16, "3: 1,0,-1  0,0,0  -1,0,1", 1.0/2.0 },
    { 17, "3: 1,-2,1  -2,4,-2  -1,-2,1", 1.0/6.0 },
    { 18, "3: -2,1,-2  1,4,1  -2,1,-2", 1.0/6.0 },
    { 19, "3: 1,1,1  1,1,1  1,1,1", 1.0/3.0 }
  };

static const FreiChenScaledKernel *FindFreiChenScaledKernel(const int key)
{
  size_t
    i;

  for (i=0; i < (sizeof(FreiChenScaledKernels)/
       sizeof(*FreiChenScaledKernels)); i++)
    if (FreiChenScaledKernels[i].key == key)
      return(FreiChenScaledKernels+i);
  return((const FreiChenScaledKernel *) NULL);
}

static KernelInfo *AcquireFreiChenKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,ExceptionInfo *exception)
{
  const FreiChenScaledKernel
    *scaled;

  const int
    key = (int) args->rho;

  switch ( key ) {
    default:
      scaled=FindFreiChenScaledKernel(key);
      if (scaled == (const FreiChenScaledKernel *) NULL)
        return(AcquireFreiChenEdgeBasisKernel(kernel,type,args,exception));
      kernel=ParseKernelArray(scaled->definition);
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      ScaleKernelInfo(kernel, scaled->scale, NoValue);
      break;
    case 13:
      kernel=ParseKernelArray("3: 2,-1,0  -1,0,1  0,1,-2");
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      kernel->values[0] = +(MagickRealType) MagickSQ2;
      kernel->values[8] = -(MagickRealType) MagickSQ2;
      CalcKernelMetaData(kernel);
      ScaleKernelInfo(kernel, (double) (1.0/2.0*MagickSQ2), NoValue);
      break;
    case 14:
      kernel=ParseKernelArray("3: 0,1,-2  -1,0,1  2,-1,0");
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      kernel->values[2] = -(MagickRealType) MagickSQ2;
      kernel->values[6] = +(MagickRealType) MagickSQ2;
      CalcKernelMetaData(kernel);
      ScaleKernelInfo(kernel, (double) (1.0/2.0*MagickSQ2), NoValue);
      break;
  }
  return(RotateFreiChenKernel(kernel,args));
}

static double SetSquareKernelGeometry(KernelInfo *kernel,
  const GeometryInfo *args)
{
  double
    scale;

  if (args->rho < 1.0)
    kernel->width = kernel->height = 3;  /* default radius = 1 */
  else
    kernel->width = kernel->height = CastDoubleToSizeT(args->rho*2+1);
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;
  scale = args->sigma;
  return(scale);
}

static void SetFlatKernelValues(KernelInfo *kernel,const double scale)
{
  ssize_t
    i;

  ssize_t
    u;

  u=(ssize_t) (kernel->width*kernel->height);
  for ( i=0; i < u; i++)
      kernel->values[i] = scale;
  kernel->minimum = kernel->maximum = scale;   /* a flat shape */
  kernel->positive_range = scale*u;
}

static KernelInfo *AcquireRectangleKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args)
{
  double
    scale;
  if ( type == SquareKernel )
    scale = SetSquareKernelGeometry(kernel,args);
  else {
      /* NOTE: user defaults set in "AcquireKernelInfo()" */
      if ( args->rho < 1.0 || args->sigma < 1.0 )
        return(DestroyKernelInfo(kernel));    /* invalid args given */
      kernel->width = CastDoubleToSizeT(args->rho);
      kernel->height = CastDoubleToSizeT(args->sigma);
      if (IsKernelOriginOutside(args,kernel) != MagickFalse)
        return(DestroyKernelInfo(kernel));    /* invalid args given */
      kernel->x = (ssize_t) args->xi;
      kernel->y = (ssize_t) args->psi;
      scale = 1.0;
    }
  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  /* set all kernel values to scale given */
  SetFlatKernelValues(kernel,scale);
  return(kernel);
}

static KernelInfo *AcquireOctagonKernel(KernelInfo *kernel,
  const GeometryInfo *args,const double nan)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  if (args->rho < 1.0)
    kernel->width = kernel->height = 5;  /* default radius = 2 */
  else
    kernel->width = kernel->height = CastDoubleToSizeT(args->rho)*2+1;
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;

  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      if ( (labs((long) u)+labs((long) v)) <=
                ((long)kernel->x + (long)(kernel->x/2)) )
        kernel->positive_range += kernel->values[i] = args->sigma;
      else
        kernel->values[i] = nan;
  kernel->minimum = kernel->maximum = args->sigma;  /* a flat shape */
  return(kernel);
}

static KernelInfo *AcquireDiskKernel(KernelInfo *kernel,
  const GeometryInfo *args,const double nan)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  ssize_t
    limit = (ssize_t)(args->rho*args->rho);

  if (args->rho < 0.4)           /* default radius approx 4.3 */
    kernel->width = kernel->height = 9L, limit = 18L;
  else
    kernel->width = kernel->height = CastDoubleToSizeT(fabs(args->rho))*2+1;
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;

  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  for ( i=0, v=-kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      if ((u*u+v*v) <= limit)
        kernel->positive_range += kernel->values[i] = args->sigma;
      else
        kernel->values[i] = nan;
  kernel->minimum = kernel->maximum = args->sigma;   /* a flat shape */
  return(kernel);
}

static MagickBooleanType IsWithinRingLimits(const ssize_t radius,
  const ssize_t limit1,const ssize_t limit2)
{
  return((limit1 < radius && radius <= limit2) ? MagickTrue : MagickFalse);
}

static void SetPeaksKernelCenter(KernelInfo *kernel,
  const KernelInfoType type)
{
  if ( type == PeaksKernel ) {
    /* set the central point in the middle */
    kernel->values[kernel->x+kernel->y*(ssize_t) kernel->width] = 1.0;
    kernel->positive_range = 1.0;
    kernel->maximum = 1.0;
  }
}

static void SetRingKernelOrigin(KernelInfo *kernel)
{
  kernel->height = kernel->width;
  kernel->x = kernel->y = (ssize_t) (kernel->width-1)/2;
}

static KernelInfo *AcquireRingKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,const double nan)
{
  ssize_t
    i;

  ssize_t
    u,
    v;

  ssize_t
    limit1,
    limit2,
    scale;

  if (args->rho < args->sigma)
    {
      kernel->width = CastDoubleToSizeT(args->sigma)*2+1;
      limit1 = (ssize_t)(args->rho*args->rho);
      limit2 = (ssize_t)(args->sigma*args->sigma);
    }
  else
    {
      kernel->width = CastDoubleToSizeT(args->rho)*2+1;
      limit1 = (ssize_t)(args->sigma*args->sigma);
      limit2 = (ssize_t)(args->rho*args->rho);
    }
  if ( limit2 <= 0 )
    kernel->width = 7L, limit1 = 7L, limit2 = 11L;

  SetRingKernelOrigin(kernel);
  if (AcquireKernelValues(kernel) == MagickFalse)
    return(DestroyKernelInfo(kernel));

  /* set a ring of points of 'scale' ( 0.0 for PeaksKernel ) */
  scale = (ssize_t) (( type == PeaksKernel) ? 0.0 : args->xi);
  for ( i=0, v= -kernel->y; v <= (ssize_t)kernel->y; v++)
    for ( u=-kernel->x; u <= (ssize_t)kernel->x; u++, i++)
      { ssize_t radius=u*u+v*v;
        if (IsWithinRingLimits(radius,limit1,limit2) != MagickFalse)
          kernel->positive_range += kernel->values[i] = (double) scale;
        else
          kernel->values[i] = nan;
      }
  kernel->minimum = kernel->maximum = (double) scale;
  SetPeaksKernelCenter(kernel,type);
  return(kernel);
}

static KernelInfo *AcquireEdgesKernel(KernelInfo *kernel,
  const KernelInfoType type,ExceptionInfo *exception)
{
  kernel=AcquireKernelInfo("ThinSE:482",exception);
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  ExpandMirrorKernelInfo(kernel); /* mirror expansion of kernels */
  return(kernel);
}

static KernelInfo *AcquireDiagonalsKernelPair(const KernelInfoType type)
{
  KernelInfo
    *kernel,
    *new_kernel;

  kernel=ParseKernelArray("3: 0,0,0  0,-,1  1,1,-");
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  new_kernel=ParseKernelArray("3: 0,0,1  0,-,1  0,1,-");
  if (new_kernel == (KernelInfo *) NULL)
    return(DestroyKernelInfo(kernel));
  new_kernel->type = type;
  LastKernelInfo(kernel)->next = new_kernel;
  ExpandMirrorKernelInfo(kernel);
  return(kernel);
}

static KernelInfo *
  SetTypeAndRotateKernel(KernelInfo *,const KernelInfoType,
    const GeometryInfo *);

static KernelInfo *AcquireDiagonalsKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args)
{
  switch ( (int) args->rho ) {
    case 0:
    default:
      return(AcquireDiagonalsKernelPair(type));
    case 1:
      kernel=ParseKernelArray("3: 0,0,0  0,-,1  1,1,-");
      break;
    case 2:
      kernel=ParseKernelArray("3: 0,0,1  0,-,1  0,1,-");
      break;
  }
  return(SetTypeAndRotateKernel(kernel,type,args));
}

static KernelInfo *SetTypeAndRotateKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args)
{
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  RotateKernelInfo(kernel, args->sigma);
  return(kernel);
}

static KernelInfo *AcquireLineEndsKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,ExceptionInfo *exception)
{
  /* Kernels for finding the end of thin lines */
  switch ( (int) args->rho ) {
    case 0:
    default:
      /* set of kernels to find all end of lines */
      return(AcquireKernelInfo("LineEnds:1>;LineEnds:2>",exception));
    case 1:
      /* kernel for 4-connected line ends - no rotation */
      kernel=ParseKernelArray("3: 0,0,-  0,1,1  0,0,-");
      break;
  case 2:
      /* kernel to add for 8-connected lines - no rotation */
      kernel=ParseKernelArray("3: 0,0,0  0,1,0  0,0,1");
      break;
  case 3:
      /* kernel to add for orthogonal line ends - does not find corners */
      kernel=ParseKernelArray("3: 0,0,0  0,1,1  0,0,0");
      break;
  case 4:
      /* traditional line end - fails on last T end */
      kernel=ParseKernelArray("3: 0,0,0  0,1,-  0,0,-");
      break;
  }
  return(SetTypeAndRotateKernel(kernel,type,args));
}

static const KeyedKernelArray
  LineJunctionsKernelArrays[] =
  {
    /* Y Junction */
    { 1, "3: 1,-,1  -,1,-  -,1,-" },
    /* Diagonal T Junctions */
    { 2, "3: 1,-,-  -,1,-  1,-,1" },
    /* Orthogonal T Junctions */
    { 3, "3: -,-,-  1,1,1  -,1,-" },
    /* Diagonal X Junctions */
    { 4, "3: 1,-,1  -,1,-  1,-,1" },
    /* Orthogonal X Junctions - minimal diamond kernel */
    { 5, "3: -,1,-  1,1,1  -,1,-" }
  };

static KernelInfo *AcquireLineJunctionsKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,ExceptionInfo *exception)
{
  const int
    key = (int) args->rho;

  size_t
    i;

  /* kernels for finding the junctions of multiple lines */
  for (i=0; i < (sizeof(LineJunctionsKernelArrays)/
       sizeof(*LineJunctionsKernelArrays)); i++)
    if (LineJunctionsKernelArrays[i].key == key)
      break;
  if (i == (sizeof(LineJunctionsKernelArrays)/
       sizeof(*LineJunctionsKernelArrays)))
    /* set of kernels to find all line junctions */
    return(AcquireKernelInfo("LineJunctions:1@;LineJunctions:2>",exception));
  kernel=ParseKernelArray(LineJunctionsKernelArrays[i].definition);
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  RotateKernelInfo(kernel, args->sigma);
  return(kernel);
}

static const char
  *const ThickRidgeKernelArrays[] =
  {
    "4x3+1+1:0,1,1,- -,1,1,- -,1,1,0",
    "4x3+2+1:0,1,1,- -,1,1,- -,1,1,0",
    "4x3+1+1:-,1,1,0 -,1,1,- 0,1,1,-",
    "4x3+2+1:-,1,1,0 -,1,1,- 0,1,1,-",
    "3x4+1+1:0,-,- 1,1,1 1,1,1 -,-,0",
    "3x4+1+2:0,-,- 1,1,1 1,1,1 -,-,0",
    "3x4+1+1:-,-,0 1,1,1 1,1,1 0,-,-",
    "3x4+1+2:-,-,0 1,1,1 1,1,1 0,-,-"
  };

static MagickBooleanType AppendThickRidgeKernels(KernelInfo *kernel,
  const KernelInfoType type)
{
  KernelInfo
    *new_kernel;

  size_t
    j;

  for (j=0; j < (sizeof(ThickRidgeKernelArrays)/
       sizeof(*ThickRidgeKernelArrays)); j++)
  {
    new_kernel=ParseKernelArray(ThickRidgeKernelArrays[j]);
    if (new_kernel == (KernelInfo *) NULL)
      return(MagickFalse);
    new_kernel->type = type;
    LastKernelInfo(kernel)->next = new_kernel;
  }
  return(MagickTrue);
}

static KernelInfo *AcquireRidgesKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args)
{
  /* Ridges - Ridge finding kernels */
  switch ( (int) args->rho ) {
    case 1:
    default:
      kernel=ParseKernelArray("3x1:0,1,0");
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      ExpandRotateKernelInfo(kernel, 90.0); /* 2 rotated kernels (symmetrical) */
      break;
    case 2:
      kernel=ParseKernelArray("4x1:0,1,1,0");
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      ExpandRotateKernelInfo(kernel, 90.0); /* 4 rotated kernels */

      /* Kernels to find a stepped 'thick' line, 4 rotates + mirrors */
      /* Unfortunately we can not yet rotate a non-square kernel */
      /* But then we can't flip a non-symmetrical kernel either */
      if (AppendThickRidgeKernels(kernel,type) == MagickFalse)
        return(DestroyKernelInfo(kernel));
      break;
  }
  return(kernel);
}

static KernelInfo *AppendConvexHullMirrors(KernelInfo *kernel,
  const KernelInfoType type)
{
  KernelInfo
    *new_kernel;

  new_kernel=ParseKernelArray("3: 1,1,1  1,0,-  -,-,0");
  if (new_kernel == (KernelInfo *) NULL)
    return(DestroyKernelInfo(kernel));
  new_kernel->type = type;
  ExpandRotateKernelInfo(new_kernel, 90.0);
  LastKernelInfo(kernel)->next = new_kernel;
  return(kernel);
}

static KernelInfo *AcquireConvexHullKernel(KernelInfo *kernel,
  const KernelInfoType type)
{
  /* first set of 8 kernels */
  kernel=ParseKernelArray("3: 1,1,-  1,0,-  1,-,0");
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  ExpandRotateKernelInfo(kernel, 90.0);
  /* append the mirror versions too - no flip function yet */
  return(AppendConvexHullMirrors(kernel,type));
}

static KernelInfo *AcquireCyclicSkeletonKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,ExceptionInfo *exception)
{
  switch ( (int) args->rho ) {
    case 1:
    default:
      /* Traditional Skeleton...
      ** A cyclically rotated single kernel
      */
      kernel=AcquireKernelInfo("ThinSE:482",exception);
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      kernel->type = type;
      ExpandRotateKernelInfo(kernel, 45.0); /* 8 rotations */
      break;
    case 2:
      /* HIPR Variation of the cyclic skeleton
      ** Corners of the traditional method made more forgiving,
      ** but the retain the same cyclic order.
      */
      kernel=AcquireKernelInfo("ThinSE:482; ThinSE:87x90;",exception);
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      if (kernel->next == (KernelInfo *) NULL)
        return(DestroyKernelInfo(kernel));
      kernel->type = type;
      kernel->next->type = type;
      ExpandRotateKernelInfo(kernel, 90.0); /* 4 rotations of the 2 kernels */
      break;
   }
  return(kernel);
}

static void SetTypesAndMirrorKernels(KernelInfo *kernel,
  const KernelInfoType type)
{
  kernel->type = type;
  kernel->next->type = type;
  kernel->next->next->type = type;
  ExpandMirrorKernelInfo(kernel);
}

static KernelInfo *AcquireSkeletonKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args,ExceptionInfo *exception)
{
  switch ( (int) args->rho ) {
    default:
      return(AcquireCyclicSkeletonKernel(kernel,type,args,exception));
    case 3:
      /* Dan Bloomberg Skeleton, from his paper on 3x3 thinning SE's
      ** "Connectivity-Preserving Morphological Image Transformations"
      ** by Dan S. Bloomberg, available on Leptonica, Selected Papers,
      **   http://www.leptonica.com/papers/conn.pdf
      */
      kernel=AcquireKernelInfo("ThinSE:41; ThinSE:42; ThinSE:43",
        exception);
      if (kernel == (KernelInfo *) NULL)
        return(kernel);
      if (kernel->next == (KernelInfo *) NULL)
        return(DestroyKernelInfo(kernel));
      if (kernel->next->next == (KernelInfo *) NULL)
        return(DestroyKernelInfo(kernel));
      SetTypesAndMirrorKernels(kernel,type); /* 12 kernels total */
      break;
   }
  return(kernel);
}

static const KeyedKernelArray
  ThinSEKernelArrays[] =
  {
    /* SE for 4-connected thinning */
    { 41, "3: -,-,1  0,-,1  -,-,1" }, /* SE_4_1 */
    { 42, "3: -,-,1  0,-,1  -,0,-" }, /* SE_4_2 */
    { 43, "3: -,0,-  0,-,1  -,-,1" }, /* SE_4_3 */
    { 44, "3: -,0,-  0,-,1  -,0,-" }, /* SE_4_4 */
    { 45, "3: -,0,1  0,-,1  -,0,-" }, /* SE_4_5 */
    { 46, "3: -,0,-  0,-,1  -,0,1" }, /* SE_4_6 */
    { 47, "3: -,1,1  0,-,1  -,0,-" }, /* SE_4_7 */
    { 48, "3: -,-,1  0,-,1  0,-,1" }, /* SE_4_8 */
    { 49, "3: 0,-,1  0,-,1  -,-,1" }, /* SE_4_9 */
    /* SE for 8-connected thinning - negatives of the above */
    { 81, "3: -,1,-  0,-,1  -,1,-" }, /* SE_8_0 */
    { 82, "3: -,1,-  0,-,1  0,-,-" }, /* SE_8_2 */
    { 83, "3: 0,-,-  0,-,1  -,1,-" }, /* SE_8_3 */
    { 84, "3: 0,-,-  0,-,1  0,-,-" }, /* SE_8_4 */
    { 85, "3: 0,-,1  0,-,1  0,-,-" }, /* SE_8_5 */
    { 86, "3: 0,-,-  0,-,1  0,-,1" }, /* SE_8_6 */
    { 87, "3: -,1,-  0,-,1  0,0,-" }, /* SE_8_7 */
    { 88, "3: -,1,-  0,-,1  0,1,-" }, /* SE_8_8 */
    { 89, "3: 0,1,-  0,-,1  -,1,-" }, /* SE_8_9 */
    /* Special combined SE kernels */
    { 423, "3: -,-,1  0,-,-  -,0,-" }, /* SE_4_2 , SE_4_3 Combined Kernel */
    { 823, "3: -,1,-  -,-,1  0,-,-" }, /* SE_8_2 , SE_8_3 Combined Kernel */
    /* SE_48_1 - General Connected Corner Kernel */
    { 481, "3: -,1,1  0,-,1  0,0,-" },
    { 482, "3: 0,-,1  0,-,1  0,-,1" } /* SE_48_2 - General Edge Kernel */
  };

static KernelInfo *AcquireThinSEKernel(KernelInfo *kernel,
  const KernelInfoType type,const GeometryInfo *args)
{
  const int
    key = (int) args->rho;

  size_t
    i;

  /* Special kernels for general thinning, while preserving connections
  ** "Connectivity-Preserving Morphological Image Transformations"
  ** by Dan S. Bloomberg, available on Leptonica, Selected Papers,
  **   http://www.leptonica.com/papers/conn.pdf
  ** And
  **   http://tpgit.github.com/Leptonica/ccthin_8c_source.html
  **
  ** Note kernels do not specify the origin pixel, allowing them
  ** to be used for both thickening and thinning operations.
  */
  for (i=0; i < ((sizeof(ThinSEKernelArrays)/
       sizeof(*ThinSEKernelArrays))-1); i++)
    if (ThinSEKernelArrays[i].key == key)
      break;
  kernel=ParseKernelArray(ThinSEKernelArrays[i].definition);
  if (kernel == (KernelInfo *) NULL)
    return(kernel);
  kernel->type = type;
  RotateKernelInfo(kernel, args->sigma);
  return(kernel);
}

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
