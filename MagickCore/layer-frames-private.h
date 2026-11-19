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

  MagickCore private image layer methods.
*/
#ifndef MAGICKCORE_LAYER_FRAMES_PRIVATE_H
#define MAGICKCORE_LAYER_FRAMES_PRIVATE_H

#include "MagickCore/cache.h"
#include "MagickCore/channel.h"
#include "MagickCore/image.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/log.h"
#include "MagickCore/layer-frames-bounds-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     C o m p a r e P i x e l s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ComparePixels() Compare the two pixels and return true if the pixels
%  differ according to the given LayerType comparison method.
%
%  This currently only used internally by CompareImagesBounds(). It is
%  doubtful that this sub-routine will be useful outside this module.
%
%  The format of the ComparePixels method is:
%
%      MagickBooleanType *ComparePixels(const LayerMethod method,
%        const PixelInfo *p,const PixelInfo *q)
%
%  A description of each parameter follows:
%
%    o method: What differences to look for. Must be one of
%              CompareAnyLayer, CompareClearLayer, CompareOverlayLayer.
%
%    o p, q: the pixels to test for appropriate differences.
%
*/

static inline MagickBooleanType ComparePixels(const LayerMethod method,
  const PixelInfo *p,const PixelInfo *q)
{
  double
    o1,
    o2;

  /*
    Any change in pixel values
  */
  if (method == CompareAnyLayer)
    return(IsFuzzyEquivalencePixelInfo(p,q) == MagickFalse ? MagickTrue : MagickFalse);
  o1 = (p->alpha_trait != UndefinedPixelTrait) ? p->alpha : (double)
    OpaqueAlpha;
  o2 = (q->alpha_trait != UndefinedPixelTrait) ? q->alpha : (double)
    OpaqueAlpha;
  /*
    Pixel goes from opaque to transparency.
  */
  if (method == CompareClearLayer)
    return((MagickBooleanType) ( (o1 >= ((double) QuantumRange/2.0)) &&
      (o2 < ((double) QuantumRange/2.0)) ) );
  /*
    Overlay would change first pixel by second.
  */
  if (method == CompareOverlayLayer)
    return(IsOverlayPixelChanged(o2,p,q));
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     C o m p a r e I m a g e B o u n d s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompareImagesBounds() Given two images return the smallest rectangular area
%  by which the two images differ, according to the given 'Compare...' layer
%  method.
%
%  This currently only used internally in this module, but may eventually
%  be used by other modules.
%
%  The format of the CompareImagesBounds method is:
%
%      RectangleInfo *CompareImagesBounds(const LayerMethod method,
%        const Image *alpha_image,const Image *beta_image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o method: What differences to look for. Must be one of CompareAnyLayer,
%      CompareClearLayer, CompareOverlayLayer.
%
%    o alpha_image, beta_image: the two images to compare.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline ssize_t FindDifferingPixel(const DifferingPixelScan *scan,
  const Quantum *p,const Quantum *q,const ssize_t count)
{
  const LayerMethod
    method = scan->method;

  const Image
    *alpha_image = scan->alpha_image,
    *beta_image = scan->beta_image;

  PixelInfo
    *alpha_pixel = scan->alpha_pixel,
    *beta_pixel = scan->beta_pixel;

  ssize_t
    i;

  for (i=0; i < count; i++)
  {
    GetPixelInfoPixel(alpha_image,p,alpha_pixel);
    GetPixelInfoPixel(beta_image,q,beta_pixel);
    if (ComparePixels(method,alpha_pixel,beta_pixel) != MagickFalse)
      break;
    p+=(ptrdiff_t) GetPixelChannels(alpha_image);
    q+=(ptrdiff_t) GetPixelChannels(beta_image);
  }
  return(i);
}

typedef struct _DifferingLineScan
{
  const Image
    *alpha_image,
    *beta_image;

  DifferingPixelScan
    scan;

  ExceptionInfo
    *exception;
} DifferingLineScan;

static inline MagickBooleanType ContinueColumnScan(const DifferingLineScan *line,
  const ssize_t x)
{
  const Quantum
    *p,
    *q;

  ssize_t
    y;

  p=GetVirtualPixels(line->alpha_image,x,0,1,line->alpha_image->rows,
    line->exception);
  q=GetVirtualPixels(line->beta_image,x,0,1,line->beta_image->rows,
    line->exception);
  if (IsEitherRowMissing(p,q) != MagickFalse)
    return(MagickFalse);
  y=FindDifferingPixel(&line->scan,p,q,(ssize_t) line->alpha_image->rows);
  if (y < (ssize_t) line->alpha_image->rows)
    return(MagickFalse);
  return(MagickTrue);
}

static inline ssize_t FindFirstDifferingColumn(const Image *alpha_image,
  const Image *beta_image,const DifferingPixelScan scan,
  ExceptionInfo *exception)
{
  const Quantum
    *p,
    *q;

  ssize_t
    x,
    y;

  for (x=0; x < (ssize_t) alpha_image->columns; x++)
  {
    p=GetVirtualPixels(alpha_image,x,0,1,alpha_image->rows,exception);
    q=GetVirtualPixels(beta_image,x,0,1,beta_image->rows,exception);
    if (IsEitherRowMissing(p,q) != MagickFalse)
      break;
    y=FindDifferingPixel(&scan,p,q,(ssize_t) alpha_image->rows);
    if (y < (ssize_t) alpha_image->rows)
      break;
  }
  return(x);
}

typedef enum
{
  FirstDifferingRow,
  LastDifferingRow
} DifferingRowSide;

static inline ssize_t FindDifferingRow(const DifferingLineScan *line,
  const DifferingRowSide side)
{
  const Image
    *const alpha_image = line->alpha_image,
    *const beta_image = line->beta_image;

  const DifferingPixelScan
    scan = line->scan;

  ExceptionInfo
    *const exception = line->exception;

  const Quantum
    *p,
    *q;

  ssize_t
    x,
    y;

  for (y=((side == FirstDifferingRow) ? 0 : (ssize_t) alpha_image->rows-1);
       ((side == FirstDifferingRow) ? y < (ssize_t) alpha_image->rows : y >= 0);
       ((side == FirstDifferingRow) ? y++ : y--))
  {
    p=GetVirtualPixels(alpha_image,0,y,alpha_image->columns,1,exception);
    q=GetVirtualPixels(beta_image,0,y,beta_image->columns,1,exception);
    if (IsEitherRowMissing(p,q) != MagickFalse)
      break;
    x=FindDifferingPixel(&scan,p,q,(ssize_t) alpha_image->columns);
    if (x < (ssize_t) alpha_image->columns)
      break;
  }
  return(y);
}

static inline RectangleInfo CompareImagesBounds(const Image *alpha_image,
  const Image *beta_image,const LayerMethod method,ExceptionInfo *exception)
{
  PixelInfo
    alpha_pixel,
    beta_pixel;

  const DifferingPixelScan
    scan = { method, alpha_image, beta_image, (&alpha_pixel), (&beta_pixel) };

  const DifferingLineScan
    line = { alpha_image, beta_image, scan, exception };

  RectangleInfo
    bounds;

  ssize_t
    x,
    y;

  /*
    Set bounding box of the differences between images.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      alpha_image->filename);
  GetPixelInfo(alpha_image,&alpha_pixel);
  GetPixelInfo(beta_image,&beta_pixel);
  x=FindFirstDifferingColumn(alpha_image,beta_image,scan,exception);
  if (x >= (ssize_t) alpha_image->columns)
    {
      /*
        Images are identical, return a null image.
      */
      bounds.x=-1;
      bounds.y=-1;
      bounds.width=1;
      bounds.height=1;
      return(bounds);
    }
  bounds.x=x;
  for (x=(ssize_t) alpha_image->columns-1; x >= 0; x--)
    if (ContinueColumnScan(&line,x) == MagickFalse)
      break;
  bounds.width=(size_t) (x-bounds.x+1);
  y=FindDifferingRow(&line,FirstDifferingRow);
  bounds.y=y;
  y=FindDifferingRow(&line,LastDifferingRow);
  bounds.height=(size_t) (y-bounds.y+1);
  return(bounds);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
