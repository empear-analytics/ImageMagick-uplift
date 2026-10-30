/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  RRRR   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                  R   R  E      SS       I       ZZ  E                       %
%                  RRRR   EEE     SSS     I     ZZZ   EEE                     %
%                  R R    E         SS    I    ZZ     E                       %
%                  R  R   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                   %
%                                                                             %
%                      SSSSS   CCCC   AAA   L      EEEEE                      %
%                      SS     C      A   A  L      E                          %
%                       SSS   C      AAAAA  L      EEE                        %
%                         SS  C      A   A  L      E                          %
%                      SSSSS   CCCC  A   A  LLLLL  EEEEE                      %
%                                                                             %
%                                                                             %
%                        MagickCore Image Scale Method                        %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
#include "MagickCore/accelerate-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/profile-private.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resize-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#if defined(MAGICKCORE_LQR_DELEGATE)
#include <lqr.h>
#endif

/*
  Typedef declarations.
*/
typedef struct _ScaleImagePair
{
  const Image
    *image,
    *scale_image;
} ScaleImagePair;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S c a l e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ScaleImage() changes the size of an image to the given dimensions.
%
%  The format of the ScaleImage method is:
%
%      Image *ScaleImage(const Image *image,const size_t columns,
%        const size_t rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o columns: the number of columns in the scaled image.
%
%    o rows: the number of rows in the scaled image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _ScaleImportPixel
{
  const Image
    *image;

  const Quantum
    *p;

  double
    alpha;

  ssize_t
    x;
} ScaleImportPixel;

static void ImportScalePixel(const ScaleImportPixel *pixel,double *x_vector)
{
  const Image
    *const image = pixel->image;

  const Quantum
    *const p = pixel->p;

  const double
    alpha = pixel->alpha;

  const ssize_t
    x = pixel->x;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & BlendPixelTrait) == 0)
      {
        x_vector[x*(ssize_t) GetPixelChannels(image)+i]=(double) p[i];
        continue;
      }
    x_vector[x*(ssize_t) GetPixelChannels(image)+i]=alpha*(double) p[i];
  }
}

static void ImportScaleScanline(const Image *image,const Quantum *p,
  double alpha,double *x_vector)
{
  ScaleImportPixel
    pixel;

  ssize_t
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelWriteMask(image,p) <= (QuantumRange/2))
      {
        p+=(ptrdiff_t) GetPixelChannels(image);
        continue;
      }
    if (image->alpha_trait != UndefinedPixelTrait)
      alpha=QuantumScale*(double) GetPixelAlpha(image,p);
    pixel.image=image;
    pixel.p=p;
    pixel.alpha=alpha;
    pixel.x=x;
    ImportScalePixel(&pixel,x_vector);
    p+=(ptrdiff_t) GetPixelChannels(image);
  }
}

typedef struct _ScaleExportPixel
{
  const Image
    *image,
    *scale_image;

  const double
    *scanline;

  double
    alpha;

  ssize_t
    x;
} ScaleExportPixel;

static void ExportScalePixel(const ScaleExportPixel *pixel,Quantum *q)
{
  const Image
    *const image = pixel->image,
    *const scale_image = pixel->scale_image;

  const double
    *const scanline = pixel->scanline;

  const double
    alpha = pixel->alpha;

  const ssize_t
    x = pixel->x;

  PixelTrait
    scale_traits;

  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    scale_traits=GetPixelChannelTraits(scale_image,channel);
    if ((traits == UndefinedPixelTrait) ||
        (scale_traits == UndefinedPixelTrait))
      continue;
    if ((traits & BlendPixelTrait) == 0)
      {
        SetPixelChannel(scale_image,channel,ClampToQuantum(
          scanline[x*(ssize_t) GetPixelChannels(image)+i]),q);
        continue;
      }
    SetPixelChannel(scale_image,channel,ClampToQuantum(alpha*scanline[
      x*(ssize_t) GetPixelChannels(image)+i]),q);
  }
}

static void ExportScaleScanline(const ScaleImagePair *pair,
  const double *scanline,double alpha,Quantum *q)
{
  const Image
    *const image = pair->image,
    *const scale_image = pair->scale_image;

  ScaleExportPixel
    pixel;

  ssize_t
    x;

  for (x=0; x < (ssize_t) scale_image->columns; x++)
  {
    if (GetPixelWriteMask(scale_image,q) <= (QuantumRange/2))
      {
        q+=(ptrdiff_t) GetPixelChannels(scale_image);
        continue;
      }
    if (image->alpha_trait != UndefinedPixelTrait)
      {
        alpha=QuantumScale*scanline[x*(ssize_t) GetPixelChannels(image)+
          GetPixelChannelOffset(image,AlphaPixelChannel)];
        alpha=MagickSafeReciprocal(alpha);
      }
    pixel.image=image;
    pixel.scale_image=scale_image;
    pixel.scanline=scanline;
    pixel.alpha=alpha;
    pixel.x=x;
    ExportScalePixel(&pixel,q);
    q+=(ptrdiff_t) GetPixelChannels(scale_image);
  }
}

static void ResetScalePixel(const Image *image,double *pixel)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    pixel[i]=0.0;
}

typedef struct _ScaleColumn
{
  const Image
    *image;

  const double
    *scanline;

  double
    *scale_scanline;

  ssize_t
    x;
} ScaleColumn;

typedef struct _ScaleColumnTargets
{
  double
    (*pixel)[CompositePixelChannel];

  ssize_t
    *t;

  PointInfo
    *scale,
    *span;

  MagickBooleanType
    *next_column;
} ScaleColumnTargets;

static void AccumulateScaleColumn(const ScaleColumn *column,
  const ScaleColumnTargets *targets)
{
  const Image
    *const image = column->image;

  const double
    *const scanline = column->scanline;

  double
    *const scale_scanline = column->scale_scanline;

  const ssize_t
    x = column->x;

  double
    (*const pixel)[CompositePixelChannel] = targets->pixel;

  ssize_t
    *const t = targets->t;

  PointInfo
    *const scale = targets->scale,
    *const span = targets->span;

  MagickBooleanType
    *const next_column = targets->next_column;

  ssize_t
    i;

  if (*next_column != MagickFalse)
    {
      ResetScalePixel(image,*pixel);
      (*t)++;
    }
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if (traits == UndefinedPixelTrait)
      continue;
    (*pixel)[i]+=(*span).x*scanline[x*(ssize_t) GetPixelChannels(image)+i];
    scale_scanline[(*t)*(ssize_t) GetPixelChannels(image)+i]=(*pixel)[i];
  }
  (*scale).x-=(*span).x;
  (*span).x=1.0;
  *next_column=MagickTrue;
}

typedef struct _ScalePartialColumn
{
  const Image
    *image;

  const PointInfo
    *scale;

  const double
    *scanline;

  ssize_t
    x;
} ScalePartialColumn;

typedef struct _ScalePartialTargets
{
  double
    (*pixel)[CompositePixelChannel];

  MagickBooleanType
    *next_column;

  ssize_t
    *t;

  PointInfo
    *span;
} ScalePartialTargets;

static void AccumulatePartialScaleColumn(const ScalePartialColumn *partial,
  const ScalePartialTargets *targets)
{
  const Image
    *const image = partial->image;

  const PointInfo
    *const scale = partial->scale;

  const double
    *const scanline = partial->scanline;

  const ssize_t
    x = partial->x;

  double
    (*const pixel)[CompositePixelChannel] = targets->pixel;

  MagickBooleanType
    *const next_column = targets->next_column;

  ssize_t
    *const t = targets->t;

  PointInfo
    *const span = targets->span;

  ssize_t
    i;

  if ((*scale).x > 0)
    {
      if (*next_column != MagickFalse)
        {
          ResetScalePixel(image,*pixel);
          *next_column=MagickFalse;
          (*t)++;
        }
      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
        (*pixel)[i]+=(*scale).x*scanline[x*(ssize_t)
          GetPixelChannels(image)+i];
      (*span).x-=(*scale).x;
    }
}

typedef struct _ScaleColumnTail
{
  const Image
    *image;

  const PointInfo
    *span;

  const double
    *scanline;

  ssize_t
    x;
} ScaleColumnTail;

static void AccumulateScaleColumnTail(const ScaleColumnTail *tail,
  double *pixel)
{
  const Image
    *const image = tail->image;

  const PointInfo
    *const span = tail->span;

  const double
    *const scanline = tail->scanline;

  const ssize_t
    x = tail->x;

  ssize_t
    i;

  if ((*span).x > 0)
    {
      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
        pixel[i]+=(*span).x*
          scanline[(x-1)*(ssize_t) GetPixelChannels(image)+i];
    }
}

typedef struct _ScaleColumnLast
{
  const Image
    *image,
    *scale_image;

  MagickBooleanType
    next_column;

  ssize_t
    t;
} ScaleColumnLast;

static void StoreLastScaleColumn(const ScaleColumnLast *last,
  const double *pixel,double *scale_scanline)
{
  const Image
    *const image = last->image,
    *const scale_image = last->scale_image;

  const MagickBooleanType
    next_column = last->next_column;

  const ssize_t
    t = last->t;

  ssize_t
    i;

  if ((next_column == MagickFalse) && (t < (ssize_t) scale_image->columns))
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      scale_scanline[t*(ssize_t) GetPixelChannels(image)+i]=pixel[i];
}

static void ScaleScanlineX(const Image *image,const Image *scale_image,
  const double *scanline,double *scale_scanline)
{
  double
    pixel[CompositePixelChannel];

  MagickBooleanType
    next_column;

  PointInfo
    scale,
    span;

  ScaleColumnLast
    last;

  ScaleColumnTail
    tail;

  ssize_t
    t,
    x;

  ResetScalePixel(image,pixel);
  next_column=MagickFalse;
  span.x=1.0;
  t=0;
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    const ScalePartialTargets
      partial_targets = { &pixel, &next_column, &t, &span };

    ScalePartialColumn
      partial;

    scale.x=(double) scale_image->columns/(double) image->columns;
    while (scale.x >= span.x)
    {
      const ScaleColumnTargets
        targets = { &pixel, &t, &scale, &span, &next_column };

      ScaleColumn
        column;

      column.image=image;
      column.scanline=scanline;
      column.scale_scanline=scale_scanline;
      column.x=x;
      AccumulateScaleColumn(&column,&targets);
    }
    partial.image=image;
    partial.scale=(&scale);
    partial.scanline=scanline;
    partial.x=x;
    AccumulatePartialScaleColumn(&partial,&partial_targets);
  }
  tail.image=image;
  tail.span=(&span);
  tail.scanline=scanline;
  tail.x=x;
  AccumulateScaleColumnTail(&tail,pixel);
  last.image=image;
  last.scale_image=scale_image;
  last.next_column=next_column;
  last.t=t;
  StoreLastScaleColumn(&last,pixel,scale_scanline);
}

static void AccumulateScaleScanline(const Image *image,const PointInfo scale,
  const double *x_vector,double *y_vector)
{
  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      y_vector[x*(ssize_t) GetPixelChannels(image)+i]+=scale.y*
        x_vector[x*(ssize_t) GetPixelChannels(image)+i];
}

static void BlendScaleScanline(const Image *image,const PointInfo span,
  const double *x_vector,double *y_vector,double *scanline)
{
  double
    pixel[CompositePixelChannel];

  ssize_t
    i,
    x;

  for (x=0; x < (ssize_t) image->columns; x++)
  {
    for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
    {
      pixel[i]=y_vector[x*(ssize_t) GetPixelChannels(image)+i]+span.y*
        x_vector[x*(ssize_t) GetPixelChannels(image)+i];
      scanline[x*(ssize_t) GetPixelChannels(image)+i]=pixel[i];
      y_vector[x*(ssize_t) GetPixelChannels(image)+i]=0.0;
    }
  }
}

static inline MagickBooleanType IsScaleBufferMissing(const double *scanline,
  const double *scale_scanline,const double *x_vector,const double *y_vector)
{
  return(((scanline == (double *) NULL) || (scale_scanline == (double *) NULL) ||
    (x_vector == (double *) NULL) || (y_vector == (double *) NULL)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsScaleRowPending(const Image *image,
  const MagickBooleanType next_row,const ssize_t number_rows)
{
  return(((next_row != MagickFalse) &&
    (number_rows < (ssize_t) image->rows)) ? MagickTrue : MagickFalse);
}

typedef struct _ScaleRows
{
  const Image
    *image;

  Image
    *scale_image;

  CacheView
    *image_view;

  double
    alpha,
    *x_vector,
    *y_vector;

  ExceptionInfo
    *exception;
} ScaleRows;

typedef struct _ScaleRowTargets
{
  ssize_t
    *n;

  MagickBooleanType
    *status;

  ssize_t
    *number_rows;

  PointInfo
    *span,
    *scale;

  MagickBooleanType
    *next_row;
} ScaleRowTargets;

static void AccumulateScaleRows(const ScaleRows *rows,
  const ScaleRowTargets *targets)
{
  const Image
    *const image = rows->image;

  Image
    *const scale_image = rows->scale_image;

  CacheView
    *const image_view = rows->image_view;

  const double
    alpha = rows->alpha;

  double
    *const x_vector = rows->x_vector,
    *const y_vector = rows->y_vector;

  ExceptionInfo
    *const exception = rows->exception;

  ssize_t
    *const n = targets->n;

  MagickBooleanType
    *const status = targets->status;

  ssize_t
    *const number_rows = targets->number_rows;

  PointInfo
    *const span = targets->span,
    *const scale = targets->scale;

  MagickBooleanType
    *const next_row = targets->next_row;

  const Quantum
    *magick_restrict p;

  while ((*scale).y < (*span).y)
  {
    if (IsScaleRowPending(image,*next_row,*number_rows) != MagickFalse)
      {
        /*
          Read a new scanline.
        */
        p=GetCacheViewVirtualPixels(image_view,0,(*n)++,image->columns,1,
          exception);
        if (p == (const Quantum *) NULL)
          {
            *status=MagickFalse;
            break;
          }
        ImportScaleScanline(image,p,alpha,x_vector);
        (*number_rows)++;
      }
    AccumulateScaleScanline(image,*scale,x_vector,y_vector);
    (*span).y-=(*scale).y;
    (*scale).y=(double) scale_image->rows/(double) image->rows;
    *next_row=MagickTrue;
  }
}

typedef struct _ScaleRead
{
  const Image
    *image;

  CacheView
    *image_view;

  double
    alpha,
    *x_vector;

  ExceptionInfo
    *exception;
} ScaleRead;

typedef struct _ScaleReadTargets
{
  ssize_t
    *n;

  MagickBooleanType
    *status;

  ssize_t
    *number_rows;

  MagickBooleanType
    *next_row;
} ScaleReadTargets;

static MagickBooleanType ReadPendingScaleRow(const ScaleRead *pending,
  const ScaleReadTargets *targets)
{
  const Image
    *const image = pending->image;

  CacheView
    *const image_view = pending->image_view;

  const double
    alpha = pending->alpha;

  double
    *const x_vector = pending->x_vector;

  ExceptionInfo
    *const exception = pending->exception;

  ssize_t
    *const n = targets->n;

  MagickBooleanType
    *const status = targets->status;

  ssize_t
    *const number_rows = targets->number_rows;

  MagickBooleanType
    *const next_row = targets->next_row;

  const Quantum
    *magick_restrict p;

  if (IsScaleRowPending(image,*next_row,*number_rows) != MagickFalse)
    {
      /*
        Read a new scanline.
      */
      p=GetCacheViewVirtualPixels(image_view,0,(*n)++,image->columns,1,
        exception);
      if (p == (const Quantum *) NULL)
        {
          *status=MagickFalse;
          return(MagickFalse);
        }
      ImportScaleScanline(image,p,alpha,x_vector);
      (*number_rows)++;
      *next_row=MagickFalse;
    }
  return(MagickTrue);
}

static void RestartScaleRow(const Image *image,Image *scale_image,
  PointInfo *scale,MagickBooleanType *next_row)
{
  if ((*scale).y <= 0)
    {
      (*scale).y=(double) scale_image->rows/(double) image->rows;
      *next_row=MagickTrue;
    }
}

typedef struct _ScaleTransfer
{
  const Image
    *image;

  Image
    *scale_image;

  double
    alpha,
    *scale_scanline,
    *scanline;
} ScaleTransfer;

static void TransferScaleScanline(const ScaleTransfer *transfer,
  Quantum *magick_restrict q)
{
  const Image
    *const image = transfer->image;

  Image
    *const scale_image = transfer->scale_image;

  const double
    alpha = transfer->alpha;

  double
    *const scale_scanline = transfer->scale_scanline,
    *const scanline = transfer->scanline;

  const ScaleImagePair
    pair = { image, scale_image };

  if (scale_image->columns == image->columns)
    {
      /*
        Transfer scanline to scaled image.
      */
      ExportScaleScanline(&pair,scanline,alpha,q);
    }
  else
    {
      /*
        Scale X direction.
      */
      ScaleScanlineX(image,scale_image,scanline,scale_scanline);
    /*
      Transfer scanline to scaled image.
    */
    ExportScaleScanline(&pair,scale_scanline,alpha,q);
  }
}

MagickExport Image *ScaleImage(const Image *image,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
#define ScaleImageTag  "Scale/Image"

  CacheView
    *image_view,
    *scale_view;

  double
    alpha,
    *scale_scanline,
    *scanline,
    *x_vector,
    *y_vector;

  Image
    *scale_image;

  MagickBooleanType
    next_row,
    proceed,
    status;

  PointInfo
    scale,
    span;

  ssize_t
    n,
    number_rows,
    y;

  /*
    Initialize scaled image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((columns == 0) || (rows == 0))
    ThrowImageException(ImageError,"NegativeOrZeroImageSize");
  if ((columns == image->columns) && (rows == image->rows))
    return(CloneImage(image,0,0,MagickTrue,exception));
  scale_image=CloneImage(image,columns,rows,MagickTrue,exception);
  if (scale_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(scale_image,DirectClass,exception) == MagickFalse)
    {
      scale_image=DestroyImage(scale_image);
      return((Image *) NULL);
    }
  /*
    Allocate memory.
  */
  x_vector=(double *) AcquireQuantumMemory((size_t) image->columns,
    MaxPixelChannels*sizeof(*x_vector));
  scanline=x_vector;
  if (image->rows != scale_image->rows)
    scanline=(double *) AcquireQuantumMemory((size_t) image->columns,
      MaxPixelChannels*sizeof(*scanline));
  scale_scanline=(double *) AcquireQuantumMemory((size_t) scale_image->columns,
    MaxPixelChannels*sizeof(*scale_scanline));
  y_vector=(double *) AcquireQuantumMemory((size_t) image->columns,
    MaxPixelChannels*sizeof(*y_vector));
  if (IsScaleBufferMissing(scanline,scale_scanline,x_vector,y_vector) !=
      MagickFalse)
    {
      if ((image->rows != scale_image->rows) && (scanline != (double *) NULL))
        scanline=(double *) RelinquishMagickMemory(scanline);
      if (scale_scanline != (double *) NULL)
        scale_scanline=(double *) RelinquishMagickMemory(scale_scanline);
      if (x_vector != (double *) NULL)
        x_vector=(double *) RelinquishMagickMemory(x_vector);
      if (y_vector != (double *) NULL)
        y_vector=(double *) RelinquishMagickMemory(y_vector);
      scale_image=DestroyImage(scale_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Scale image.
  */
  number_rows=0;
  next_row=MagickTrue;
  span.y=1.0;
  scale.y=(double) scale_image->rows/(double) image->rows;
  (void) memset(y_vector,0,(size_t) MaxPixelChannels*image->columns*
    sizeof(*y_vector));
  n=0;
  status=MagickTrue;
  image_view=AcquireVirtualCacheView(image,exception);
  scale_view=AcquireAuthenticCacheView(scale_image,exception);
  for (y=0; y < (ssize_t) scale_image->rows; y++)
  {
    const Quantum
      *magick_restrict p;

    Quantum
      *magick_restrict q;

    ScaleTransfer
      transfer;

    if (status == MagickFalse)
      break;
    q=QueueCacheViewAuthenticPixels(scale_view,0,y,scale_image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        break;
      }
    alpha=1.0;
    if (scale_image->rows == image->rows)
      {
        /*
          Read a new scanline.
        */
        p=GetCacheViewVirtualPixels(image_view,0,n++,image->columns,1,
          exception);
        if (p == (const Quantum *) NULL)
          {
            status=MagickFalse;
            break;
          }
        ImportScaleScanline(image,p,alpha,x_vector);
      }
    else
      {
        const ScaleRowTargets
          targets = { &n, &status, &number_rows, &span, &scale, &next_row };

        const ScaleReadTargets
          pending_targets = { &n, &status, &number_rows, &next_row };

        ScaleRead
          pending;

        ScaleRows
          scale_rows;

        /*
          Scale Y direction.
        */
        scale_rows.image=image;
        scale_rows.scale_image=scale_image;
        scale_rows.image_view=image_view;
        scale_rows.alpha=alpha;
        scale_rows.x_vector=x_vector;
        scale_rows.y_vector=y_vector;
        scale_rows.exception=exception;
        AccumulateScaleRows(&scale_rows,&targets);
        pending.image=image;
        pending.image_view=image_view;
        pending.alpha=alpha;
        pending.x_vector=x_vector;
        pending.exception=exception;
        if (ReadPendingScaleRow(&pending,&pending_targets) == MagickFalse)
          break;
        BlendScaleScanline(image,span,x_vector,y_vector,scanline);
        scale.y-=span.y;
        RestartScaleRow(image,scale_image,&scale,&next_row);
        span.y=1.0;
      }
    transfer.image=image;
    transfer.scale_image=scale_image;
    transfer.alpha=alpha;
    transfer.scale_scanline=scale_scanline;
    transfer.scanline=scanline;
    TransferScaleScanline(&transfer,q);
    if (SyncCacheViewAuthenticPixels(scale_view,exception) == MagickFalse)
      {
        status=MagickFalse;
        break;
      }
    proceed=SetImageProgress(image,ScaleImageTag,(MagickOffsetType) y,
      image->rows);
    if (proceed == MagickFalse)
      {
        status=MagickFalse;
        break;
      }
  }
  scale_view=DestroyCacheView(scale_view);
  image_view=DestroyCacheView(image_view);
  /*
    Free allocated memory.
  */
  y_vector=(double *) RelinquishMagickMemory(y_vector);
  scale_scanline=(double *) RelinquishMagickMemory(scale_scanline);
  if (scale_image->rows != image->rows)
    scanline=(double *) RelinquishMagickMemory(scanline);
  x_vector=(double *) RelinquishMagickMemory(x_vector);
  scale_image->type=image->type;
  if (status == MagickFalse)
    scale_image=DestroyImage(scale_image);
  return(scale_image);
}
