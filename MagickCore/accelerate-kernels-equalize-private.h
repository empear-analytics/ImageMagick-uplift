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
  inside its accelerateKernels initialiser: the Equalize kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E q u a l i z e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

    STRINGIFY(
    /*
    */
    __kernel void Equalize(__global CLPixelType * restrict im,
      const ChannelType channel,
      __global CLPixelType * restrict equalize_map,
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

        if ((channel & SyncChannels) != 0)
        {
          if (getRedF4(white) != getRedF4(black))
          {
            ePos = ScaleQuantumToMap(getRed(oValue));
            eValue = equalize_map[ePos];
            red = getRed(eValue);
            ePos = ScaleQuantumToMap(getGreen(oValue));
            eValue = equalize_map[ePos];
            green = getRed(eValue);
            ePos = ScaleQuantumToMap(getBlue(oValue));
            eValue = equalize_map[ePos];
            blue = getRed(eValue);
            ePos = ScaleQuantumToMap(getAlpha(oValue));
            eValue = equalize_map[ePos];
            alpha = getRed(eValue);

            //write back
            im[c]=(CLPixelType)(blue, green, red, alpha);
          }

        }

        // for equalizing, we always need all channels?
        // otherwise something more

     }
    )

