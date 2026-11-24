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

  MagickCore private graphic matrix methods.
*/
#ifndef MAGICKCORE_MATRIX_PRIVATE_H
#define MAGICKCORE_MATRIX_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern MagickPrivate MagickBooleanType
  GaussJordanElimination(double **,double **,const size_t,const size_t);

typedef struct _LeastSquaresSystem
{
  double
    **matrix,
    **vectors;

  size_t
    rank,
    number_vectors;
} LeastSquaresSystem;

extern MagickPrivate void
  LeastSquaresAddTerms(const LeastSquaresSystem *,const double *,
    const double *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
