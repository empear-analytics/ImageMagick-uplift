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
%                  SSSSS  PPPP    AAA   RRRR   SSSSS  EEEEE                   %
%                  SS     P   P  A   A  R   R  SS     E                       %
%                   SSS   PPPP   AAAAA  RRRR    SSS   EEE                     %
%                     SS  P      A   A  R R       SS  E                       %
%                  SSSSS  P      A   A  R  R   SSSSS  EEEEE                   %
%                                                                             %
%                                                                             %
%                       MagickCore Sparse Color Methods                       %
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S p a r s e C o l o r I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SparseColorImage(), given a set of coordinates, interpolates the colors
%  found at those coordinates, across the whole image, using various methods.
%
%  The format of the SparseColorImage() method is:
%
%      Image *SparseColorImage(const Image *image,
%        const SparseColorMethod method,const size_t number_arguments,
%        const double *arguments,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image to be filled in.
%
%    o method: the method to fill in the gradient between the control points.
%
%        The methods used for SparseColor() are often simular to methods
%        used for DistortImage(), and even share the same code for determination
%        of the function coefficients, though with more dimensions (or resulting
%        values).
%
%    o number_arguments: the number of arguments given.
%
%    o arguments: array of floating point arguments for this method--
%        x,y,color_values-- with color_values given as normalized values.
%
%    o exception: return any errors or warnings in this structure
%
*/
static inline MagickBooleanType IsSparseBlackChannel(const Image *traits_image,
  const Image *image)
{
  return((((GetPixelBlackTraits(traits_image) & UpdatePixelTrait) != 0) &&
    (image->colorspace == CMYKColorspace)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsSparseAlphaChannel(const Image *traits_image,
  const Image *image)
{
  return((((GetPixelAlphaTraits(traits_image) & UpdatePixelTrait) != 0) &&
    (image->alpha_trait != UndefinedPixelTrait)) ? MagickTrue : MagickFalse);
}

static void PrintBarycentricSparseFX(const Image *image,const double *coeff)
{
  ssize_t x=0;
  (void) FormatLocaleFile(stderr, "Barycentric Sparse Color:\n");
  if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
    (void) FormatLocaleFile(stderr, "  -channel R -fx '%+lf*i %+lf*j %+lf' \\\n",
        coeff[x], coeff[x+1], coeff[x+2]),x+=3;
  if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
    (void) FormatLocaleFile(stderr, "  -channel G -fx '%+lf*i %+lf*j %+lf' \\\n",
        coeff[x], coeff[x+1], coeff[x+2]),x+=3;
  if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
    (void) FormatLocaleFile(stderr, "  -channel B -fx '%+lf*i %+lf*j %+lf' \\\n",
        coeff[x], coeff[x+1], coeff[x+2]),x+=3;
  if (IsSparseBlackChannel(image,image) != MagickFalse)
    (void) FormatLocaleFile(stderr, "  -channel K -fx '%+lf*i %+lf*j %+lf' \\\n",
        coeff[x], coeff[x+1], coeff[x+2]),x+=3;
  if (IsSparseAlphaChannel(image,image) != MagickFalse)
    (void) FormatLocaleFile(stderr, "  -channel A -fx '%+lf*i %+lf*j %+lf' \\\n",
        coeff[x], coeff[x+1], coeff[x+2]),x+=3;
}

static void PrintBilinearSparseFX(const Image *image,const double *coeff)
{
  ssize_t x=0;
  (void) FormatLocaleFile(stderr, "Bilinear Sparse Color\n");
  if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
    (void) FormatLocaleFile(stderr, "   -channel R -fx '%+lf*i %+lf*j %+lf*i*j %+lf;\n",
        coeff[ x ], coeff[x+1],
        coeff[x+2], coeff[x+3]),x+=4;
  if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
    (void) FormatLocaleFile(stderr, "   -channel G -fx '%+lf*i %+lf*j %+lf*i*j %+lf;\n",
        coeff[ x ], coeff[x+1],
        coeff[x+2], coeff[x+3]),x+=4;
  if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
    (void) FormatLocaleFile(stderr, "   -channel B -fx '%+lf*i %+lf*j %+lf*i*j %+lf;\n",
        coeff[ x ], coeff[x+1],
        coeff[x+2], coeff[x+3]),x+=4;
  if (IsSparseBlackChannel(image,image) != MagickFalse)
    (void) FormatLocaleFile(stderr, "   -channel K -fx '%+lf*i %+lf*j %+lf*i*j %+lf;\n",
        coeff[ x ], coeff[x+1],
        coeff[x+2], coeff[x+3]),x+=4;
  if (IsSparseAlphaChannel(image,image) != MagickFalse)
    (void) FormatLocaleFile(stderr, "   -channel A -fx '%+lf*i %+lf*j %+lf*i*j %+lf;\n",
        coeff[ x ], coeff[x+1],
        coeff[x+2], coeff[x+3]),x+=4;
}

static void PrintSparseColorMethodFX(const Image *image,const double *coeff,
  const SparseColorMethod sparse_method)
{
  switch (sparse_method) {
    case BarycentricColorInterpolate:
    {
      PrintBarycentricSparseFX(image,coeff);
      break;
    }
    case BilinearColorInterpolate:
    {
      PrintBilinearSparseFX(image,coeff);
      break;
    }
    default:
      /* sparse color method is too complex for FX emulation */
      break;
  }
}

static void PrintSparseColorFX(const Image *image,const double *coeff,
  const SparseColorMethod sparse_method)
{
  if (IsStringTrue(GetImageArtifact(image,"verbose")) != MagickFalse) {

    PrintSparseColorMethodFX(image,coeff,sparse_method);
  }
}

static size_t GetSparseColorCount(const Image *image)
{
  size_t
    number_colors;

  number_colors=0;
  if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
    number_colors++;
  if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
    number_colors++;
  if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
    number_colors++;
  if (IsSparseBlackChannel(image,image) != MagickFalse)
    number_colors++;
  if (IsSparseAlphaChannel(image,image) != MagickFalse)
    number_colors++;
  return(number_colors);
}

typedef struct _SparseControlColor
{
  const Image
    *image;

  Image
    *sparse_image;

  const double
    *arguments;
} SparseControlColor;

static void SetSparseControlColor(const SparseControlColor *control,
  const size_t k,PixelInfo *pixel)
{
  const Image
    *const image = control->image;

  Image
    *const sparse_image = control->sparse_image;

  const double
    *const arguments = control->arguments;

  ssize_t x=(ssize_t) k+2;
  if ((GetPixelRedTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel->red=arguments[x++];
  if ((GetPixelGreenTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel->green=arguments[x++];
  if ((GetPixelBlueTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel->blue=arguments[x++];
  if (IsSparseBlackChannel(sparse_image,image) != MagickFalse)
    pixel->black=arguments[x++];
  if (IsSparseAlphaChannel(sparse_image,sparse_image) != MagickFalse)
    pixel->alpha=arguments[x++];
}

static double ManhattanDistance(const ssize_t i,const ssize_t j,
  const double *arguments,const size_t k)
{
  return(fabs((double)i-arguments[ k ])+
    fabs((double)j-arguments[k+1]));
}

static double VoronoiDistance(const ssize_t i,const ssize_t j,
  const double *arguments,const size_t k)
{
  return(
      ((double) i-arguments[ k ])*((double) i-arguments[ k ])
    + ((double) j-arguments[k+1])*((double) j-arguments[k+1]));
}

typedef struct _SparseColorPoint
{
  const Image
    *image;

  Image
    *sparse_image;

  size_t
    number_arguments;

  const double
    *arguments;

  size_t
    number_colors;

  ssize_t
    i,
    j;
} SparseColorPoint;

static void ManhattanSparseColor(const SparseColorPoint *point,
  PixelInfo *pixel)
{
  const Image
    *const image = point->image;

  Image
    *const sparse_image = point->sparse_image;

  const size_t
    number_arguments = point->number_arguments;

  const double
    *const arguments = point->arguments;

  const size_t
    number_colors = point->number_colors;

  const ssize_t
    i = point->i,
    j = point->j;

  double
    minimum = MagickMaximumValue;

  size_t
    k;

  /*
    Just use the closest control point you can find!
  */
  for (k=0; k<number_arguments; k+=2+number_colors)
  {
    double distance = ManhattanDistance(i,j,arguments,k);
    if ( distance < minimum ) {
      SparseControlColor
        control;

      control.image=image;
      control.sparse_image=sparse_image;
      control.arguments=arguments;
      SetSparseControlColor(&control,k,pixel);
      minimum = distance;
    }
  }
}

static PixelInfo VoronoiSparseColor(const SparseColorPoint *point,
  PixelInfo pixel)
{
  double
    minimum = MagickMaximumValue;

  size_t
    k;

  /*
    Just use the closest control point you can find!
  */
  for (k=0; k<point->number_arguments; k+=2+point->number_colors) {
    double distance = VoronoiDistance(point->i,point->j,point->arguments,k);
    if ( distance < minimum ) {
      SparseControlColor
        control;

      control.image=point->image;
      control.sparse_image=point->sparse_image;
      control.arguments=point->arguments;
      SetSparseControlColor(&control,k,&pixel);
      minimum = distance;
    }
  }
  return(pixel);
}

typedef struct _SparseColorPlane
{
  Image
    *sparse_image;

  double
    *coeff;

  ssize_t
    i,
    j;
} SparseColorPlane;

static PixelInfo BarycentricSparseColor(const SparseColorPlane *plane,
  PixelInfo pixel)
{
  Image
    *const sparse_image = plane->sparse_image;

  double
    *const coeff = plane->coeff;

  const ssize_t
    i = plane->i,
    j = plane->j;

  ssize_t x=0;
  if ((GetPixelRedTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.red     = coeff[x]*i +coeff[x+1]*j
                    +coeff[x+2], x+=3;
  if ((GetPixelGreenTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.green   = coeff[x]*i +coeff[x+1]*j
                    +coeff[x+2], x+=3;
  if ((GetPixelBlueTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.blue    = coeff[x]*i +coeff[x+1]*j
                    +coeff[x+2], x+=3;
  if (IsSparseBlackChannel(sparse_image,sparse_image) != MagickFalse)
    pixel.black   = coeff[x]*i +coeff[x+1]*j
                    +coeff[x+2], x+=3;
  if (IsSparseAlphaChannel(sparse_image,sparse_image) != MagickFalse)
    pixel.alpha = coeff[x]*i +coeff[x+1]*j
                    +coeff[x+2], x+=3;
  return(pixel);
}

static PixelInfo BilinearSparseColor(const SparseColorPlane *plane,
  const Image *image,PixelInfo pixel)
{
  ssize_t x=0;
  if ((GetPixelRedTraits(plane->sparse_image) & UpdatePixelTrait) != 0)
    pixel.red     = plane->coeff[x]*plane->i     + plane->coeff[x+1]*plane->j +
                    plane->coeff[x+2]*plane->i*plane->j + plane->coeff[x+3], x+=4;
  if ((GetPixelGreenTraits(plane->sparse_image) & UpdatePixelTrait) != 0)
    pixel.green   = plane->coeff[x]*plane->i     + plane->coeff[x+1]*plane->j +
                    plane->coeff[x+2]*plane->i*plane->j + plane->coeff[x+3], x+=4;
  if ((GetPixelBlueTraits(plane->sparse_image) & UpdatePixelTrait) != 0)
    pixel.blue    = plane->coeff[x]*plane->i     + plane->coeff[x+1]*plane->j +
                    plane->coeff[x+2]*plane->i*plane->j + plane->coeff[x+3], x+=4;
  if (IsSparseBlackChannel(plane->sparse_image,image) != MagickFalse)
    pixel.black   = plane->coeff[x]*plane->i     + plane->coeff[x+1]*plane->j +
                    plane->coeff[x+2]*plane->i*plane->j + plane->coeff[x+3], x+=4;
  if (IsSparseAlphaChannel(plane->sparse_image,
      plane->sparse_image) != MagickFalse)
    pixel.alpha = plane->coeff[x]*plane->i     + plane->coeff[x+1]*plane->j +
                    plane->coeff[x+2]*plane->i*plane->j + plane->coeff[x+3], x+=4;
  return(pixel);
}

static PixelInfo AddWeightedControlColor(const SparseControlColor *control,
  ssize_t x,const double weight,PixelInfo pixel)
{
  if ((GetPixelRedTraits(control->sparse_image) & UpdatePixelTrait) != 0)
    pixel.red     += control->arguments[x++]*weight;
  if ((GetPixelGreenTraits(control->sparse_image) & UpdatePixelTrait) != 0)
    pixel.green   += control->arguments[x++]*weight;
  if ((GetPixelBlueTraits(control->sparse_image) & UpdatePixelTrait) != 0)
    pixel.blue    += control->arguments[x++]*weight;
  if (IsSparseBlackChannel(control->sparse_image,control->image) != MagickFalse)
    pixel.black   += control->arguments[x++]*weight;
  if (IsSparseAlphaChannel(control->sparse_image,
      control->sparse_image) != MagickFalse)
    pixel.alpha += control->arguments[x++]*weight;
  return(pixel);
}

typedef struct _SparseColorWeights
{
  const Image
    *image;

  Image
    *sparse_image;

  size_t
    number_arguments;

  const double
    *arguments;

  size_t
    number_colors;

  double
    *coeff;

  ssize_t
    i,
    j;
} SparseColorWeights;

static void AccumulateInverseSparseColor(const SparseColorWeights *weights,
  PixelInfo *pixel,double *denominator)
{
  size_t
    k;

  for (k=0; k<weights->number_arguments; k+=2+weights->number_colors)
  {
    double weight =
        ((double) weights->i-weights->arguments[ k ])*((double) weights->i-weights->arguments[ k ])
      + ((double) weights->j-weights->arguments[k+1])*((double) weights->j-weights->arguments[k+1]);
    ssize_t x = (ssize_t) k+2;

    SparseControlColor
      control;

    weight = pow(weight,weights->coeff[0]); /* inverse of power factor */
    weight = ( weight < 1.0 ) ? 1.0 : 1.0/weight;
    control.image=weights->image;
    control.sparse_image=weights->sparse_image;
    control.arguments=weights->arguments;
    *pixel=AddWeightedControlColor(&control,x,weight,*pixel);
    *denominator += weight;
  }
}

static PixelInfo ClearSparseColor(const Image *image,
  const Image *sparse_image,PixelInfo pixel)
{
  if ((GetPixelRedTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.red=0.0;
  if ((GetPixelGreenTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.green=0.0;
  if ((GetPixelBlueTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.blue=0.0;
  if (IsSparseBlackChannel(sparse_image,image) != MagickFalse)
    pixel.black=0.0;
  if (IsSparseAlphaChannel(sparse_image,sparse_image) != MagickFalse)
    pixel.alpha=0.0;
  return(pixel);
}

static PixelInfo DivideSparseColor(const SparseColorWeights *inverse,
  PixelInfo pixel,const double denominator)
{
  if ((GetPixelRedTraits(inverse->sparse_image) & UpdatePixelTrait) != 0)
    pixel.red/=denominator;
  if ((GetPixelGreenTraits(inverse->sparse_image) & UpdatePixelTrait) != 0)
    pixel.green/=denominator;
  if ((GetPixelBlueTraits(inverse->sparse_image) & UpdatePixelTrait) != 0)
    pixel.blue/=denominator;
  if (IsSparseBlackChannel(inverse->sparse_image,inverse->image) != MagickFalse)
    pixel.black/=denominator;
  if (IsSparseAlphaChannel(inverse->sparse_image,
      inverse->sparse_image) != MagickFalse)
    pixel.alpha/=denominator;
  return(pixel);
}

static PixelInfo InverseSparseColor(const SparseColorWeights *inverse,
  PixelInfo pixel)
{
  double
    denominator;

  SparseColorWeights
    weights;

  pixel=ClearSparseColor(inverse->image,inverse->sparse_image,pixel);
  denominator = 0.0;
  weights.image=inverse->image;
  weights.sparse_image=inverse->sparse_image;
  weights.number_arguments=inverse->number_arguments;
  weights.arguments=inverse->arguments;
  weights.number_colors=inverse->number_colors;
  weights.coeff=inverse->coeff;
  weights.i=inverse->i;
  weights.j=inverse->j;
  AccumulateInverseSparseColor(&weights,&pixel,&denominator);
  pixel=DivideSparseColor(inverse,pixel,denominator);
  return(pixel);
}

#define SparseColorTag  "Distort/SparseColor"

static PixelInfo ClampSparseColor(const Image *image,
  Image *sparse_image,PixelInfo pixel)
{
  if ((GetPixelRedTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.red=(MagickRealType) ClampPixel((double) QuantumRange*
      pixel.red);
  if ((GetPixelGreenTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.green=(MagickRealType) ClampPixel((double) QuantumRange*
      pixel.green);
  if ((GetPixelBlueTraits(sparse_image) & UpdatePixelTrait) != 0)
    pixel.blue=(MagickRealType) ClampPixel((double) QuantumRange*
      pixel.blue);
  if (IsSparseBlackChannel(sparse_image,image) != MagickFalse)
    pixel.black=(MagickRealType) ClampPixel((double) QuantumRange*
      pixel.black);
  if (IsSparseAlphaChannel(sparse_image,image) != MagickFalse)
    pixel.alpha=(MagickRealType) ClampPixel((double) QuantumRange*
      pixel.alpha);
  return(pixel);
}

typedef struct _SparseColorPixelInput
{
  SparseColorMethod
    sparse_method;

  const Image
    *image;

  Image
    *sparse_image;

  size_t
    number_arguments;

  const double
    *arguments;

  size_t
    number_colors;

  double
    *coeff;

  ssize_t
    i,
    j;
} SparseColorPixelInput;

static void SparseColorPixel(const SparseColorPixelInput *pixel_input,
  PixelInfo *pixel)
{
  switch (pixel_input->sparse_method)
  {
    case BilinearColorInterpolate:
    {
      SparseColorPlane
        plane;

      plane.sparse_image=pixel_input->sparse_image;
      plane.coeff=pixel_input->coeff;
      plane.i=pixel_input->i;
      plane.j=pixel_input->j;
      (*pixel)=BilinearSparseColor(&plane,pixel_input->image,(*pixel));
      break;
    }
    case InverseColorInterpolate:
    case ShepardsColorInterpolate:
    { /* Inverse (Squared) Distance weights average (IDW) */
      SparseColorWeights
        inverse;

      inverse.image=pixel_input->image;
      inverse.sparse_image=pixel_input->sparse_image;
      inverse.number_arguments=pixel_input->number_arguments;
      inverse.arguments=pixel_input->arguments;
      inverse.number_colors=pixel_input->number_colors;
      inverse.coeff=pixel_input->coeff;
      inverse.i=pixel_input->i;
      inverse.j=pixel_input->j;
      (*pixel)=InverseSparseColor(&inverse,(*pixel));
      break;
    }
    case ManhattanColorInterpolate:
    {
      SparseColorPoint
        point;

      point.image=pixel_input->image;
      point.sparse_image=pixel_input->sparse_image;
      point.number_arguments=pixel_input->number_arguments;
      point.arguments=pixel_input->arguments;
      point.number_colors=pixel_input->number_colors;
      point.i=pixel_input->i;
      point.j=pixel_input->j;
      ManhattanSparseColor(&point,&(*pixel));
      break;
    }
    case VoronoiColorInterpolate:
    default:
    {
      SparseColorPoint
        point;

      point.image=pixel_input->image;
      point.sparse_image=pixel_input->sparse_image;
      point.number_arguments=pixel_input->number_arguments;
      point.arguments=pixel_input->arguments;
      point.number_colors=pixel_input->number_colors;
      point.i=pixel_input->i;
      point.j=pixel_input->j;
      (*pixel)=VoronoiSparseColor(&point,(*pixel));
      break;
    }
  }
}

typedef struct _SparseColorRowInput
{
  SparseColorMethod
    sparse_method;

  const Image
    *image;

  Image
    *sparse_image;

  size_t
    number_arguments;

  const double
    *arguments;

  size_t
    number_colors;

  double
    *coeff;
} SparseColorRowInput;

static void SparseColorRow(const SparseColorRowInput *row,const ssize_t j,
  Quantum *q,PixelInfo pixel)
{
  const SparseColorMethod
    sparse_method = row->sparse_method;

  const Image
    *const image = row->image;

  Image
    *const sparse_image = row->sparse_image;

  const size_t
    number_arguments = row->number_arguments;

  const double
    *const arguments = row->arguments;

  const size_t
    number_colors = row->number_colors;

  double
    *const coeff = row->coeff;

  ssize_t
    i;

  for (i=0; i < (ssize_t) sparse_image->columns; i++)
  {
    GetPixelInfoPixel(sparse_image,q,&pixel);
    switch (sparse_method)
    {
      case BarycentricColorInterpolate:
      {
        SparseColorPlane
          plane;

        plane.sparse_image=sparse_image;
        plane.coeff=coeff;
        plane.i=i;
        plane.j=j;
        pixel=BarycentricSparseColor(&plane,pixel);
        break;
      }
      default:
      {
        SparseColorPixelInput
          pixel_input;

        pixel_input.sparse_method=sparse_method;
        pixel_input.image=image;
        pixel_input.sparse_image=sparse_image;
        pixel_input.number_arguments=number_arguments;
        pixel_input.arguments=arguments;
        pixel_input.number_colors=number_colors;
        pixel_input.coeff=coeff;
        pixel_input.i=i;
        pixel_input.j=j;
        SparseColorPixel(&pixel_input,&pixel);
        break;
      }
    }
    /* set the color directly back into the source image */
    pixel=ClampSparseColor(image,sparse_image,pixel);
    SetPixelViaPixelInfo(sparse_image,&pixel,q);
    q+=(ptrdiff_t) GetPixelChannels(sparse_image);
  }
}

static void SetSparseColorRGB(Image **sparse_image,ExceptionInfo *exception)
{
  if (IsGrayColorspace((*sparse_image)->colorspace) != MagickFalse)
    (void) SetImageColorspace((*sparse_image),sRGBColorspace,exception);
}

static MagickBooleanType CloneSparseColorImage(const Image *image,
  Image **sparse_image,ExceptionInfo *exception)
{
  (*sparse_image)=CloneImage(image,0,0,MagickTrue,exception);
  if ((*sparse_image) == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass((*sparse_image),DirectClass,exception) == MagickFalse)
    { /* if image is ColorMapped - change it to DirectClass */
      (*sparse_image)=DestroyImage((*sparse_image));
      return(MagickFalse);
    }
  SetSparseColorRGB(sparse_image,exception);
  return(MagickTrue);
}

typedef struct _SparseColorRender
{
  const Image
    *image;

  SparseColorMethod
    sparse_method;

  size_t
    number_arguments;

  const double
    *arguments;

  size_t
    number_colors;

  double
    *coeff;
} SparseColorRender;

static Image *FinishSparseColorImage(const MagickBooleanType status,
  Image *sparse_image)
{
  if (status == MagickFalse)
    sparse_image=DestroyImage(sparse_image);
  return(sparse_image);
}

static Image *RenderSparseColorImage(const SparseColorRender *render,
  Image *sparse_image,ExceptionInfo *exception)
{
  /* ----- MAIN CODE ----- */
  const Image
    *const image = render->image;

  const SparseColorMethod
    sparse_method = render->sparse_method;

  const size_t
    number_arguments = render->number_arguments;

  const double
    *const arguments = render->arguments;

  const size_t
    number_colors = render->number_colors;

  double
    *const coeff = render->coeff;

  CacheView
    *sparse_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    j;

  status=MagickTrue;
  progress=0;
  sparse_view=AcquireAuthenticCacheView(sparse_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,sparse_image,sparse_image->rows,1)
#endif
  for (j=0; j < (ssize_t) sparse_image->rows; j++)
  {
    PixelInfo
      pixel;    /* pixel to assign to distorted image */

    Quantum
      *magick_restrict q;

    SparseColorRowInput
      row;

    q=GetCacheViewAuthenticPixels(sparse_view,0,j,sparse_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    GetPixelInfo(sparse_image,&pixel);
    row.sparse_method=sparse_method;
    row.image=image;
    row.sparse_image=sparse_image;
    row.number_arguments=number_arguments;
    row.arguments=arguments;
    row.number_colors=number_colors;
    row.coeff=coeff;
    SparseColorRow(&row,j,q,pixel);
    SyncDistortRow(&status,sparse_view,exception);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        SetDistortProgress(&status,image,SparseColorTag,progress);
      }
  }
  sparse_view=DestroyCacheView(sparse_view);
  return(FinishSparseColorImage(status,sparse_image));
}

static SparseColorMethod SelectSparseColorMethod(
  const DistortMethod distort_method,const SparseColorMethod method)
{
  SparseColorMethod
    sparse_method;

  sparse_method = (SparseColorMethod) distort_method;
  if ( distort_method == ShepardsDistortion )
    sparse_method = method;   /* return non-distort methods to normal */
  return(sparse_method);
}

static DistortMethod GetSparseDistortMethod(const SparseColorMethod method)
{
  DistortMethod
    distort_method;

  distort_method=(DistortMethod) method;
  if ( distort_method >= SentinelDistortion )
    distort_method = ShepardsDistortion; /* Pretend to be Shepards */
  return(distort_method);
}

MagickExport Image *SparseColorImage(const Image *image,
  const SparseColorMethod method,const size_t number_arguments,
  const double *arguments,ExceptionInfo *exception)
{
  double
    *coeff;

  Image
    *sparse_image;

  size_t
    number_colors;

  SparseColorMethod
    sparse_method;

  SparseColorRender
    render;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);

  /* Determine number of color values needed per control point */
  number_colors=GetSparseColorCount(image);

  /*
    Convert input arguments into mapping coefficients, in this case
    we are mapping (distorting) colors, rather than coordinates.
  */
  { DistortMethod
      distort_method;

    const CoefficientInput input = { number_arguments, arguments,
      number_colors };

    distort_method=GetSparseDistortMethod(method);
    coeff = GenerateCoefficients(image, &distort_method, &input, exception);
    if ( coeff == (double *) NULL )
      return((Image *) NULL);
    /*
      Note some Distort Methods may fall back to other simpler methods,
      Currently the only fallback of concern is Bilinear to Affine
      (Barycentric), which is also sparse_colr method.  This also ensures
      correct two and one color Barycentric handling.
    */
    sparse_method=SelectSparseColorMethod(distort_method,method);
    if ( sparse_method == InverseColorInterpolate )
      coeff[0]=0.5;            /* sqrt() the squared distance for inverse */
  }

  /* Verbose output */
  PrintSparseColorFX(image,coeff,sparse_method);

  /* Generate new image for generated interpolated gradient.
   * ASIDE: Actually we could have just replaced the colors of the original
   * image, but IM Core policy, is if storage class could change then clone
   * the image.
   */

  if (CloneSparseColorImage(image,&sparse_image,exception) == MagickFalse)
    return((Image *) NULL);
  render.image=image;
  render.sparse_method=sparse_method;
  render.number_arguments=number_arguments;
  render.arguments=arguments;
  render.number_colors=number_colors;
  render.coeff=coeff;
  sparse_image=RenderSparseColorImage(&render,sparse_image,exception);
  coeff = (double *) RelinquishMagickMemory(coeff);
  return(sparse_image);
}
