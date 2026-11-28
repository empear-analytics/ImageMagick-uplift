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

  MagickCore private colorspace conversion from RGB to Jzazbz; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_JZAZBZ_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_JZAZBZ_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _RGBToJzazbzInput
{
  const double
    red,
    green,
    blue,
    white_luminance;
} RGBToJzazbzInput;

static inline void ConvertRGBToJzazbz(const RGBToJzazbzInput *rgb,double *Jz,
  double *az,double *bz)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(rgb->red,rgb->blue,rgb->green,&X,&Y,&Z);
  {
    const XYZToJzazbzInput
      xyz = { X, Y, Z, rgb->white_luminance };

    ConvertXYZToJzazbz(&xyz,Jz,az,bz);
  }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
