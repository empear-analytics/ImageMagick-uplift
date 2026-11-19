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
#include "MagickCore/distort-map-private.h"



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
        if (PrintAffineFX(&fx,exception) == MagickFalse)
          return(MagickFalse);
        break;
      }
      case PerspectiveDistortion:
      {
        if (PrintPerspectiveFX(&fx,exception) == MagickFalse)
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


