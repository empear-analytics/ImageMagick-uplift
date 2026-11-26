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

  MagickCore private methods of stream.c: the row writers for any channel
  map of each storage type, StreamCharMapPixels to StreamShortMapPixels;
  included by that file only.
*/
#ifndef MAGICKCORE_STREAM_MAP_PRIVATE_H
#define MAGICKCORE_STREAM_MAP_PRIVATE_H

static void StreamCharMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  unsigned char
    *q = (unsigned char *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamCharQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamDoubleMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  double
    *q = (double *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamDoubleQuantum(image,quantum_info,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamFloatMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumInfo
    *quantum_info = row->quantum_info;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  float
    *q = (float *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamFloatQuantum(image,quantum_info,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  unsigned int
    *q = (unsigned int *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamLongQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamLongLongMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  MagickSizeType
    *q = (MagickSizeType *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamLongLongQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamQuantumMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  Quantum
    *q = (Quantum *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=(Quantum) 0;
      StreamQuantumQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void StreamShortMapPixels(const StreamRowInfo *row,const Quantum *p,
  void *pixels)
{
  const Image
    *image = row->image;

  const QuantumType
    *quantum_map = row->quantum_map;

  const size_t
    length = row->length;

  unsigned short
    *q = (unsigned short *) pixels;

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) GetImageExtent(image); x++)
  {
    for (i=0; i < (ssize_t) length; i++)
    {
      *q=0;
      StreamShortQuantum(image,quantum_map[i],p,q);
      q++;
    }
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

#endif
