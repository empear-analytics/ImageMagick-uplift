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

  MagickCore private colorspace conversions to XYZ: Luv and LCHuv;
  included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_LUV_LMS_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_LUV_LMS_TO_RGB_PRIVATE_H

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
