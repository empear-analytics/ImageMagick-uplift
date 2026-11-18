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

  MagickCore private methods of morphology-builtin.c: IsBaseKernelNeeded,
  whether AcquireKernelBuiltIn allocates a base kernel; included by that file
  only.
*/
#ifndef MAGICKCORE_MORPHOLOGY_BUILTIN_BASE_PRIVATE_H
#define MAGICKCORE_MORPHOLOGY_BUILTIN_BASE_PRIVATE_H

static MagickBooleanType IsBaseKernelNeeded(const KernelInfoType type)
{
  if (IsKernelBuiltWithoutBase(type) != MagickFalse)
    return(MagickFalse); /* A pre-generated kernel is not needed */
  switch(type) {
#if 0
    /* set to 1 to do a compile-time check that we haven't missed anything */
    case UnityKernel:
    case GaussianKernel:
    case DoGKernel:
    case LoGKernel:
    case BlurKernel:
    case CometKernel:
    case BinomialKernel:
    case DiamondKernel:
    case SquareKernel:
    case RectangleKernel:
    case OctagonKernel:
    case DiskKernel:
    case PlusKernel:
    case CrossKernel:
    case RingKernel:
    case PeaksKernel:
    case ChebyshevKernel:
    case ManhattanKernel:
    case OctagonalKernel:
    case EuclideanKernel:
#else
    default:
#endif
      return(MagickTrue);
  }
  return(MagickFalse);
}

#endif
