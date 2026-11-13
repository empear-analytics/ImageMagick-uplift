/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      PPPP   IIIII  X   X  EEEEE  L                          %
%                      P   P    I     X X   E      L                          %
%                      PPPP     I      X    EEE    L                          %
%                      P        I     X X   E      L                          %
%                      P      IIIII  X   X  EEEEE  LLLLL                      %
%                                                                             %
%                       GGGG   AAA   M   M  M   M   AAA                       %
%                      G      A   A  MM MM  MM MM  A   A                      %
%                      G  GG  AAAAA  M M M  M M M  AAAAA                      %
%                      G   G  A   A  M   M  M   M  A   A                      %
%                       GGG   A   A  M   M  M   M  A   A                      %
%                                                                             %
%                      sRGB Gamma Encoding and Decoding                       %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               October 1998                                  %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/property.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e c o d e P i x e l G a m m a                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DecodePixelGamma() applies the expansive power-law nonlinearity to the pixel.
%
%  The format of the DecodePixelGamma method is:
%
%      double DecodePixelGamma(const MagickRealType pixel)
%
%  A description of each parameter follows:
%
%    o pixel: the pixel.
%
*/

static inline double DecodeGamma(const double x)
{
  div_t
    quotient;

  double
    p,
    term[9];

  int
    exponent;

  static const double coefficient[] =  /* terms for x^(7/5), x=1.5 */
  {
    1.7917488588043277509,
    0.82045614371976854984,
    0.027694100686325412819,
    -0.00094244335181762134018,
    0.000064355540911469709545,
    -5.7224404636060757485e-06,
    5.8767669437311184313e-07,
    -6.6139920053589721168e-08,
    7.9323242696227458163e-09
  };

  static const double powers_of_two[] =  /* (2^x)^(7/5) */
  {
    1.0,
    2.6390158215457883983,
    6.9644045063689921093,
    1.8379173679952558018e+01,
    4.8502930128332728543e+01
  };

  /*
    Compute x^2.4 == x*x^(7/5) == pow(x,2.4).
  */
  term[0]=1.0;
  term[1]=4.0*frexp(x,&exponent)-3.0;
  term[2]=2.0*term[1]*term[1]-term[0];
  term[3]=2.0*term[1]*term[2]-term[1];
  term[4]=2.0*term[1]*term[3]-term[2];
  term[5]=2.0*term[1]*term[4]-term[3];
  term[6]=2.0*term[1]*term[5]-term[4];
  term[7]=2.0*term[1]*term[6]-term[5];
  term[8]=2.0*term[1]*term[7]-term[6];
  p=coefficient[0]*term[0]+coefficient[1]*term[1]+coefficient[2]*term[2]+
    coefficient[3]*term[3]+coefficient[4]*term[4]+coefficient[5]*term[5]+
    coefficient[6]*term[6]+coefficient[7]*term[7]+coefficient[8]*term[8];
  quotient=div(exponent-1,5);
  if (quotient.rem < 0)
    {
      quotient.quot-=1;
      quotient.rem+=5;
    }
  return(x*ldexp(powers_of_two[quotient.rem]*p,7*quotient.quot));
}

MagickExport MagickRealType DecodePixelGamma(const MagickRealType pixel)
{
  if (pixel <= (0.0404482362771076*(double) QuantumRange))
    return(pixel/12.92);
  return((MagickRealType) ((double) QuantumRange*DecodeGamma((double)
    (QuantumScale*pixel+0.055)/1.055)));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E n c o d e P i x e l G a m m a                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodePixelGamma() cancels any nonlinearity in the pixel.
%
%  The format of the EncodePixelGamma method is:
%
%      MagickRealType EncodePixelGamma(const double MagickRealType)
%
%  A description of each parameter follows:
%
%    o pixel: the pixel.
%
*/

static inline double EncodeGamma(const double x)
{
  div_t
    quotient;

  double
    p,
    term[9];

  int
    exponent;

  static const double coefficient[] =  /* Chebychevi poly: x^(5/12), x=1.5 */
  {
    1.1758200232996901923,
    0.16665763094889061230,
    -0.0083154894939042125035,
    0.00075187976780420279038,
    -0.000083240178519391795367,
    0.000010229209410070008679,
    -1.3400466409860246e-06,
    1.8333422241635376682e-07,
    -2.5878596761348859722e-08
  };

  static const double powers_of_two[] =  /* (2^N)^(5/12) */
  {
    1.0,
    1.3348398541700343678,
    1.7817974362806785482,
    2.3784142300054420538,
    3.1748021039363991669,
    4.2378523774371812394,
    5.6568542494923805819,
    7.5509945014535482244,
    1.0079368399158985525e1,
    1.3454342644059433809e1,
    1.7959392772949968275e1,
    2.3972913230026907883e1
  };

  /*
    Compute x^(1/2.4) == x^(5/12) == pow(x,1.0/2.4).
  */
  term[0]=1.0;
  term[1]=4.0*frexp(x,&exponent)-3.0;
  term[2]=2.0*term[1]*term[1]-term[0];
  term[3]=2.0*term[1]*term[2]-term[1];
  term[4]=2.0*term[1]*term[3]-term[2];
  term[5]=2.0*term[1]*term[4]-term[3];
  term[6]=2.0*term[1]*term[5]-term[4];
  term[7]=2.0*term[1]*term[6]-term[5];
  term[8]=2.0*term[1]*term[7]-term[6];
  p=coefficient[0]*term[0]+coefficient[1]*term[1]+coefficient[2]*term[2]+
    coefficient[3]*term[3]+coefficient[4]*term[4]+coefficient[5]*term[5]+
    coefficient[6]*term[6]+coefficient[7]*term[7]+coefficient[8]*term[8];
  quotient=div(exponent-1,12);
  if (quotient.rem < 0)
    {
      quotient.quot-=1;
      quotient.rem+=12;
    }
  return(ldexp(powers_of_two[quotient.rem]*p,5*quotient.quot));
}

MagickExport MagickRealType EncodePixelGamma(const MagickRealType pixel)
{
  if (pixel <= (0.0031306684425005883*(double) QuantumRange))
    return(12.92*pixel);
  return((MagickRealType) QuantumRange*(1.055*EncodeGamma((double) QuantumScale*
    pixel)-0.055));
}
