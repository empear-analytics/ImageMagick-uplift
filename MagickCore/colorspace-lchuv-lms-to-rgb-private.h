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

  MagickCore private colorspace conversions from LCHuv to RGB, and from LMS
  to XYZ and RGB; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_LCHUV_LMS_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_LCHUV_LMS_TO_RGB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertLCHuvToRGB(const double luma,const double chroma,
  const double hue,const IlluminantType illuminant,double *red,double *green,
  double *blue)
{
  double
    X,
    Y,
    Z;

  const IlluminantRGBTarget
    rgb = { red, green, blue };

  const LuvXYZTarget
    xyz = { illuminant, (&X), (&Y), (&Z) };

  /*
    Convert LCHuv to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  ConvertLCHuvToXYZ(100.0*luma,255.0*(chroma-0.5),360.0*hue,&xyz);
  ConvertXYZToRGB(X,Y,Z,&rgb);
}

static inline void ConvertLMSToXYZ(const double L,const double M,const double S,
  double *X,double *Y,double *Z)
{
  *X=1.096123820835514*L-0.278869000218287*M+0.182745179382773*S;
  *Y=0.454369041975359*L+0.473533154307412*M+0.072097803717229*S;
  *Z=(-0.009627608738429)*L-0.005698031216113*M+1.015325639954543*S;
}

static inline void ConvertLMSToRGB(const double L,const double M,
  const double S,double *red,double *green,double *blue)
{
  double
    X,
    Y,
    Z;

  const IlluminantRGBTarget
    rgb = { red, green, blue };

  ConvertLMSToXYZ(L,M,S,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,&rgb);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
