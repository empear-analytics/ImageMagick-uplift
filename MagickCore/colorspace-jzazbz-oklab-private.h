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

  MagickCore private colorspace conversions between RGB, XYZ, Jzazbz,
  Oklab and Oklch; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_JZAZBZ_OKLAB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_JZAZBZ_OKLAB_PRIVATE_H

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

static inline void ConvertRGBToJzazbz(const double red,const double green,
  const double blue,const double white_luminance,double *Jz,double *az,
  double *bz)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,blue,green,&X,&Y,&Z);
  ConvertXYZToJzazbz(X,Y,Z,white_luminance,Jz,az,bz);
}

static inline void ConvertJzazbzToXYZ(const double Jz,const double az,
  const double bz,const double white_luminance,double *X,double *Y,double *Z)
{
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

  g=Jz+Jzazbz_d0;
  azz=az-0.5;
  bzz=bz-0.5;
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
  L*=white_luminance;
  M*=white_luminance;
  S*=white_luminance;
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
  *Z=Zp;
  *X=Xp;
  *Y=Yp;
}

static inline void ConvertJzazbzToRGB(const double Jz,const double az,
  const double bz,const double white_luminance,double *red,double *green,
  double *blue)
{
  double
    X,
    Y,
    Z;

  ConvertJzazbzToXYZ(Jz,az,bz,white_luminance,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,red,blue,green);
}

static inline void ConvertOklabToRGB(const double L,const double a,
  const double b,double *red,double *green,double *blue)
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
  const double H,double *red,double *green,double *blue)
{
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
