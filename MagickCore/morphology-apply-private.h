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

  MagickCore private methods shared by the morphology methods: RotateKernelInfo
  and the kernel rotation helpers it calls.
*/
#ifndef MAGICKCORE_MORPHOLOGY_APPLY_PRIVATE_H
#define MAGICKCORE_MORPHOLOGY_APPLY_PRIVATE_H

#include "MagickCore/morphology.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Method prototypes.
*/
static void
  RotateKernelInfo(KernelInfo *, double);

static inline void RotateKernelOriginBy45Degrees(KernelInfo *kernel)
{
  if ( kernel->x != 1 || kernel->y != 1 ) {
    ssize_t x,y;
    x = (ssize_t) kernel->x-1;
    y = (ssize_t) kernel->y-1;
         if ( x == y  ) x = 0;
    else if ( x == 0  ) x = -y;
    else if ( x == -y ) y = 0;
    else if ( y == 0  ) y = x;
    kernel->x = (ssize_t) x+1;
    kernel->y = (ssize_t) y+1;
  }
}

static inline void RotateKernelRingBy45Degrees(KernelInfo *kernel)
{
  double t  = kernel->values[0];
  kernel->values[0] = kernel->values[3];
  kernel->values[3] = kernel->values[6];
  kernel->values[6] = kernel->values[7];
  kernel->values[7] = kernel->values[8];
  kernel->values[8] = kernel->values[5];
  kernel->values[5] = kernel->values[2];
  kernel->values[2] = kernel->values[1];
  kernel->values[1] = t;
}

typedef struct _KernelRotation
{
  KernelInfo
    *kernel;

  double
    angle;
} KernelRotation;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     R o t a t e K e r n e l I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RotateKernelInfo() rotates the kernel by the angle given.
%
%  Currently it is restricted to 90 degree angles, of either 1D kernels
%  or square kernels. And 'circular' rotations of 45 degrees for 3x3 kernels.
%  It will ignore useless rotations for specific 'named' built-in kernels.
%
%  The format of the RotateKernelInfo method is:
%
%      void RotateKernelInfo(KernelInfo *kernel, double angle)
%
%  A description of each parameter follows:
%
%    o kernel: the Morphology/Convolution kernel
%
%    o angle: angle to rotate in degrees
%
% This function is currently internal to this module only, but can be exported
% to other modules if needed.
*/
static inline double RotateKernelBy45Degrees(const KernelRotation *rotation)
{
  KernelInfo
    *const kernel = rotation->kernel;

  double
    angle = rotation->angle;

  /* Rotate a 3x3 square by 45 degree angle */
  RotateKernelRingBy45Degrees(kernel);
  /* rotate non-centered origin */
  RotateKernelOriginBy45Degrees(kernel);
  angle = fmod(angle+315.0, 360.0);  /* angle reduced 45 degrees */
  kernel->angle = fmod(kernel->angle+45.0, 360.0);
  return(angle);
}

static inline double TurnTransposedKernelAngle(const KernelRotation *rotation)
{
  KernelInfo
    *const kernel = rotation->kernel;

  double
    angle = rotation->angle;

  if ( kernel->width == 1 ) {
    angle = fmod(angle+270.0, 360.0);     /* angle reduced 90 degrees */
    kernel->angle = fmod(kernel->angle+90.0, 360.0);
  } else {
    angle = fmod(angle+90.0, 360.0);   /* angle increased 90 degrees */
    kernel->angle = fmod(kernel->angle+270.0, 360.0);
  }
  return(angle);
}

static inline double TransposeKernel(KernelInfo *kernel,double angle)
{
  /* Do a transpose of a 1 dimensional kernel,
  ** which results in a fast 90 degree rotation of some type.
  */
  ssize_t
    t;
  t = (ssize_t) kernel->width;
  kernel->width = kernel->height;
  kernel->height = (size_t) t;
  t = kernel->x;
  kernel->x = kernel->y;
  kernel->y = t;
  {
    const KernelRotation
      rotation = { kernel, angle };

    angle=TurnTransposedKernelAngle(&rotation);
  }
  return(angle);
}

static inline void RotateSquareKernelValues(KernelInfo *kernel)
{
  ssize_t
    i,j,x,y;

  MagickRealType
    *k,t;

  k=kernel->values;
  for( i=0, x=(ssize_t) kernel->width-1;  i<=x;   i++, x--)
    for( j=0, y=(ssize_t) kernel->height-1;  j<y;   j++, y--)
      { t                    = k[i+j*(ssize_t) kernel->width];
        k[i+j*(ssize_t) kernel->width] = k[j+x*(ssize_t) kernel->width];
        k[j+x*(ssize_t) kernel->width] = k[x+y*(ssize_t) kernel->width];
        k[x+y*(ssize_t) kernel->width] = k[y+i*(ssize_t) kernel->width];
        k[y+i*(ssize_t) kernel->width] = t;
      }
}

static inline void RotateSquareKernelOrigin(KernelInfo *kernel)
{
  ssize_t x,y;
  x = (ssize_t) (kernel->x*2-(ssize_t) kernel->width+1);
  y = (ssize_t) (kernel->y*2-(ssize_t) kernel->height+1);
  kernel->x = (ssize_t) ( -y +(ssize_t) kernel->width-1)/2;
  kernel->y = (ssize_t) ( +x +(ssize_t) kernel->height-1)/2;
}

static inline double RotateSquareKernelBy90Degrees(KernelInfo *kernel,double angle)
{
  /* Rotate a square array of values by 90 degrees */
  RotateSquareKernelValues(kernel);
  /* rotate the origin - relative to center of array */
  RotateSquareKernelOrigin(kernel);
  angle = fmod(angle+270.0, 360.0);     /* angle reduced 90 degrees */
  kernel->angle = fmod(kernel->angle+90.0, 360.0);
  return(angle);
}

static inline void ReverseKernelValues(KernelInfo *kernel)
{
  MagickRealType
    t;

  MagickRealType
    *k;

  ssize_t
    i,
    j;

  k=kernel->values;
  j=(ssize_t) (kernel->width*kernel->height-1);
  for (i=0;  i < j;  i++, j--)
    t=k[i],  k[i]=k[j],  k[j]=t;
}

static inline double ReflectKernel(KernelInfo *kernel,double angle)
{
  /* For a 180 degree rotation - also know as a reflection
   * This is actually a very very common operation!
   * Basically all that is needed is a reversal of the kernel data!
   * And a reflection of the origin
   */
  ReverseKernelValues(kernel);

  kernel->x = (ssize_t) kernel->width  - kernel->x - 1;
  kernel->y = (ssize_t) kernel->height - kernel->y - 1;
  angle = fmod(angle-180.0, 360.0);   /* angle+180 degrees */
  kernel->angle = fmod(kernel->angle+180.0, 360.0);
  return(angle);
}

static inline double AttemptKernelRotationBy45Degrees(KernelInfo *kernel,
  double angle)
{
  if ( kernel->width == 3 && kernel->height == 3 )
    {
      const KernelRotation
        rotation = { kernel, angle };

      angle=RotateKernelBy45Degrees(&rotation);
    }
  else
    perror("Unable to rotate non-3x3 kernel by 45 degrees");
  return(angle);
}

static inline double AttemptKernelRotationBy90Degrees(KernelInfo *kernel,
  double angle)
{
  if ( kernel->width == 1 || kernel->height == 1 )
    angle=TransposeKernel(kernel,angle);
  else if ( kernel->width == kernel->height )
    angle=RotateSquareKernelBy90Degrees(kernel,angle);
  else
    perror("Unable to rotate a non-square, non-linear kernel 90 degrees");
  return(angle);
}

static inline void AttemptKernelRotations(KernelInfo *kernel,double angle)
{
  /* Attempt rotations by 45 degrees  -- 3x3 kernels only */
  if ( 22.5 < fmod(angle,90.0) && fmod(angle,90.0) <= 67.5 )
    angle=AttemptKernelRotationBy45Degrees(kernel,angle);
  if ( 45.0 < fmod(angle, 180.0)  && fmod(angle,180.0) <= 135.0 )
    angle=AttemptKernelRotationBy90Degrees(kernel,angle);
  if ( 135.0 < angle && angle <= 225.0 )
    angle=ReflectKernel(kernel,angle);
}

static const KernelInfoType
  RotationUselessKernels[] =
  {
    /* These built-in kernels are cylindrical kernels, rotating is useless */
    GaussianKernel,
    DoGKernel,
    LoGKernel,
    DiskKernel,
    PeaksKernel,
    LaplacianKernel,
    ChebyshevKernel,
    ManhattanKernel,
    EuclideanKernel,
    /* These may be rotatable at non-90 angles in the future */
    /* but simply rotating them in multiples of 90 degrees is useless */
    SquareKernel,
    DiamondKernel,
    PlusKernel,
    CrossKernel
  };

static inline MagickBooleanType IsRotationUselessKernel(const KernelInfoType type)
{
  size_t
    i;

  for (i=0; i < (sizeof(RotationUselessKernels)/
       sizeof(*RotationUselessKernels)); i++)
    if (RotationUselessKernels[i] == type)
      return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsNearZeroAngle(const double angle)
{
  return((337.5 < angle || angle <= 22.5) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNearHalfTurnAngle(const double angle)
{
  return((135.0 < angle && angle <= 225.0) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNearThreeQuarterTurnAngle(const double angle)
{
  return((225.0 < angle && angle <= 315.0) ? MagickTrue : MagickFalse);
}

static inline double NormalizeKernelAngle(double angle)
{
  angle = fmod(angle, 360.0);
  if ( angle < 0 )
    angle += 360.0;
  return(angle);
}

static inline double TurnBlurKernelAngle(double angle)
{
  if (IsNearThreeQuarterTurnAngle(angle) != MagickFalse)
    angle -= 180;
  return(angle);
}

static inline void RotateKernelInfo(KernelInfo *kernel, double angle)
{
  /* angle the lower kernels first */
  if ( kernel->next != (KernelInfo *) NULL)
    RotateKernelInfo(kernel->next, angle);

  /* WARNING: Currently assumes the kernel (rightly) is horizontally symmetrical
  **
  ** TODO: expand beyond simple 90 degree rotates, flips and flops
  */

  /* Modulus the angle */
  angle=NormalizeKernelAngle(angle);

  if (IsNearZeroAngle(angle) != MagickFalse)
    return;   /* Near zero angle - no change! - At least not at this time */

  /* Handle special cases */
  if (IsRotationUselessKernel(kernel->type) != MagickFalse)
    return;
  switch (kernel->type) {
    /* These only allows a +/-90 degree rotation (by transpose) */
    /* A 180 degree rotation is useless */
    case BlurKernel:
      if (IsNearHalfTurnAngle(angle) != MagickFalse)
        return;
      angle=TurnBlurKernelAngle(angle);
      break;

    default:
      break;
  }
  AttemptKernelRotations(kernel,angle);
  /* At this point angle should at least between -45 (315) and +45 degrees
   * In the future some form of non-orthogonal angled rotates could be
   * performed here, possibly with a linear kernel restriction.
   */

  return;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
