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

  MagickCore private colorspace conversion from RGB to CAT02 LMS; included
  by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_XYZ_CMY_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_XYZ_CMY_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertRGBToCAT02LMS(const double R,const double G,
  const double B,double *L,double *M,double *S)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(R,G,B,&X,&Y,&Z);
  ConvertXYZToCAT02LMS(X,Y,Z,L,M,S);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
