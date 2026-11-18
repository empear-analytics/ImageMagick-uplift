/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                         DDDD   RRRR    AAA   W   W                          %
%                         D   D  R   R  A   A  W   W                          %
%                         D   D  RRRR   AAAAA  W W W                          %
%                         D   D  R R    A   A  WW WW                          %
%                         DDDD   R  R   A   A  W   W                          %
%                                                                             %
%                   AAA   FFFFF  FFFFF  IIIII  N   N  EEEEE                   %
%                  A   A  F      F        I    NN  N  E                       %
%                  AAAAA  FFF    FFF      I    N N N  EEE                     %
%                  A   A  F      F        I    N  NN  E                       %
%                  A   A  F      F      IIIII  N   N  EEEEE                   %
%                                                                             %
%                                                                             %
%                   MagickCore Affine Image Drawing Methods                   %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1998                                   %
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
% Bill Radcliffe of Corbis (www.corbis.com) contributed the polygon
% rendering code based on Paul Heckbert's "Concave Polygon Scan Conversion",
% Graphics Gems, 1990.  Leonard Rosenthal and David Harr of Appligent
% (www.appligent.com) contributed the dash pattern, linecap stroking
% algorithm, and minor rendering improvements.
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/constitute-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     D r a w A f f i n e I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawAffineImage() composites the source over the destination image as
%  dictated by the affine transform.
%
%  The format of the DrawAffineImage method is:
%
%      MagickBooleanType DrawAffineImage(Image *image,const Image *source,
%        const AffineMatrix *affine,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o source: the source image.
%
%    o affine: the affine transform.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static void RaiseEdgeStart(const double intercept,SegmentInfo *inverse_edge)
{
  double
    x;

  x=intercept;
  if (x > (*inverse_edge).x1)
    (*inverse_edge).x1=x;
}

static void LowerEdgeEnd(const double intercept,SegmentInfo *inverse_edge)
{
  double
    x;

  x=intercept;
  if (x < (*inverse_edge).x2)
    (*inverse_edge).x2=x;
}

static inline MagickBooleanType IsAffineEdgeOutside(const double z,
  const size_t extent)
{
  return(((z < 0.0) || ((size_t) floor(z+0.5) >= extent)) ? MagickTrue :
    MagickFalse);
}

static SegmentInfo AffineEdge(const Image *image,const AffineMatrix *affine,
  const double y,const SegmentInfo *edge)
{
  double
    intercept,
    z;

  SegmentInfo
    inverse_edge;

  /*
    Determine left and right edges.
  */
  inverse_edge.x1=edge->x1;
  inverse_edge.y1=edge->y1;
  inverse_edge.x2=edge->x2;
  inverse_edge.y2=edge->y2;
  z=affine->ry*y+affine->tx;
  if (affine->sx >= MagickEpsilon)
    {
      intercept=(-z/affine->sx);
      RaiseEdgeStart(intercept,&inverse_edge);
      intercept=(-z+(double) image->columns)/affine->sx;
      LowerEdgeEnd(intercept,&inverse_edge);
    }
  else
    if (affine->sx < -MagickEpsilon)
      {
        intercept=(-z+(double) image->columns)/affine->sx;
        RaiseEdgeStart(intercept,&inverse_edge);
        intercept=(-z/affine->sx);
        LowerEdgeEnd(intercept,&inverse_edge);
      }
    else
      if (IsAffineEdgeOutside(z,image->columns) != MagickFalse)
        {
          inverse_edge.x2=edge->x1;
          return(inverse_edge);
        }
  /*
    Determine top and bottom edges.
  */
  z=affine->sy*y+affine->ty;
  if (affine->rx >= MagickEpsilon)
    {
      intercept=(-z/affine->rx);
      RaiseEdgeStart(intercept,&inverse_edge);
      intercept=(-z+(double) image->rows)/affine->rx;
      LowerEdgeEnd(intercept,&inverse_edge);
    }
  else
    if (affine->rx < -MagickEpsilon)
      {
        intercept=(-z+(double) image->rows)/affine->rx;
        RaiseEdgeStart(intercept,&inverse_edge);
        intercept=(-z/affine->rx);
        LowerEdgeEnd(intercept,&inverse_edge);
      }
    else
      if (IsAffineEdgeOutside(z,image->rows) != MagickFalse)
        {
          inverse_edge.x2=edge->x2;
          return(inverse_edge);
        }
  return(inverse_edge);
}

static AffineMatrix InverseAffineMatrix(const AffineMatrix *affine)
{
  AffineMatrix
    inverse_affine;

  double
    determinant;

  determinant=MagickSafeReciprocal(affine->sx*affine->sy-affine->rx*
    affine->ry);
  inverse_affine.sx=determinant*affine->sy;
  inverse_affine.rx=determinant*(-affine->rx);
  inverse_affine.ry=determinant*(-affine->ry);
  inverse_affine.sy=determinant*affine->sx;
  inverse_affine.tx=(-affine->tx)*inverse_affine.sx-affine->ty*
    inverse_affine.ry;
  inverse_affine.ty=(-affine->tx)*inverse_affine.rx-affine->ty*
    inverse_affine.sy;
  return(inverse_affine);
}

typedef struct _AffineRowInput
{
  const Image
    *source;

  CacheView
    *source_view;

  Image
    *image;

  AffineMatrix
    inverse_affine;

  PixelInfo
    zero;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} AffineRowInput;

static void DrawAffineRowPixels(const AffineRowInput *input,
  const SegmentInfo inverse_edge,Quantum *magick_restrict q,
  MagickBooleanType *status)
{
  const Image
    *source = input->source;

  CacheView
    *source_view = input->source_view;

  Image
    *image = input->image;

  const AffineMatrix
    inverse_affine = input->inverse_affine;

  const PixelInfo
    zero = input->zero;

  const ssize_t
    y = input->y;

  ExceptionInfo
    *exception = input->exception;

  PixelInfo
    composite,
    pixel;

  PointInfo
    point;

  ssize_t
    x;

  pixel=zero;
  composite=zero;
  for (x=CastDoubleToSsizeT(ceil(inverse_edge.x1-0.5));
       x <= CastDoubleToSsizeT(floor(inverse_edge.x2+0.5)); x++)
  {
    point.x=(double) x*inverse_affine.sx+y*inverse_affine.ry+
      inverse_affine.tx;
    point.y=(double) x*inverse_affine.rx+y*inverse_affine.sy+
      inverse_affine.ty;
    (*status)=InterpolatePixelInfo(source,source_view,
      UndefinedInterpolatePixel,point.x,point.y,&pixel,exception);
    if ((*status) == MagickFalse)
      break;
    GetPixelInfoPixel(image,q,&composite);
    CompositePixelInfoOver(&pixel,pixel.alpha,&composite,composite.alpha,
      &composite);
    SetPixelViaPixelInfo(image,&composite,q);
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static void GetAffineImageExtent(const Image *source,
  const AffineMatrix *affine,PointInfo *extent)
{
  ssize_t
    i;

  extent[0].x=0.0;
  extent[0].y=0.0;
  extent[1].x=(double) source->columns;
  extent[1].y=0.0;
  extent[2].x=(double) source->columns;
  extent[2].y=(double) source->rows;
  extent[3].x=0.0;
  extent[3].y=(double) source->rows;
  for (i=0; i < 4; i++)
  {
    PointInfo
      point;

    point=extent[i];
    extent[i].x=point.x*affine->sx+point.y*affine->ry+affine->tx;
    extent[i].y=point.x*affine->rx+point.y*affine->sy+affine->ty;
  }
}

static void GetAffineImageBounds(const PointInfo *extent,PointInfo *min,
  PointInfo *max)
{
  ssize_t
    i;

  (*min)=extent[0];
  (*max)=extent[0];
  for (i=1; i < 4; i++)
  {
    if ((*min).x > extent[i].x)
      (*min).x=extent[i].x;
    if ((*min).y > extent[i].y)
      (*min).y=extent[i].y;
    if ((*max).x < extent[i].x)
      (*max).x=extent[i].x;
    if ((*max).y < extent[i].y)
      (*max).y=extent[i].y;
  }
}

typedef struct _AffineImageRowInput
{
  const Image
    *source;

  CacheView
    *source_view;

  Image
    *image;

  CacheView
    *image_view;

  AffineMatrix
    inverse_affine;

  SegmentInfo
    edge;

  PixelInfo
    zero;

  ExceptionInfo
    *exception;
} AffineImageRowInput;

static void DrawAffineImageRow(const AffineImageRowInput *row,const ssize_t y,
  MagickBooleanType *status)
{
  const Image
    *source = row->source;

  CacheView
    *source_view = row->source_view;

  Image
    *image = row->image;

  CacheView
    *image_view = row->image_view;

  const AffineMatrix
    inverse_affine = row->inverse_affine;

  const SegmentInfo
    edge = row->edge;

  const PixelInfo
    zero = row->zero;

  ExceptionInfo
    *exception = row->exception;

  const AffineRowInput
    input = { source, source_view, image, inverse_affine, zero, y,
      exception };

  Quantum
    *magick_restrict q;

  SegmentInfo
    inverse_edge;

  if ((*status) == MagickFalse)
    return;
  inverse_edge=AffineEdge(source,&inverse_affine,(double) y,&edge);
  if (inverse_edge.x2 < inverse_edge.x1)
    return;
  if (inverse_edge.x1 < 0.0)
    inverse_edge.x1=0.0;
  if (inverse_edge.x2 > ((double) image->columns-1.0))
    inverse_edge.x2=(double) image->columns-1.0;
  q=GetCacheViewAuthenticPixels(image_view,CastDoubleToSsizeT(
    ceil(inverse_edge.x1-0.5)),y,(size_t) CastDoubleToSsizeT(floor(
    inverse_edge.x2+0.5)-ceil(inverse_edge.x1-0.5)+1),1,exception);
  if (q == (Quantum *) NULL)
    return;
  DrawAffineRowPixels(&input,inverse_edge,q,status);
  if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
    (*status)=MagickFalse;
}

MagickExport MagickBooleanType DrawAffineImage(Image *image,
  const Image *source,const AffineMatrix *affine,ExceptionInfo *exception)
{
  AffineMatrix
    inverse_affine;

  CacheView
    *image_view,
    *source_view;

  MagickBooleanType
    status;

  PixelInfo
    zero;

  PointInfo
    extent[4],
    min,
    max;

  SegmentInfo
    edge;

  ssize_t
    start,
    stop,
    y;

  /*
    Determine bounding box.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(source != (const Image *) NULL);
  assert(source->signature == MagickCoreSignature);
  assert(affine != (AffineMatrix *) NULL);
  GetAffineImageExtent(source,affine,extent);
  GetAffineImageBounds(extent,&min,&max);
  /*
    Affine transform image.
  */
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  status=MagickTrue;
  edge.x1=min.x;
  edge.y1=min.y;
  edge.x2=max.x;
  edge.y2=max.y;
  inverse_affine=InverseAffineMatrix(affine);
  if (edge.y1 < 0.0)
    edge.y1=0.0;
  if (edge.y2 > ((double) image->rows-1.0))
    edge.y2=(double) image->rows-1.0;
  GetPixelInfo(image,&zero);
  start=CastDoubleToSsizeT(ceil(edge.y1-0.5));
  stop=CastDoubleToSsizeT(floor(edge.y2+0.5));
  source_view=AcquireVirtualCacheView(source,exception);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(source,image,(size_t) (stop-start),2)
#endif
  for (y=start; y <= stop; y++)
  {
    const AffineImageRowInput
      row = { source, source_view, image, image_view, inverse_affine, edge,
        zero, exception };

    DrawAffineImageRow(&row,y,&status);
  }
  source_view=DestroyCacheView(source_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}
