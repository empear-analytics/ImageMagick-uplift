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
  inside its accelerateKernels initialiser: the MotionBlur kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o t i o n B l u r                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
    __kernel
    void MotionBlur(const __global CLPixelType *input, __global CLPixelType *output,
                    const unsigned int imageWidth, const unsigned int imageHeight,
                    const __global float *filter, const unsigned int width, const __global int2* offset,
                    const float4 bias,
                    const ChannelType channel, const unsigned int matte) {

      int2 currentPixel;
      currentPixel.x = get_global_id(0);
      currentPixel.y = get_global_id(1);

      if (currentPixel.x >= imageWidth
          || currentPixel.y >= imageHeight)
          return;

      float4 pixel;
      pixel.x = (float)bias.x;
      pixel.y = (float)bias.y;
      pixel.z = (float)bias.z;
      pixel.w = (float)bias.w;

      if (((channel & AlphaChannel) == 0) || (matte == 0)) {

        for (int i = 0; i < width; i++) {
          // only support EdgeVirtualPixelMethod through ClampToCanvas
          // TODO: implement other virtual pixel method
          int2 samplePixel = currentPixel + offset[i];
          samplePixel.x = ClampToCanvas(samplePixel.x, imageWidth);
          samplePixel.y = ClampToCanvas(samplePixel.y, imageHeight);
          CLPixelType samplePixelValue = input[ samplePixel.y * imageWidth + samplePixel.x];

          pixel.x += (filter[i] * (float)samplePixelValue.x);
          pixel.y += (filter[i] * (float)samplePixelValue.y);
          pixel.z += (filter[i] * (float)samplePixelValue.z);
          pixel.w += (filter[i] * (float)samplePixelValue.w);
        }

        CLPixelType outputPixel;
        outputPixel.x = ClampToQuantum(pixel.x);
        outputPixel.y = ClampToQuantum(pixel.y);
        outputPixel.z = ClampToQuantum(pixel.z);
        outputPixel.w = ClampToQuantum(pixel.w);
        output[currentPixel.y * imageWidth + currentPixel.x] = outputPixel;
      }
      else {

        float gamma = 0.0f;
        for (int i = 0; i < width; i++) {
          // only support EdgeVirtualPixelMethod through ClampToCanvas
          // TODO: implement other virtual pixel method
          int2 samplePixel = currentPixel + offset[i];
          samplePixel.x = ClampToCanvas(samplePixel.x, imageWidth);
          samplePixel.y = ClampToCanvas(samplePixel.y, imageHeight);

          CLPixelType samplePixelValue = input[ samplePixel.y * imageWidth + samplePixel.x];

          float alpha = QuantumScale*samplePixelValue.w;
          float k = filter[i];
          pixel.x = pixel.x + k * alpha * samplePixelValue.x;
          pixel.y = pixel.y + k * alpha * samplePixelValue.y;
          pixel.z = pixel.z + k * alpha * samplePixelValue.z;

          pixel.w += k * alpha * samplePixelValue.w;

          gamma+=k*alpha;
        }
        gamma = MagickSafeReciprocal(gamma);
        pixel.xyz = gamma*pixel.xyz;

        CLPixelType outputPixel;
        outputPixel.x = ClampToQuantum(pixel.x);
        outputPixel.y = ClampToQuantum(pixel.y);
        outputPixel.z = ClampToQuantum(pixel.z);
        outputPixel.w = ClampToQuantum(pixel.w);
        output[currentPixel.y * imageWidth + currentPixel.x] = outputPixel;
      }
    }
  )

