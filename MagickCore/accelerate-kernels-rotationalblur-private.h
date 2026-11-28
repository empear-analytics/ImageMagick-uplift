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
  inside its accelerateKernels2 initialiser: the RotationalBlur kernels, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R o t a t i o n a l B l u r                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
  __kernel void RotationalBlur(const __global CLQuantum *image,
    const unsigned int number_channels,const unsigned int channel,
    const float2 blurCenter,__constant float *cos_theta,
    __constant float *sin_theta,const unsigned int cossin_theta_size,
    __global CLQuantum *filteredImage)
  {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int columns = get_global_size(0);
    const int rows = get_global_size(1);
    unsigned int step = 1;
    float center_x = (float) x - blurCenter.x;
    float center_y = (float) y - blurCenter.y;
    float radius = hypot(center_x, center_y);

    float blur_radius = hypot(blurCenter.x, blurCenter.y);

    if (radius > MagickEpsilon)
    {
      step = (unsigned int) (blur_radius / radius);
      if (step == 0)
        step = 1;
      if (step >= cossin_theta_size)
        step = cossin_theta_size-1;
    }

    float4 result = 0.0f;
    float normalize = 0.0f;
    float gamma = 0.0f;

    for (unsigned int i=0; i<cossin_theta_size; i+=step)
    {
      int cx = ClampToCanvas(blurCenter.x+center_x*cos_theta[i]-center_y*sin_theta[i]+0.5f,columns);
      int cy = ClampToCanvas(blurCenter.y+center_x*sin_theta[i]+center_y*cos_theta[i]+0.5f,rows);

      float4 pixel = ReadAllChannels(image, number_channels, columns, cx, cy);

      if ((number_channels == 4) || (number_channels == 2))
      {
        float alpha = (float)(QuantumScale*pixel.w);

        gamma += alpha;

        result.x += alpha * pixel.x;
        result.y += alpha * pixel.y;
        result.z += alpha * pixel.z;
        result.w += pixel.w;
      }
      else
        result += pixel;

      normalize += 1.0f;
    }

    normalize = MagickSafeReciprocal(normalize);

    if ((number_channels == 4) || (number_channels == 2))
    {
      gamma = MagickSafeReciprocal(gamma);
      result.x *= gamma;
      result.y *= gamma;
      result.z *= gamma;
      result.w *= normalize;
    }
    else
      result *= normalize;

    WriteFloat4(filteredImage, number_channels, columns, x, y, channel, result);
  }
  )

