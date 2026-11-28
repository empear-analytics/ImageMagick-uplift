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

  MagickCore private kernels for accelerated functions.
*/

#ifndef MAGICKCORE_ACCELERATE_KERNELS_PRIVATE_H
#define MAGICKCORE_ACCELERATE_KERNELS_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_OPENCL_SUPPORT)

/*
  Define declarations.
*/
#define OPENCL_DEFINE(VAR,...)	"\n #""define " #VAR " " #__VA_ARGS__ " \n"
#define OPENCL_ELIF(...)	"\n #""elif " #__VA_ARGS__ " \n"
#define OPENCL_ELSE()		"\n #""else " " \n"
#define OPENCL_ENDIF()		"\n #""endif " " \n"
#define OPENCL_IF(...)		"\n #""if " #__VA_ARGS__ " \n"
#define STRINGIFY(...) #__VA_ARGS__ "\n"

const char *accelerateKernels =

/*
  Define declarations.
*/
  OPENCL_DEFINE(SigmaUniform, (attenuate*0.015625f))
  OPENCL_DEFINE(SigmaGaussian, (attenuate*0.015625f))
  OPENCL_DEFINE(SigmaImpulse, (attenuate*0.1f))
  OPENCL_DEFINE(SigmaLaplacian, (attenuate*0.0390625f))
  OPENCL_DEFINE(SigmaMultiplicativeGaussian, (attenuate*0.5f))
  OPENCL_DEFINE(SigmaPoisson, (attenuate*12.5f))
  OPENCL_DEFINE(SigmaRandom, (attenuate))
  OPENCL_DEFINE(TauGaussian, (attenuate*0.078125f))
  OPENCL_DEFINE(MagickMax(x,y), (((x) > (y)) ? (x) : (y)))
  OPENCL_DEFINE(MagickMin(x,y), (((x) < (y)) ? (x) : (y)))
  OPENCL_DEFINE(QuantumScale, (1.0/QuantumRange))

/*
  Typedef declarations.
*/
  STRINGIFY(
    typedef enum
    {
      UndefinedColorspace,
      CMYColorspace,           /* negated linear RGB colorspace */
      CMYKColorspace,          /* CMY with Black separation */
      GRAYColorspace,          /* Single Channel greyscale (non-linear) image */
      HCLColorspace,
      HCLpColorspace,
      HSBColorspace,
      HSIColorspace,
      HSLColorspace,
      HSVColorspace,           /* alias for HSB */
      HWBColorspace,
      LabColorspace,
      LCHColorspace,           /* alias for LCHuv */
      LCHabColorspace,         /* Cylindrical (Polar) Lab */
      LCHuvColorspace,         /* Cylindrical (Polar) Luv */
      LogColorspace,
      LMSColorspace,
      LuvColorspace,
      OHTAColorspace,
      Rec601YCbCrColorspace,
      Rec709YCbCrColorspace,
      RGBColorspace,           /* Linear RGB colorspace */
      scRGBColorspace,         /* ??? */
      sRGBColorspace,          /* Default: non-linear sRGB colorspace */
      TransparentColorspace,
      xyYColorspace,
      XYZColorspace,           /* IEEE Color Reference colorspace */
      YCbCrColorspace,
      YCCColorspace,
      YDbDrColorspace,
      YIQColorspace,
      YPbPrColorspace,
      YUVColorspace,
      LinearGRAYColorspace     /* Single Channel greyscale (linear) image */
    } ColorspaceType;
  )

  STRINGIFY(
    typedef enum
    {
      UndefinedCompositeOp,
      AlphaCompositeOp,
      AtopCompositeOp,
      BlendCompositeOp,
      BlurCompositeOp,
      BumpmapCompositeOp,
      ChangeMaskCompositeOp,
      ClearCompositeOp,
      ColorBurnCompositeOp,
      ColorDodgeCompositeOp,
      ColorizeCompositeOp,
      CopyBlackCompositeOp,
      CopyBlueCompositeOp,
      CopyCompositeOp,
      CopyCyanCompositeOp,
      CopyGreenCompositeOp,
      CopyMagentaCompositeOp,
      CopyAlphaCompositeOp,
      CopyRedCompositeOp,
      CopyYellowCompositeOp,
      DarkenCompositeOp,
      DarkenIntensityCompositeOp,
      DifferenceCompositeOp,
      DisplaceCompositeOp,
      DissolveCompositeOp,
      DistortCompositeOp,
      DivideDstCompositeOp,
      DivideSrcCompositeOp,
      DstAtopCompositeOp,
      DstCompositeOp,
      DstInCompositeOp,
      DstOutCompositeOp,
      DstOverCompositeOp,
      ExclusionCompositeOp,
      HardLightCompositeOp,
      HardMixCompositeOp,
      HueCompositeOp,
      InCompositeOp,
      IntensityCompositeOp,
      LightenCompositeOp,
      LightenIntensityCompositeOp,
      LinearBurnCompositeOp,
      LinearDodgeCompositeOp,
      LinearLightCompositeOp,
      LuminizeCompositeOp,
      MathematicsCompositeOp,
      MinusDstCompositeOp,
      MinusSrcCompositeOp,
      ModulateCompositeOp,
      ModulusAddCompositeOp,
      ModulusSubtractCompositeOp,
      MultiplyCompositeOp,
      NoCompositeOp,
      OutCompositeOp,
      OverCompositeOp,
      OverlayCompositeOp,
      PegtopLightCompositeOp,
      PinLightCompositeOp,
      PlusCompositeOp,
      ReplaceCompositeOp,
      SaturateCompositeOp,
      ScreenCompositeOp,
      SoftLightCompositeOp,
      SrcAtopCompositeOp,
      SrcCompositeOp,
      SrcInCompositeOp,
      SrcOutCompositeOp,
      SrcOverCompositeOp,
      ThresholdCompositeOp,
      VividLightCompositeOp,
      XorCompositeOp,
      StereoCompositeOp
    } CompositeOperator;
  )

  STRINGIFY(
    typedef enum
    {
      UndefinedFunction,
      ArcsinFunction,
      ArctanFunction,
      PolynomialFunction,
      SinusoidFunction
    } MagickFunction;
  )

  STRINGIFY(
    typedef enum
    {
      UndefinedNoise,
      UniformNoise,
      GaussianNoise,
      MultiplicativeGaussianNoise,
      ImpulseNoise,
      LaplacianNoise,
      PoissonNoise,
      RandomNoise
    } NoiseType;
  )

  STRINGIFY(
    typedef enum
    {
      UndefinedPixelIntensityMethod = 0,
      AveragePixelIntensityMethod,
      BrightnessPixelIntensityMethod,
      LightnessPixelIntensityMethod,
      MSPixelIntensityMethod,
      Rec601LumaPixelIntensityMethod,
      Rec601LuminancePixelIntensityMethod,
      Rec709LumaPixelIntensityMethod,
      Rec709LuminancePixelIntensityMethod,
      RMSPixelIntensityMethod
    } PixelIntensityMethod;
  )

  STRINGIFY(
    typedef enum
    {
      BoxWeightingFunction = 0,
      TriangleWeightingFunction,
      CubicBCWeightingFunction,
      HannWeightingFunction,
      HammingWeightingFunction,
      BlackmanWeightingFunction,
      GaussianWeightingFunction,
      QuadraticWeightingFunction,
      JincWeightingFunction,
      SincWeightingFunction,
      SincFastWeightingFunction,
      KaiserWeightingFunction,
      WelchWeightingFunction,
      BohmanWeightingFunction,
      LagrangeWeightingFunction,
      CosineWeightingFunction
    } ResizeWeightingFunctionType;
  )

  STRINGIFY(
    typedef enum
    {
      UndefinedChannel = 0x0000,
      RedChannel = 0x0001,
      GrayChannel = 0x0001,
      CyanChannel = 0x0001,
      GreenChannel = 0x0002,
      MagentaChannel = 0x0002,
      BlueChannel = 0x0004,
      YellowChannel = 0x0004,
      BlackChannel = 0x0008,
      AlphaChannel = 0x0010,
      OpacityChannel = 0x0010,
      IndexChannel = 0x0020,             /* Color Index Table? */
      ReadMaskChannel = 0x0040,          /* Pixel is Not Readable? */
      WriteMaskChannel = 0x0080,         /* Pixel is Write Protected? */
      MetaChannel = 0x0100,              /* ???? */
      CompositeChannels = 0x001F,
      AllChannels = 0x7ffffff, /* 0x7FFFFFFFFFFFFFFF for 64-bit channel masks */
      /*
        Special purpose channel types.
        FUTURE: are these needed any more - they are more like hacks
        SyncChannels for example is NOT a real channel but a 'flag'
        It really says -- "User has not defined channels"
        Though it does have extra meaning in the "-auto-level" operator
      */
      TrueAlphaChannel = 0x0100, /* extract actual alpha channel from opacity */
      RGBChannels = 0x0200,      /* set alpha from grayscale mask in RGB */
      GrayChannels = 0x0400,
      SyncChannels = 0x20000,    /* channels modified as a single unit */
      DefaultChannels = AllChannels
    } ChannelType;  /* must correspond to PixelChannel */
  )

/*
  Helper functions.
*/

OPENCL_IF((MAGICKCORE_QUANTUM_DEPTH == 8))

  STRINGIFY(
    static inline CLQuantum ScaleCharToQuantum(const unsigned char value)
    {
      return((CLQuantum) value);
    }
  )

OPENCL_ELIF((MAGICKCORE_QUANTUM_DEPTH == 16))

  STRINGIFY(
    static inline CLQuantum ScaleCharToQuantum(const unsigned char value)
    {
      return((CLQuantum) (257.0f*value));
    }
  )

OPENCL_ELIF((MAGICKCORE_QUANTUM_DEPTH == 32))

  STRINGIFY(
    static inline CLQuantum ScaleCharToQuantum(const unsigned char value)
    {
      return((CLQuantum) (16843009.0*value));
    }
  )

OPENCL_ENDIF()

OPENCL_IF((MAGICKCORE_HDRI_SUPPORT == 1))

  STRINGIFY(
    static inline CLQuantum ClampToQuantum(const float value)
      {
        return (CLQuantum) clamp(value, 0.0f, QuantumRange);
      }
  )

OPENCL_ELSE()

  STRINGIFY(
    static inline CLQuantum ClampToQuantum(const float value)
      {
        return (CLQuantum) (clamp(value, 0.0f, QuantumRange) + 0.5f);
      }
  )

OPENCL_ENDIF()

  STRINGIFY(
    static inline int ClampToCanvas(const int offset,const int range)
      {
        return clamp(offset, (int)0, range-1);
      }
  )

  STRINGIFY(
    static inline uint ScaleQuantumToMap(CLQuantum value)
      {
        if (value >= (CLQuantum) MaxMap)
          return ((uint)MaxMap);
        else
          return ((uint)value);
      }
  )

  STRINGIFY(
    static inline float MagickSafeReciprocal(const float x)
    {
      float sign = x < (float) 0.0 ? (float) -1.0 : (float) 1.0;
      return((sign*x) >= MagickEpsilon ? (float) 1.0/x : sign*((float) 1.0/MagickEpsilon));
    }
  )

#include "MagickCore/accelerate-kernels-pixel-private.h"

#include "MagickCore/accelerate-kernels-noise-private.h"
#include "MagickCore/accelerate-kernels-blur-private.h"
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    C o n t r a s t                                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(

  static inline float4 ConvertRGBToHSB(const float4 pixel)
  {
    float4 result=0.0f;
    result.w=pixel.w;
    float tmax=MagickMax(MagickMax(pixel.x,pixel.y),pixel.z);
    if (tmax != 0.0f)
    {
      float tmin=MagickMin(MagickMin(pixel.x,pixel.y),pixel.z);
      float delta=tmax-tmin;

      result.y=delta/tmax;
      result.z=QuantumScale*tmax;
      if (delta != 0.0f)
      {
        result.x =((pixel.x == tmax) ? 0.0f : ((pixel.y == tmax) ? 2.0f : 4.0f));
        result.x+=((pixel.x == tmax) ? (pixel.y-pixel.z) : ((pixel.y == tmax) ?
          (pixel.z-pixel.x) : (pixel.x-pixel.y)))/delta;
        result.x/=6.0f;
        result.x+=(result.x < 0.0f) ? 0.0f : 1.0f;
      }
    }
    return(result);
  }

  static inline float4 ConvertHSBToRGB(const float4 pixel)
  {
    float hue=pixel.x;
    float saturation=pixel.y;
    float brightness=pixel.z;

    float4 result=pixel;

    if (saturation == 0.0f)
    {
      result.x=result.y=result.z=ClampToQuantum(QuantumRange*brightness);
    }
    else
    {
      float h=6.0f*(hue-floor(hue));
      float f=h-floor(h);
      float p=brightness*(1.0f-saturation);
      float q=brightness*(1.0f-saturation*f);
      float t=brightness*(1.0f-(saturation*(1.0f-f)));
      int ih = (int)h;

      if (ih == 1)
      {
        result.x=ClampToQuantum(QuantumRange*q);
        result.y=ClampToQuantum(QuantumRange*brightness);
        result.z=ClampToQuantum(QuantumRange*p);
      }
      else if (ih == 2)
      {
        result.x=ClampToQuantum(QuantumRange*p);
        result.y=ClampToQuantum(QuantumRange*brightness);
        result.z=ClampToQuantum(QuantumRange*t);
      }
      else if (ih == 3)
      {
        result.x=ClampToQuantum(QuantumRange*p);
        result.y=ClampToQuantum(QuantumRange*q);
        result.z=ClampToQuantum(QuantumRange*brightness);
      }
      else if (ih == 4)
      {
        result.x=ClampToQuantum(QuantumRange*t);
        result.y=ClampToQuantum(QuantumRange*p);
        result.z=ClampToQuantum(QuantumRange*brightness);
      }
      else if (ih == 5)
      {
        result.x=ClampToQuantum(QuantumRange*brightness);
        result.y=ClampToQuantum(QuantumRange*p);
        result.z=ClampToQuantum(QuantumRange*q);
      }
      else
      {
        result.x=ClampToQuantum(QuantumRange*brightness);
        result.y=ClampToQuantum(QuantumRange*t);
        result.z=ClampToQuantum(QuantumRange*p);
      }
    }
    return(result);
  }

  __kernel void Contrast(__global CLQuantum *image,
    const unsigned int number_channels,const int sign)
  {
    const int x=get_global_id(0);
    const int y=get_global_id(1);
    const unsigned int columns=get_global_size(0);

    float4 pixel=ReadAllChannels(image,number_channels,columns,x,y);
    if (number_channels < 3)
      pixel.y=pixel.z=pixel.x;

    pixel=ConvertRGBToHSB(pixel);
    float brightness=pixel.z;
    brightness+=0.5f*sign*(0.5f*(sinpi(brightness-0.5f)+1.0f)-brightness);
    brightness=clamp(brightness,0.0f,1.0f);
    pixel.z=brightness;
    pixel=ConvertHSBToRGB(pixel);

    WriteAllChannels(image,number_channels,columns,x,y,pixel);
  }
  )

#include "MagickCore/accelerate-kernels-contraststretch-private.h"
#include "MagickCore/accelerate-kernels-despeckle-private.h"
#include "MagickCore/accelerate-kernels-equalize-private.h"
#include "MagickCore/accelerate-kernels-function-private.h"
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     G r a y s c a l e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
  __kernel void Grayscale(__global CLQuantum *image,const int number_channels,
    const unsigned int colorspace,const unsigned int method)
  {
    const unsigned int x = get_global_id(0);
    const unsigned int y = get_global_id(1);
    const unsigned int columns = get_global_size(0);
    __global CLQuantum *p = image + getPixelIndex(number_channels, columns, x, y);

    float
      blue,
      green,
      red;

    red=getPixelRed(p);
    green=getPixelGreen(p);
    blue=getPixelBlue(p);

    CLQuantum intensity=ClampToQuantum(GetPixelIntensity(colorspace, method, red, green, blue));

    setPixelRed(p,intensity);
    setPixelGreen(p,intensity);
    setPixelBlue(p,intensity);
  }
  )

#include "MagickCore/accelerate-kernels-localcontrast-private.h"
#include "MagickCore/accelerate-kernels-modulate-private.h"
#include "MagickCore/accelerate-kernels-motionblur-private.h"
#include "MagickCore/accelerate-kernels-resizefilter-private.h"

  ;
  const char *accelerateKernels2 =

  STRINGIFY(

  static inline unsigned int getNumWorkItemsPerPixel(const unsigned int pixelPerWorkgroup, const unsigned int numWorkItems) {
    return (numWorkItems/pixelPerWorkgroup);
  }

  // returns the index of the pixel for the current workitem to compute.
  // returns -1 if this workitem doesn't need to participate in any computation
  static inline int pixelToCompute(const unsigned itemID, const unsigned int pixelPerWorkgroup, const unsigned int numWorkItems) {
    const unsigned int numWorkItemsPerPixel = getNumWorkItemsPerPixel(pixelPerWorkgroup, numWorkItems);
    int pixelIndex = itemID/numWorkItemsPerPixel;
    pixelIndex = (pixelIndex<pixelPerWorkgroup)?pixelIndex:-1;
    return pixelIndex;
  }

  )

#include "MagickCore/accelerate-kernels-resizehorizontal-private.h"

#include "MagickCore/accelerate-kernels-resizevertical-private.h"

#include "MagickCore/accelerate-kernels-rotationalblur-private.h"
#include "MagickCore/accelerate-kernels-unsharpmask-private.h"
#include "MagickCore/accelerate-kernels-wavelet-private.h"

  ;

#endif // MAGICKCORE_OPENCL_SUPPORT

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif // MAGICKCORE_ACCELERATE_KERNELS_PRIVATE_H
