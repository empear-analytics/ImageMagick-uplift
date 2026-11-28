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

  MagickCore private colorspace conversion from XYZ to Adobe98; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_HUE_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_HUE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _XYZToAdobe98Input
{
  double
    X,
    Y,
    Z;
} XYZToAdobe98Input;

static inline void ConvertXYZToAdobe98(const XYZToAdobe98Input *xyz,
  double *red,double *green,double *blue)
{
  const double
    X = xyz->X,
    Y = xyz->Y,
    Z = xyz->Z;

  double
    b,
    g,
    r;

  r=2.041587903810746500*X-0.56500697427885960*Y-0.34473135077832956*Z;
  g=(-0.969243636280879500)*X+1.87596750150772020*Y+0.04155505740717557*Z;
  b=0.013444280632031142*X-0.11836239223101838*Y+1.01517499439120540*Z;
  *red=QuantumScale*EncodePixelGamma((double) QuantumRange*r);
  *green=QuantumScale*EncodePixelGamma((double) QuantumRange*g);
  *blue=QuantumScale*EncodePixelGamma((double) QuantumRange*b);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
