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

static MagickBooleanType ExportCharBGRPixels(const Image *image,
  const RectangleInfo *roi,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
      *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
      *q++=ScaleQuantumToChar(GetPixelRed(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportCharBGRAPixels(const Image *image,
  const RectangleInfo *roi,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
      *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
      *q++=ScaleQuantumToChar(GetPixelRed(image,p));
      *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportCharBGRPPixels(const Image *image,
  const RectangleInfo *roi,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
      *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
      *q++=ScaleQuantumToChar(GetPixelRed(image,p));
      *q++=ScaleQuantumToChar((Quantum) 0);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportCharIPixels(const Image *image,
  const RectangleInfo *roi,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(image,p)));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportCharRGBPixels(const Image *image,
  const RectangleInfo *roi,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToChar(GetPixelRed(image,p));
      *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
      *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportCharRGBAPixels(const Image *image,
  const RectangleInfo *roi,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToChar(GetPixelRed(image,p));
      *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
      *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
      *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportCharRGBPPixels(const Image *image,
  const RectangleInfo *roi,unsigned char *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToChar(GetPixelRed(image,p));
      *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
      *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
      *q++=ScaleQuantumToChar((Quantum) 0);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  unsigned char
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  q=(unsigned char *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportCharBGRPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportCharBGRAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportCharBGRPPixels(image,roi,q,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportCharIPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportCharRGBPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportCharRGBAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportCharRGBPPixels(image,roi,q,exception));
  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
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
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportDoubleBGRPixels(const Image *image,
  const RectangleInfo *roi,double *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=QuantumScale*(double) GetPixelBlue(image,p);
      *q++=QuantumScale*(double) GetPixelGreen(image,p);
      *q++=QuantumScale*(double) GetPixelRed(image,p);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportDoubleBGRAPixels(const Image *image,
  const RectangleInfo *roi,double *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=QuantumScale*(double) GetPixelBlue(image,p);
      *q++=QuantumScale*(double) GetPixelGreen(image,p);
      *q++=QuantumScale*(double) GetPixelRed(image,p);
      *q++=QuantumScale*(double) GetPixelAlpha(image,p);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportDoubleBGRPPixels(const Image *image,
  const RectangleInfo *roi,double *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=QuantumScale*(double) GetPixelBlue(image,p);
      *q++=QuantumScale*(double) GetPixelGreen(image,p);
      *q++=QuantumScale*(double) GetPixelRed(image,p);
      *q++=0.0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportDoubleIPixels(const Image *image,
  const RectangleInfo *roi,double *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=(double) (QuantumScale*(double) GetPixelIntensity(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportDoubleRGBPixels(const Image *image,
  const RectangleInfo *roi,double *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=QuantumScale*(double) GetPixelRed(image,p);
      *q++=QuantumScale*(double) GetPixelGreen(image,p);
      *q++=QuantumScale*(double) GetPixelBlue(image,p);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportDoubleRGBAPixels(const Image *image,
  const RectangleInfo *roi,double *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=QuantumScale*(double) GetPixelRed(image,p);
      *q++=QuantumScale*(double) GetPixelGreen(image,p);
      *q++=QuantumScale*(double) GetPixelBlue(image,p);
      *q++=QuantumScale*(double) GetPixelAlpha(image,p);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportDoubleRGBPPixels(const Image *image,
  const RectangleInfo *roi,double *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=QuantumScale*(double) GetPixelRed(image,p);
      *q++=QuantumScale*(double) GetPixelGreen(image,p);
      *q++=QuantumScale*(double) GetPixelBlue(image,p);
      *q++=0.0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const Quantum
    *magick_restrict p;

  double
    *magick_restrict q;

  ssize_t
    x;

  size_t
    length;

  ssize_t
    y;

  q=(double *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportDoubleBGRPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportDoubleBGRAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportDoubleBGRPPixels(image,roi,q,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportDoubleIPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportDoubleRGBPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportDoubleRGBAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportDoubleRGBPPixels(image,roi,q,exception));
  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
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
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportFloatBGRPixels(const Image *image,
  const RectangleInfo *roi,float *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportFloatBGRAPixels(const Image *image,
  const RectangleInfo *roi,float *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportFloatBGRPPixels(const Image *image,
  const RectangleInfo *roi,float *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
      *q++=0.0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportFloatIPixels(const Image *image,
  const RectangleInfo *roi,float *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=(float) (QuantumScale*(double) GetPixelIntensity(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportFloatRGBPixels(const Image *image,
  const RectangleInfo *roi,float *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportFloatRGBAPixels(const Image *image,
  const RectangleInfo *roi,float *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportFloatRGBPPixels(const Image *image,
  const RectangleInfo *roi,float *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=(float) (QuantumScale*(double) GetPixelRed(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelGreen(image,p));
      *q++=(float) (QuantumScale*(double) GetPixelBlue(image,p));
      *q++=0.0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const Quantum
    *magick_restrict p;

  float
    *magick_restrict q;

  ssize_t
    x;

  size_t
    length;

  ssize_t
    y;

  q=(float *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportFloatBGRPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportFloatBGRAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportFloatBGRPPixels(image,roi,q,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportFloatIPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportFloatRGBPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportFloatRGBAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportFloatRGBPPixels(image,roi,q,exception));
  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
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
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongBGRPixels(const Image *image,
  const RectangleInfo *roi,unsigned int *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
      *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLong(GetPixelRed(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongBGRAPixels(const Image *image,
  const RectangleInfo *roi,unsigned int *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
      *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLong(GetPixelRed(image,p));
      *q++=ScaleQuantumToLong(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongBGRPPixels(const Image *image,
  const RectangleInfo *roi,unsigned int *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
      *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLong(GetPixelRed(image,p));
      *q++=0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongIPixels(const Image *image,
  const RectangleInfo *roi,unsigned int *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLong(ClampToQuantum(GetPixelIntensity(image,p)));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongRGBPixels(const Image *image,
  const RectangleInfo *roi,unsigned int *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLong(GetPixelRed(image,p));
      *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongRGBAPixels(const Image *image,
  const RectangleInfo *roi,unsigned int *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLong(GetPixelRed(image,p));
      *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
      *q++=ScaleQuantumToLong(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongRGBPPixels(const Image *image,
  const RectangleInfo *roi,unsigned int *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLong(GetPixelRed(image,p));
      *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
      *q++=0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  unsigned int
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  q=(unsigned int *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportLongBGRPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportLongBGRAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportLongBGRPPixels(image,roi,q,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportLongIPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportLongRGBPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportLongRGBAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportLongRGBPPixels(image,roi,q,exception));
  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
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
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongLongBGRPixels(const Image *image,
  const RectangleInfo *roi,MagickSizeType *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongLongBGRAPixels(const Image *image,
  const RectangleInfo *roi,MagickSizeType *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongLongBGRPPixels(const Image *image,
  const RectangleInfo *roi,MagickSizeType *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
      *q++=0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongLongIPixels(const Image *image,
  const RectangleInfo *roi,MagickSizeType *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLongLong(ClampToQuantum(
        GetPixelIntensity(image,p)));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongLongRGBPixels(const Image *image,
  const RectangleInfo *roi,MagickSizeType *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongLongRGBAPixels(const Image *image,
  const RectangleInfo *roi,MagickSizeType *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportLongLongRGBPPixels(const Image *image,
  const RectangleInfo *roi,MagickSizeType *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
      *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
      *q++=0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  MagickSizeType
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  q=(MagickSizeType *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportLongLongBGRPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportLongLongBGRAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportLongLongBGRPPixels(image,roi,q,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportLongLongIPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportLongLongRGBPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportLongLongRGBAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportLongLongRGBPPixels(image,roi,q,exception));
  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
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
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportQuantumBGRPixels(const Image *image,
  const RectangleInfo *roi,Quantum *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=GetPixelBlue(image,p);
      *q++=GetPixelGreen(image,p);
      *q++=GetPixelRed(image,p);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportQuantumBGRAPixels(const Image *image,
  const RectangleInfo *roi,Quantum *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=GetPixelBlue(image,p);
      *q++=GetPixelGreen(image,p);
      *q++=GetPixelRed(image,p);
      *q++=(Quantum) (GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportQuantumBGRPPixels(const Image *image,
  const RectangleInfo *roi,Quantum *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=GetPixelBlue(image,p);
      *q++=GetPixelGreen(image,p);
      *q++=GetPixelRed(image,p);
      *q++=(Quantum) 0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportQuantumIPixels(const Image *image,
  const RectangleInfo *roi,Quantum *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ClampToQuantum(GetPixelIntensity(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportQuantumRGBPixels(const Image *image,
  const RectangleInfo *roi,Quantum *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=GetPixelRed(image,p);
      *q++=GetPixelGreen(image,p);
      *q++=GetPixelBlue(image,p);
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportQuantumRGBAPixels(const Image *image,
  const RectangleInfo *roi,Quantum *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=GetPixelRed(image,p);
      *q++=GetPixelGreen(image,p);
      *q++=GetPixelBlue(image,p);
      *q++=(Quantum) (GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportQuantumRGBPPixels(const Image *image,
  const RectangleInfo *roi,Quantum *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=GetPixelRed(image,p);
      *q++=GetPixelGreen(image,p);
      *q++=GetPixelBlue(image,p);
      *q++=(Quantum) 0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const Quantum
    *magick_restrict p;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  size_t
    length;

  ssize_t
    y;

  q=(Quantum *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportQuantumBGRPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportQuantumBGRAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportQuantumBGRPPixels(image,roi,q,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportQuantumIPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportQuantumRGBPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportQuantumRGBAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportQuantumRGBPPixels(image,roi,q,exception));
  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      ssize_t
        i;

      for (i=0; i < (ssize_t) length; i++)
      {
        *q=(Quantum) 0;
        switch (quantum_map[i])
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
        q++;
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportShortBGRPixels(const Image *image,
  const RectangleInfo *roi,unsigned short *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
      *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
      *q++=ScaleQuantumToShort(GetPixelRed(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportShortBGRAPixels(const Image *image,
  const RectangleInfo *roi,unsigned short *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
      *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
      *q++=ScaleQuantumToShort(GetPixelRed(image,p));
      *q++=ScaleQuantumToShort(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportShortBGRPPixels(const Image *image,
  const RectangleInfo *roi,unsigned short *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
      *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
      *q++=ScaleQuantumToShort(GetPixelRed(image,p));
      *q++=0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportShortIPixels(const Image *image,
  const RectangleInfo *roi,unsigned short *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToShort(ClampToQuantum(GetPixelIntensity(image,p)));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportShortRGBPixels(const Image *image,
  const RectangleInfo *roi,unsigned short *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToShort(GetPixelRed(image,p));
      *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
      *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportShortRGBAPixels(const Image *image,
  const RectangleInfo *roi,unsigned short *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToShort(GetPixelRed(image,p));
      *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
      *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
      *q++=ScaleQuantumToShort(GetPixelAlpha(image,p));
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
}

static MagickBooleanType ExportShortRGBPPixels(const Image *image,
  const RectangleInfo *roi,unsigned short *magick_restrict q,
  ExceptionInfo *exception)
{
  const Quantum
    *magick_restrict p;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      *q++=ScaleQuantumToShort(GetPixelRed(image,p));
      *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
      *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
      *q++=0;
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  const Quantum
    *magick_restrict p;

  ssize_t
    x;

  unsigned short
    *magick_restrict q;

  size_t
    length;

  ssize_t
    y;

  q=(unsigned short *) pixels;
  if (LocaleCompare(map,"BGR") == 0)
    return(ExportShortBGRPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRA") == 0)
    return(ExportShortBGRAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"BGRP") == 0)
    return(ExportShortBGRPPixels(image,roi,q,exception));
  if (LocaleCompare(map,"I") == 0)
    return(ExportShortIPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGB") == 0)
    return(ExportShortRGBPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBA") == 0)
    return(ExportShortRGBAPixels(image,roi,q,exception));
  if (LocaleCompare(map,"RGBP") == 0)
    return(ExportShortRGBPPixels(image,roi,q,exception));
  length=strlen(map);
  for (y=0; y < (ssize_t) roi->height; y++)
  {
    p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) roi->width; x++)
    {
      ssize_t
        i;

      for (i=0; i < (ssize_t) length; i++)
      {
        *q=0;
        switch (quantum_map[i])
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
        q++;
      }
      p+=(ptrdiff_t) GetPixelChannels(image);
    }
  }
  return(y < (ssize_t) roi->height ? MagickFalse : MagickTrue);
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

  ssize_t
    i;

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
  for (i=0; i < (ssize_t) length; i++)
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
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "UnrecognizedPixelMap","`%s'",map);
        return(MagickFalse);
      }
    }
  }
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
    case DoublePixel:
    {
      status=ExportDoublePixel(image,&roi,&buffer,exception);
      break;
    }
    case FloatPixel:
    {
      status=ExportFloatPixel(image,&roi,&buffer,exception);
      break;
    }
    case LongPixel:
    {
      status=ExportLongPixel(image,&roi,&buffer,exception);
      break;
    }
    case LongLongPixel:
    {
      status=ExportLongLongPixel(image,&roi,&buffer,exception);
      break;
    }
    case QuantumPixel:
    {
      status=ExportQuantumPixel(image,&roi,&buffer,exception);
      break;
    }
    case ShortPixel:
    {
      status=ExportShortPixel(image,&roi,&buffer,exception);
      break;
    }
    default:
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnrecognizedPixelMap","`%s'",map);
      status=MagickFalse;
    }
  }
  quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
  return(status);
}
