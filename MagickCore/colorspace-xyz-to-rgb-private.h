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

  MagickCore private colorspace conversions to RGB: Adobe98, CAT02 LMS,
  CMYK and CMY; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_XYZ_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_XYZ_TO_RGB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _XYZToRGBTarget
{
  double
    *red,
    *green,
    *blue;
} XYZToRGBTarget;

typedef struct _CAT02LMSXYZTarget
{
  double
    *X,
    *Y,
    *Z;
} CAT02LMSXYZTarget;

typedef struct _CAT02LMSRGBTarget
{
  double
    *R,
    *G,
    *B;
} CAT02LMSRGBTarget;

static inline void ConvertAdobe98ToRGB(const double r,const double g,
  const double b,const XYZToRGBTarget *target)
{
  double
    X,
    Y,
    Z;

  ConvertAdobe98ToXYZ(r,g,b,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,target->red,target->green,target->blue);
}

static inline void ConvertCAT02LMSToXYZ(const double L,const double M,
  const double S,const CAT02LMSXYZTarget *target)
{
  /*
    Convert CAT02LMS to XYZ colorspace.
  */
  *target->X=1.096123820835514*L-0.278869000218287*M+0.182745179382773*S;
  *target->Y=0.454369041975359*L+0.473533154307412*M+0.072097803717229*S;
  *target->Z=(-0.009627608738429)*L-0.005698031216113*M+1.015325639954543*S;
}

static inline void ConvertCAT02LMSToRGB(const double L,const double M,
  const double S,const CAT02LMSRGBTarget *target)
{
  double
    X,
    Y,
    Z;

  const CAT02LMSXYZTarget
    xyz = { (&X), (&Y), (&Z) };

  ConvertCAT02LMSToXYZ(L,M,S,&xyz);
  ConvertXYZToRGB(X,Y,Z,target->R,target->G,target->B);
}

static inline void ConvertCMYKToRGB(PixelInfo *pixel)
{
  pixel->red=(((double) QuantumRange-(QuantumScale*pixel->red*
    ((double) QuantumRange-pixel->black)+pixel->black)));
  pixel->green=(((double) QuantumRange-(QuantumScale*pixel->green*
    ((double) QuantumRange-pixel->black)+pixel->black)));
  pixel->blue=(((double) QuantumRange-(QuantumScale*pixel->blue*
    ((double) QuantumRange-pixel->black)+pixel->black)));
}

static inline void ConvertCMYToRGB(const double cyan,const double magenta,
  const double yellow,const XYZToRGBTarget *target)
{
	*target->red=(double) QuantumRange*(1.0-cyan);
  *target->green=(double) QuantumRange*(1.0-magenta);
  *target->blue=(double) QuantumRange*(1.0-yellow);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
