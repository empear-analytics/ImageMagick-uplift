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
  inside its accelerateKernels initialiser: the pixel helper functions, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
  STRINGIFY(

  static inline unsigned int getPixelIndex(const unsigned int number_channels,
    const unsigned int columns, const unsigned int x, const unsigned int y)
  {
    return (x * number_channels) + (y * columns * number_channels);
  }

  static inline float getPixelRed(const __global CLQuantum *p)   { return (float)*p; }
  static inline float getPixelGreen(const __global CLQuantum *p) { return (float)*(p+1); }
  static inline float getPixelBlue(const __global CLQuantum *p)  { return (float)*(p+2); }
  static inline float getPixelAlpha(const __global CLQuantum *p,const unsigned int number_channels) { return (float)*(p+number_channels-1); }

  static inline void setPixelRed(__global CLQuantum *p,const CLQuantum value)   { *p=value; }
  static inline void setPixelGreen(__global CLQuantum *p,const CLQuantum value) { *(p+1)=value; }
  static inline void setPixelBlue(__global CLQuantum *p,const CLQuantum value)  { *(p+2)=value; }
  static inline void setPixelAlpha(__global CLQuantum *p,const unsigned int number_channels,const CLQuantum value) { *(p+number_channels-1)=value; }

  static inline CLQuantum getBlue(CLPixelType p)               { return p.x; }
  static inline void setBlue(CLPixelType* p, CLQuantum value)  { (*p).x = value; }
  static inline float getBlueF4(float4 p)                      { return p.x; }
  static inline void setBlueF4(float4* p, float value)         { (*p).x = value; }

  static inline CLQuantum getGreen(CLPixelType p)              { return p.y; }
  static inline void setGreen(CLPixelType* p, CLQuantum value) { (*p).y = value; }
  static inline float getGreenF4(float4 p)                     { return p.y; }
  static inline void setGreenF4(float4* p, float value)        { (*p).y = value; }

  static inline CLQuantum getRed(CLPixelType p)                { return p.z; }
  static inline void setRed(CLPixelType* p, CLQuantum value)   { (*p).z = value; }
  static inline float getRedF4(float4 p)                       { return p.z; }
  static inline void setRedF4(float4* p, float value)          { (*p).z = value; }

  static inline CLQuantum getAlpha(CLPixelType p)              { return p.w; }
  static inline void setAlpha(CLPixelType* p, CLQuantum value) { (*p).w = value; }
  static inline float getAlphaF4(float4 p)                     { return p.w; }
  static inline void setAlphaF4(float4* p, float value)        { (*p).w = value; }

  static inline void ReadChannels(const __global CLQuantum *p, const unsigned int number_channels,
    const ChannelType channel, float *red, float *green, float *blue, float *alpha)
  {
    if ((channel & RedChannel) != 0)
      *red=getPixelRed(p);

    if (number_channels > 2)
      {
        if ((channel & GreenChannel) != 0)
          *green=getPixelGreen(p);

        if ((channel & BlueChannel) != 0)
          *blue=getPixelBlue(p);
      }

    if (((number_channels == 4) || (number_channels == 2)) &&
        ((channel & AlphaChannel) != 0))
      *alpha=getPixelAlpha(p,number_channels);
  }

  static inline float4 ReadAllChannels(const __global CLQuantum *image, const unsigned int number_channels,
    const unsigned int columns, const unsigned int x, const unsigned int y)
  {
    const __global CLQuantum *p = image + getPixelIndex(number_channels, columns, x, y);

    float4 pixel;

    pixel.x=getPixelRed(p);

    if (number_channels > 2)
      {
        pixel.y=getPixelGreen(p);
        pixel.z=getPixelBlue(p);
      }

    if ((number_channels == 4) || (number_channels == 2))
      pixel.w=getPixelAlpha(p,number_channels);
    return(pixel);
  }

  static inline float4 ReadFloat4(const __global CLQuantum *image, const unsigned int number_channels,
    const unsigned int columns, const unsigned int x, const unsigned int y, const ChannelType channel)
  {
    const __global CLQuantum *p = image + getPixelIndex(number_channels, columns, x, y);

    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 0.0f;

    ReadChannels(p, number_channels, channel, &red, &green, &blue, &alpha);
    return (float4)(red, green, blue, alpha);
  }

  static inline void WriteChannels(__global CLQuantum *p, const unsigned int number_channels,
    const ChannelType channel, float red, float green, float blue, float alpha)
  {
    if ((channel & RedChannel) != 0)
      setPixelRed(p,ClampToQuantum(red));

    if (number_channels > 2)
      {
        if ((channel & GreenChannel) != 0)
          setPixelGreen(p,ClampToQuantum(green));

        if ((channel & BlueChannel) != 0)
          setPixelBlue(p,ClampToQuantum(blue));
      }

    if (((number_channels == 4) || (number_channels == 2)) &&
        ((channel & AlphaChannel) != 0))
      setPixelAlpha(p,number_channels,ClampToQuantum(alpha));
  }

  static inline void WriteAllChannels(__global CLQuantum *image, const unsigned int number_channels,
    const unsigned int columns, const unsigned int x, const unsigned int y, float4 pixel)
  {
    __global CLQuantum *p = image + getPixelIndex(number_channels, columns, x, y);

    setPixelRed(p,ClampToQuantum(pixel.x));

    if (number_channels > 2)
      {
        setPixelGreen(p,ClampToQuantum(pixel.y));
        setPixelBlue(p,ClampToQuantum(pixel.z));
      }

    if ((number_channels == 4) || (number_channels == 2))
      setPixelAlpha(p,number_channels,ClampToQuantum(pixel.w));
  }

  static inline void WriteFloat4(__global CLQuantum *image, const unsigned int number_channels,
    const unsigned int columns, const unsigned int x, const unsigned int y, const ChannelType channel,
    float4 pixel)
  {
    __global CLQuantum *p = image + getPixelIndex(number_channels, columns, x, y);
    WriteChannels(p, number_channels, channel, pixel.x, pixel.y, pixel.z, pixel.w);
  }

  static inline float GetPixelIntensity(const unsigned int colorspace,
    const unsigned int method,float red,float green,float blue)
  {
    float intensity;

    if ((colorspace == GRAYColorspace) || (colorspace == LinearGRAYColorspace))
      return red;

    switch (method)
    {
      case AveragePixelIntensityMethod:
        {
          intensity=(red+green+blue)/3.0;
          break;
        }
      case BrightnessPixelIntensityMethod:
        {
          intensity=MagickMax(MagickMax(red,green),blue);
          break;
        }
      case LightnessPixelIntensityMethod:
        {
          intensity=(MagickMin(MagickMin(red,green),blue)+
              MagickMax(MagickMax(red,green),blue))/2.0;
          break;
        }
      case MSPixelIntensityMethod:
        {
          intensity=(float) (((float) red*red+green*green+blue*blue)/
              (3.0*QuantumRange));
          break;
        }
      case Rec601LumaPixelIntensityMethod:
        {
          /*
          if (image->colorspace == RGBColorspace)
          {
            red=EncodePixelGamma(red);
            green=EncodePixelGamma(green);
            blue=EncodePixelGamma(blue);
          }
          */
          intensity=0.298839*red+0.586811*green+0.114350*blue;
          break;
        }
      case Rec601LuminancePixelIntensityMethod:
        {
          /*
          if (image->colorspace == sRGBColorspace)
          {
            red=DecodePixelGamma(red);
            green=DecodePixelGamma(green);
            blue=DecodePixelGamma(blue);
          }
          */
          intensity=0.298839*red+0.586811*green+0.114350*blue;
          break;
        }
      case Rec709LumaPixelIntensityMethod:
      default:
        {
          /*
          if (image->colorspace == RGBColorspace)
          {
            red=EncodePixelGamma(red);
            green=EncodePixelGamma(green);
            blue=EncodePixelGamma(blue);
          }
          */
          intensity=0.212656*red+0.715158*green+0.072186*blue;
          break;
        }
      case Rec709LuminancePixelIntensityMethod:
        {
          /*
          if (image->colorspace == sRGBColorspace)
          {
            red=DecodePixelGamma(red);
            green=DecodePixelGamma(green);
            blue=DecodePixelGamma(blue);
          }
          */
          intensity=0.212656*red+0.715158*green+0.072186*blue;
          break;
        }
      case RMSPixelIntensityMethod:
        {
          intensity=(float) (sqrt((float) red*red+green*green+blue*blue)/
              sqrt(3.0));
          break;
        }
    }

    return intensity;
  }

  static inline int mirrorBottom(int value)
  {
      return (value < 0) ? - (value) : value;
  }

  static inline int mirrorTop(int value, int width)
  {
      return (value >= width) ? (2 * width - value - 1) : value;
  }
  )
