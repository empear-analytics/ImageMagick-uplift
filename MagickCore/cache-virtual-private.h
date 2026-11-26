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

  MagickCore private methods of cache.c: the virtual pixel helpers of
  GetVirtualPixelCacheNexus, DitherX to ReadCacheRegionPixels; included by
  that file only.
*/
#ifndef MAGICKCORE_CACHE_VIRTUAL_PRIVATE_H
#define MAGICKCORE_CACHE_VIRTUAL_PRIVATE_H

static inline ssize_t DitherX(const ssize_t x,const size_t columns)
{
  ssize_t
    index;

  index=x+DitherMatrix[x & 0x07]-32L;
  if (index < 0L)
    return(0L);
  if (index >= (ssize_t) columns)
    return((ssize_t) columns-1L);
  return(index);
}

static inline ssize_t DitherY(const ssize_t y,const size_t rows)
{
  return(DitherX(y,rows));
}

static inline ssize_t EdgeX(const ssize_t x,const size_t columns)
{
  if (x < 0L)
    return(0L);
  if (x >= (ssize_t) columns)
    return((ssize_t) (columns-1));
  return(x);
}

static inline ssize_t EdgeY(const ssize_t y,const size_t rows)
{
  if (y < 0L)
    return(0L);
  if (y >= (ssize_t) rows)
    return((ssize_t) (rows-1));
  return(y);
}

static inline MagickBooleanType IsOffsetSumOverflow(const MagickOffsetType x,
  const MagickOffsetType y)
{
  return((((y > 0) && (x > ((MagickOffsetType) MAGICK_SSIZE_MAX-y))) ||
    ((y < 0) && (x < ((MagickOffsetType) MAGICK_SSIZE_MIN-y)))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsOffsetOverflow(const MagickOffsetType x,
  const MagickOffsetType y)
{
  if (IsOffsetSumOverflow(x,y) != MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static inline ssize_t RandomX(RandomInfo *random_info,const size_t columns)
{
  double
    x;

  x=(double) columns*GetPseudoRandomValue(random_info);
  return(CastDoubleToSsizeT(x));
}

static inline ssize_t RandomY(RandomInfo *random_info,const size_t rows)
{
  double
    y;

  y=(double) rows*GetPseudoRandomValue(random_info);
  return(CastDoubleToSsizeT(y));
}

static inline MagickModulo VirtualPixelModulo(const ssize_t offset,
  const size_t extent)
{
  MagickModulo
    modulo;

  modulo.quotient=offset;
  modulo.remainder=0;
  if (extent != 0)
    {
      modulo.quotient=offset/((ssize_t) extent);
      modulo.remainder=offset % ((ssize_t) extent);
    }
  if ((modulo.remainder != 0) && ((offset ^ ((ssize_t) extent)) < 0))
    {
      modulo.quotient-=1;
      modulo.remainder+=((ssize_t) extent);
    }
  return(modulo);
}

static inline MagickBooleanType IsRegionInsideCache(const CacheInfo *cache_info,
  const ssize_t x,const ssize_t y,const size_t columns,const size_t rows)
{
  return(((x >= 0) &&
    ((x+(ssize_t) columns-1) < (ssize_t) cache_info->columns) &&
    (y >= 0) &&
    ((y+(ssize_t) rows-1) < (ssize_t) cache_info->rows)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsOffsetOutsideCache(
  const CacheInfo *cache_info,const ssize_t x_offset,const ssize_t y_offset,
  const MagickSizeType length)
{
  return((((x_offset < 0) || (x_offset >= (ssize_t) cache_info->columns)) ||
    ((y_offset < 0) || (y_offset >= (ssize_t) cache_info->rows)) ||
    (length == 0)) ? MagickTrue : MagickFalse);
}

typedef struct _VirtualPixelRows
{
  const Image
    *image;

  VirtualPixelMethod
    virtual_pixel_method;

  ssize_t
    x,
    y;

  size_t
    columns,
    rows;

  CacheInfo
    *cache_info;

  NexusInfo
    *virtual_nexus;

  Quantum
    *virtual_pixel;

  void
    *virtual_metacontent;

  ExceptionInfo
    *exception;
} VirtualPixelRows;

typedef struct _VirtualPixelOffsets
{
  ssize_t
    x_offset,
    y_offset;
} VirtualPixelOffsets;

typedef struct _VirtualPixelTargets
{
  MagickSizeType
    *length;

  Quantum
    *magick_restrict *q;

  unsigned char
    *magick_restrict *s;
} VirtualPixelTargets;

static MagickBooleanType TransferVirtualPixel(
  const VirtualPixelRows *virtual_rows,const VirtualPixelOffsets *offsets,
  const VirtualPixelTargets *targets,MagickBooleanType *broke)
{
  const Image
    *const image = virtual_rows->image;

  const VirtualPixelMethod
    virtual_pixel_method = virtual_rows->virtual_pixel_method;

  const ssize_t
    x_offset = offsets->x_offset,
    y_offset = offsets->y_offset;

  CacheInfo
    *const cache_info = virtual_rows->cache_info;

  NexusInfo
    *const virtual_nexus = virtual_rows->virtual_nexus;

  Quantum
    *const virtual_pixel = virtual_rows->virtual_pixel;

  void
    *const virtual_metacontent = virtual_rows->virtual_metacontent;

  ExceptionInfo
    *const exception = virtual_rows->exception;

  MagickSizeType
    *const length = targets->length;

  Quantum
    *magick_restrict *const q = targets->q;

  unsigned char
    *magick_restrict *const s = targets->s;

  const Quantum
    *magick_restrict p;

  const void
    *magick_restrict r;

  (*broke)=MagickFalse;
  if (IsOffsetOutsideCache(cache_info,x_offset,y_offset,(*length)) !=
      MagickFalse)
    {
      MagickModulo
        x_modulo,
        y_modulo;

      /*
        Transfer a single pixel.
      */
      (*length)=(MagickSizeType) 1;
      switch (virtual_pixel_method)
      {
        case EdgeVirtualPixelMethod:
        default:
        {
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            EdgeX(x_offset,cache_info->columns),
            EdgeY(y_offset,cache_info->rows),1UL,1UL,virtual_nexus,
            exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case RandomVirtualPixelMethod:
        {
          if (cache_info->random_info == (RandomInfo *) NULL)
            cache_info->random_info=AcquireRandomInfo();
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            RandomX(cache_info->random_info,cache_info->columns),
            RandomY(cache_info->random_info,cache_info->rows),1UL,1UL,
            virtual_nexus,exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case DitherVirtualPixelMethod:
        {
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            DitherX(x_offset,cache_info->columns),
            DitherY(y_offset,cache_info->rows),1UL,1UL,virtual_nexus,
            exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case TileVirtualPixelMethod:
        {
          x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
          y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
            exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case MirrorVirtualPixelMethod:
        {
          x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
          if ((x_modulo.quotient & 0x01) == 1L)
            x_modulo.remainder=(ssize_t) cache_info->columns-
              x_modulo.remainder-1L;
          y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
          if ((y_modulo.quotient & 0x01) == 1L)
            y_modulo.remainder=(ssize_t) cache_info->rows-
              y_modulo.remainder-1L;
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
            exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case HorizontalTileEdgeVirtualPixelMethod:
        {
          x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            x_modulo.remainder,EdgeY(y_offset,cache_info->rows),1UL,1UL,
            virtual_nexus,exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case VerticalTileEdgeVirtualPixelMethod:
        {
          y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            EdgeX(x_offset,cache_info->columns),y_modulo.remainder,1UL,1UL,
            virtual_nexus,exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case BackgroundVirtualPixelMethod:
        case BlackVirtualPixelMethod:
        case GrayVirtualPixelMethod:
        case TransparentVirtualPixelMethod:
        case MaskVirtualPixelMethod:
        case WhiteVirtualPixelMethod:
        {
          p=virtual_pixel;
          r=virtual_metacontent;
          break;
        }
        case CheckerTileVirtualPixelMethod:
        {
          x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
          y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
          if (((x_modulo.quotient ^ y_modulo.quotient) & 0x01) != 0L)
            {
              p=virtual_pixel;
              r=virtual_metacontent;
              break;
            }
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
            exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case HorizontalTileVirtualPixelMethod:
        {
          if ((y_offset < 0) || (y_offset >= (ssize_t) cache_info->rows))
            {
              p=virtual_pixel;
              r=virtual_metacontent;
              break;
            }
          x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
          y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
            exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
        case VerticalTileVirtualPixelMethod:
        {
          if ((x_offset < 0) || (x_offset >= (ssize_t) cache_info->columns))
            {
              p=virtual_pixel;
              r=virtual_metacontent;
              break;
            }
          x_modulo=VirtualPixelModulo(x_offset,cache_info->columns);
          y_modulo=VirtualPixelModulo(y_offset,cache_info->rows);
          p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,
            x_modulo.remainder,y_modulo.remainder,1UL,1UL,virtual_nexus,
            exception);
          r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
          break;
        }
      }
      if (p == (const Quantum *) NULL)
        {
          (*broke)=MagickTrue;
          return(MagickTrue);
        }
      (void) memcpy((*q),p,(size_t) (cache_info->number_channels*(*length)*
        sizeof(*p)));
      (*q)+=(ptrdiff_t) cache_info->number_channels;
      if (((*s) != (void *) NULL) && (r != (const void *) NULL))
        {
          (void) memcpy((*s),r,(size_t) cache_info->metacontent_extent);
          (*s)+=(ptrdiff_t) cache_info->metacontent_extent;
        }
      return(MagickFalse);
    }
  return(MagickTrue);
}

static ssize_t GetVirtualPixelRows(const VirtualPixelRows *virtual_rows,
  Quantum *magick_restrict q,unsigned char *magick_restrict s)
{
  const Image
    *const image = virtual_rows->image;

  const VirtualPixelMethod
    virtual_pixel_method = virtual_rows->virtual_pixel_method;

  const ssize_t
    x = virtual_rows->x,
    y = virtual_rows->y;

  const size_t
    columns = virtual_rows->columns,
    rows = virtual_rows->rows;

  CacheInfo
    *const cache_info = virtual_rows->cache_info;

  NexusInfo
    *const virtual_nexus = virtual_rows->virtual_nexus;

  ExceptionInfo
    *const exception = virtual_rows->exception;

  const Quantum
    *magick_restrict p;

  const void
    *magick_restrict r;

  MagickSizeType
    length;

  ssize_t
    u,
    v;

  const VirtualPixelTargets
    targets = { &length, &q, &s };

  MagickBooleanType
    broke = MagickFalse;

  for (v=0; v < (ssize_t) rows; v++)
  {
    ssize_t
      y_offset;

    y_offset=y+v;
    if ((virtual_pixel_method == EdgeVirtualPixelMethod) ||
        (virtual_pixel_method == UndefinedVirtualPixelMethod))
      y_offset=EdgeY(y_offset,cache_info->rows);
    for (u=0; u < (ssize_t) columns; u+=(ssize_t) length)
    {
      ssize_t
        x_offset;

      x_offset=x+u;
      length=(MagickSizeType) MagickMin((ssize_t) cache_info->columns-
        x_offset,(ssize_t) columns-u);
      {
        const VirtualPixelOffsets
          offsets = { x_offset, y_offset };

        if (TransferVirtualPixel(virtual_rows,&offsets,&targets,&broke) ==
            MagickFalse)
          continue;
      }
      if (broke != MagickFalse)
        break;
      /*
        Transfer a run of pixels.
      */
      p=GetVirtualPixelCacheNexus(image,virtual_pixel_method,x_offset,y_offset,
        (size_t) length,1UL,virtual_nexus,exception);
      if (p == (const Quantum *) NULL)
        break;
      r=GetVirtualMetacontentFromNexus(cache_info,virtual_nexus);
      (void) memcpy(q,p,(size_t) (cache_info->number_channels*length*
        sizeof(*p)));
      q+=(ptrdiff_t) (cache_info->number_channels*length);
      if ((r != (void *) NULL) && (s != (const void *) NULL))
        {
          (void) memcpy(s,r,(size_t) length);
          s+=(ptrdiff_t) (length*cache_info->metacontent_extent);
        }
    }
    if (u < (ssize_t) columns)
      break;
  }
  return(v);
}

static const Quantum *ReadCacheRegionPixels(
  CacheInfo *magick_restrict cache_info,NexusInfo *magick_restrict nexus_info,
  Quantum *magick_restrict pixels,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  /*
    Pixel request is inside cache extents.
  */
  if (nexus_info->authentic_pixel_cache != MagickFalse)
    return(pixels);
  status=ReadPixelCachePixels(cache_info,nexus_info,exception);
  if (status == MagickFalse)
    return((const Quantum *) NULL);
  if (cache_info->metacontent_extent != 0)
    {
      status=ReadPixelCacheMetacontent(cache_info,nexus_info,exception);
      if (status == MagickFalse)
        return((const Quantum *) NULL);
    }
  return(pixels);
}

#endif
