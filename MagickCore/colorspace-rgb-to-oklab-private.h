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

  MagickCore private colorspace conversions from RGB to Oklab and Oklch;
  included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_OKLAB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_OKLAB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertRGBToOklab(const double red,const double green,
  const double blue,double *L,double *a,double *b)
{
#define Oklab_lR  (0.4122214708)
#define Oklab_lG  (0.5363325363)
#define Oklab_lB  (0.0514459929)
#define Oklab_mR  (0.2119034982)
#define Oklab_mG  (0.6806995451)
#define Oklab_mB  (0.1073969566)
#define Oklab_sR  (0.0883024619)
#define Oklab_sG  (0.2817188376)
#define Oklab_sB  (0.6299787005)
#define Oklab_Ll  (0.2104542553)
#define Oklab_Lm  (0.7936177850)
#define Oklab_Ls  (-0.0040720468)
#define Oklab_al  (1.9779984951)
#define Oklab_am  (-2.4285922050)
#define Oklab_as  (0.4505937099)
#define Oklab_bl  (0.0259040371)
#define Oklab_bm  (0.7827717662)
#define Oklab_bs  (-0.8086757660)

  double
    B,
    G,
    l,
    lR,
    lG,
    lB,
    m,
    mR,
    mG,
    mB,
    R,
    s,
    sR,
    sG,
    sB;

  R=DecodePixelGamma(red);
  G=DecodePixelGamma(green);
  B=DecodePixelGamma(blue);
  lR=Oklab_lR*QuantumScale;  /* constant folding */
  lG=Oklab_lG*QuantumScale;
  lB=Oklab_lB*QuantumScale;
  mR=Oklab_mR*QuantumScale;
  mG=Oklab_mG*QuantumScale;
  mB=Oklab_mB*QuantumScale;
  sR=Oklab_sR*QuantumScale;
  sG=Oklab_sG*QuantumScale;
  sB=Oklab_sB*QuantumScale;
  l=lR*R+lG*G+lB*B;
  m=mR*R+mG*G+mB*B;
  s=sR*R+sG*G+sB*B;
  l=cbrt(l);
  m=cbrt(m);
  s=cbrt(s);
  *L=Oklab_Ll*l+Oklab_Lm*m+Oklab_Ls*s;
  *a=Oklab_al*l+Oklab_am*m+Oklab_as*s+0.5;
  *b=Oklab_bl*l+Oklab_bm*m+Oklab_bs*s+0.5;
}

static inline void ConvertRGBToOklch(const double red,const double green,
  const double blue,double *L,double *C,double *H)
{
  /* Linear RGB → LMS */
  const double l = Oklab_lR*QuantumScale*DecodePixelGamma(red)+
    Oklab_lG*QuantumScale*DecodePixelGamma(green)+
    Oklab_lB*QuantumScale*DecodePixelGamma(blue);
  const double m = Oklab_mR*QuantumScale*DecodePixelGamma(red)+
    Oklab_mG*QuantumScale*DecodePixelGamma(green)+
    Oklab_mB*QuantumScale*DecodePixelGamma(blue);
  const double s = Oklab_sR*QuantumScale*DecodePixelGamma(red)+
    Oklab_sG*QuantumScale*DecodePixelGamma(green)+
    Oklab_sB*QuantumScale*DecodePixelGamma(blue);

  /* Nonlinear transform */
  const double l_ = cbrt(l);
  const double m_ = cbrt(m);
  const double s_ = cbrt(s);

  /* LMS → OKLab */
  const double L_ok = Oklab_Ll*l_+Oklab_Lm*m_+Oklab_Ls*s_;
  const double a_ok = Oklab_al*l_+Oklab_am*m_+Oklab_as*s_;
  const double b_ok = Oklab_bl*l_+Oklab_bm*m_+Oklab_bs*s_;

  /* OKLab → OKLCH */
  const double C_ok = sqrt(a_ok*a_ok+b_ok*b_ok);
  double H_ok = atan2(b_ok,a_ok);  /* radians */

  if (H_ok < 0.0)
    H_ok+=2.0*MagickPI;
  *L=L_ok;
  *C=C_ok;
  *H=H_ok;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
