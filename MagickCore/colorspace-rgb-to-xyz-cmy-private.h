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

  MagickCore private colorspace conversions from RGB and XYZ: XYZ, CAT02
  LMS and CMY; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_XYZ_CMY_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_XYZ_CMY_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertXYZToCAT02LMS(const double X,const double Y,
  const double Z,double *L,double *M,double *S)
{
  *L=0.7328*X+0.4296*Y-0.1624*Z;
  *M=(-0.7036)*X+1.6975*Y+0.0061*Z;
  *S=0.0030*X+0.0136*Y+0.9834*Z;
}

static inline void ConvertRGBToXYZ(const double red,const double green,
  const double blue,double *X,double *Y,double *Z)
{
  double
    b,
    g,
    r;

  /*
    Convert RGB to XYZ colorspace.
  */
  r=QuantumScale*DecodePixelGamma(red);
  g=QuantumScale*DecodePixelGamma(green);
  b=QuantumScale*DecodePixelGamma(blue);
  *X=(0.4123955889674142161*r)+(0.3575834307637148171*g)+
    (0.1804926473817015735*b);
  *Y=(0.2125862307855955516*r)+(0.7151703037034108499*g)+
    (0.07220049864333622685*b);
  *Z=(0.01929721549174694484*r)+(0.1191838645808485318*g)+
    (0.9504971251315797660*b);
}

static inline void ConvertRGBToCAT02LMS(const double R,const double G,
  const double B,double *L,double *M,double *S)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(R,G,B,&X,&Y,&Z);
  ConvertXYZToCAT02LMS(X,Y,Z,L,M,S);
}

static inline void ConvertRGBToCMY(const double red,const double green,
  const double blue,double *cyan,double *magenta,double *yellow)
{
  *cyan=QuantumScale*((double) QuantumRange-red);
  *magenta=QuantumScale*((double) QuantumRange-green);
  *yellow=QuantumScale*((double) QuantumRange-blue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
