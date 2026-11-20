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

  MagickCore private colorspace conversions to RGB from the hue-based
  colourspaces (HCL, HCLp, HSB, HSI, HSV, HWB); included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_TO_RGB_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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
