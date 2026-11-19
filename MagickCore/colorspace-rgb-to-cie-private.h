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

  MagickCore private colorspace conversions from RGB and XYZ: Lab, LCHab,
  Luv, LCHuv, ProPhoto, LMS and xyY; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_CIE_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_CIE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertXYZToLab(const double X,const double Y,const double Z,
  const IlluminantType illuminant,double *L,double *a,double *b)
{
  double
    x,
    y,
    z;

  if ((X/illuminant_tristimulus[illuminant].x) > CIEEpsilon)
    x=pow(X/illuminant_tristimulus[illuminant].x,1.0/3.0);
  else
    x=(CIEK*X/illuminant_tristimulus[illuminant].x+16.0)/116.0;
  if ((Y/illuminant_tristimulus[illuminant].y) > CIEEpsilon)
    y=pow(Y/illuminant_tristimulus[illuminant].y,1.0/3.0);
  else
    y=(CIEK*Y/illuminant_tristimulus[illuminant].y+16.0)/116.0;
  if ((Z/illuminant_tristimulus[illuminant].z) > CIEEpsilon)
    z=pow(Z/illuminant_tristimulus[illuminant].z,1.0/3.0);
  else
    z=(CIEK*Z/illuminant_tristimulus[illuminant].z+16.0)/116.0;
  *L=((116.0*y)-16.0)/100.0;
  *a=(500.0*(x-y))/255.0+0.5;
  *b=(200.0*(y-z))/255.0+0.5;
}

static inline void ConvertRGBToLab(const double red,const double green,
  const double blue,const IlluminantType illuminant,double *L,double *a,
  double *b)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  ConvertXYZToLab(X,Y,Z,illuminant,L,a,b);
}

static inline void ConvertXYZToLCHab(const double X,const double Y,
  const double Z,const IlluminantType illuminant,double *luma,double *chroma,
  double *hue)
{
  double
    a,
    b;

  ConvertXYZToLab(X,Y,Z,illuminant,luma,&a,&b);
  *chroma=hypot(a-0.5,b-0.5)/1.0+0.5;
  *hue=180.0*atan2(b-0.5,a-0.5)/MagickPI/360.0;
  if (*hue < 0.0)
    *hue+=1.0;
}

static inline void ConvertRGBToLCHab(const double red,const double green,
  const double blue,const IlluminantType illuminant,double *luma,double *chroma,
  double *hue)
{
  double
    X,
    Y,
    Z;

  /*
    Convert RGB to LCHab colorspace.
  */
  assert(luma != (double *) NULL);
  assert(chroma != (double *) NULL);
  assert(hue != (double *) NULL);
  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  ConvertXYZToLCHab(X,Y,Z,illuminant,luma,chroma,hue);
}

static inline void ConvertXYZToLuv(const double X,const double Y,const double Z,
  const IlluminantType illuminant,double *L,double *u,double *v)
{
  double
    alpha;

  if ((Y/illuminant_tristimulus[illuminant].y) > CIEEpsilon)
    *L=(double) (116.0*pow(Y/illuminant_tristimulus[illuminant].y,
      1.0/3.0)-16.0);
  else
    *L=CIEK*(Y/illuminant_tristimulus[illuminant].y);
  alpha=MagickSafeReciprocal(X+15.0*Y+3.0*Z);
  *u=13.0*(*L)*((4.0*alpha*X)-(4.0*illuminant_tristimulus[illuminant].x/
    (illuminant_tristimulus[illuminant].x+15.0*
    illuminant_tristimulus[illuminant].y+3.0*
    illuminant_tristimulus[illuminant].z)));
  *v=13.0*(*L)*((9.0*alpha*Y)-(9.0*illuminant_tristimulus[illuminant].y/
    (illuminant_tristimulus[illuminant].x+15.0*
    illuminant_tristimulus[illuminant].y+3.0*
    illuminant_tristimulus[illuminant].z)));
  *L/=100.0;
  *u=(*u+134.0)/354.0;
  *v=(*v+140.0)/262.0;
}

static inline void ConvertXYZToLCHuv(const double X,const double Y,
  const double Z,const IlluminantType illuminant,double *luma,double *chroma,
  double *hue)
{
  double
    u,
    v;

  ConvertXYZToLuv(X,Y,Z,illuminant,luma,&u,&v);
  *chroma=hypot(354.0*u-134.0,262.0*v-140.0)/255.0+0.5;
  *hue=180.0*atan2(262.0*v-140.0,354.0*u-134.0)/MagickPI/360.0;
  if (*hue < 0.0)
    *hue+=1.0;
}

static inline void ConvertRGBToLCHuv(const double red,const double green,
  const double blue,const IlluminantType illuminant,double *luma,double *chroma,
  double *hue)
{
  double
    X,
    Y,
    Z;

  /*
    Convert RGB to LCHuv colorspace.
  */
  assert(luma != (double *) NULL);
  assert(chroma != (double *) NULL);
  assert(hue != (double *) NULL);
  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  ConvertXYZToLCHuv(X,Y,Z,illuminant,luma,chroma,hue);
}

static inline void ConvertXYZToProPhoto(const double X,const double Y,
  const double Z,double *red,double *green,double *blue)
{
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
  ConvertXYZToProPhoto(X,Y,Z,r,g,b);
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
