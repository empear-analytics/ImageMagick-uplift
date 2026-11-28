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
  inside its accelerateKernels initialiser: the resize filter weighting functions, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R e s i z e                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

  STRINGIFY(
  // Based on Box from resize.c
  float BoxResizeFilter(const float x)
  {
    return 1.0f;
  }
  )

  STRINGIFY(
  // Based on CubicBC from resize.c
  float CubicBC(const float x,const __global float* resizeFilterCoefficients)
  {
    /*
    Cubic Filters using B,C determined values:
    Mitchell-Netravali  B = 1/3 C = 1/3  "Balanced" cubic spline filter
    Catmull-Rom         B = 0   C = 1/2  Interpolatory and exact on linears
    Spline              B = 1   C = 0    B-Spline Gaussian approximation
    Hermite             B = 0   C = 0    B-Spline interpolator

    See paper by Mitchell and Netravali, Reconstruction Filters in Computer
    Graphics Computer Graphics, Volume 22, Number 4, August 1988
    http://www.cs.utexas.edu/users/fussell/courses/cs384g/lectures/mitchell/
    Mitchell.pdf.

    Coefficients are determined from B,C values:
    P0 = (  6 - 2*B       )/6 = coeff[0]
    P1 =         0
    P2 = (-18 +12*B + 6*C )/6 = coeff[1]
    P3 = ( 12 - 9*B - 6*C )/6 = coeff[2]
    Q0 = (      8*B +24*C )/6 = coeff[3]
    Q1 = (    -12*B -48*C )/6 = coeff[4]
    Q2 = (      6*B +30*C )/6 = coeff[5]
    Q3 = (    - 1*B - 6*C )/6 = coeff[6]

    which are used to define the filter:

    P0 + P1*x + P2*x^2 + P3*x^3      0 <= x < 1
    Q0 + Q1*x + Q2*x^2 + Q3*x^3      1 <= x < 2

    which ensures function is continuous in value and derivative (slope).
    */
    if (x < 1.0)
      return(resizeFilterCoefficients[0]+x*(x*
      (resizeFilterCoefficients[1]+x*resizeFilterCoefficients[2])));
    if (x < 2.0)
      return(resizeFilterCoefficients[3]+x*(resizeFilterCoefficients[4]+x*
      (resizeFilterCoefficients[5]+x*resizeFilterCoefficients[6])));
    return(0.0);
  }
  )

  STRINGIFY(
  float Sinc(const float x)
  {
    if (x != 0.0f)
    {
      const float alpha=(float) (MagickPI*x);
      return sinpi(x)/alpha;
    }
    return(1.0f);
  }
  )

  STRINGIFY(
  float Triangle(const float x)
  {
    /*
    1st order (linear) B-Spline, bilinear interpolation, Tent 1D filter, or
    a Bartlett 2D Cone filter.  Also used as a Bartlett Windowing function
    for Sinc().
    */
    return ((x<1.0f)?(1.0f-x):0.0f);
  }
  )


  STRINGIFY(
  float Hann(const float x)
  {
    /*
    Cosine window function:
      0.5+0.5*cos(pi*x).
    */
    const float cosine=cos((MagickPI*x));
    return(0.5f+0.5f*cosine);
  }
  )

  STRINGIFY(
  float Hamming(const float x)
  {
    /*
      Offset cosine window function:
       .54 + .46 cos(pi x).
    */
    const float cosine=cos((MagickPI*x));
    return(0.54f+0.46f*cosine);
  }
  )

  STRINGIFY(
  float Blackman(const float x)
  {
    /*
      Blackman: 2nd order cosine windowing function:
        0.42 + 0.5 cos(pi x) + 0.08 cos(2pi x)

      Refactored by Chantal Racette and Nicolas Robidoux to one trig call and
      five flops.
    */
    const float cosine=cos((MagickPI*x));
    return(0.34f+cosine*(0.5f+cosine*0.16f));
  }
  )

  STRINGIFY(
  static inline float applyResizeFilter(const float x, const ResizeWeightingFunctionType filterType, const __global float* filterCoefficients)
  {
    switch (filterType)
    {
    /* Call Sinc even for SincFast to get better precision on GPU
       and to avoid thread divergence.  Sinc is pretty fast on GPU anyway...*/
    case SincWeightingFunction:
    case SincFastWeightingFunction:
      return Sinc(x);
    case CubicBCWeightingFunction:
      return CubicBC(x,filterCoefficients);
    case BoxWeightingFunction:
      return BoxResizeFilter(x);
    case TriangleWeightingFunction:
      return Triangle(x);
    case HannWeightingFunction:
      return Hann(x);
    case HammingWeightingFunction:
      return Hamming(x);
    case BlackmanWeightingFunction:
      return Blackman(x);

    default:
      return 0.0f;
    }
  }
  )


  STRINGIFY(
  static inline float getResizeFilterWeight(const __global float* resizeFilterCubicCoefficients, const ResizeWeightingFunctionType resizeFilterType
           , const ResizeWeightingFunctionType resizeWindowType
           , const float resizeFilterScale, const float resizeWindowSupport, const float resizeFilterBlur, const float x)
  {
    float scale;
    float xBlur = fabs(x/resizeFilterBlur);
    if (resizeWindowSupport < MagickEpsilon
        || resizeWindowType == BoxWeightingFunction)
    {
      scale = 1.0f;
    }
    else
    {
      scale = resizeFilterScale;
      scale = applyResizeFilter(xBlur*scale, resizeWindowType, resizeFilterCubicCoefficients);
    }
    float weight = scale * applyResizeFilter(xBlur, resizeFilterType, resizeFilterCubicCoefficients);
    return weight;
  }

  )
