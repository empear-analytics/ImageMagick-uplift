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
  inside its accelerateKernels initialiser: the helper functions chosen by quantum depth and HDRI support, OpenCL C
  source in STRINGIFY and OPENCL_* calls, verbatim. This is a fragment,
  without an include guard.
*/
/*
  Helper functions.
*/

OPENCL_IF((MAGICKCORE_QUANTUM_DEPTH == 8))

  STRINGIFY(
    static inline CLQuantum ScaleCharToQuantum(const unsigned char value)
    {
      return((CLQuantum) value);
    }
  )

OPENCL_ELIF((MAGICKCORE_QUANTUM_DEPTH == 16))

  STRINGIFY(
    static inline CLQuantum ScaleCharToQuantum(const unsigned char value)
    {
      return((CLQuantum) (257.0f*value));
    }
  )

OPENCL_ELIF((MAGICKCORE_QUANTUM_DEPTH == 32))

  STRINGIFY(
    static inline CLQuantum ScaleCharToQuantum(const unsigned char value)
    {
      return((CLQuantum) (16843009.0*value));
    }
  )

OPENCL_ENDIF()

OPENCL_IF((MAGICKCORE_HDRI_SUPPORT == 1))

  STRINGIFY(
    static inline CLQuantum ClampToQuantum(const float value)
      {
        return (CLQuantum) clamp(value, 0.0f, QuantumRange);
      }
  )

OPENCL_ELSE()

  STRINGIFY(
    static inline CLQuantum ClampToQuantum(const float value)
      {
        return (CLQuantum) (clamp(value, 0.0f, QuantumRange) + 0.5f);
      }
  )

OPENCL_ENDIF()

  STRINGIFY(
    static inline int ClampToCanvas(const int offset,const int range)
      {
        return clamp(offset, (int)0, range-1);
      }
  )

  STRINGIFY(
    static inline uint ScaleQuantumToMap(CLQuantum value)
      {
        if (value >= (CLQuantum) MaxMap)
          return ((uint)MaxMap);
        else
          return ((uint)value);
      }
  )

  STRINGIFY(
    static inline float MagickSafeReciprocal(const float x)
    {
      float sign = x < (float) 0.0 ? (float) -1.0 : (float) 1.0;
      return((sign*x) >= MagickEpsilon ? (float) 1.0/x : sign*((float) 1.0/MagickEpsilon));
    }
  )
