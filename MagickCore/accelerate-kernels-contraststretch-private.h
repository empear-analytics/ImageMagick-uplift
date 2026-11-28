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
  inside its accelerateKernels initialiser: the ContrastStretch kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    C o n t r a s t S t r e t c h                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

    STRINGIFY(
    /*
    */
    __kernel void Histogram(__global CLPixelType * restrict im,
      const ChannelType channel,
      const unsigned int colorspace,
      const unsigned int method,
      __global uint4 * restrict histogram)
      {
        const int x = get_global_id(0);
        const int y = get_global_id(1);
        const int columns = get_global_size(0);
        const int c = x + y * columns;
        if ((channel & SyncChannels) != 0)
        {
          float red=(float)getRed(im[c]);
          float green=(float)getGreen(im[c]);
          float blue=(float)getBlue(im[c]);

          float intensity = GetPixelIntensity(colorspace, method, red, green, blue);
          uint pos = ScaleQuantumToMap(ClampToQuantum(intensity));
          atomic_inc((__global uint *)(&(histogram[pos]))+2); //red position
        }
        else
        {
          // for equalizing, we always need all channels?
          // otherwise something more
        }
      }
    )

    STRINGIFY(
    /*
    */
    __kernel void ContrastStretch(__global CLPixelType * restrict im,
      const ChannelType channel,
      __global CLPixelType * restrict stretch_map,
      const float4 white, const float4 black)
      {
        const int x = get_global_id(0);
        const int y = get_global_id(1);
        const int columns = get_global_size(0);
        const int c = x + y * columns;

        uint ePos;
        CLPixelType oValue, eValue;
        CLQuantum red, green, blue, alpha;

        //read from global
        oValue=im[c];

        if ((channel & RedChannel) != 0)
        {
          if (getRedF4(white) != getRedF4(black))
          {
            ePos = ScaleQuantumToMap(getRed(oValue));
            eValue = stretch_map[ePos];
            red = getRed(eValue);
          }
        }

        if ((channel & GreenChannel) != 0)
        {
          if (getGreenF4(white) != getGreenF4(black))
          {
            ePos = ScaleQuantumToMap(getGreen(oValue));
            eValue = stretch_map[ePos];
            green = getGreen(eValue);
          }
        }

        if ((channel & BlueChannel) != 0)
        {
          if (getBlueF4(white) != getBlueF4(black))
          {
            ePos = ScaleQuantumToMap(getBlue(oValue));
            eValue = stretch_map[ePos];
            blue = getBlue(eValue);
          }
        }

        if ((channel & AlphaChannel) != 0)
        {
          if (getAlphaF4(white) != getAlphaF4(black))
          {
            ePos = ScaleQuantumToMap(getAlpha(oValue));
            eValue = stretch_map[ePos];
            alpha = getAlpha(eValue);
          }
        }

        //write back
        im[c]=(CLPixelType)(blue, green, red, alpha);

      }
    )
