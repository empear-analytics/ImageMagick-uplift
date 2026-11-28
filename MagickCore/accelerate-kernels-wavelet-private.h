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
  inside its accelerateKernels2 initialiser: the WaveletDenoise kernel, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    W a v e l e t D e n o i s e                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
    __kernel __attribute__((reqd_work_group_size(64, 4, 1)))
    void WaveletDenoise(__global CLQuantum *srcImage,__global CLQuantum *dstImage,
      const unsigned int number_channels,const unsigned int max_channels,
      const float threshold,const int passes,const unsigned int imageWidth,
      const unsigned int imageHeight)
  {
    const int pad = (1 << (passes - 1));
    const int tileSize = 64;
    const int tileRowPixels = 64;
    const float noise[] = { 0.8002, 0.2735, 0.1202, 0.0585, 0.0291, 0.0152, 0.0080, 0.0044 };

    CLQuantum stage[48]; // 16 * 3 (we only need 3 channels)

    local float buffer[64 * 64];

    int srcx = (get_group_id(0) + get_global_offset(0) / tileSize) * (tileSize - 2 * pad) - pad + get_local_id(0);
    int srcy = (get_group_id(1) + get_global_offset(1) / 4) * (tileSize - 2 * pad) - pad;

    for (int i = get_local_id(1); i < tileSize; i += get_local_size(1)) {
      int pos = (mirrorTop(mirrorBottom(srcx), imageWidth) * number_channels) +
                (mirrorTop(mirrorBottom(srcy + i), imageHeight)) * imageWidth * number_channels;

      for (int channel = 0; channel < max_channels; ++channel)
        stage[(i / 4) + (16 * channel)] = srcImage[pos + channel];
    }

    for (int channel = 0; channel < max_channels; ++channel) {
      // Load LDS
      for (int i = get_local_id(1); i < tileSize; i += get_local_size(1))
        buffer[get_local_id(0) + i * tileRowPixels] = convert_float(stage[(i / 4) + (16 * channel)]);

      // Process

      float tmp[16];
      float accum[16];
      float pixel;

      for (int i = 0; i < 16; i++)
        accum[i]=0.0f;

      for (int pass = 0; pass < passes; ++pass) {
        const int radius = 1 << pass;
        const int x = get_local_id(0);
        const float thresh = threshold * noise[pass];

        // Apply horizontal hat
        for (int i = get_local_id(1); i < tileSize; i += get_local_size(1)) {
          const int offset = i * tileRowPixels;
          if (pass == 0)
            tmp[i / 4] = buffer[x + offset]; // snapshot input on first pass
          pixel = 0.5f * tmp[i / 4] + 0.25 * (buffer[mirrorBottom(x - radius) + offset] + buffer[mirrorTop(x + radius, tileSize) + offset]);
          barrier(CLK_LOCAL_MEM_FENCE);
          buffer[x + offset] = pixel;
        }
        barrier(CLK_LOCAL_MEM_FENCE);

        // Apply vertical hat
        for (int i = get_local_id(1); i < tileSize; i += get_local_size(1)) {
          pixel = 0.5f * buffer[x + i * tileRowPixels] + 0.25 * (buffer[x + mirrorBottom(i - radius) * tileRowPixels] + buffer[x + mirrorTop(i + radius, tileRowPixels) * tileRowPixels]);
          float delta = tmp[i / 4] - pixel;
          tmp[i / 4] = pixel; // hold output in tmp until all workitems are done
          if (delta < -thresh)
            delta += thresh;
          else if (delta > thresh)
            delta -= thresh;
          else
            delta = 0;
          accum[i / 4] += delta;
        }
        barrier(CLK_LOCAL_MEM_FENCE);

        if (pass < passes - 1)
          for (int i = get_local_id(1); i < tileSize; i += get_local_size(1))
            buffer[x + i * tileRowPixels] = tmp[i / 4]; // store lowpass for next pass
        else  // last pass
          for (int i = get_local_id(1); i < tileSize; i += get_local_size(1))
            accum[i / 4] += tmp[i / 4]; // add the lowpass signal back to output
        barrier(CLK_LOCAL_MEM_FENCE);
      }

      for (int i = get_local_id(1); i < tileSize; i += get_local_size(1))
        stage[(i / 4) + (16 * channel)] = ClampToQuantum(accum[i / 4]);

      barrier(CLK_LOCAL_MEM_FENCE);
    }

    // Write from stage to output

    if ((get_local_id(0) >= pad) && (get_local_id(0) < tileSize - pad) && (srcx >= 0) && (srcx < imageWidth)) {
      for (int i = get_local_id(1); i < tileSize; i += get_local_size(1)) {
        if ((i >= pad) && (i < tileSize - pad) && (srcy + i >= 0) && (srcy + i < imageHeight)) {
          int pos = (srcx * number_channels) + ((srcy + i) * (imageWidth * number_channels));
          for (int channel = 0; channel < max_channels; ++channel) {
            dstImage[pos + channel] = stage[(i / 4) + (16 * channel)];
          }
        }
      }
    }
  }
  )
