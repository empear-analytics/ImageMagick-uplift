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

  MagickCore private X11 window image methods.
*/
#ifndef MAGICKCORE_XWINDOW_IMAGE_PRIVATE_H
#define MAGICKCORE_XWINDOW_IMAGE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define XBlueGamma(color) ClampToQuantum((*XBlueGammaReference()) == 1.0 ? (double) \
  (color) : ((pow((QuantumScale*(double) (color)),1.0* \
  MagickSafeReciprocal((double) (*XBlueGammaReference())))*(double) QuantumRange)))
#define XGreenGamma(color) ClampToQuantum((*XGreenGammaReference()) == 1.0 ? (double) \
  (color) : ((pow((QuantumScale*(double) (color)),1.0* \
  MagickSafeReciprocal((double) (*XGreenGammaReference())))*(double) QuantumRange)))
#define XRedGamma(color) ClampToQuantum((*XRedGammaReference()) == 1.0 ? (double) \
  (color) : ((pow((QuantumScale*(double) (color)),1.0* \
  MagickSafeReciprocal((double) (*XRedGammaReference())))*(double) QuantumRange)))
#define XStandardPixel(map,color)  (size_t) (map->base_pixel+ \
  (((color)->red*map->red_max/65535L)*map->red_mult)+ \
  (((color)->green*map->green_max/65535L)*map->green_mult)+ \
  (((color)->blue*map->blue_max/65535L)*map->blue_mult))

extern MagickPrivate double
  *XBlueGammaReference(void);
extern MagickPrivate double
  *XGreenGammaReference(void);
extern MagickPrivate double
  *XRedGammaReference(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
