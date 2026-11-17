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

  MagickCore private image evaluate and polynomial methods.
*/
#ifndef MAGICKCORE_STATISTIC_EVALUATE_PRIVATE_H
#define MAGICKCORE_STATISTIC_EVALUATE_PRIVATE_H

#include "MagickCore/cache-view.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/resource_.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _PixelChannels
{
  double
    channel[MaxPixelChannels];
} PixelChannels;

static inline PixelChannels **DestroyPixelTLS(const Image *images,
  PixelChannels **pixels)
{
  ssize_t
    i;

  size_t
    rows;

  assert(pixels != (PixelChannels **) NULL);
  rows=MagickMax(GetImageListLength(images),(size_t)
    GetMagickResourceLimit(ThreadResource));
  for (i=0; i < (ssize_t) rows; i++)
    if (pixels[i] != (PixelChannels *) NULL)
      pixels[i]=(PixelChannels *) RelinquishMagickMemory(pixels[i]);
  pixels=(PixelChannels **) RelinquishMagickMemory(pixels);
  return(pixels);
}

static inline void ClearPixelChannelsRow(PixelChannels **pixels,const ssize_t i,
  const size_t columns)
{
  ssize_t
    j;

  for (j=0; j < (ssize_t) columns; j++)
  {
    ssize_t
      k;

    for (k=0; k < MaxPixelChannels; k++)
      pixels[i][j].channel[k]=0.0;
  }
}

static inline size_t GetWidestEvaluateColumns(const Image *images,size_t columns)
{
  const Image
    *next;

  for (next=images; next != (Image *) NULL; next=next->next)
    columns=MagickMax(next->columns,columns);
  return(columns);
}

static inline PixelChannels **AcquirePixelTLS(const Image *images)
{
  PixelChannels
    **pixels;

  ssize_t
    i;

  size_t
    columns,
    number_images,
    rows;

  number_images=GetImageListLength(images);
  rows=MagickMax(number_images,(size_t) GetMagickResourceLimit(ThreadResource));
  pixels=(PixelChannels **) AcquireQuantumMemory(rows,sizeof(*pixels));
  if (pixels == (PixelChannels **) NULL)
    return((PixelChannels **) NULL);
  (void) memset(pixels,0,rows*sizeof(*pixels));
  columns=GetWidestEvaluateColumns(images,MagickMax(number_images,
    MaxPixelChannels));
  for (i=0; i < (ssize_t) rows; i++)
  {
    pixels[i]=(PixelChannels *) AcquireQuantumMemory(columns,sizeof(**pixels));
    if (pixels[i] == (PixelChannels *) NULL)
      return(DestroyPixelTLS(images,pixels));
    ClearPixelChannelsRow(pixels,i,columns);
  }
  return(pixels);
}

static inline Image *AcquireImageCanvas(const Image *images,ExceptionInfo *exception)
{
  const Image
    *p,
    *q;

  size_t
    columns,
    rows;

  q=images;
  columns=images->columns;
  rows=images->rows;
  for (p=images; p != (Image *) NULL; p=p->next)
  {
    if (p->number_channels > q->number_channels)
      q=p;
    if (p->columns > columns)
      columns=p->columns;
    if (p->rows > rows)
      rows=p->rows;
  }
  return(CloneImage(q,columns,rows,MagickTrue,exception));
}

static inline void ClearPolynomialPixels(const Image *image,
  PixelChannels *polynomial_pixel)
{
  ssize_t
    i,
    j;

  for (j=0; j < (ssize_t) image->columns; j++)
    for (i=0; i < MaxPixelChannels; i++)
      polynomial_pixel[j].channel[i]=0.0;
}

static inline MagickBooleanType AcquireDirectImageCanvas(const Image *images,
  Image **image,ExceptionInfo *exception)
{
  *image=AcquireImageCanvas(images,exception);
  if (*image == (Image *) NULL)
    return(MagickFalse);
  if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
    {
      *image=DestroyImage(*image);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static inline MagickBooleanType SetProgress(const Image *images,
  const Image *image,const char *tag,const MagickOffsetType progress)
{
  MagickBooleanType
    proceed,
    status;

  status=MagickTrue;
  proceed=SetImageProgress(images,tag,progress,image->rows);
  if (proceed == MagickFalse)
    status=MagickFalse;
  return(status);
}

typedef struct _EvaluateCanvasTargets
{
  Image
    **image;

  size_t
    *number_images;

  PixelChannels
    **magick_restrict *evaluate_pixels;
} EvaluateCanvasTargets;

static inline MagickBooleanType AcquireEvaluateCanvas(const Image *images,
  const EvaluateCanvasTargets *targets,ExceptionInfo *exception)
{
  Image
    **const image = targets->image;

  size_t
    *const number_images = targets->number_images;

  PixelChannels
    **magick_restrict *const evaluate_pixels = targets->evaluate_pixels;

  if (AcquireDirectImageCanvas(images,image,exception) == MagickFalse)
    return(MagickFalse);
  *number_images=GetImageListLength(images);
  *evaluate_pixels=AcquirePixelTLS(images);
  if (*evaluate_pixels == (PixelChannels **) NULL)
    {
      *image=DestroyImage(*image);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",images->filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static inline void SyncEvaluateView(CacheView *evaluate_view,
  ExceptionInfo *exception,MagickBooleanType *status)
{
  if (SyncCacheViewAuthenticPixels(evaluate_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
