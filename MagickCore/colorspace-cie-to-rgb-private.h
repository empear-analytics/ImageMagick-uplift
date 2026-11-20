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

  MagickCore private colorspace conversions to XYZ and RGB: Display P3,
  ProPhoto, and Luv to RGB; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_CIE_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_CIE_TO_RGB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertDisplayP3ToXYZ(const double red,const double green,
  const double blue,double *X,double *Y,double *Z)
{
  double
    b,
    g,
    r;

  /*
    Convert Display P3 to XYZ colorspace.
  */
  r=QuantumScale*DecodePixelGamma((double) QuantumRange*red);
  g=QuantumScale*DecodePixelGamma((double) QuantumRange*green);
  b=QuantumScale*DecodePixelGamma((double) QuantumRange*blue);
  *X=0.4865709486482162*r+0.26566769316909306*g+0.1982172852343625*b;
  *Y=0.2289745640697488*r+0.69173852183650640*g+0.0792869140937450*b;
  *Z=0.0000000000000000*r+0.04511338185890264*g+1.0439443689009760*b;
}

static inline void ConvertDisplayP3ToRGB(const double r,const double g,
  const double b,double *red,double *green,double *blue)
{
  double
    X,
    Y,
    Z;

  ConvertDisplayP3ToXYZ(r,g,b,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,red,green,blue);
}

static inline void ConvertLuvToRGB(const double L,const double u,
  const double v,const IlluminantType illuminant,double *red,double *green,
  double *blue)
{
  double
    X,
    Y,
    Z;

  ConvertLuvToXYZ(100.0*L,354.0*u-134.0,262.0*v-140.0,illuminant,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,red,green,blue);
}

static inline void ConvertProPhotoToXYZ(const double red,const double green,
  const double blue,double *X,double *Y,double *Z)
{
  double
    b,
    g,
    r;

  /*
    Convert ProPhoto to XYZ colorspace.
  */
  r=QuantumScale*DecodePixelGamma((double) QuantumRange*red);
  g=QuantumScale*DecodePixelGamma((double) QuantumRange*green);
  b=QuantumScale*DecodePixelGamma((double) QuantumRange*blue);
  *X=0.4865709486482162*r+0.26566769316909306*g+0.1982172852343625*b;
  *X=0.7977604896723027*r+0.13518583717574031*g+0.03134934958152480000*b;
  *Y=0.2880711282292934*r+0.71184321781010140*g+0.00008565396060525902*b;
  *Z=0.0000000000000000*r+0.00000000000000000*g+0.82510460251046010000*b;
}

static inline void ConvertProPhotoToRGB(const double r,const double g,
  const double b,double *red,double *green,double *blue)
{
  double
    X,
    Y,
    Z;

  ConvertProPhotoToXYZ(r,g,b,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,red,green,blue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
