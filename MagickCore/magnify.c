/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               M   M   AAA    GGGG  N   N  IIIII  FFFFF  Y   Y               %
%               MM MM  A   A  G      NN  N    I    F       Y Y                %
%               M M M  AAAAA  G  GG  N N N    I    FFF      Y                 %
%               M   M  A   A  G   G  N  NN    I    F        Y                 %
%               M   M  A   A   GGGG  N   N  IIIII  F        Y                 %
%                                                                             %
%                                                                             %
%                       MagickCore Image Magnify Methods                      %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resize-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g n i f y I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagnifyImage() doubles the size of the image with a pixel art scaling
%  algorithm.
%
%  The format of the MagnifyImage method is:
%
%      Image *MagnifyImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline void CopyPixels(const Quantum *source,const ssize_t source_offset,
  Quantum *destination,const ssize_t destination_offset,const size_t channels)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) channels; i++)
    destination[(ssize_t) channels*destination_offset+i]=
      source[source_offset*(ssize_t) channels+i];
}

static inline void MixPixels(const Quantum *source,const ssize_t *source_offset,
  const size_t source_size,Quantum *destination,
  const ssize_t destination_offset,const size_t channels)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) channels; i++)
  {
    ssize_t
      j,
      sum = 0;

    for (j=0; j < (ssize_t) source_size; j++)
      sum+=(ssize_t) source[source_offset[j]*(ssize_t) channels+i];
    destination[(ssize_t) channels*destination_offset+i]=(Quantum) (sum/
      (ssize_t) source_size);
  }
}

static inline void Mix2Pixels(const Quantum *source,
  const ssize_t source_offset1,const ssize_t source_offset2,
  Quantum *destination,const ssize_t destination_offset,const size_t channels)
{
  const ssize_t
    offsets[2] = { source_offset1, source_offset2 };

  MixPixels(source,offsets,2,destination,destination_offset,channels);
}

static inline int PixelsEqual(const Quantum *source1,ssize_t offset1,
  const Quantum *source2,ssize_t offset2,const size_t channels)
{
  ssize_t
    i;

  offset1*=(ssize_t) channels;
  offset2*=(ssize_t) channels;
  for (i=0; i < (ssize_t) channels; i++)
    if (source1[offset1+i] != source2[offset2+i])
      return(0);
  return(1);
}

static inline MagickBooleanType IsEagle2XTopLeft(const Quantum *pixels,
  const size_t channels)
{
  return((PixelsEqual(pixels,0,pixels,1,channels) &&
    PixelsEqual(pixels,1,pixels,3,channels)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle2XTopRight(const Quantum *pixels,
  const size_t channels)
{
  return((PixelsEqual(pixels,1,pixels,2,channels) &&
    PixelsEqual(pixels,2,pixels,5,channels)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle2XBottomLeft(const Quantum *pixels,
  const size_t channels)
{
  return((PixelsEqual(pixels,3,pixels,6,channels) &&
    PixelsEqual(pixels,6,pixels,7,channels)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle2XBottomRight(const Quantum *pixels,
  const size_t channels)
{
  return((PixelsEqual(pixels,5,pixels,8,channels) &&
    PixelsEqual(pixels,8,pixels,7,channels)) ? MagickTrue : MagickFalse);
}

static inline void Eagle2X(const Image *source,const Quantum *pixels,
  Quantum *result,const size_t channels)
{
  ssize_t
    i;

  (void) source;
  for (i=0; i < 4; i++)
    CopyPixels(pixels,4,result,i,channels);
  if (IsEagle2XTopLeft(pixels,channels) != MagickFalse)
    CopyPixels(pixels,0,result,0,channels);
  if (IsEagle2XTopRight(pixels,channels) != MagickFalse)
    CopyPixels(pixels,2,result,1,channels);
  if (IsEagle2XBottomLeft(pixels,channels) != MagickFalse)
    CopyPixels(pixels,6,result,2,channels);
  if (IsEagle2XBottomRight(pixels,channels) != MagickFalse)
    CopyPixels(pixels,8,result,3,channels);
}

static void Hq2XHelper(const unsigned int rule,const Quantum *source,
  Quantum *destination,const ssize_t destination_offset,const size_t channels,
  const ssize_t e,const ssize_t a,const ssize_t b,const ssize_t d,
  const ssize_t f,const ssize_t h)
{
#define caseA(N,A,B,C,D) \
  case N: \
  { \
    const ssize_t \
      offsets[4] = { A, B, C, D }; \
 \
    MixPixels(source,offsets,4,destination,destination_offset,channels);\
    break; \
  }
#define caseB(N,A,B,C,D,E,F,G,H) \
  case N: \
  { \
    const ssize_t \
      offsets[8] = { A, B, C, D, E, F, G, H }; \
 \
    MixPixels(source,offsets,8,destination,destination_offset,channels);\
    break; \
  }

  switch (rule)
  {
    case 0:
    {
      CopyPixels(source,e,destination,destination_offset,channels);
      break;
    }
    caseA(1,e,e,e,a)
    caseA(2,e,e,e,d)
    caseA(3,e,e,e,b)
    caseA(4,e,e,d,b)
    caseA(5,e,e,a,b)
    caseA(6,e,e,a,d)
    caseB(7,e,e,e,e,e,b,b,d)
    caseB(8,e,e,e,e,e,d,d,b)
    caseB(9,e,e,e,e,e,e,d,b)
    caseB(10,e,e,d,d,d,b,b,b)
    case 11:
    {
      const ssize_t
        offsets[16] = { e, e, e, e, e, e, e, e, e, e, e, e, e, e, d, b };

      MixPixels(source,offsets,16,destination,destination_offset,channels);
      break;
    }
    case 12:
    {
      if (PixelsEqual(source,b,source,d,channels))
        {
          const ssize_t
            offsets[4] = { e, e, d, b };

          MixPixels(source,offsets,4,destination,destination_offset,channels);
        }
      else
        CopyPixels(source,e,destination,destination_offset,channels);
      break;
    }
    case 13:
    {
      if (PixelsEqual(source,b,source,d,channels))
        {
          const ssize_t
            offsets[8] = { e, e, d, d, d, b, b, b };

          MixPixels(source,offsets,8,destination,destination_offset,channels);
        }
      else
        CopyPixels(source,e,destination,destination_offset,channels);
      break;
    }
    case 14:
    {
      if (PixelsEqual(source,b,source,d,channels))
        {
          const ssize_t
            offsets[16] = { e, e, e, e, e, e, e, e, e, e, e, e, e, e, d, b };

          MixPixels(source,offsets,16,destination,destination_offset,channels);
        }
      else
        CopyPixels(source,e,destination,destination_offset,channels);
      break;
    }
    case 15:
    {
      if (PixelsEqual(source,b,source,d,channels))
        {
          const ssize_t
            offsets[4] = { e, e, d, b };

          MixPixels(source,offsets,4,destination,destination_offset,channels);
        }
      else
        {
          const ssize_t
            offsets[4] = { e, e, e, a };

          MixPixels(source,offsets,4,destination,destination_offset,channels);
        }
      break;
    }
    case 16:
    {
      if (PixelsEqual(source,b,source,d,channels))
        {
          const ssize_t
            offsets[8] = { e, e, e, e, e, e, d, b };

          MixPixels(source,offsets,8,destination,destination_offset,channels);
        }
      else
        {
          const ssize_t
            offsets[4] = { e, e, e, a };

          MixPixels(source,offsets,4,destination,destination_offset,channels);
        }
      break;
    }
    case 17:
    {
      if (PixelsEqual(source,b,source,d,channels))
        {
          const ssize_t
            offsets[8] = { e, e, d, d, d, b, b, b };

          MixPixels(source,offsets,8,destination,destination_offset,channels);
        }
      else
        {
          const ssize_t
            offsets[4] = { e, e, e, a };

          MixPixels(source,offsets,4,destination,destination_offset,channels);
        }
      break;
    }
    case 18:
    {
      if (PixelsEqual(source,b,source,f,channels))
        {
          const ssize_t
            offsets[8] = { e, e, e, e, e, b, b, d };

          MixPixels(source,offsets,8,destination,destination_offset,channels);
        }
      else
        {
          const ssize_t
            offsets[4] = { e, e, e, d };

          MixPixels(source,offsets,4,destination,destination_offset,channels);
        }
      break;
    }
    default:
    {
      if (PixelsEqual(source,d,source,h,channels))
        {
          const ssize_t
            offsets[8] = { e, e, e, e, e, d, d, b };

          MixPixels(source,offsets,8,destination,destination_offset,channels);
        }
      else
        {
          const ssize_t
            offsets[4] = { e, e, e, b };

          MixPixels(source,offsets,4,destination,destination_offset,channels);
        }
      break;
    }
  }
  #undef caseA
  #undef caseB
}

static inline unsigned int Hq2XPatternToNumber(const int *pattern)
{
  ssize_t
    i;

  unsigned int
    result,
    order;

  result=0;
  order=1;
  for (i=7; i >= 0; i--)
  {
    result+=order*(unsigned int) pattern[i];
    order*=2;
  }
  return(result);
}

static inline void Hq2X(const Image *source,const Quantum *pixels,
  Quantum *result,const size_t channels)
{
  static const unsigned int
    Hq2XTable[] =
    {
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 15, 12, 5,  3, 17, 13,
      4, 4, 6, 18, 4, 4, 6, 18, 5,  3, 12, 12, 5,  3,  1, 12,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 17, 13, 5,  3, 16, 14,
      4, 4, 6, 18, 4, 4, 6, 18, 5,  3, 16, 12, 5,  3,  1, 14,
      4, 4, 6,  2, 4, 4, 6,  2, 5, 19, 12, 12, 5, 19, 16, 12,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3, 16, 12,
      4, 4, 6,  2, 4, 4, 6,  2, 5, 19,  1, 12, 5, 19,  1, 14,
      4, 4, 6,  2, 4, 4, 6, 18, 5,  3, 16, 12, 5, 19,  1, 14,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 15, 12, 5,  3, 17, 13,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3, 16, 12,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 17, 13, 5,  3, 16, 14,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 13, 5,  3,  1, 14,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3, 16, 13,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3,  1, 12,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3,  1, 14,
      4, 4, 6,  2, 4, 4, 6,  2, 5,  3,  1, 12, 5,  3,  1, 14
    };

  const int
    pattern1[] =
    {
      !PixelsEqual(pixels,4,pixels,8,channels),
      !PixelsEqual(pixels,4,pixels,7,channels),
      !PixelsEqual(pixels,4,pixels,6,channels),
      !PixelsEqual(pixels,4,pixels,5,channels),
      !PixelsEqual(pixels,4,pixels,3,channels),
      !PixelsEqual(pixels,4,pixels,2,channels),
      !PixelsEqual(pixels,4,pixels,1,channels),
      !PixelsEqual(pixels,4,pixels,0,channels)
    };

#define Rotated(p) p[2], p[4], p[7], p[1], p[6], p[0], p[3], p[5]
  const int pattern2[] = { Rotated(pattern1) };
  const int pattern3[] = { Rotated(pattern2) };
  const int pattern4[] = { Rotated(pattern3) };
#undef Rotated
  (void) source;
  Hq2XHelper(Hq2XTable[Hq2XPatternToNumber(pattern1)],pixels,result,0,
    channels,4,0,1,3,5,7);
  Hq2XHelper(Hq2XTable[Hq2XPatternToNumber(pattern2)],pixels,result,1,
    channels,4,2,5,1,7,3);
  Hq2XHelper(Hq2XTable[Hq2XPatternToNumber(pattern3)],pixels,result,3,
    channels,4,8,7,5,3,1);
  Hq2XHelper(Hq2XTable[Hq2XPatternToNumber(pattern4)],pixels,result,2,
    channels,4,6,3,7,1,5);
}

static inline MagickBooleanType IsFish2XUniform(const int ae,const int bd,
  const int ab)
{
  return((ae && bd && ab) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsFish2XTopOdd(const int ad,const int de,
  const int ab)
{
  return((ad && de && !ab) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsFish2XTopLeftOdd(const int be,const int de,
  const int ab)
{
  return((be && de && !ab) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsFish2XCentreOdd(const int ad,const int ab,
  const int be)
{
  return((ad && ab && !be) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsFish2XLeftOdd(const int ab,const int be,
  const int ad)
{
  return((ab && be && !ad) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsFish2XMainDiagonal(const int ae,
  const int bd,const MagickFloatType *intensities)
{
  return((ae && (!bd || intensities[1] > intensities[0])) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsFish2XAntiDiagonal(const int bd,
  const int ae,const MagickFloatType *intensities)
{
  return((bd && (!ae || intensities[0] > intensities[1])) ? MagickTrue :
    MagickFalse);
}

static void Fish2X(const Image *source,const Quantum *pixels,Quantum *result,
  const size_t channels)
{
#define Corner(A,B,C,D) \
  { \
    if (intensities[B] > intensities[A]) \
      { \
        const ssize_t    \
          offsets[3] = { B, C, D }; \
 \
        MixPixels(pixels,offsets,3,result,3,channels); \
      } \
    else \
      { \
        const ssize_t    \
          offsets[3] = { A, B, C }; \
 \
        MixPixels(pixels,offsets,3,result,3,channels); \
      } \
  }

#define Line(A,B,C,D) \
  { \
    if (intensities[C] > intensities[A]) \
      Mix2Pixels(pixels,C,D,result,3,channels); \
    else \
      Mix2Pixels(pixels,A,B,result,3,channels); \
  }

  const ssize_t
    pixels_offsets[4] = { 0, 1, 3, 4 };

  int
    ab,
    ad,
    ae,
    bd,
    be,
    de;

  MagickFloatType
    intensities[9];

  ssize_t
    i;

  for (i=0; i < 9; i++)
    intensities[i]=(MagickFloatType) GetPixelIntensity(source,pixels+
      i*(ssize_t) channels);
  CopyPixels(pixels,0,result,0,channels);
  CopyPixels(pixels,(ssize_t) (intensities[0] > intensities[1] ? 0 : 1),result,
    1,channels);
  CopyPixels(pixels,(ssize_t) (intensities[0] > intensities[3] ? 0 : 3),result,
    2,channels);
  ae=PixelsEqual(pixels,0,pixels,4,channels);
  bd=PixelsEqual(pixels,1,pixels,3,channels);
  ab=PixelsEqual(pixels,0,pixels,1,channels);
  de=PixelsEqual(pixels,3,pixels,4,channels);
  ad=PixelsEqual(pixels,0,pixels,3,channels);
  be=PixelsEqual(pixels,1,pixels,4,channels);
  if (IsFish2XUniform(ae,bd,ab) != MagickFalse)
    {
      CopyPixels(pixels,0,result,3,channels);
      return;
    }
  if (IsFish2XTopOdd(ad,de,ab) != MagickFalse)
    {
      Corner(1,0,4,3)
      return;
    }
  if (IsFish2XTopLeftOdd(be,de,ab) != MagickFalse)
    {
      Corner(0,1,3,4)
      return;
    }
  if (IsFish2XCentreOdd(ad,ab,be) != MagickFalse)
    {
      Corner(4,3,1,0)
      return;
    }
  if (IsFish2XLeftOdd(ab,be,ad) != MagickFalse)
    {
      Corner(3,0,4,1)
      return;
    }
  if (IsFish2XMainDiagonal(ae,bd,intensities) != MagickFalse)
    {
      Mix2Pixels(pixels,0,4,result,3,channels);
      return;
    }
  if (IsFish2XAntiDiagonal(bd,ae,intensities) != MagickFalse)
    {
      Mix2Pixels(pixels,1,3,result,3,channels);
      return;
    }
  if (ab)
    {
      Line(0,1,3,4)
      return;
    }
  if (de)
    {
      Line(3,4,0,1)
      return;
    }
  if (ad)
    {
      Line(0,3,1,4)
      return;
    }
  if (be)
    {
      Line(1,4,0,3)
      return;
    }
  MixPixels(pixels,pixels_offsets,4,result,3,channels);
#undef Corner
#undef Line
}

static void Xbr2X(const Image *magick_unused(source),const Quantum *pixels,
  Quantum *result,const size_t channels)
{
#define WeightVar(M,N) const int w_##M##_##N = \
  PixelsEqual(pixels,M,pixels,N,channels) ? 0 : 1;

  WeightVar(12,11)
  WeightVar(12,7)
  WeightVar(12,13)
  WeightVar(12,17)
  WeightVar(12,16)
  WeightVar(12,8)
  WeightVar(6,10)
  WeightVar(6,2)
  WeightVar(11,7)
  WeightVar(11,17)
  WeightVar(11,5)
  WeightVar(7,13)
  WeightVar(7,1)
  WeightVar(12,6)
  WeightVar(12,18)
  WeightVar(8,14)
  WeightVar(8,2)
  WeightVar(13,17)
  WeightVar(13,9)
  WeightVar(7,3)
  WeightVar(16,10)
  WeightVar(16,22)
  WeightVar(17,21)
  WeightVar(11,15)
  WeightVar(18,14)
  WeightVar(18,22)
  WeightVar(17,23)
  WeightVar(17,19)
#undef WeightVar

  magick_unreferenced(source);

  if (
    w_12_16 + w_12_8 + w_6_10 + w_6_2 + (4 * w_11_7) <
    w_11_17 + w_11_5 + w_7_13 + w_7_1 + (4 * w_12_6)
  )
    Mix2Pixels(pixels,(ssize_t) (w_12_11 <= w_12_7 ? 11 : 7),12,result,0,
      channels);
  else
    CopyPixels(pixels,12,result,0,channels);
  if (
    w_12_18 + w_12_6 + w_8_14 + w_8_2 + (4 * w_7_13) <
    w_13_17 + w_13_9 + w_11_7 + w_7_3 + (4 * w_12_8)
  )
    Mix2Pixels(pixels,(ssize_t) (w_12_7 <= w_12_13 ? 7 : 13),12,result,1,
      channels);
  else
    CopyPixels(pixels,12,result,1,channels);
  if (
    w_12_6 + w_12_18 + w_16_10 + w_16_22 + (4 * w_11_17) <
    w_11_7 + w_11_15 + w_13_17 + w_17_21 + (4 * w_12_16)
  )
    Mix2Pixels(pixels,(ssize_t) (w_12_11 <= w_12_17 ? 11 : 17),12,result,2,
      channels);
  else
    CopyPixels(pixels,12,result,2,channels);
  if (
    w_12_8 + w_12_16 + w_18_14 + w_18_22 + (4 * w_13_17) <
    w_11_17 + w_17_23 + w_17_19 + w_7_13 + (4 * w_12_18)
  )
    Mix2Pixels(pixels,(ssize_t) (w_12_13 <= w_12_17 ? 13 : 17),12,result,3,
      channels);
  else
    CopyPixels(pixels,12,result,3,channels);
}

static void Scale2X(const Image *magick_unused(source),const Quantum *pixels,
  Quantum *result,const size_t channels)
{
  magick_unreferenced(source);

  if (PixelsEqual(pixels,1,pixels,7,channels) ||
      PixelsEqual(pixels,3,pixels,5,channels))
    {
      ssize_t
        i;

      for (i=0; i < 4; i++)
        CopyPixels(pixels,4,result,i,channels);
      return;
    }
    if (PixelsEqual(pixels,1,pixels,3,channels))
      CopyPixels(pixels,3,result,0,channels);
    else
      CopyPixels(pixels,4,result,0,channels);
    if (PixelsEqual(pixels,1,pixels,5,channels))
      CopyPixels(pixels,5,result,1,channels);
    else
      CopyPixels(pixels,4,result,1,channels);
    if (PixelsEqual(pixels,3,pixels,7,channels))
      CopyPixels(pixels,3,result,2,channels);
    else
      CopyPixels(pixels,4,result,2,channels);
    if (PixelsEqual(pixels,5,pixels,7,channels))
      CopyPixels(pixels,5,result,3,channels);
    else
      CopyPixels(pixels,4,result,3,channels);
}

static inline MagickBooleanType IsEpbx2XBlendable(const Quantum *pixels,
  const size_t channels)
{
  return((
    !PixelsEqual(pixels,3,pixels,5,channels) &&
    !PixelsEqual(pixels,1,pixels,7,channels) &&
    (
      PixelsEqual(pixels,4,pixels,3,channels) ||
      PixelsEqual(pixels,4,pixels,7,channels) ||
      PixelsEqual(pixels,4,pixels,5,channels) ||
      PixelsEqual(pixels,4,pixels,1,channels) ||
      (
        (
          !PixelsEqual(pixels,0,pixels,8,channels) ||
           PixelsEqual(pixels,4,pixels,6,channels) ||
           PixelsEqual(pixels,3,pixels,2,channels)
        ) &&
        (
          !PixelsEqual(pixels,6,pixels,2,channels) ||
           PixelsEqual(pixels,4,pixels,0,channels) ||
           PixelsEqual(pixels,4,pixels,8,channels)
        )
      )
    )
  ) ? MagickTrue : MagickFalse);
}

static void Epbx2X(const Image *magick_unused(source),const Quantum *pixels,
  Quantum *result,const size_t channels)
{
#define HelperCond(a,b,c,d,e,f,g) ( \
  PixelsEqual(pixels,a,pixels,b,channels) && ( \
    PixelsEqual(pixels,c,pixels,d,channels) || \
    PixelsEqual(pixels,c,pixels,e,channels) || \
    PixelsEqual(pixels,a,pixels,f,channels) || \
    PixelsEqual(pixels,b,pixels,g,channels) \
    ) \
  )

  ssize_t
    i;

  magick_unreferenced(source);

  for (i=0; i < 4; i++)
    CopyPixels(pixels,4,result,i,channels);
  if (IsEpbx2XBlendable(pixels,channels) != MagickFalse)
    {
      if (HelperCond(1,3,4,0,8,2,6))
        Mix2Pixels(pixels,1,3,result,0,channels);
      if (HelperCond(5,1,4,2,6,8,0))
        Mix2Pixels(pixels,5,1,result,1,channels);
      if (HelperCond(3,7,4,6,2,0,8))
        Mix2Pixels(pixels,3,7,result,2,channels);
      if (HelperCond(7,5,4,8,0,6,2))
        Mix2Pixels(pixels,7,5,result,3,channels);
    }

#undef HelperCond
}

static inline MagickBooleanType IsEagle3XTopLeftCorner(const Quantum *pixels,
  const size_t channels)
{
  return((PixelsEqual(pixels,0,pixels,1,channels) &&
    PixelsEqual(pixels,0,pixels,3,channels)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle3XTopRightCorner(const Quantum *pixels,
  const size_t channels)
{
  return((PixelsEqual(pixels,1,pixels,2,channels) &&
    PixelsEqual(pixels,2,pixels,5,channels)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle3XBottomLeftCorner(const Quantum *pixels,
  const size_t channels)
{
  return((PixelsEqual(pixels,3,pixels,6,channels) &&
    PixelsEqual(pixels,6,pixels,7,channels)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle3XBottomRightCorner(
  const Quantum *pixels,const size_t channels)
{
  return((PixelsEqual(pixels,5,pixels,7,channels) &&
    PixelsEqual(pixels,7,pixels,8,channels)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle3XTopEdge(const ssize_t corner_tl,
  const ssize_t corner_tr)
{
  return((corner_tl && corner_tr) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle3XLeftEdge(const ssize_t corner_tl,
  const ssize_t corner_bl)
{
  return((corner_tl && corner_bl) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle3XRightEdge(const ssize_t corner_tr,
  const ssize_t corner_br)
{
  return((corner_tr && corner_br) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsEagle3XBottomEdge(const ssize_t corner_bl,
  const ssize_t corner_br)
{
  return((corner_bl && corner_br) ? MagickTrue : MagickFalse);
}

static inline void Eagle3X(const Image *magick_unused(source),
  const Quantum *pixels,Quantum *result,const size_t channels)
{
  ssize_t
    corner_tl,
    corner_tr,
    corner_bl,
    corner_br;

  magick_unreferenced(source);

  corner_tl=IsEagle3XTopLeftCorner(pixels,channels) != MagickFalse;
  corner_tr=IsEagle3XTopRightCorner(pixels,channels) != MagickFalse;
  corner_bl=IsEagle3XBottomLeftCorner(pixels,channels) != MagickFalse;
  corner_br=IsEagle3XBottomRightCorner(pixels,channels) != MagickFalse;
  CopyPixels(pixels,(ssize_t) (corner_tl ? 0 : 4),result,0,channels);
  if (IsEagle3XTopEdge(corner_tl,corner_tr) != MagickFalse)
    Mix2Pixels(pixels,0,2,result,1,channels);
  else
    CopyPixels(pixels,4,result,1,channels);
  CopyPixels(pixels,(ssize_t) (corner_tr ? 1 : 4),result,2,channels);
  if (IsEagle3XLeftEdge(corner_tl,corner_bl) != MagickFalse)
    Mix2Pixels(pixels,0,6,result,3,channels);
  else
    CopyPixels(pixels,4,result,3,channels);
  CopyPixels(pixels,4,result,4,channels);
  if (IsEagle3XRightEdge(corner_tr,corner_br) != MagickFalse)
    Mix2Pixels(pixels,2,8,result,5,channels);
  else
    CopyPixels(pixels,4,result,5,channels);
  CopyPixels(pixels,(ssize_t) (corner_bl ? 3 : 4),result,6,channels);
  if (IsEagle3XBottomEdge(corner_bl,corner_br) != MagickFalse)
    Mix2Pixels(pixels,6,8,result,7,channels);
  else
    CopyPixels(pixels,4,result,7,channels);
  CopyPixels(pixels,(ssize_t) (corner_br ? 5 : 4),result,8,channels);
}

static inline void Eagle3XB(const Image *magick_unused(source),
  const Quantum *pixels,Quantum *result,const size_t channels)
{
  ssize_t
    corner_tl,
    corner_tr,
    corner_bl,
    corner_br;

  magick_unreferenced(source);

  corner_tl=IsEagle3XTopLeftCorner(pixels,channels) != MagickFalse;
  corner_tr=IsEagle3XTopRightCorner(pixels,channels) != MagickFalse;
  corner_bl=IsEagle3XBottomLeftCorner(pixels,channels) != MagickFalse;
  corner_br=IsEagle3XBottomRightCorner(pixels,channels) != MagickFalse;
  CopyPixels(pixels,(ssize_t) (corner_tl ? 0 : 4),result,0,channels);
  CopyPixels(pixels,4,result,1,channels);
  CopyPixels(pixels,(ssize_t) (corner_tr ? 1 : 4),result,2,channels);
  CopyPixels(pixels,4,result,3,channels);
  CopyPixels(pixels,4,result,4,channels);
  CopyPixels(pixels,4,result,5,channels);
  CopyPixels(pixels,(ssize_t) (corner_bl ? 3 : 4),result,6,channels);
  CopyPixels(pixels,4,result,7,channels);
  CopyPixels(pixels,(ssize_t) (corner_br ? 5 : 4),result,8,channels);
}

static inline MagickBooleanType IsScale3XTopEdge(const Quantum *pixels,
  const size_t channels)
{
  return((
    (
      PixelsEqual(pixels,3,pixels,1,channels) &&
      !PixelsEqual(pixels,4,pixels,2,channels)
    ) ||
    (
      PixelsEqual(pixels,5,pixels,1,channels) &&
      !PixelsEqual(pixels,4,pixels,0,channels)
    )
  ) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsScale3XLeftEdge(const Quantum *pixels,
  const size_t channels)
{
  return((
    (
      PixelsEqual(pixels,3,pixels,1,channels) &&
      !PixelsEqual(pixels,4,pixels,6,channels)
    ) ||
    (
      PixelsEqual(pixels,3,pixels,7,channels) &&
      !PixelsEqual(pixels,4,pixels,0,channels)
    )
  ) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsScale3XRightEdge(const Quantum *pixels,
  const size_t channels)
{
  return((
    (
      PixelsEqual(pixels,5,pixels,1,channels) &&
      !PixelsEqual(pixels,4,pixels,8,channels)
    ) ||
    (
      PixelsEqual(pixels,5,pixels,7,channels) &&
      !PixelsEqual(pixels,4,pixels,2,channels)
    )
  ) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsScale3XBottomEdge(const Quantum *pixels,
  const size_t channels)
{
  return((
    (
      PixelsEqual(pixels,3,pixels,7,channels) &&
      !PixelsEqual(pixels,4,pixels,8,channels)
    ) ||
    (
      PixelsEqual(pixels,5,pixels,7,channels) &&
      !PixelsEqual(pixels,4,pixels,6,channels)
    )
  ) ? MagickTrue : MagickFalse);
}

static inline void Scale3X(const Image *magick_unused(source),
  const Quantum *pixels,Quantum *result,const size_t channels)
{
  magick_unreferenced(source);

  if (PixelsEqual(pixels,1,pixels,7,channels) ||
      PixelsEqual(pixels,3,pixels,5,channels))
    {
      ssize_t
        i;

      for (i=0; i < 9; i++)
        CopyPixels(pixels,4,result,i,channels);
      return;
    }
  if (PixelsEqual(pixels,3,pixels,1,channels))
    CopyPixels(pixels,3,result,0,channels);
  else
    CopyPixels(pixels,4,result,0,channels);

  if (IsScale3XTopEdge(pixels,channels) != MagickFalse)
    CopyPixels(pixels,1,result,1,channels);
  else
    CopyPixels(pixels,4,result,1,channels);
  if (PixelsEqual(pixels,5,pixels,1,channels))
    CopyPixels(pixels,5,result,2,channels);
  else
    CopyPixels(pixels,4,result,2,channels);
  if (IsScale3XLeftEdge(pixels,channels) != MagickFalse)
    CopyPixels(pixels,3,result,3,channels);
  else
    CopyPixels(pixels,4,result,3,channels);
  CopyPixels(pixels,4,result,4,channels);
  if (IsScale3XRightEdge(pixels,channels) != MagickFalse)
    CopyPixels(pixels,5,result,5,channels);
  else
    CopyPixels(pixels,4,result,5,channels);
  if (PixelsEqual(pixels,3,pixels,7,channels))
    CopyPixels(pixels,3,result,6,channels);
  else
    CopyPixels(pixels,4,result,6,channels);
  if (IsScale3XBottomEdge(pixels,channels) != MagickFalse)
    CopyPixels(pixels,7,result,7,channels);
  else
    CopyPixels(pixels,4,result,7,channels);
  if (PixelsEqual(pixels,5,pixels,7,channels))
    CopyPixels(pixels,5,result,8,channels);
  else
    CopyPixels(pixels,4,result,8,channels);
}

static void CopyMagnifiedPixels(const Image *magnify_image,const Quantum *r,
  const size_t channels,const size_t magnification,Quantum *q)
{
  ssize_t
    i,
    j;

  for (j=0; j < (ssize_t) magnification; j++)
    for (i=0; i < (ssize_t) (channels*magnification); i++)
      q[j*(ssize_t) channels*(ssize_t) magnify_image->columns+i]=
        r[j*magnification*(ssize_t) channels+i];
}

static void SelectMagnifyMethod(const char *option,
  void (**scaling_method)(const Image *,const Quantum *,Quantum *,size_t),
  size_t *magnification,size_t *width)
{
  *scaling_method=Scale2X;
  *magnification=2;
  *width=3;
  switch (*option)
  {
    case 'e':
    {
      if (LocaleCompare(option,"eagle2x") == 0)
        {
          *scaling_method=Eagle2X;
          *magnification=2;
          *width=3;
          break;
        }
      if (LocaleCompare(option,"eagle3x") == 0)
        {
          *scaling_method=Eagle3X;
          *magnification=3;
          *width=3;
          break;
        }
      if (LocaleCompare(option,"eagle3xb") == 0)
        {
          *scaling_method=Eagle3XB;
          *magnification=3;
          *width=3;
          break;
        }
      if (LocaleCompare(option,"epbx2x") == 0)
        {
          *scaling_method=Epbx2X;
          *magnification=2;
          *width=3;
          break;
        }
      break;
    }
    case 'f':
    {
      if (LocaleCompare(option,"fish2x") == 0)
        {
          *scaling_method=Fish2X;
          *magnification=2;
          *width=3;
          break;
        }
      break;
    }
    case 'h':
    {
      if (LocaleCompare(option,"hq2x") == 0)
        {
          *scaling_method=Hq2X;
          *magnification=2;
          *width=3;
          break;
        }
      break;
    }
    case 's':
    {
      if (LocaleCompare(option,"scale2x") == 0)
        {
          *scaling_method=Scale2X;
          *magnification=2;
          *width=3;
          break;
        }
      if (LocaleCompare(option,"scale3x") == 0)
        {
          *scaling_method=Scale3X;
          *magnification=3;
          *width=3;
          break;
        }
      break;
    }
    case 'x':
    {
      if (LocaleCompare(option,"xbr2x") == 0)
        {
          *scaling_method=Xbr2X;
          *magnification=2;
          *width=5;
        }
      break;
    }
    default:
      break;
  }
}

static Image *CloneMagnifySourceImage(const Image *image,
  ExceptionInfo *exception)
{
  Image
    *source_image;

  OffsetInfo
    offset;

  RectangleInfo
    rectangle;

  source_image=CloneImage(image,image->columns,image->rows,MagickTrue,
    exception);
  if (source_image == (Image *) NULL)
    return((Image *) NULL);
  offset.x=0;
  offset.y=0;
  rectangle.x=0;
  rectangle.y=0;
  rectangle.width=image->columns;
  rectangle.height=image->rows;
  (void) CopyImagePixels(source_image,image,&rectangle,&offset,exception);
  if (IssRGBCompatibleColorspace(source_image->colorspace) == MagickFalse)
    (void) TransformImageColorspace(source_image,sRGBColorspace,exception);
  return(source_image);
}

MagickExport Image *MagnifyImage(const Image *image,ExceptionInfo *exception)
{
#define MagnifyImageTag  "Magnify/Image"
#define MaxMagnification  9

  CacheView
    *image_view,
    *magnify_view;

  const char
    *option;

  Image
    *source_image,
    *magnify_image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  size_t
    magnification,
    width;

  ssize_t
    y;

  void
    (*scaling_method)(const Image *,const Quantum *,Quantum *,size_t);

  /*
    Initialize magnified image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  option=GetImageOption(image->image_info,"magnify:method");
  if (option == (char *) NULL)
    option="scale2x";
  SelectMagnifyMethod(option,&scaling_method,&magnification,&width);
  assert((magnification*magnification) <= MaxMagnification);
  /*
    Make a working copy of the source image and convert it to RGB colorspace.
  */
  source_image=CloneMagnifySourceImage(image,exception);
  if (source_image == (Image *) NULL)
    return((Image *) NULL);
  magnify_image=CloneImage(source_image,magnification*source_image->columns,
    magnification*source_image->rows,MagickTrue,exception);
  if (magnify_image == (Image *) NULL)
    {
      source_image=DestroyImage(source_image);
      return((Image *) NULL);
    }
  /*
    Magnify the image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireVirtualCacheView(source_image,exception);
  magnify_view=AcquireAuthenticCacheView(magnify_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(source_image,magnify_image,source_image->rows,1)
#endif
  for (y=0; y < (ssize_t) source_image->rows; y++)
  {
    Quantum
      r[MaxMagnification*MaxPixelChannels];  /* result pixels */

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=QueueCacheViewAuthenticPixels(magnify_view,0,magnification*y,
      magnify_image->columns,magnification,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    /*
      Magnify this row of pixels.
    */
    for (x=0; x < (ssize_t) source_image->columns; x++)
    {
      const Quantum
        *magick_restrict p;

      size_t
        channels;

      p=GetCacheViewVirtualPixels(image_view,x-width/2,y-width/2,width,width,
        exception);
      if (p == (Quantum *) NULL)
        {
          status=MagickFalse;
          continue;
        }
      channels=GetPixelChannels(source_image);
      scaling_method(source_image,p,r,channels);
      /*
        Copy the result pixels into the final image.
      */
      CopyMagnifiedPixels(magnify_image,r,channels,magnification,q);
      q+=(ptrdiff_t) magnification*GetPixelChannels(magnify_image);
    }
    if (SyncCacheViewAuthenticPixels(magnify_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,MagnifyImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  magnify_view=DestroyCacheView(magnify_view);
  image_view=DestroyCacheView(image_view);
  source_image=DestroyImage(source_image);
  if (status == MagickFalse)
    magnify_image=DestroyImage(magnify_image);
  return(magnify_image);
}
