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

  MagickCore private colorspace conversion from Jzazbz to XYZ; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_JZAZBZ_TO_XYZ_PRIVATE_H
#define MAGICKCORE_COLORSPACE_JZAZBZ_TO_XYZ_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define Jzazbz_Ca  (0.138605043271539)
#define Jzazbz_Cb  (0.0580473161561189)
#define Jzazbz_Sa  (-0.0960192420263189)
#define Jzazbz_Sb  (-0.811891896056039)
#define Jzazbz_XL  (1.92422643578761)
#define Jzazbz_XM  (-1.00479231259537)
#define Jzazbz_XS  (0.037651404030618)
#define Jzazbz_YL  (0.350316762094999)
#define Jzazbz_YM  (0.726481193931655)
#define Jzazbz_YS  (-0.065384422948085)
#define Jzazbz_ZL  (-0.0909828109828476)
#define Jzazbz_ZM  (-0.312728290523074)
#define Jzazbz_ZS  (1.52276656130526)
#define mJzazbz_c3  (-2392.0/128.0)

typedef struct _JzazbzToXYZInput
{
  const double
    Jz,
    az,
    bz,
    white_luminance;

  double
    *X,
    *Y,
    *Z;
} JzazbzToXYZInput;

static inline void ConvertJzazbzToXYZ(const JzazbzToXYZInput *jzazbz)
{
  double
    azz,
    bzz,
    C,
    dL,
    dM,
    dS,
    g,
    gL,
    gM,
    gS,
    Jnr,
    Jpr,
    L,
    Lp,
    M,
    Mp,
    S,
    Sp,
    nL,
    nM,
    nS,
    Xp,
    Zp,
    Yp;

  g=jzazbz->Jz+Jzazbz_d0;
  azz=jzazbz->az-0.5;
  bzz=jzazbz->bz-0.5;
  C=Jzazbz_Ca*azz+Jzazbz_Cb*bzz;
  Sp=g/(1.0+Jzazbz_d*(1.0-g));
  Lp=Sp+C;
  Mp=Sp-C;
  Sp+=Jzazbz_Sa*azz;
  Sp+=Jzazbz_Sb*bzz;
  Jpr=1.0/Jzazbz_p;
  gL=pow(Lp,Jpr);
  gM=pow(Mp,Jpr);
  gS=pow(Sp,Jpr);
  Jnr=1.0/Jzazbz_n;
  nL=gL-Jzazbz_c1;
  nM=gM-Jzazbz_c1;
  nS=gS-Jzazbz_c1;
  dL=Jzazbz_c2+mJzazbz_c3*gL;
  dM=Jzazbz_c2+mJzazbz_c3*gM;
  dS=Jzazbz_c2+mJzazbz_c3*gS;
  L=pow(nL/dL,Jnr);
  M=pow(nM/dM,Jnr);
  S=pow(nS/dS,Jnr);
  L*=jzazbz->white_luminance;
  M*=jzazbz->white_luminance;
  S*=jzazbz->white_luminance;
  Zp=Jzazbz_ZL*L;
  Xp=Jzazbz_XL*L;
  Yp=Jzazbz_YL*L;
  Zp+=Jzazbz_ZM*M;
  Xp+=Jzazbz_XM*M;
  Yp+=Jzazbz_YM*M;
  Zp+=Jzazbz_ZS*S;
  Xp+=Jzazbz_XS*S;
  Yp+=Jzazbz_YS*S;
  Zp=IsNaN(Zp) != 0 ? 0.0 : Zp;
  Xp=Zp+(Xp-Zp)/Jzazbz_b;  /* If Xp and Zp are vectorized, better done */
  Xp=IsNaN(Xp) != 0 ? 0.0 : Xp;  /* as rJzazbz_b = 1.0/Jzazbz_b; */
  Yp=Xp+(Yp-Xp)/Jzazbz_g;  /* Xp=rJzazbz_b*Xp+(1.0-rJzazbz_b)*Zp; etc. */
  Yp=IsNaN(Yp) != 0 ? 0.0 : Yp;
  *jzazbz->Z=Zp;
  *jzazbz->X=Xp;
  *jzazbz->Y=Yp;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
