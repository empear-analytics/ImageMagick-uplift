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
  inside its accelerateKernels initialiser: the Blur kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    B l u r                                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
  /*
  Reduce image noise and reduce detail levels by row
  */
  __kernel void BlurRow(const __global CLQuantum *image,
    const unsigned int number_channels,const ChannelType channel,
    __constant float *filter,const unsigned int width,
    const unsigned int imageColumns,const unsigned int imageRows,
    __local float4 *temp,__global float4 *tempImage)
  {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const int columns = imageColumns;

    const unsigned int radius = (width-1)/2;
    const int wsize = get_local_size(0);
    const unsigned int loadSize = wsize+width;

    //group coordinate
    const int groupX=get_local_size(0)*get_group_id(0);

    //parallel load and clamp
    for (int i=get_local_id(0); i < loadSize; i=i+get_local_size(0))
    {
      int cx = ClampToCanvas(i + groupX - radius, columns);
      temp[i] = ReadFloat4(image, number_channels, columns, cx, y, channel);
    }

    // barrier
    barrier(CLK_LOCAL_MEM_FENCE);

    // only do the work if this is not a patched item
    if (get_global_id(0) < columns)
    {
      // compute
      float4 result = (float4) 0;

      int i = 0;

      for ( ; i+7 < width; )
      {
        for (int j=0; j < 8; j++)
          result+=filter[i+j]*temp[i+j+get_local_id(0)];
        i+=8;
      }

      for ( ; i < width; i++)
        result+=filter[i]*temp[i+get_local_id(0)];

      // write back to global
      tempImage[y*columns+x] = result;
    }
  }
  )

  STRINGIFY(
  /*
  Reduce image noise and reduce detail levels by line
  */
  __kernel void BlurColumn(const __global float4 *blurRowData,
    const unsigned int number_channels,const ChannelType channel,
    __constant float *filter,const unsigned int width,
    const unsigned int imageColumns,const unsigned int imageRows,
    __local float4 *temp,__global CLQuantum *filteredImage)
  {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const int columns = imageColumns;
    const int rows = imageRows;

    unsigned int radius = (width-1)/2;
    const int wsize = get_local_size(1);
    const unsigned int loadSize = wsize+width;

    //group coordinate
    const int groupX=get_local_size(0)*get_group_id(0);
    const int groupY=get_local_size(1)*get_group_id(1);
    //notice that get_local_size(0) is 1, so
    //groupX=get_group_id(0);

    //parallel load and clamp
    for (int i = get_local_id(1); i < loadSize; i=i+get_local_size(1))
      temp[i] = blurRowData[ClampToCanvas(i+groupY-radius, rows) * columns + groupX];

    // barrier
    barrier(CLK_LOCAL_MEM_FENCE);

    // only do the work if this is not a patched item
    if (get_global_id(1) < rows)
    {
      // compute
      float4 result = (float4) 0;

      int i = 0;

      for ( ; i+7 < width; )
      {
        for (int j=0; j < 8; j++)
          result+=filter[i+j]*temp[i+j+get_local_id(1)];
        i+=8;
      }

      for ( ; i < width; i++)
        result+=filter[i]*temp[i+get_local_id(1)];

      // write back to global
      WriteFloat4(filteredImage, number_channels, columns, x, y, channel, result);
    }
  }
  )

