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

  MagickCore private methods of morphology-primitive.c: the direct pass's
  pixel and kernel-row helpers, from the MorphologyDirectPixel typedef to
  ForwardDirectChannelValue; included by that file only.
*/
#ifndef MAGICKCORE_MORPHOLOGY_PRIMITIVE_DIRECT_ROW_PRIVATE_H
#define MAGICKCORE_MORPHOLOGY_PRIMITIVE_DIRECT_ROW_PRIVATE_H

typedef struct _MorphologyDirectPixel
{
  Image
    *image;

  const KernelInfo
    *kernel;

  Quantum
    *q;

  ssize_t
    x,
    i;
} MorphologyDirectPixel;

static magick_attribute((__always_inline__)) inline double
  MinimizeLeadingRowDistance(const MorphologyDirectPixel *row,
  const OffsetInfo offset,double pixel)
{
  Image
    *const image = row->image;

  const KernelInfo
    *const kernel = row->kernel;

  Quantum
    *const q = row->q;

  const ssize_t
    x = row->x,
    i = row->i;

  const MagickRealType
    *magick_restrict k;

  const Quantum
    *magick_restrict pixels;

  ssize_t
    u;

  k=(&kernel->values[(ssize_t) kernel->width*(kernel->y+1)-1]);
  pixels=q-offset.x*(ssize_t) GetPixelChannels(image);
  for (u=0; u < offset.x; u++)
  {
    if (!IsNaN(*k) && ((x+u-offset.x) >= 0))
      {
        if (((double) pixels[i]+(*k)) < pixel)
          pixel=(double) pixels[i]+(*k);
      }
    k--;
    pixels+=(ptrdiff_t) GetPixelChannels(image);
  }
  return(pixel);
}

static magick_attribute((__always_inline__)) inline double
  MinimizeTrailingRowDistance(const MorphologyDirectPixel *row,
  const OffsetInfo offset,const MagickRealType *magick_restrict k,double pixel)
{
  Image
    *const image = row->image;

  const KernelInfo
    *const kernel = row->kernel;

  Quantum
    *const q = row->q;

  const ssize_t
    x = row->x,
    i = row->i;

  const Quantum
    *magick_restrict pixels;

  ssize_t
    u;

  pixels=q;
  for (u=offset.x+1; u < (ssize_t) kernel->width; u++)
  {
    pixels+=(ptrdiff_t) GetPixelChannels(image);
    if (!IsNaN(*k) && ((x+u-offset.x) < (ssize_t) image->columns))
      {
        if (((double) pixels[i]+(*k)) < pixel)
          pixel=(double) pixels[i]+(*k);
      }
    k--;
  }
  return(pixel);
}

typedef struct _MorphologyKernelRows
{
  Image
    *image;

  const KernelInfo
    *kernel;

  ssize_t
    i;
} MorphologyKernelRows;

static magick_attribute((__always_inline__)) inline void
  MinimizeKernelRowDistance(const MorphologyKernelRows *rows,
  double *pixel,const MagickRealType *magick_restrict *k,
  const Quantum *magick_restrict *pixels)
{
  Image
    *const image = rows->image;

  const KernelInfo
    *const kernel = rows->kernel;

  const ssize_t
    i = rows->i;

  ssize_t
    u;

  for (u=0; u < (ssize_t) kernel->width; u++)
  {
    *pixel=MinimizeKernelDistance(*k,*pixels,i,*pixel);
    (*k)--;
    *pixels+=(ptrdiff_t) GetPixelChannels(image);
  }
  *pixels+=(image->columns-1)*GetPixelChannels(image);
}

static magick_attribute((__always_inline__)) inline double
  MinimizeTrailingRowsDistance(const MorphologyKernelRows *rows,
  const OffsetInfo offset,const Quantum *magick_restrict pixels,double pixel)
{
  Image
    *const image = rows->image;

  const KernelInfo
    *const kernel = rows->kernel;

  const ssize_t
    i = rows->i;

  const MagickRealType
    *magick_restrict k;

  ssize_t
    u,
    v;

  k=(&kernel->values[(ssize_t) kernel->width*(kernel->y+1)-1]);
  for (v=offset.y; v < (ssize_t) kernel->height; v++)
  {
    for (u=0; u < (ssize_t) kernel->width; u++)
    {
      pixel=MinimizeKernelDistance(k,pixels,i,pixel);
      k--;
      pixels+=(ptrdiff_t) GetPixelChannels(image);
    }
    pixels+=(image->columns-1)*GetPixelChannels(image);
  }
  return(pixel);
}

static OffsetInfo GetDirectKernelOffset(const MorphologyMethod method,
  const KernelInfo *kernel)
{
  OffsetInfo
    offset;

  switch(method)
  {
    case DistanceMorphology:
    case VoronoiMorphology:
    {
      /*
        Kernel reflected about origin.
      */
      offset.x=(ssize_t) kernel->width-kernel->x-1;
      offset.y=(ssize_t) kernel->height-kernel->y-1;
      break;
    }
    default:
    {
      offset.x=kernel->x;
      offset.y=kernel->y;
      break;
    }
  }
  return(offset);
}

typedef enum
{
  ForwardDirectScan,
  ReverseDirectScan
} DirectScanSide;

typedef struct _MorphologyDirectScan
{
  DirectScanSide
    side;

  Image
    *image;

  MorphologyMethod
    method;

  const KernelInfo
    *kernel;

  OffsetInfo
    offset;

  CacheView
    *image_view,
    *morphology_view;

  size_t
    width;

  ExceptionInfo
    *exception;
} MorphologyDirectScan;

static double ForwardDirectChannelValue(const MorphologyDirectScan *scan,
  const MorphologyDirectPixel *input,const Quantum *magick_restrict pixels,
  double pixel)
{
  Image
    *const image = input->image;

  const KernelInfo
    *const kernel = input->kernel;

  Quantum
    *const q = input->q;

  const ssize_t
    x = input->x,
    i = input->i;

  const MorphologyMethod
    method = scan->method;

  const OffsetInfo
    offset = scan->offset;

  const MagickRealType
    *magick_restrict k;

  MorphologyDirectPixel
    row;

  MorphologyKernelRows
    rows;

  ssize_t
    v;

  switch (method)
  {
    case DistanceMorphology:
    {
      k=(&kernel->values[kernel->width*kernel->height-1]);
      for (v=0; v <= offset.y; v++)
      {
        rows.image=image;
        rows.kernel=kernel;
        rows.i=i;
        MinimizeKernelRowDistance(&rows,&pixel,&k,&pixels);
      }
      row.image=image;
      row.kernel=kernel;
      row.q=q;
      row.x=x;
      row.i=i;
      pixel=MinimizeLeadingRowDistance(&row,offset,pixel);
      break;
    }
    case VoronoiMorphology:
    {
      k=(&kernel->values[kernel->width*kernel->height-1]);
      for (v=0; v < offset.y; v++)
      {
        rows.image=image;
        rows.kernel=kernel;
        rows.i=i;
        MinimizeKernelRowDistance(&rows,&pixel,&k,&pixels);
      }
      row.image=image;
      row.kernel=kernel;
      row.q=q;
      row.x=x;
      row.i=i;
      pixel=MinimizeLeadingRowDistance(&row,offset,pixel);
      break;
    }
    default:
      break;
  }
  return(pixel);
}

#endif
