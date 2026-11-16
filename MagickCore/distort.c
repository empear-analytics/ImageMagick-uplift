/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               DDDD   IIIII  SSSSS  TTTTT   OOO   RRRR   TTTTT               %
%               D   D    I    SS       T    O   O  R   R    T                 %
%               D   D    I     SSS     T    O   O  RRRR     T                 %
%               D   D    I       SS    T    O   O  R R      T                 %
%               DDDD   IIIII  SSSSS    T     OOO   R  R     T                 %
%                                                                             %
%                                                                             %
%                     MagickCore Image Distortion Methods                     %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                              Anthony Thyssen                                %
%                                 June 2007                                   %
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
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/list.h"
#include "MagickCore/matrix.h"
#include "MagickCore/matrix-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/registry.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/shear.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform.h"
#include "MagickCore/distort-private.h"



static inline void CoefficientsToAffineArgs(double *coeff)
{
  /* map  internal c0,c1,c2,c3,c4,c5  to  external sx,ry,rx,sy,tx,ty */
  double tmp[4];  /* note indexes 0 and 5 remain unchanged */
  tmp[0]=coeff[3]; tmp[1]=coeff[1]; tmp[2]=coeff[4]; tmp[3]=coeff[2];
  coeff[1]=tmp[0]; coeff[2]=tmp[1]; coeff[3]=tmp[2]; coeff[4]=tmp[3];
}

static const char
  *const PolyBasisStrings[] =
  {
    "", /* constant */
    "*ii",
    "*jj", /* affine                order = 1   terms = 3 */
    "*ii*jj", /* bilinear           order = 1.5 terms = 4 */
    "*ii*ii",
    "*jj*jj", /* quadratic          order = 2   terms = 6 */
    "*ii*ii*ii",
    "*ii*ii*jj",
    "*ii*jj*jj",
    "*jj*jj*jj", /* cubic           order = 3   terms = 10 */
    "*ii*ii*ii*ii",
    "*ii*ii*ii*jj",
    "*ii*ii*jj*jj",
    "*ii*jj*jj*jj",
    "*jj*jj*jj*jj", /* quartic      order = 4   terms = 15 */
    "*ii*ii*ii*ii*ii",
    "*ii*ii*ii*ii*jj",
    "*ii*ii*ii*jj*jj",
    "*ii*ii*jj*jj*jj",
    "*ii*jj*jj*jj*jj",
    "*jj*jj*jj*jj*jj" /* quintic   order = 5   terms = 21 */
  };

static const char *poly_basis_str(ssize_t n)
{
  /* return the result for this polynomial term */
  if ((size_t) n < (sizeof(PolyBasisStrings)/sizeof(*PolyBasisStrings)))
    return(PolyBasisStrings[n]);
  return( "UNKNOWN" ); /* should never happen */
}
static MagickBooleanType IsPolyBasisDxZero(const ssize_t n)
{
  switch(n) {
    case  0:  /* constant */
    case  2:  /* affine      order = 1   terms = 3 */
    case  5:  /* quadratic   order = 2   terms = 6 */
    case  9:  /* cubic       order = 3   terms = 10 */
    case 14:  /* quartic     order = 4   terms = 15 */
    case 20:  /* quintic     order = 5   terms = 21 */
      return(MagickTrue);
    default:  break;
  }
  return(MagickFalse);
}
static double poly_basis_dx(ssize_t n, double x, double y)
{
  /* polynomial term for x derivative */
  if (IsPolyBasisDxZero(n) != MagickFalse)
    return( 0.0 );
  switch(n) {
    case  1:  return( 1.0 );
    case  3:  return(  y  ); /* bilinear    order = 1.5 terms = 4 */
    case  4:  return(  x  );
    case  6:  return( x*x );
    case  7:  return( x*y );
    case  8:  return( y*y );
    case 10:  return( x*x*x );
    case 11:  return( x*x*y );
    case 12:  return( x*y*y );
    case 13:  return( y*y*y );
    case 15:  return( x*x*x*x );
    case 16:  return( x*x*x*y );
    case 17:  return( x*x*y*y );
    case 18:  return( x*y*y*y );
    case 19:  return( y*y*y*y );
  }
  return( 0.0 ); /* should never happen */
}
static double poly_basis_dy(ssize_t n, double x, double y)
{
  /* polynomial term for y derivative */
  switch(n) {
    case  0:  return( 0.0 ); /* constant */
    case  1:  return( 0.0 );
    case  2:  return( 1.0 ); /* affine      order = 1   terms = 3 */
    case  3:  return(  x  ); /* bilinear    order = 1.5 terms = 4 */
    case  4:  return( 0.0 );
    case  5:  return(  y  ); /* quadratic   order = 2   terms = 6 */
    default:  return( poly_basis_dx(n-1,x,y) ); /* weird but true */
  }
  /* NOTE: the only reason that last is not true for 'quadratic'
     is due to the re-arrangement of terms to allow for 'bilinear'
  */
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e n e r a t e C o e f f i c i e n t s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GenerateCoefficients() takes user provided input arguments and generates
%  the coefficients, needed to apply the specific distortion for either
%  distorting images (generally using control points) or generating a color
%  gradient from sparsely separated color points.
%
%  The format of the GenerateCoefficients() method is:
%
%    Image *GenerateCoefficients(const Image *image,DistortMethod method,
%        const size_t number_arguments,const double *arguments,
%        size_t number_values, ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image to be distorted.
%
%    o method: the method of image distortion/ sparse gradient
%
%    o number_arguments: the number of arguments given.
%
%    o arguments: the arguments for this distortion method.
%
%    o number_values: the style and format of given control points, (caller type)
%         0: 2 dimensional mapping of control points (Distort)
%            Format:  u,v,x,y  where u,v is the 'source' of the
%            the color to be plotted, for DistortImage()
%         N: Interpolation of control points with N values (usually r,g,b)
%            Format: x,y,r,g,b    mapping x,y to color values r,g,b
%            IN future, variable number of values may be given (1 to N)
%
%    o exception: return any errors or warnings in this structure
%
%  Note that the returned array of double values must be freed by the
%  calling method using RelinquishMagickMemory().  This however may change in
%  the future to require a more 'method' specific method.
%
%  Because of this, this method should not be classed as stable or used
%  outside other MagickCore library methods.
*/



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D i s t o r t I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DistortImage() distorts an image using various distortion methods, by
%  mapping color lookups of the source image to a new destination image
%  usually of the same size as the source image, unless 'bestfit' is set to
%  true.
%
%  If 'bestfit' is enabled, and distortion allows it, the destination image is
%  adjusted to ensure the whole source 'image' will just fit within the final
%  destination image, which will be sized and offset accordingly.  Also in
%  many cases the virtual offset of the source image will be taken into
%  account in the mapping.
%
%  If the '-verbose' control option has been set print to standard error the
%  equivalent '-fx' formula with coefficients for the function, if practical.
%
%  The format of the DistortImage() method is:
%
%      Image *DistortImage(const Image *image,const DistortMethod method,
%        const size_t number_arguments,const double *arguments,
%        MagickBooleanType bestfit, ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image to be distorted.
%
%    o method: the method of image distortion.
%
%        ArcDistortion always ignores source image offset, and always
%        'bestfit' the destination image with the top left corner offset
%        relative to the polar mapping center.
%
%        Affine, Perspective, and Bilinear, do least squares fitting of the
%        distortion when more than the minimum number of control point pairs
%        are provided.
%
%        Perspective, and Bilinear, fall back to a Affine distortion when less
%        than 4 control point pairs are provided.  While Affine distortions
%        let you use any number of control point pairs, that is Zero pairs is
%        a No-Op (viewport only) distortion, one pair is a translation and
%        two pairs of control points do a scale-rotate-translate, without any
%        shearing.
%
%    o number_arguments: the number of arguments given.
%
%    o arguments: an array of floating point arguments for this method.
%
%    o bestfit: Attempt to 'bestfit' the size of the resulting image.
%        This also forces the resulting image to be a 'layered' virtual
%        canvas image.  Can be overridden using 'distort:viewport' setting.
%
%    o exception: return any errors or warnings in this structure
%
%  Extra Controls from Image meta-data (artifacts)...
%
%    o "verbose"
%        Output to stderr alternatives, internal coefficients, and FX
%        equivalents for the distortion operation (if feasible).
%        This forms an extra check of the distortion method, and allows users
%        access to the internal constants IM calculates for the distortion.
%
%    o "distort:viewport"
%        Directly set the output image canvas area and offset to use for the
%        resulting image, rather than use the original images canvas, or a
%        calculated 'bestfit' canvas.
%
%    o "distort:scale"
%        Scale the size of the output canvas by this amount to provide a
%        method of Zooming, and for super-sampling the results.
%
%  Other settings that can effect results include
%
%    o 'interpolate' For source image lookups (scale enlargements)
%
%    o 'filter'      Set filter to use for area-resampling (scale shrinking).
%                    Set to 'point' to turn off and use 'interpolate' lookup
%                    instead
%
*/
static inline MagickBooleanType IsArcPageReset(const DistortMethod method,
  const MagickBooleanType bestfit,const MagickBooleanType viewport_given)
{
  return(( method == ArcDistortion && !bestfit && !viewport_given ) ?
    MagickTrue : MagickFalse);
}

static void PrintArcFX(const double *coeff,const char *image_gen)
{
  ssize_t
    i;

  (void) FormatLocaleFile(stderr,"Arc Distort, Internal Coefficients:\n");
  for (i=0; i < 5; i++)
    (void) FormatLocaleFile(stderr,
      "  c%.17g = %+lf\n",(double) i,coeff[i]);
  (void) FormatLocaleFile(stderr,"Arc Distort, FX Equivalent:\n");
  (void) FormatLocaleFile(stderr,"%s", image_gen);
  (void) FormatLocaleFile(stderr,"  -fx 'ii=i+page.x; jj=j+page.y;\n");
  (void) FormatLocaleFile(stderr,"       xx=(atan2(jj,ii)%+lf)/(2*pi);\n",
    -coeff[0]);
  (void) FormatLocaleFile(stderr,"       xx=xx-round(xx);\n");
  (void) FormatLocaleFile(stderr,"       xx=xx*%lf %+lf;\n",coeff[1],
    coeff[4]);
  (void) FormatLocaleFile(stderr,
    "       yy=(%lf - hypot(ii,jj)) * %lf;\n",coeff[2],coeff[3]);
  (void) FormatLocaleFile(stderr,"       v.p{xx-.5,yy-.5}' \\\n");
}

static void PrintDePolarFX(const double *coeff,const char *image_gen)
{
  ssize_t
    i;

  (void) FormatLocaleFile(stderr,
    "DePolar Distort, Internal Coefficients\n");
  for (i=0; i < 8; i++)
    (void) FormatLocaleFile(stderr,"  c%.17g = %+lf\n",(double) i,
      coeff[i]);
  (void) FormatLocaleFile(stderr,"DePolar Distort, FX Equivalent:\n");
  (void) FormatLocaleFile(stderr,"%s", image_gen);
  (void) FormatLocaleFile(stderr,"  -fx 'aa=(i+.5)*%lf %+lf;\n",
    coeff[6],+coeff[4]);
  (void) FormatLocaleFile(stderr,"       rr=(j+.5)*%lf %+lf;\n",
    coeff[7],+coeff[1]);
  (void) FormatLocaleFile(stderr,"       xx=rr*sin(aa) %+lf;\n",
    coeff[2]);
  (void) FormatLocaleFile(stderr,"       yy=rr*cos(aa) %+lf;\n",
    coeff[3]);
  (void) FormatLocaleFile(stderr,"       v.p{xx-.5,yy-.5}' \\\n");
}

static void PrintCylinder2PlaneFX(const double *coeff,const char *image_gen,
  const char *lookup)
{
  (void) FormatLocaleFile(stderr,
    "Cylinder to Plane Distort, Internal Coefficients\n");
  (void) FormatLocaleFile(stderr,"  cylinder_radius = %+lf\n",coeff[1]);
  (void) FormatLocaleFile(stderr,
    "Cylinder to Plane Distort, FX Equivalent:\n");
  (void) FormatLocaleFile(stderr, "%s", image_gen);
  (void) FormatLocaleFile(stderr,
    "  -fx 'ii=i+page.x%+lf+0.5; jj=j+page.y%+lf+0.5;\n",-coeff[4],
    -coeff[5]);
  (void) FormatLocaleFile(stderr,"       aa=atan(ii/%+lf);\n",coeff[1]);
  (void) FormatLocaleFile(stderr,"       xx=%lf*aa%+lf;\n",
    coeff[1],coeff[2]);
  (void) FormatLocaleFile(stderr,"       yy=jj*cos(aa)%+lf;\n",coeff[3]);
  (void) FormatLocaleFile(stderr,"       %s' \\\n", lookup);
}

static void PrintBarrelCenter(const double *coeff,const double xc,
  const double yc)
{
  if ( fabs(coeff[8]-xc-0.5) < 0.1 && fabs(coeff[9]-yc-0.5) < 0.1 )
    (void) FormatLocaleFile(stderr,"  -fx 'xc=(w-1)/2;  yc=(h-1)/2;\n");
  else
    (void) FormatLocaleFile(stderr,"  -fx 'xc=%lf;  yc=%lf;\n",coeff[8]-
      0.5,coeff[9]-0.5);
}

static void PrintBarrelFX(const Image *image,const double *coeff,
  const DistortMethod method,const char *image_gen)
{
  double
    xc,
    yc;

  /*
    NOTE: This does the barrel roll in pixel coords not image coords
    The internal distortion must do it in image coordinates,
    so that is what the center coeff (8,9) is given in.
  */
  xc=((double)image->columns-1.0)/2.0+image->page.x;
  yc=((double)image->rows-1.0)/2.0+image->page.y;
  (void) FormatLocaleFile(stderr, "Barrel%s Distort, FX Equivalent:\n",
    method == BarrelDistortion ? "" : "Inv");
  (void) FormatLocaleFile(stderr, "%s", image_gen);
  PrintBarrelCenter(coeff,xc,yc);
  (void) FormatLocaleFile(stderr,
    "       ii=i-xc;  jj=j-yc;  rr=hypot(ii,jj);\n");
  (void) FormatLocaleFile(stderr,
    "       ii=ii%s(%lf*rr*rr*rr %+lf*rr*rr %+lf*rr %+lf);\n",
    method == BarrelDistortion ? "*" : "/",coeff[0],coeff[1],coeff[2],
    coeff[3]);
  (void) FormatLocaleFile(stderr,
    "       jj=jj%s(%lf*rr*rr*rr %+lf*rr*rr %+lf*rr %+lf);\n",
    method == BarrelDistortion ? "*" : "/",coeff[4],coeff[5],coeff[6],
    coeff[7]);
  (void) FormatLocaleFile(stderr,"       p{ii+xc,jj+yc}' \\\n");
}

static void PrintPolynomialXTerms(const double *coeff,const size_t nterms)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) nterms; i++)
  {
    if ((i != 0) && (i%4 == 0))
      (void) FormatLocaleFile(stderr, "\n         ");
    (void) FormatLocaleFile(stderr," %+lf%s",coeff[2+i],
      poly_basis_str(i));
  }
}

static void PrintPolynomialFX(const double *coeff,const char *image_gen,
  const char *lookup)
{
  size_t
    nterms =  CastDoubleToSizeT(coeff[1]);

  ssize_t
    i;

  (void) FormatLocaleFile(stderr,
    "Polynomial (order %lg, terms %lu), FX Equivalent\n",coeff[0],
    (unsigned long) nterms);
  (void) FormatLocaleFile(stderr,"%s", image_gen);
  (void) FormatLocaleFile(stderr,
    "  -fx 'ii=i+page.x+0.5; jj=j+page.y+0.5;\n");
  (void) FormatLocaleFile(stderr, "       xx =");
  PrintPolynomialXTerms(coeff,nterms);
  (void) FormatLocaleFile(stderr,";\n       yy =");
  for (i=0; i < (ssize_t) nterms; i++)
  {
    if ((i != 0) && (i%4 == 0))
      (void) FormatLocaleFile(stderr,"\n         ");
    (void) FormatLocaleFile(stderr," %+lf%s",coeff[2+i+(int) nterms],
      poly_basis_str(i));
  }
  (void) FormatLocaleFile(stderr,";\n       %s' \\\n", lookup);
}

static void ParseDistortViewport(const char *artifact,
  RectangleInfo *geometry,MagickBooleanType *viewport_given,
  ExceptionInfo *exception)
{
  if ( artifact != (const char *) NULL ) {
    MagickStatusType flags=ParseAbsoluteGeometry(artifact,geometry);
    if (flags==NoValue)
      (void) ThrowMagickException(exception,GetMagickModule(),
           OptionWarning,"InvalidSetting","'%s' '%s'",
           "distort:viewport",artifact);
    else
      *viewport_given = MagickTrue;
  }
}

static void GetDistortViewport(const Image *image,RectangleInfo *geometry,
  MagickBooleanType *viewport_given,ExceptionInfo *exception)
{
  { const char *artifact=GetImageArtifact(image,"distort:viewport");
    *viewport_given = MagickFalse;
    ParseDistortViewport(artifact,geometry,viewport_given,exception);
  }
}

static PointInfo ShepardsSourcePoint(const PointInfo d,
  const size_t number_arguments,const double *arguments,const double *coeff)
{
  double
    denominator;

  PointInfo
    s;

  size_t
    k;

  denominator = s.x = s.y = 0;
  for(k=0; k<number_arguments; k+=4) {
    double weight =
        ((double)d.x-arguments[k+2])*((double)d.x-arguments[k+2])
      + ((double)d.y-arguments[k+3])*((double)d.y-arguments[k+3]);
    weight = pow(weight,coeff[0]); /* shepards power factor */
    weight = ( weight < 1.0 ) ? 1.0 : 1.0/weight;

    s.x += (arguments[ k ]-arguments[k+2])*weight;
    s.y += (arguments[k+1]-arguments[k+3])*weight;
    denominator += weight;
  }
  s.x /= denominator;
  s.y /= denominator;
  s.x += d.x;   /* make it as relative displacement */
  s.y += d.y;
  return(s);
}

static void MapBilinearForwardPoint(const double *coeff,PointInfo *d,
  PointInfo *s,double *validity)
{
  double
    b,
    c;

  (*d).x -= coeff[3];  (*d).y -= coeff[7];
  b = coeff[6]*(*d).x - coeff[2]*(*d).y + coeff[8];
  c = coeff[4]*(*d).x - coeff[0]*(*d).y;

  (*validity) = 1.0;
  /* Handle Special degenerate (non-quadratic) case
   * Currently without horizon anti-aliasing */
  if ( fabs(coeff[9]) < MagickEpsilon )
    (*s).y =  -c/b;
  else {
    c = b*b - 2*coeff[9]*c;
    if ( c < 0.0 )
      (*validity) = 0.0;
    else
      (*s).y = ( -b + sqrt(c) )/coeff[9];
  }
  if ( (*validity) > 0.0 )
    (*s).x = ( (*d).x - coeff[1]*(*s).y) / ( coeff[0] + coeff[2]*(*s).y );

  /* NOTE: the sign of the square root should be -ve for parts
           where the source image becomes 'flipped' or 'mirrored'.
     FUTURE: Horizon handling
     FUTURE: Scaling factors or Derivatives (how?)
  */
}

#define ScaleFilter(F,A,B,C,D) \
    ScaleResampleFilter( (F), \
      output_scaling*(A), output_scaling*(B), \
      output_scaling*(C), output_scaling*(D) )

typedef struct _DistortPixelInput
{
  Image
    *distort_image;

  ResampleFilter
    **resample_filter;

  int
    id;

  PointInfo
    s;

  double
    validity;

  PixelInfo
    invalid;

  ExceptionInfo
    *exception;
} DistortPixelInput;

static MagickBooleanType ResampleDistortPixel(const DistortPixelInput *input,
  PixelInfo *pixel,Quantum *magick_restrict q)
{
  Image
    *const distort_image = input->distort_image;

  ResampleFilter
    **const resample_filter = input->resample_filter;

  const int
    id = input->id;

  const PointInfo
    s = input->s;

  const double
    validity = input->validity;

  const PixelInfo
    invalid = input->invalid;

  ExceptionInfo
    *const exception = input->exception;

  MagickBooleanType
    status;

  /* resample the source image to find its correct color */
  status=ResamplePixelColor(resample_filter[id],s.x,s.y,pixel,
    exception);
  if (status == MagickFalse)
    SetPixelViaPixelInfo(distort_image,&invalid,q);
  else
    {
      /* if validity between 0.0 & 1.0 mix result with invalid pixel */
      if ( validity < 1.0 ) {
        /* Do a blend of sample color and invalid pixel */
        /* should this be a 'Blend', or an 'Over' compose */
        CompositePixelInfoBlend(pixel,validity,&invalid,(1.0-validity),
          pixel);
      }
      SetPixelViaPixelInfo(distort_image,pixel,q);
    }
  return(status);
}

typedef struct _DistortMapping
{
  PointInfo
    d;

  double
    *coeff,
    output_scaling;

  ResampleFilter
    **resample_filter;

  int
    id;

  const Image
    *image;

  Image
    *distort_image;

  DistortMethod
    method;
} DistortMapping;

static void MapPerspectivePoint(const DistortMapping *mapping,double *validity,
  PointInfo *s)
{
  const PointInfo
    d = mapping->d;

  double
    *const coeff = mapping->coeff;

  const double
    output_scaling = mapping->output_scaling;

  ResampleFilter
    **const resample_filter = mapping->resample_filter;

  const int
    id = mapping->id;

  double
    p,n,r,abs_r,abs_c6,abs_c7,scale;

  /* perspective is a ratio of affines */
  p=coeff[0]*d.x+coeff[1]*d.y+coeff[2];
  n=coeff[3]*d.x+coeff[4]*d.y+coeff[5];
  r=coeff[6]*d.x+coeff[7]*d.y+1.0;
  /* Pixel Validity -- is it a 'sky' or 'ground' pixel */
  (*validity) = (r*coeff[8] < 0.0) ? 0.0 : 1.0;
  /* Determine horizon anti-alias blending */
  abs_r = fabs(r)*2;
  abs_c6 = fabs(coeff[6]);
  abs_c7 = fabs(coeff[7]);
  if ( abs_c6 > abs_c7 ) {
    if ( abs_r < abs_c6*output_scaling )
      (*validity) = 0.5 - coeff[8]*r/(coeff[6]*output_scaling);
  }
  else if ( abs_r < abs_c7*output_scaling )
    (*validity) = 0.5 - coeff[8]*r/(coeff[7]*output_scaling);
  /* Perspective Sampling Point (if valid) */
  if ( (*validity) > 0.0 ) {
    /* divide by r affine, for perspective scaling */
    scale = 1.0/r;
    (*s).x = p*scale;
    (*s).y = n*scale;
    /* Perspective Partial Derivatives or Scaling Vectors */
    scale *= scale;
    ScaleFilter( resample_filter[id],
      (r*coeff[0] - p*coeff[6])*scale,
      (r*coeff[1] - p*coeff[7])*scale,
      (r*coeff[3] - n*coeff[6])*scale,
      (r*coeff[4] - n*coeff[7])*scale );
  }
}

static PointInfo MapPolynomialPoint(const DistortMapping *mapping)
{
  const PointInfo
    d = mapping->d;

  double
    *const coeff = mapping->coeff;

  const double
    output_scaling = mapping->output_scaling;

  ResampleFilter
    **const resample_filter = mapping->resample_filter;

  const int
    id = mapping->id;

  PointInfo
    s;

  ssize_t
    k;

  ssize_t
    nterms=(ssize_t)coeff[1];

  PointInfo
    du,dv; /* the du,dv vectors from unit dx,dy -- derivatives */

  s.x=s.y=du.x=du.y=dv.x=dv.y=0.0;
  for(k=0; k < nterms; k++) {
    s.x  += poly_basis_fn(k,d.x,d.y)*coeff[2+k];
    du.x += poly_basis_dx(k,d.x,d.y)*coeff[2+k];
    du.y += poly_basis_dy(k,d.x,d.y)*coeff[2+k];
    s.y  += poly_basis_fn(k,d.x,d.y)*coeff[2+k+nterms];
    dv.x += poly_basis_dx(k,d.x,d.y)*coeff[2+k+nterms];
    dv.y += poly_basis_dy(k,d.x,d.y)*coeff[2+k+nterms];
  }
  ScaleFilter( resample_filter[id], du.x,du.y,dv.x,dv.y );
  return(s);
}

static void MapPlane2CylinderPoint(const DistortMapping *mapping,PointInfo *d,
  double *validity,PointInfo *s)
{
  double
    *const coeff = mapping->coeff;

  const double
    output_scaling = mapping->output_scaling;

  ResampleFilter
    **const resample_filter = mapping->resample_filter;

  const int
    id = mapping->id;

  /* relative to center of distortion */
  (*d).x -= coeff[4]; (*d).y -= coeff[5];

  /* is pixel valid - horizon of a infinite Virtual-Pixel Plane
   * (see Anthony Thyssen's personal note) */
  (*validity) = (double) (coeff[1]*MagickPI2 - fabs((*d).x))/output_scaling +
    0.5;

  if ( (*validity) > 0.0 ) {
    double cx,tx;
    (*d).x /= coeff[1];           /* x'= x/r */
    cx = 1/cos((*d).x);           /* cx = 1/cos(x/r) */
    tx = tan((*d).x);             /* tx = tan(x/r) */
    (*s).x = coeff[1]*tx;         /* u = r * tan(x/r) */
    (*s).y = (*d).y*cx;              /* v = y / cos(x/r) */
    /* derivatives...  (see Anthony Thyssen's personal notes) */
    ScaleFilter( resample_filter[id],
          cx*cx, 0.0, (*s).y*cx/coeff[1], cx );
#if 0
/*if ( i == 0 && j == 0 )*/
if ( d.x == 0.5 && d.y == 0.5 ) {
  fprintf(stderr, "x=%lf  y=%lf  u=%lf  v=%lf\n", d.x*coeff[1], d.y, s.x, s.y);
  fprintf(stderr, "radius = %lf  phi = %lf  validity = %lf\n",
      coeff[1],  (double)(d.x * 180.0/MagickPI), validity );
  fprintf(stderr, "du/dx=%lf  du/dx=%lf  dv/dx=%lf  dv/dy=%lf\n",
      cx*cx, 0.0, s.y*cx/coeff[1], cx);
  fflush(stderr); }
#endif
  }
  /* add center of distortion in source */
  (*s).x += coeff[2]; (*s).y += coeff[3];
}

static PointInfo MapCylinder2PlanePoint(const DistortMapping *mapping)
{
  double
    *const coeff = mapping->coeff;

  const double
    output_scaling = mapping->output_scaling;

  ResampleFilter
    **const resample_filter = mapping->resample_filter;

  const int
    id = mapping->id;

  PointInfo
    d = mapping->d;

  PointInfo
    s;

  double ax, cx;
  /* relative to center of distortion */
  d.x -= coeff[4]; d.y -= coeff[5];
  d.x /= coeff[1];        /* x' = x/r */
  ax=atan(d.x);           /* aa = atan(x/r) = u/r  */
  cx=cos(ax);             /* cx = cos(atan(x/r)) = 1/sqrt(x^2+u^2) */
  s.x = coeff[1]*ax;      /* u  = r*atan(x/r) */
  s.y = d.y*cx;           /* v  = y*cos(u/r) */
  /* derivatives... (see personal notes) */
  ScaleFilter( resample_filter[id],
        1.0/(1.0+d.x*d.x), 0.0, -d.x*s.y*cx*cx/coeff[1], s.y/d.y );
#if 0
if ( i == 0 && j == 0 ) {
  fprintf(stderr, "x=%lf  y=%lf  u=%lf  v=%lf\n", d.x*coeff[1], d.y, s.x, s.y);
  fprintf(stderr, "phi = %lf\n", (double)(ax * 180.0/MagickPI) );
  fprintf(stderr, "du/dx=%lf  du/dx=%lf  dv/dx=%lf  dv/dy=%lf\n",
                1.0/(1.0+d.x*d.x), 0.0, -d.x*s.y*cx*cx/coeff[1], s.y/d.y );
  fflush(stderr); }
#endif
  /* add center of distortion in source */
  s.x += coeff[2]; s.y += coeff[3];
  return(s);
}

static PointInfo MapBarrelPoint(const DistortMapping *mapping,const PointInfo d,
  const double r)
{
  double
    *const coeff = mapping->coeff;

  const double
    output_scaling = mapping->output_scaling;

  ResampleFilter
    **const resample_filter = mapping->resample_filter;

  const int
    id = mapping->id;

  const DistortMethod
    method = mapping->method;

  double
    fx,fy,gx,gy;

  PointInfo
    s;

  fx = ((coeff[0]*r + coeff[1])*r + coeff[2])*r + coeff[3];
  fy = ((coeff[4]*r + coeff[5])*r + coeff[6])*r + coeff[7];
  gx = ((3*coeff[0]*r + 2*coeff[1])*r + coeff[2])/r;
  gy = ((3*coeff[4]*r + 2*coeff[5])*r + coeff[6])/r;
  /* adjust functions and scaling for 'inverse' form */
  if ( method == BarrelInverseDistortion ) {
    fx = 1/fx;  fy = 1/fy;
    gx *= -fx*fx;  gy *= -fy*fy;
  }
  /* Set the source pixel to lookup and EWA derivative vectors */
  s.x = d.x*fx + coeff[8];
  s.y = d.y*fy + coeff[9];
  ScaleFilter( resample_filter[id],
      gx*d.x*d.x + fx, gx*d.x*d.y,
      gy*d.x*d.y,      gy*d.y*d.y + fy );
  return(s);
}

static void ScaleBarrelOrigin(const DistortMapping *mapping)
{
  double
    *const coeff = mapping->coeff;

  const double
    output_scaling = mapping->output_scaling;

  ResampleFilter
    **const resample_filter = mapping->resample_filter;

  const int
    id = mapping->id;

  const DistortMethod
    method = mapping->method;

  if ( method == BarrelDistortion )
    ScaleFilter( resample_filter[id],
         coeff[3], 0, 0, coeff[7] );
  else /* method == BarrelInverseDistortion */
    /* FUTURE, trap for D==0 causing division by zero */
    ScaleFilter( resample_filter[id],
         1.0/coeff[3], 0, 0, 1.0/coeff[7] );
}

static PointInfo MapArcPoint(const DistortMapping *mapping)
{
  const PointInfo
    d = mapping->d;

  double
    *const coeff = mapping->coeff;

  const double
    output_scaling = mapping->output_scaling;

  ResampleFilter
    **const resample_filter = mapping->resample_filter;

  const int
    id = mapping->id;

  const Image
    *const image = mapping->image;

  Image
    *const distort_image = mapping->distort_image;

  PointInfo
    s;

  /* what is the angle and radius in the destination image */
  s.x  = (double) ((atan2(d.y,d.x) - coeff[0])/Magick2PI);
  s.x -= MagickRound(s.x);     /* angle */
  s.y  = hypot(d.x,d.y);       /* radius */

  /* Arc Distortion Partial Scaling Vectors
    Are derived by mapping the perpendicular unit vectors
    dR  and  dA*R*2PI  rather than trying to map dx and dy
    The results is a very simple orthogonal aligned ellipse.
  */
  if ( s.y > MagickEpsilon )
    ScaleFilter( resample_filter[id],
        (double) (coeff[1]/(Magick2PI*s.y)), 0, 0, coeff[3] );
  else
    ScaleFilter( resample_filter[id],
        distort_image->columns*2, 0, 0, coeff[3] );

  /* now scale the angle and radius for source image lookup point */
  s.x = s.x*coeff[1] + coeff[4] + image->page.x +0.5;
  s.y = (coeff[2] - s.y) * coeff[3] + image->page.y;
  return(s);
}

static PointInfo MapPolarPoint(const DistortMapping *mapping)
{
  double
    *const coeff = mapping->coeff;

  const double
    output_scaling = mapping->output_scaling;

  ResampleFilter
    **const resample_filter = mapping->resample_filter;

  const int
    id = mapping->id;

  const Image
    *const image = mapping->image;

  Image
    *const distort_image = mapping->distort_image;

  PointInfo
    d = mapping->d;

  PointInfo
    s;

  d.x -= coeff[2];
  d.y -= coeff[3];
  s.x  = atan2(d.x,d.y) - (coeff[4]+coeff[5])/2;
  s.x /= Magick2PI;
  s.x -= MagickRound(s.x);
  s.x *= Magick2PI;       /* angle - relative to centerline */
  s.y  = hypot(d.x,d.y);  /* radius */

  /* Polar Scaling vectors are based on mapping dR and dA vectors
     This results in very simple orthogonal scaling vectors
  */
  if ( s.y > MagickEpsilon )
    ScaleFilter( resample_filter[id],
      (double) (coeff[6]/(Magick2PI*s.y)), 0, 0, coeff[7] );
  else
    ScaleFilter( resample_filter[id],
        distort_image->columns*2, 0, 0, coeff[7] );

  /* now finish mapping radius/angle to source x,y coords */
  s.x = s.x*coeff[6] + (double)image->columns/2.0 + image->page.x;
  s.y = (s.y-coeff[1])*coeff[7] + image->page.y;
  return(s);
}

static void MapBarrelDistortion(const DistortMapping *mapping,PointInfo *d,
  PointInfo *s)
{
  double
    *const coeff = mapping->coeff;

  double
    r;

  /* Radial Polynomial Distortion (de-normalized) */
  (*d).x -= coeff[8];
  (*d).y -= coeff[9];
  r = sqrt((*d).x*(*d).x+(*d).y*(*d).y);
  if ( r > MagickEpsilon ) {
    (*s)=MapBarrelPoint(mapping,(*d),r);
  }
  else {
    /* Special handling to avoid divide by zero when r==0
    **
    ** The source and destination pixels match in this case
    ** which was set at the top of the loop using  s = d;
    ** otherwise...   s.x=coeff[8]; s.y=coeff[9];
    */
    ScaleBarrelOrigin(mapping);
  }
}

static void PrintAffineProjection(const double *inverse)
{
  ssize_t
    i;

  (void) FormatLocaleFile(stderr, "Affine projection:\n");
  (void) FormatLocaleFile(stderr,
    "  -distort AffineProjection \\\n    '");
  for (i=0; i < 5; i++)
    (void) FormatLocaleFile(stderr, "%.*g,",GetMagickPrecision(),
      inverse[i]);
  (void) FormatLocaleFile(stderr, "%.*g'\n",GetMagickPrecision(),
    inverse[5]);
  (void) FormatLocaleFile(stderr,
    "Equivalent scale, rotation(deg), translation:\n");
  (void) FormatLocaleFile(stderr,"  %.*g,%.*g,%.*g,%.*g\n",
    GetMagickPrecision(),sqrt(inverse[0]*inverse[0]+
    inverse[1]*inverse[1]),GetMagickPrecision(),
    RadiansToDegrees(atan2(inverse[1],inverse[0])),
    GetMagickPrecision(),inverse[4],GetMagickPrecision(),inverse[5]);
}

static MagickBooleanType PrintAffineFX(double *coeff,const char *image_gen,
  const char *lookup,ExceptionInfo *exception)
{
  double
    *inverse;

  inverse=(double *) AcquireQuantumMemory(6,sizeof(*inverse));
  if (inverse == (double *) NULL)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","%s","DistortImages");
      return(MagickFalse);
    }
  InvertAffineCoefficients(coeff, inverse);
  CoefficientsToAffineArgs(inverse);
  PrintAffineProjection(inverse);
  inverse=(double *) RelinquishMagickMemory(inverse);
  (void) FormatLocaleFile(stderr,"Affine distort, FX equivalent:\n");
  (void) FormatLocaleFile(stderr, "%s", image_gen);
  (void) FormatLocaleFile(stderr,
    "  -fx 'ii=i+page.x+0.5; jj=j+page.y+0.5;\n");
  (void) FormatLocaleFile(stderr,"       xx=%+.*g*ii %+.*g*jj %+.*g;\n",
    GetMagickPrecision(),coeff[0],GetMagickPrecision(),coeff[1],
    GetMagickPrecision(),coeff[2]);
  (void) FormatLocaleFile(stderr,"       yy=%+.*g*ii %+.*g*jj %+.*g;\n",
    GetMagickPrecision(),coeff[3],GetMagickPrecision(),coeff[4],
    GetMagickPrecision(),coeff[5]);
  (void) FormatLocaleFile(stderr,"       %s' \\\n",lookup);
  return(MagickTrue);
}

static MagickBooleanType PrintPerspectiveFX(double *coeff,
  const char *image_gen,const char *lookup,ExceptionInfo *exception)
{
  double
    *inverse;

  ssize_t
    i;

  inverse=(double *) AcquireQuantumMemory(8,sizeof(*inverse));
  if (inverse == (double *) NULL)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","%s",
        "DistortCoefficients");
      return(MagickFalse);
    }
  InvertPerspectiveCoefficients(coeff, inverse);
  (void) FormatLocaleFile(stderr,"Perspective Projection:\n");
  (void) FormatLocaleFile(stderr,
    "  -distort PerspectiveProjection \\\n      '");
  for (i=0; i < 4; i++)
    (void) FormatLocaleFile(stderr, "%.*g, ",GetMagickPrecision(),
      inverse[i]);
  (void) FormatLocaleFile(stderr, "\n       ");
  for ( ; i < 7; i++)
    (void) FormatLocaleFile(stderr, "%.*g, ",GetMagickPrecision(),
      inverse[i]);
  (void) FormatLocaleFile(stderr, "%.*g'\n",GetMagickPrecision(),
    inverse[7]);
  inverse=(double *) RelinquishMagickMemory(inverse);
  (void) FormatLocaleFile(stderr,"Perspective Distort, FX Equivalent:\n");
  (void) FormatLocaleFile(stderr,"%.1024s",image_gen);
  (void) FormatLocaleFile(stderr,
    "  -fx 'ii=i+page.x+0.5; jj=j+page.y+0.5;\n");
  (void) FormatLocaleFile(stderr,"       rr=%+.*g*ii %+.*g*jj + 1;\n",
    GetMagickPrecision(),coeff[6],GetMagickPrecision(),coeff[7]);
  (void) FormatLocaleFile(stderr,
    "       xx=(%+.*g*ii %+.*g*jj %+.*g)/rr;\n",
    GetMagickPrecision(),coeff[0],GetMagickPrecision(),coeff[1],
    GetMagickPrecision(),coeff[2]);
  (void) FormatLocaleFile(stderr,
    "       yy=(%+.*g*ii %+.*g*jj %+.*g)/rr;\n",
    GetMagickPrecision(),coeff[3],GetMagickPrecision(),coeff[4],
    GetMagickPrecision(),coeff[5]);
  (void) FormatLocaleFile(stderr,"       rr%s0 ? %s : blue' \\\n",
    coeff[8] < 0.0 ? "<" : ">", lookup);
  return(MagickTrue);
}

static void PrintBilinearForwardYTerm(const double *coeff)
{
  /* Handle Special degenerate (non-quadratic) or trapezoidal case */
  if (coeff[9] != 0)
    {
      (void) FormatLocaleFile(stderr,
        "       rt=bb*bb %+lf*(%lf*ii%+lf*jj);\n",-2*coeff[9],coeff[4],
        -coeff[0]);
    (void) FormatLocaleFile(stderr,
      "       yy=( -bb + sqrt(rt) ) / %lf;\n",coeff[9]);
    }
  else
    (void) FormatLocaleFile(stderr,"       yy=(%lf*ii%+lf*jj)/bb;\n",
      -coeff[4],coeff[0]);
}

static void PrintBilinearForwardLookup(const double *coeff,
  const char *lookup)
{
  if ( coeff[9] != 0 )
    (void) FormatLocaleFile(stderr,"       (rt < 0 ) ? red : %s'\n",
      lookup);
  else
    (void) FormatLocaleFile(stderr,"       %s' \\\n", lookup);
}

static void PrintBilinearForwardFX(const double *coeff,const char *image_gen,
  const char *lookup)
{
  (void) FormatLocaleFile(stderr,"BilinearForward Mapping Equations:\n");
  (void) FormatLocaleFile(stderr,"%s", image_gen);
  (void) FormatLocaleFile(stderr,"    i = %+lf*x %+lf*y %+lf*x*y %+lf;\n",
    coeff[0],coeff[1],coeff[2],coeff[3]);
  (void) FormatLocaleFile(stderr,"    j = %+lf*x %+lf*y %+lf*x*y %+lf;\n",
    coeff[4],coeff[5],coeff[6],coeff[7]);
#if 0
        /* for debugging */
        (void) FormatLocaleFile(stderr, "   c8 = %+lf  c9 = 2*a = %+lf;\n",
            coeff[8], coeff[9]);
#endif
  (void) FormatLocaleFile(stderr,
    "BilinearForward Distort, FX Equivalent:\n");
  (void) FormatLocaleFile(stderr,"%s", image_gen);
  (void) FormatLocaleFile(stderr,
    "  -fx 'ii=i+page.x%+lf; jj=j+page.y%+lf;\n",0.5-coeff[3],0.5-
    coeff[7]);
  (void) FormatLocaleFile(stderr,"       bb=%lf*ii %+lf*jj %+lf;\n",
    coeff[6], -coeff[2], coeff[8]);
  PrintBilinearForwardYTerm(coeff);
  (void) FormatLocaleFile(stderr,
    "       xx=(ii %+lf*yy)/(%lf %+lf*yy);\n",-coeff[1],coeff[0],
    coeff[2]);
  PrintBilinearForwardLookup(coeff,lookup);
}

static void PrintBilinearReverseFX(const double *coeff,const char *image_gen,
  const char *lookup)
{
#if 0
        (void) FormatLocaleFile(stderr, "Polynomial Projection Distort:\n");
        (void) FormatLocaleFile(stderr, "  -distort PolynomialProjection \\\n");
        (void) FormatLocaleFile(stderr, "      '1.5, %lf, %lf, %lf, %lf,\n",
            coeff[3], coeff[0], coeff[1], coeff[2]);
        (void) FormatLocaleFile(stderr, "            %lf, %lf, %lf, %lf'\n",
            coeff[7], coeff[4], coeff[5], coeff[6]);
#endif
  (void) FormatLocaleFile(stderr,
    "BilinearReverse Distort, FX Equivalent:\n");
  (void) FormatLocaleFile(stderr,"%s", image_gen);
  (void) FormatLocaleFile(stderr,
    "  -fx 'ii=i+page.x+0.5; jj=j+page.y+0.5;\n");
  (void) FormatLocaleFile(stderr,
    "       xx=%+lf*ii %+lf*jj %+lf*ii*jj %+lf;\n",coeff[0],coeff[1],
    coeff[2], coeff[3]);
  (void) FormatLocaleFile(stderr,
     "       yy=%+lf*ii %+lf*jj %+lf*ii*jj %+lf;\n",coeff[4],coeff[5],
     coeff[6], coeff[7]);
  (void) FormatLocaleFile(stderr,"       %s' \\\n", lookup);
}

typedef struct _DistortVerboseFX
{
  DistortMethod
    method;

  const Image
    *image;

  double
    *coeff;

  char
    *image_gen;

  const char
    *lookup;
} DistortVerboseFX;

static void PrintLensDistortFX(const DistortVerboseFX *fx)
{
  const DistortMethod
    method = fx->method;

  const Image
    *const image = fx->image;

  double
    *const coeff = fx->coeff;

  char
    *const image_gen = fx->image_gen;

  const char
    *const lookup = fx->lookup;

  switch (method)
  {
    case Plane2CylinderDistortion:
    {
      (void) FormatLocaleFile(stderr,
        "Plane to Cylinder Distort, Internal Coefficients\n");
      (void) FormatLocaleFile(stderr,"  cylinder_radius = %+lf\n",coeff[1]);
      (void) FormatLocaleFile(stderr,
        "Plane to Cylinder Distort, FX Equivalent:\n");
      (void) FormatLocaleFile(stderr,"%s", image_gen);
      (void) FormatLocaleFile(stderr,
        "  -fx 'ii=i+page.x%+lf+0.5; jj=j+page.y%+lf+0.5;\n",-coeff[4],
        -coeff[5]);
      (void) FormatLocaleFile(stderr,"       ii=ii/%+lf;\n",coeff[1]);
      (void) FormatLocaleFile(stderr,"       xx=%lf*tan(ii)%+lf;\n",coeff[1],
        coeff[2] );
      (void) FormatLocaleFile(stderr,"       yy=jj/cos(ii)%+lf;\n",coeff[3]);
      (void) FormatLocaleFile(stderr,"       %s' \\\n", lookup);
      break;
    }
    case BarrelDistortion:
    case BarrelInverseDistortion:
    {
      PrintBarrelFX(image,coeff,method,image_gen);
      break;
    }
    default:
      break;
  }
}

static void PrintRadialDistortFX(const DistortVerboseFX *fx)
{
  const DistortMethod
    method = fx->method;

  double
    *const coeff = fx->coeff;

  char
    *const image_gen = fx->image_gen;

  const char
    *const lookup = fx->lookup;

  ssize_t
    i;

  switch (method)
  {
    case ArcDistortion:
    {
      PrintArcFX(coeff,image_gen);
      break;
    }
    case PolarDistortion:
    {
      (void) FormatLocaleFile(stderr,"Polar Distort, Internal Coefficients\n");
      for (i=0; i < 8; i++)
        (void) FormatLocaleFile(stderr,"  c%.17g = %+lf\n",(double) i,
          coeff[i]);
      (void) FormatLocaleFile(stderr,"Polar Distort, FX Equivalent:\n");
      (void) FormatLocaleFile(stderr,"%s", image_gen);
      (void) FormatLocaleFile(stderr,
        "  -fx 'ii=i+page.x%+lf; jj=j+page.y%+lf;\n",-coeff[2],-coeff[3]);
      (void) FormatLocaleFile(stderr,"       xx=(atan2(ii,jj)%+lf)/(2*pi);\n",
        -(coeff[4]+coeff[5])/2 );
      (void) FormatLocaleFile(stderr,"       xx=xx-round(xx);\n");
      (void) FormatLocaleFile(stderr,"       xx=xx*2*pi*%lf + v.w/2;\n",
        coeff[6] );
      (void) FormatLocaleFile(stderr,"       yy=(hypot(ii,jj)%+lf)*%lf;\n",
        -coeff[1],coeff[7] );
      (void) FormatLocaleFile(stderr,"       v.p{xx-.5,yy-.5}' \\\n");
      break;
    }
    case DePolarDistortion:
    {
      PrintDePolarFX(coeff,image_gen);
      break;
    }
    case Cylinder2PlaneDistortion:
    {
      PrintCylinder2PlaneFX(coeff,image_gen,lookup);
      break;
    }
    default:
    {
      PrintLensDistortFX(fx);
      break;
    }
  }
}

static void PrintDistortFX(const DistortVerboseFX *fx)
{
  const DistortMethod
    method = fx->method;

  double
    *const coeff = fx->coeff;

  char
    *const image_gen = fx->image_gen;

  const char
    *const lookup = fx->lookup;

  switch (method)
  {
    case BilinearForwardDistortion:
    {
      PrintBilinearForwardFX(coeff,image_gen,lookup);
      break;
    }
    case BilinearReverseDistortion:
    {
      PrintBilinearReverseFX(coeff,image_gen,lookup);
      break;
    }
    case PolynomialDistortion:
    {
      PrintPolynomialFX(coeff,image_gen,lookup);
      break;
    }
    default:
    {
      PrintRadialDistortFX(fx);
      break;
    }
  }
}

typedef struct _VerboseDestination
{
  MagickBooleanType
    bestfit,
    viewport_given;

  RectangleInfo
    geometry;
} VerboseDestination;

static void SetVerboseDestination(const VerboseDestination *destination,
  char (*image_gen)[MagickPathExtent],const char **lookup)
{
  const MagickBooleanType
    bestfit = destination->bestfit,
    viewport_given = destination->viewport_given;

  const RectangleInfo
    geometry = destination->geometry;

  if ( bestfit || viewport_given ) {
    (void) FormatLocaleString((*image_gen),MagickPathExtent,
      "  -size %.17gx%.17g -page %+.20g%+.20g xc: +insert \\\n",
      (double) geometry.width,(double) geometry.height,(double) geometry.x,
      (double) geometry.y);
    (*lookup)="v.p{xx-v.page.x-0.5,yy-v.page.y-0.5}";
  }
  else {
    (*image_gen)[0] = '\0';             /* no destination to generate */
    (*lookup) = "p{xx-page.x-0.5,yy-page.y-0.5}"; /* simplify lookup */
  }
}

typedef struct _DistortVerboseInput
{
  const Image
    *image;

  DistortMethod
    method;

  double
    *coeff;

  MagickBooleanType
    bestfit,
    viewport_given;

  RectangleInfo
    geometry;
} DistortVerboseInput;

static MagickBooleanType PrintDistortVerbose(
  const DistortVerboseInput *verbose_input,ExceptionInfo *exception)
{
  const Image
    *const image = verbose_input->image;

  const DistortMethod
    method = verbose_input->method;

  double
    *const coeff = verbose_input->coeff;

  const MagickBooleanType
    bestfit = verbose_input->bestfit,
    viewport_given = verbose_input->viewport_given;

  const RectangleInfo
    geometry = verbose_input->geometry;

  if (IsStringTrue(GetImageArtifact(image,"verbose")) != MagickFalse) {
    char image_gen[MagickPathExtent];
    const char *lookup;
    DistortVerboseFX
      fx;

    VerboseDestination
      destination;

    /* Set destination image size and virtual offset */
    destination.bestfit=bestfit;
    destination.viewport_given=viewport_given;
    destination.geometry=geometry;
    SetVerboseDestination(&destination,&image_gen,&lookup);

    fx.method=method;
    fx.image=image;
    fx.coeff=coeff;
    fx.image_gen=image_gen;
    fx.lookup=lookup;
    switch (method)
    {
      case AffineDistortion:
      case RigidAffineDistortion:
      {
        if (PrintAffineFX(coeff,image_gen,lookup,exception) == MagickFalse)
          return(MagickFalse);
        break;
      }
      case PerspectiveDistortion:
      {
        if (PrintPerspectiveFX(coeff,image_gen,lookup,exception) ==
            MagickFalse)
          return(MagickFalse);
        break;
      }
      default:
      {
        PrintDistortFX(&fx);
        break;
      }
    }
  }
  return(MagickTrue);
}

/* defines to figure out the bounds of the distorted image */
#define InitalBounds(p) \
{ \
  /* printf("%lg,%lg -> %lg,%lg\n", s.x,s.y, d.x,d.y); */ \
  min.x = max.x = p.x; \
  min.y = max.y = p.y; \
}
#define ExpandBounds(p) \
{ \
  /* printf("%lg,%lg -> %lg,%lg\n", s.x,s.y, d.x,d.y); */ \
  min.x = MagickMin(min.x,p.x); \
  max.x = MagickMax(max.x,p.x); \
  min.y = MagickMin(min.y,p.y); \
  max.y = MagickMax(max.y,p.y); \
}

static void AffineBestFitBounds(const double *coeff,const Image *image,
  PointInfo *caller_min,PointInfo *caller_max)
{
  double
    inverse[6];

  PointInfo
    d,
    max = (*caller_max),
    min = (*caller_min),
    s;

  InvertAffineCoefficients(coeff, inverse);
  s.x = (double) image->page.x;
  s.y = (double) image->page.y;
  d.x = inverse[0]*s.x+inverse[1]*s.y+inverse[2];
  d.y = inverse[3]*s.x+inverse[4]*s.y+inverse[5];
  InitalBounds(d);
  s.x = (double) image->page.x+image->columns;
  s.y = (double) image->page.y;
  d.x = inverse[0]*s.x+inverse[1]*s.y+inverse[2];
  d.y = inverse[3]*s.x+inverse[4]*s.y+inverse[5];
  ExpandBounds(d);
  s.x = (double) image->page.x;
  s.y = (double) image->page.y+image->rows;
  d.x = inverse[0]*s.x+inverse[1]*s.y+inverse[2];
  d.y = inverse[3]*s.x+inverse[4]*s.y+inverse[5];
  ExpandBounds(d);
  s.x = (double) image->page.x+image->columns;
  s.y = (double) image->page.y+image->rows;
  d.x = inverse[0]*s.x+inverse[1]*s.y+inverse[2];
  d.y = inverse[3]*s.x+inverse[4]*s.y+inverse[5];
  ExpandBounds(d);
  (*caller_min)=min;
  (*caller_max)=max;
}

static void PerspectiveBestFitBounds(const double *coeff,const Image *image,
  PointInfo *caller_min,PointInfo *caller_max)
{
  double
    inverse[8],
    scale;

  PointInfo
    d,
    max = (*caller_max),
    min = (*caller_min),
    s;

  InvertPerspectiveCoefficients(coeff, inverse);
  s.x = (double) image->page.x;
  s.y = (double) image->page.y;
  scale=inverse[6]*s.x+inverse[7]*s.y+1.0;
  scale=MagickSafeReciprocal(scale);
  d.x = scale*(inverse[0]*s.x+inverse[1]*s.y+inverse[2]);
  d.y = scale*(inverse[3]*s.x+inverse[4]*s.y+inverse[5]);
  InitalBounds(d);
  s.x = (double) image->page.x+image->columns;
  s.y = (double) image->page.y;
  scale=inverse[6]*s.x+inverse[7]*s.y+1.0;
  scale=MagickSafeReciprocal(scale);
  d.x = scale*(inverse[0]*s.x+inverse[1]*s.y+inverse[2]);
  d.y = scale*(inverse[3]*s.x+inverse[4]*s.y+inverse[5]);
  ExpandBounds(d);
  s.x = (double) image->page.x;
  s.y = (double) image->page.y+image->rows;
  scale=inverse[6]*s.x+inverse[7]*s.y+1.0;
  scale=MagickSafeReciprocal(scale);
  d.x = scale*(inverse[0]*s.x+inverse[1]*s.y+inverse[2]);
  d.y = scale*(inverse[3]*s.x+inverse[4]*s.y+inverse[5]);
  ExpandBounds(d);
  s.x = (double) image->page.x+image->columns;
  s.y = (double) image->page.y+image->rows;
  scale=inverse[6]*s.x+inverse[7]*s.y+1.0;
  scale=MagickSafeReciprocal(scale);
  d.x = scale*(inverse[0]*s.x+inverse[1]*s.y+inverse[2]);
  d.y = scale*(inverse[3]*s.x+inverse[4]*s.y+inverse[5]);
  ExpandBounds(d);
  (*caller_min)=min;
  (*caller_max)=max;
}

static void ScaleArcCoefficients(double *coeff,const Image *image)
{
  /*
    Convert the angle_to_width and radius_to_height
    to appropriate scaling factors, to allow faster processing
    in the mapping function.
  */
  coeff[1] = (double) (Magick2PI*image->columns/coeff[1]);
  coeff[3] = (double)image->rows/coeff[3];
}

static void ArcBestFitBounds(double *coeff,const Image *image,
  PointInfo *caller_min,PointInfo *caller_max)
{
  double
    a,
    ca,
    sa;

  PointInfo
    d,
    max = (*caller_max),
    min = (*caller_min);

  /* Forward Map Corners */
  a = coeff[0]-coeff[1]/2; ca = cos(a); sa = sin(a);
  d.x = coeff[2]*ca;
  d.y = coeff[2]*sa;
  InitalBounds(d);
  d.x = (coeff[2]-coeff[3])*ca;
  d.y = (coeff[2]-coeff[3])*sa;
  ExpandBounds(d);
  a = coeff[0]+coeff[1]/2; ca = cos(a); sa = sin(a);
  d.x = coeff[2]*ca;
  d.y = coeff[2]*sa;
  ExpandBounds(d);
  d.x = (coeff[2]-coeff[3])*ca;
  d.y = (coeff[2]-coeff[3])*sa;
  ExpandBounds(d);
  /* Orthogonal points along top of arc */
  for( a=(double) (ceil((double) ((coeff[0]-coeff[1]/2.0)/MagickPI2))*MagickPI2);
         a<(coeff[0]+coeff[1]/2.0); a+=MagickPI2 ) {
    ca = cos(a); sa = sin(a);
    d.x = coeff[2]*ca;
    d.y = coeff[2]*sa;
    ExpandBounds(d);
  }
  ScaleArcCoefficients(coeff,image);
  (*caller_min)=min;
  (*caller_max)=max;
}

static Image *DistortResizeFromArguments(const Image *image,
  const size_t number_arguments,const double *arguments,
  ExceptionInfo *exception)
{
  Image
    *distort_image;

  if ( number_arguments != 2 )
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                "InvalidArgument","%s : '%s'","Resize",
                "Invalid number of args: 2 only");
      return((Image *) NULL);
    }
  distort_image=DistortResizeImage(image,CastDoubleToSizeT(arguments[0]),
    CastDoubleToSizeT(arguments[1]),exception);
  return(distort_image);
}

typedef struct _DistortScaleTargets
{
  double
    *output_scaling;

  RectangleInfo
    *geometry;
} DistortScaleTargets;

static void ScaleDistortRectangle(RectangleInfo *geometry,
  const double *output_scaling)
{
  (*geometry).width=CastDoubleToSizeT((*output_scaling)*(*geometry).width+0.5);
  (*geometry).height=CastDoubleToSizeT((*output_scaling)*(*geometry).height+0.5);
  (*geometry).x=(ssize_t) ((*output_scaling)*(*geometry).x+0.5);
  (*geometry).y=(ssize_t) ((*output_scaling)*(*geometry).y+0.5);
}

static MagickBooleanType ScaleDistortGeometry(const Image *image,
  const DistortScaleTargets *targets)
{
  double
    *const output_scaling = targets->output_scaling;

  RectangleInfo
    *const geometry = targets->geometry;

  { const char *artifact;
    artifact=GetImageArtifact(image,"distort:scale");
    (*output_scaling) = 1.0;
    if (artifact != (const char *) NULL) {
      (*output_scaling) = fabs(StringToDouble(artifact,(char **) NULL));
      ScaleDistortRectangle(geometry,output_scaling);
      if ( (*output_scaling) < 0.1 ) {
        return(MagickFalse);
      }
      (*output_scaling) = 1/(*output_scaling);
    }
  }
  return(MagickTrue);
}

typedef struct _DistortPointInput
{
  DistortMethod
    method;

  double
    *coeff,
    output_scaling;

  DistortMapping
    mapping;

  ResampleFilter
    **resample_filter;

  int
    id;

  ssize_t
    i,
    j;

  size_t
    number_arguments;

  const double
    *arguments;
} DistortPointInput;

static void MapDistortLensPoint(const DistortPointInput *tile,PointInfo *d,
  PointInfo *s)
{
  const DistortMethod
    method = tile->method;

  double
    *const coeff = tile->coeff;

  const DistortMapping
    mapping = tile->mapping;

  const size_t
    number_arguments = tile->number_arguments;

  const double
    *const arguments = tile->arguments;

        switch (method)
        {
          case BarrelDistortion:
          case BarrelInverseDistortion:
          { /* Lens Barrel Distortion Correction */
            MapBarrelDistortion(&mapping,&(*d),&(*s));
            break;
          }
          case ShepardsDistortion:
          { /* Shepards Method, or Inverse Weighted Distance for
               displacement around the destination image control points
               The input arguments are the coefficients to the function.
               This is more of a 'displacement' function rather than an
               absolute distortion function.

               Note: We can not determine derivatives using shepards method
               so only a point sample interpolation can be used.
            */
            (*s)=ShepardsSourcePoint((*d),number_arguments,arguments,coeff);
            break;
          }
          default:
            break; /* use the default no-op given above */
        }
}

static void MapDistortCurvedPoint(const DistortPointInput *tile,PointInfo *s,
  PointInfo *d,double *validity)
{
  const DistortMethod
    method = tile->method;

  double
    *const coeff = tile->coeff;

  const double
    output_scaling = tile->output_scaling;

  const DistortMapping
    mapping = tile->mapping;

  const ssize_t
    i = tile->i,
    j = tile->j;

        switch (method)
        {
          case PolynomialDistortion:
          {
            /* multi-ordered polynomial */
            (*s)=MapPolynomialPoint(&mapping);
            break;
          }
          case ArcDistortion:
          {
            (*s)=MapArcPoint(&mapping);
            break;
          }
          case PolarDistortion:
          { /* 2D Cartesian to Polar View */
            (*s)=MapPolarPoint(&mapping);
            break;
          }
          case DePolarDistortion:
          { /* @D Polar to Cartesian  */
            /* ignore all destination virtual offsets */
            (*d).x = ((double)i+0.5)*output_scaling*coeff[6]+coeff[4];
            (*d).y = ((double)j+0.5)*output_scaling*coeff[7]+coeff[1];
            (*s).x = (*d).y*sin((*d).x) + coeff[2];
            (*s).y = (*d).y*cos((*d).x) + coeff[3];
            /* derivatives are useless - better to use SuperSampling */
            break;
          }
          case Cylinder2PlaneDistortion:
          { /* 3D Cylinder to Tangential Plane */
            (*s)=MapCylinder2PlanePoint(&mapping);
            break;
          }
          case Plane2CylinderDistortion:
          { /* 3D Cylinder to Tangential Plane */
            MapPlane2CylinderPoint(&mapping,&(*d),&(*validity),&(*s));
            break;
          }
          default:
          {
            MapDistortLensPoint(tile,&(*d),&(*s));
            break;
          }
        }
}

static void MapDistortPoint(const DistortPointInput *tile,PointInfo *s,
  double *validity,PointInfo *d)
{
  const DistortMethod
    method = tile->method;

  double
    *const coeff = tile->coeff;

  const double
    output_scaling = tile->output_scaling;

  const DistortMapping
    mapping = tile->mapping;

  ResampleFilter
    **const resample_filter = tile->resample_filter;

  const int
    id = tile->id;

        switch (method)
        {
          case AffineDistortion:
          case RigidAffineDistortion:
          {
            (*s).x=coeff[0]*(*d).x+coeff[1]*(*d).y+coeff[2];
            (*s).y=coeff[3]*(*d).x+coeff[4]*(*d).y+coeff[5];
            /* Affine partial derivatives are constant -- set above */
            break;
          }
          case PerspectiveDistortion:
          {
            MapPerspectivePoint(&mapping,&(*validity),&(*s));
            break;
          }
          case BilinearReverseDistortion:
          {
            /* Reversed Mapped is just a simple polynomial */
            (*s).x=coeff[0]*(*d).x+coeff[1]*(*d).y+coeff[2]*(*d).x*(*d).y+coeff[3];
            (*s).y=coeff[4]*(*d).x+coeff[5]*(*d).y
                    +coeff[6]*(*d).x*(*d).y+coeff[7];
            /* Bilinear partial derivatives of scaling vectors */
            ScaleFilter( resample_filter[id],
                coeff[0] + coeff[2]*(*d).y,
                coeff[1] + coeff[2]*(*d).x,
                coeff[4] + coeff[6]*(*d).y,
                coeff[5] + coeff[6]*(*d).x );
            break;
          }
          case BilinearForwardDistortion:
          {
            /* Forward mapped needs reversed polynomial equations
             * which unfortunately requires a square root!  */
            MapBilinearForwardPoint(coeff,&(*d),&(*s),&(*validity));
            break;
          }
#if 0
          case BilinearDistortion:
            /* Bilinear mapping of any Quadrilateral to any Quadrilateral */
            /* UNDER DEVELOPMENT */
            break;
#endif
          default:
          {
            MapDistortCurvedPoint(tile,&(*s),&(*d),&(*validity));
            break;
          }
        }
}

static void SetDistortNoBestFit(const DistortMethod method,
  MagickBooleanType *bestfit,MagickBooleanType *fix_bounds)
{
    switch (method)
    {
      case ShepardsDistortion:
      case BilinearForwardDistortion:
      case BilinearReverseDistortion:
#if 0
      case QuadrilateralDistortion:
#endif
      default:
        /* no calculated bestfit available for these distortions */
        (*bestfit) = MagickFalse;
        (*fix_bounds) = MagickFalse;
        break;
    }
}

typedef struct _DistortBestFitTile
{
  DistortMethod
    method;

  size_t
    number_arguments;

  double
    *coeff;
} DistortBestFitTile;

static void SetCylinder2PlaneBestFit(double *coeff,RectangleInfo *geometry,
  MagickBooleanType *fix_bounds)
{
  /* direct calculation so center of distortion is either a pixel
   * center, or pixel edge. This allows for reversibility of the
   * distortion */
  (*geometry).x = (*geometry).y = 0;
  (*geometry).width = CastDoubleToSizeT(ceil( 2.0*coeff[1]*tan(coeff[0]/2.0) ));
  (*geometry).height = CastDoubleToSizeT(ceil( 2.0*coeff[3]/cos(coeff[0]/2.0) ));
  /* correct center of distortion relative to new size */
  coeff[4] = (double) (*geometry).width/2.0;
  coeff[5] = (double) (*geometry).height/2.0;
  (*fix_bounds) = MagickFalse;
}

static void SetDistortCylinderBestFit(const DistortBestFitTile *tile,
  RectangleInfo *geometry,MagickBooleanType *fix_bounds,
  MagickBooleanType *bestfit)
{
  const DistortMethod
    method = tile->method;

  double
    *const coeff = tile->coeff;

    switch (method)
    {
      case Cylinder2PlaneDistortion:
      {
        SetCylinder2PlaneBestFit(coeff,geometry,fix_bounds);
        break;
      }
      case Plane2CylinderDistortion:
      {
        /* direct calculation center is either pixel center, or pixel edge
         * so as to allow reversibility of the image distortion */
        (*geometry).x = (*geometry).y = 0;
        (*geometry).width = CastDoubleToSizeT(ceil(coeff[0]*coeff[1]));  /* FOV * radius */
        (*geometry).height = CastDoubleToSizeT(2.0*coeff[3]);              /* input image height */
        /* correct center of distortion relative to new size */
        coeff[4] = (double) (*geometry).width/2.0;
        coeff[5] = (double) (*geometry).height/2.0;
        (*fix_bounds) = MagickFalse;
        break;
      }
      default:
      {
        SetDistortNoBestFit(method,&(*bestfit),&(*fix_bounds));
        break;
      }
    }
}

typedef struct _DistortBestFitTargets
{
  PointInfo
    *min,
    *max;

  MagickBooleanType
    *fix_bounds;

  RectangleInfo
    *geometry;

  MagickBooleanType
    *bestfit;
} DistortBestFitTargets;

static void SetDePolarBestFit(double *coeff,RectangleInfo *geometry,
  MagickBooleanType *fix_bounds)
{
  /* direct calculation as it needs to tile correctly
   * for reversibility in a DePolar-Polar cycle */
  (*fix_bounds) = MagickFalse;
  (*geometry).x = (*geometry).y = 0;
  (*geometry).height = CastDoubleToSizeT(ceil(coeff[0]-coeff[1]));
  (*geometry).width = CastDoubleToSizeT(ceil((coeff[0]-coeff[1])*
    (coeff[5]-coeff[4])*0.5));
  /* correct scaling factors relative to new size */
  coeff[6]=(coeff[5]-coeff[4]) * MagickSafeReciprocal(
    (double) (*geometry).width); /* changed width */
  coeff[7]=(coeff[0]-coeff[1]) * MagickSafeReciprocal(
    (double) (*geometry).height); /* should be about 1.0 */
}

static void SetDistortPolarBestFit(const DistortBestFitTile *tile,
  const DistortBestFitTargets *targets)
{
  const DistortMethod
    method = tile->method;

  const size_t
    number_arguments = tile->number_arguments;

  double
    *const coeff = tile->coeff;

  PointInfo
    *const min = targets->min,
    *const max = targets->max;

  MagickBooleanType
    *const fix_bounds = targets->fix_bounds;

  RectangleInfo
    *const geometry = targets->geometry;

  MagickBooleanType
    *const bestfit = targets->bestfit;

    switch (method)
    {
      case PolarDistortion:
      {
        if (number_arguments < 2)
          coeff[2] = coeff[3] = 0.0;
        (*min).x = coeff[2]-coeff[0];
        (*max).x = coeff[2]+coeff[0];
        (*min).y = coeff[3]-coeff[0];
        (*max).y = coeff[3]+coeff[0];
        /* should be about 1.0 if Rmin = 0 */
        coeff[7]=(double) (*geometry).height/(coeff[0]-coeff[1]);
        break;
      }
      case DePolarDistortion:
      {
        SetDePolarBestFit(coeff,geometry,fix_bounds);
        break;
      }
      default:
      {
        SetDistortCylinderBestFit(tile,&(*geometry),&(*fix_bounds),&(*bestfit));
        break;
      }
    }
}

typedef struct _DistortBestFitInput
{
  const Image
    *image;

  DistortMethod
    method;

  size_t
    number_arguments;

  double
    *coeff;
} DistortBestFitInput;

static void SetBestFitGeometry(RectangleInfo *geometry,
  const MagickBooleanType fix_bounds,const PointInfo min,const PointInfo max)
{
  /* Set the output image geometry to calculated 'bestfit'.
     Yes this tends to 'over do' the file image size, ON PURPOSE!
     Do not do this for DePolar which needs to be exact for virtual tiling.
  */
  if ( fix_bounds ) {
    (*geometry).x = CastDoubleToSsizeT(floor(min.x-0.5));
    (*geometry).y = CastDoubleToSsizeT(floor(min.y-0.5));
    (*geometry).width=CastDoubleToSizeT(ceil(max.x-(*geometry).x+0.5));
    (*geometry).height=CastDoubleToSizeT(ceil(max.y-(*geometry).y+0.5));
  }
}

static void SetDistortBestFit(const DistortBestFitInput *fit,
  RectangleInfo *geometry,MagickBooleanType *bestfit)
{
  const Image
    *const image = fit->image;

  const DistortMethod
    method = fit->method;

  const size_t
    number_arguments = fit->number_arguments;

  double
    *const coeff = fit->coeff;

  /* Work out the 'best fit', (required for ArcDistortion) */
  if ( (*bestfit) ) {
    PointInfo
      s,min,max;  /* source, dest coords --mapping--> min, max coords */

    MagickBooleanType
      fix_bounds = MagickTrue;   /* enlarge bounds for VP handling */

    DistortBestFitTile
      tile;

    const DistortBestFitTargets
      targets = { &min, &max, &fix_bounds, &(*geometry), &(*bestfit) };

    s.x=s.y=min.x=max.x=min.y=max.y=0.0;   /* keep compiler happy */

    tile.method=method;
    tile.number_arguments=number_arguments;
    tile.coeff=coeff;
    switch (method)
    {
      case AffineDistortion:
      case RigidAffineDistortion:
      {
        AffineBestFitBounds(coeff,image,&min,&max);
        break;
      }
      case PerspectiveDistortion:
      {
        PerspectiveBestFitBounds(coeff,image,&min,&max);
        break;
      }
      case ArcDistortion:
      {
        ArcBestFitBounds(coeff,image,&min,&max);
        break;
      }
      default:
      {
        SetDistortPolarBestFit(&tile,&targets);
        break;
      }
    }

    SetBestFitGeometry(geometry,fix_bounds,min,max);

  }  /* end bestfit destination image calculations */
}

typedef struct _DistortLineInput
{
  const Image
    *image;

  Image
    *distort_image;

  CacheView
    *distort_view;

  DistortMethod
    method;

  double
    *coeff,
    output_scaling;

  RectangleInfo
    geometry;

  ssize_t
    j;

  int
    id;

  MagickBooleanType
    bestfit;

  size_t
    number_arguments;

  const double
    *arguments;

  ResampleFilter
    **resample_filter;

  PixelInfo
    invalid,
    zero;

  ExceptionInfo
    *exception;
} DistortLineInput;

typedef struct _DistortPixelTargets
{
  double
    *validity;

  PixelInfo
    *pixel;

  Quantum
    *magick_restrict *q;
} DistortPixelTargets;

static void DistortImagePixel(const DistortLineInput *line,
  const DistortPixelTargets *targets,const ssize_t i,MagickBooleanType *status)
{
  double
    *const validity = targets->validity;

  PixelInfo
    *const pixel = targets->pixel;

  Quantum
    *magick_restrict *const q = targets->q;

  PointInfo
    d,
    s;  /* transform destination image x,y  to source image x,y */

        DistortMapping
          mapping;

        DistortPointInput
          tile;

        /* map pixel coordinate to distortion space coordinate */
        d.x = (double) (line->geometry.x+i+0.5)*line->output_scaling;
        d.y = (double) (line->geometry.y+line->j+0.5)*line->output_scaling;
        s = d;  /* default is a no-op mapping */
        mapping.d=d;
        mapping.coeff=line->coeff;
        mapping.output_scaling=line->output_scaling;
        mapping.resample_filter=line->resample_filter;
        mapping.id=line->id;
        mapping.image=line->image;
        mapping.distort_image=line->distort_image;
        mapping.method=line->method;
        tile.method=line->method;
        tile.coeff=line->coeff;
        tile.output_scaling=line->output_scaling;
        tile.mapping=mapping;
        tile.resample_filter=line->resample_filter;
        tile.id=line->id;
        tile.i=i;
        tile.j=line->j;
        tile.number_arguments=line->number_arguments;
        tile.arguments=line->arguments;
        MapDistortPoint(&tile,&s,validity,&d);
        /* map virtual canvas location back to real image coordinate */
        if ( line->bestfit && line->method != ArcDistortion ) {
          s.x -= line->image->page.x;
          s.y -= line->image->page.y;
        }
        s.x -= 0.5;
        s.y -= 0.5;

        if ( (*validity) <= 0.0 ) {
          /* result of distortion is an invalid pixel - don't resample */
          SetPixelViaPixelInfo(line->distort_image,&line->invalid,(*q));
        }
        else {
          DistortPixelInput
            distort_input;

          distort_input.distort_image=line->distort_image;
          distort_input.resample_filter=line->resample_filter;
          distort_input.id=line->id;
          distort_input.s=s;
          distort_input.validity=(*validity);
          distort_input.invalid=line->invalid;
          distort_input.exception=line->exception;
          (*status)=ResampleDistortPixel(&distort_input,pixel,(*q));
        }
        (*q)+=(ptrdiff_t) GetPixelChannels(line->distort_image);
}

static MagickBooleanType DistortImageLine(const DistortLineInput *line,
  MagickBooleanType *status)
{
  Image
    *const distort_image = line->distort_image;

  CacheView
    *const distort_view = line->distort_view;

  const DistortMethod
    method = line->method;

  double
    *const coeff = line->coeff;

  const double
    output_scaling = line->output_scaling;

  const ssize_t
    j = line->j;

  const int
    id = line->id;

  ResampleFilter
    **const resample_filter = line->resample_filter;

  const PixelInfo
    zero = line->zero;

  ExceptionInfo
    *const exception = line->exception;

      double
        validity;  /* how mathematically valid is this the mapping */

      PixelInfo
        pixel;    /* pixel color to assign to distorted image */

      ssize_t
        i;

      Quantum
        *magick_restrict q;

      const DistortPixelTargets
        targets = { &validity, &pixel, &q };

      q=QueueCacheViewAuthenticPixels(distort_view,0,j,distort_image->columns,1,
        exception);
      if (q == (Quantum *) NULL)
        {
          (*status)=MagickFalse;
          return(MagickFalse);
        }
      pixel=zero;

      /* Define constant scaling vectors for Affine Distortions
        Other methods are either variable, or use interpolated lookup
      */
      switch (method)
      {
        case AffineDistortion:
        case RigidAffineDistortion:
          ScaleFilter( resample_filter[id],
            coeff[0], coeff[1],
            coeff[3], coeff[4] );
          break;
        default:
          break;
      }

      /* Initialize default pixel validity
      *    negative:         pixel is invalid  output 'matte_color'
      *    0.0 to 1.0:       antialiased, mix with resample output
      *    1.0 or greater:   use resampled output.
      */
      validity = 1.0;

      for (i=0; i < (ssize_t) distort_image->columns; i++)
        DistortImagePixel(line,&targets,i,status);
      SyncDistortRow(status,distort_view,exception);
  return(MagickTrue);
}

#define DistortImageTag  "Distort/Image"

typedef struct _DistortRenderInput
{
  const Image
    *image;

  Image
    *distort_image;

  DistortMethod
    method;

  double
    *coeff,
    output_scaling;

  RectangleInfo
    geometry;

  MagickBooleanType
    bestfit;

  size_t
    number_arguments;

  const double
    *arguments;

  PixelInfo
    invalid;

  ExceptionInfo
    *exception;
} DistortRenderInput;

static MagickBooleanType RenderDistortImage(const DistortRenderInput *render)
{
  const Image
    *const image = render->image;

  Image
    *const distort_image = render->distort_image;

  const DistortMethod
    method = render->method;

  double
    *const coeff = render->coeff;

  const double
    output_scaling = render->output_scaling;

  const RectangleInfo
    geometry = render->geometry;

  const MagickBooleanType
    bestfit = render->bestfit;

  const size_t
    number_arguments = render->number_arguments;

  const double
    *const arguments = render->arguments;

  const PixelInfo
    invalid = render->invalid;

  ExceptionInfo
    *const exception = render->exception;


    CacheView
      *distort_view;

    MagickBooleanType
      status;

    MagickOffsetType
      progress;

    PixelInfo
      zero;

    ResampleFilter
      **magick_restrict resample_filter;

    ssize_t
      j;

    status=MagickTrue;
    progress=0;
    GetPixelInfo(distort_image,&zero);
    resample_filter=AcquireResampleFilterTLS(image,UndefinedVirtualPixelMethod,
      MagickFalse,exception);
    distort_view=AcquireAuthenticCacheView(distort_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp parallel for schedule(static) shared(progress,status) \
      magick_number_threads(image,distort_image,distort_image->rows,1)
#endif
    for (j=0; j < (ssize_t) distort_image->rows; j++)
    {
      const int
        id = GetOpenMPThreadId();

      const DistortLineInput
        line = { image, distort_image, distort_view, method, coeff,
          output_scaling, geometry, j, id, bestfit, number_arguments,
          arguments, resample_filter, invalid, zero, exception };

      if (DistortImageLine(&line,&status) == MagickFalse)
        continue;
      if (image->progress_monitor != (MagickProgressMonitor) NULL)
        {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
          #pragma omp atomic
#endif
          progress++;
          SetDistortProgress(&status,image,DistortImageTag,progress);
        }
    }
    distort_view=DestroyCacheView(distort_view);
    resample_filter=DestroyResampleFilterTLS(resample_filter);
  return(status);
}

typedef struct _DistortLayoutInput
{
  const Image
    *image;

  DistortMethod
    method;

  size_t
    number_arguments;

  ExceptionInfo
    *exception;
} DistortLayoutInput;

typedef struct _DistortLayoutTargets
{
  RectangleInfo
    *geometry;

  MagickBooleanType
    *bestfit,
    *viewport_given;

  double
    *output_scaling;
} DistortLayoutTargets;

static void SetDefaultDistortGeometry(RectangleInfo *geometry,
  const Image *image)
{
  /* default output image bounds, when no 'bestfit' is requested */
  (*geometry).width=image->columns;
  (*geometry).height=image->rows;
  (*geometry).x=0;
  (*geometry).y=0;
}

static MagickBooleanType LayoutDistortImage(const DistortLayoutInput *layout,
  double *coeff,const DistortLayoutTargets *targets)
{
  const Image
    *const image = layout->image;

  const DistortMethod
    method = layout->method;

  const size_t
    number_arguments = layout->number_arguments;

  ExceptionInfo
    *const exception = layout->exception;

  RectangleInfo
    *const geometry = targets->geometry;

  MagickBooleanType
    *const bestfit = targets->bestfit,
    *const viewport_given = targets->viewport_given;

  double
    *const output_scaling = targets->output_scaling;

  DistortVerboseInput
    verbose_input;

  DistortScaleTargets
    scale_targets;

  DistortBestFitInput
    fit;

  /*
    Determine the size and offset for a 'bestfit' destination.
    Usually the four corners of the source image is enough.
  */

  SetDefaultDistortGeometry(geometry,image);

  if ( method == ArcDistortion ) {
    (*bestfit) = MagickTrue;  /* always calculate a 'best fit' viewport */
  }

  fit.image=image;
  fit.method=method;
  fit.number_arguments=number_arguments;
  fit.coeff=coeff;
  SetDistortBestFit(&fit,geometry,bestfit);

  /* The user provided a 'viewport' expert option which may
     overrides some parts of the current output image geometry.
     This also overrides its default 'bestfit' setting.
  */
  GetDistortViewport(image,geometry,viewport_given,exception);

  /* Verbose output */
  verbose_input.image=image;
  verbose_input.method=method;
  verbose_input.coeff=coeff;
  verbose_input.bestfit=(*bestfit);
  verbose_input.viewport_given=(*viewport_given);
  verbose_input.geometry=(*geometry);
  if (PrintDistortVerbose(&verbose_input,exception) == MagickFalse)
    return(MagickFalse);
  /*
    The user provided a 'scale' expert option will scale the output image size,
    by the factor given allowing for super-sampling of the distorted image
    space.  Any scaling factors must naturally be halved as a result.
  */
  scale_targets.output_scaling=(output_scaling);
  scale_targets.geometry=(geometry);
  if (ScaleDistortGeometry(image,&scale_targets) == MagickFalse)
    {
      coeff = (double *) RelinquishMagickMemory(coeff);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
              "InvalidArgument","%s", "-set option:distort:scale" );
      return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _DistortCreateInput
{
  const Image
    *image;

  DistortMethod
    method;

  double
    output_scaling;

  RectangleInfo
    geometry;

  MagickBooleanType
    bestfit;

  size_t
    number_arguments;

  const double
    *arguments;

  ExceptionInfo
    *exception;
} DistortCreateInput;

static void SetDistortImageAttributes(Image **distort_image,
  const RectangleInfo geometry,ExceptionInfo *exception)
{
  if ((IsPixelInfoGray(&(*distort_image)->background_color) == MagickFalse) &&
      (IsGrayColorspace((*distort_image)->colorspace) != MagickFalse))
    (void) SetImageColorspace((*distort_image),sRGBColorspace,exception);
  if ((*distort_image)->background_color.alpha_trait != UndefinedPixelTrait)
    (*distort_image)->alpha_trait=BlendPixelTrait;
  (*distort_image)->page.x=geometry.x;
  (*distort_image)->page.y=geometry.y;
}

static MagickBooleanType CreateDistortImage(const DistortCreateInput *create,
  double *coeff,Image **distort_image)
{
  const Image
    *const image = create->image;

  const DistortMethod
    method = create->method;

  const double
    output_scaling = create->output_scaling;

  const RectangleInfo
    geometry = create->geometry;

  const MagickBooleanType
    bestfit = create->bestfit;

  const size_t
    number_arguments = create->number_arguments;

  const double
    *const arguments = create->arguments;

  ExceptionInfo
    *const exception = create->exception;

  PixelInfo
    invalid;  /* the color to assign when distort result is invalid */


  /*
    Initialize the distort image attributes.
  */
  (*distort_image)=CloneImage(image,geometry.width,geometry.height,MagickTrue,
    exception);
  if ((*distort_image) == (Image *) NULL)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      return(MagickFalse);
    }
  /* if image is ColorMapped - change it to DirectClass */
  if (SetImageStorageClass((*distort_image),DirectClass,exception) == MagickFalse)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      (*distort_image)=DestroyImage((*distort_image));
      return(MagickFalse);
    }
  SetDistortImageAttributes(distort_image,geometry,exception);
  ConformPixelInfo((*distort_image),&(*distort_image)->matte_color,&invalid,
    exception);

  { /* ----- MAIN CODE -----
       Sample the source image to each pixel in the distort image.
     */
    DistortRenderInput
      render;

    MagickBooleanType
      status;

    render.image=image;
    render.distort_image=(*distort_image);
    render.method=method;
    render.coeff=coeff;
    render.output_scaling=output_scaling;
    render.geometry=geometry;
    render.bestfit=bestfit;
    render.number_arguments=number_arguments;
    render.arguments=arguments;
    render.invalid=invalid;
    render.exception=exception;
    status=RenderDistortImage(&render);

    if (status == MagickFalse)
      (*distort_image)=DestroyImage((*distort_image));
  }
  return(MagickTrue);
}

static void ResetArcPage(Image *distort_image,const DistortMethod method,
  const MagickBooleanType bestfit,const MagickBooleanType viewport_given)
{
  /* Arc does not return an offset unless 'bestfit' is in effect
     And the user has not provided an overriding 'viewport'.
   */
  if (IsArcPageReset(method,bestfit,viewport_given) != MagickFalse) {
    distort_image->page.x = 0;
    distort_image->page.y = 0;
  }
}

MagickExport Image *DistortImage(const Image *image, DistortMethod method,
  const size_t number_arguments,const double *arguments,
  MagickBooleanType bestfit,ExceptionInfo *exception)
{
  double
    *coeff,
    output_scaling;

  Image
    *distort_image;

  RectangleInfo
    geometry;  /* geometry of the distorted space viewport */

  MagickBooleanType
    viewport_given;

  DistortLayoutInput
    layout;

  DistortLayoutTargets
    layout_targets;

  DistortCreateInput
    create;

  const CoefficientInput input = { number_arguments, arguments, 0 };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  /*
    Handle Special Compound Distortions
  */
  if ( method == ResizeDistortion )
    return(DistortResizeFromArguments(image,number_arguments,arguments,
      exception));

  /*
    Convert input arguments (usually as control points for reverse mapping)
    into mapping coefficients to apply the distortion.

    Note that some distortions are mapped to other distortions,
    and as such do not require specific code after this point.
  */
  coeff = GenerateCoefficients(image, &method, &input, exception);
  if ( coeff == (double *) NULL )
    return((Image *) NULL);

  layout.image=image;
  layout.method=method;
  layout.number_arguments=number_arguments;
  layout.exception=exception;
  layout_targets.geometry=(&geometry);
  layout_targets.bestfit=(&bestfit);
  layout_targets.viewport_given=(&viewport_given);
  layout_targets.output_scaling=(&output_scaling);
  if (LayoutDistortImage(&layout,coeff,&layout_targets) == MagickFalse)
    return((Image *) NULL);

  create.image=image;
  create.method=method;
  create.output_scaling=output_scaling;
  create.geometry=geometry;
  create.bestfit=bestfit;
  create.number_arguments=number_arguments;
  create.arguments=arguments;
  create.exception=exception;
  if (CreateDistortImage(&create,coeff,&distort_image) == MagickFalse)
    return((Image *) NULL);

  ResetArcPage(distort_image,method,bestfit,viewport_given);
  coeff=(double *) RelinquishMagickMemory(coeff);
  return(distort_image);
}


