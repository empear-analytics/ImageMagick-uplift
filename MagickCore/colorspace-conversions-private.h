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

  MagickCore private colorspace conversions between RGB and the luma-chroma
  colourspaces (YDbDr, YIQ, YPbPr, YCbCr, YUV) and from RGB to CMYK; the
  other converters stand in the colorspace-*-private.h headers that
  colorspace-private.h includes before this one. Included by
  colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_CONVERSIONS_PRIVATE_H
#define MAGICKCORE_COLORSPACE_CONVERSIONS_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void ConvertRGBToYDbDr(const double red,const double green,
  const double blue,double *Y,double *Db,double *Dr)
{
  *Y=QuantumScale*(0.298839*red+0.586811*green+0.114350*blue);
  *Db=QuantumScale*(-0.450*red-0.883*green+1.333*blue)+0.5;
  *Dr=QuantumScale*(-1.333*red+1.116*green+0.217*blue)+0.5;
}

static inline void ConvertRGBToYIQ(const double red,const double green,
  const double blue,double *Y,double *I,double *Q)
{
  *Y=QuantumScale*(0.298839*red+0.586811*green+0.114350*blue);
  *I=QuantumScale*(0.595716*red-0.274453*green-0.321263*blue)+0.5;
  *Q=QuantumScale*(0.211456*red-0.522591*green+0.311135*blue)+0.5;
}

static inline void ConvertRGBToYPbPr(const double red,const double green,
  const double blue,double *Y,double *Pb,double *Pr)
{
  *Y=QuantumScale*(0.298839*red+0.586811*green+0.114350*blue);
  *Pb=QuantumScale*((-0.1687367)*red-0.331264*green+0.5*blue)+0.5;
  *Pr=QuantumScale*(0.5*red-0.418688*green-0.081312*blue)+0.5;
}

static inline void ConvertRGBToYCbCr(const double red,const double green,
  const double blue,double *Y,double *Cb,double *Cr)
{
  ConvertRGBToYPbPr(red,green,blue,Y,Cb,Cr);
}

static inline void ConvertRGBToYUV(const double red,const double green,
  const double blue,double *Y,double *U,double *V)
{
  *Y=QuantumScale*(0.298839*red+0.586811*green+0.114350*blue);
  *U=QuantumScale*((-0.147)*red-0.289*green+0.436*blue)+0.5;
  *V=QuantumScale*(0.615*red-0.515*green-0.100*blue)+0.5;
}

static inline void ConvertRGBToCMYK(PixelInfo *pixel)
{
  MagickRealType
    black,
    blue,
    cyan,
    green,
    magenta,
    red,
    yellow;

  if (pixel->colorspace != sRGBColorspace)
    {
      red=QuantumScale*pixel->red;
      green=QuantumScale*pixel->green;
      blue=QuantumScale*pixel->blue;
    }
  else
    {
      red=QuantumScale*DecodePixelGamma(pixel->red);
      green=QuantumScale*DecodePixelGamma(pixel->green);
      blue=QuantumScale*DecodePixelGamma(pixel->blue);
    }
  if ((fabs((double) red) < MagickEpsilon) &&
      (fabs((double) green) < MagickEpsilon) &&
      (fabs((double) blue) < MagickEpsilon))
    {
      pixel->black=(MagickRealType) QuantumRange;
      return;
    }
  cyan=(MagickRealType) (1.0-red);
  magenta=(MagickRealType) (1.0-green);
  yellow=(MagickRealType) (1.0-blue);
  black=cyan;
  if (magenta < black)
    black=magenta;
  if (yellow < black)
    black=yellow;
  cyan=(MagickRealType) (MagickSafeReciprocal(1.0-black)*(cyan-black));
  magenta=(MagickRealType) (MagickSafeReciprocal(1.0-black)*(magenta-black));
  yellow=(MagickRealType) (MagickSafeReciprocal(1.0-black)*(yellow-black));
  pixel->colorspace=CMYKColorspace;
  pixel->red=(MagickRealType) QuantumRange*cyan;
  pixel->green=(MagickRealType) QuantumRange*magenta;
  pixel->blue=(MagickRealType) QuantumRange*yellow;
  pixel->black=(MagickRealType) QuantumRange*black;
}

static inline void ConvertYPbPrToRGB(const double Y,const double Pb,
  const double Pr,double *red,double *green,double *blue)
{
  *red=(double) QuantumRange*(0.99999999999914679361*Y-1.2188941887145875e-06*
    (Pb-0.5)+1.4019995886561440468*(Pr-0.5));
  *green=(double) QuantumRange*(0.99999975910502514331*Y-0.34413567816504303521*
    (Pb-0.5)-0.71413649331646789076*(Pr-0.5));
  *blue=(double) QuantumRange*(1.00000124040004623180*Y+1.77200006607230409200*
    (Pb-0.5)+2.1453384174593273e-06*(Pr-0.5));
}

static inline void ConvertYCbCrToRGB(const double Y,const double Cb,
  const double Cr,double *red,double *green,double *blue)
{
  ConvertYPbPrToRGB(Y,Cb,Cr,red,green,blue);
}

static inline void ConvertYDbDrToRGB(const double Y,const double Db,
  const double Dr,double *red,double *green,double *blue)
{
  *red=(double) QuantumRange*(Y+9.2303716147657e-05*(Db-0.5)-
    0.52591263066186533*(Dr-0.5));
  *green=(double) QuantumRange*(Y-0.12913289889050927*(Db-0.5)+
    0.26789932820759876*(Dr-0.5));
  *blue=(double) QuantumRange*(Y+0.66467905997895482*(Db-0.5)-
    7.9202543533108e-05*(Dr-0.5));
}

static inline void ConvertYIQToRGB(const double Y,const double I,const double Q,
  double *red,double *green,double *blue)
{
  *red=(double) QuantumRange*(Y+0.9562957197589482261*(I-0.5)+
    0.6210244164652610754*(Q-0.5));
  *green=(double) QuantumRange*(Y-0.2721220993185104464*(I-0.5)-
    0.6473805968256950427*(Q-0.5));
  *blue=(double) QuantumRange*(Y-1.1069890167364901945*(I-0.5)+
    1.7046149983646481374*(Q-0.5));
}

static inline void ConvertxyYToRGB(const double low_x,const double low_y,
  const double cap_Y,double *red,double *green,double *blue)
{
  double
    gamma,
    X,
    Y,
    Z;

  gamma=MagickSafeReciprocal(low_y);
  X=gamma*cap_Y*low_x;
  Y=cap_Y;
  Z=gamma*cap_Y*(1.0-low_x-low_y);
  ConvertXYZToRGB(X,Y,Z,red,green,blue);
}

static inline void ConvertYUVToRGB(const double Y,const double U,const double V,
  double *red,double *green,double *blue)
{
  *red=(double) QuantumRange*(Y-3.945707070708279e-05*(U-0.5)+
    1.1398279671717170825*(V-0.5));
  *green=(double) QuantumRange*(Y-0.3946101641414141437*(U-0.5)-
    0.5805003156565656797*(V-0.5));
  *blue=(double) QuantumRange*(Y+2.0319996843434342537*(U-0.5)-
    4.813762626262513e-04*(V-0.5));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
