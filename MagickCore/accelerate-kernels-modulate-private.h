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

  MagickCore private text included in place by accelerate-kernels-private.h,
  inside its accelerateKernels initialiser: the Modulate kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o d u l a t e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(

  static inline void ConvertRGBToHSL(const CLQuantum red,const CLQuantum green, const CLQuantum blue,
    float *hue, float *saturation, float *lightness)
  {
  float
    c,
    tmax,
    tmin;

  /*
     Convert RGB to HSL colorspace.
     */
  tmax=MagickMax(QuantumScale*red,MagickMax(QuantumScale*green, QuantumScale*blue));
  tmin=MagickMin(QuantumScale*red,MagickMin(QuantumScale*green, QuantumScale*blue));

  c=tmax-tmin;

  *lightness=(tmax+tmin)/2.0;
  if (c <= 0.0)
  {
    *hue=0.0;
    *saturation=0.0;
    return;
  }

  if (tmax == (QuantumScale*red))
  {
    *hue=(QuantumScale*green-QuantumScale*blue)/c;
    if ((QuantumScale*green) < (QuantumScale*blue))
      *hue+=6.0;
  }
  else
    if (tmax == (QuantumScale*green))
      *hue=2.0+(QuantumScale*blue-QuantumScale*red)/c;
    else
      *hue=4.0+(QuantumScale*red-QuantumScale*green)/c;

  *hue*=60.0/360.0;
  if (*lightness <= 0.5)
    *saturation=c/(2.0*(*lightness));
  else
    *saturation=c/(2.0-2.0*(*lightness));
  }

  static inline void ConvertHSLToRGB(const float hue,const float saturation, const float lightness,
      CLQuantum *red,CLQuantum *green,CLQuantum *blue)
  {
    float
      b,
      c,
      g,
      h,
      tmin,
      r,
      x;

    /*
       Convert HSL to RGB colorspace.
       */
    h=hue*360.0;
    if (lightness <= 0.5)
      c=2.0*lightness*saturation;
    else
      c=(2.0-2.0*lightness)*saturation;
    tmin=lightness-0.5*c;
    h-=360.0*floor(h/360.0);
    h/=60.0;
    x=c*(1.0-fabs(h-2.0*floor(h/2.0)-1.0));
    switch ((int) floor(h) % 6)
    {
      case 0:
      default:
        {
          r=tmin+c;
          g=tmin+x;
          b=tmin;
          break;
        }
      case 1:
        {
          r=tmin+x;
          g=tmin+c;
          b=tmin;
          break;
        }
      case 2:
        {
          r=tmin;
          g=tmin+c;
          b=tmin+x;
          break;
        }
      case 3:
        {
          r=tmin;
          g=tmin+x;
          b=tmin+c;
          break;
        }
      case 4:
        {
          r=tmin+x;
          g=tmin;
          b=tmin+c;
          break;
        }
      case 5:
        {
          r=tmin+c;
          g=tmin;
          b=tmin+x;
          break;
        }
    }
    *red=ClampToQuantum(QuantumRange*r);
    *green=ClampToQuantum(QuantumRange*g);
    *blue=ClampToQuantum(QuantumRange*b);
  }

  static inline void ModulateHSL(const float percent_hue, const float percent_saturation,const float percent_lightness,
    CLQuantum *red,CLQuantum *green,CLQuantum *blue)
  {
    float
      hue,
      lightness,
      saturation;

    /*
    Increase or decrease color lightness, saturation, or hue.
    */
    ConvertRGBToHSL(*red,*green,*blue,&hue,&saturation,&lightness);
    hue+=0.5*(0.01*percent_hue-1.0);
    while (hue < 0.0)
      hue+=1.0;
    while (hue >= 1.0)
      hue-=1.0;
    saturation*=0.01*percent_saturation;
    lightness*=0.01*percent_lightness;
    ConvertHSLToRGB(hue,saturation,lightness,red,green,blue);
  }

  __kernel void Modulate(__global CLPixelType *im,
    const float percent_brightness,
    const float percent_hue,
    const float percent_saturation,
    const int colorspace)
  {

    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int columns = get_global_size(0);
    const int c = x + y * columns;

    CLPixelType pixel = im[c];

    CLQuantum
        blue,
        green,
        red;

    red=getRed(pixel);
    green=getGreen(pixel);
    blue=getBlue(pixel);

    switch (colorspace)
    {
      case HSLColorspace:
      default:
        {
          ModulateHSL(percent_hue, percent_saturation, percent_brightness,
              &red, &green, &blue);
        }

    }

    CLPixelType filteredPixel;

    setRed(&filteredPixel, red);
    setGreen(&filteredPixel, green);
    setBlue(&filteredPixel, blue);
    filteredPixel.w = pixel.w;

    im[c] = filteredPixel;
  }
  )

