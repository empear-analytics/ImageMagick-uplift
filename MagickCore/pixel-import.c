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
%                  IIIII  M   M  PPPP    OOO   RRRR   TTTTT                   %
%                    I    MM MM  P   P  O   O  R   R    T                     %
%                    I    M M M  PPPP   O   O  RRRR     T                     %
%                    I    M   M  P      O   O  R R      T                     %
%                  IIIII  M   M  P       OOO   R  R     T                     %
%                                                                             %
%                     MagickCore Methods to Import Pixels                     %
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
typedef struct _ImportPixelBuffer
{
  const char
    *magick_restrict map;

  const QuantumType
    *quantum_map;

  const void
    *pixels;
} ImportPixelBuffer;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m p o r t I m a g e P i x e l s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImportImagePixels() accepts pixel data and stores in the image at the
%  location you specify.  The method returns MagickTrue on success otherwise
%  MagickFalse if an error is encountered.  The pixel data can be either char,
%  Quantum, short int, unsigned int, unsigned long long, float, or double in
%  the order specified by map.
%
%  Suppose your want to upload the first scanline of a 640x480 image from
%  character data in red-green-blue order:
%
%      ImportImagePixels(image,0,0,640,1,"RGB",CharPixel,pixels);
%
%  The format of the ImportImagePixels method is:
%
%      MagickBooleanType ImportImagePixels(Image *image,const ssize_t x,
%        const ssize_t y,const size_t width,const size_t height,
%        const char *map,const StorageType type,const void *pixels,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o x,y,width,height:  These values define the perimeter
%      of a region of pixels you want to define.
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

typedef struct _ImportRowInfo
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ImportRowInfo;

typedef const void
  *(*ImportRowMethod)(const ImportRowInfo *,Quantum *magick_restrict,
    const void *);

static MagickBooleanType ImportPixelRows(const ImportRowInfo *row,
  const ImportRowMethod method,const void *pixels,ExceptionInfo *exception)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  Quantum
    *magick_restrict q;

  ssize_t
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    pixels=method(row,q,pixels);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static const void *ImportCharBGRPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
    SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
    SetPixelRed(image,ScaleCharToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportCharBGRAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
    SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
    SetPixelRed(image,ScaleCharToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportCharBGROPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
    SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
    SetPixelRed(image,ScaleCharToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportCharBGRPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
    SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
    SetPixelRed(image,ScaleCharToQuantum(*p++),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportCharIPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelGray(image,ScaleCharToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportCharRGBPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleCharToQuantum(*p++),q);
    SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
    SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportCharRGBAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleCharToQuantum(*p++),q);
    SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
    SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportCharRGBOPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleCharToQuantum(*p++),q);
    SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
    SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportCharRGBPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleCharToQuantum(*p++),q);
    SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
    SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static void ImportCharAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      SetPixelAlpha(image,ScaleCharToQuantum(*p),q);
      break;
    }
    case OpacityQuantum:
    {
      SetPixelAlpha(image,ScaleCharToQuantum(*p),q);
      break;
    }
    case BlackQuantum:
    {
      SetPixelBlack(image,ScaleCharToQuantum(*p),q);
      break;
    }
    case IndexQuantum:
    {
      SetPixelGray(image,ScaleCharToQuantum(*p),q);
      break;
    }
    default:
      break;
  }
}

static void ImportCharQuantum(const Image *image,const QuantumType quantum,
  const unsigned char *magick_restrict p,Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      SetPixelRed(image,ScaleCharToQuantum(*p),q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      SetPixelGreen(image,ScaleCharToQuantum(*p),q);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      SetPixelBlue(image,ScaleCharToQuantum(*p),q);
      break;
    }
    default:
    {
      ImportCharAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static const void *ImportCharMapRow(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  const unsigned char
    *magick_restrict p = (const unsigned char *) pixels;

  Image
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
      ImportCharQuantum(image,quantum_map[i],p,q);
      p++;
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

typedef struct _ImportCharMapInfo
{
  const char
    *map;

  ImportRowMethod
    import_pixels;
} ImportCharMapInfo;

static const ImportCharMapInfo
  ImportCharMaps[] =
  {
    { "BGR", ImportCharBGRPixels },
    { "BGRA", ImportCharBGRAPixels },
    { "BGRO", ImportCharBGROPixels },
    { "BGRP", ImportCharBGRPPixels },
    { "I", ImportCharIPixels },
    { "RGB", ImportCharRGBPixels },
    { "RGBA", ImportCharRGBAPixels },
    { "RGBO", ImportCharRGBOPixels },
    { "RGBP", ImportCharRGBPPixels }
  };

static MagickBooleanType ImportCharPixel(Image *image,
  const RectangleInfo *roi,const ImportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  const void
    *pixels = buffer->pixels;

  ImportRowInfo
    row;

  size_t
    i;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  for (i=0; i < (sizeof(ImportCharMaps)/sizeof(*ImportCharMaps)); i++)
    if (LocaleCompare(map,ImportCharMaps[i].map) == 0)
      return(ImportPixelRows(&row,ImportCharMaps[i].import_pixels,pixels,
        exception));
  row.length=strlen(map);
  return(ImportPixelRows(&row,ImportCharMapRow,pixels,exception));
}

static const void *ImportDoubleBGRPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const double
    *magick_restrict p = (const double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportDoubleBGRAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const double
    *magick_restrict p = (const double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportDoubleBGRPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const double
    *magick_restrict p = (const double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportDoubleIPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const double
    *magick_restrict p = (const double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelGray(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportDoubleRGBPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const double
    *magick_restrict p = (const double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportDoubleRGBAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const double
    *magick_restrict p = (const double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportDoubleRGBPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const double
    *magick_restrict p = (const double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static void ImportDoubleAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const double *magick_restrict p,
  Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(*p)),q);
      break;
    }
    case OpacityQuantum:
    {
      SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(*p)),q);
      break;
    }
    case BlackQuantum:
    {
      SetPixelBlack(image,ClampToQuantum((double) QuantumRange*(*p)),q);
      break;
    }
    case IndexQuantum:
    {
      SetPixelGray(image,ClampToQuantum((double) QuantumRange*(*p)),q);
      break;
    }
    default:
      break;
  }
}

static void ImportDoubleQuantum(const Image *image,const QuantumType quantum,
  const double *magick_restrict p,Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      SetPixelRed(image,ClampToQuantum((double) QuantumRange*(*p)),q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(*p)),q);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(*p)),q);
      break;
    }
    default:
    {
      ImportDoubleAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static const void *ImportDoubleMapRow(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  const double
    *magick_restrict p = (const double *) pixels;

  Image
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
      ImportDoubleQuantum(image,quantum_map[i],p,q);
      p++;
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static MagickBooleanType ImportDoublePixel(Image *image,
  const RectangleInfo *roi,const ImportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  const void
    *pixels = buffer->pixels;

  ImportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportPixelRows(&row,ImportDoubleBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportPixelRows(&row,ImportDoubleBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportPixelRows(&row,ImportDoubleBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportPixelRows(&row,ImportDoubleIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportPixelRows(&row,ImportDoubleRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportPixelRows(&row,ImportDoubleRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportPixelRows(&row,ImportDoubleRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ImportPixelRows(&row,ImportDoubleMapRow,pixels,exception));
}

static const void *ImportFloatBGRPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const float
    *magick_restrict p = (const float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportFloatBGRAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const float
    *magick_restrict p = (const float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportFloatBGRPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const float
    *magick_restrict p = (const float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportFloatIPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const float
    *magick_restrict p = (const float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelGray(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportFloatRGBPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const float
    *magick_restrict p = (const float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportFloatRGBAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const float
    *magick_restrict p = (const float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportFloatRGBPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const float
    *magick_restrict p = (const float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static void ImportFloatAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const float *magick_restrict p,
  Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
      break;
    }
    case OpacityQuantum:
    {
      SetPixelAlpha(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
      break;
    }
    case BlackQuantum:
    {
      SetPixelBlack(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
      break;
    }
    case IndexQuantum:
    {
      SetPixelGray(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
      break;
    }
    default:
      break;
  }
}

static void ImportFloatQuantum(const Image *image,const QuantumType quantum,
  const float *magick_restrict p,Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      SetPixelRed(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      SetPixelGreen(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      SetPixelBlue(image,ClampToQuantum((double) QuantumRange*(double)
      (*p)),q);
      break;
    }
    default:
    {
      ImportFloatAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static const void *ImportFloatMapRow(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  const float
    *magick_restrict p = (const float *) pixels;

  Image
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
      ImportFloatQuantum(image,quantum_map[i],p,q);
      p++;
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static MagickBooleanType ImportFloatPixel(Image *image,
  const RectangleInfo *roi,const ImportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  const void
    *pixels = buffer->pixels;

  ImportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportPixelRows(&row,ImportFloatBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportPixelRows(&row,ImportFloatBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportPixelRows(&row,ImportFloatBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportPixelRows(&row,ImportFloatIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportPixelRows(&row,ImportFloatRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportPixelRows(&row,ImportFloatRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportPixelRows(&row,ImportFloatRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ImportPixelRows(&row,ImportFloatMapRow,pixels,exception));
}

static const void *ImportLongBGRPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned int
    *magick_restrict p = (const unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
    SetPixelRed(image,ScaleLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongBGRAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned int
    *magick_restrict p = (const unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
    SetPixelRed(image,ScaleLongToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongBGRPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned int
    *magick_restrict p = (const unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
    SetPixelRed(image,ScaleLongToQuantum(*p++),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongIPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned int
    *magick_restrict p = (const unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelGray(image,ScaleLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongRGBPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned int
    *magick_restrict p = (const unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
    SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongRGBAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned int
    *magick_restrict p = (const unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
    SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongRGBPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned int
    *magick_restrict p = (const unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
    SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static void ImportLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const unsigned int *magick_restrict p,
  Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      SetPixelAlpha(image,ScaleLongToQuantum(*p),q);
      break;
    }
    case OpacityQuantum:
    {
      SetPixelAlpha(image,ScaleLongToQuantum(*p),q);
      break;
    }
    case BlackQuantum:
    {
      SetPixelBlack(image,ScaleLongToQuantum(*p),q);
      break;
    }
    case IndexQuantum:
    {
      SetPixelGray(image,ScaleLongToQuantum(*p),q);
      break;
    }
    default:
      break;
  }
}

static void ImportLongQuantum(const Image *image,const QuantumType quantum,
  const unsigned int *magick_restrict p,Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      SetPixelRed(image,ScaleLongToQuantum(*p),q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      SetPixelGreen(image,ScaleLongToQuantum(*p),q);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      SetPixelBlue(image,ScaleLongToQuantum(*p),q);
      break;
    }
    default:
    {
      ImportLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static const void *ImportLongMapRow(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  const unsigned int
    *magick_restrict p = (const unsigned int *) pixels;

  Image
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
      ImportLongQuantum(image,quantum_map[i],p,q);
      p++;
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static MagickBooleanType ImportLongPixel(Image *image,
  const RectangleInfo *roi,const ImportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  const void
    *pixels = buffer->pixels;

  ImportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportPixelRows(&row,ImportLongBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportPixelRows(&row,ImportLongBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportPixelRows(&row,ImportLongBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportPixelRows(&row,ImportLongIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportPixelRows(&row,ImportLongRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportPixelRows(&row,ImportLongRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportPixelRows(&row,ImportLongRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ImportPixelRows(&row,ImportLongMapRow,pixels,exception));
}

static const void *ImportLongLongBGRPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const MagickSizeType
    *magick_restrict p = (const MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongLongBGRAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const MagickSizeType
    *magick_restrict p = (const MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleLongLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongLongBGRPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const MagickSizeType
    *magick_restrict p = (const MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongLongIPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const MagickSizeType
    *magick_restrict p = (const MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelGray(image,ScaleLongLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongLongRGBPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const MagickSizeType
    *magick_restrict p = (const MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongLongRGBAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const MagickSizeType
    *magick_restrict p = (const MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleLongLongToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportLongLongRGBPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const MagickSizeType
    *magick_restrict p = (const MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
    SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static void ImportLongLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const MagickSizeType *magick_restrict p,
  Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      SetPixelAlpha(image,ScaleLongLongToQuantum(*p),q);
      break;
    }
    case OpacityQuantum:
    {
      SetPixelAlpha(image,ScaleLongLongToQuantum(*p),q);
      break;
    }
    case BlackQuantum:
    {
      SetPixelBlack(image,ScaleLongLongToQuantum(*p),q);
      break;
    }
    case IndexQuantum:
    {
      SetPixelGray(image,ScaleLongLongToQuantum(*p),q);
      break;
    }
    default:
      break;
  }
}

static void ImportLongLongQuantum(const Image *image,const QuantumType quantum,
  const MagickSizeType *magick_restrict p,Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      SetPixelRed(image,ScaleLongLongToQuantum(*p),q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      SetPixelGreen(image,ScaleLongLongToQuantum(*p),q);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      SetPixelBlue(image,ScaleLongLongToQuantum(*p),q);
      break;
    }
    default:
    {
      ImportLongLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static const void *ImportLongLongMapRow(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  const MagickSizeType
    *magick_restrict p = (const MagickSizeType *) pixels;

  Image
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
      ImportLongLongQuantum(image,quantum_map[i],p,q);
      p++;
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static MagickBooleanType ImportLongLongPixel(Image *image,
  const RectangleInfo *roi,const ImportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  const void
    *pixels = buffer->pixels;

  ImportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportPixelRows(&row,ImportLongLongBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportPixelRows(&row,ImportLongLongBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportPixelRows(&row,ImportLongLongBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportPixelRows(&row,ImportLongLongIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportPixelRows(&row,ImportLongLongRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportPixelRows(&row,ImportLongLongRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportPixelRows(&row,ImportLongLongRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ImportPixelRows(&row,ImportLongLongMapRow,pixels,exception));
}

static const void *ImportQuantumBGRPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const Quantum
    *magick_restrict p = (const Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,*p++,q);
    SetPixelGreen(image,*p++,q);
    SetPixelRed(image,*p++,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportQuantumBGRAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const Quantum
    *magick_restrict p = (const Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,*p++,q);
    SetPixelGreen(image,*p++,q);
    SetPixelRed(image,*p++,q);
    SetPixelAlpha(image,*p++,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportQuantumBGRPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const Quantum
    *magick_restrict p = (const Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,*p++,q);
    SetPixelGreen(image,*p++,q);
    SetPixelRed(image,*p++,q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportQuantumIPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const Quantum
    *magick_restrict p = (const Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelGray(image,*p++,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportQuantumRGBPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const Quantum
    *magick_restrict p = (const Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,*p++,q);
    SetPixelGreen(image,*p++,q);
    SetPixelBlue(image,*p++,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportQuantumRGBAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const Quantum
    *magick_restrict p = (const Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,*p++,q);
    SetPixelGreen(image,*p++,q);
    SetPixelBlue(image,*p++,q);
    SetPixelAlpha(image,*p++,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportQuantumRGBPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const Quantum
    *magick_restrict p = (const Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,*p++,q);
    SetPixelGreen(image,*p++,q);
    SetPixelBlue(image,*p++,q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static void ImportQuantumAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *magick_restrict p,
  Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      SetPixelAlpha(image,*p,q);
      break;
    }
    case OpacityQuantum:
    {
      SetPixelAlpha(image,*p,q);
      break;
    }
    case BlackQuantum:
    {
      SetPixelBlack(image,*p,q);
      break;
    }
    case IndexQuantum:
    {
      SetPixelGray(image,*p,q);
      break;
    }
    default:
      break;
  }
}

static void ImportQuantumQuantum(const Image *image,const QuantumType quantum,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      SetPixelRed(image,*p,q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      SetPixelGreen(image,*p,q);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      SetPixelBlue(image,*p,q);
      break;
    }
    default:
    {
      ImportQuantumAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static const void *ImportQuantumMapRow(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  const Quantum
    *magick_restrict p = (const Quantum *) pixels;

  Image
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
      ImportQuantumQuantum(image,quantum_map[i],p,q);
      p++;
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static MagickBooleanType ImportQuantumPixel(Image *image,
  const RectangleInfo *roi,const ImportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  const void
    *pixels = buffer->pixels;

  ImportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportPixelRows(&row,ImportQuantumBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportPixelRows(&row,ImportQuantumBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportPixelRows(&row,ImportQuantumBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportPixelRows(&row,ImportQuantumIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportPixelRows(&row,ImportQuantumRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportPixelRows(&row,ImportQuantumRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportPixelRows(&row,ImportQuantumRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ImportPixelRows(&row,ImportQuantumMapRow,pixels,exception));
}

static const void *ImportShortBGRPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned short
    *magick_restrict p = (const unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
    SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
    SetPixelRed(image,ScaleShortToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportShortBGRAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned short
    *magick_restrict p = (const unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
    SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
    SetPixelRed(image,ScaleShortToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleShortToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportShortBGRPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned short
    *magick_restrict p = (const unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
    SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
    SetPixelRed(image,ScaleShortToQuantum(*p++),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportShortIPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned short
    *magick_restrict p = (const unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelGray(image,ScaleShortToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportShortRGBPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned short
    *magick_restrict p = (const unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleShortToQuantum(*p++),q);
    SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
    SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportShortRGBAPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned short
    *magick_restrict p = (const unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleShortToQuantum(*p++),q);
    SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
    SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
    SetPixelAlpha(image,ScaleShortToQuantum(*p++),q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static const void *ImportShortRGBPPixels(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  Image
    *image = row->image;

  const RectangleInfo
    *roi = row->roi;

  const unsigned short
    *magick_restrict p = (const unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) roi->width; x++)
  {
    SetPixelRed(image,ScaleShortToQuantum(*p++),q);
    SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
    SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
    p++;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static void ImportShortAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const unsigned short *magick_restrict p,
  Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case AlphaQuantum:
    {
      SetPixelAlpha(image,ScaleShortToQuantum(*p),q);
      break;
    }
    case OpacityQuantum:
    {
      SetPixelAlpha(image,ScaleShortToQuantum(*p),q);
      break;
    }
    case BlackQuantum:
    {
      SetPixelBlack(image,ScaleShortToQuantum(*p),q);
      break;
    }
    case IndexQuantum:
    {
      SetPixelGray(image,ScaleShortToQuantum(*p),q);
      break;
    }
    default:
      break;
  }
}

static void ImportShortQuantum(const Image *image,const QuantumType quantum,
  const unsigned short *magick_restrict p,Quantum *magick_restrict q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      SetPixelRed(image,ScaleShortToQuantum(*p),q);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      SetPixelGreen(image,ScaleShortToQuantum(*p),q);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      SetPixelBlue(image,ScaleShortToQuantum(*p),q);
      break;
    }
    default:
    {
      ImportShortAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static const void *ImportShortMapRow(const ImportRowInfo *row,
  Quantum *magick_restrict q,const void *pixels)
{
  const unsigned short
    *magick_restrict p = (const unsigned short *) pixels;

  Image
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
      ImportShortQuantum(image,quantum_map[i],p,q);
      p++;
    }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(p);
}

static MagickBooleanType ImportShortPixel(Image *image,
  const RectangleInfo *roi,const ImportPixelBuffer *buffer,
  ExceptionInfo *exception)
{
  const char
    *magick_restrict map = buffer->map;

  const QuantumType
    *quantum_map = buffer->quantum_map;

  const void
    *pixels = buffer->pixels;

  ImportRowInfo
    row;

  row.image=image;
  row.roi=roi;
  row.quantum_map=quantum_map;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportPixelRows(&row,ImportShortBGRPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportPixelRows(&row,ImportShortBGRAPixels,pixels,
      exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportPixelRows(&row,ImportShortBGRPPixels,pixels,
      exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportPixelRows(&row,ImportShortIPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportPixelRows(&row,ImportShortRGBPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportPixelRows(&row,ImportShortRGBAPixels,pixels,
      exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportPixelRows(&row,ImportShortRGBPPixels,pixels,
      exception));
  row.length=strlen(map);
  return(ImportPixelRows(&row,ImportShortMapRow,pixels,exception));
}

static MagickBooleanType ImportMapQuantumGPR(const char *map,const ssize_t i,
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

typedef struct _ImportMapRequest
{
  Image
    *image;

  const char
    *map;

  ssize_t
    i;

  ExceptionInfo
    *exception;
} ImportMapRequest;

static MagickBooleanType ImportMapQuantumAO(const ImportMapRequest *request,
  QuantumType *quantum_map)
{
  Image
    *image = request->image;

  const char
    *map = request->map;

  const ssize_t
    i = request->i;

  ExceptionInfo
    *exception = request->exception;

  switch (map[i])
  {
    case 'a':
    case 'A':
    {
      quantum_map[i]=AlphaQuantum;
      image->alpha_trait=BlendPixelTrait;
      break;
    }
    case 'O':
    case 'o':
    {
      quantum_map[i]=OpacityQuantum;
      image->alpha_trait=BlendPixelTrait;
      break;
    }
    default:
    {
      if (ImportMapQuantumGPR(map,i,quantum_map,exception) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType ImportMapQuantumBI(const ImportMapRequest *request,
  QuantumType *quantum_map)
{
  Image
    *image = request->image;

  const char
    *map = request->map;

  const ssize_t
    i = request->i;

  ExceptionInfo
    *exception = request->exception;

  switch (map[i])
  {
    case 'B':
    case 'b':
    {
      quantum_map[i]=BlueQuantum;
      break;
    }
    case 'I':
    case 'i':
    {
      quantum_map[i]=IndexQuantum;
      (void) SetImageColorspace(image,GRAYColorspace,exception);
      break;
    }
    default:
    {
      if (ImportMapQuantumAO(request,quantum_map) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType ImportMapQuantumCK(const ImportMapRequest *request,
  QuantumType *quantum_map)
{
  Image
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
      (void) SetImageColorspace(image,CMYKColorspace,exception);
      break;
    }
    case 'K':
    case 'k':
    {
      quantum_map[i]=BlackQuantum;
      (void) SetImageColorspace(image,CMYKColorspace,exception);
      break;
    }
    default:
    {
      if (ImportMapQuantumBI(request,quantum_map) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType ImportMapQuantumMY(const ImportMapRequest *request,
  QuantumType *quantum_map)
{
  Image
    *image = request->image;

  const char
    *map = request->map;

  const ssize_t
    i = request->i;

  ExceptionInfo
    *exception = request->exception;

  switch (map[i])
  {
    case 'm':
    case 'M':
    {
      quantum_map[i]=MagentaQuantum;
      (void) SetImageColorspace(image,CMYKColorspace,exception);
      break;
    }
    case 'Y':
    case 'y':
    {
      quantum_map[i]=YellowQuantum;
      (void) SetImageColorspace(image,CMYKColorspace,exception);
      break;
    }
    default:
    {
      if (ImportMapQuantumCK(request,quantum_map) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _ImportTypeRequest
{
  Image
    *image;

  RectangleInfo
    roi;

  ImportPixelBuffer
    buffer;

  StorageType
    type;

  ExceptionInfo
    *exception;
} ImportTypeRequest;

static void ImportPixelsOfType(const ImportTypeRequest *request,
  MagickBooleanType *status)
{
  Image
    *image = request->image;

  const RectangleInfo
    roi = request->roi;

  const ImportPixelBuffer
    buffer = request->buffer;

  const StorageType
    type = request->type;

  ExceptionInfo
    *exception = request->exception;

  switch (type)
  {
    case DoublePixel:
    {
      (*status)=ImportDoublePixel(image,&roi,&buffer,exception);
      break;
    }
    case FloatPixel:
    {
      (*status)=ImportFloatPixel(image,&roi,&buffer,exception);
      break;
    }
    case LongPixel:
    {
      (*status)=ImportLongPixel(image,&roi,&buffer,exception);
      break;
    }
    case LongLongPixel:
    {
      (*status)=ImportLongLongPixel(image,&roi,&buffer,exception);
      break;
    }
    case QuantumPixel:
    {
      (*status)=ImportQuantumPixel(image,&roi,&buffer,exception);
      break;
    }
    case ShortPixel:
    {
      (*status)=ImportShortPixel(image,&roi,&buffer,exception);
      break;
    }
    default:
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnrecognizedStorageType","`%d'",type);
      (*status)=MagickFalse;
    }
  }
}

typedef struct _ImportMapParse
{
  Image
    *image;

  const char
    *map;

  size_t
    length;

  QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ImportMapParse;

static MagickBooleanType ImportMapQuantums(const ImportMapParse *parse)
{
  Image
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
    ImportMapRequest
      request;

    request.image=image;
    request.map=map;
    request.i=i;
    request.exception=exception;
    if (ImportMapQuantumMY(&request,quantum_map) == MagickFalse)
      return(MagickFalse);
  }
  return(MagickTrue);
}

MagickExport MagickBooleanType ImportImagePixels(Image *image,const ssize_t x,
  const ssize_t y,const size_t width,const size_t height,const char *map,
  const StorageType type,const void *pixels,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  QuantumType
    *quantum_map;

  RectangleInfo
    roi;

  ImportMapParse
    parse;

  size_t
    length;

  ImportPixelBuffer buffer;

  /*
    Allocate image structure.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  length=strlen(map);
  quantum_map=(QuantumType *) AcquireQuantumMemory(length,sizeof(*quantum_map));
  if (quantum_map == (QuantumType *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  parse.image=image;
  parse.map=map;
  parse.length=length;
  parse.quantum_map=quantum_map;
  parse.exception=exception;
  if (ImportMapQuantums(&parse) == MagickFalse)
    return(MagickFalse);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Transfer the pixels from the pixel data to the image.
  */
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
      status=ImportCharPixel(image,&roi,&buffer,exception);
      break;
    }
    default:
    {
      ImportTypeRequest
        request;

      request.image=image;
      request.roi=roi;
      request.buffer=buffer;
      request.type=type;
      request.exception=exception;
      ImportPixelsOfType(&request,&status);
      break;
    }
  }
  quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
  return(status);
}
