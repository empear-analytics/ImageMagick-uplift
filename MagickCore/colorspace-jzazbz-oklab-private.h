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

  MagickCore private colorspace conversions from Oklab and Oklch to RGB;
  included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_JZAZBZ_OKLAB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_JZAZBZ_OKLAB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _OklabRGBTarget
{
  double
    *red,
    *green,
    *blue;
} OklabRGBTarget;

static inline void ConvertOklabToRGB(const double L,const double a,
  const double b,const OklabRGBTarget *target)
{
#define Oklab_la  (0.3963377774)
#define Oklab_lb  (0.2158037573)
#define Oklab_ma  (-0.1055613458)
#define Oklab_mb  (-0.0638541728)
#define Oklab_sa  (-0.0894841775)
#define Oklab_sb  (-1.2914855480)
#define Oklab_Rl  (4.0767416621)
#define Oklab_Rm  (-3.3077115913)
#define Oklab_Rs  (0.2309699292)
#define Oklab_Gl  (-1.2684380046)
#define Oklab_Gm  (2.6097574011)
#define Oklab_Gs  (-0.3413193965)
#define Oklab_Bl  (-0.0041960863)
#define Oklab_Bm  (-0.7034186147)
#define Oklab_Bs  (1.7076147010)

  double
    *const red = target->red,
    *const green = target->green,
    *const blue = target->blue;

  double
    aa,
    B,
    bb,
    Bl,
    Bm,
    Bs,
    G,
    Gl,
    Gm,
    Gs,
    l,
    m,
    R,
    Rl,
    Rm,
    Rs,
    s;

  aa=a-0.5;
  bb=b-0.5;
  l=L+Oklab_la*aa+Oklab_lb*bb;
  m=L+Oklab_ma*aa+Oklab_mb*bb;
  s=L+Oklab_sa*aa+Oklab_sb*bb;
  l*=l*l;
  m*=m*m;
  s*=s*s;
  Rl=Oklab_Rl*(double) QuantumRange;  /* constant folding */
  Rm=Oklab_Rm*(double) QuantumRange;
  Rs=Oklab_Rs*(double) QuantumRange;
  Gl=Oklab_Gl*(double) QuantumRange;
  Gm=Oklab_Gm*(double) QuantumRange;
  Gs=Oklab_Gs*(double) QuantumRange;
  Bl=Oklab_Bl*(double) QuantumRange;
  Bm=Oklab_Bm*(double) QuantumRange;
  Bs=Oklab_Bs*(double) QuantumRange;
  R=Rl*l+Rm*m+Rs*s;
  G=Gl*l+Gm*m+Gs*s;
  B=Bl*l+Bm*m+Bs*s;
  *red=EncodePixelGamma(R);
  *green=EncodePixelGamma(G);
  *blue=EncodePixelGamma(B);
}

static inline void ConvertOklchToRGB(const double L,const double C,
  const double H,const OklabRGBTarget *target)
{
  double
    *const red = target->red,
    *const green = target->green,
    *const blue = target->blue;

  /* OKLCH → OKLab */
  const double a = C*cos(H);
  const double b_ = C*sin(H);

  /* OKLab → nonlinear LMS */
  const double l_ = L+Oklab_la*a+Oklab_lb*b_;
  const double m_ = L+Oklab_ma*a+Oklab_mb*b_;
  const double s_ = L+Oklab_sa*a+Oklab_sb*b_;

  /* Undo nonlinear transform */
  const double l = l_*l_*l_;
  const double m = m_*m_*m_;
  const double s = s_*s_*s_;

  /* LMS → linear RGB */
  *red=EncodePixelGamma((double) QuantumRange*(Oklab_Rl*l+Oklab_Rm*m+
    Oklab_Rs*s));
  *green=EncodePixelGamma((double) QuantumRange*(Oklab_Gl*l+Oklab_Gm*m+
    Oklab_Gs*s));
  *blue=EncodePixelGamma((double) QuantumRange*(Oklab_Bl*l+Oklab_Bm*m+
    Oklab_Bs*s));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
