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

  MagickCore private methods shared by the image distortion methods: the
  polynomial, radial, cylinder, barrel and Shepards coefficient generators of
  DistortImage and SparseColorImage, GenerateCoefficients, and the row
  progress and sync helpers.
*/
#ifndef MAGICKCORE_DISTORT_PRIVATE_H
#define MAGICKCORE_DISTORT_PRIVATE_H

#include "MagickCore/artifact.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/matrix.h"
#include "MagickCore/matrix-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/statistic-private.h"
#include "MagickCore/string-private.h"
#include "MagickCore/distort-linear-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void SetPolynomialVectors(double **vectors,double *coeff,
  const size_t number_values,const size_t nterms)
{
  size_t
    i;

  /* fake a number_values x3 vectors matrix from coefficients array */
  for (i=0; i < number_values; i++)
    vectors[i] = &(coeff[2+i*nterms]);
}

static inline size_t SetPolynomialOrder(double *coeff,const double *arguments)
{
  size_t
    nterms;

  /* first two coefficients hold polynomial order information */
  coeff[0] = arguments[0];
  coeff[1] = (double) poly_number_terms(arguments[0]);
  nterms = CastDoubleToSizeT(coeff[1]);
  return(nterms);
}

static inline double *PolynomialCoefficients(double *coeff,
  const DistortMethod *method,const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    number_values = context->number_values,
    cp_size = context->cp_size,
    cp_x = context->cp_x,
    cp_y = context->cp_y,
    cp_values = context->cp_values;

  size_t
    i;

  /* Polynomial Distortion

     First two coefficients are used to hole global polynomial information
       c0 = Order of the polynomial being created
       c1 = number_of_terms in one polynomial equation

     Rest of the coefficients map to the equations....
        v = c0 + c1*x + c2*y + c3*x*y + c4*x^2 + c5*y^2 + c6*x^3 + ...
     for each 'value' (number_values of them) given.
     As such total coefficients =  2 + number_terms * number_values

     Input Arguments are sets of control points...
     For Distort Images    order  [u,v, x,y] ...
     For Sparse Gradients  order  [x,y, r,g,b] ...

     Polynomial Distortion Notes...
       + UNDER DEVELOPMENT -- Do not expect this to remain as is.
       + Currently polynomial is a reversed mapped distortion.
       + Order 1.5 is fudged to map into a bilinear distortion.
         though it is not the same order as that distortion.
  */
  double
    **matrix,
    **vectors,
    *terms;

  size_t
    nterms;   /* number of polynomial terms per number_values */

  ssize_t
    j;

  MagickBooleanType
    status;

  nterms=SetPolynomialOrder(coeff,arguments);

  /* create matrix, a fake vectors matrix, and least sqs terms */
  matrix=AcquireMagickMatrix(nterms,nterms);
  vectors=(double **) AcquireQuantumMemory(number_values,
    sizeof(*vectors));
  terms=(double *) AcquireQuantumMemory(nterms,sizeof(*terms));
  if (IsMatrixMemoryMissing(matrix,vectors,terms) != MagickFalse)
  {
    matrix  = RelinquishMagickMatrix(matrix, nterms);
    vectors = (double **) RelinquishMagickMemory(vectors);
    terms   = (double *) RelinquishMagickMemory(terms);
    coeff   = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed",
            "%s", "DistortCoefficients");
    return((double *) NULL);
  }
  SetPolynomialVectors(vectors,coeff,number_values,nterms);
  /* Add given control point pairs for least squares solving */
  for (i=1; i < number_arguments; i+=cp_size) { /* NB: start = 1 not 0 */
    for (j=0; j < (ssize_t) nterms; j++)
      terms[j] = poly_basis_fn(j,arguments[i+cp_x],arguments[i+cp_y]);
    LeastSquaresAddTerms(matrix,vectors,terms,
         &(arguments[i+cp_values]),nterms,number_values);
  }
  terms = (double *) RelinquishMagickMemory(terms);
  /* Solve for LeastSquares Coefficients */
  status=GaussJordanElimination(matrix,vectors,nterms,number_values);
  matrix  = RelinquishMagickMatrix(matrix, nterms);
  vectors = (double **) RelinquishMagickMemory(vectors);
  if ( status == MagickFalse ) {
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : 'Unsolvable Matrix'",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return((double *) NULL);
  }
  return(coeff);
}

static inline void NormalizeArcAngle(double *coeff)
{
  coeff[0] /= Magick2PI;  /* normalize radians */
  coeff[0] -= MagickRound(coeff[0]);
  coeff[0] *= Magick2PI;  /* de-normalize back to radians */
}

static inline void SetArcCoefficients(double *coeff,const Image *image,
  const double *arguments,const size_t number_arguments)
{
  coeff[0] = -MagickPI2;   /* -90, place at top! */
  if ( number_arguments >= 1 )
    coeff[1] = DegreesToRadians(arguments[0]);
  else
    coeff[1] = MagickPI2;   /* zero arguments - center is at top */
  if ( number_arguments >= 2 )
    coeff[0] += DegreesToRadians(arguments[1]);
  NormalizeArcAngle(coeff);
  coeff[3] = (double)image->rows-1;
  coeff[2] = (double)image->columns/coeff[1] + coeff[3]/2.0;
  if ( number_arguments >= 3 )
    SetArcOuterRadius(coeff,number_arguments,arguments);
  coeff[4] = ((double)image->columns-1.0)/2.0;
}

static inline double *ArcCoefficients(double *coeff,const DistortMethod *method,
  const CoefficientContext *context)
{
  const Image
    *const image = context->image;

  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments;

  /* Arc Distortion
     Args: arc_width  rotate  top_edge_radius  bottom_edge_radius
     All but first argument are optional
        arc_width      The angle over which to arc the image side-to-side
        rotate         Angle to rotate image from vertical center
        top_radius     Set top edge of source image at this radius
        bottom_radius  Set bottom edge to this radius (radial scaling)

     By default, if the radii arguments are nor provided the image radius
     is calculated so the horizontal center-line is fits the given arc
     without scaling.

     The output image size is ALWAYS adjusted to contain the whole image,
     and an offset is given to position image relative to the 0,0 point of
     the origin, allowing users to use relative positioning onto larger
     background (via -flatten).

     The arguments are converted to these coefficients
        c0: angle for center of source image
        c1: angle scale for mapping to source image
        c2: radius for top of source image
        c3: radius scale for mapping source image
        c4: centerline of arc within source image

     Note the coefficients use a center angle, so asymptotic join is
     furthest from both sides of the source image. This also means that
     for arc angles greater than 360 the sides of the image will be
     trimmed equally.

     Arc Distortion Notes...
       + Does not use a set of CPs
       + Will only work with Image Distortion
       + Can not be used for generating a sparse gradient (interpolation)
  */
  if ( number_arguments >= 1 && arguments[0] < MagickEpsilon ) {
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : 'Arc Angle Too Small'",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return((double *) NULL);
  }
  if ( number_arguments >= 3 && arguments[2] < MagickEpsilon ) {
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : 'Outer Radius Too Small'",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return((double *) NULL);
  }
  SetArcCoefficients(coeff,image,arguments,number_arguments);

  return(coeff);
}

static inline void SetPolarCenter(double *coeff,const Image *image,
  const double *arguments,const size_t number_arguments)
{
  /* Center X,Y */
  if ( number_arguments >= 4 ) {
    coeff[2] = arguments[2];
    coeff[3] = arguments[3];
  }
  else { /* center of actual image */
    coeff[2] = (double)(image->columns)/2.0+image->page.x;
    coeff[3] = (double)(image->rows)/2.0+image->page.y;
  }
}

static inline void SetPolarAngles(double *coeff,const double *arguments,
  const size_t number_arguments)
{
  /* Angle from,to - about polar center 0 is downward */
  coeff[4] = -MagickPI;
  if ( number_arguments >= 5 )
    coeff[4] = DegreesToRadians(arguments[4]);
  coeff[5] = coeff[4];
  if ( number_arguments >= 6 )
    coeff[5] = DegreesToRadians(arguments[5]);
  if ( fabs(coeff[4]-coeff[5]) < MagickEpsilon )
    coeff[5] += Magick2PI; /* same angle is a full circle */
}

static inline void SetPolarRadii(double *coeff,const double *arguments,
  const size_t number_arguments)
{
  /* Rmax -  if 0 calculate appropriate value */
  if ( number_arguments >= 1 )
    coeff[0] = arguments[0];
  else
    coeff[0] = 0.0;
  /* Rmin  - usually 0 */
}

static inline void SetPolarCoefficients(double *coeff,const Image *image,
  const double *arguments,const size_t number_arguments)
{
  SetPolarRadii(coeff,arguments,number_arguments);
  coeff[1] = number_arguments >= 2 ? arguments[1] : 0.0;
  SetPolarCenter(coeff,image,arguments,number_arguments);
  SetPolarAngles(coeff,arguments,number_arguments);
}

static inline void SetPolarRatios(double *coeff,const Image *image,
  const DistortMethod *method)
{
  /* conversion ratios */
  if ( *method == PolarDistortion ) {
    coeff[6]=(double) image->columns/(coeff[5]-coeff[4]);
    coeff[7]=(double) image->rows/(coeff[0]-coeff[1]);
  }
  else { /* *method == DePolarDistortion */
    coeff[6]=(coeff[5]-coeff[4])/image->columns;
    coeff[7]=(coeff[0]-coeff[1])/image->rows;
  }
}

static inline double *PolarCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  const DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

  const Image
    *const image = context->image;

  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments;

  /* (De)Polar Distortion   (same set of arguments)
     Args:  Rmax, Rmin,  Xcenter,Ycenter,  Afrom,Ato
     DePolar can also have the extra arguments of Width, Height

     Coefficients 0 to 5 is the sanitized version first 6 input args
     Coefficient 6  is the angle to coord ratio  and visa-versa
     Coefficient 7  is the radius to coord ratio and visa-versa

     WARNING: It is possible for  Radius max<min  and/or  Angle from>to
  */
  if (IsInvalidPolarArgumentCount(number_arguments,method) != MagickFalse) {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionError,"InvalidArgument", "%s : number of arguments",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
    coeff=(double *) RelinquishMagickMemory(coeff);
    return((double *) NULL);
  }
  SetPolarCoefficients(coeff,image,arguments,number_arguments);
  /* if radius 0 or negative,  its a special value... */
  if ( coeff[0] < MagickEpsilon )
    SetPolarSpecialRadius(image,coeff);
  /* IF Rmax <= 0 or Rmin < 0 OR Rmax < Rmin, THEN error */
  if (IsInvalidPolarRadius(coeff) != MagickFalse) {
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument", "%s : Invalid Radius",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
    coeff=(double *) RelinquishMagickMemory(coeff);
    return((double *) NULL);
  }
  SetPolarRatios(coeff,image,method);
  return(coeff);
}

static inline void SetCylinderCoefficients(double *coeff,const Image *image,
  const double *arguments,const DistortMethod *method)
{
  coeff[0] = DegreesToRadians(arguments[0]);
  if ( *method == Cylinder2PlaneDistortion )
    /* image is curved around cylinder, so FOV angle (in radians)
     * scales directly to image X coordinate, according to its radius.
     */
    coeff[1] = (double) image->columns/coeff[0];
  else
    /* radius is distance away from an image with this angular FOV */
    coeff[1] = (double) image->columns / ( 2 * tan(coeff[0]/2) );

  coeff[2] = (double)(image->columns)/2.0+image->page.x;
  coeff[3] = (double)(image->rows)/2.0+image->page.y;
  coeff[4] = coeff[2];
  coeff[5] = coeff[3]; /* assuming image size is the same */
}

static inline double *CylinderCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  const DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

  const Image
    *const image = context->image;

  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments;

  /* 3D Cylinder to/from a Tangential Plane

     Projection between a cylinder and flat plain from a point on the
     center line of the cylinder.

     The two surfaces coincide in 3D space at the given centers of
     distortion (perpendicular to projection point) on both images.

     Args:  FOV_arc_width
     Coefficients: FOV(radians), Radius, center_x,y, dest_center_x,y

     FOV (Field Of View) the angular field of view of the distortion,
     across the width of the image, in degrees.  The centers are the
     points of least distortion in the input and resulting images.

     These centers are however determined later.

     Coeff 0 is the FOV angle of view of image width in radians
     Coeff 1 is calculated radius of cylinder.
     Coeff 2,3  center of distortion of input image
     Coefficients 4,5 Center of Distortion of dest (determined later)
  */
  if (number_arguments < 1) {
    coeff=(double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument", "%s : 'Needs at least 1 argument'",
        CommandOptionToMnemonic(MagickDistortOptions,*method));
    return((double *) NULL);
  }
  if ( arguments[0] < MagickEpsilon || arguments[0] > 160.0 ) {
    coeff=(double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument", "%s : Invalid FOV Angle",
        CommandOptionToMnemonic(MagickDistortOptions,*method));
    return((double *) NULL);
  }
  SetCylinderCoefficients(coeff,image,arguments,method);
  return(coeff);
}

static inline void SetBarrelCenter(double *coeff,const Image *image,
  const double *arguments,const size_t number_arguments)
{
  /* X,Y Center of Distortion (image coordinates) */
  if ( number_arguments == 5 )  {
    coeff[8] = arguments[3];
    coeff[9] = arguments[4];
  }
  else if ( number_arguments == 6 ) {
    coeff[8] = arguments[4];
    coeff[9] = arguments[5];
  }
  else if ( number_arguments == 10 ) {
    coeff[8] = arguments[8];
    coeff[9] = arguments[9];
  }
  else {
    /* center of the image provided (image coordinates) */
    coeff[8] = (double)image->columns/2.0 + image->page.x;
    coeff[9] = (double)image->rows/2.0    + image->page.y;
  }
}

static inline void SetBarrelYCoefficients(double *coeff,const double *arguments,
  const size_t number_arguments,const double rscale)
{
  /* Y coefficients: as given OR same as X coefficients */
  if ( number_arguments >= 8 ) {
    coeff[4] = arguments[4] * pow(rscale,3.0);
    coeff[5] = arguments[5] * rscale*rscale;
    coeff[6] = arguments[6] * rscale;
    coeff[7] = arguments[7];
  }
  else {
    coeff[4] = coeff[0];
    coeff[5] = coeff[1];
    coeff[6] = coeff[2];
    coeff[7] = coeff[3];
  }
}

static inline void SetBarrelXCoefficients(double *coeff,const double *arguments,
  const size_t number_arguments,const double rscale)
{
  /* A,B,C,D coefficients */
  coeff[0] = arguments[0];
  coeff[1] = arguments[1];
  coeff[2] = arguments[2];
  if ((number_arguments == 3) || (number_arguments == 5) )
    coeff[3] = 1.0 - coeff[0] - coeff[1] - coeff[2];
  else
    coeff[3] = arguments[3];
  /* de-normalize the coefficients */
  coeff[0] *= pow(rscale,3.0);
  coeff[1] *= rscale*rscale;
  coeff[2] *= rscale;
}

static inline double *BarrelCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  const DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

  const Image
    *const image = context->image;

  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments;

  /* Barrel Distortion
       Rs=(A*Rd^3 + B*Rd^2 + C*Rd + D)*Rd
     BarrelInv Distortion
       Rs=Rd/(A*Rd^3 + B*Rd^2 + C*Rd + D)

    Where Rd is the normalized radius from corner to middle of image
    Input Arguments are one of the following forms (number of arguments)...
        3:  A,B,C
        4:  A,B,C,D
        5:  A,B,C    X,Y
        6:  A,B,C,D  X,Y
        8:  Ax,Bx,Cx,Dx  Ay,By,Cy,Dy
       10:  Ax,Bx,Cx,Dx  Ay,By,Cy,Dy   X,Y

    Returns 10 coefficient values, which are de-normalized (pixel scale)
      Ax, Bx, Cx, Dx,   Ay, By, Cy, Dy,    Xc, Yc
  */
  /* Radius de-normalization scaling factor */
  double
    rscale = 2.0/MagickMin((double) image->columns,(double) image->rows);

  /* sanity check  number of args must = 3,4,5,6,8,10 or error */
  if (IsInvalidBarrelArgumentCount(number_arguments) != MagickFalse)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionError,"InvalidArgument", "%s : number of arguments",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
      return((double *) NULL);
    }
  SetBarrelXCoefficients(coeff,arguments,number_arguments,rscale);
  SetBarrelYCoefficients(coeff,arguments,number_arguments,rscale);
  SetBarrelCenter(coeff,image,arguments,number_arguments);
  return(coeff);
}

static inline double *ShepardsCoefficients(double *coeff,const DistortMethod *method,
  const CoefficientContext *context)
{
  const Image
    *const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    cp_size = context->cp_size;

  /* Shepards Distortion  input arguments are the coefficients!
     Just check the number of arguments is valid!
     Args:  u1,v1, x1,y1, ...
      OR :  u1,v1, r1,g1,c1, ...
  */
  if ( number_arguments%cp_size != 0 ||
       number_arguments < cp_size ) {
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidArgument", "%s : 'requires CP's (4 numbers each)'",
          CommandOptionToMnemonic(MagickDistortOptions, *method));
    coeff=(double *) RelinquishMagickMemory(coeff);
    return((double *) NULL);
  }
  /* User defined weighting power for Shepard's Method */
  return(SetShepardsPower(image,coeff,exception));
}

static inline MagickBooleanType GetPolynomialCoefficientCount(const CoefficientContext *context,
  size_t *number_coefficients)
{
  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    number_values = context->number_values,
    cp_size = context->cp_size;

  size_t
    i;

  /* number of coefficients depend on the given polynomial 'order' */
  if (number_arguments < 1)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
             "InvalidArgument","%s : '%s'","Polynomial",
             "Needs at least 1 argument");
      return(MagickFalse);
    }
  i = poly_number_terms(arguments[0]);
  *number_coefficients = 2 + i*number_values;
  if (i == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                 "InvalidArgument","%s : '%s'","Polynomial",
                 "Invalid order, should be integer 1 to 5, or 1.5");
      return(MagickFalse);
    }
  if ((number_arguments < (1+i*cp_size)) ||
      (((number_arguments-1) % cp_size) != 0)) 
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
             "InvalidArgument", "%s : 'require at least %.17g CPs'",
             "Polynomial", (double) i);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static inline void CountProjectionCoefficients(const DistortMethod *method,
  size_t *number_coefficients)
{
  switch (*method) {
    case PolarDistortion:
    case DePolarDistortion:
      *number_coefficients=8;
      break;
    case PerspectiveDistortion:
    case PerspectiveProjectionDistortion:
      *number_coefficients=9;
      break;
    case BarrelDistortion:
    case BarrelInverseDistortion:
      *number_coefficients=10;
      break;
    default:
      perror("unknown method given"); /* just fail assertion */
  }
}

static inline void CountFixedCoefficients(const DistortMethod *method,
  size_t *number_coefficients)
{
  switch (*method) {
    case ShepardsDistortion:
      *number_coefficients=1;  /* The power factor to use */
      break;
    case ArcDistortion:
      *number_coefficients=5;
      break;
    case ScaleRotateTranslateDistortion:
    case AffineProjectionDistortion:
    case Plane2CylinderDistortion:
    case Cylinder2PlaneDistortion:
      *number_coefficients=6;
      break;
    default:
    {
      CountProjectionCoefficients(method,&(*number_coefficients));
      break;
    }
  }
}

typedef struct _CoefficientCountTargets
{
  size_t
    *number_coefficients,
    *cp_x,
    *cp_y,
    *cp_values;
} CoefficientCountTargets;

static inline void CountBilinearCoefficients(const DistortMethod *method,
  const CoefficientContext *context,const CoefficientCountTargets *targets)
{
  const size_t
    number_values = context->number_values;

  size_t
    *const number_coefficients = targets->number_coefficients,
    *const cp_x = targets->cp_x,
    *const cp_y = targets->cp_y,
    *const cp_values = targets->cp_values;

  switch (*method) {
    case BilinearReverseDistortion:
      *number_coefficients=4*number_values;
      break;
    /*
      The rest are constants as they are only used for image distorts
    */
    case BilinearForwardDistortion:
      *number_coefficients=10; /* 2*4 coeff plus 2 constants */
      *cp_x = 0;        /* Reverse src/dest coords for forward mapping */
      *cp_y = 1;
      *cp_values = 2;
      break;
#if 0
    case QuadrilateralDistortion:
      number_coefficients=19; /* BilinearForward + BilinearReverse */
#endif
      break;
    default:
    {
      CountFixedCoefficients(method,&(*number_coefficients));
      break;
    }
  }
}

static inline MagickBooleanType CountCoefficients(const DistortMethod *method,
  const CoefficientContext *context,const CoefficientCountTargets *targets)
{
  const size_t
    number_values = context->number_values;

  size_t
    *const number_coefficients = targets->number_coefficients;

  switch (*method) {
    case AffineDistortion:
    case RigidAffineDistortion:
    /* also BarycentricColorInterpolate: */
      *number_coefficients=3*number_values;
      break;
    case PolynomialDistortion:
      if (GetPolynomialCoefficientCount(context,&(*number_coefficients)) ==
          MagickFalse)
        return(MagickFalse);
      break;
    default:
    {
      CountBilinearCoefficients(method,context,targets);
      break;
    }
  }
  return(MagickTrue);
}

static inline double *GenerateRadialCoefficients(const CoefficientCall *call)
{
  DistortMethod
    *const method = call->method;

  switch (*method)
  {
    case PolarDistortion:
    case DePolarDistortion:
      return(PolarCoefficients(call));
    case Cylinder2PlaneDistortion:
    case Plane2CylinderDistortion:
      return(CylinderCoefficients(call));
    case BarrelDistortion:
    case BarrelInverseDistortion:
      return(BarrelCoefficients(call));
    default:
      break;
  }
  /* you should never reach this point */
  perror("no method handler"); /* just fail assertion */
  return((double *) NULL);
}

static inline double *GenerateMappedCoefficients(const CoefficientCall *call)
{
  double
    *const coeff = call->coeff;

  DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

  switch (*method)
  {
    case BilinearForwardDistortion:
    case BilinearReverseDistortion:
      return(BilinearCoefficients(coeff,method,context));
#if 0
    case QuadrilateralDistortion:
    {
      /* Map a Quadrilateral to a unit square using BilinearReverse
         Then map that unit square back to the final Quadrilateral
         using BilinearForward.

         Input Arguments are sets of control points...
         For Distort Images    u,v, x,y  ...
         For Sparse Gradients  x,y, r,g,b  ...

      */
      /* UNDER CONSTRUCTION */
      return(coeff);
    }
#endif

    case PolynomialDistortion:
      return(PolynomialCoefficients(coeff,method,context));
    case ArcDistortion:
      return(ArcCoefficients(coeff,method,context));
    case ShepardsDistortion:
      return(ShepardsCoefficients(coeff,method,context));
    default:
      break;
  }
  return(GenerateRadialCoefficients(call));
}

static inline double *GenerateAffineFamilyCoefficients(
  const CoefficientCall *call)
{
  double
    *const coeff = call->coeff;

  DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

  switch (*method)
  {
    case AffineDistortion:
      return(AffineCoefficients(coeff,method,context));
    case RigidAffineDistortion:
      return(RigidAffineCoefficients(coeff,method,context));
    case AffineProjectionDistortion:
      return(AffineProjectionCoefficients(coeff,method,context));
    case ScaleRotateTranslateDistortion:
      return(ScaleRotateTranslateCoefficients(coeff,method,context));
    case PerspectiveDistortion:
      return(PerspectiveCoefficients(coeff,method,context));
    case PerspectiveProjectionDistortion:
      return(PerspectiveProjectionCoefficients(coeff,method,context));
    default:
      break;
  }
  return(GenerateMappedCoefficients(call));
}

static inline void ZeroCoefficients(double *coeff,
  const size_t number_coefficients)
{
  size_t
    i;

  /* zero out coefficients array */
  for (i=0; i < number_coefficients; i++)
    coeff[i] = 0.0;
}

/*
  Typedef declarations.
*/
typedef struct _CoefficientInput
{
  size_t
    number_arguments;

  const double
    *arguments;

  size_t
    number_values;
} CoefficientInput;

static inline double *GenerateCoefficients(const Image *image,
  DistortMethod *method,const CoefficientInput *input,ExceptionInfo *exception)
{
  const size_t
    number_arguments = input->number_arguments;

  const double
    *arguments = input->arguments;

  size_t
    number_values = input->number_values;

  double
    *coeff;

  size_t
    number_coefficients, /* number of coefficients to return (array size) */
    cp_size,      /* number floating point numbers per control point */
    cp_x,cp_y,    /* the x,y indexes for control point */
    cp_values;    /* index of values for this control point */
    /* number_values   Number of values given per control point */

  CoefficientContext
    context;

  const CoefficientCountTargets
    targets = { &number_coefficients,&cp_x,&cp_y,&cp_values };

  if ( number_values == 0 ) {
    /* Image distortion using control points (or other distortion)
       That is generate a mapping so that   x,y->u,v   given  u,v,x,y
    */
    number_values = 2;   /* special case: two values of u,v */
    cp_values = 0;       /* the values i,j are BEFORE the destination CP x,y */
    cp_x = 2;            /* location of x,y in input control values */
    cp_y = 3;
    /* NOTE: cp_values, also used for later 'reverse map distort' tests */
  }
  else {
    cp_x = 0;            /* location of x,y in input control values */
    cp_y = 1;
    cp_values = 2;       /* and the other values are after x,y */
    /* Typically in this case the values are R,G,B color values */
  }
  cp_size = number_values+2; /* each CP definition involves this many numbers */

  /* If not enough control point pairs are found for specific distortions
     fall back to Affine distortion (allowing 0 to 3 point pairs)
  */
  if (IsAffineFallback(number_arguments,cp_size,method) != MagickFalse)
    *method = AffineDistortion;
  context.image=image;
  context.arguments=arguments;
  context.exception=exception;
  context.number_arguments=number_arguments;
  context.number_values=number_values;
  context.cp_size=cp_size;

  number_coefficients=0;
  if (CountCoefficients(method,&context,&targets) == MagickFalse)
    return((double *) NULL);

  context.cp_x=cp_x;
  context.cp_y=cp_y;
  context.cp_values=cp_values;

  /* allocate the array of coefficients needed */
  coeff=(double *) AcquireQuantumMemory(number_coefficients,sizeof(*coeff));
  if (coeff == (double *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","%s",
        "GenerateCoefficients");
      return((double *) NULL);
    }

  ZeroCoefficients(coeff,number_coefficients);

  {
    const CoefficientCall
      call = { coeff, method, &context };

    return(GenerateAffineFamilyCoefficients(&call));
  }
}

static inline void SetDistortProgress(MagickBooleanType *status,const Image *image,
  const char *tag,const MagickOffsetType progress)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,tag,progress,image->rows);
  if (proceed == MagickFalse)
    (*status)=MagickFalse;
}

static inline void SyncDistortRow(MagickBooleanType *status,CacheView *view,
  ExceptionInfo *exception)
{
  MagickBooleanType
    sync;

  sync=SyncCacheViewAuthenticPixels(view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
