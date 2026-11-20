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

  MagickCore private colorspace conversion from RGB to CMYK; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_LUMA_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_LUMA_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertRGBToCMYK(PixelInfo *pixel)
{
  MagickRealType
    black,
    blue,
    cyan,
    green,
    magenta,
    red,
    yellow;

  if (pixel->colorspace != sRGBColorspace)
    {
      red=QuantumScale*pixel->red;
      green=QuantumScale*pixel->green;
      blue=QuantumScale*pixel->blue;
    }
  else
    {
      red=QuantumScale*DecodePixelGamma(pixel->red);
      green=QuantumScale*DecodePixelGamma(pixel->green);
      blue=QuantumScale*DecodePixelGamma(pixel->blue);
    }
  if ((fabs((double) red) < MagickEpsilon) &&
      (fabs((double) green) < MagickEpsilon) &&
      (fabs((double) blue) < MagickEpsilon))
    {
      pixel->black=(MagickRealType) QuantumRange;
      return;
    }
  cyan=(MagickRealType) (1.0-red);
  magenta=(MagickRealType) (1.0-green);
  yellow=(MagickRealType) (1.0-blue);
  black=cyan;
  if (magenta < black)
    black=magenta;
  if (yellow < black)
    black=yellow;
  cyan=(MagickRealType) (MagickSafeReciprocal(1.0-black)*(cyan-black));
  magenta=(MagickRealType) (MagickSafeReciprocal(1.0-black)*(magenta-black));
  yellow=(MagickRealType) (MagickSafeReciprocal(1.0-black)*(yellow-black));
  pixel->colorspace=CMYKColorspace;
  pixel->red=(MagickRealType) QuantumRange*cyan;
  pixel->green=(MagickRealType) QuantumRange*magenta;
  pixel->blue=(MagickRealType) QuantumRange*yellow;
  pixel->black=(MagickRealType) QuantumRange*black;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
