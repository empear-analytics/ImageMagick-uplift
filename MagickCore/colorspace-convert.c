/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     CCCC   OOO   L       OOO   RRRR   SSSSS  PPPP    AAA    CCCC  EEEEE     %
%    C      O   O  L      O   O  R   R  SS     P   P  A   A  C      E         %
%    C      O   O  L      O   O  RRRR    SSS   PPPP   AAAAA  C      EEE       %
%    C      O   O  L      O   O  R R       SS  P      A   A  C      E         %
%     CCCC   OOO   LLLLL   OOO   R  R   SSSSS  P      A   A   CCCC  EEEEE     %
%                                                                             %
%                CCCC   OOO   N   N  V   V  EEEEE  RRRR   TTTTT               %
%               C      O   O  NN  N  V   V  E      R   R    T                 %
%               C      O   O  N N N  V   V  EEE    RRRR     T                 %
%               C      O   O  N  NN   V V   E      R R      T                 %
%                CCCC   OOO   N   N    V    EEEEE  R  R     T                 %
%                                                                             %
%                                                                             %
%                  MagickCore Colorspace Conversion Methods                   %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
#include "MagickCore/attribute.h"
#include "MagickCore/property.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/utility.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n v e r t H S L T o R G B                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvertHSLToRGB() transforms a (hue, saturation, lightness) to a (red,
%  green, blue) triple.
%
%  The format of the ConvertHSLToRGBImage method is:
%
%      void ConvertHSLToRGB(const double hue,const double saturation,
%        const double lightness,double *red,double *green,double *blue)
%
%  A description of each parameter follows:
%
%    o hue, saturation, lightness: A double value representing a
%      component of the HSL color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
*/
static double ComputeHSLChroma(const double saturation,
  const double lightness)
{
  double
    c;

  if (lightness <= 0.5)
    c=2.0*lightness*saturation;
  else
    c=(2.0-2.0*lightness)*saturation;
  return(c);
}

MagickExport void ConvertHSLToRGB(const double hue,const double saturation,
  const double lightness,double *red,double *green,double *blue)
{
  double
    c,
    h,
    min,
    x;

  /*
    Convert HSL to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  h=hue*360.0;
  c=ComputeHSLChroma(saturation,lightness);
  min=lightness-0.5*c;
  h-=360.0*floor(h/360.0);
  h/=60.0;
  x=c*(1.0-fabs(h-2.0*floor(h/2.0)-1.0));
  switch ((int) floor(h))
  {
    case 0:
    default:
    {
      *red=(double) QuantumRange*(min+c);
      *green=(double) QuantumRange*(min+x);
      *blue=(double) QuantumRange*min;
      break;
    }
    case 1:
    {
      *red=(double) QuantumRange*(min+x);
      *green=(double) QuantumRange*(min+c);
      *blue=(double) QuantumRange*min;
      break;
    }
    case 2:
    {
      *red=(double) QuantumRange*min;
      *green=(double) QuantumRange*(min+c);
      *blue=(double) QuantumRange*(min+x);
      break;
    }
    case 3:
    {
      *red=(double) QuantumRange*min;
      *green=(double) QuantumRange*(min+x);
      *blue=(double) QuantumRange*(min+c);
      break;
    }
    case 4:
    {
      *red=(double) QuantumRange*(min+x);
      *green=(double) QuantumRange*min;
      *blue=(double) QuantumRange*(min+c);
      break;
    }
    case 5:
    {
      *red=(double) QuantumRange*(min+c);
      *green=(double) QuantumRange*min;
      *blue=(double) QuantumRange*(min+x);
      break;
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n v e r t R G B T o G e n e r i c                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvertRGBToGeneric() transforms a (red, green, blue) to a generic (X, Y, Z)
%  triple.
%
%  The format of the ConvertRGBToGeneric method is:
%
%      void ConvertRGBToGeneric(const double red,const double green,
%        const double blue,double *hue,double *saturation,double *lightness)
%
%  A description of each parameter follows:
%
%    o red, green, blue: A Quantum value representing the red, green, and
%      blue component of a pixel..
%
%    o white_luminance: white luminance.
%
%    o illuminant: illuminant, typically D65.
%
%    o X, Y, Z: A pointer to a double value representing a component of a
%      generic color space.
%
*/
typedef struct _RGBToGenericInput
{
  ColorspaceType
    colorspace;

  double
    R,
    G,
    B;

  double
    *X,
    *Y,
    *Z;
} RGBToGenericInput;

static void ConvertRGBToYDbDrThroughYUV(const RGBToGenericInput *input)
{
  const ColorspaceType
    colorspace = input->colorspace;

  const double
    R = input->R,
    G = input->G,
    B = input->B;

  double
    *const X = input->X,
    *const Y = input->Y,
    *const Z = input->Z;

  switch (colorspace)
  {
    case YDbDrColorspace:
    {
      ConvertRGBToYDbDr(R,G,B,X,Y,Z);
      break;
    }
    case YIQColorspace:
    {
      ConvertRGBToYIQ(R,G,B,X,Y,Z);
      break;
    }
    case YPbPrColorspace:
    {
      ConvertRGBToYPbPr(R,G,B,X,Y,Z);
      break;
    }
    case YUVColorspace:
    {
      ConvertRGBToYUV(R,G,B,X,Y,Z);
      break;
    }
    default:
    {
      *X=QuantumScale*R;
      *Y=QuantumScale*G;
      *Z=QuantumScale*B;
      break;
    }
  }
}

static void ConvertRGBToOklabThroughYCbCr(const RGBToGenericInput *input)
{
  const ColorspaceType
    colorspace = input->colorspace;

  const double
    R = input->R,
    G = input->G,
    B = input->B;

  double
    *const X = input->X,
    *const Y = input->Y,
    *const Z = input->Z;

  switch (colorspace)
  {
    case OklabColorspace:
    {
      ConvertRGBToOklab(R,G,B,X,Y,Z);
      break;
    }
    case OklchColorspace:
    {
      ConvertRGBToOklch(R,G,B,X,Y,Z);
      break;
    }
    case ProPhotoColorspace:
    {
      ConvertRGBToProPhoto(R,G,B,X,Y,Z);
      break;
    }
    case xyYColorspace:
    {
      ConvertRGBToxyY(R,G,B,X,Y,Z);
      break;
    }
    case XYZColorspace:
    {
      ConvertRGBToXYZ(R,G,B,X,Y,Z);
      break;
    }
    case YCbCrColorspace:
    {
      ConvertRGBToYCbCr(R,G,B,X,Y,Z);
      break;
    }
    default:
    {
      ConvertRGBToYDbDrThroughYUV(input);
      break;
    }
  }
}

MagickPrivate void ConvertRGBToGeneric(const ColorspaceType colorspace,
  const double R,const double G,const double B,const double white_luminance,
  const IlluminantType illuminant,double *X,double *Y,double *Z)
{
  RGBToGenericInput
    input;

  input.colorspace=colorspace;
  input.R=R;
  input.G=G;
  input.B=B;
  input.X=X;
  input.Y=Y;
  input.Z=Z;
  switch (colorspace)
  {
    case Adobe98Colorspace:
    {
      ConvertRGBToAdobe98(R,G,B,X,Y,Z);
      break;
    }
    case CAT02LMSColorspace:
    {
      double
        L,
        M,
        S;

      ConvertRGBToCAT02LMS(R,G,B,&L,&M,&S);
      ConvertCAT02LMSToXYZ(L,M,S,X,Y,Z);
      break;
    }
    case CMYColorspace:
    {
      ConvertRGBToCMY(R,G,B,X,Y,Z);
      break;
    }
    case DisplayP3Colorspace:
    {
      ConvertRGBToDisplayP3(R,G,B,X,Y,Z);
      break;
    }
    case HCLColorspace:
    {
      ConvertRGBToHCL(R,G,B,X,Y,Z);
      break;
    }
    case HCLpColorspace:
    {
      ConvertRGBToHCLp(R,G,B,X,Y,Z);
      break;
    }
    case HSBColorspace:
    {
      ConvertRGBToHSB(R,G,B,X,Y,Z);
      break;
    }
    case HSIColorspace:
    {
      ConvertRGBToHSI(R,G,B,X,Y,Z);
      break;
    }
    case HSLColorspace:
    {
      ConvertRGBToHSL(R,G,B,X,Y,Z);
      break;
    }
    case HSVColorspace:
    {
      ConvertRGBToHSV(R,G,B,X,Y,Z);
      break;
    }
    case HWBColorspace:
    {
      ConvertRGBToHWB(R,G,B,X,Y,Z);
      break;
    }
    case JzazbzColorspace:
    {
      ConvertRGBToJzazbz(R,G,B,white_luminance,X,Y,Z);
      break;
    }
    case LabColorspace:
    {
      ConvertRGBToLab(R,G,B,illuminant,X,Y,Z);
      break;
    }
    case LCHColorspace:
    case LCHabColorspace:
    {
      ConvertRGBToLCHab(R,G,B,illuminant,X,Y,Z);
      break;
    }
    case LCHuvColorspace:
    {
      ConvertRGBToLCHuv(R,G,B,illuminant,X,Y,Z);
      break;
    }
    case LMSColorspace:
    {
      ConvertRGBToLMS(R,G,B,X,Y,Z);
      break;
    }
    case LuvColorspace:
    {
      ConvertRGBToLuv(R,G,B,illuminant,X,Y,Z);
      break;
    }
    default:
    {
      ConvertRGBToOklabThroughYCbCr(&input);
      break;
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n v e r t R G B T o H S L                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvertRGBToHSL() transforms a (red, green, blue) to a (hue, saturation,
%  lightness) triple.
%
%  The format of the ConvertRGBToHSL method is:
%
%      void ConvertRGBToHSL(const double red,const double green,
%        const double blue,double *hue,double *saturation,double *lightness)
%
%  A description of each parameter follows:
%
%    o red, green, blue: A Quantum value representing the red, green, and
%      blue component of a pixel..
%
%    o hue, saturation, lightness: A pointer to a double value representing a
%      component of the HSL color space.
%
*/
MagickExport void ConvertRGBToHSL(const double red,const double green,
  const double blue,double *hue,double *saturation,double *lightness)
{
  double
    c,
    max,
    min;

  /*
    Convert RGB to HSL colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(lightness != (double *) NULL);
  max=MagickMax(QuantumScale*red,MagickMax(QuantumScale*green,
    QuantumScale*blue));
  min=MagickMin(QuantumScale*red,MagickMin(QuantumScale*green,
    QuantumScale*blue));
  c=max-min;
  *lightness=(max+min)/2.0;
  if (c <= 0.0)
    {
      *hue=0.0;
      *saturation=0.0;
      return;
    }
  if (fabs(max-QuantumScale*red) < MagickEpsilon)
    {
      *hue=(QuantumScale*green-QuantumScale*blue)/c;
      if ((QuantumScale*green) < (QuantumScale*blue))
        *hue+=6.0;
    }
  else
    if (fabs(max-QuantumScale*green) < MagickEpsilon)
      *hue=2.0+(QuantumScale*blue-QuantumScale*red)/c;
    else
      *hue=4.0+(QuantumScale*red-QuantumScale*green)/c;
  *hue*=60.0/360.0;
  if (*lightness <= 0.5)
    *saturation=c*MagickSafeReciprocal(2.0*(*lightness));
  else
    *saturation=c*MagickSafeReciprocal(2.0-2.0*(*lightness));
}
