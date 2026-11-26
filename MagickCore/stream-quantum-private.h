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

  MagickCore private methods of stream.c: the row writers and the per-channel
  quantum writers of each storage type, from StreamRowInfo to
  StreamShortQuantum; included by that file only.
*/
#ifndef MAGICKCORE_STREAM_QUANTUM_PRIVATE_H
#define MAGICKCORE_STREAM_QUANTUM_PRIVATE_H

typedef struct _StreamRowInfo
{
  const Image
    *image;

  const QuantumInfo
    *quantum_info;

  const QuantumType
    *quantum_map;

  size_t
    length;
} StreamRowInfo;

typedef void
  (*StreamRowMethod)(const StreamRowInfo *,const Quantum *,void *);

static void StreamPixelQueue(const StreamRowInfo *row,
  const StreamRowMethod method,void *pixels)
{
  const Quantum
    *p;

  p=GetAuthenticPixelQueue(row->image);
  if (p == (const Quantum *) NULL)
    return;
  method(row,p,pixels);
}

static void StreamCharBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar((Quantum) 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(ClampToQuantum(GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToChar(GetPixelRed(image,p));
    *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
    *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
    *q++=ScaleQuantumToChar((Quantum) 0);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(ClampToQuantum(GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLong(GetPixelBlue(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=0U;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(GetPixelRed(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelGreen(image,p));
    *q++=ScaleQuantumToLongLong(GetPixelBlue(image,p));
    *q++=0U;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    *q++=GetPixelAlpha(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelRed(image,p);
    *q++=(Quantum) 0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    *q++=GetPixelAlpha(image,p);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=GetPixelRed(image,p);
    *q++=GetPixelGreen(image,p);
    *q++=GetPixelBlue(image,p);
    *q++=(Quantum) 0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(ClampToQuantum(
      GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=ScaleQuantumToShort(GetPixelAlpha(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToShort(GetPixelRed(image,p));
    *q++=ScaleQuantumToShort(GetPixelGreen(image,p));
    *q++=ScaleQuantumToShort(GetPixelBlue(image,p));
    *q++=0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ScaleQuantumToLongLong(ClampToQuantum(
      GetPixelIntensity(image,p)));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=ClampToQuantum(GetPixelIntensity(image,p));
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelAlpha(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelIntensity(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelAlpha(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  double
    *q = (double *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatBGRPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatBGRAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelAlpha(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatBGRPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatIPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelIntensity(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatRGBPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatRGBAPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelAlpha(image,p))*
      quantum_info->scale+quantum_info->minimum);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatRGBPPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  float
    *q = (float *) pixels;

  ssize_t
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    *q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
      quantum_info->scale+quantum_info->minimum);
    *q++=0.0;
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamCharAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned char *q)
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
      *q=ScaleQuantumToChar(GetPixelOpacity(image,p));
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

static void StreamCharQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned char *q)
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
      StreamCharAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

typedef struct _StreamDoubleChannel
{
  const Image
    *image;

  const QuantumInfo
    *quantum_info;

  QuantumType
    quantum;

  const Quantum
    *p;

  double
    *q;
} StreamDoubleChannel;

static void StreamDoubleAlphaIndexQuantum(const StreamDoubleChannel *channel)
{
  const Image
    *image = channel->image;

  const QuantumInfo
    *quantum_info = channel->quantum_info;

  const QuantumType
    quantum = channel->quantum;

  const Quantum
    *p = channel->p;

  double
    *q = channel->q;

  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelAlpha(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case OpacityQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelOpacity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=(double) ((QuantumScale*(double) GetPixelBlack(image,p))*
          quantum_info->scale+quantum_info->minimum);
      break;
    }
    case IndexQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelIntensity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
      *q=0;
  }
}

static void StreamDoubleQuantum(const Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum,const Quantum *p,
  double *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelRed(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelGreen(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
    {
      StreamDoubleChannel
        channel;

      channel.image=image;
      channel.quantum_info=quantum_info;
      channel.quantum=quantum;
      channel.p=p;
      channel.q=q;
      StreamDoubleAlphaIndexQuantum(&channel);
      break;
    }
  }
}

typedef struct _StreamFloatChannel
{
  const Image
    *image;

  const QuantumInfo
    *quantum_info;

  QuantumType
    quantum;

  const Quantum
    *p;

  float
    *q;
} StreamFloatChannel;

static void StreamFloatAlphaIndexQuantum(const StreamFloatChannel *channel)
{
  const Image
    *image = channel->image;

  const QuantumInfo
    *quantum_info = channel->quantum_info;

  const QuantumType
    quantum = channel->quantum;

  const Quantum
    *p = channel->p;

  float
    *q = channel->q;

  switch (quantum)
  {
    case AlphaQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelAlpha(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case OpacityQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelOpacity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace == CMYKColorspace)
        *q=(float) ((QuantumScale*(double) GetPixelBlack(image,p))*
          quantum_info->scale+quantum_info->minimum);
      break;
    }
    case IndexQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelIntensity(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
      *q=0;
  }
}

static void StreamFloatQuantum(const Image *image,
  const QuantumInfo *quantum_info,const QuantumType quantum,const Quantum *p,
  float *q)
{
  switch (quantum)
  {
    case RedQuantum:
    case CyanQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelRed(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelGreen(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      *q=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*
        quantum_info->scale+quantum_info->minimum);
      break;
    }
    default:
    {
      StreamFloatChannel
        channel;

      channel.image=image;
      channel.quantum_info=quantum_info;
      channel.quantum=quantum;
      channel.p=p;
      channel.q=q;
      StreamFloatAlphaIndexQuantum(&channel);
      break;
    }
  }
}

static void StreamLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned int *q)
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
      *q=ScaleQuantumToLong(GetPixelOpacity(image,p));
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

static void StreamLongQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned int *q)
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
      StreamLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamLongLongAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,MagickSizeType *q)
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
      *q=ScaleQuantumToLongLong(GetPixelOpacity(image,p));
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
      *q=0;
  }
}

static void StreamLongLongQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,MagickSizeType *q)
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
      StreamLongLongAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamQuantumAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,Quantum *q)
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
      *q=GetPixelOpacity(image,p);
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
      *q=(Quantum) 0;
  }
}

static void StreamQuantumQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,Quantum *q)
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
      StreamQuantumAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

static void StreamShortAlphaIndexQuantum(const Image *image,
  const QuantumType quantum,const Quantum *p,unsigned short *q)
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
      *q=ScaleQuantumToShort(GetPixelOpacity(image,p));
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
      *q=ScaleQuantumToShort(ClampToQuantum(
        GetPixelIntensity(image,p)));
      break;
    }
    default:
      break;
  }
}

static void StreamShortQuantum(const Image *image,const QuantumType quantum,
  const Quantum *p,unsigned short *q)
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
      StreamShortAlphaIndexQuantum(image,quantum,p,q);
      break;
    }
  }
}

#endif
