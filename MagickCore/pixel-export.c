/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      PPPP   IIIII  X   X  EEEEE  L                          %
%                      P   P    I     X X   E      L                          %
%                      PPPP     I      X    EEE    L                          %
%                      P        I     X X   E      L                          %
%                      P      IIIII  X   X  EEEEE  LLLLL                      %
%                                                                             %
%                  EEEEE  X   X  PPPP    OOO   RRRR   TTTTT                   %
%                  E       X X   P   P  O   O  R   R    T                     %
%                  EEE      X    PPPP   O   O  RRRR     T                     %
%                  E       X X   P      O   O  R R      T                     %
%                  EEEEE  X   X  P       OOO   R  R     T                     %
%                                                                             %
%                     MagickCore Methods to Export Pixels                     %
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
#include "MagickCore/cache-private.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/pixel-private.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/stream.h"
#include "MagickCore/string_.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"


/*
  Typedef declarations.
*/
typedef struct _ExportPixelBuffer
{
  const char
    *magick_restrict map;

  const QuantumType
    *quantum_map;

  void
    *pixels;
} ExportPixelBuffer;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p o r t I m a g e P i x e l s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExportImagePixels() extracts pixel data from an image and returns it to you.
%  The method returns MagickTrue on success otherwise MagickFalse if an error is
%  encountered.  The data is returned as char, short int, Quantum, unsigned int,
%  unsigned long long, float, or double in the order specified by map.
%
%  Suppose you want to extract the first scanline of a 640x480 image as
%  character data in red-green-blue order:
%
%      ExportImagePixels(image,0,0,640,1,"RGB",CharPixel,pixels,exception);
%
%  The format of the ExportImagePixels method is:
%
%      MagickBooleanType ExportImagePixels(const Image *image,const ssize_t x,
%        const ssize_t y,const size_t width,const size_t height,
%        const char *map,const StorageType type,void *pixels,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y,width,height:  These values define the perimeter
%      of a region of pixels you want to extract.
%
%    o map:  This string reflects the expected ordering of the pixel array.
%      It can be any combination or order of R = red, G = green, B = blue,
%      A = alpha (0 is transparent), O = opacity (0 is opaque), C = cyan,
%      Y = yellow, M = magenta, K = black, I = intensity (for grayscale),
%      P = pad.
%
%    o type: Define the data type of the pixels.  Float and double types are
%      normalized to [0..1] otherwise [0..QuantumRange].  Choose from these
%      types: CharPixel (char *), DoublePixel (double *), FloatPixel (float *),
%      LongPixel (unsigned int *), LongLongPixel (unsigned long long *),
%      QuantumPixel (Quantum *), or ShortPixel (unsigned short *).
%
%    o pixels: This array of values contain the pixel components as defined by
%      map and type.  You must preallocate this array where the expected
%      length varies depending on the values of width, height, map, and type.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _ExportRowInfo
{
  const Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ExportRowInfo;

typedef void
  *(*ExportRowMethod)(const ExportRowInfo *,const Quantum *magick_restrict,
    void *);

static MagickBooleanType ExportPixelRows(const ExportRowInfo *row,
  const ExportRowMethod method,void *pixels,ExceptionInfo *exception)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const Quantum
    *magick_restrict p;

  ssize_t
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    pixels=method(row,p,pixels);
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static void *ExportCharBGRPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned char
    *magick_restrict q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportCharBGRAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned char
    *magick_restrict q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportCharBGRPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned char
    *magick_restrict q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar((Quantum) 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportCharIPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned char
    *magick_restrict q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportCharRGBPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned char
    *magick_restrict q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportCharRGBAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned char
    *magick_restrict q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportCharRGBPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned char
    *magick_restrict q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar((Quantum) 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void ExportCharAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *magick_restrict p,
  unsigned char *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelAlpha(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToChar(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void ExportCharQuantum(const Image *image,const QuantumType quantum,
  const Quantum *magick_restrict p,unsigned char *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToChar(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      ExportCharAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void *ExportCharMapRow(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  unsigned char
    *magick_restrict q = (unsigned char *) pixels;

  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      ExportCharQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static MagickBooleanType ExportCharPixel(const Image *image,
  const RectangleInfo *roi,const ExportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  void
    *pixels = buffer->pixels;

  ExportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportPixelRows(&row,ExportCharBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportPixelRows(&row,ExportCharBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportPixelRows(&row,ExportCharBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportPixelRows(&row,ExportCharIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportPixelRows(&row,ExportCharRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportPixelRows(&row,ExportCharRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportPixelRows(&row,ExportCharRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ExportPixelRows(&row,ExportCharMapRow,pixels,exception));
}

static void *ExportDoubleBGRPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  double
    *magick_restrict q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=QuantumScale*(double) GetPixelBlue(image,p);
    *q++=QuantumScale*(double) GetPixelGreen(image,p);
    *q++=QuantumScale*(double) GetPixelRed(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportDoubleBGRAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  double
    *magick_restrict q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=QuantumScale*(double) GetPixelBlue(image,p);
    *q++=QuantumScale*(double) GetPixelGreen(image,p);
    *q++=QuantumScale*(double) GetPixelRed(image,p);
    *q++=QuantumScale*(double) GetPixelAlpha(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportDoubleBGRPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  double
    *magick_restrict q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=QuantumScale*(double) GetPixelBlue(image,p);
    *q++=QuantumScale*(double) GetPixelGreen(image,p);
    *q++=QuantumScale*(double) GetPixelRed(image,p);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportDoubleIPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  double
    *magick_restrict q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=(double) (QuantumScale*(double) GetPixelIntensity(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportDoubleRGBPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  double
    *magick_restrict q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=QuantumScale*(double) GetPixelRed(image,p);
    *q++=QuantumScale*(double) GetPixelGreen(image,p);
    *q++=QuantumScale*(double) GetPixelBlue(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportDoubleRGBAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  double
    *magick_restrict q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=QuantumScale*(double) GetPixelRed(image,p);
    *q++=QuantumScale*(double) GetPixelGreen(image,p);
    *q++=QuantumScale*(double) GetPixelBlue(image,p);
    *q++=QuantumScale*(double) GetPixelAlpha(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportDoubleRGBPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  double
    *magick_restrict q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=QuantumScale*(double) GetPixelRed(image,p);
    *q++=QuantumScale*(double) GetPixelGreen(image,p);
    *q++=QuantumScale*(double) GetPixelBlue(image,p);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void ExportDoubleAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *magick_restrict p,
  double *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=QuantumScale*(double) GetPixelAlpha(image,p);
      break;
    }
    case OpacityQuantum:
    {
      *q=QuantumScale*(double) GetPixelAlpha(image,p);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=QuantumScale*(double) GetPixelBlack(image,p);
      break;
    }
    case IndexQuantum:
    {
      *q=QuantumScale*(double) GetPixelIntensity(image,p);
      break;
    }
    default:
      *q=0;
  }
}

static void ExportDoubleQuantum(const Image *image,const QuantumType quantum,
  const Quantum *magick_restrict p,double *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=QuantumScale*(double) GetPixelRed(image,p);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=QuantumScale*(double) GetPixelGreen(image,p);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=QuantumScale*(double) GetPixelBlue(image,p);
      break;
    }
    default:
    {
      ExportDoubleAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void *ExportDoubleMapRow(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  double
    *magick_restrict q = (double *) pixels;

  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      ExportDoubleQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static MagickBooleanType ExportDoublePixel(const Image *image,
  const RectangleInfo *roi,const ExportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  void
    *pixels = buffer->pixels;

  ExportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportPixelRows(&row,ExportDoubleBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportPixelRows(&row,ExportDoubleBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportPixelRows(&row,ExportDoubleBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportPixelRows(&row,ExportDoubleIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportPixelRows(&row,ExportDoubleRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportPixelRows(&row,ExportDoubleRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportPixelRows(&row,ExportDoubleRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ExportPixelRows(&row,ExportDoubleMapRow,pixels,exception));
}

static void *ExportFloatBGRPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  float
    *magick_restrict q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportFloatBGRAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  float
    *magick_restrict q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportFloatBGRPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  float
    *magick_restrict q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportFloatIPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  float
    *magick_restrict q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=(float) (QuantumScale*(double) GetPixelIntensity(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportFloatRGBPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  float
    *magick_restrict q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportFloatRGBAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  float
    *magick_restrict q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportFloatRGBPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  float
    *magick_restrict q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
    *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void ExportFloatAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *magick_restrict p,
  float *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=(float) (QuantumScale*((double) (GetPixelAlpha(image,p))));
      break;
    }
    case OpacityQuantum:
    {
      *q=(float) (QuantumScale*(double) GetPixelAlpha(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=(float) (QuantumScale*(double) GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=(float) (QuantumScale*(double) GetPixelIntensity(image,p));
      break;
    }
    default:
      *q=0;
  }
}

static void ExportFloatQuantum(const Image *image,const QuantumType quantum,
  const Quantum *magick_restrict p,float *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=(float) (QuantumScale*(double) GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=(float) (QuantumScale*(double) GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=(float) (QuantumScale*(double) GetPixelBlue(image,p));
      break;
    }
    default:
    {
      ExportFloatAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void *ExportFloatMapRow(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  float
    *magick_restrict q = (float *) pixels;

  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      ExportFloatQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static MagickBooleanType ExportFloatPixel(const Image *image,
  const RectangleInfo *roi,const ExportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  void
    *pixels = buffer->pixels;

  ExportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportPixelRows(&row,ExportFloatBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportPixelRows(&row,ExportFloatBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportPixelRows(&row,ExportFloatBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportPixelRows(&row,ExportFloatIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportPixelRows(&row,ExportFloatRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportPixelRows(&row,ExportFloatRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportPixelRows(&row,ExportFloatRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ExportPixelRows(&row,ExportFloatMapRow,pixels,exception));
}

static void *ExportLongBGRPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned int
    *magick_restrict q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongBGRAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned int
    *magick_restrict q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongBGRPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned int
    *magick_restrict q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongIPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned int
    *magick_restrict q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLong(ClampToQuantum(GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongRGBPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned int
    *magick_restrict q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongRGBAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned int
    *magick_restrict q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongRGBPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned int
    *magick_restrict q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void ExportLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *magick_restrict p,
  unsigned int *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelAlpha(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToLong(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToLong(ClampToQuantum(GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void ExportLongQuantum(const Image *image,const QuantumType quantum,
  const Quantum *magick_restrict p,unsigned int *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToLong(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      ExportLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void *ExportLongMapRow(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  unsigned int
    *magick_restrict q = (unsigned int *) pixels;

  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      ExportLongQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static MagickBooleanType ExportLongPixel(const Image *image,
  const RectangleInfo *roi,const ExportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  void
    *pixels = buffer->pixels;

  ExportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportPixelRows(&row,ExportLongBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportPixelRows(&row,ExportLongBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportPixelRows(&row,ExportLongBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportPixelRows(&row,ExportLongIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportPixelRows(&row,ExportLongRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportPixelRows(&row,ExportLongRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportPixelRows(&row,ExportLongRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ExportPixelRows(&row,ExportLongMapRow,pixels,exception));
}

static void *ExportLongLongBGRPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  MagickSizeType
    *magick_restrict q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongLongBGRAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  MagickSizeType
    *magick_restrict q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongLongBGRPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  MagickSizeType
    *magick_restrict q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongLongIPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  MagickSizeType
    *magick_restrict q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLongLong(ClampToQuantum(
      GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongLongRGBPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  MagickSizeType
    *magick_restrict q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongLongRGBAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  MagickSizeType
    *magick_restrict q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportLongLongRGBPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  MagickSizeType
    *magick_restrict q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void ExportLongLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *magick_restrict p,
  MagickSizeType *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToLongLong(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToLongLong(ClampToQuantum(
        GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void ExportLongLongQuantum(const Image *image,const QuantumType quantum,
  const Quantum *magick_restrict p,MagickSizeType *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      ExportLongLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void *ExportLongLongMapRow(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  MagickSizeType
    *magick_restrict q = (MagickSizeType *) pixels;

  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      ExportLongLongQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static MagickBooleanType ExportLongLongPixel(const Image *image,
  const RectangleInfo *roi,const ExportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  void
    *pixels = buffer->pixels;

  ExportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportPixelRows(&row,ExportLongLongBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportPixelRows(&row,ExportLongLongBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportPixelRows(&row,ExportLongLongBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportPixelRows(&row,ExportLongLongIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportPixelRows(&row,ExportLongLongRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportPixelRows(&row,ExportLongLongRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportPixelRows(&row,ExportLongLongRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ExportPixelRows(&row,ExportLongLongMapRow,pixels,exception));
}

static void *ExportQuantumBGRPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  Quantum
    *magick_restrict q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportQuantumBGRAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  Quantum
    *magick_restrict q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    *q++=(Quantum) (GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportQuantumBGRPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  Quantum
    *magick_restrict q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    *q++=(Quantum) 0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportQuantumIPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  Quantum
    *magick_restrict q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ClampToQuantum(GetPixelIntensity(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportQuantumRGBPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  Quantum
    *magick_restrict q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportQuantumRGBAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  Quantum
    *magick_restrict q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    *q++=(Quantum) (GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportQuantumRGBPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  Quantum
    *magick_restrict q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    *q++=(Quantum) 0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void ExportQuantumAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=GetPixelAlpha(image,p);
      break;
    }
    case OpacityQuantum:
    {
      *q=GetPixelAlpha(image,p);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=GetPixelBlack(image,p);
      break;
    }
    case IndexQuantum:
    {
      *q=ClampToQuantum(GetPixelIntensity(image,p));
      break;
    }
    default:
    {
      *q=(Quantum) 0;
      break;
    }
  }
}

static void ExportQuantumQuantum(const Image *image,const QuantumType quantum,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=GetPixelRed(image,p);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=GetPixelGreen(image,p);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=GetPixelBlue(image,p);
      break;
    }
    default:
    {
      ExportQuantumAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void *ExportQuantumMapRow(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  Quantum
    *magick_restrict q = (Quantum *) pixels;

  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) length; i++)
    {
      *q=(Quantum) 0;
      ExportQuantumQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static MagickBooleanType ExportQuantumPixel(const Image *image,
  const RectangleInfo *roi,const ExportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  void
    *pixels = buffer->pixels;

  ExportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportPixelRows(&row,ExportQuantumBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportPixelRows(&row,ExportQuantumBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportPixelRows(&row,ExportQuantumBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportPixelRows(&row,ExportQuantumIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportPixelRows(&row,ExportQuantumRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportPixelRows(&row,ExportQuantumRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportPixelRows(&row,ExportQuantumRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ExportPixelRows(&row,ExportQuantumMapRow,pixels,exception));
}

static void *ExportShortBGRPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned short
    *magick_restrict q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportShortBGRAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned short
    *magick_restrict q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportShortBGRPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned short
    *magick_restrict q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportShortIPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned short
    *magick_restrict q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToShort(ClampToQuantum(GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportShortRGBPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned short
    *magick_restrict q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportShortRGBAPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned short
    *magick_restrict q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void *ExportShortRGBPPixels(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  unsigned short
    *magick_restrict q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static void ExportShortAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *magick_restrict p,
  unsigned short *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelAlpha(image,p));
      break;
    }
    case OpacityQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelAlpha(image,p));
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=ScaleQuantumToShort(GetPixelBlack(image,p));
      break;
    }
    case IndexQuantum:
    {
      *q=ScaleQuantumToShort(ClampToQuantum(GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void ExportShortQuantum(const Image *image,const QuantumType quantum,
  const Quantum *magick_restrict p,unsigned short *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelRed(image,p));
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelGreen(image,p));
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=ScaleQuantumToShort(GetPixelBlue(image,p));
      break;
    }
    default:
    {
      ExportShortAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void *ExportShortMapRow(const ExportRowInfo *row,
  const Quantum *magick_restrict p,void *pixels)
{
  unsigned short
    *magick_restrict q = (unsigned short *) pixels;

  const Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    ssize_t
      i;

    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      ExportShortQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(q);
}

static MagickBooleanType ExportShortPixel(const Image *image,
  const RectangleInfo *roi,const ExportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  void
    *pixels = buffer->pixels;

  ExportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportPixelRows(&row,ExportShortBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportPixelRows(&row,ExportShortBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportPixelRows(&row,ExportShortBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportPixelRows(&row,ExportShortIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportPixelRows(&row,ExportShortRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportPixelRows(&row,ExportShortRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportPixelRows(&row,ExportShortRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ExportPixelRows(&row,ExportShortMapRow,pixels,exception));
}

static MagickBooleanType ExportMapQuantumOPR(const char *map,const ssize_t i,
  QuantumType *quantum_map,ExceptionInfo *exception)
{
  switch (map[i])
  {
    case 'o':
    case 'O':
    {
      quantum_map[i]=OpacityQuantum;
      break;
    }
    case 'P':
    case 'p':
    {
      quantum_map[i]=UndefinedQuantum;
      break;
    }
    case 'R':
    case 'r':
    {
      quantum_map[i]=RedQuantum;
      break;
    }
    default:
    {
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnrecognizedPixelMap","`%s'",map);
      return(MagickFalse);
    }
  }
  return(MagickTrue);
}

static MagickBooleanType ExportMapQuantumGI(const char *map,const ssize_t i,
  QuantumType *quantum_map,ExceptionInfo *exception)
{
  switch (map[i])
  {
    case 'g':
    case 'G':
    {
      quantum_map[i]=GreenQuantum;
      break;
    }
    case 'I':
    case 'i':
    {
      quantum_map[i]=IndexQuantum;
      break;
    }
    default:
    {
      if (ExportMapQuantumOPR(map,i,quantum_map,exception) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType ExportMapQuantumAB(const char *map,const ssize_t i,
  QuantumType *quantum_map,ExceptionInfo *exception)
{
  switch (map[i])
  {
    case 'A':
    case 'a':
    {
      quantum_map[i]=AlphaQuantum;
      break;
    }
    case 'B':
    case 'b':
    {
      quantum_map[i]=BlueQuantum;
      break;
    }
    default:
    {
      if (ExportMapQuantumGI(map,i,quantum_map,exception) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _ExportMapRequest
{
  const Image
    *image;

  const char
    *map;

  ssize_t
    i;

  ExceptionInfo
    *exception;
} ExportMapRequest;

static MagickBooleanType ExportMapQuantumY(const ExportMapRequest *request,
  QuantumType *quantum_map)
{
  const Image
    *image = request->image;

  const char
    *map = request->map;

  const ssize_t
    i = request->i;

  ExceptionInfo
    *exception = request->exception;

  switch (map[i])
  {
    case 'Y':
    case 'y':
    {
      quantum_map[i]=YellowQuantum;
      if (image->colorspace == CMYKColorspace)
        break;
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",map);
      return(MagickFalse);
    }
    default:
    {
      if (ExportMapQuantumAB(map,i,quantum_map,exception) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType ExportMapQuantumM(const ExportMapRequest *request,
  QuantumType *quantum_map)
{
  const Image
    *image = request->image;

  const char
    *map = request->map;

  const ssize_t
    i = request->i;

  ExceptionInfo
    *exception = request->exception;

  switch (map[i])
  {
    case 'M':
    case 'm':
    {
      quantum_map[i]=MagentaQuantum;
      if (image->colorspace == CMYKColorspace)
        break;
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",map);
      return(MagickFalse);
    }
    default:
    {
      if (ExportMapQuantumY(request,quantum_map) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType ExportMapQuantumK(const ExportMapRequest *request,
  QuantumType *quantum_map)
{
  const Image
    *image = request->image;

  const char
    *map = request->map;

  const ssize_t
    i = request->i;

  ExceptionInfo
    *exception = request->exception;

  switch (map[i])
  {
    case 'K':
    case 'k':
    {
      quantum_map[i]=BlackQuantum;
      if (image->colorspace == CMYKColorspace)
        break;
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",map);
      return(MagickFalse);
    }
    default:
    {
      if (ExportMapQuantumM(request,quantum_map) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType ExportMapQuantumC(const ExportMapRequest *request,
  QuantumType *quantum_map)
{
  const Image
    *image = request->image;

  const char
    *map = request->map;

  const ssize_t
    i = request->i;

  ExceptionInfo
    *exception = request->exception;

  switch (map[i])
  {
    case 'C':
    case 'c':
    {
      quantum_map[i]=CyanQuantum;
      if (image->colorspace == CMYKColorspace)
        break;
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ColorSeparatedImageRequired","`%s'",map);
      return(MagickFalse);
    }
    default:
    {
      if (ExportMapQuantumK(request,quantum_map) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _ExportTypeRequest
{
  const Image
    *image;

  RectangleInfo
    roi;

  ExportPixelBuffer
    buffer;

  StorageType
    type;

  const char
    *map;

  ExceptionInfo
    *exception;
} ExportTypeRequest;

static void ExportPixelsOfType(const ExportTypeRequest *request,
  MagickBooleanType *status)
{
  const Image
    *image = request->image;

  const RectangleInfo
    roi = request->roi;

  const ExportPixelBuffer
    buffer = request->buffer;

  const StorageType
    type = request->type;

  const char
    *map = request->map;

  ExceptionInfo
    *exception = request->exception;

  switch (type)
  {
    case DoublePixel:
    {
      (*status)=ExportDoublePixel(image,&roi,&buffer,exception);
      break;
    }
    case FloatPixel:
    {
      (*status)=ExportFloatPixel(image,&roi,&buffer,exception);
      break;
    }
    case LongPixel:
    {
      (*status)=ExportLongPixel(image,&roi,&buffer,exception);
      break;
    }
    case LongLongPixel:
    {
      (*status)=ExportLongLongPixel(image,&roi,&buffer,exception);
      break;
    }
    case QuantumPixel:
    {
      (*status)=ExportQuantumPixel(image,&roi,&buffer,exception);
      break;
    }
    case ShortPixel:
    {
      (*status)=ExportShortPixel(image,&roi,&buffer,exception);
      break;
    }
    default:
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnrecognizedPixelMap","`%s'",map);
      (*status)=MagickFalse;
    }
  }
}

typedef struct _ExportMapParse
{
  const Image
    *image;

  const char
    *map;

  size_t
    length;

  QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ExportMapParse;

static MagickBooleanType ExportMapQuantums(const ExportMapParse *parse)
{
  const Image
    *image = parse->image;

  const char
    *map = parse->map;

  const size_t
    length = parse->length;

  QuantumType
    *quantum_map = parse->quantum_map;

  ExceptionInfo
    *exception = parse->exception;

  ssize_t
    i;

  for (i=0; i < (ssize_t) length; i++)
  {
    ExportMapRequest
      request;

    request.image=image;
    request.map=map;
    request.i=i;
    request.exception=exception;
    if (ExportMapQuantumC(&request,quantum_map) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

MagickExport MagickBooleanType ExportImagePixels(const Image *image,
  const ssize_t x,const ssize_t y,const size_t width,const size_t height,
  const char *map,const StorageType type,void *pixels,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  QuantumType
    *quantum_map;

  RectangleInfo
    roi;

  ExportMapParse
    parse;

  size_t
    length;

  ExportPixelBuffer buffer;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  length=strlen(map);
  quantum_map=(QuantumType *) AcquireQuantumMemory(length,sizeof(*quantum_map));
  if (quantum_map == (QuantumType *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  parse.image=image;
  parse.map=map;
  parse.length=length;
  parse.quantum_map=quantum_map;
  parse.exception=exception;
  if (ExportMapQuantums(&parse) == MagickFalse)
    return(MagickFalse);
  roi.width=width;
  roi.height=height;
  roi.x=x;
  roi.y=y;
  buffer.map=map;
  buffer.quantum_map=quantum_map;
  buffer.pixels=pixels;
  switch (type)
  {
    case CharPixel:
    {
      status=ExportCharPixel(image,&roi,&buffer,exception);
      break;
    }
    default:
    {
      ExportTypeRequest
        request;

      request.image=image;
      request.roi=roi;
      request.buffer=buffer;
      request.type=type;
      request.map=map;
      request.exception=exception;
      ExportPixelsOfType(&request,&status);
      break;
    }
  }
  quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
  return(status);
}
