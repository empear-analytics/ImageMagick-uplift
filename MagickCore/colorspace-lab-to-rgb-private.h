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

  MagickCore private colorspace conversions to XYZ and RGB: Lab and LCHab;
  included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_LAB_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_LAB_TO_RGB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertLabToXYZ(const double L,const double a,const double b,
  const IlluminantType illuminant,double *X,double *Y,double *Z)
{
  double
    x,
    y,
    z;

  y=(L+16.0)/116.0;
  x=y+a/500.0;
  z=y-b/200.0;
  if ((x*x*x) > CIEEpsilon)
    x=(x*x*x);
  else
    x=(116.0*x-16.0)/CIEK;
  if (L > (CIEK*CIEEpsilon))
    y=(y*y*y);
  else
    y=L/CIEK;
  if ((z*z*z) > CIEEpsilon)
    z=(z*z*z);
  else
    z=(116.0*z-16.0)/CIEK;
  *X=illuminant_tristimulus[illuminant].x*x;
  *Y=illuminant_tristimulus[illuminant].y*y;
  *Z=illuminant_tristimulus[illuminant].z*z;
}

static inline void ConvertLabToRGB(const double L,const double a,
  const double b,const IlluminantType illuminant,double *red,double *green,
  double *blue)
{
  double
    X,
    Y,
    Z;

  const IlluminantRGBTarget
    rgb = { red, green, blue };

  ConvertLabToXYZ(100.0*L,255.0*(a-0.5),255.0*(b-0.5),illuminant,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,&rgb);
}

static inline void ConvertLCHabToXYZ(const double luma,const double chroma,
  const double hue,const IlluminantType illuminant,double *X,double *Y,
  double *Z)
{
  ConvertLabToXYZ(luma,chroma*cos(DegreesToRadians(hue)),chroma*
    sin(DegreesToRadians(hue)),illuminant,X,Y,Z);
}

static inline void ConvertLCHabToRGB(const double luma,const double chroma,
  const double hue,const IlluminantType illuminant,double *red,double *green,
  double *blue)
{
  double
    X,
    Y,
    Z;

  const IlluminantRGBTarget
    rgb = { red, green, blue };

  /*
    Convert LCHab to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  ConvertLCHabToXYZ(100.0*luma,255.0*(chroma-0.5),360.0*hue,illuminant,
    &X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,&rgb);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
