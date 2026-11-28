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
  inside its accelerateKernels initialiser: the Function kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     F u n c t i o n                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
  /*
  apply FunctionImageChannel(brightness-contrast)
  */
  CLQuantum ApplyFunction(float pixel,const MagickFunction function,
    const unsigned int number_parameters,__constant float *parameters)
  {
    float result = 0.0f;

    switch (function)
    {
    case PolynomialFunction:
      {
        for (unsigned int i=0; i < number_parameters; i++)
          result = result*QuantumScale*pixel + parameters[i];
        result *= QuantumRange;
        break;
      }
    case SinusoidFunction:
      {
        float  freq,phase,ampl,bias;
        freq  = ( number_parameters >= 1 ) ? parameters[0] : 1.0f;
        phase = ( number_parameters >= 2 ) ? parameters[1] : 0.0f;
        ampl  = ( number_parameters >= 3 ) ? parameters[2] : 0.5f;
        bias  = ( number_parameters >= 4 ) ? parameters[3] : 0.5f;
        result = QuantumRange*(ampl*sin(2.0f*MagickPI*
          (freq*QuantumScale*pixel + phase/360.0f)) + bias);
        break;
      }
    case ArcsinFunction:
      {
        float  width,range,center,bias;
        width  = ( number_parameters >= 1 ) ? parameters[0] : 1.0f;
        center = ( number_parameters >= 2 ) ? parameters[1] : 0.5f;
        range  = ( number_parameters >= 3 ) ? parameters[2] : 1.0f;
        bias   = ( number_parameters >= 4 ) ? parameters[3] : 0.5f;

        result = 2.0f/width*(QuantumScale*pixel - center);
        result = range/MagickPI*asin(result)+bias;
        result = ( result <= -1.0f ) ? bias - range/2.0f : result;
        result = ( result >= 1.0f ) ? bias + range/2.0f : result;
        result *= QuantumRange;
        break;
      }
    case ArctanFunction:
      {
        float slope,range,center,bias;
        slope  = ( number_parameters >= 1 ) ? parameters[0] : 1.0f;
        center = ( number_parameters >= 2 ) ? parameters[1] : 0.5f;
        range  = ( number_parameters >= 3 ) ? parameters[2] : 1.0f;
        bias   = ( number_parameters >= 4 ) ? parameters[3] : 0.5f;
        result = MagickPI*slope*(QuantumScale*pixel-center);
        result = QuantumRange*(range/MagickPI*atan(result) + bias);
        break;
      }
    case UndefinedFunction:
      break;
    }
    return(ClampToQuantum(result));
  }
  )

  STRINGIFY(
  /*
  Improve brightness / contrast of the image
  channel : define which channel is improved
  function : the function called to enhance the brightness contrast
  number_parameters : numbers of parameters
  parameters : the parameter
  */
  __kernel void ComputeFunction(__global CLQuantum *image,const unsigned int number_channels,
    const ChannelType channel,const MagickFunction function,const unsigned int number_parameters,
    __constant float *parameters)
  {
    const unsigned int x = get_global_id(0);
    const unsigned int y = get_global_id(1);
    const unsigned int columns = get_global_size(0);
    __global CLQuantum *p = image + getPixelIndex(number_channels, columns, x, y);

    float red;
    float green;
    float blue;
    float alpha;

    ReadChannels(p, number_channels, channel, &red, &green, &blue, &alpha);

    if ((channel & RedChannel) != 0)
      red=ApplyFunction(red, function, number_parameters, parameters);

    if (number_channels > 2)
      {
        if ((channel & GreenChannel) != 0)
          green=ApplyFunction(green, function, number_parameters, parameters);

        if ((channel & BlueChannel) != 0)
          blue=ApplyFunction(blue, function, number_parameters, parameters);
      }

    if (((number_channels == 4) || (number_channels == 2)) &&
        ((channel & AlphaChannel) != 0))
      alpha=ApplyFunction(alpha, function, number_parameters, parameters);

    WriteChannels(p, number_channels, channel, red, green, blue, alpha);
  }
  )

