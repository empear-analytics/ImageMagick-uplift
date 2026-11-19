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

  MagickCore private colorspace conversions from RGB: XYZ, CAT02 LMS, CMY,
  Adobe98, Display P3 and the hue-based colourspaces (HCL, HCLp, HSB, HSI,
  HSV, HWB); included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_HUE_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_HUE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertXYZToCAT02LMS(const double X,const double Y,
  const double Z,double *L,double *M,double *S)
{
  *L=0.7328*X+0.4296*Y-0.1624*Z;
  *M=(-0.7036)*X+1.6975*Y+0.0061*Z;
  *S=0.0030*X+0.0136*Y+0.9834*Z;
}

static inline void ConvertRGBToXYZ(const double red,const double green,
  const double blue,double *X,double *Y,double *Z)
{
  double
    b,
    g,
    r;

  /*
    Convert RGB to XYZ colorspace.
  */
  r=QuantumScale*DecodePixelGamma(red);
  g=QuantumScale*DecodePixelGamma(green);
  b=QuantumScale*DecodePixelGamma(blue);
  *X=(0.4123955889674142161*r)+(0.3575834307637148171*g)+
    (0.1804926473817015735*b);
  *Y=(0.2125862307855955516*r)+(0.7151703037034108499*g)+
    (0.07220049864333622685*b);
  *Z=(0.01929721549174694484*r)+(0.1191838645808485318*g)+
    (0.9504971251315797660*b);
}

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

static inline void ConvertRGBToCMY(const double red,const double green,
  const double blue,double *cyan,double *magenta,double *yellow)
{
  *cyan=QuantumScale*((double) QuantumRange-red);
  *magenta=QuantumScale*((double) QuantumRange-green);
  *yellow=QuantumScale*((double) QuantumRange-blue);
}

static inline void ConvertRGBToHCL(const double red,const double green,
  const double blue,double *hue,double *chroma,double *luma)
{
  double
    c,
    h,
    max;

  /*
    Convert RGB to HCL colorspace.
  */
  assert(hue != (double *) NULL);
  assert(chroma != (double *) NULL);
  assert(luma != (double *) NULL);
  max=MagickMax(red,MagickMax(green,blue));
  c=max-(double) MagickMin(red,MagickMin(green,blue));
  h=0.0;
  if (fabs(c) < MagickEpsilon)
    h=0.0;
  else
    if (fabs(red-max) < MagickEpsilon)
      h=fmod((green-blue)/c+6.0,6.0);
    else
      if (fabs(green-max) < MagickEpsilon)
        h=((blue-red)/c)+2.0;
      else
        if (fabs(blue-max) < MagickEpsilon)
          h=((red-green)/c)+4.0;
  *hue=(h/6.0);
  *chroma=QuantumScale*c;
  *luma=QuantumScale*(0.298839*red+0.586811*green+0.114350*blue);
}

static inline void ConvertRGBToHCLp(const double red,const double green,
  const double blue,double *hue,double *chroma,double *luma)
{
  double
    c,
    h,
    max;

  /*
    Convert RGB to HCL colorspace.
  */
  assert(hue != (double *) NULL);
  assert(chroma != (double *) NULL);
  assert(luma != (double *) NULL);
  max=MagickMax(red,MagickMax(green,blue));
  c=max-MagickMin(red,MagickMin(green,blue));
  h=0.0;
  if (fabs(c) < MagickEpsilon)
    h=0.0;
  else
    if (fabs(red-max) < MagickEpsilon)
      h=fmod((green-blue)/c+6.0,6.0);
    else
      if (fabs(green-max) < MagickEpsilon)
        h=((blue-red)/c)+2.0;
      else
        if (fabs(blue-max) < MagickEpsilon)
          h=((red-green)/c)+4.0;
  *hue=(h/6.0);
  *chroma=QuantumScale*c;
  *luma=QuantumScale*(0.298839*red+0.586811*green+0.114350*blue);
}

static inline void ConvertRGBToHSB(const double red,const double green,
  const double blue,double *hue,double *saturation,double *brightness)
{
  double
    delta,
    max,
    min;

  /*
    Convert RGB to HSB colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(brightness != (double *) NULL);
  *hue=0.0;
  *saturation=0.0;
  *brightness=0.0;
  min=red < green ? red : green;
  if (blue < min)
    min=blue;
  max=red > green ? red : green;
  if (blue > max)
    max=blue;
  if (fabs(max) < MagickEpsilon)
    return;
  delta=max-min;
  *saturation=delta/max;
  *brightness=QuantumScale*max;
  if (fabs(delta) < MagickEpsilon)
    return;
  if (fabs(red-max) < MagickEpsilon)
    *hue=(green-blue)/delta;
  else
    if (fabs(green-max) < MagickEpsilon)
      *hue=2.0+(blue-red)/delta;
    else
      *hue=4.0+(red-green)/delta;
  *hue/=6.0;
  if (*hue < 0.0)
    *hue+=1.0;
}

static inline void ConvertRGBToHSI(const double red,const double green,
  const double blue,double *hue,double *saturation,double *intensity)
{
  double
    alpha,
    beta;

  /*
    Convert RGB to HSI colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(intensity != (double *) NULL);
  *intensity=(QuantumScale*red+QuantumScale*green+QuantumScale*blue)/3.0;
  if (*intensity <= 0.0)
    {
      *hue=0.0;
      *saturation=0.0;
      return;
    }
  *saturation=1.0-MagickMin(QuantumScale*red,MagickMin(QuantumScale*green,
    QuantumScale*blue))/(*intensity);
  alpha=0.5*(2.0*QuantumScale*red-QuantumScale*green-QuantumScale*blue);
  beta=0.8660254037844385*(QuantumScale*green-QuantumScale*blue);
  *hue=atan2(beta,alpha)*(180.0/MagickPI)/360.0;
  if (*hue < 0.0)
    *hue+=1.0;
}

static inline void ConvertXYZToAdobe98(const double X,const double Y,
  const double Z,double *red,double *green,double *blue)
{
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

static inline void ConvertRGBToAdobe98(const double red,const double green,
  const double blue,double *r,double *g,double *b)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  ConvertXYZToAdobe98(X,Y,Z,r,g,b);
}

static inline void ConvertXYZToDisplayP3(const double X,const double Y,
  const double Z,double *red,double *green,double *blue)
{
  double
    b,
    g,
    r;

  r=2.49349691194142500*X-0.93138361791912390*Y-0.402710784450716840*Z;
  g=(-0.82948896956157470)*X+1.76266406031834630*Y+0.023624685841943577*Z;
  b=0.03584583024378447*X-0.07617238926804182*Y+0.956884524007687200*Z;
  *red=QuantumScale*EncodePixelGamma((double) QuantumRange*r);
  *green=QuantumScale*EncodePixelGamma((double) QuantumRange*g);
  *blue=QuantumScale*EncodePixelGamma((double) QuantumRange*b);
}

static inline void ConvertRGBToDisplayP3(const double red,const double green,
  const double blue,double *r,double *g,double *b)
{
  double
    X,
    Y,
    Z;

  ConvertRGBToXYZ(red,green,blue,&X,&Y,&Z);
  ConvertXYZToDisplayP3(X,Y,Z,r,g,b);
}

static inline void ConvertRGBToHSV(const double red,const double green,
  const double blue,double *hue,double *saturation,double *value)
{
  double
    c,
    max,
    min;

  /*
    Convert RGB to HSV colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(value != (double *) NULL);
  max=MagickMax(QuantumScale*red,MagickMax(QuantumScale*green,
    QuantumScale*blue));
  min=MagickMin(QuantumScale*red,MagickMin(QuantumScale*green,
    QuantumScale*blue));
  c=max-min;
  *value=max;
  if (c <= 0.0)
    {
      *hue=0.0;
      *saturation=0.0;
      return;
    }
  if (fabs(max-QuantumScale*red) < MagickEpsilon)
    {
      *hue=(QuantumScale*green-QuantumScale*blue)/c;
      if ((QuantumScale*green) < (QuantumScale*blue))
        *hue+=6.0;
    }
  else
    if (fabs(max-QuantumScale*green) < MagickEpsilon)
      *hue=2.0+(QuantumScale*blue-QuantumScale*red)/c;
    else
      *hue=4.0+(QuantumScale*red-QuantumScale*green)/c;
  *hue*=60.0/360.0;
  *saturation=c*MagickSafeReciprocal(max);
}

static inline void ConvertRGBToHWB(const double red,const double green,
  const double blue,double *hue,double *whiteness,double *blackness)
{
  double
    f,
    p,
    v,
    w;

  /*
    Convert RGB to HWB colorspace.
  */
  assert(hue != (double *) NULL);
  assert(whiteness != (double *) NULL);
  assert(blackness != (double *) NULL);
  w=MagickMin(red,MagickMin(green,blue));
  v=MagickMax(red,MagickMax(green,blue));
  *blackness=1.0-QuantumScale*v;
  *whiteness=QuantumScale*w;
  if (fabs(v-w) < MagickEpsilon)
    {
      *hue=(-1.0);
      return;
    }
  f=(fabs(red-w) < MagickEpsilon) ? green-blue :
    ((fabs(green-w) < MagickEpsilon) ? blue-red : red-green);
  p=(fabs(red-w) < MagickEpsilon) ? 3.0 :
    ((fabs(green-w) < MagickEpsilon) ? 5.0 : 1.0);
  *hue=(p-f/(v-1.0*w))/6.0;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
