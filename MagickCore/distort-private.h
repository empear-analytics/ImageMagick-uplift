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
  coefficient generators of DistortImage and SparseColorImage, their checks
  and polynomial terms, and the row progress and sync helpers.
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

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Numerous internal routines for image distortions.
*/
static inline void AffineArgsToCoefficients(double *affine)
{
  /* map  external sx,ry,rx,sy,tx,ty  to  internal c0,c2,c4,c1,c3,c5 */
  double tmp[4];  /* note indexes  0 and 5 remain unchanged */
  tmp[0]=affine[1]; tmp[1]=affine[2]; tmp[2]=affine[3]; tmp[3]=affine[4];
  affine[3]=tmp[0]; affine[1]=tmp[1]; affine[4]=tmp[2]; affine[2]=tmp[3];
}

static inline void InvertAffineCoefficients(const double *coeff,double *inverse)
{
  /* From "Digital Image Warping" by George Wolberg, page 50 */
  double determinant;

  determinant=MagickSafeReciprocal(coeff[0]*coeff[4]-coeff[1]*coeff[3]);
  inverse[0]=determinant*coeff[4];
  inverse[1]=determinant*(-coeff[1]);
  inverse[2]=determinant*(coeff[1]*coeff[5]-coeff[2]*coeff[4]);
  inverse[3]=determinant*(-coeff[3]);
  inverse[4]=determinant*coeff[0];
  inverse[5]=determinant*(coeff[2]*coeff[3]-coeff[0]*coeff[5]);
}

static inline void InvertPerspectiveCoefficients(const double *coeff,
  double *inverse)
{
  /* From "Digital Image Warping" by George Wolberg, page 53 */
  double determinant;

  determinant=MagickSafeReciprocal(coeff[0]*coeff[4]-coeff[3]*coeff[1]);
  inverse[0]=determinant*(coeff[4]-coeff[7]*coeff[5]);
  inverse[1]=determinant*(coeff[7]*coeff[2]-coeff[1]);
  inverse[2]=determinant*(coeff[1]*coeff[5]-coeff[4]*coeff[2]);
  inverse[3]=determinant*(coeff[6]*coeff[5]-coeff[3]);
  inverse[4]=determinant*(coeff[0]-coeff[6]*coeff[2]);
  inverse[5]=determinant*(coeff[3]*coeff[2]-coeff[0]*coeff[5]);
  inverse[6]=determinant*(coeff[3]*coeff[7]-coeff[6]*coeff[4]);
  inverse[7]=determinant*(coeff[6]*coeff[1]-coeff[0]*coeff[7]);
}

/*
 * Polynomial Term Defining Functions
 *
 * Order must either be an integer, or 1.5 to produce
 * the 2 number_valuesal polynomial function...
 *    affine     1   (3)      u = c0 + c1*x + c2*y
 *    bilinear   1.5 (4)      u = '' + c3*x*y
 *    quadratic  2   (6)      u = '' + c4*x*x + c5*y*y
 *    cubic      3   (10)     u = '' + c6*x^3 + c7*x*x*y + c8*x*y*y + c9*y^3
 *    quartic    4   (15)     u = '' + c10*x^4 + ... + c14*y^4
 *    quintic    5   (21)     u = '' + c15*x^5 + ... + c20*y^5
 * number in parenthesis minimum number of points needed.
 * Anything beyond quintic, has not been implemented until
 * a more automated way of determining terms is found.

 * Note the slight re-ordering of the terms for a quadratic polynomial
 * which is to allow the use of a bi-linear (order=1.5) polynomial.
 * All the later polynomials are ordered simply from x^N to y^N
 */
static inline MagickBooleanType IsInvalidPolyOrder(const double order)
{
  return(( order < 1 || order > 5 ||
       ( order != floor(order) && (order-1.5) > MagickEpsilon) ) ?
    MagickTrue : MagickFalse);
}

static inline size_t poly_number_terms(double order)
{
 /* Return the number of terms for a 2d polynomial */
  if (IsInvalidPolyOrder(order) != MagickFalse)
    return 0; /* invalid polynomial order */
  return(CastDoubleToSizeT(floor((order+1.0)*(order+2.0)/2.0)));
}

static inline double PolyBasisFnQuintic(ssize_t n, double x, double y)
{
  switch(n) {
    case 15:  return( x*x*x*x*x );
    case 16:  return( x*x*x*x*y );
    case 17:  return( x*x*x*y*y );
    case 18:  return( x*x*y*y*y );
    case 19:  return( x*y*y*y*y );
    case 20:  return( y*y*y*y*y ); /* quintic   order = 5   terms = 21 */
    default:  break;
  }
  return( 0 ); /* should never happen */
}

static inline double PolyBasisFnQuartic(ssize_t n, double x, double y)
{
  switch(n) {
    case 10:  return( x*x*x*x );
    case 11:  return( x*x*x*y );
    case 12:  return( x*x*y*y );
    case 13:  return( x*y*y*y );
    case 14:  return( y*y*y*y ); /* quartic     order = 4   terms = 15 */
    default:  break;
  }
  return(PolyBasisFnQuintic(n,x,y));
}

static inline double PolyBasisFnCubic(ssize_t n, double x, double y)
{
  switch(n) {
    case  6:  return( x*x*x );
    case  7:  return( x*x*y );
    case  8:  return( x*y*y );
    case  9:  return( y*y*y ); /* cubic         order = 3   terms = 10 */
    default:  break;
  }
  return(PolyBasisFnQuartic(n,x,y));
}

static inline double poly_basis_fn(ssize_t n, double x, double y)
{
  /* Return the result for this polynomial term */
  switch(n) {
    case  0:  return( 1.0 ); /* constant */
    case  1:  return(  x  );
    case  2:  return(  y  ); /* affine          order = 1   terms = 3 */
    case  3:  return( x*y ); /* bilinear        order = 1.5 terms = 4 */
    case  4:  return( x*x );
    case  5:  return( y*y ); /* quadratic       order = 2   terms = 6 */
    default:  break;
  }
  return(PolyBasisFnCubic(n,x,y));
}

static inline double MagickRound(double x)
{
  /*
    Round the fraction to nearest integer.
  */
  if ((x-floor(x)) < (ceil(x)-x))
    return(floor(x));
  return(ceil(x));
}

static inline MagickBooleanType IsAffineFallback(const size_t number_arguments,
  const size_t cp_size,const DistortMethod *method)
{
  return(( number_arguments < 4*cp_size &&
       (  *method == BilinearForwardDistortion
       || *method == BilinearReverseDistortion
       || *method == PerspectiveDistortion
       ) ) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsMatrixMemoryMissing(double **matrix,
  double **vectors,const double *terms)
{
  return(((matrix  == (double **) NULL) || (vectors == (double **) NULL) ||
    (terms   == (double *) NULL)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsInvalidPolarArgumentCount(
  const size_t number_arguments,const DistortMethod *method)
{
  return(( number_arguments == 3
          || ( number_arguments > 6 && *method == PolarDistortion )
          || number_arguments > 8 ) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsInvalidPolarRadius(const double *coeff)
{
  return(( coeff[0] < MagickEpsilon || coeff[1] < -MagickEpsilon
           || (coeff[0]-coeff[1]) < MagickEpsilon ) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsInvalidBarrelArgumentCount(
  const size_t number_arguments)
{
  return(( (number_arguments  < 3) || (number_arguments == 7) ||
           (number_arguments == 9) || (number_arguments > 10) ) ?
    MagickTrue : MagickFalse);
}

static inline void SetAffineSingleCoefficients(double *coeff,const double *arguments,
  const size_t cp_values,const size_t number_values)
{
  size_t
    i;

  if ( cp_values == 0 ) {
    /* image distortion - translate the image */
    coeff[0] = 1.0;
    coeff[2] = arguments[0] - arguments[2];
    coeff[4] = 1.0;
    coeff[5] = arguments[1] - arguments[3];
  }
  else {
    /* sparse gradient - use the values directly */
    for (i=0; i<number_values; i++)
      coeff[i*3+2] = arguments[cp_values+i];
  }
}

static inline void SetPolarFurthestRadius(const Image *image,double *coeff)
{
  /* furthest diagonal if radius == -1 */
  if ( fabs(-1.0-coeff[0]) < MagickEpsilon ) {
    double rx,ry;
    rx = coeff[2]-image->page.x;
    ry = coeff[3]-image->page.y;
    coeff[0] = rx*rx+ry*ry;
    ry = coeff[3]-image->page.y-image->rows;
    coeff[0] = MagickMax(coeff[0],rx*rx+ry*ry);
    rx = coeff[2]-image->page.x-image->columns;
    coeff[0] = MagickMax(coeff[0],rx*rx+ry*ry);
    ry = coeff[3]-image->page.y;
    coeff[0] = MagickMax(coeff[0],rx*rx+ry*ry);
    coeff[0] = sqrt(coeff[0]);
  }
}

static inline void SetPolarSpecialRadius(const Image *image,double *coeff)
{
  /* Use closest edge  if radius == 0 */
  if ( fabs(coeff[0]) < MagickEpsilon ) {
    coeff[0]=MagickMin(fabs(coeff[2]-image->page.x),
                       fabs(coeff[3]-image->page.y));
    coeff[0]=MagickMin(coeff[0],
                 fabs(coeff[2]-image->page.x-image->columns));
    coeff[0]=MagickMin(coeff[0],
                 fabs(coeff[3]-image->page.y-image->rows));
  }
  SetPolarFurthestRadius(image,coeff);
}

static inline void SetArcOuterRadius(double *coeff,const size_t number_arguments,
  const double *arguments)
{
  if ( number_arguments >= 4 )
    coeff[3] = arguments[2] - arguments[3];
  else
    coeff[3] *= arguments[2]/coeff[2];
  coeff[2] = arguments[2];
}

static inline double *SetShepardsPower(const Image *image,double *coeff,
  ExceptionInfo *exception)
{
  { const char *artifact=GetImageArtifact(image,"shepards:power");
    if ( artifact != (const char *) NULL ) {
      coeff[0]=StringToDouble(artifact,(char **) NULL) / 2.0;
      if ( coeff[0] < MagickEpsilon ) {
        (void) ThrowMagickException(exception,GetMagickModule(),
            OptionError,"InvalidArgument","%s", "-define shepards:power" );
        coeff=(double *) RelinquishMagickMemory(coeff);
        return((double *) NULL);
      }
    }
    else
      coeff[0]=1.0;  /* Default power of 2 (Inverse Squared) */
  }
  return(coeff);
}

typedef struct _AffineThirdPoint
{
  double
    **matrix,
    **vectors;

  const double
    *arguments;

  size_t
    number_values,
    cp_size,
    cp_x,
    cp_y,
    cp_values;
} AffineThirdPoint;

static inline void AddAffineThirdPoint(const AffineThirdPoint *point,double *terms)
{
  double
    **const matrix = point->matrix,
    **const vectors = point->vectors;

  const double
    *const arguments = point->arguments;

  const size_t
    number_values = point->number_values,
    cp_size = point->cp_size,
    cp_x = point->cp_x,
    cp_y = point->cp_y,
    cp_values = point->cp_values;

  /* Only two pairs were given, but we need 3 to solve the affine.
     Fake extra coordinates by rotating p1 around p0 by 90 degrees.
       x2 = x0 - (y1-y0)   y2 = y0 + (x1-x0)
   */
  terms[0] = arguments[cp_x]
           - ( arguments[cp_size+cp_y] - arguments[cp_y] ); /* x2 */
  terms[1] = arguments[cp_y] +
           + ( arguments[cp_size+cp_x] - arguments[cp_x] ); /* y2 */
  terms[2] = 1;                                             /* 1 */
  if ( cp_values == 0 ) {
    /* Image Distortion - rotate the u,v coordinates too */
    double
      uv2[2];
    uv2[0] = arguments[0] - arguments[5] + arguments[1];   /* u2 */
    uv2[1] = arguments[1] + arguments[4] - arguments[0];   /* v2 */
    LeastSquaresAddTerms(matrix,vectors,terms,uv2,3UL,2UL);
  }
  else {
    /* Sparse Gradient - use values of p0 for linear gradient */
    LeastSquaresAddTerms(matrix,vectors,terms,
          &(arguments[cp_values]),3UL,number_values);
  }
}

static inline void SetAffineVectors(double **vectors,double *coeff,
  const size_t number_values)
{
  size_t
    i;

  /* fake a number_values x3 vectors matrix from coefficients array */
  for (i=0; i < number_values; i++)
    vectors[i] = &(coeff[i*3]);
}

typedef struct _CoefficientContext
{
  const Image
    *image;

  const double
    *arguments;

  ExceptionInfo
    *exception;

  size_t
    number_arguments,
    number_values,
    cp_size,
    cp_x,
    cp_y,
    cp_values;
} CoefficientContext;

static inline void AddAffineTerms(double *coeff,double **matrix,double **vectors,
  const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  const size_t
    number_arguments = context->number_arguments,
    number_values = context->number_values,
    cp_size = context->cp_size,
    cp_x = context->cp_x,
    cp_y = context->cp_y,
    cp_values = context->cp_values;

  double
    terms[3];

  size_t
    i;

  SetAffineVectors(vectors,coeff,number_values);
  /* Add given control point pairs for least squares solving */
  for (i=0; i < number_arguments; i+=cp_size) {
    terms[0] = arguments[i+cp_x];  /* x */
    terms[1] = arguments[i+cp_y];  /* y */
    terms[2] = 1;                  /* 1 */
    LeastSquaresAddTerms(matrix,vectors,terms,
             &(arguments[i+cp_values]),3UL,number_values);
  }
  if ( number_arguments == 2*cp_size ) {
    AffineThirdPoint
      point;

    point.matrix=matrix;
    point.vectors=vectors;
    point.arguments=arguments;
    point.number_values=number_values;
    point.cp_size=cp_size;
    point.cp_x=cp_x;
    point.cp_y=cp_y;
    point.cp_values=cp_values;
    AddAffineThirdPoint(&point,terms);
  }
}

static inline double *AffineCoefficients(double *coeff,const DistortMethod *method,
  const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    number_values = context->number_values,
    cp_size = context->cp_size,
    cp_values = context->cp_values;

  /* Affine Distortion
       v =  c0*x + c1*y + c2
     for each 'value' given

     Input Arguments are sets of control points...
     For Distort Images    u,v, x,y  ...
     For Sparse Gradients  x,y, r,g,b  ...
  */
  double
    **matrix,
    **vectors;

  MagickBooleanType
    status;

  if ( number_arguments%cp_size != 0 ||
       number_arguments < cp_size ) {
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
           "InvalidArgument", "%s : 'require at least %.17g CPs'",
           "Affine", 1.0);
    coeff=(double *) RelinquishMagickMemory(coeff);
    return((double *) NULL);
  }
  /* handle special cases of not enough arguments */
  if ( number_arguments == cp_size ) {
    /* Only 1 CP Set Given */
    SetAffineSingleCoefficients(coeff,arguments,cp_values,number_values);
    return(coeff);
  }
  /* 2 or more points (usually 3) given.
     Solve a least squares simultaneous equation for coefficients.
  */
  /* create matrix, and a fake vectors matrix */
  matrix=AcquireMagickMatrix(3UL,3UL);
  vectors=(double **) AcquireQuantumMemory(number_values,
    sizeof(*vectors));
  if (matrix == (double **) NULL || vectors == (double **) NULL)
  {
    matrix  = RelinquishMagickMatrix(matrix, 3UL);
    vectors = (double **) RelinquishMagickMemory(vectors);
    coeff   = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed",
            "%s", "DistortCoefficients");
    return((double *) NULL);
  }
  AddAffineTerms(coeff,matrix,vectors,context);
  /* Solve for LeastSquares Coefficients */
  status=GaussJordanElimination(matrix,vectors,3UL,number_values);
  matrix = RelinquishMagickMatrix(matrix, 3UL);
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

static inline void AddRigidAffineTerms(double **matrix,double *vectors[1],
  const double *arguments,const size_t number_arguments)
{
  double
    terms[5];

  size_t
    i;

  for (i=0; i < number_arguments; i+=4)
  {
    terms[0]=arguments[i+0];
    terms[1]=(-arguments[i+1]);
    terms[2]=1.0;
    terms[3]=0.0;
    LeastSquaresAddTerms(matrix,vectors,terms,&(arguments[i+2]),4UL,1UL);
    terms[0]=arguments[i+1];
    terms[1]=arguments[i+0];
    terms[2]=0.0;
    terms[3]=1.0;
    LeastSquaresAddTerms(matrix,vectors,terms,&(arguments[i+3]),4UL,1UL);
  }
}

static inline void ConvertRigidAffineCoefficients(double *coeff)
{
  double
    inverse[6];

  /*
    Convert (S, R, Tx, Ty) to an affine projection.
  */
  inverse[0]=coeff[0];
  inverse[1]=coeff[1];
  inverse[2]=(-coeff[1]);
  inverse[3]=coeff[0];
  inverse[4]=coeff[2];
  inverse[5]=coeff[3];
  AffineArgsToCoefficients(inverse);
  InvertAffineCoefficients(inverse,coeff);
}

static inline double *RigidAffineCoefficients(double *coeff,DistortMethod *method,
  const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    cp_size = context->cp_size;

  double
    **matrix,
    *vectors[1];

  MagickBooleanType
    status;

  /*
    Rigid affine (also known as a Euclidean transform), restricts affine
    coefficients to 4 (S, R, Tx, Ty) with Sy=Sx and Ry = -Rx so that one has
    only scale, rotation and translation. No skew.
  */
  if (((number_arguments % cp_size) != 0) || (number_arguments < cp_size))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument", "%s : 'require at least %.17g CPs'",
        CommandOptionToMnemonic(MagickDistortOptions,*method),2.0);
      coeff=(double *) RelinquishMagickMemory(coeff);
      return((double *) NULL);
    }
  /*
    Rigid affine requires a 4x4 least-squares matrix (zeroed).
  */
  matrix=AcquireMagickMatrix(4UL,4UL);
  if (matrix == (double **) NULL)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","%s",
        CommandOptionToMnemonic(MagickDistortOptions,*method));
      return((double *) NULL);
    }
  /*
    Add control points for least squares solving.
  */
  vectors[0]=(&(coeff[0]));
  AddRigidAffineTerms(matrix,vectors,arguments,number_arguments);
  /*
    Solve for least-squares coefficients.
  */
  status=GaussJordanElimination(matrix,vectors,4UL,1UL);
  matrix=RelinquishMagickMatrix(matrix,4UL);
  if (status == MagickFalse)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : 'Unsolvable Matrix'",
        CommandOptionToMnemonic(MagickDistortOptions,*method));
      return((double *) NULL);
    }
  ConvertRigidAffineCoefficients(coeff);
  *method=AffineDistortion;
  return(coeff);
}

static inline void SetAffineProjectionCoefficients(double *coeff,
  const double *arguments)
{
  size_t
    i;

  double inverse[8];
  for(i=0; i<6UL; i++ )
    inverse[i] = arguments[i];
  AffineArgsToCoefficients(inverse); /* map into coefficients */
  InvertAffineCoefficients(inverse, coeff); /* invert */
}

static inline double *AffineProjectionCoefficients(double *coeff,
  DistortMethod *method,const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments;

  /*
    Arguments: Affine Matrix (forward mapping)
    Arguments  sx, rx, ry, sy, tx, ty
    Where      u = sx*x + ry*y + tx
               v = rx*x + sy*y + ty

    Returns coefficients (in there inverse form) ordered as...
         sx ry tx  rx sy ty

    AffineProjection Distortion Notes...
       + Will only work with a 2 number_values for Image Distortion
       + Can not be used for generating a sparse gradient (interpolation)
  */
  if (number_arguments != 6) {
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidArgument","%s : 'Needs 6 coeff values'",
          CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return((double *) NULL);
  }
  /* FUTURE: trap test for sx*sy-rx*ry == 0 (determinant = 0, no inverse) */
  SetAffineProjectionCoefficients(coeff,arguments);
  *method = AffineDistortion;

  return(coeff);
}

typedef struct _ScaleRotateTranslateTargets
{
  double
    *sx,
    *sy,
    *a,
    *nx,
    *ny;
} ScaleRotateTranslateTargets;

static inline MagickBooleanType ParseScaleRotateTranslateArguments(double *coeff,
  const DistortMethod *method,const CoefficientContext *context,
  const ScaleRotateTranslateTargets *targets)
{
  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments;

  double
    *const sx = targets->sx,
    *const sy = targets->sy,
    *const a = targets->a,
    *const nx = targets->nx,
    *const ny = targets->ny;

  switch ( number_arguments ) {
  case 3:
    *a = arguments[2];
    break;
  case 4:
    *sx = *sy = arguments[2];
    *a = arguments[3];
    break;
  case 5:
    *sx = arguments[2];
    *sy = arguments[3];
    *a = arguments[4];
    break;
  case 6:
    *sx = *sy = arguments[2];
    *a = arguments[3];
    *nx = arguments[4];
    *ny = arguments[5];
    break;
  case 7:
    *sx = arguments[2];
    *sy = arguments[3];
    *a = arguments[4];
    *nx = arguments[5];
    *ny = arguments[6];
    break;
  default:
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : 'Too Many Arguments (7 or less)'",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return(MagickFalse);
  }
  return(MagickTrue);
}

static inline double *ScaleRotateTranslateCoefficients(double *coeff,
  DistortMethod *method,const CoefficientContext *context)
{
  const Image
    *const image = context->image;

  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments;

  /* Scale, Rotate and Translate Distortion
     An alternative Affine Distortion
     Argument options, by number of arguments given:
       7: x,y, sx,sy, a, nx,ny
       6: x,y,   s,   a, nx,ny
       5: x,y, sx,sy, a
       4: x,y,   s,   a
       3: x,y,        a
       2:        s,   a
       1:             a
     Where actions are (in order of application)
        x,y     'center' of transforms     (default = image center)
        sx,sy   scale image by this amount (default = 1)
        a       angle of rotation          (argument required)
        nx,ny   move 'center' here         (default = x,y or no movement)
     And convert to affine mapping coefficients

     ScaleRotateTranslate Distortion Notes...
       + Does not use a set of CPs in any normal way
       + Will only work with a 2 number_valuesal Image Distortion
       + Cannot be used for generating a sparse gradient (interpolation)
  */
  double
    cosine, sine,
    x,y,sx,sy,a,nx,ny;

  const ScaleRotateTranslateTargets
    targets = { &sx,&sy,&a,&nx,&ny };

  /* set default center, and default scale */
  x = nx = (double)(image->columns)/2.0 + (double)image->page.x;
  y = ny = (double)(image->rows)/2.0    + (double)image->page.y;
  sx = sy = 1.0;
  switch ( number_arguments ) {
  case 0:
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidArgument","%s : 'Needs at least 1 argument'",
          CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return((double *) NULL);
  case 1:
    a = arguments[0];
    break;
  case 2:
    sx = sy = arguments[0];
    a = arguments[1];
    break;
  default:
    x = nx = arguments[0];
    y = ny = arguments[1];
    if (ParseScaleRotateTranslateArguments(coeff,method,context,&targets) ==
        MagickFalse)
      return((double *) NULL);
    break;
  }
  /* Trap if sx or sy == 0 -- image is scaled out of existence! */
  if ( fabs(sx) < MagickEpsilon || fabs(sy) < MagickEpsilon ) {
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidArgument","%s : 'Zero Scale Given'",
          CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return((double *) NULL);
  }
  /* Save the given arguments as an affine distortion */
  a=DegreesToRadians(a); cosine=cos(a); sine=sin(a);

  *method = AffineDistortion;
  coeff[0]=cosine/sx;
  coeff[1]=sine/sx;
  coeff[2]=x-nx*coeff[0]-ny*coeff[1];
  coeff[3]=(-sine)/sy;
  coeff[4]=cosine/sy;
  coeff[5]=y-nx*coeff[3]-ny*coeff[4];
  return(coeff);
}

static inline void AddPerspectiveTerms(double **matrix,double *vectors[1],
  const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  const size_t
    number_arguments = context->number_arguments,
    cp_x = context->cp_x,
    cp_y = context->cp_y,
    cp_values = context->cp_values;

  double
    terms[8];

  size_t
    cp_u = cp_values,
    cp_v = cp_values+1;

  size_t
    i;

  /* Add control points for least squares solving */
  for (i=0; i < number_arguments; i+=4) {
    terms[0]=arguments[i+cp_x];            /*   c0*x   */
    terms[1]=arguments[i+cp_y];            /*   c1*y   */
    terms[2]=1.0;                          /*   c2*1   */
    terms[3]=0.0;
    terms[4]=0.0;
    terms[5]=0.0;
    terms[6]=-terms[0]*arguments[i+cp_u];  /* 1/(c6*x) */
    terms[7]=-terms[1]*arguments[i+cp_u];  /* 1/(c7*y) */
    LeastSquaresAddTerms(matrix,vectors,terms,&(arguments[i+cp_u]),
        8UL,1UL);

    terms[0]=0.0;
    terms[1]=0.0;
    terms[2]=0.0;
    terms[3]=arguments[i+cp_x];           /*   c3*x   */
    terms[4]=arguments[i+cp_y];           /*   c4*y   */
    terms[5]=1.0;                         /*   c5*1   */
    terms[6]=-terms[3]*arguments[i+cp_v]; /* 1/(c6*x) */
    terms[7]=-terms[4]*arguments[i+cp_v]; /* 1/(c7*y) */
    LeastSquaresAddTerms(matrix,vectors,terms,&(arguments[i+cp_v]),
        8UL,1UL);
  }
}

static inline void SetPerspectiveGroundSign(double *coeff,const double *arguments,
  const size_t cp_x,const size_t cp_y)
{
  /*
    Calculate 9'th coefficient! The ground-sky determination.
    What is sign of the 'ground' in r() denominator affine function?
    Just use any valid image coordinate (first control point) in
    destination for determination of what part of view is 'ground'.
  */
  coeff[8] = coeff[6]*arguments[cp_x]
                  + coeff[7]*arguments[cp_y] + 1.0;
  coeff[8] = (coeff[8] < 0.0) ? -1.0 : +1.0;
}

static inline double *PerspectiveCoefficients(double *coeff,
  const DistortMethod *method,const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    cp_size = context->cp_size,
    cp_x = context->cp_x,
    cp_y = context->cp_y;

  /*
     Perspective Distortion (a ratio of affine distortions)

            p(x,y)    c0*x + c1*y + c2
        u = ------ = ------------------
            r(x,y)    c6*x + c7*y + 1

            q(x,y)    c3*x + c4*y + c5
        v = ------ = ------------------
            r(x,y)    c6*x + c7*y + 1

       c8 = Sign of 'r', or the denominator affine, for the actual image.
            This determines what part of the distorted image is 'ground'
            side of the horizon, the other part is 'sky' or invalid.
            Valid values are  +1.0  or  -1.0  only.

     Input Arguments are sets of control points...
     For Distort Images    u,v, x,y  ...
     For Sparse Gradients  x,y, r,g,b  ...

     Perspective Distortion Notes...
       + Can be thought of as ratio of  3 affine transformations
       + Not separable: r() or c6 and c7 are used by both equations
       + All 8 coefficients must be determined simultaneously
       + Will only work with a 2 number_valuesal Image Distortion
       + Can not be used for generating a sparse gradient (interpolation)
       + It is not linear, but is simple to generate an inverse
       + All lines within an image remain lines.
       + but distances between points may vary.
  */
  double
    **matrix,
    *vectors[1];

  MagickBooleanType
    status;

  if ( number_arguments%cp_size != 0 ||
       number_arguments < cp_size*4 ) {
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidArgument", "%s : 'require at least %.17g CPs'",
          CommandOptionToMnemonic(MagickDistortOptions, *method), 4.0);
    coeff=(double *) RelinquishMagickMemory(coeff);
    return((double *) NULL);
  }
  /* fake 1x8 vectors matrix directly using the coefficients array */
  vectors[0] = &(coeff[0]);
  /* 8x8 least-squares matrix (zeroed) */
  matrix = AcquireMagickMatrix(8UL,8UL);
  if (matrix == (double **) NULL) {
    coeff=(double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed",
              "%s", "DistortCoefficients");
    return((double *) NULL);
  }
  AddPerspectiveTerms(matrix,vectors,context);
  /* Solve for LeastSquares Coefficients */
  status=GaussJordanElimination(matrix,vectors,8UL,1UL);
  matrix = RelinquishMagickMatrix(matrix, 8UL);
  if ( status == MagickFalse ) {
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : 'Unsolvable Matrix'",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return((double *) NULL);
  }
  SetPerspectiveGroundSign(coeff,arguments,cp_x,cp_y);

  return(coeff);
}

static inline double *PerspectiveProjectionCoefficients(double *coeff,
  DistortMethod *method,const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments;

  /*
    Arguments: Perspective Coefficients (forward mapping)
  */
  if (number_arguments != 8) {
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidArgument", "%s : 'Needs 8 coefficient values'",
          CommandOptionToMnemonic(MagickDistortOptions, *method));
    return((double *) NULL);
  }
  /* FUTURE: trap test  c0*c4-c3*c1 == 0  (determinate = 0, no inverse) */
  InvertPerspectiveCoefficients(arguments, coeff);
  /*
    Calculate 9'th coefficient! The ground-sky determination.
    What is sign of the 'ground' in r() denominator affine function?
    Just use any valid image coordinate in destination for determination.
    For a forward mapped perspective the images 0,0 coord will map to
    c2,c5 in the distorted image, so set the sign of denominator of that.
  */
  coeff[8] = coeff[6]*arguments[2]
                       + coeff[7]*arguments[5] + 1.0;
  coeff[8] = (coeff[8] < 0.0) ? -1.0 : +1.0;
  *method = PerspectiveDistortion;

  return(coeff);
}

static inline void SetBilinearVectors(double **vectors,double *coeff,
  const size_t number_values)
{
  size_t
    i;

  /* fake a number_values x4 vectors matrix from coefficients array */
  for (i=0; i < number_values; i++)
    vectors[i] = &(coeff[i*4]);
}

static inline MagickBooleanType SolveBilinearCoefficients(double *coeff,
  double **matrix,double **vectors,const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  const size_t
    number_arguments = context->number_arguments,
    number_values = context->number_values,
    cp_size = context->cp_size,
    cp_x = context->cp_x,
    cp_y = context->cp_y,
    cp_values = context->cp_values;

  double
    terms[4];

  size_t
    i;

  MagickBooleanType
    status;

  SetBilinearVectors(vectors,coeff,number_values);
  /* Add given control point pairs for least squares solving */
  for (i=0; i < number_arguments; i+=cp_size) {
    terms[0] = arguments[i+cp_x];   /*  x  */
    terms[1] = arguments[i+cp_y];   /*  y  */
    terms[2] = terms[0]*terms[1];   /* x*y */
    terms[3] = 1;                   /*  1  */
    LeastSquaresAddTerms(matrix,vectors,terms,
         &(arguments[i+cp_values]),4UL,number_values);
  }
  /* Solve for LeastSquares Coefficients */
  status=GaussJordanElimination(matrix,vectors,4UL,number_values);
  return(status);
}

static inline void SetBilinearForwardCoefficients(double *coeff,
  const DistortMethod *method)
{
  if ( *method == BilinearForwardDistortion ) {
     /* Bilinear Forward Mapped Distortion

     The above least-squares solved for coefficients but in the forward
     direction, due to changes to indexing constants.

        i = c0*x + c1*y + c2*x*y + c3;
        j = c4*x + c5*y + c6*x*y + c7;

     where i,j are in the destination image, NOT the source.

     Reverse Pixel mapping however needs to use reverse of these
     functions.  It required a full page of algebra to work out the
     reversed mapping formula, but resolves down to the following...

        c8 = c0*c5-c1*c4;
        c9 = 2*(c2*c5-c1*c6);   // '2*a' in the quadratic formula

        i = i - c3;   j = j - c7;
        b = c6*i - c2*j + c8;   // So that   a*y^2 + b*y + c == 0
        c = c4*i -  c0*j;       // y = ( -b +- sqrt(bb - 4ac) ) / (2*a)

        r = b*b - c9*(c+c);
        if ( c9 != 0 )
          y = ( -b + sqrt(r) ) / c9;
        else
          y = -c/b;

        x = ( i - c1*y) / ( c1 - c2*y );

     NB: if 'r' is negative there is no solution!
     NB: the sign of the sqrt() should be negative if image becomes
         flipped or flopped, or crosses over itself.
     NB: technically coefficient c5 is not needed, anymore,
         but kept for completeness.

     See Anthony Thyssen <A.Thyssen@griffith.edu.au>
     or  Fred Weinhaus <fmw@alink.net>  for more details.

     */
     coeff[8] = coeff[0]*coeff[5] - coeff[1]*coeff[4];
     coeff[9] = 2*(coeff[2]*coeff[5] - coeff[1]*coeff[6]);
  }
}

static inline double *BilinearCoefficients(double *coeff,const DistortMethod *method,
  const CoefficientContext *context)
{
  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    number_values = context->number_values,
    cp_size = context->cp_size;

  /* Bilinear Distortion (Forward mapping)
        v = c0*x + c1*y + c2*x*y + c3;
     for each 'value' given

     This is actually a simple polynomial Distortion!  The difference
     however is when we need to reverse the above equation to generate a
     BilinearForwardDistortion (see below).

     Input Arguments are sets of control points...
     For Distort Images    u,v, x,y  ...
     For Sparse Gradients  x,y, r,g,b  ...

  */
  double
    **matrix,
    **vectors;

  MagickBooleanType
    status;

  /* check the number of arguments */
  if ( number_arguments%cp_size != 0 ||
       number_arguments < cp_size*4 ) {
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "InvalidArgument", "%s : 'require at least %.17g CPs'",
          CommandOptionToMnemonic(MagickDistortOptions, *method), 4.0);
    coeff=(double *) RelinquishMagickMemory(coeff);
    return((double *) NULL);
  }
  /* create matrix, and a fake vectors matrix */
  matrix=AcquireMagickMatrix(4UL,4UL);
  vectors=(double **) AcquireQuantumMemory(number_values,sizeof(*vectors));
  if (matrix == (double **) NULL || vectors == (double **) NULL)
  {
    matrix  = RelinquishMagickMatrix(matrix, 4UL);
    vectors = (double **) RelinquishMagickMemory(vectors);
    coeff   = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed",
            "%s", "DistortCoefficients");
    return((double *) NULL);
  }
  status=SolveBilinearCoefficients(coeff,matrix,vectors,context);
  matrix  = RelinquishMagickMatrix(matrix, 4UL);
  vectors = (double **) RelinquishMagickMemory(vectors);
  if ( status == MagickFalse ) {
    coeff = (double *) RelinquishMagickMemory(coeff);
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "InvalidArgument","%s : 'Unsolvable Matrix'",
        CommandOptionToMnemonic(MagickDistortOptions, *method) );
    return((double *) NULL);
  }
  SetBilinearForwardCoefficients(coeff,method);
  return(coeff);
}

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

static inline double *PolarCoefficients(double *coeff,const DistortMethod *method,
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

static inline double *CylinderCoefficients(double *coeff,const DistortMethod *method,
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

static inline double *BarrelCoefficients(double *coeff,const DistortMethod *method,
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

static inline double *GenerateRadialCoefficients(double *coeff,DistortMethod *method,
  const CoefficientContext *context)
{
  switch (*method)
  {
    case PolarDistortion:
    case DePolarDistortion:
      return(PolarCoefficients(coeff,method,context));
    case Cylinder2PlaneDistortion:
    case Plane2CylinderDistortion:
      return(CylinderCoefficients(coeff,method,context));
    case BarrelDistortion:
    case BarrelInverseDistortion:
      return(BarrelCoefficients(coeff,method,context));
    default:
      break;
  }
  /* you should never reach this point */
  perror("no method handler"); /* just fail assertion */
  return((double *) NULL);
}

static inline double *GenerateMappedCoefficients(double *coeff,DistortMethod *method,
  const CoefficientContext *context)
{
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
  return(GenerateRadialCoefficients(coeff,method,context));
}

static inline double *GenerateAffineFamilyCoefficients(double *coeff,
  DistortMethod *method,const CoefficientContext *context)
{
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
  return(GenerateMappedCoefficients(coeff,method,context));
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

  return(GenerateAffineFamilyCoefficients(coeff,method,&context));
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
