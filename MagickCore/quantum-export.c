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
%                   EEEEE  X   X  PPPP    OOO   RRRR   TTTTT                  %
%                   E       X X   P   P  O   O  R   R    T                    %
%                   EEE      X    PPPP   O   O  RRRR     T                    %
%                   E       X X   P      O   O  R R      T                    %
%                   EEEEE  X   X  P       OOO   R  R     T                    %
%                                                                             %
%                 MagickCore Methods to Export Quantum Pixels                 %
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

  const Quantum
    *magick_restrict p;

  unsigned char
    *magick_restrict q;
} QuantumPixelSpan;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E x p o r t Q u a n t u m P i x e l s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExportQuantumPixels() transfers one or more pixel components from the image
%  pixel cache to a user supplied buffer.  The pixels are returned in network
%  byte order.  It returns the number of exported pixels.
%
%  The format of the ExportQuantumPixels method is:
%
%      size_t ExportQuantumPixels(const Image *image,CacheView *image_view,
%        QuantumInfo *quantum_info,const QuantumType quantum_type,
%        unsigned char *magick_restrict pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o image_view: the image cache view.
%
%    o quantum_info: the quantum info.
%
%    o quantum_type: Declare which pixel components to transfer (RGB, RGBA,
%      etc).
%
%    o pixels:  The components are transferred to this buffer.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline unsigned char *PopQuantumDoublePixel(QuantumInfo *quantum_info,
  const double pixel,unsigned char *magick_restrict pixels)
{
  double
    *p;

  unsigned char
    quantum[8];

  (void) memset(quantum,0,sizeof(quantum));
  p=(double *) quantum;
  *p=(double) (pixel*quantum_info->state.inverse_scale+quantum_info->minimum);
  if (quantum_info->endian == LSBEndian)
    {
      *pixels++=quantum[0];
      *pixels++=quantum[1];
      *pixels++=quantum[2];
      *pixels++=quantum[3];
      *pixels++=quantum[4];
      *pixels++=quantum[5];
      *pixels++=quantum[6];
      *pixels++=quantum[7];
      return(pixels);
    }
  *pixels++=quantum[7];
  *pixels++=quantum[6];
  *pixels++=quantum[5];
  *pixels++=quantum[4];
  *pixels++=quantum[3];
  *pixels++=quantum[2];
  *pixels++=quantum[1];
  *pixels++=quantum[0];
  return(pixels);
}

static inline unsigned char *PopQuantumFloatPixel(QuantumInfo *quantum_info,
  const float pixel,unsigned char *magick_restrict pixels)
{
  float
    *p;

  unsigned char
    quantum[4];

  (void) memset(quantum,0,sizeof(quantum));
  p=(float *) quantum;
  *p=(float) ((double) pixel*quantum_info->state.inverse_scale+
    quantum_info->minimum);
  if (quantum_info->endian == LSBEndian)
    {
      *pixels++=quantum[0];
      *pixels++=quantum[1];
      *pixels++=quantum[2];
      *pixels++=quantum[3];
      return(pixels);
    }
  *pixels++=quantum[3];
  *pixels++=quantum[2];
  *pixels++=quantum[1];
  *pixels++=quantum[0];
  return(pixels);
}

static inline unsigned char *PopQuantumPixel(QuantumInfo *quantum_info,
  const QuantumAny pixel,unsigned char *magick_restrict pixels)
{
  ssize_t
    i;

  size_t
    quantum_bits;

  if (quantum_info->state.bits == 0UL)
    quantum_info->state.bits=8U;
  for (i=(ssize_t) quantum_info->depth; i > 0L; )
  {
    quantum_bits=(size_t) i;
    if (quantum_bits > quantum_info->state.bits)
      quantum_bits=quantum_info->state.bits;
    i-=(ssize_t) quantum_bits;
    if (i < 0)
      i=0;
    if (quantum_info->state.bits == 8UL)
      *pixels='\0';
    quantum_info->state.bits-=quantum_bits;
    *pixels|=(((pixel >> i) &~ (((QuantumAny) ~0UL) << quantum_bits)) <<
      quantum_info->state.bits);
    if (quantum_info->state.bits == 0UL)
      {
        pixels++;
        quantum_info->state.bits=8UL;
      }
  }
  return(pixels);
}

static inline unsigned char *PopQuantumLongPixel(QuantumInfo *quantum_info,
  const size_t pixel,unsigned char *magick_restrict pixels)
{
  ssize_t
    i;

  size_t
    quantum_bits;

  if (quantum_info->state.bits == 0U)
    quantum_info->state.bits=32UL;
  for (i=(ssize_t) quantum_info->depth; i > 0; )
  {
    quantum_bits=(size_t) i;
    if (quantum_bits > quantum_info->state.bits)
      quantum_bits=quantum_info->state.bits;
    quantum_info->state.pixel|=(((pixel >> ((ssize_t) quantum_info->depth-i)) &
      quantum_info->state.mask[quantum_bits]) << (32U-
        quantum_info->state.bits));
    i-=(ssize_t) quantum_bits;
    quantum_info->state.bits-=quantum_bits;
    if (quantum_info->state.bits == 0U)
      {
        pixels=PopLongPixel(quantum_info->endian,quantum_info->state.pixel,
          pixels);
        quantum_info->state.pixel=0U;
        quantum_info->state.bits=32U;
      }
  }
  return(pixels);
}

static void ExportPixelChannel(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const Quantum *magick_restrict p,
  unsigned char *magick_restrict q,PixelChannel channel)
{
  QuantumAny
    range;

  ssize_t
    x;

  p+=(ptrdiff_t) image->channel_map[channel].offset;
  switch (quantum_info->depth)
  {
    case 8:
    {
      unsigned char
        pixel;

      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=ScaleQuantumToChar(*p);
        q=PopCharPixel(pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 16:
    {
      unsigned short
        pixel;

      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            pixel=SinglePrecisionToHalf(QuantumScale*(*p));
            q=PopShortPixel(quantum_info->endian,pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=ScaleQuantumToShort(*p);
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 32:
    {
      unsigned int
        pixel;

      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            q=PopQuantumFloatPixel(quantum_info,(float) *p,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=ScaleQuantumToLong(*p);
        q=PopLongPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            q=PopQuantumDoublePixel(quantum_info,(double) *p,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(*p,range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

static void ExportAlphaQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  if (image->alpha_trait == UndefinedPixelTrait)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageDoesNotHaveAnAlphaChannel","`%s'",image->filename);
      return;
    }
  ExportPixelChannel(image,quantum_info,number_pixels,p,q,AlphaPixelChannel);
}

static void Export8BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopCharPixel(ScaleQuantumToChar(GetPixelBlue(image,p)),q);
    q=PopCharPixel(ScaleQuantumToChar(GetPixelGreen(image,p)),q);
    q=PopCharPixel(ScaleQuantumToChar(GetPixelRed(image,p)),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportUnpacked10BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) (
      ScaleQuantumToAny(GetPixelRed(image,p),range) << 22 |
      ScaleQuantumToAny(GetPixelGreen(image,p),range) << 12 |
      ScaleQuantumToAny(GetPixelBlue(image,p),range) << 2);
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportLong10BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
      range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export10BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
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
      ExportUnpacked10BitBGRQuantum(image,quantum_info,span,range);
      return;
    }
  if (quantum_info->quantum == 32UL)
    {
      ExportLong10BitBGRQuantum(image,quantum_info,span,range);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportUnpacked12BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned int
    pixel;

  for (x=0; x < (3*(ssize_t) number_pixels-1); x+=2)
  {
    switch (x % 3)
    {
      default:
      case 0:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),
          range);
        break;
      }
      case 1:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
          range);
        break;
      }
      case 2:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),
          range);
        p+=(ptrdiff_t) GetPixelChannels(image);
        break;
      }
    }
    q=PopShortPixel(quantum_info->endian,(unsigned short) (pixel << 4),
      q);
    switch ((x+1) % 3)
    {
      default:
      case 0:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),
          range);
        break;
      }
      case 1:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
          range);
        break;
      }
      case 2:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),
          range);
        p+=(ptrdiff_t) GetPixelChannels(image);
        break;
      }
    }
    q=PopShortPixel(quantum_info->endian,(unsigned short) (pixel << 4),
      q);
    q+=(ptrdiff_t) quantum_info->pad;
  }
  for (bit=0; bit < (ssize_t) (3*number_pixels % 2); bit++)
  {
    switch ((x+bit) % 3)
    {
      default:
      case 0:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),
          range);
        break;
      }
      case 1:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
          range);
        break;
      }
      case 2:
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),
          range);
        p+=(ptrdiff_t) GetPixelChannels(image);
        break;
      }
    }
    q=PopShortPixel(quantum_info->endian,(unsigned short) (pixel << 4),
      q);
    q+=(ptrdiff_t) quantum_info->pad;
  }
  if (bit != 0)
    p+=(ptrdiff_t) GetPixelChannels(image);
}

static void ExportLong12BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
      range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export12BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
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
      ExportUnpacked12BitBGRQuantum(image,quantum_info,span,range);
      return;
    }
  if (quantum_info->quantum == 32UL)
    {
      ExportLong12BitBGRQuantum(image,quantum_info,span,range);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportFloat16BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=SinglePrecisionToHalf(QuantumScale*(double)
      GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=SinglePrecisionToHalf(QuantumScale*(double)
      GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=SinglePrecisionToHalf(QuantumScale*(double)
      GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ExportFloat16BitBGRQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportFloat32BitBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),q);
    q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),q);
    q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ExportFloat32BitBGRQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelBlue(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthBGRQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportBGRQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 8:
    {
      Export8BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Export10BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 12:
    {
      Export12BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitBGRQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatBGRQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthBGRQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(GetPixelBlue(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelGreen(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelRed(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelAlpha(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportUnpacked10BitBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  ssize_t
    i;

  size_t
    quantum;

  ssize_t
    n;

  n=0;
  quantum=0;
  pixel=0;
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < 4; i++)
    {
      switch (i)
      {
        case 0: quantum=(size_t) GetPixelRed(image,p); break;
        case 1: quantum=(size_t) GetPixelGreen(image,p); break;
        case 2: quantum=(size_t) GetPixelBlue(image,p); break;
        case 3: quantum=(size_t) GetPixelAlpha(image,p); break;
      }
      switch (n % 3)
      {
        case 0:
        {
          pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
            range) << 22);
          break;
        }
        case 1:
        {
          pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
            range) << 12);
          break;
        }
        case 2:
        {
          pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
            range) << 2);
          q=PopLongPixel(quantum_info->endian,pixel,q);
          pixel=0;
          break;
        }
      }
      n++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportLong10BitBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
      range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelAlpha(image,p),
      range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export10BitBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
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
      ExportUnpacked10BitBGRAQuantum(image,quantum_info,span,range);
      return;
    }
  if (quantum_info->quantum == 32UL)
    {
      ExportLong10BitBGRAQuantum(image,quantum_info,span,range);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelAlpha(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportFloat16BitBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=SinglePrecisionToHalf(QuantumScale*(double)
      GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=SinglePrecisionToHalf(QuantumScale*(double)
      GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=SinglePrecisionToHalf(QuantumScale*(double)
      GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=SinglePrecisionToHalf(QuantumScale*(double)
      GetPixelAlpha(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ExportFloat16BitBGRAQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelAlpha(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportFloat32BitBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    float
      float_pixel;

    q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),q);
    q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),q);
    q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),q);
    float_pixel=(float) GetPixelAlpha(image,p);
    q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      ExportFloat32BitBGRAQuantum(image,quantum_info,span);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelAlpha(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelBlue(image,p),q);
    pixel=(double) GetPixelAlpha(image,p);
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthBGRAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelAlpha(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportBGRAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 8:
    {
      Export8BitBGRAQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Export10BitBGRAQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitBGRAQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitBGRAQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatBGRAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthBGRAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(GetPixelBlue(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelGreen(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelRed(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelOpacity(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportUnpacked10BitBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  ssize_t
    i;

  size_t
    quantum;

  ssize_t
    n;

  n=0;
  quantum=0;
  pixel=0;
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < 4; i++)
    {
      switch (i)
      {
        case 0: quantum=(size_t) GetPixelRed(image,p); break;
        case 1: quantum=(size_t) GetPixelGreen(image,p); break;
        case 2: quantum=(size_t) GetPixelBlue(image,p); break;
        case 3: quantum=(size_t) GetPixelOpacity(image,p); break;
      }
      switch (n % 3)
      {
        case 0:
        {
          pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
            range) << 22);
          break;
        }
        case 1:
        {
          pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
            range) << 12);
          break;
        }
        case 2:
        {
          pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
            range) << 2);
          q=PopLongPixel(quantum_info->endian,pixel,q);
          pixel=0;
          break;
        }
      }
      n++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportLong10BitBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,QuantumAny range)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
      range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelOpacity(image,p),
      range);
    q=PopQuantumLongPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export10BitBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
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
      ExportUnpacked10BitBGROQuantum(image,quantum_info,span,range);
      return;
    }
  if (quantum_info->quantum == 32UL)
    {
      ExportLong10BitBGROQuantum(image,quantum_info,span,range);
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelOpacity(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlue(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelGreen(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelRed(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelOpacity(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelOpacity(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        float
          float_pixel;

        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),q);
        float_pixel=(float) GetPixelOpacity(image,p);
        q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelOpacity(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelBlue(image,p),q);
    pixel=(double) GetPixelOpacity(image,p);
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthBGROQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelOpacity(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportBGROQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 8:
    {
      Export8BitBGROQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Export10BitBGROQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitBGROQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitBGROQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatBGROQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthBGROQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void ExportBlackQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  if (image->colorspace != CMYKColorspace)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",image->filename);
      return;
    }
  ExportPixelChannel(image,quantum_info,number_pixels,p,q,BlackPixelChannel);
}

static void ExportCbYCrYQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  Quantum
    cbcr[4];

  ssize_t
    i,
    x;

  unsigned int
    pixel;

  size_t
    quantum;

  ssize_t
    n;

  n=0;
  quantum=0;
  switch (quantum_info->depth)
  {
    case 10:
    {
      if (quantum_info->pack == MagickFalse)
        {
          for (x=0; x < (ssize_t) number_pixels; x+=2)
          {
            for (i=0; i < 4; i++)
            {
              switch (n % 3)
              {
                case 0:
                {
                  quantum=(size_t) GetPixelRed(image,p);
                  break;
                }
                case 1:
                {
                  quantum=(size_t) GetPixelGreen(image,p);
                  break;
                }
                case 2:
                {
                  quantum=(size_t) GetPixelBlue(image,p);
                  break;
                }
              }
              cbcr[i]=(Quantum) quantum;
              n++;
            }
            pixel=(unsigned int) ((size_t) (cbcr[1]) << 22 | (size_t)
              (cbcr[0]) << 12 | (size_t) (cbcr[2]) << 2);
            q=PopLongPixel(quantum_info->endian,pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            pixel=(unsigned int) ((size_t) (cbcr[3]) << 22 | (size_t)
              (cbcr[0]) << 12 | (size_t) (cbcr[2]) << 2);
            q=PopLongPixel(quantum_info->endian,pixel,q);
            p+=(ptrdiff_t) GetPixelChannels(image);
            q+=(ptrdiff_t) quantum_info->pad;
          }
          break;
        }
      break;
    }
    default:
    {
      QuantumAny
        range;

      for (x=0; x < (ssize_t) number_pixels; x+=2)
      {
        for (i=0; i < 4; i++)
        {
          switch (n % 3)
          {
            case 0:
            {
              quantum=(size_t) GetPixelRed(image,p);
              break;
            }
            case 1:
            {
              quantum=(size_t) GetPixelGreen(image,p);
              break;
            }
            case 2:
            {
              quantum=(size_t) GetPixelBlue(image,p);
              break;
            }
          }
          cbcr[i]=(Quantum) quantum;
          n++;
        }
        range=GetQuantumRange(quantum_info->depth);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[1],range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[0],range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[2],range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[3],range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[0],range),q);
        q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(cbcr[2],range),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      break;
    }
  }
}

static void Export8BitCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(GetPixelRed(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelGreen(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelBlue(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelBlack(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelRed(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelGreen(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlue(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlack(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlack(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlack(image,p),
          q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlack(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelBlue(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelBlack(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthCMYKQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  QuantumAny
    range;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlack(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportCMYKQuantum(const Image *image,QuantumInfo *quantum_info,
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
      Export8BitCMYKQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitCMYKQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitCMYKQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatCMYKQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthCMYKQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitCMYKAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(GetPixelRed(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelGreen(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelBlue(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelBlack(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelAlpha(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelRed(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelGreen(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlue(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlack(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelAlpha(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlack(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelAlpha(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumFloatPixel(quantum_info,(float)
          GetPixelRed(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float)
          GetPixelGreen(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float)
          GetPixelBlue(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float)
          GetPixelBlack(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float)
          GetPixelAlpha(image,p),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlack(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelAlpha(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelBlue(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelBlack(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelAlpha(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthCMYKAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  QuantumAny
    range;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlack(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelAlpha(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportCMYKAQuantum(const Image *image,QuantumInfo *quantum_info,
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
      Export8BitCMYKAQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitCMYKAQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitCMYKAQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatCMYKAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthCMYKAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitCMYKOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(GetPixelRed(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelGreen(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelBlue(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelBlack(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelOpacity(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelRed(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelGreen(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlue(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlack(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelOpacity(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlack(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelOpacity(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        float
          float_pixel;

        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlack(image,p),
          q);
        float_pixel=(float) (GetPixelOpacity(image,p));
        q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlack(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelOpacity(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelBlue(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelBlack(image,p),q);
    pixel=(double) (GetPixelOpacity(image,p));
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthCMYKOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  QuantumAny
    range;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlack(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(
      GetPixelOpacity(image,p),range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportCMYKOQuantum(const Image *image,QuantumInfo *quantum_info,
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
      Export8BitCMYKOQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitCMYKOQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitCMYKOQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatCMYKOQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthCMYKOQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export1BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    threshold;

  unsigned char
    black,
    white;

  ssize_t
    bit;

  black=0x00;
  white=0x01;
  if (quantum_info->min_is_white != MagickFalse)
    {
      black=0x01;
      white=0x00;
    }
  threshold=(double) QuantumRange/2.0;
  for (x=((ssize_t) number_pixels-7); x > 0; x-=8)
  {
    *q='\0';
    *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 7;
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 6;
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 5;
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 4;
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 3;
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 2;
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 1;
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) < threshold ? black : white) << 0;
    p+=(ptrdiff_t) GetPixelChannels(image);
    q++;
  }
  if ((number_pixels % 8) != 0)
    {
      *q='\0';
      for (bit=7; bit >= (ssize_t) (8-(number_pixels % 8)); bit--)
      {
        *q|=(GetPixelLuma(image,p) < threshold ? black : white) << bit;
        p+=(ptrdiff_t) GetPixelChannels(image);
      }
      q++;
    }
}

static void Export4BitGrayQuantum(const Image *image,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < ((ssize_t) number_pixels-1) ; x+=2)
  {
    pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
    *q=(((pixel >> 4) & 0xf) << 4);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
    *q|=pixel >> 4;
    p+=(ptrdiff_t) GetPixelChannels(image);
    q++;
  }
  if ((number_pixels % 2) != 0)
    {
      pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
      *q=(((pixel >> 4) & 0xf) << 4);
      p+=(ptrdiff_t) GetPixelChannels(image);
      q++;
    }
}

static void Export8BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export10BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      unsigned int
        pixel;

      for (x=0; x < ((ssize_t) number_pixels-2); x+=3)
      {
        pixel=(unsigned int) (ScaleQuantumToAny(ClampToQuantum(
          GetPixelLuma(image,p+2*GetPixelChannels(image))),range) << 22 |
          ScaleQuantumToAny(ClampToQuantum(GetPixelLuma(image,p+
          GetPixelChannels(image))),range) << 12 | ScaleQuantumToAny(
          ClampToQuantum(GetPixelLuma(image,p)),range) << 2);
        q=PopLongPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) 3*GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      if (x < (ssize_t) number_pixels)
        {
          pixel=0U;
          if (x++ < ((ssize_t) number_pixels-1))
            pixel|=ScaleQuantumToAny(ClampToQuantum(GetPixelLuma(image,p+
              GetPixelChannels(image))),range) << 12;
          if (x++ < (ssize_t) number_pixels)
            pixel|=ScaleQuantumToAny(ClampToQuantum(GetPixelLuma(image,p)),
              range) << 2;
          q=PopLongPixel(quantum_info->endian,pixel,q);
        }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(ClampToQuantum(
      GetPixelLuma(image,p)),range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export12BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  unsigned short
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=ScaleQuantumToShort(ClampToQuantum(GetPixelLuma(image,p)));
        q=PopShortPixel(quantum_info->endian,(unsigned short) (pixel >> 4),
          q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(ClampToQuantum(
      GetPixelLuma(image,p)),range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelLuma(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(ClampToQuantum(GetPixelLuma(image,p)));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        float
          float_pixel;

        float_pixel=(float) GetPixelLuma(image,p);
        q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(ClampToQuantum(GetPixelLuma(image,p)));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    double
      pixel;

    pixel=GetPixelLuma(image,p);
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthGrayQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(ClampToQuantum(
      GetPixelLuma(image,p)),range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 1:
    {
      Export1BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 4:
    {
      Export4BitGrayQuantum(image,span);
      break;
    }
    case 8:
    {
      Export8BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Export10BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 12:
    {
      Export12BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitGrayQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatGrayQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthGrayQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export1BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    threshold;

  unsigned char
    black,
    pixel,
    white;

  ssize_t
    bit;

  black=0x00;
  white=0x01;
  if (quantum_info->min_is_white != MagickFalse)
    {
      black=0x01;
      white=0x00;
    }
  threshold=(double) QuantumRange/2.0;
  for (x=((ssize_t) number_pixels-3); x > 0; x-=4)
  {
    *q='\0';
    *q|=(GetPixelLuma(image,p) > threshold ? black : white) << 7;
    pixel=(unsigned char) (GetPixelAlpha(image,p) == OpaqueAlpha ?
      0x00 : 0x01);
    *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 6);
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) > threshold ? black : white) << 5;
    pixel=(unsigned char) (GetPixelAlpha(image,p) == OpaqueAlpha ?
      0x00 : 0x01);
    *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 4);
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) > threshold ? black : white) << 3;
    pixel=(unsigned char) (GetPixelAlpha(image,p) == OpaqueAlpha ?
      0x00 : 0x01);
    *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 2);
    p+=(ptrdiff_t) GetPixelChannels(image);
    *q|=(GetPixelLuma(image,p) > threshold ? black : white) << 1;
    pixel=(unsigned char) (GetPixelAlpha(image,p) == OpaqueAlpha ?
      0x00 : 0x01);
    *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q++;
  }
  if ((number_pixels % 4) != 0)
    {
      *q='\0';
      for (bit=0; bit <= (ssize_t) (number_pixels % 4); bit+=2)
      {
        *q|=(GetPixelLuma(image,p) > threshold ? black : white) <<
          (7-bit);
        pixel=(unsigned char) (GetPixelAlpha(image,p) == OpaqueAlpha ?
          0x00 : 0x01);
        *q|=(((int) pixel != 0 ? 0x00 : 0x01) << (unsigned char)
          (7-bit-1));
        p+=(ptrdiff_t) GetPixelChannels(image);
      }
      q++;
    }
}

static void Export4BitGrayAlphaQuantum(const Image *image,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels ; x++)
  {
    pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
    *q=(((pixel >> 4) & 0xf) << 4);
    pixel=(unsigned char) (16.0*QuantumScale*(double)
      GetPixelAlpha(image,p)+0.5);
    *q|=pixel & 0xf;
    p+=(ptrdiff_t) GetPixelChannels(image);
    q++;
  }
}

static void Export8BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelAlpha(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelLuma(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelAlpha(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(ClampToQuantum(GetPixelLuma(image,p)));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelAlpha(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        float
          float_pixel;

        float_pixel=(float) GetPixelLuma(image,p);
        q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
        float_pixel=(float) (GetPixelAlpha(image,p));
        q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(ClampToQuantum(GetPixelLuma(image,p)));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelAlpha(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    double
      pixel;

    pixel=GetPixelLuma(image,p);
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    pixel=(double) (GetPixelAlpha(image,p));
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthGrayAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(ClampToQuantum(
      GetPixelLuma(image,p)),range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelAlpha(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportGrayAlphaQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 1:
    {
      Export1BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 4:
    {
      Export4BitGrayAlphaQuantum(image,span);
      break;
    }
    case 8:
    {
      Export8BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatGrayAlphaQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthGrayAlphaQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export1BitIndexQuantum(const Image *image,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned char
    pixel;

  for (x=((ssize_t) number_pixels-7); x > 0; x-=8)
  {
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q=((pixel & 0x01) << 7);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 6);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 5);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 4);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 3);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 2);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 1);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q++;
  }
  if ((number_pixels % 8) != 0)
    {
      *q='\0';
      for (bit=7; bit >= (ssize_t) (8-(number_pixels % 8)); bit--)
      {
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << (unsigned char) bit);
        p+=(ptrdiff_t) GetPixelChannels(image);
      }
      q++;
    }
}

static void Export4BitIndexQuantum(const Image *image,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < ((ssize_t) number_pixels-1) ; x+=2)
  {
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q=((pixel & 0xf) << 4);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0xf) << 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q++;
  }
  if ((number_pixels % 2) != 0)
    {
      pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
      *q=((pixel & 0xf) << 4);
      p+=(ptrdiff_t) GetPixelChannels(image);
      q++;
    }
}

static void Export8BitIndexQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopCharPixel((unsigned char) ((ssize_t) GetPixelIndex(image,p)),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopShortPixel(quantum_info->endian,SinglePrecisionToHalf(
          QuantumScale*(double) GetPixelIndex(image,p)),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopShortPixel(quantum_info->endian,(unsigned short)
      GetPixelIndex(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelIndex(image,p),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopLongPixel(quantum_info->endian,(unsigned int)
      GetPixelIndex(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelIndex(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthIndexQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,(QuantumAny) GetPixelIndex(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportIndexQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  if (image->storage_class != PseudoClass)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColormappedImageRequired","`%s'",image->filename);
      return;
    }
  switch (quantum_info->depth)
  {
    case 1:
    {
      Export1BitIndexQuantum(image,span);
      break;
    }
    case 4:
    {
      Export4BitIndexQuantum(image,span);
      break;
    }
    case 8:
    {
      Export8BitIndexQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitIndexQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitIndexQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatIndexQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthIndexQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export1BitIndexAlphaQuantum(const Image *image,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned char
    pixel;

  for (x=((ssize_t) number_pixels-3); x > 0; x-=4)
  {
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q=((pixel & 0x01) << 7);
    pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
      TransparentAlpha ? 1 : 0);
    *q|=((pixel & 0x01) << 6);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 5);
    pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
      TransparentAlpha ? 1 : 0);
    *q|=((pixel & 0x01) << 4);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 3);
    pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
      TransparentAlpha ? 1 : 0);
    *q|=((pixel & 0x01) << 2);
    p+=(ptrdiff_t) GetPixelChannels(image);
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q|=((pixel & 0x01) << 1);
    pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
      TransparentAlpha ? 1 : 0);
    *q|=((pixel & 0x01) << 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q++;
  }
  if ((number_pixels % 4) != 0)
    {
      *q='\0';
      for (bit=3; bit >= (ssize_t) (4-(number_pixels % 4)); bit-=2)
      {
        pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
        *q|=((pixel & 0x01) << (unsigned char) (bit+4));
        pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)
          TransparentAlpha ? 1 : 0);
        *q|=((pixel & 0x01) << (unsigned char) (bit+4-1));
        p+=(ptrdiff_t) GetPixelChannels(image);
      }
      q++;
    }
}

static void Export4BitIndexAlphaQuantum(const Image *image,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels ; x++)
  {
    pixel=(unsigned char) ((ssize_t) GetPixelIndex(image,p));
    *q=((pixel & 0xf) << 4);
    pixel=(unsigned char) (16.0*QuantumScale*(double)
      GetPixelAlpha(image,p)+0.5);
    *q|=((pixel & 0xf) << 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q++;
  }
}

static void Export8BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopCharPixel((unsigned char) ((ssize_t) GetPixelIndex(image,p)),q);
    pixel=ScaleQuantumToChar(GetPixelAlpha(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopShortPixel(quantum_info->endian,(unsigned short)
          ((ssize_t) GetPixelIndex(image,p)),q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelAlpha(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopShortPixel(quantum_info->endian,(unsigned short)
      ((ssize_t) GetPixelIndex(image,p)),q);
    pixel=ScaleQuantumToShort(GetPixelAlpha(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        float
          float_pixel;

        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelIndex(image,p),q);
        float_pixel=(float) GetPixelAlpha(image,p);
        q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopLongPixel(quantum_info->endian,(unsigned int)
      GetPixelIndex(image,p),q);
    pixel=ScaleQuantumToLong(GetPixelAlpha(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    double
      pixel;

    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelIndex(image,p),q);
    pixel=(double) GetPixelAlpha(image,p);
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  QuantumAny
    range;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,(QuantumAny) GetPixelIndex(image,p),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelAlpha(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportIndexAlphaQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span,
  ExceptionInfo *exception)
{
  if (image->storage_class != PseudoClass)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColormappedImageRequired","`%s'",image->filename);
      return;
    }
  switch (quantum_info->depth)
  {
    case 1:
    {
      Export1BitIndexAlphaQuantum(image,span);
      break;
    }
    case 4:
    {
      Export4BitIndexAlphaQuantum(image,span);
      break;
    }
    case 8:
    {
      Export8BitIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatIndexAlphaQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthIndexAlphaQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
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
      pixel=ScaleQuantumToChar(p[i]);
      q=PopCharPixel(pixel,q);
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
        {
          pixel=SinglePrecisionToHalf(QuantumScale*(double) p[i]);
          q=PopShortPixel(quantum_info->endian,pixel,q);
        }
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      pixel=ScaleQuantumToShort(p[i]);
      q=PopShortPixel(quantum_info->endian,pixel,q);
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < (ssize_t) GetImageChannels(image); i++)
          q=PopQuantumFloatPixel(quantum_info,(float) p[i],q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
    {
      pixel=ScaleQuantumToLong(p[i]);
      q=PopLongPixel(quantum_info->endian,pixel,q);
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
      q=PopQuantumDoublePixel(quantum_info,(double) p[i],q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthMultispectralQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    i,
    x;

  QuantumAny
    range;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    for (i=0; i < (ssize_t) GetImageChannels(image); i++)
      q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(p[i],range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportMultispectralQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  if (image->number_meta_channels == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "MultispectralImageRequired","`%s'",image->filename);
      return;
    }
  if (quantum_info->meta_channel != 0)
    {
      ExportPixelChannel(image,quantum_info,number_pixels,p,q,
        (PixelChannel) (MetaPixelChannels+quantum_info->meta_channel-1));
      return;
    }
  switch (quantum_info->depth)
  {
    case 8:
    {
      Export8BitMultispectralQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitMultispectralQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitMultispectralQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatMultispectralQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthMultispectralQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(GetPixelOpacity(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelOpacity(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelOpacity(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelOpacity(image,p),q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelOpacity(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelOpacity(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthOpacityQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(
      GetPixelOpacity(image,p),range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportOpacityQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span,ExceptionInfo *exception)
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
      Export8BitOpacityQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitOpacityQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitOpacityQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatOpacityQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthOpacityQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopCharPixel(ScaleQuantumToChar(GetPixelRed(image,p)),q);
    q=PopCharPixel(ScaleQuantumToChar(GetPixelGreen(image,p)),q);
    q=PopCharPixel(ScaleQuantumToChar(GetPixelBlue(image,p)),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export10BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
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
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=(unsigned int) (
          ScaleQuantumToAny(GetPixelRed(image,p),range) << 22 |
          ScaleQuantumToAny(GetPixelGreen(image,p),range) << 12 |
          ScaleQuantumToAny(GetPixelBlue(image,p),range) << 2);
        q=PopLongPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  if (quantum_info->quantum == 32UL)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
          range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export12BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  ssize_t
    bit;

  unsigned int
    pixel;

  range=GetQuantumRange(quantum_info->depth);
  if (quantum_info->pack == MagickFalse)
    {
      for (x=0; x < (3*(ssize_t) number_pixels-1); x+=2)
      {
        switch (x % 3)
        {
          default:
          case 0:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),
              range);
            break;
          }
          case 1:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
              range);
            break;
          }
          case 2:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),
              range);
            p+=(ptrdiff_t) GetPixelChannels(image);
            break;
          }
        }
        q=PopShortPixel(quantum_info->endian,(unsigned short) (pixel << 4),
          q);
        switch ((x+1) % 3)
        {
          default:
          case 0:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),
              range);
            break;
          }
          case 1:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
              range);
            break;
          }
          case 2:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),
              range);
            p+=(ptrdiff_t) GetPixelChannels(image);
            break;
          }
        }
        q=PopShortPixel(quantum_info->endian,(unsigned short) (pixel << 4),
          q);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      for (bit=0; bit < (ssize_t) (3*number_pixels % 2); bit++)
      {
        switch ((x+bit) % 3)
        {
          default:
          case 0:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),
              range);
            break;
          }
          case 1:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
              range);
            break;
          }
          case 2:
          {
            pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),
              range);
            p+=(ptrdiff_t) GetPixelChannels(image);
            break;
          }
        }
        q=PopShortPixel(quantum_info->endian,(unsigned short) (pixel << 4),
          q);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      if (bit != 0)
        p+=(ptrdiff_t) GetPixelChannels(image);
      return;
    }
  if (quantum_info->quantum == 32UL)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
          range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelRed(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelGreen(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlue(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),
          q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelBlue(image,p),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthRGBQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportRGBQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 8:
    {
      Export8BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Export10BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 12:
    {
      Export12BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitRGBQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatRGBQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthRGBQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(GetPixelRed(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelGreen(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelBlue(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelAlpha(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export10BitRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
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
      ssize_t
        i;

      size_t
        quantum;

      ssize_t
        n;

      n=0;
      quantum=0;
      pixel=0;
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < 4; i++)
        {
          switch (i)
          {
            case 0: quantum=(size_t) GetPixelRed(image,p); break;
            case 1: quantum=(size_t) GetPixelGreen(image,p); break;
            case 2: quantum=(size_t) GetPixelBlue(image,p); break;
            case 3: quantum=(size_t) GetPixelAlpha(image,p); break;
          }
          switch (n % 3)
          {
            case 0:
            {
              pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
                range) << 22);
              break;
            }
            case 1:
            {
              pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
                range) << 12);
              break;
            }
            case 2:
            {
              pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
                range) << 2);
              q=PopLongPixel(quantum_info->endian,pixel,q);
              pixel=0;
              break;
            }
          }
          n++;
        }
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  if (quantum_info->quantum == 32UL)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
          range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelAlpha(image,p),
          range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelAlpha(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelRed(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelGreen(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlue(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelAlpha(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelAlpha(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        float
          float_pixel;

        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),q);
        float_pixel=(float) GetPixelAlpha(image,p);
        q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelAlpha(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double) GetPixelBlue(image,p),q);
    pixel=(double) GetPixelAlpha(image,p);
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthRGBAQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelAlpha(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportRGBAQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 8:
    {
      Export8BitRGBAQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Export10BitRGBAQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitRGBAQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitRGBAQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatRGBAQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthRGBAQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void Export8BitRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned char
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToChar(GetPixelRed(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelGreen(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelBlue(image,p));
    q=PopCharPixel(pixel,q);
    pixel=ScaleQuantumToChar(GetPixelOpacity(image,p));
    q=PopCharPixel(pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export10BitRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
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
      ssize_t
        i;

      size_t
        quantum;

      ssize_t
        n;

      n=0;
      quantum=0;
      pixel=0;
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        for (i=0; i < 4; i++)
        {
          switch (i)
          {
            case 0: quantum=(size_t) GetPixelRed(image,p); break;
            case 1: quantum=(size_t) GetPixelGreen(image,p); break;
            case 2: quantum=(size_t) GetPixelBlue(image,p); break;
            case 3: quantum=(size_t) GetPixelOpacity(image,p); break;
          }
          switch (n % 3)
          {
            case 0:
            {
              pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
                range) << 22);
              break;
            }
            case 1:
            {
              pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
                range) << 12);
              break;
            }
            case 2:
            {
              pixel|=(size_t) (ScaleQuantumToAny((Quantum) quantum,
                range) << 2);
              q=PopLongPixel(quantum_info->endian,pixel,q);
              pixel=0;
              break;
            }
          }
          n++;
        }
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  if (quantum_info->quantum == 32UL)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),
          range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        pixel=(unsigned int) ScaleQuantumToAny(GetPixelOpacity(image,p),
          range);
        q=PopQuantumLongPixel(quantum_info,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelRed(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelGreen(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelBlue(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    pixel=(unsigned int) ScaleQuantumToAny(GetPixelOpacity(image,p),range);
    q=PopQuantumPixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export16BitRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned short
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelRed(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelGreen(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelBlue(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        pixel=SinglePrecisionToHalf(QuantumScale*(double)
          GetPixelOpacity(image,p));
        q=PopShortPixel(quantum_info->endian,pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToShort(GetPixelRed(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelGreen(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelBlue(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToShort(GetPixelOpacity(image,p));
    q=PopShortPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export32BitRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  unsigned int
    pixel;

  if (quantum_info->format == FloatingPointQuantumFormat)
    {
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        float
          float_pixel;

        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelRed(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelGreen(image,p),
          q);
        q=PopQuantumFloatPixel(quantum_info,(float) GetPixelBlue(image,p),
          q);
        float_pixel=(float) GetPixelOpacity(image,p);
        q=PopQuantumFloatPixel(quantum_info,float_pixel,q);
        p+=(ptrdiff_t) GetPixelChannels(image);
        q+=(ptrdiff_t) quantum_info->pad;
      }
      return;
    }
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    pixel=ScaleQuantumToLong(GetPixelRed(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelGreen(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelBlue(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    pixel=ScaleQuantumToLong(GetPixelOpacity(image,p));
    q=PopLongPixel(quantum_info->endian,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void Export64BitFloatRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  ssize_t
    x;

  double
    pixel;

  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelRed(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelGreen(image,p),q);
    q=PopQuantumDoublePixel(quantum_info,(double)
      GetPixelBlue(image,p),q);
    pixel=(double) GetPixelOpacity(image,p);
    q=PopQuantumDoublePixel(quantum_info,pixel,q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportAnyDepthRGBOQuantum(const Image *image,
  QuantumInfo *quantum_info,const QuantumPixelSpan *span)
{
  const MagickSizeType
    number_pixels = span->number_pixels;

  const Quantum
    *magick_restrict p = span->p;

  unsigned char
    *magick_restrict q = span->q;

  QuantumAny
    range;

  ssize_t
    x;

  range=GetQuantumRange(quantum_info->depth);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelRed(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelGreen(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelBlue(image,p),
      range),q);
    q=PopQuantumPixel(quantum_info,ScaleQuantumToAny(GetPixelOpacity(image,p),
      range),q);
    p+=(ptrdiff_t) GetPixelChannels(image);
    q+=(ptrdiff_t) quantum_info->pad;
  }
}

static void ExportRGBOQuantum(const Image *image,QuantumInfo *quantum_info,
  const QuantumPixelSpan *span)
{
  switch (quantum_info->depth)
  {
    case 8:
    {
      Export8BitRGBOQuantum(image,quantum_info,span);
      break;
    }
    case 10:
    {
      Export10BitRGBOQuantum(image,quantum_info,span);
      break;
    }
    case 16:
    {
      Export16BitRGBOQuantum(image,quantum_info,span);
      break;
    }
    case 32:
    {
      Export32BitRGBOQuantum(image,quantum_info,span);
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          Export64BitFloatRGBOQuantum(image,quantum_info,span);
          break;
        }
      magick_fallthrough;
    }
    default:
    {
      ExportAnyDepthRGBOQuantum(image,quantum_info,span);
      break;
    }
  }
}

static void SwapRedGreenQuantum(const Image *image,CacheView *image_view,
  const MagickSizeType number_pixels)
{
  Quantum
    quantum;

  Quantum
    *magick_restrict r;

  ssize_t
    x;

  if (image_view == (CacheView *) NULL)
    r=GetAuthenticPixelQueue(image);
  else
    r=GetCacheViewAuthenticPixelQueue(image_view);
  for (x=0; x < (ssize_t) number_pixels; x++)
  {
    quantum=GetPixelRed(image,r);
    SetPixelRed(image,GetPixelGreen(image,r),r);
    SetPixelGreen(image,quantum,r);
    r+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void AssociateQuantumAlpha(const Image *image,CacheView *image_view)
{
  double
    Sa;

  Quantum
    *magick_restrict r;

  ssize_t
    x;

  /*
    Associate alpha.
  */
  if (image_view == (CacheView *) NULL)
    r=GetAuthenticPixelQueue(image);
  else
    r=GetCacheViewAuthenticPixelQueue(image_view);
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    ssize_t
      i;

    Sa=QuantumScale*(double) GetPixelAlpha(image,r);
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      PixelChannel channel = GetPixelChannelChannel(image,i);
      PixelTrait traits = GetPixelChannelTraits(image,channel);
      if ((traits & UpdatePixelTrait) == 0)
        continue;
      r[i]=ClampToQuantum(Sa*(double) r[i]);
    }
    r+=(ptrdiff_t) GetPixelChannels(image);
  }
}

MagickExport size_t ExportQuantumPixels(const Image *image,
  CacheView *image_view,QuantumInfo *quantum_info,
  const QuantumType quantum_type,unsigned char *magick_restrict pixels,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  MagickSizeType
    number_pixels;

  size_t
    extent;

  unsigned char
    *magick_restrict q;

  QuantumPixelSpan span;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(quantum_info != (QuantumInfo *) NULL);
  assert(quantum_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (pixels == (unsigned char *) NULL)
    pixels=(unsigned char *) GetQuantumPixels(quantum_info);
  if (image_view == (CacheView *) NULL)
    {
      number_pixels=GetImageExtent(image);
      p=GetVirtualPixelQueue(image);
    }
  else
    {
      number_pixels=GetCacheViewExtent(image_view);
      p=GetCacheViewVirtualPixelQueue(image_view);
    }
  if (p == (Quantum *) NULL)
    return(0);
  if (quantum_info->alpha_type == AssociatedQuantumAlpha)
    AssociateQuantumAlpha(image,image_view);
  if ((quantum_type == CbYCrQuantum) || (quantum_type == CbYCrAQuantum))
    SwapRedGreenQuantum(image,image_view,number_pixels);
  q=pixels;
  ResetQuantumState(quantum_info);
  extent=GetQuantumExtent(image,quantum_info,quantum_type);
  span.number_pixels=number_pixels;
  span.p=p;
  span.q=q;

  switch (quantum_type)
  {
    case AlphaQuantum:
    {
      ExportAlphaQuantum(image,quantum_info,&span,exception);
      break;
    }
    case BGRQuantum:
    {
      ExportBGRQuantum(image,quantum_info,&span);
      break;
    }
    case BGRAQuantum:
    {
      ExportBGRAQuantum(image,quantum_info,&span);
      break;
    }
    case BGROQuantum:
    {
      ExportBGROQuantum(image,quantum_info,&span);
      break;
    }
    case BlackQuantum:
    {
      ExportBlackQuantum(image,quantum_info,&span,exception);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      ExportPixelChannel(image,quantum_info,number_pixels,p,q,BluePixelChannel);
      break;
    }
    case CMYKQuantum:
    {
      ExportCMYKQuantum(image,quantum_info,&span,exception);
      break;
    }
    case CMYKAQuantum:
    {
      ExportCMYKAQuantum(image,quantum_info,&span,exception);
      break;
    }
    case MultispectralQuantum:
    {
      ExportMultispectralQuantum(image,quantum_info,&span,exception);
      break;
    }
    case CMYKOQuantum:
    {
      ExportCMYKOQuantum(image,quantum_info,&span,exception);
      break;
    }
    case CbYCrYQuantum:
    {
      ExportCbYCrYQuantum(image,quantum_info,&span);
      break;
    }
    case GrayQuantum:
    {
      ExportGrayQuantum(image,quantum_info,&span);
      break;
    }
    case GrayAlphaQuantum:
    {
      ExportGrayAlphaQuantum(image,quantum_info,&span);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      ExportPixelChannel(image,quantum_info,number_pixels,p,q,
        GreenPixelChannel);
      break;
    }
    case IndexQuantum:
    {
      ExportIndexQuantum(image,quantum_info,&span,exception);
      break;
    }
    case IndexAlphaQuantum:
    {
      ExportIndexAlphaQuantum(image,quantum_info,&span,exception);
      break;
    }
    case RedQuantum:
    case CyanQuantum:
    {
      ExportPixelChannel(image,quantum_info,number_pixels,p,q,RedPixelChannel);
      break;
    }
    case OpacityQuantum:
    {
      ExportOpacityQuantum(image,quantum_info,&span,exception);
      break;
    }
    case RGBQuantum:
    case CbYCrQuantum:
    {
      ExportRGBQuantum(image,quantum_info,&span);
      break;
    }
    case RGBAQuantum:
    case CbYCrAQuantum:
    {
      ExportRGBAQuantum(image,quantum_info,&span);
      break;
    }
    case RGBOQuantum:
    {
      ExportRGBOQuantum(image,quantum_info,&span);
      break;
    }
    default:
      break;
  }
  if ((quantum_type == CbYCrQuantum) || (quantum_type == CbYCrAQuantum))
    SwapRedGreenQuantum(image,image_view,number_pixels);
  return(extent);
}
