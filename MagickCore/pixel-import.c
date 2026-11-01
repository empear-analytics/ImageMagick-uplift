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

static MagickBooleanType ImportCharBGRPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
      SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
      SetPixelRed(image,ScaleCharToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportCharBGRAPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
      SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
      SetPixelRed(image,ScaleCharToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportCharBGROPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
      SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
      SetPixelRed(image,ScaleCharToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportCharBGRPPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
      SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
      SetPixelRed(image,ScaleCharToQuantum(*p++),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportCharIPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelGray(image,ScaleCharToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportCharRGBPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleCharToQuantum(*p++),q);
      SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
      SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportCharRGBAPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleCharToQuantum(*p++),q);
      SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
      SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportCharRGBOPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleCharToQuantum(*p++),q);
      SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
      SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleCharToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportCharRGBPPixels(Image *image,const RectangleInfo *roi,
  const unsigned char *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleCharToQuantum(*p++),q);
      SetPixelGreen(image,ScaleCharToQuantum(*p++),q);
      SetPixelBlue(image,ScaleCharToQuantum(*p++),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

typedef struct _ImportCharRowInfo
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ImportCharRowInfo;

static const unsigned char *ImportCharMapRow(const ImportCharRowInfo *row,
  const unsigned char *magick_restrict p,Quantum *magick_restrict q)
{
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

typedef struct _ImportCharMapLoop
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const char
    *map;

  const QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ImportCharMapLoop;

static MagickBooleanType ImportCharMapPixels(const ImportCharMapLoop *loop,
  const unsigned char *magick_restrict p)
{
  Image
    *image = loop->image;

  const RectangleInfo
    *roi = loop->roi;

  const char
    *map = loop->map;

  const QuantumType
    *quantum_map = loop->quantum_map;

  ExceptionInfo
    *exception = loop->exception;

  Quantum
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    ImportCharRowInfo
      row;

    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    row.image=image;
    row.roi=roi;
    row.quantum_map=quantum_map;
    row.length=length;
    p=ImportCharMapRow(&row,p,q);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

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

  const unsigned char
    *magick_restrict p;

  ImportCharMapLoop
    loop;

  p=(const unsigned char *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportCharBGRPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportCharBGRAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRO") == 0)
    return(ImportCharBGROPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportCharBGRPPixels(image,roi,p,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportCharIPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportCharRGBPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportCharRGBAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBO") == 0)
    return(ImportCharRGBOPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportCharRGBPPixels(image,roi,p,exception));
  loop.image=image;
  loop.roi=roi;
  loop.map=map;
  loop.quantum_map=quantum_map;
  loop.exception=exception;
  return(ImportCharMapPixels(&loop,p));
}

static MagickBooleanType ImportDoubleBGRPixels(Image *image,const RectangleInfo *roi,
  const double *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportDoubleBGRAPixels(Image *image,const RectangleInfo *roi,
  const double *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportDoubleBGRPPixels(Image *image,const RectangleInfo *roi,
  const double *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportDoubleIPixels(Image *image,const RectangleInfo *roi,
  const double *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelGray(image,ClampToQuantum((double) QuantumRange*(*p)),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportDoubleRGBPixels(Image *image,const RectangleInfo *roi,
  const double *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportDoubleRGBAPixels(Image *image,const RectangleInfo *roi,
  const double *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportDoubleRGBPPixels(Image *image,const RectangleInfo *roi,
  const double *magick_restrict p,ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

typedef struct _ImportDoubleRowInfo
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ImportDoubleRowInfo;

static const double *ImportDoubleMapRow(const ImportDoubleRowInfo *row,
  const double *magick_restrict p,Quantum *magick_restrict q)
{
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

typedef struct _ImportDoubleMapLoop
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const char
    *map;

  const QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ImportDoubleMapLoop;

static MagickBooleanType ImportDoubleMapPixels(const ImportDoubleMapLoop *loop,
  const double *magick_restrict p)
{
  Image
    *image = loop->image;

  const RectangleInfo
    *roi = loop->roi;

  const char
    *map = loop->map;

  const QuantumType
    *quantum_map = loop->quantum_map;

  ExceptionInfo
    *exception = loop->exception;

  Quantum
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

   length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    ImportDoubleRowInfo
      row;

    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    row.image=image;
    row.roi=roi;
    row.quantum_map=quantum_map;
    row.length=length;
    p=ImportDoubleMapRow(&row,p,q);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const double
    *magick_restrict p;

  ImportDoubleMapLoop
    loop;

  p=(const double *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportDoubleBGRPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportDoubleBGRAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportDoubleBGRPPixels(image,roi,p,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportDoubleIPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportDoubleRGBPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportDoubleRGBAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportDoubleRGBPPixels(image,roi,p,exception));
  loop.image=image;
  loop.roi=roi;
  loop.map=map;
  loop.quantum_map=quantum_map;
  loop.exception=exception;
  return(ImportDoubleMapPixels(&loop,p));
}

static MagickBooleanType ImportFloatBGRPixels(Image *image,
  const RectangleInfo *roi,const float *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportFloatBGRAPixels(Image *image,
  const RectangleInfo *roi,const float *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportFloatBGRPPixels(Image *image,
  const RectangleInfo *roi,const float *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportFloatIPixels(Image *image,
  const RectangleInfo *roi,const float *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelGray(image,ClampToQuantum((double) QuantumRange*(double)
        (*p)),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportFloatRGBPixels(Image *image,
  const RectangleInfo *roi,const float *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportFloatRGBAPixels(Image *image,
  const RectangleInfo *roi,const float *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportFloatRGBPPixels(Image *image,
  const RectangleInfo *roi,const float *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
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
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

typedef struct _ImportFloatRowInfo
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ImportFloatRowInfo;

static const float *ImportFloatMapRow(const ImportFloatRowInfo *row,
  const float *magick_restrict p,Quantum *magick_restrict q)
{
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

typedef struct _ImportFloatMapLoop
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const char
    *map;

  const QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ImportFloatMapLoop;

static MagickBooleanType ImportFloatMapPixels(const ImportFloatMapLoop *loop,
  const float *magick_restrict p)
{
  Image
    *image = loop->image;

  const RectangleInfo
    *roi = loop->roi;

  const char
    *map = loop->map;

  const QuantumType
    *quantum_map = loop->quantum_map;

  ExceptionInfo
    *exception = loop->exception;

  Quantum
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    ImportFloatRowInfo
      row;

    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    row.image=image;
    row.roi=roi;
    row.quantum_map=quantum_map;
    row.length=length;
    p=ImportFloatMapRow(&row,p,q);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const float
    *magick_restrict p;

  ImportFloatMapLoop
    loop;

  p=(const float *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportFloatBGRPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportFloatBGRAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportFloatBGRPPixels(image,roi,p,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportFloatIPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportFloatRGBPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportFloatRGBAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportFloatRGBPPixels(image,roi,p,exception));
  loop.image=image;
  loop.roi=roi;
  loop.map=map;
  loop.quantum_map=quantum_map;
  loop.exception=exception;
  return(ImportFloatMapPixels(&loop,p));
}

static MagickBooleanType ImportLongBGRPixels(Image *image,
  const RectangleInfo *roi,const unsigned int *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
      SetPixelRed(image,ScaleLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongBGRAPixels(Image *image,
  const RectangleInfo *roi,const unsigned int *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
      SetPixelRed(image,ScaleLongToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongBGRPPixels(Image *image,
  const RectangleInfo *roi,const unsigned int *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
      SetPixelRed(image,ScaleLongToQuantum(*p++),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongIPixels(Image *image,
  const RectangleInfo *roi,const unsigned int *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelGray(image,ScaleLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongRGBPixels(Image *image,
  const RectangleInfo *roi,const unsigned int *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
      SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongRGBAPixels(Image *image,
  const RectangleInfo *roi,const unsigned int *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
      SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongRGBPPixels(Image *image,
  const RectangleInfo *roi,const unsigned int *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongToQuantum(*p++),q);
      SetPixelBlue(image,ScaleLongToQuantum(*p++),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

typedef struct _ImportLongRowInfo
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ImportLongRowInfo;

static const unsigned int *ImportLongMapRow(const ImportLongRowInfo *row,
  const unsigned int *magick_restrict p,Quantum *magick_restrict q)
{
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

typedef struct _ImportLongMapLoop
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const char
    *map;

  const QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ImportLongMapLoop;

static MagickBooleanType ImportLongMapPixels(const ImportLongMapLoop *loop,
  const unsigned int *magick_restrict p)
{
  Image
    *image = loop->image;

  const RectangleInfo
    *roi = loop->roi;

  const char
    *map = loop->map;

  const QuantumType
    *quantum_map = loop->quantum_map;

  ExceptionInfo
    *exception = loop->exception;

  Quantum
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    ImportLongRowInfo
      row;

    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    row.image=image;
    row.roi=roi;
    row.quantum_map=quantum_map;
    row.length=length;
    p=ImportLongMapRow(&row,p,q);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const unsigned int
    *magick_restrict p;

  ImportLongMapLoop
    loop;

  p=(const unsigned int *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportLongBGRPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportLongBGRAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportLongBGRPPixels(image,roi,p,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportLongIPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportLongRGBPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportLongRGBAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportLongRGBPPixels(image,roi,p,exception));
  loop.image=image;
  loop.roi=roi;
  loop.map=map;
  loop.quantum_map=quantum_map;
  loop.exception=exception;
  return(ImportLongMapPixels(&loop,p));
}

static MagickBooleanType ImportLongLongBGRPixels(Image *image,
  const RectangleInfo *roi,const MagickSizeType *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongLongBGRAPixels(Image *image,
  const RectangleInfo *roi,const MagickSizeType *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleLongLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongLongBGRPPixels(Image *image,
  const RectangleInfo *roi,const MagickSizeType *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongLongIPixels(Image *image,
  const RectangleInfo *roi,const MagickSizeType *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelGray(image,ScaleLongLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongLongRGBPixels(Image *image,
  const RectangleInfo *roi,const MagickSizeType *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongLongRGBAPixels(Image *image,
  const RectangleInfo *roi,const MagickSizeType *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleLongLongToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportLongLongRGBPPixels(Image *image,
  const RectangleInfo *roi,const MagickSizeType *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelGreen(image,ScaleLongLongToQuantum(*p++),q);
      SetPixelBlue(image,ScaleLongLongToQuantum(*p++),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

typedef struct _ImportLongLongRowInfo
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ImportLongLongRowInfo;

static const MagickSizeType *ImportLongLongMapRow(
  const ImportLongLongRowInfo *row,const MagickSizeType *magick_restrict p,
  Quantum *magick_restrict q)
{
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

typedef struct _ImportLongLongMapLoop
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const char
    *map;

  const QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ImportLongLongMapLoop;

static MagickBooleanType ImportLongLongMapPixels(
  const ImportLongLongMapLoop *loop,const MagickSizeType *magick_restrict p)
{
  Image
    *image = loop->image;

  const RectangleInfo
    *roi = loop->roi;

  const char
    *map = loop->map;

  const QuantumType
    *quantum_map = loop->quantum_map;

  ExceptionInfo
    *exception = loop->exception;

  Quantum
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    ImportLongLongRowInfo
      row;

    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    row.image=image;
    row.roi=roi;
    row.quantum_map=quantum_map;
    row.length=length;
    p=ImportLongLongMapRow(&row,p,q);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const MagickSizeType
    *magick_restrict p;

  ImportLongLongMapLoop
    loop;

  p=(const MagickSizeType *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportLongLongBGRPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportLongLongBGRAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportLongLongBGRPPixels(image,roi,p,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportLongLongIPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportLongLongRGBPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportLongLongRGBAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportLongLongRGBPPixels(image,roi,p,exception));
  loop.image=image;
  loop.roi=roi;
  loop.map=map;
  loop.quantum_map=quantum_map;
  loop.exception=exception;
  return(ImportLongLongMapPixels(&loop,p));
}

static MagickBooleanType ImportQuantumBGRPixels(Image *image,
  const RectangleInfo *roi,const Quantum *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,*p++,q);
      SetPixelGreen(image,*p++,q);
      SetPixelRed(image,*p++,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportQuantumBGRAPixels(Image *image,
  const RectangleInfo *roi,const Quantum *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,*p++,q);
      SetPixelGreen(image,*p++,q);
      SetPixelRed(image,*p++,q);
      SetPixelAlpha(image,*p++,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportQuantumBGRPPixels(Image *image,
  const RectangleInfo *roi,const Quantum *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,*p++,q);
      SetPixelGreen(image,*p++,q);
      SetPixelRed(image,*p++,q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportQuantumIPixels(Image *image,
  const RectangleInfo *roi,const Quantum *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelGray(image,*p++,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportQuantumRGBPixels(Image *image,
  const RectangleInfo *roi,const Quantum *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,*p++,q);
      SetPixelGreen(image,*p++,q);
      SetPixelBlue(image,*p++,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportQuantumRGBAPixels(Image *image,
  const RectangleInfo *roi,const Quantum *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,*p++,q);
      SetPixelGreen(image,*p++,q);
      SetPixelBlue(image,*p++,q);
      SetPixelAlpha(image,*p++,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportQuantumRGBPPixels(Image *image,
  const RectangleInfo *roi,const Quantum *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,*p++,q);
      SetPixelGreen(image,*p++,q);
      SetPixelBlue(image,*p++,q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

typedef struct _ImportQuantumRowInfo
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ImportQuantumRowInfo;

static const Quantum *ImportQuantumMapRow(const ImportQuantumRowInfo *row,
  const Quantum *magick_restrict p,Quantum *magick_restrict q)
{
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

typedef struct _ImportQuantumMapLoop
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const char
    *map;

  const QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ImportQuantumMapLoop;

static MagickBooleanType ImportQuantumMapPixels(
  const ImportQuantumMapLoop *loop,const Quantum *magick_restrict p)
{
  Image
    *image = loop->image;

  const RectangleInfo
    *roi = loop->roi;

  const char
    *map = loop->map;

  const QuantumType
    *quantum_map = loop->quantum_map;

  ExceptionInfo
    *exception = loop->exception;

  Quantum
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    ImportQuantumRowInfo
      row;

    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    row.image=image;
    row.roi=roi;
    row.quantum_map=quantum_map;
    row.length=length;
    p=ImportQuantumMapRow(&row,p,q);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const Quantum
    *magick_restrict p;

  ImportQuantumMapLoop
    loop;

  p=(const Quantum *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportQuantumBGRPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportQuantumBGRAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportQuantumBGRPPixels(image,roi,p,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportQuantumIPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportQuantumRGBPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportQuantumRGBAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportQuantumRGBPPixels(image,roi,p,exception));
  loop.image=image;
  loop.roi=roi;
  loop.map=map;
  loop.quantum_map=quantum_map;
  loop.exception=exception;
  return(ImportQuantumMapPixels(&loop,p));
}

static MagickBooleanType ImportShortBGRPixels(Image *image,
  const RectangleInfo *roi,const unsigned short *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
      SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
      SetPixelRed(image,ScaleShortToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportShortBGRAPixels(Image *image,
  const RectangleInfo *roi,const unsigned short *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
      SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
      SetPixelRed(image,ScaleShortToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleShortToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportShortBGRPPixels(Image *image,
  const RectangleInfo *roi,const unsigned short *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
      SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
      SetPixelRed(image,ScaleShortToQuantum(*p++),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportShortIPixels(Image *image,
  const RectangleInfo *roi,const unsigned short *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelGray(image,ScaleShortToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportShortRGBPixels(Image *image,
  const RectangleInfo *roi,const unsigned short *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleShortToQuantum(*p++),q);
      SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
      SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportShortRGBAPixels(Image *image,
  const RectangleInfo *roi,const unsigned short *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleShortToQuantum(*p++),q);
      SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
      SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
      SetPixelAlpha(image,ScaleShortToQuantum(*p++),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ImportShortRGBPPixels(Image *image,
  const RectangleInfo *roi,const unsigned short *magick_restrict p,
  ExceptionInfo *exception)
{
  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      SetPixelRed(image,ScaleShortToQuantum(*p++),q);
      SetPixelGreen(image,ScaleShortToQuantum(*p++),q);
      SetPixelBlue(image,ScaleShortToQuantum(*p++),q);
      p++;
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

typedef struct _ImportShortRowInfo
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const QuantumType
    *quantum_map;

  size_t
    length;
} ImportShortRowInfo;

static const unsigned short *ImportShortMapRow(const ImportShortRowInfo *row,
  const unsigned short *magick_restrict p,Quantum *magick_restrict q)
{
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

typedef struct _ImportShortMapLoop
{
  Image
    *image;

  const RectangleInfo
    *roi;

  const char
    *map;

  const QuantumType
    *quantum_map;

  ExceptionInfo
    *exception;
} ImportShortMapLoop;

static MagickBooleanType ImportShortMapPixels(const ImportShortMapLoop *loop,
  const unsigned short *magick_restrict p)
{
  Image
    *image = loop->image;

  const RectangleInfo
    *roi = loop->roi;

  const char
    *map = loop->map;

  const QuantumType
    *quantum_map = loop->quantum_map;

  ExceptionInfo
    *exception = loop->exception;

  Quantum
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    ImportShortRowInfo
      row;

    q=GetAuthenticPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    row.image=image;
    row.roi=roi;
    row.quantum_map=quantum_map;
    row.length=length;
    p=ImportShortMapRow(&row,p,q);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const unsigned short
    *magick_restrict p;

  ImportShortMapLoop
    loop;

  p=(const unsigned short *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ImportShortBGRPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ImportShortBGRAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ImportShortBGRPPixels(image,roi,p,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ImportShortIPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ImportShortRGBPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ImportShortRGBAPixels(image,roi,p,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ImportShortRGBPPixels(image,roi,p,exception));
  loop.image=image;
  loop.roi=roi;
  loop.map=map;
  loop.quantum_map=quantum_map;
  loop.exception=exception;
  return(ImportShortMapPixels(&loop,p));
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
      ImportMapRequest
        request;

      request.image=image;
      request.map=map;
      request.i=i;
      request.exception=exception;
      if (ImportMapQuantumAO(&request,quantum_map) == MagickFalse)
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
      ImportMapRequest
        request;

      request.image=image;
      request.map=map;
      request.i=i;
      request.exception=exception;
      if (ImportMapQuantumBI(&request,quantum_map) == MagickFalse)
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
      ImportMapRequest
        request;

      request.image=image;
      request.map=map;
      request.i=i;
      request.exception=exception;
      if (ImportMapQuantumCK(&request,quantum_map) == MagickFalse)
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
    switch (map[i])
    {
      default:
      {
        ImportMapRequest
          request;

        request.image=image;
        request.map=map;
        request.i=i;
        request.exception=exception;
        if (ImportMapQuantumMY(&request,quantum_map) == MagickFalse)
          return(MagickFalse);
        break;
      }
    }
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
