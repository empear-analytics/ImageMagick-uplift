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

  MagickCore private colorspace conversion from XYZ to Jzazbz; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_XYZ_TO_JZAZBZ_PRIVATE_H
#define MAGICKCORE_COLORSPACE_XYZ_TO_JZAZBZ_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertXYZToJzazbz(const double X,const double Y,
  const double Z,const double white_luminance,double *Jz,double *az,double *bz)
{
#define Jzazbz_b  (1.15)  /* https://observablehq.com/@jrus/jzazbz */
#define Jzazbz_g  (0.66)  /* refactored by N. Robidoux */
#define Jzazbz_c1  (3424.0/4096.0)
#define Jzazbz_c2  (2413.0/128.0)
#define Jzazbz_c3  (2392.0/128.0)
#define Jzazbz_n  (2610.0/16384.0)
#define Jzazbz_p  (1.7*2523.0/32.0)
#define Jzazbz_d  (-0.56)
#define Jzazbz_d0  (1.6295499532821566e-11)
#define Jzazbz_LX  (0.41478972)
#define Jzazbz_LY  (0.579999)
#define Jzazbz_LZ  (0.0146480)
#define Jzazbz_MX  (-0.2015100)
#define Jzazbz_MY  (1.120649)
#define Jzazbz_MZ  (0.0531008)
#define Jzazbz_SX  (-0.0166008)
#define Jzazbz_SY  (0.264800)
#define Jzazbz_SZ  (0.6684799)
#define Jzazbz_aL  (3.52400)
#define Jzazbz_aM  (-4.066708)
#define Jzazbz_aS  (0.542708)
#define Jzazbz_bL  (0.199076)
#define Jzazbz_bM  (1.096799)
#define Jzazbz_bS  (-1.295875)

  double
    a,
    b,
    dL,
    dM,
    dS,
    gL,
    gM,
    gS,
    nL,
    nM,
    nS,
    Iz,
    J,
    JdI,
    L,
    Lp,
    M,
    Mp,
    S,
    Sp,
    WLr,
    Xp,
    Yp;

  WLr=MagickSafeReciprocal(white_luminance);
  Xp=Z+Jzazbz_b*(X-Z); /* If X and Z are vectorized, better done */
  Yp=X+Jzazbz_g*(Y-X); /* as Xp=Jzazbz_b*X+(1.0-Jzazbz_b)*Z; etc. */
  L=Jzazbz_LZ*Z;
  M=Jzazbz_MZ*Z;
  S=Jzazbz_SZ*Z;
  L+=Jzazbz_LX*Xp;
  M+=Jzazbz_MX*Xp;
  S+=Jzazbz_SX*Xp;
  L+=Jzazbz_LY*Yp;
  M+=Jzazbz_MY*Yp;
  S+=Jzazbz_SY*Yp;
  gL=pow(L*WLr,Jzazbz_n);
  gM=pow(M*WLr,Jzazbz_n);
  gS=pow(S*WLr,Jzazbz_n);
  nL=Jzazbz_c1+Jzazbz_c2*gL;
  nM=Jzazbz_c1+Jzazbz_c2*gM;
  nS=Jzazbz_c1+Jzazbz_c2*gS;
  dL=1.0+Jzazbz_c3*gL;
  dM=1.0+Jzazbz_c3*gM;
  dS=1.0+Jzazbz_c3*gS;
  Lp=pow(nL/dL,Jzazbz_p);
  Mp=pow(nM/dM,Jzazbz_p);
  Sp=pow(nS/dS,Jzazbz_p);
  Iz=(Lp+Mp)*0.5;
  JdI=Jzazbz_d*Iz;
  J=(JdI+Iz)/(JdI+1.0)-Jzazbz_d0;
  a=0.5+Jzazbz_aL*Lp;
  b=0.5+Jzazbz_bL*Lp;
  a+=Jzazbz_aM*Mp;
  b+=Jzazbz_bM*Mp;
  a+=Jzazbz_aS*Sp;
  b+=Jzazbz_bS*Sp;
  *Jz=IsNaN(J) != 0 ? 0.0 : J;
  *az=IsNaN(a) != 0 ? 0.5 : a;
  *bz=IsNaN(b) != 0 ? 0.5 : b;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
