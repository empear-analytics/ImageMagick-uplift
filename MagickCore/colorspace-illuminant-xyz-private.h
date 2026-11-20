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

  MagickCore private colorspace constants and conversions: the illuminant
  constants and table, Adobe98 to XYZ and XYZ to RGB; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_ILLUMINANT_XYZ_PRIVATE_H
#define MAGICKCORE_COLORSPACE_ILLUMINANT_XYZ_PRIVATE_H

#define IlluminantX  0.95047
#define IlluminantY  1.0
#define IlluminantZ  1.08883
#define CIEEpsilon  (216.0/24389.0)
#define CIEK  (24389.0/27.0)

static const PrimaryInfo
  illuminant_tristimulus[] =
  {
    { 1.09850, 1.00000, 0.35585 },  /* A */
    { 0.99072, 1.00000, 0.85223 },  /* B */
    { 0.98074, 1.00000, 1.18232 },  /* C */
    { 0.96422, 1.00000, 0.82521 },  /* D50 */
    { 0.95682, 1.00000, 0.92149 },  /* D55 */
    { 0.95047, 1.00000, 1.08883 },  /* D65 */
    { 0.94972, 1.00000, 1.22638 },  /* D75 */
    { 1.00000, 1.00000, 1.00000 },  /* E */
    { 0.99186, 1.00000, 0.67393 },  /* F2 */
    { 0.95041, 1.00000, 1.08747 },  /* F7 */
    { 1.00962, 1.00000, 0.64350 }   /* F11 */
  };

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertAdobe98ToXYZ(const double red,const double green,
  const double blue,double *X,double *Y,double *Z)
{
  double
    b,
    g,
    r;

  /*
    Convert Adobe '98 to XYZ colorspace.
  */
  r=QuantumScale*DecodePixelGamma((double) QuantumRange*red);
  g=QuantumScale*DecodePixelGamma((double) QuantumRange*green);
  b=QuantumScale*DecodePixelGamma((double) QuantumRange*blue);
  *X=0.57666904291013050*r+0.18555823790654630*g+0.18822864623499470*b;
  *Y=0.29734497525053605*r+0.62736356625546610*g+0.07529145849399788*b;
  *Z=0.02703136138641234*r+0.07068885253582723*g+0.99133753683763880*b;
}

static inline void ConvertXYZToRGB(const double X,const double Y,const double Z,
  double *red,double *green,double *blue)
{
  double
    b,
    g,
    min,
    r;

  r=(3.240969941904521*X)+(-1.537383177570093*Y)+(-0.498610760293*Z);
  g=(-0.96924363628087*X)+(1.87596750150772*Y)+(0.041555057407175*Z);
  b=(0.055630079696993*X)+(-0.20397695888897*Y)+(1.056971514242878*Z);
  min=MagickMin(r,MagickMin(g,b));
  if (min < 0.0)
    {
      r-=min;
      g-=min;
      b-=min;
    }
  *red=EncodePixelGamma((double) QuantumRange*r);
  *green=EncodePixelGamma((double) QuantumRange*g);
  *blue=EncodePixelGamma((double) QuantumRange*b);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
