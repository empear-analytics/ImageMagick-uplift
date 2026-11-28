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
  inside its accelerateKernels2 initialiser: the UnsharpMask kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     U n s h a r p M a s k                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
  __kernel void UnsharpMaskBlurColumn(const __global CLQuantum* image,
    const __global float4 *blurRowData,const unsigned int number_channels,
    const ChannelType channel,const unsigned int columns,
    const unsigned int rows,__local float4* cachedData,
    __local float* cachedFilter,const __global float *filter,
    const unsigned int width,const float gain, const float threshold,
    __global CLQuantum *filteredImage)
  {
    const unsigned int radius = (width-1)/2;

    // cache the pixel shared by the workgroup
    const int groupX = get_group_id(0);
    const int groupStartY = get_group_id(1)*get_local_size(1) - radius;
    const int groupStopY = (get_group_id(1)+1)*get_local_size(1) + radius;

    if ((groupStartY >= 0) && (groupStopY < rows))
    {
      event_t e = async_work_group_strided_copy(cachedData,
        blurRowData+groupStartY*columns+groupX,groupStopY-groupStartY,columns,0);
      wait_group_events(1,&e);
    }
    else
    {
      for (int i = get_local_id(1); i < (groupStopY - groupStartY); i+=get_local_size(1))
        cachedData[i] = blurRowData[ClampToCanvas(groupStartY+i,rows)*columns + groupX];

      barrier(CLK_LOCAL_MEM_FENCE);
    }
    // cache the filter as well
    event_t e = async_work_group_copy(cachedFilter,filter,width,0);
    wait_group_events(1,&e);

    // only do the work if this is not a patched item
    const int cy = get_global_id(1);

    if (cy < rows)
    {
      float4 blurredPixel = (float4) 0.0f;

      int i = 0;

      for ( ; i+7 < width; )
      {
        for (int j=0; j < 8; j++, i++)
          blurredPixel+=cachedFilter[i+j]*cachedData[i+j+get_local_id(1)];
      }

      for ( ; i < width; i++)
        blurredPixel+=cachedFilter[i]*cachedData[i+get_local_id(1)];

      float4 inputImagePixel = ReadFloat4(image,number_channels,columns,groupX,cy,channel);
      float4 outputPixel = inputImagePixel - blurredPixel;

      float quantumThreshold = QuantumRange*threshold;

      int4 mask = isless(fabs(2.0f*outputPixel), (float4)quantumThreshold);
      outputPixel = select(inputImagePixel + outputPixel * gain, inputImagePixel, mask);

      //write back
      WriteFloat4(filteredImage,number_channels,columns,groupX,cy,channel,outputPixel);
    }
  }
  )

  STRINGIFY(
  __kernel void UnsharpMask(const __global CLQuantum *image,const unsigned int number_channels,
    const ChannelType channel,__constant float *filter,const unsigned int width,
    const unsigned int columns,const unsigned int rows,__local float4 *pixels,
    const float gain,const float threshold,__global CLQuantum *filteredImage)
  {
    const unsigned int x = get_global_id(0);
    const unsigned int y = get_global_id(1);

    const unsigned int radius = (width - 1) / 2;

    int row = y - radius;
    int baseRow = get_group_id(1) * get_local_size(1) - radius;
    int endRow = (get_group_id(1) + 1) * get_local_size(1) + radius;

    while (row < endRow) {
      int srcy = (row < 0) ? -row : row; // mirror pad
      srcy = (srcy >= rows) ? (2 * rows - srcy - 1) : srcy;

      float4 value = 0.0f;

      int ix = x - radius;
      int i = 0;

      while (i + 7 < width) {
        for (int j = 0; j < 8; ++j) { // unrolled
          int srcx = ix + j;
          srcx = (srcx < 0) ? -srcx : srcx;
          srcx = (srcx >= columns) ? (2 * columns - srcx - 1) : srcx;
          value += filter[i + j] * ReadFloat4(image, number_channels, columns, srcx, srcy, channel);
        }
        ix += 8;
        i += 8;
      }

      while (i < width) {
        int srcx = (ix < 0) ? -ix : ix; // mirror pad
        srcx = (srcx >= columns) ? (2 * columns - srcx - 1) : srcx;
        value += filter[i] * ReadFloat4(image, number_channels, columns, srcx, srcy, channel);
        ++i;
        ++ix;
      }
      pixels[(row - baseRow) * get_local_size(0) + get_local_id(0)] = value;
      row += get_local_size(1);
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    const int px = get_local_id(0);
    const int py = get_local_id(1);
    const int prp = get_local_size(0);
    float4 value = (float4)(0.0f);

    int i = 0;
    while (i + 7 < width) {
      for (int j = 0; j < 8; ++j) // unrolled
        value += (float4)(filter[i]) * pixels[px + (py + i + j) * prp];
      i += 8;
    }
    while (i < width) {
      value += (float4)(filter[i]) * pixels[px + (py + i) * prp];
      ++i;
    }

    if ((x < columns) && (y < rows)) {
      float4 srcPixel = ReadFloat4(image, number_channels, columns, x, y, channel);
      float4 diff = srcPixel - value;

      float quantumThreshold = QuantumRange*threshold;

      int4 mask = isless(fabs(2.0f * diff), (float4)quantumThreshold);
      value = select(srcPixel + diff * gain, srcPixel, mask);

      WriteFloat4(filteredImage, number_channels, columns, x, y, channel, value);
    }
  }
  )

