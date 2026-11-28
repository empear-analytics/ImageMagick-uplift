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
  coefficient helpers and the affine, perspective and bilinear coefficient
  generators, from AffineArgsToCoefficients to BilinearCoefficients;
  included by distort-private.h only.
*/
#ifndef MAGICKCORE_DISTORT_LINEAR_PRIVATE_H
#define MAGICKCORE_DISTORT_LINEAR_PRIVATE_H

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

typedef struct _CoefficientCall
{
  double
    *coeff;

  DistortMethod
    *method;

  const CoefficientContext
    *context;
} CoefficientCall;

static inline MagickBooleanType IsInvalidPolarRadius(const CoefficientCall *call)
{
  const double
    *const coeff = call->coeff;

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

static inline void SetAffineSingleCoefficients(const CoefficientCall *call)
{
  double
    *const coeff = call->coeff;

  const CoefficientContext
    *const context = call->context;

  const double
    *const arguments = context->arguments;

  const size_t
    cp_values = context->cp_values,
    number_values = context->number_values;

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

    const LeastSquaresSystem
      least_squares = { matrix, vectors, 3UL, 2UL };

    uv2[0] = arguments[0] - arguments[5] + arguments[1];   /* u2 */
    uv2[1] = arguments[1] + arguments[4] - arguments[0];   /* v2 */
    LeastSquaresAddTerms(&least_squares,terms,uv2);
  }
  else {
    /* Sparse Gradient - use values of p0 for linear gradient */
    {
      const LeastSquaresSystem
        least_squares = { matrix, vectors, 3UL, number_values };

      LeastSquaresAddTerms(&least_squares,terms,&(arguments[cp_values]));
    }
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

  const LeastSquaresSystem
    least_squares = { matrix, vectors, 3UL, number_values };

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
    LeastSquaresAddTerms(&least_squares,terms,&(arguments[i+cp_values]));
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

static inline double *AffineCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  const DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    number_values = context->number_values,
    cp_size = context->cp_size;

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
    SetAffineSingleCoefficients(call);
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
  const CoefficientContext *context)
{
  const double
    *const arguments = context->arguments;

  const size_t
    number_arguments = context->number_arguments;

  const LeastSquaresSystem
    least_squares = { matrix, vectors, 4UL, 1UL };

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
    LeastSquaresAddTerms(&least_squares,terms,&(arguments[i+2]));
    terms[0]=arguments[i+1];
    terms[1]=arguments[i+0];
    terms[2]=0.0;
    terms[3]=1.0;
    LeastSquaresAddTerms(&least_squares,terms,&(arguments[i+3]));
  }
}

static inline void ConvertRigidAffineCoefficients(const CoefficientCall *call)
{
  double
    *const coeff = call->coeff;

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

static inline double *RigidAffineCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

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
  AddRigidAffineTerms(matrix,vectors,context);
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
  ConvertRigidAffineCoefficients(call);
  *method=AffineDistortion;
  return(coeff);
}

static inline void SetAffineProjectionCoefficients(const CoefficientCall *call)
{
  double
    *const coeff = call->coeff;

  const CoefficientContext
    *const context = call->context;

  const double
    *const arguments = context->arguments;

  size_t
    i;

  double inverse[8];
  for(i=0; i<6UL; i++ )
    inverse[i] = arguments[i];
  AffineArgsToCoefficients(inverse); /* map into coefficients */
  InvertAffineCoefficients(inverse, coeff); /* invert */
}

static inline double *AffineProjectionCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

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
  SetAffineProjectionCoefficients(call);
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

static inline MagickBooleanType ParseScaleRotateTranslateArguments(
  const CoefficientCall *call,const ScaleRotateTranslateTargets *targets)
{
  double
    *coeff = call->coeff;

  const DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

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

static inline double *ScaleRotateTranslateCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  DistortMethod
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
    if (ParseScaleRotateTranslateArguments(call,&targets) ==
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

  const LeastSquaresSystem
    least_squares = { matrix, vectors, 8UL, 1UL };

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
    LeastSquaresAddTerms(&least_squares,terms,&(arguments[i+cp_u]));

    terms[0]=0.0;
    terms[1]=0.0;
    terms[2]=0.0;
    terms[3]=arguments[i+cp_x];           /*   c3*x   */
    terms[4]=arguments[i+cp_y];           /*   c4*y   */
    terms[5]=1.0;                         /*   c5*1   */
    terms[6]=-terms[3]*arguments[i+cp_v]; /* 1/(c6*x) */
    terms[7]=-terms[4]*arguments[i+cp_v]; /* 1/(c7*y) */
    LeastSquaresAddTerms(&least_squares,terms,&(arguments[i+cp_v]));
  }
}

static inline void SetPerspectiveGroundSign(const CoefficientCall *call)
{
  double
    *const coeff = call->coeff;

  const CoefficientContext
    *const context = call->context;

  const double
    *const arguments = context->arguments;

  const size_t
    cp_x = context->cp_x,
    cp_y = context->cp_y;

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

static inline double *PerspectiveCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  const DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

  ExceptionInfo
    *const exception = context->exception;

  const size_t
    number_arguments = context->number_arguments,
    cp_size = context->cp_size;

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
  SetPerspectiveGroundSign(call);

  return(coeff);
}

static inline double *PerspectiveProjectionCoefficients(
  const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

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

  const LeastSquaresSystem
    least_squares = { matrix, vectors, 4UL, number_values };

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
    LeastSquaresAddTerms(&least_squares,terms,&(arguments[i+cp_values]));
  }
  /* Solve for LeastSquares Coefficients */
  status=GaussJordanElimination(matrix,vectors,4UL,number_values);
  return(status);
}

static inline void SetBilinearForwardCoefficients(const CoefficientCall *call)
{
  double
    *const coeff = call->coeff;

  const DistortMethod
    *const method = call->method;

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

static inline double *BilinearCoefficients(const CoefficientCall *call)
{
  double
    *coeff = call->coeff;

  const DistortMethod
    *const method = call->method;

  const CoefficientContext
    *const context = call->context;

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
  SetBilinearForwardCoefficients(call);
  return(coeff);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
