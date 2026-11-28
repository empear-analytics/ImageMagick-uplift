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

  MagickCore private colorspace conversion from Jzazbz to RGB; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_JZAZBZ_PRIVATE_H
#define MAGICKCORE_COLORSPACE_JZAZBZ_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _JzazbzToRGBInput
{
  const double
    Jz,
    az,
    bz,
    white_luminance;

  double
    *red,
    *green,
    *blue;
} JzazbzToRGBInput;

static inline void ConvertJzazbzToRGB(const JzazbzToRGBInput *jzazbz)
{
  double
    X,
    Y,
    Z;

  {
    const JzazbzToXYZInput
      xyz = { jzazbz->Jz, jzazbz->az, jzazbz->bz, jzazbz->white_luminance,
        (&X), (&Y), (&Z) };

    ConvertJzazbzToXYZ(&xyz);
  }
  {
    const IlluminantRGBTarget
      rgb = { jzazbz->red, jzazbz->blue, jzazbz->green };

    ConvertXYZToRGB(X,Y,Z,&rgb);
  }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
