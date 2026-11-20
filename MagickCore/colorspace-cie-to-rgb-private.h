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

  MagickCore private colorspace conversions to XYZ and RGB: Luv, LCHuv,
  LMS, Display P3 and ProPhoto; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_CIE_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_CIE_TO_RGB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertLuvToXYZ(const double L,const double u,const double v,
  const IlluminantType illuminant,double *X,double *Y,double *Z)
{
  double
    gamma;

  if (L > (CIEK*CIEEpsilon))
    *Y=(double) pow((L+16.0)/116.0,3.0);
  else
    *Y=L/CIEK;
  gamma=MagickSafeReciprocal((((52.0*L*MagickSafeReciprocal(u+13.0*L*
    (4.0*illuminant_tristimulus[illuminant].x/
    (illuminant_tristimulus[illuminant].x+15.0*
    illuminant_tristimulus[illuminant].y+3.0*
    illuminant_tristimulus[illuminant].z))))-1.0)/3.0)-(-1.0/3.0));
  *X=gamma*((*Y*((39.0*L*MagickSafeReciprocal(v+13.0*L*(9.0*
    illuminant_tristimulus[illuminant].y/
    (illuminant_tristimulus[illuminant].x+15.0*
    illuminant_tristimulus[illuminant].y+3.0*
    illuminant_tristimulus[illuminant].z))))-5.0))+5.0*(*Y));
  *Z=(*X*(((52.0*L*MagickSafeReciprocal(u+13.0*L*(4.0*
    illuminant_tristimulus[illuminant].x/
    (illuminant_tristimulus[illuminant].x+15.0*
    illuminant_tristimulus[illuminant].y+3.0*
    illuminant_tristimulus[illuminant].z))))-1.0)/3.0))-5.0*(*Y);
}

static inline void ConvertLCHuvToXYZ(const double luma,const double chroma,
  const double hue,const IlluminantType illuminant,double *X,double *Y,
  double *Z)
{
  ConvertLuvToXYZ(luma,chroma*cos(DegreesToRadians(hue)),chroma*
    sin(DegreesToRadians(hue)),illuminant,X,Y,Z);
}

static inline void ConvertLCHuvToRGB(const double luma,const double chroma,
  const double hue,const IlluminantType illuminant,double *red,double *green,
  double *blue)
{
  double
    X,
    Y,
    Z;

  /*
    Convert LCHuv to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  ConvertLCHuvToXYZ(100.0*luma,255.0*(chroma-0.5),360.0*hue,illuminant,
    &X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,red,green,blue);
}

static inline void ConvertLMSToXYZ(const double L,const double M,const double S,
  double *X,double *Y,double *Z)
{
  *X=1.096123820835514*L-0.278869000218287*M+0.182745179382773*S;
  *Y=0.454369041975359*L+0.473533154307412*M+0.072097803717229*S;
  *Z=(-0.009627608738429)*L-0.005698031216113*M+1.015325639954543*S;
}

static inline void ConvertLMSToRGB(const double L,const double M,
  const double S,double *red,double *green,double *blue)
{
  double
    X,
    Y,
    Z;

  ConvertLMSToXYZ(L,M,S,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,red,green,blue);
}

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
