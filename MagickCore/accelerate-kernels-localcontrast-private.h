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
  inside its accelerateKernels initialiser: the LocalContrast kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L o c a l C o n t r a s t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

    STRINGIFY(

      __kernel void LocalContrastBlurRow(__global CLPixelType *srcImage, __global CLPixelType *dstImage, __global float *tmpImage,
          const int radius,
          const int imageWidth,
          const int imageHeight)
      {
        const float4 RGB = ((float4)(0.2126f, 0.7152f, 0.0722f, 0.0f));

        int x = get_local_id(0);
        int y = get_global_id(1);

        if ((x >= imageWidth) || (y >= imageHeight))
          return;

        global CLPixelType *src = srcImage + y * imageWidth;

        for (int i = x; i < imageWidth; i += get_local_size(0)) {
            float sum = 0.0f;
            float weight = 1.0f;

            int j = i - radius;
            while ((j + 7) < i) {
                for (int k = 0; k < 8; ++k) // Unroll 8x
                    sum += (weight + k) * dot(RGB, convert_float4(src[mirrorBottom(j+k)]));
                weight += 8.0f;
                j+=8;
            }
            while (j < i) {
                sum += weight * dot(RGB, convert_float4(src[mirrorBottom(j)]));
                weight += 1.0f;
                ++j;
            }

            while ((j + 7) < radius + i) {
                for (int k = 0; k < 8; ++k) // Unroll 8x
                    sum += (weight - k) * dot(RGB, convert_float4(src[mirrorTop(j + k, imageWidth)]));
                weight -= 8.0f;
                j+=8;
            }
            while (j < radius + i) {
                sum += weight * dot(RGB, convert_float4(src[mirrorTop(j, imageWidth)]));
                weight -= 1.0f;
                ++j;
            }

            tmpImage[i + y * imageWidth] = sum / ((radius + 1) * (radius + 1));
        }
      }
    )

    STRINGIFY(
      __kernel void LocalContrastBlurApplyColumn(__global CLPixelType *srcImage, __global CLPixelType *dstImage, __global float *blurImage,
          const int radius,
          const float strength,
          const int imageWidth,
          const int imageHeight)
      {
        const float4 RGB = (float4)(0.2126f, 0.7152f, 0.0722f, 0.0f);

        int x = get_global_id(0);
        int y = get_global_id(1);

        if ((x >= imageWidth) || (y >= imageHeight))
                return;

        global float *src = blurImage + x;

        float sum = 0.0f;
        float weight = 1.0f;

        int j = y - radius;
        while ((j + 7) < y) {
            for (int k = 0; k < 8; ++k) // Unroll 8x
                sum += (weight + k) * src[mirrorBottom(j+k) * imageWidth];
            weight += 8.0f;
            j+=8;
        }
        while (j < y) {
            sum += weight * src[mirrorBottom(j) * imageWidth];
            weight += 1.0f;
            ++j;
        }

        while ((j + 7) < radius + y) {
            for (int k = 0; k < 8; ++k) // Unroll 8x
                sum += (weight - k) * src[mirrorTop(j + k, imageHeight) * imageWidth];
            weight -= 8.0f;
            j+=8;
        }
        while (j < radius + y) {
            sum += weight * src[mirrorTop(j, imageHeight) * imageWidth];
            weight -= 1.0f;
            ++j;
        }

        CLPixelType pixel = srcImage[x + y * imageWidth];
        float srcVal = dot(RGB, convert_float4(pixel));
        float mult = (srcVal - (sum / ((radius + 1) * (radius + 1)))) * (strength / 100.0f);
        mult = (srcVal + mult) / srcVal;

        pixel.x = ClampToQuantum(pixel.x * mult);
        pixel.y = ClampToQuantum(pixel.y * mult);
        pixel.z = ClampToQuantum(pixel.z * mult);

        dstImage[x + y * imageWidth] = pixel;
      }
    )

