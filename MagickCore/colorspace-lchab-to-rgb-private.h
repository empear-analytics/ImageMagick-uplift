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

  MagickCore private colorspace conversion from LCHab to RGB; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_LCHAB_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_LCHAB_TO_RGB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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
