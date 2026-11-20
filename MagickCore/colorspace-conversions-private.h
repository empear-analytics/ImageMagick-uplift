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

  MagickCore private colorspace conversions from YPbPr, YCbCr, YDbDr, YIQ,
  xyY and YUV to RGB; the other converters stand in the
  colorspace-*-private.h headers that colorspace-private.h includes before
  this one. Included by colorspace-private.h only.
*/
#ifndef MAGICKCORE_COLORSPACE_CONVERSIONS_PRIVATE_H
#define MAGICKCORE_COLORSPACE_CONVERSIONS_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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
