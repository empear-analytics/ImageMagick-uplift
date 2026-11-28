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

  MagickCore private colorspace conversions from RGB and XYZ: Adobe98,
  Display P3, HSV and HWB; included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_RGB_TO_HUE_PRIVATE_H
#define MAGICKCORE_COLORSPACE_RGB_TO_HUE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _XYZToAdobe98Input
{
  double
    X,
    Y,
    Z;
} XYZToAdobe98Input;

static inline void ConvertXYZToAdobe98(const XYZToAdobe98Input *xyz,
  double *red,double *green,double *blue)
{
  const double
    X = xyz->X,
    Y = xyz->Y,
    Z = xyz->Z;

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
  {
    const XYZToAdobe98Input
      xyz = { X, Y, Z };

    ConvertXYZToAdobe98(&xyz,r,g,b);
  }
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
