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

  MagickCore private colorspace conversions to RGB: the illuminant constants
  and table, XYZ, Adobe98, CAT02 LMS, CMYK, CMY and the hue-based
  colourspaces (HCL, HCLp, HSB, HSI, HSV, HWB) to RGB; included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_TO_RGB_PRIVATE_H

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

static inline void ConvertAdobe98ToRGB(const double r,const double g,
  const double b,double *red,double *green,double *blue)
{
  double
    X,
    Y,
    Z;

  ConvertAdobe98ToXYZ(r,g,b,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,red,green,blue);
}

static inline void ConvertCAT02LMSToXYZ(const double L,const double M,
  const double S,double *X,double *Y,double *Z)
{
  /*
    Convert CAT02LMS to XYZ colorspace.
  */
  *X=1.096123820835514*L-0.278869000218287*M+0.182745179382773*S;
  *Y=0.454369041975359*L+0.473533154307412*M+0.072097803717229*S;
  *Z=(-0.009627608738429)*L-0.005698031216113*M+1.015325639954543*S;
}

static inline void ConvertCAT02LMSToRGB(const double L,const double M,
  const double S,double *R,double *G,double *B)
{
  double
    X,
    Y,
    Z;

  ConvertCAT02LMSToXYZ(L,M,S,&X,&Y,&Z);
  ConvertXYZToRGB(X,Y,Z,R,G,B);
}

static inline void ConvertCMYKToRGB(PixelInfo *pixel)
{
  pixel->red=(((double) QuantumRange-(QuantumScale*pixel->red*
    ((double) QuantumRange-pixel->black)+pixel->black)));
  pixel->green=(((double) QuantumRange-(QuantumScale*pixel->green*
    ((double) QuantumRange-pixel->black)+pixel->black)));
  pixel->blue=(((double) QuantumRange-(QuantumScale*pixel->blue*
    ((double) QuantumRange-pixel->black)+pixel->black)));
}

static inline void ConvertCMYToRGB(const double cyan,const double magenta,
  const double yellow,double *red,double *green,double *blue)
{
	*red=(double) QuantumRange*(1.0-cyan);
  *green=(double) QuantumRange*(1.0-magenta);
  *blue=(double) QuantumRange*(1.0-yellow);
}

static inline void ConvertHCLToRGB(const double hue,const double chroma,
  const double luma,double *red,double *green,double *blue)
{
  double
    b,
    c,
    g,
    h,
    m,
    r,
    x;

  /*
    Convert HCL to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  h=6.0*hue;
  c=chroma;
  x=c*(1.0-fabs(fmod(h,2.0)-1.0));
  r=0.0;
  g=0.0;
  b=0.0;
  if ((0.0 <= h) && (h < 1.0))
    {
      r=c;
      g=x;
    }
  else
    if ((1.0 <= h) && (h < 2.0))
      {
        r=x;
        g=c;
      }
    else
      if ((2.0 <= h) && (h < 3.0))
        {
          g=c;
          b=x;
        }
      else
        if ((3.0 <= h) && (h < 4.0))
          {
            g=x;
            b=c;
          }
        else
          if ((4.0 <= h) && (h < 5.0))
            {
              r=x;
              b=c;
            }
          else
            if ((5.0 <= h) && (h < 6.0))
              {
                r=c;
                b=x;
              }
  m=luma-(0.298839*r+0.586811*g+0.114350*b);
  *red=(double) QuantumRange*(r+m);
  *green=(double) QuantumRange*(g+m);
  *blue=(double) QuantumRange*(b+m);
}

static inline void ConvertHCLpToRGB(const double hue,const double chroma,
  const double luma,double *red,double *green,double *blue)
{
  double
    b,
    c,
    g,
    h,
    m,
    r,
    x,
    z;

  /*
    Convert HCLp to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  h=6.0*hue;
  c=chroma;
  x=c*(1.0-fabs(fmod(h,2.0)-1.0));
  r=0.0;
  g=0.0;
  b=0.0;
  if ((0.0 <= h) && (h < 1.0))
    {
      r=c;
      g=x;
    }
  else
    if ((1.0 <= h) && (h < 2.0))
      {
        r=x;
        g=c;
      }
    else
      if ((2.0 <= h) && (h < 3.0))
        {
          g=c;
          b=x;
        }
      else
        if ((3.0 <= h) && (h < 4.0))
          {
            g=x;
            b=c;
          }
        else
          if ((4.0 <= h) && (h < 5.0))
            {
              r=x;
              b=c;
            }
          else
            if ((5.0 <= h) && (h < 6.0))
              {
                r=c;
                b=x;
              }
  m=luma-(0.298839*r+0.586811*g+0.114350*b);
  z=1.0;
  if (m < 0.0)
    {
      z=luma/(luma-m);
      m=0.0;
    }
  else
    if (m+c > 1.0)
      {
        z=(1.0-luma)/(m+c-luma);
        m=1.0-z*c;
      }
  *red=(double) QuantumRange*(z*r+m);
  *green=(double) QuantumRange*(z*g+m);
  *blue=(double) QuantumRange*(z*b+m);
}

static inline void ConvertHSBToRGB(const double hue,const double saturation,
  const double brightness,double *red,double *green,double *blue)
{
  double
    f,
    h,
    p,
    q,
    t;

  /*
    Convert HSB to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  if (fabs(saturation) < MagickEpsilon)
    {
      *red=(double) QuantumRange*brightness;
      *green=(*red);
      *blue=(*red);
      return;
    }
  h=6.0*(hue-floor(hue));
  f=h-floor((double) h);
  p=brightness*(1.0-saturation);
  q=brightness*(1.0-saturation*f);
  t=brightness*(1.0-(saturation*(1.0-f)));
  switch ((int) h)
  {
    case 0:
    default:
    {
      *red=(double) QuantumRange*brightness;
      *green=(double) QuantumRange*t;
      *blue=(double) QuantumRange*p;
      break;
    }
    case 1:
    {
      *red=(double) QuantumRange*q;
      *green=(double) QuantumRange*brightness;
      *blue=(double) QuantumRange*p;
      break;
    }
    case 2:
    {
      *red=(double) QuantumRange*p;
      *green=(double) QuantumRange*brightness;
      *blue=(double) QuantumRange*t;
      break;
    }
    case 3:
    {
      *red=(double) QuantumRange*p;
      *green=(double) QuantumRange*q;
      *blue=(double) QuantumRange*brightness;
      break;
    }
    case 4:
    {
      *red=(double) QuantumRange*t;
      *green=(double) QuantumRange*p;
      *blue=(double) QuantumRange*brightness;
      break;
    }
    case 5:
    {
      *red=(double) QuantumRange*brightness;
      *green=(double) QuantumRange*p;
      *blue=(double) QuantumRange*q;
      break;
    }
  }
}

static inline void ConvertHSIToRGB(const double hue,const double saturation,
  const double intensity,double *red,double *green,double *blue)
{
  double
    b,
    g,
    h,
    r;

  /*
    Convert HSI to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  h=360.0*hue;
  h-=360.0*floor(h/360.0);
  if (h < 120.0)
    {
      b=intensity*(1.0-saturation);
      r=intensity*(1.0+saturation*cos(h*(MagickPI/180.0))/cos((60.0-h)*
        (MagickPI/180.0)));
      g=3.0*intensity-r-b;
    }
  else
    if (h < 240.0)
      {
        h-=120.0;
        r=intensity*(1.0-saturation);
        g=intensity*(1.0+saturation*cos(h*(MagickPI/180.0))/cos((60.0-h)*
          (MagickPI/180.0)));
        b=3.0*intensity-r-g;
      }
    else
      {
        h-=240.0;
        g=intensity*(1.0-saturation);
        b=intensity*(1.0+saturation*cos(h*(MagickPI/180.0))/cos((60.0-h)*
          (MagickPI/180.0)));
        r=3.0*intensity-g-b;
      }
  *red=(double) QuantumRange*r;
  *green=(double) QuantumRange*g;
  *blue=(double) QuantumRange*b;
}

static inline void ConvertHSVToRGB(const double hue,const double saturation,
  const double value,double *red,double *green,double *blue)
{
  double
    c,
    h,
    min,
    x;

  /*
    Convert HSV to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  h=hue*360.0;
  c=value*saturation;
  min=value-c;
  h-=360.0*floor(h/360.0);
  h/=60.0;
  x=c*(1.0-fabs(h-2.0*floor(h/2.0)-1.0));
  switch ((int) floor(h))
  {
    case 0:
    default:
    {
      *red=(double) QuantumRange*(min+c);
      *green=(double) QuantumRange*(min+x);
      *blue=(double) QuantumRange*min;
      break;
    }
    case 1:
    {
      *red=(double) QuantumRange*(min+x);
      *green=(double) QuantumRange*(min+c);
      *blue=(double) QuantumRange*min;
      break;
    }
    case 2:
    {
      *red=(double) QuantumRange*min;
      *green=(double) QuantumRange*(min+c);
      *blue=(double) QuantumRange*(min+x);
      break;
    }
    case 3:
    {
      *red=(double) QuantumRange*min;
      *green=(double) QuantumRange*(min+x);
      *blue=(double) QuantumRange*(min+c);
      break;
    }
    case 4:
    {
      *red=(double) QuantumRange*(min+x);
      *green=(double) QuantumRange*min;
      *blue=(double) QuantumRange*(min+c);
      break;
    }
    case 5:
    {
      *red=(double) QuantumRange*(min+c);
      *green=(double) QuantumRange*min;
      *blue=(double) QuantumRange*(min+x);
      break;
    }
  }
}

static inline void ConvertHWBToRGB(const double hue,const double whiteness,
  const double blackness,double *red,double *green,double *blue)
{
  double
    b,
    f,
    g,
    n,
    r,
    v;

  ssize_t
    i;

  /*
    Convert HWB to RGB colorspace.
  */
  assert(red != (double *) NULL);
  assert(green != (double *) NULL);
  assert(blue != (double *) NULL);
  v=1.0-blackness;
  if (fabs(hue-(-1.0)) < MagickEpsilon)
    {
      *red=(double) QuantumRange*v;
      *green=(double) QuantumRange*v;
      *blue=(double) QuantumRange*v;
      return;
    }
  i=CastDoubleToSsizeT(floor(6.0*hue));
  f=6.0*hue-(double) i;
  if ((i & 0x01) != 0)
    f=1.0-f;
  n=whiteness+f*(v-whiteness);  /* linear interpolation */
  switch (i)
  {
    case 0:
    default: r=v; g=n; b=whiteness; break;
    case 1: r=n; g=v; b=whiteness; break;
    case 2: r=whiteness; g=v; b=n; break;
    case 3: r=whiteness; g=n; b=v; break;
    case 4: r=n; g=whiteness; b=v; break;
    case 5: r=v; g=whiteness; b=n; break;
  }
  *red=(double) QuantumRange*r;
  *green=(double) QuantumRange*g;
  *blue=(double) QuantumRange*b;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
