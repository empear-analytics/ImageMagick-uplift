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

  MagickCore private colorspace conversion from RGB to CMY; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_CMY_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_CMY_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertRGBToCMY(const double red,const double green,
  const double blue,double *cyan,double *magenta,double *yellow)
{
  *cyan=QuantumScale*((double) QuantumRange-red);
  *magenta=QuantumScale*((double) QuantumRange-green);
  *yellow=QuantumScale*((double) QuantumRange-blue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
