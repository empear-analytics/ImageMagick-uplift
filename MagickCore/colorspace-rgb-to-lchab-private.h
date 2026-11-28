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

  MagickCore private colorspace conversions from RGB to Lab and from RGB and
  XYZ to LCHab; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_LCHAB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_LCHAB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertRGBToLab(const double red,const double green,
  const double blue,const IlluminantType illuminant,double *L,double *a,
  double *b)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  {
    const LabXYZInput
      xyz = { X, Y, Z, illuminant };

    ConvertXYZToLab(&xyz,L,a,b);
  }
}

static inline void ConvertXYZToLCHab(const LabXYZInput *xyz,double *luma,
  double *chroma,double *hue)
{
  double
    a,
    b;

  ConvertXYZToLab(xyz,luma,&a,&b);
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
  {
    const LabXYZInput
      xyz = { X, Y, Z, illuminant };

    ConvertXYZToLCHab(&xyz,luma,chroma,hue);
  }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
