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

  MagickCore private colorspace conversions to RGB from HCL and HCLp;
  included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_HCL_TO_RGB_PRIVATE_H
#define MAGICKCORE_COLORSPACE_HCL_TO_RGB_PRIVATE_H

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
