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

  MagickCore private colorspace conversions from RGB and XYZ: ProPhoto, LMS,
  Luv and xyY; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_CIE_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_CIE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _ConvertXYZInput
{
  double
    X,
    Y,
    Z;
} ConvertXYZInput;

static inline void ConvertXYZToProPhoto(const ConvertXYZInput *xyz,
  double *red,double *green,double *blue)
{
  const double
    X = xyz->X,
    Y = xyz->Y,
    Z = xyz->Z;

  double
    b,
    g,
    r;

  r=1.3457989731028281*X-0.25558010007997534*Y-0.05110628506753401*Z;
  g=(-0.5446224939028347)*X+1.50823274131327810*Y+0.02053603239147973*Z;
  b=0.0000000000000000*X+0.0000000000000000*Y+1.21196754563894540*Z;
  *red=QuantumScale*EncodePixelGamma((double) QuantumRange*r);
  *green=QuantumScale*EncodePixelGamma((double) QuantumRange*g);
  *blue=QuantumScale*EncodePixelGamma((double) QuantumRange*b);
}

static inline void ConvertRGBToProPhoto(const double red,const double green,
  const double blue,double *r,double *g,double *b)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  {
    const ConvertXYZInput
      xyz = { X, Y, Z };

    ConvertXYZToProPhoto(&xyz,r,g,b);
  }
}

static inline void ConvertXYZToLMS(const double x,const double y,
  const double z,double *L,double *M,double *S)
{
  *L=0.7328*x+0.4296*y-0.1624*z;
  *M=(-0.7036*x+1.6975*y+0.0061*z);
  *S=0.0030*x+0.0136*y+0.9834*z;
}

static inline void ConvertRGBToLMS(const double red,const double green,
  const double blue,double *L,double *M,double *S)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  ConvertXYZToLMS(X,Y,Z,L,M,S);
}

static inline void ConvertRGBToLuv(const double red,const double green,
  const double blue,const IlluminantType illuminant,double *L,double *u,
  double *v)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  ConvertXYZToLuv(X,Y,Z,illuminant,L,u,v);
}

static inline void ConvertRGBToxyY(const double red,const double green,
  const double blue,double *low_x,double *low_y,double *cap_Y)
{
  double
    gamma,
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  gamma=MagickSafeReciprocal(X+Y+Z);
  *low_x=gamma*X;
  *low_y=gamma*Y;
  *cap_Y=Y;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
