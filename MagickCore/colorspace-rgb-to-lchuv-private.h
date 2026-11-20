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

  MagickCore private colorspace conversions from RGB and XYZ: Luv and
  LCHuv; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_LCHUV_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_LCHUV_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
