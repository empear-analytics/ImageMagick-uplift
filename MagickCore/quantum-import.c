/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                QQQ   U   U   AAA   N   N  TTTTT  U   U  M   M               %
%               Q   Q  U   U  A   A  NN  N    T    U   U  MM MM               %
%               Q   Q  U   U  AAAAA  N N N    T    U   U  M M M               %
%               Q  QQ  U   U  A   A  N  NN    T    U   U  M   M               %
%                QQQQ   UUU   A   A  N   N    T     UUU   M   M               %
%                                                                             %
%                   IIIII  M   M  PPPP    OOO   RRRR   TTTTT                  %
%                     I    MM MM  P   P  O   O  R   R    T                    %
%                     I    M M M  PPPP   O   O  RRRR     T                    %
%                     I    M   M  P      O   O  R R      T                    %
%                   IIIII  M   M  P       OOO   R  R     T                    %
%                                                                             %
%                 MagickCore Methods to Import Quantum Pixels                 %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               October 1998                                  %
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
#include "MagickCore/property.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/utility.h"

/*
  Typedef declarations.
*/
typedef struct _QuantumPixelSpan
{
  MagickSizeType
    number_pixels;

  const unsigned char
    *magick_restrict p;

  Quantum
    *magick_restrict q;
} QuantumPixelSpan;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m p o r t Q u a n t u m P i x e l s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImportQuantumPixels() transfers one or more pixel components from a user
%  supplied buffer into the image pixel cache of an image.  The pixels are
%  expected in network byte order.  It returns the number of imported pixels.
%
%  The format of the ImportQuantumPixels method is:
%
%      size_t ImportQuantumPixels(const Image *image,CacheView *image_view,
%        QuantumInfo *quantum_info,const QuantumType quantum_type,
%        const unsigned char *magick_restrict pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image cache view.
%
%    o quantum_info: the quantum info.
%
%    o quantum_type: Declare which pixel components to transfer (red, green,
%      blue, opacity, RGB, or RGBA).
%
%    o pixels:  The pixel components are transferred from this buffer.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline Quantum PushColormapIndex(const Image *image,const size_t index,
  MagickBooleanType *range_exception)
{
  if (index < image->colors)
    return((Quantum) index);
  *range_exception=MagickTrue;
  return((Quantum) 0);
}

static inline const unsigned char *PushDoublePixel(QuantumInfo *quantum_info,
  const unsigned char *magick_restrict pixels,double *pixel)
{
  double
    *p;

  unsigned char
    quantum[8];

  if (quantum_info->endian == LSBEndian)
    {
      quantum[0]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[2]=(*pixels++);
      quantum[3]=(*pixels++);
      quantum[4]=(*pixels++);
      quantum[5]=(*pixels++);
      quantum[6]=(*pixels++);
      quantum[7]=(*pixels++);
    }
  else
    {
      quantum[7]=(*pixels++);
      quantum[6]=(*pixels++);
      quantum[5]=(*pixels++);
      quantum[4]=(*pixels++);
      quantum[3]=(*pixels++);
      quantum[2]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[0]=(*pixels++);
    }
  p=(double *) quantum;
  *pixel=(*p);
  *pixel-=quantum_info->minimum;
  *pixel*=quantum_info->scale;
  return(pixels);
}

static inline float ScaleFloatPixel(const QuantumInfo *quantum_info,
  const unsigned char *quantum)
{
  double
    pixel;

  pixel=(double) (*((float *) quantum));
  pixel-=quantum_info->minimum;
  pixel*=quantum_info->scale;
  if (pixel < (double) -FLT_MAX)
    return(-FLT_MAX);
  if (pixel > (double) FLT_MAX)
    return(FLT_MAX);
  return((float) pixel);
}

static inline const unsigned char *PushQuantumFloatPixel(
  const QuantumInfo *quantum_info,const unsigned char *magick_restrict pixels,
  float *pixel)
{
  unsigned char
    quantum[4];

  if (quantum_info->endian == LSBEndian)
    {
      quantum[0]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[2]=(*pixels++);
      quantum[3]=(*pixels++);
    }
  else
    {
      quantum[3]=(*pixels++);
      quantum[2]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[0]=(*pixels++);
    }
  *pixel=ScaleFloatPixel(quantum_info,quantum);
  return(pixels);
}

static inline const unsigned char *PushQuantumFloat24Pixel(
  const QuantumInfo *quantum_info,const unsigned char *magick_restrict pixels,
  float *pixel)
{
  unsigned char
    quantum[4];

  if (quantum_info->endian == LSBEndian)
    {
      quantum[0]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[2]=(*pixels++);
    }
  else
    {
      quantum[2]=(*pixels++);
      quantum[1]=(*pixels++);
      quantum[0]=(*pixels++);
    }
  if ((quantum[0] | quantum[1] | quantum[2]) == 0U)
    quantum[3]=0;
  else
    {
      unsigned char
        exponent,
        sign_bit;

      sign_bit=(quantum[2] & 0x80);
      exponent=(quantum[2] & 0x7F);
      if (exponent != 0)
        exponent=exponent-63+127;
      quantum[3]=sign_bit | (exponent >> 1);
      quantum[2]=((exponent & 1) << 7) | ((quantum[1] & 0xFE) >> 1);
      quantum[1]=((quantum[1] & 0x01) << 7) | ((quantum[0] & 0xFE) >> 1);
      quantum[0]=(quantum[0] & 0x01) << 7;
    }
  *pixel=ScaleFloatPixel(quantum_info,quantum);
  return(pixels);
}

static inline const unsigned char *PushQuantumPixel(QuantumInfo *quantum_info,
  const unsigned char *magick_restrict pixels,unsigned int *quantum)
{
  ssize_t
    i;

  size_t
    quantum_bits;

  *quantum=(QuantumAny) 0;
  for (i=(ssize_t) quantum_info->depth; i > 0L; )
  {
    if (quantum_info->state.bits == 0UL)
      {
        quantum_info->state.pixel=(*pixels++);
        quantum_info->state.bits=8UL;
      }
    quantum_bits=(size_t) i;
    if (quantum_bits > quantum_info->state.bits)
      quantum_bits=quantum_info->state.bits;
    i-=(ssize_t) quantum_bits;
    quantum_info->state.bits-=quantum_bits;
    if (quantum_bits < 64)
      *quantum=(unsigned int) (((MagickSizeType) *quantum << quantum_bits) |
        ((quantum_info->state.pixel >> quantum_info->state.bits) &~
        ((~0UL) << quantum_bits)));
  }
  return(pixels);
}

static inline const unsigned char *PushQuantumLongPixel(
  QuantumInfo *quantum_info,const unsigned char *magick_restrict pixels,
  unsigned int *quantum)
{
  ssize_t
    i;

  size_t
    quantum_bits;

  *quantum=0UL;
  for (i=(ssize_t) quantum_info->depth; i > 0; )
  {
    if (quantum_info->state.bits == 0)
      {
        pixels=PushLongPixel(quantum_info->endian,pixels,
          &quantum_info->state.pixel);
        quantum_info->state.bits=32U;
      }
    quantum_bits=(size_t) i;
    if (quantum_bits > quantum_info->state.bits)
      quantum_bits=quantum_info->state.bits;
    *quantum|=(((quantum_info->state.pixel >> (32U-quantum_info->state.bits)) &
      quantum_info->state.mask[quantum_bits]) << ((ssize_t)
      quantum_info->depth-i));
    i-=(ssize_t) quantum_bits;
    quantum_info->state.bits-=quantum_bits;
  }
  return(pixels);
}

static void Import8BitPixelChannel(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,Quantum *magick_restrict q)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    *q=ScaleCharToQuantum(pixel);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitPixelChannel(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  Quantum *magick_restrict q)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    *q=ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel));
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitPixelChannel(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  Quantum *magick_restrict q)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitPixelChannel(image,quantum_info,span,q);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    *q=ScaleShortToQuantum(pixel);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitPixelChannel(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  Quantum *magick_restrict q)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    *q=ClampToQuantum(pixel);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitPixelChannel(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  Quantum *magick_restrict q)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    *q=ScaleLongToQuantum(pixel);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitPixelChannel(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  Quantum *magick_restrict q)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitPixelChannel(image,quantum_info,span,q);
      return;
    }
  else
    {
      ImportInteger32BitPixelChannel(image,quantum_info,span,q);
      return;
    }
}

static void Import24BitFloatPixelChannel(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  Quantum *magick_restrict q)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    *q=ClampToQuantum(pixel);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatPixelChannel(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  Quantum *magick_restrict q)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    *q=ClampToQuantum(pixel);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthPixelChannel(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  Quantum *magick_restrict q)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    *q=ScaleAnyToQuantum(pixel,range);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportPixelChannel(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,PixelChannel channel)
{
  Quantum
    *magick_restrict q = span->q;

  q+=(ptrdiff_t) image->channel_map[channel].offset;
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitPixelChannel(image,quantum_info,span,q);
      break;
    }
    case 16:
    {
      Import16BitPixelChannel(image,quantum_info,span,q);
      break;
    }
    case 32:
    {
      Import32BitPixelChannel(image,quantum_info,span,q);
      break;
    }
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatPixelChannel(image,quantum_info,span,q);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatPixelChannel(image,quantum_info,span,q);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthPixelChannel(image,quantum_info,span,q);
      break;
    }
  }
}

static void ImportAlphaQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  if (image->alpha_trait == UndefinedPixelTrait)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageDoesNotHaveAnAlphaChannel","`%s'",image->filename);
      return;
    }
  ImportPixelChannel(image,quantum_info,span,AlphaPixelChannel);
}

static Quantum *ImportUnpacked12BitSample(const Image *image,
  const ssize_t index,const unsigned short pixel,const QuantumAny range,
  Quantum *magick_restrict q)
{
  switch (index % 3)
  {
    default:
    case 0:
    {
      SetPixelRed(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
        range),q);
      break;
    }
    case 1:
    {
      SetPixelGreen(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
        range),q);
      break;
    }
    case 2:
    {
      SetPixelBlue(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
        range),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
      break;
    }
  }
  return(q);
}

static void ImportUnpacked12BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) (3*number_pixels-1); x+=2)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    q=ImportUnpacked12BitSample(image,x,pixel,range,q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    q=ImportUnpacked12BitSample(image,x+1,pixel,range,q);
    p+=(ptrdiff_t) quantum_info->pad;
  }
  for (bit=0; bit < (ssize_t) (3*number_pixels % 2); bit++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    q=ImportUnpacked12BitSample(image,x+bit,pixel,range,q);
    p+=(ptrdiff_t) quantum_info->pad;
  }
  if (bit != 0)
    p++;
}

static void ImportPacked12BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->quantum == 32U)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumLongPixel(quantum_info,p,&pixel);
        SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumLongPixel(quantum_info,p,&pixel);
        SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumLongPixel(quantum_info,p,&pixel);
        SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import12BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  QuantumAny
    range;

  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked12BitBGRQuantum(image,quantum_info,span,range);
      return;
    }
  else
    {
      ImportPacked12BitBGRQuantum(image,quantum_info,span,range);
      return;
    }
}

static void ImportUnpacked10BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,range),q);
    SetPixelGreen(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,range),
      q);
    SetPixelBlue(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportLong10BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumLongPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumLongPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumLongPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import10BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked10BitBGRQuantum(image,quantum_info,span,range);
      return;
    }
  if (quantum_info->quantum == 32U)
    {
      ImportLong10BitBGRQuantum(image,quantum_info,span,range);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitBGRQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitBGRQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitBGRQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    SetPixelAlpha(image,OpaqueAlpha,q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatBGRQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatBGRQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthBGRQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Import10BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 12:
    {
      Import12BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitBGRQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthBGRQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportUnpacked10BitBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,unsigned int pixel)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    i;

  size_t
    quantum;

  ssize_t
    n;

  n=0;
  quantum=0;
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < 4; i++)
    {
      switch (n % 3)
      {
        case 0:
        {
          p=PushLongPixel(quantum_info->endian,p,&pixel);
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 22) & 0x3ff) << 6)));
          break;
        }
        case 1:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 12) & 0x3ff) << 6)));
          break;
        }
        case 2:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 2) & 0x3ff) << 6)));
          break;
        }
      }
      switch (i)
      {
        case 0: SetPixelRed(image,(Quantum) quantum,q); break;
        case 1: SetPixelGreen(image,(Quantum) quantum,q); break;
        case 2: SetPixelBlue(image,(Quantum) quantum,q); break;
        case 3: SetPixelAlpha(image,(Quantum) quantum,q); break;
      }
      n++;
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import10BitBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  pixel=0;
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked10BitBGRAQuantum(image,quantum_info,span,pixel);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitBGRAQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitBGRAQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitBGRAQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelAlpha(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatBGRAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatBGRAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthBGRAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitBGRAQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Import10BitBGRAQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitBGRAQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitBGRAQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthBGRAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportUnpacked10BitBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,unsigned int pixel)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    i;

  size_t
    quantum;

  ssize_t
    n;

  n=0;
  quantum=0;
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < 4; i++)
    {
      switch (n % 3)
      {
        case 0:
        {
          p=PushLongPixel(quantum_info->endian,p,&pixel);
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 22) & 0x3ff) << 6)));
          break;
        }
        case 1:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 12) & 0x3ff) << 6)));
          break;
        }
        case 2:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 2) & 0x3ff) << 6)));
          break;
        }
      }
      switch (i)
      {
        case 0: SetPixelRed(image,(Quantum) quantum,q); break;
        case 1: SetPixelGreen(image,(Quantum) quantum,q); break;
        case 2: SetPixelBlue(image,(Quantum) quantum,q); break;
        case 3: SetPixelOpacity(image,(Quantum) quantum,q); break;
      }
      n++;
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import10BitBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  pixel=0;
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked10BitBGROQuantum(image,quantum_info,span,pixel);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ScaleShortToQuantum((unsigned short)
      (pixel << 6)),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitBGROQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitBGROQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      unsigned int
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushLongPixel(quantum_info->endian,p,&pixel);
        SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
        p=PushLongPixel(quantum_info->endian,p,&pixel);
        SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
        p=PushLongPixel(quantum_info->endian,p,&pixel);
        SetPixelRed(image,ScaleLongToQuantum(pixel),q);
        p=PushLongPixel(quantum_info->endian,p,&pixel);
        SetPixelOpacity(image,ScaleLongToQuantum(pixel),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
    }
}

static void Import8BitBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelOpacity(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatBGROQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatBGROQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthBGROQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitBGROQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Import10BitBGROQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitBGROQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitBGROQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthBGROQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportBlackQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  if (image->colorspace != CMYKColorspace)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",image->filename);
      return;
    }
  ImportPixelChannel(image,quantum_info,span,BlackPixelChannel);
}

static void ImportUnpacked10BitCbYCrYQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,unsigned int pixel)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  Quantum
    cbcr[4];

  ssize_t
    x;

  ssize_t
    i;

  size_t
    quantum;

  ssize_t
    n;

  n=0;
  quantum=0;
  for (x=0; x < ((ssize_t) number_pixels-3); x+=4)
  {
    for (i=0; i < 4; i++)
    {
      switch (n % 3)
      {
        case 0:
        {
          p=PushLongPixel(quantum_info->endian,p,&pixel);
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 22) & 0x3ff) << 6)));
          break;
        }
        case 1:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 12) & 0x3ff) << 6)));
          break;
        }
        case 2:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 2) & 0x3ff) << 6)));
          break;
        }
      }
      cbcr[i]=(Quantum) (quantum);
      n++;
    }
    p+=(ptrdiff_t) quantum_info->pad;
    SetPixelRed(image,cbcr[1],q);
    SetPixelGreen(image,cbcr[0],q);
    SetPixelBlue(image,cbcr[2],q);
    q+=(ptrdiff_t) GetPixelChannels(image);
    SetPixelRed(image,cbcr[3],q);
    SetPixelGreen(image,cbcr[0],q);
    SetPixelBlue(image,cbcr[2],q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportCbYCrYQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 10:
    {
      pixel=0;
      if (quantum_info->pack == MagickFalse)
        {
          ImportUnpacked10BitCbYCrYQuantum(image,quantum_info,span,pixel);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      break;
    }
  }
}

static void ImportFloat16BitCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitCMYKQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitCMYKQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitCMYKQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlack(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatCMYKQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatCMYKQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthCMYKQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  if (image->colorspace != CMYKColorspace)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",image->filename);
      return;
    }
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitCMYKQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitCMYKQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitCMYKQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthCMYKQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportFloat16BitCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitCMYKAQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitCMYKAQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitCMYKAQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitCMYKAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlack(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelAlpha(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatCMYKAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatCMYKAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthCMYKAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportCMYKAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  if (image->colorspace != CMYKColorspace)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",image->filename);
      return;
    }
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitCMYKAQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitCMYKAQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitCMYKAQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthCMYKAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Import8BitCMYKOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlack(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelOpacity(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitCMYKOQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlack(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitCMYKOQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitCMYKOQuantum(image,quantum_info,span);
      return;
    }
}

static void Import24BitFloatCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlack(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatCMYKOQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatCMYKOQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthCMYKOQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportCMYKOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  if (image->colorspace != CMYKColorspace)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",image->filename);
      return;
    }
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitCMYKOQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitCMYKOQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitCMYKOQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthCMYKOQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Import1BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  Quantum
    black,
    white;

  black=(Quantum) 0;
  white=QuantumRange;
  if (quantum_info->min_is_white != MagickFalse)
    {
      black=QuantumRange;
      white=(Quantum) 0;
    }
  for (x=0; x < ((ssize_t) number_pixels-7); x+=8)
  {
    for (bit=0; bit < 8; bit++)
    {
      SetPixelGray(image,((*p) & (1 << (7-bit))) == 0 ? black : white,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    p++;
  }
  for (bit=0; bit < (ssize_t) (number_pixels % 8); bit++)
  {
    SetPixelGray(image,((*p) & (0x01 << (7-bit))) == 0 ? black : white,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (bit != 0)
    p++;
}

static void Import4BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned char
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < ((ssize_t) number_pixels-1); x+=2)
  {
    pixel=(unsigned char) ((*p >> 4) & 0xf);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((*p) & 0xf);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  for (bit=0; bit < (ssize_t) (number_pixels % 2); bit++)
  {
    pixel=(unsigned char) (*p++ >> 4);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import10BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  pixel=0;
  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      if (image->endian == LSBEndian)
        {
          for (x=0; x < ((ssize_t) number_pixels-2); x+=3)
          {
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,
              range),q);
            q+=(ptrdiff_t) GetPixelChannels(image);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
              range),q);
            q+=(ptrdiff_t) GetPixelChannels(image);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,
              range),q);
            p+=(ptrdiff_t) quantum_info->pad;
            q+=(ptrdiff_t) GetPixelChannels(image);
          }
          if (x++ < ((ssize_t) number_pixels-1))
            {
              p=PushLongPixel(quantum_info->endian,p,&pixel);
              SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,
                range),q);
              q+=(ptrdiff_t) GetPixelChannels(image);
            }
          if (x++ < (ssize_t) number_pixels)
            {
              SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
                range),q);
              q+=(ptrdiff_t) GetPixelChannels(image);
            }
          return;
        }
      for (x=0; x < ((ssize_t) number_pixels-2); x+=3)
      {
        p=PushLongPixel(quantum_info->endian,p,&pixel);
        SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,range),
          q);
        q+=(ptrdiff_t) GetPixelChannels(image);
        SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,range),
          q);
        q+=(ptrdiff_t) GetPixelChannels(image);
        SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,range),
          q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      if (x++ < ((ssize_t) number_pixels-1))
        {
          p=PushLongPixel(quantum_info->endian,p,&pixel);
          SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,
            range),q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
      if (x++ < (ssize_t) number_pixels)
        {
          SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
            range),q);
          q+=(ptrdiff_t) GetPixelChannels(image);
        }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportUnpacked12BitGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned short
    pixel;

  for (x=0; x < ((ssize_t) number_pixels-1); x+=2)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
      range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
      range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  for (bit=0; bit < (ssize_t) (number_pixels % 2); bit++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
      range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  if (bit != 0)
    p++;
}

static void ImportPacked12BitGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import12BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  QuantumAny
    range;

  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked12BitGrayQuantum(image,quantum_info,span,range);
      return;
    }
  else
    {
      ImportPacked12BitGrayQuantum(image,quantum_info,span,range);
      return;
    }
}

static void ImportMinIsWhite16BitGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,QuantumRange-ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->min_is_white != MagickFalse)
    {
      ImportMinIsWhite16BitGrayQuantum(image,quantum_info,span);
      return;
    }
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitGrayQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGray(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitGrayQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitGrayQuantum(image,quantum_info,span);
      return;
    }
}

static void ImportMinIsWhite8BitGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelGray(image,QuantumRange-ScaleCharToQuantum(pixel),q);
    SetPixelAlpha(image,OpaqueAlpha,q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import8BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  if (quantum_info->min_is_white != MagickFalse)
    {
      ImportMinIsWhite8BitGrayQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelGray(image,ScaleCharToQuantum(pixel),q);
    SetPixelAlpha(image,OpaqueAlpha,q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGray(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGray(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 32:
    {
      Import32BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatGrayQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatGrayQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthGrayQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 1:
    {
      Import1BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 4:
    {
      Import4BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 8:
    {
      Import8BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Import10BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 12:
    {
      Import12BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitGrayQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthGrayQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Import1BitGrayAlphaQuantum(const Image *image,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned char
    pixel;

  bit=0;
  for (x=((ssize_t) number_pixels-3); x > 0; x-=4)
  {
    for (bit=0; bit < 8; bit+=2)
    {
      pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
      SetPixelGray(image,(Quantum) (pixel == 0 ? 0 : QuantumRange),q);
      SetPixelAlpha(image,((*p) & (1UL << (unsigned char) (6-bit))) == 0 ?
        TransparentAlpha : OpaqueAlpha,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    p++;
  }
  if ((number_pixels % 4) != 0)
    for (bit=3; bit >= (ssize_t) (4-(number_pixels % 4)); bit-=2)
    {
      pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
      SetPixelGray(image,(Quantum) (pixel != 0 ? 0 : QuantumRange),q);
      SetPixelAlpha(image,((*p) & (1UL << (unsigned char) (6-bit))) == 0 ?
        TransparentAlpha : OpaqueAlpha,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
  if (bit != 0)
    p++;
}

static void Import4BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned char
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned char) ((*p >> 4) & 0xf);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    pixel=(unsigned char) ((*p) & 0xf);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import10BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import12BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitGrayAlphaQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGray(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGray(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitGrayAlphaQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitGrayAlphaQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelGray(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelAlpha(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGray(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGray(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 32:
    {
      Import32BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatGrayAlphaQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatGrayAlphaQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportGrayAlphaQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 1:
    {
      Import1BitGrayAlphaQuantum(image,span);
      break;
    }
    case 4:
    {
      Import4BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 8:
    {
      Import8BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Import10BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 12:
    {
      Import12BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
  }
}

static inline unsigned char GetColormapBit(const QuantumInfo *quantum_info,
  const unsigned char *magick_restrict p,const ssize_t bit)
{
  unsigned char
    pixel;

  if (quantum_info->min_is_white == MagickFalse)
    pixel=(unsigned char) (((*p) & (1 << (7-bit))) == 0 ? 0x00 : 0x01);
  else
    pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x00 : 0x01);
  return(pixel);
}

static MagickBooleanType Import1BitIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned char
    pixel;

  for (x=0; x < ((ssize_t) number_pixels-7); x+=8)
  {
    for (bit=0; bit < 8; bit++)
    {
      pixel=GetColormapBit(quantum_info,p,bit);
      SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),
        q);
      SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
        GetPixelIndex(image,q),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    p++;
  }
  for (bit=0; bit < (ssize_t) (number_pixels % 8); bit++)
  {
    pixel=GetColormapBit(quantum_info,p,bit);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import4BitIndexQuantum(const Image *image,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned char
    pixel;

  for (x=0; x < ((ssize_t) number_pixels-1); x+=2)
  {
    pixel=(unsigned char) ((*p >> 4) & 0xf);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((*p) & 0xf);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  for (bit=0; bit < (ssize_t) (number_pixels % 2); bit++)
  {
    pixel=(unsigned char) ((*p++ >> 4) & 0xf);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import8BitIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import16BitIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelIndex(image,PushColormapIndex(image,(size_t)
          ClampToQuantum((double) QuantumRange*(double)
          HalfToSinglePrecision(pixel)),&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      return(range_exception);
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType ImportFloat32BitIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,(size_t)
      ClampToQuantum(pixel),&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType ImportInteger32BitIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,pixel,
      &range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import32BitIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  MagickBooleanType
    range_exception = MagickFalse;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      return(ImportFloat32BitIndexQuantum(image,quantum_info,span));
    }
  else
    {
      return(ImportInteger32BitIndexQuantum(image,quantum_info,span));
    }
  return(range_exception);
}

static MagickBooleanType Import24BitFloatIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,(size_t)
      ClampToQuantum(pixel),&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import64BitFloatIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,(size_t)
      ClampToQuantum(pixel),&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType ImportAnyDepthIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static void ImportOtherDepthIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  MagickBooleanType *range_exception)
{
  switch (quantum_info->depth)
  {
    case 32:
    {
      *range_exception=Import32BitIndexQuantum(image,quantum_info,span);
      break;
    }
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          *range_exception=Import24BitFloatIndexQuantum(image,quantum_info,
            span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          *range_exception=Import64BitFloatIndexQuantum(image,quantum_info,
            span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      *range_exception=ImportAnyDepthIndexQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportIndexQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  MagickBooleanType
    range_exception;

  if (image->storage_class != PseudoClass)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColormappedImageRequired","`%s'",image->filename);
      return;
    }
  range_exception=MagickFalse;
  switch (quantum_info->depth)
  {
    case 1:
    {
      range_exception=Import1BitIndexQuantum(image,quantum_info,span);
      break;
    }
    case 4:
    {
      range_exception=Import4BitIndexQuantum(image,span);
      break;
    }
    case 8:
    {
      range_exception=Import8BitIndexQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      range_exception=Import16BitIndexQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthIndexQuantum(image,quantum_info,span,&range_exception);
      break;
    }
  }
  if (range_exception != MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
      "InvalidColormapIndex","`%s'",image->filename);
}

static void Import1BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned char
    pixel;

  for (x=((ssize_t) number_pixels-3); x > 0; x-=4)
  {
    for (bit=0; bit < 8; bit+=2)
    {
      pixel=GetColormapBit(quantum_info,p,bit);
      SetPixelGray(image,(Quantum) (pixel == 0 ? 0 : QuantumRange),q);
      SetPixelAlpha(image,((*p) & (1UL << (unsigned char) (6-bit))) == 0 ?
        TransparentAlpha : OpaqueAlpha,q);
      SetPixelIndex(image,(Quantum) (pixel == 0 ? 0 : 1),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  if ((number_pixels % 4) != 0)
    for (bit=3; bit >= (ssize_t) (4-(number_pixels % 4)); bit-=2)
    {
      pixel=GetColormapBit(quantum_info,p,bit);
      SetPixelIndex(image,(Quantum) (pixel == 0 ? 0 : 1),q);
      SetPixelGray(image,(Quantum) (pixel == 0 ? 0 : QuantumRange),q);
      SetPixelAlpha(image,((*p) & (1UL << (unsigned char) (6-bit))) == 0 ?
        TransparentAlpha : OpaqueAlpha,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
}

static MagickBooleanType Import4BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned char
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned char) ((*p >> 4) & 0xf);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    pixel=(unsigned char) ((*p) & 0xf);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import8BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p=PushCharPixel(p,&pixel);
    SetPixelAlpha(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import16BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelIndex(image,PushColormapIndex(image,(size_t)
          ClampToQuantum((double) QuantumRange*(double)
          HalfToSinglePrecision(pixel)),&range_exception),q);
        SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
          GetPixelIndex(image,q),q);
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
          HalfToSinglePrecision(pixel)),q);
        p+=(ptrdiff_t) quantum_info->pad;
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      return(range_exception);
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType ImportFloat32BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,(size_t)
      ClampToQuantum(pixel),&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType ImportInteger32BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,pixel,
      &range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import32BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  MagickBooleanType
    range_exception = MagickFalse;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      return(ImportFloat32BitIndexAlphaQuantum(image,quantum_info,span));
    }
  else
    {
      return(ImportInteger32BitIndexAlphaQuantum(image,quantum_info,span));
    }
  return(range_exception);
}

static MagickBooleanType Import24BitFloatIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,(size_t)
      ClampToQuantum(pixel),&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType Import64BitFloatIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,(size_t)
      ClampToQuantum(pixel),&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static MagickBooleanType ImportAnyDepthIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  MagickBooleanType
    range_exception = MagickFalse;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelIndex(image,PushColormapIndex(image,pixel,&range_exception),q);
    SetPixelViaPixelInfo(image,image->colormap+(ssize_t)
      GetPixelIndex(image,q),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(range_exception);
}

static void ImportOtherDepthIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  MagickBooleanType *range_exception)
{
  switch (quantum_info->depth)
  {
    case 32:
    {
      *range_exception=Import32BitIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          *range_exception=Import24BitFloatIndexAlphaQuantum(image,quantum_info,
            span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          *range_exception=Import64BitFloatIndexAlphaQuantum(image,quantum_info,
            span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      *range_exception=ImportAnyDepthIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportIndexAlphaQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  MagickBooleanType
    range_exception;

  if (image->storage_class != PseudoClass)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColormappedImageRequired","`%s'",image->filename);
      return;
    }
  range_exception=MagickFalse;
  switch (quantum_info->depth)
  {
    case 1:
    {
      Import1BitIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 4:
    {
      range_exception=Import4BitIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 8:
    {
      range_exception=Import8BitIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      range_exception=Import16BitIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthIndexAlphaQuantum(image,quantum_info,span,
        &range_exception);
      break;
    }
  }
  if (range_exception != MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
      "InvalidColormapIndex","`%s'",image->filename);
}

static void ImportFloat16BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      p=PushShortPixel(quantum_info->endian,p,&pixel);
      q[i]=ClampToQuantum((double) QuantumRange*(double)HalfToSinglePrecision(pixel));
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitMultispectralQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      p=PushShortPixel(quantum_info->endian,p,&pixel);
      q[i]=ScaleShortToQuantum(pixel);
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      p=PushQuantumFloatPixel(quantum_info,p,&pixel);
      q[i]=ClampToQuantum(pixel);
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      p=PushLongPixel(quantum_info->endian,p,&pixel);
      q[i]=ScaleLongToQuantum(pixel);
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitMultispectralQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitMultispectralQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      p=PushCharPixel(p,&pixel);
      q[i]=ScaleCharToQuantum(pixel);
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
      q[i]=ClampToQuantum(pixel);
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      p=PushDoublePixel(quantum_info,p,&pixel);
      q[i]=ClampToQuantum(pixel);
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    i,
    x;

  unsigned int
    pixel = 0;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      p=PushQuantumPixel(quantum_info,p,&pixel);
      q[i]=ScaleAnyToQuantum(pixel,range);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatMultispectralQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatMultispectralQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthMultispectralQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportMultispectralQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  if (image->number_meta_channels == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "MultispectralImageRequired","`%s'",image->filename);
      return;
    }
  if (quantum_info->meta_channel != 0)
    {
      ImportPixelChannel(image,quantum_info,span,
        (PixelChannel) (MetaPixelChannels+quantum_info->meta_channel-1));
      return;
    }
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitMultispectralQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitMultispectralQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitMultispectralQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthMultispectralQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Import8BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelOpacity(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitOpacityQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitOpacityQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitOpacityQuantum(image,quantum_info,span);
      return;
    }
}

static void Import24BitFloatOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ScaleAnyToQuantum(pixel,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatOpacityQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatOpacityQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthOpacityQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportOpacityQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo* exception)
{
  if (image->alpha_trait == UndefinedPixelTrait)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageDoesNotHaveAnAlphaChannel","`%s'",image->filename);
      return;
    }
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitOpacityQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitOpacityQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitOpacityQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthOpacityQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportUnpacked10BitRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,range),q);
    SetPixelGreen(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,range),
      q);
    SetPixelBlue(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,range),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportLong10BitRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumLongPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumLongPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumLongPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import10BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked10BitRGBQuantum(image,quantum_info,span,range);
      return;
    }
  if (quantum_info->quantum == 32U)
    {
      ImportLong10BitRGBQuantum(image,quantum_info,span,range);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportUnpacked12BitRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) (3*number_pixels-1); x+=2)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    q=ImportUnpacked12BitSample(image,x,pixel,range,q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    q=ImportUnpacked12BitSample(image,x+1,pixel,range,q);
    p+=(ptrdiff_t) quantum_info->pad;
  }
  for (bit=0; bit < (ssize_t) (3*number_pixels % 2); bit++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    q=ImportUnpacked12BitSample(image,x+bit,pixel,range,q);
    p+=(ptrdiff_t) quantum_info->pad;
  }
  if (bit != 0)
    p++;
}

static void ImportPacked12BitRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->quantum == 32U)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumLongPixel(quantum_info,p,&pixel);
        SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumLongPixel(quantum_info,p,&pixel);
        SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
        p=PushQuantumLongPixel(quantum_info,p,&pixel);
        SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import12BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  QuantumAny
    range;

  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked12BitRGBQuantum(image,quantum_info,span,range);
      return;
    }
  else
    {
      ImportPacked12BitRGBQuantum(image,quantum_info,span,range);
      return;
    }
}

static void ImportFloat16BitRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitRGBQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitRGBQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitRGBQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    SetPixelAlpha(image,OpaqueAlpha,q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatRGBQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatRGBQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthRGBQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Import10BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 12:
    {
      Import12BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitRGBQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthRGBQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportUnpacked10BitRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,unsigned int pixel)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    i;

  size_t
    quantum;

  ssize_t
    n;

  n=0;
  quantum=0;
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < 4; i++)
    {
      switch (n % 3)
      {
        case 0:
        {
          p=PushLongPixel(quantum_info->endian,p,&pixel);
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 22) & 0x3ff) << 6)));
          break;
        }
        case 1:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 12) & 0x3ff) << 6)));
          break;
        }
        case 2:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 2) & 0x3ff) << 6)));
          break;
        }
      }
      switch (i)
      {
        case 0: SetPixelRed(image,(Quantum) quantum,q); break;
        case 1: SetPixelGreen(image,(Quantum) quantum,q); break;
        case 2: SetPixelBlue(image,(Quantum) quantum,q); break;
        case 3: SetPixelAlpha(image,(Quantum) quantum,q); break;
      }
      n++;
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import10BitRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  pixel=0;
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked10BitRGBAQuantum(image,quantum_info,span,pixel);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitRGBAQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelAlpha(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitRGBAQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitRGBAQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelAlpha(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelAlpha(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatRGBAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatRGBAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthRGBAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitRGBAQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Import10BitRGBAQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitRGBAQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitRGBAQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthRGBAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportUnpacked10BitRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,unsigned int pixel)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    i;

  size_t
    quantum;

  ssize_t
    n;

  n=0;
  quantum=0;
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < 4; i++)
    {
      switch (n % 3)
      {
        case 0:
        {
          p=PushLongPixel(quantum_info->endian,p,&pixel);
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 22) & 0x3ff) << 6)));
          break;
        }
        case 1:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 12) & 0x3ff) << 6)));
          break;
        }
        case 2:
        {
          quantum=(size_t) (ScaleShortToQuantum((unsigned short)
            (((pixel >> 2) & 0x3ff) << 6)));
          break;
        }
      }
      switch (i)
      {
        case 0: SetPixelRed(image,(Quantum) quantum,q); break;
        case 1: SetPixelGreen(image,(Quantum) quantum,q); break;
        case 2: SetPixelBlue(image,(Quantum) quantum,q); break;
        case 3: SetPixelOpacity(image,(Quantum) quantum,q); break;
      }
      n++;
    }
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import10BitRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  pixel=0;
  if (quantum_info->pack == MagickFalse)
    {
      ImportUnpacked10BitRGBOQuantum(image,quantum_info,span,pixel);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ScaleShortToQuantum((unsigned short) (pixel << 6)),
      q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat16BitRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum((double) QuantumRange*(double)
      HalfToSinglePrecision(pixel)),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import16BitRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat16BitRGBOQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleShortToQuantum(pixel),q);
    p=PushShortPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ScaleShortToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportFloat32BitRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloatPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportInteger32BitRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelRed(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelGreen(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelBlue(image,ScaleLongToQuantum(pixel),q);
    p=PushLongPixel(quantum_info->endian,p,&pixel);
    SetPixelOpacity(image,ScaleLongToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import32BitRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ImportFloat32BitRGBOQuantum(image,quantum_info,span);
      return;
    }
  else
    {
      ImportInteger32BitRGBOQuantum(image,quantum_info,span);
      return;
    }
}

static void Import8BitRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushCharPixel(p,&pixel);
    SetPixelRed(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelGreen(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelBlue(image,ScaleCharToQuantum(pixel),q);
    p=PushCharPixel(p,&pixel);
    SetPixelOpacity(image,ScaleCharToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import24BitFloatRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  float
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushQuantumFloat24Pixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void Import64BitFloatRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelRed(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ClampToQuantum(pixel),q);
    p=PushDoublePixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ClampToQuantum(pixel),q);
    p+=(ptrdiff_t) quantum_info->pad;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportAnyDepthRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const unsigned char
    *magick_restrict p = span->p;

  Quantum
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelRed(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelGreen(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelBlue(image,ScaleAnyToQuantum(pixel,range),q);
    p=PushQuantumPixel(quantum_info,p,&pixel);
    SetPixelOpacity(image,ScaleAnyToQuantum(pixel,range),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void ImportOtherDepthRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 24:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import24BitFloatRGBOQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Import64BitFloatRGBOQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ImportAnyDepthRGBOQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ImportRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 8:
    {
      Import8BitRGBOQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Import10BitRGBOQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Import16BitRGBOQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Import32BitRGBOQuantum(image,quantum_info,span);
      break;
    }
    default:
    {
      ImportOtherDepthRGBOQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void DisassociateQuantumAlpha(const Image *image,CacheView *image_view,
  const MagickSizeType number_pixels)
{
  double
    gamma,
    Sa;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  /*
    Disassociate alpha.
  */
  q=GetAuthenticPixelQueue(image);
  if (image_view != (CacheView *) NULL)
    q=GetCacheViewAuthenticPixelQueue(image_view);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    ssize_t
      i;

    Sa=QuantumScale*(double) GetPixelAlpha(image,q);
    gamma=MagickSafeReciprocal(Sa);
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((channel == AlphaPixelChannel) ||
          ((traits & UpdatePixelTrait) == 0))
        continue;
      q[i]=ClampToQuantum(gamma*(double) q[i]);
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport size_t ImportQuantumPixels(const Image *image,
  CacheView *image_view,QuantumInfo *quantum_info,
  const QuantumType quantum_type,const unsigned char *magick_restrict pixels,
  ExceptionInfo *exception)
{
  const unsigned char
    *magick_restrict p;

  MagickSizeType
    number_pixels;

  ssize_t
    x;

  Quantum
    *magick_restrict q;

  size_t
    extent;

  QuantumPixelSpan span;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (pixels == (const unsigned char *) NULL)
    pixels=(const unsigned char *) GetQuantumPixels(quantum_info);
  x=0;
  p=pixels;
  if (image_view == (CacheView *) NULL)
    {
      number_pixels=GetImageExtent(image);
      q=GetAuthenticPixelQueue(image);
    }
  else
    {
      number_pixels=GetCacheViewExtent(image_view);
      q=GetCacheViewAuthenticPixelQueue(image_view);
    }
  if (q == (Quantum *) NULL)
    return(0);
  ResetQuantumState(quantum_info);
  extent=GetQuantumExtent(image,quantum_info,quantum_type);
  span.number_pixels=number_pixels;
  span.p=p;
  span.q=q;

  switch (quantum_type)
  {
    case AlphaQuantum:
    {
      ImportAlphaQuantum(image,quantum_info,&span,exception);
      break;
    }
    case BGRQuantum:
    {
      ImportBGRQuantum(image,quantum_info,&span);
      break;
    }
    case BGRAQuantum:
    {
      ImportBGRAQuantum(image,quantum_info,&span);
      break;
    }
    case BGROQuantum:
    {
      ImportBGROQuantum(image,quantum_info,&span);
      break;
    }
    case BlackQuantum:
    {
      ImportBlackQuantum(image,quantum_info,&span,exception);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      ImportPixelChannel(image,quantum_info,&span,BluePixelChannel);
      break;
    }
    case CMYKQuantum:
    {
      ImportCMYKQuantum(image,quantum_info,&span,exception);
      break;
    }
    case CMYKAQuantum:
    {
      ImportCMYKAQuantum(image,quantum_info,&span,exception);
      break;
    }
    case MultispectralQuantum:
    {
      ImportMultispectralQuantum(image,quantum_info,&span,exception);
      break;
    }
    case CMYKOQuantum:
    {
      ImportCMYKOQuantum(image,quantum_info,&span,exception);
      break;
    }
    case CbYCrYQuantum:
    {
      ImportCbYCrYQuantum(image,quantum_info,&span);
      break;
    }
    case GrayQuantum:
    {
      ImportGrayQuantum(image,quantum_info,&span);
      break;
    }
    case GrayAlphaQuantum:
    {
      ImportGrayAlphaQuantum(image,quantum_info,&span);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      ImportPixelChannel(image,quantum_info,&span,GreenPixelChannel);
      break;
    }
    case IndexQuantum:
    {
      ImportIndexQuantum(image,quantum_info,&span,exception);
      break;
    }
    case IndexAlphaQuantum:
    {
      ImportIndexAlphaQuantum(image,quantum_info,&span,exception);
      break;
    }
    case OpacityQuantum:
    {
      ImportOpacityQuantum(image,quantum_info,&span,exception);
      break;
    }
    case RedQuantum:
    case CyanQuantum:
    {
      ImportPixelChannel(image,quantum_info,&span,RedPixelChannel);
      break;
    }
    case RGBQuantum:
    case CbYCrQuantum:
    {
      ImportRGBQuantum(image,quantum_info,&span);
      break;
    }
    case RGBAQuantum:
    case CbYCrAQuantum:
    {
      ImportRGBAQuantum(image,quantum_info,&span);
      break;
    }
    case RGBOQuantum:
    {
      ImportRGBOQuantum(image,quantum_info,&span);
      break;
    }
    default:
      break;
  }
  if ((quantum_type == CbYCrQuantum) || (quantum_type == CbYCrAQuantum))
    {
      Quantum
        quantum;

      q=GetAuthenticPixelQueue(image);
      if (image_view != (CacheView *) NULL)
        q=GetCacheViewAuthenticPixelQueue(image_view);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        quantum=GetPixelRed(image,q);
        SetPixelRed(image,GetPixelGreen(image,q),q);
        SetPixelGreen(image,quantum,q);
        q+=(ptrdiff_t) GetPixelChannels(image);
      }
    }
  if (quantum_info->alpha_type == AssociatedQuantumAlpha)
    DisassociateQuantumAlpha(image,image_view,number_pixels);
  return(extent);
}
