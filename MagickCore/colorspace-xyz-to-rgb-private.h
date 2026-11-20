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

static inline void ConvertAdobe98ToRGB(const double r,const double g,
  const double b,double *red,double *green,double *blue)
{
  double
    X,
    Y,
    Z;

  ConvertAdobe98ToXYZ(r,g,b,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,red,green,blue);
}

static inline void ConvertCAT02LMSToXYZ(const double L,const double M,
  const double S,double *X,double *Y,double *Z)
{
  /*
    Convert CAT02LMS to XYZ colorspace.
  */
  *X=1.096123820835514*L-0.278869000218287*M+0.182745179382773*S;
  *Y=0.454369041975359*L+0.473533154307412*M+0.072097803717229*S;
  *Z=(-0.009627608738429)*L-0.005698031216113*M+1.015325639954543*S;
}

static inline void ConvertCAT02LMSToRGB(const double L,const double M,
  const double S,double *R,double *G,double *B)
{
  double
    X,
    Y,
    Z;

  ConvertCAT02LMSToXYZ(L,M,S,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,R,G,B);
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
  const double yellow,double *red,double *green,double *blue)
{
	*red=(double) QuantumRange*(1.0-cyan);
  *green=(double) QuantumRange*(1.0-magenta);
  *blue=(double) QuantumRange*(1.0-yellow);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
