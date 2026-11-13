/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  V   V  IIIII  SSSSS  U   U   AAA   L                       %
%                  V   V    I    SS     U   U  A   A  L                       %
%                  V   V    I     SSS   U   U  AAAAA  L                       %
%                   V V     I       SS  U   U  A   A  L                       %
%                    V    IIIII  SSSSS   UUU   A   A  LLLLL                   %
%                                                                             %
%               EEEEE  FFFFF  FFFFF  EEEEE   CCCC  TTTTT  SSSSS               %
%               E      F      F      E      C        T    SS                  %
%               EEE    FFF    FFF    EEE    C        T     SSS                %
%               E      F      F      E      C        T       SS               %
%               EEEEE  F      F      EEEEE   CCCC    T    SSSSS               %
%                                                                             %
%                  PPPP   L       AAA   SSSSS  M   M   AAA                    %
%                  P   P  L      A   A  SS     MM MM  A   A                   %
%                  PPPP   L      AAAAA   SSS   M M M  AAAAA                   %
%                  P      L      A   A     SS  M   M  A   A                   %
%                  P      LLLLL  A   A  SSSSS  M   M  A   A                   %
%                                                                             %
%                                                                             %
%                      MagickCore Plasma Fractal Methods                      %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                 October 1996                                %
%                                                                             %
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
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/decorate.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/gem-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/visual-effects.h"

/*
  Typedef declarations.
*/
typedef struct _PlasmaContext
{
  Image
    *image;

  CacheView
    *image_view,
    *u_view,
    *v_view;

  RandomInfo
    *magick_restrict random_info;

  ExceptionInfo
    *exception;
} PlasmaContext;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     P l a s m a I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PlasmaImage() initializes an image with plasma fractal values.  The image
%  must be initialized with a base color and the random number generator
%  seeded before this method is called.
%
%  The format of the PlasmaImage method is:
%
%      MagickBooleanType PlasmaImage(Image *image,const SegmentInfo *segment,
%        size_t attenuate,size_t depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o segment:   Define the region to apply plasma fractals values.
%
%    o attenuate: Define the plasma attenuation factor.
%
%    o depth: Limit the plasma recursion depth.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline Quantum PlasmaPixel(RandomInfo *magick_restrict random_info,
  const double pixel,const double noise)
{
  MagickRealType
    plasma;

  plasma=pixel+noise*GetPseudoRandomValue(random_info)-noise/2.0;
  return(ClampToQuantum(plasma));
}

static inline MagickBooleanType IsPlasmaPixelMissing(const Quantum *u,
  const Quantum *v,const Quantum *q)
{
  return(((u == (const Quantum *) NULL) || (v == (const Quantum *) NULL) ||
    (q == (Quantum *) NULL)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsPlasmaSegmentAtMidpoint(
  const SegmentInfo *magick_restrict segment,const ssize_t x_mid,
  const ssize_t y_mid)
{
  return(((fabs(segment->x1-x_mid) < MagickEpsilon) &&
    (fabs(segment->x2-x_mid) < MagickEpsilon) &&
    (fabs(segment->y1-y_mid) < MagickEpsilon) &&
    (fabs(segment->y2-y_mid) < MagickEpsilon)) ? MagickTrue : MagickFalse);
}

static void SetPlasmaPixelChannels(const Image *image,
  RandomInfo *magick_restrict random_info,const Quantum *magick_restrict u,
  const Quantum *magick_restrict v,Quantum *magick_restrict q,
  const double plasma)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if (traits == UndefinedPixelTrait)
      continue;
    q[i]=PlasmaPixel(random_info,((double) u[i]+(double) v[i])/2.0,plasma);
  }
}

static MagickBooleanType PlasmaImageProxy(const PlasmaContext *,
  const SegmentInfo *magick_restrict,size_t,size_t);

static MagickBooleanType DividePlasmaSegment(const PlasmaContext *context,
  const SegmentInfo *magick_restrict segment,size_t attenuate,size_t depth)
{
  MagickStatusType
    status;

  SegmentInfo
    local_info;

  ssize_t
    x_mid,
    y_mid;

  /*
    Divide the area into quadrants and recurse.
  */
  depth--;
  attenuate++;
  x_mid=CastDoubleToSsizeT(ceil((segment->x1+segment->x2)/2-0.5));
  y_mid=CastDoubleToSsizeT(ceil((segment->y1+segment->y2)/2-0.5));
  local_info=(*segment);
  local_info.x2=(double) x_mid;
  local_info.y2=(double) y_mid;
  status=PlasmaImageProxy(context,&local_info,attenuate,depth);
  local_info=(*segment);
  local_info.y1=(double) y_mid;
  local_info.x2=(double) x_mid;
  status&=(MagickStatusType) PlasmaImageProxy(context,&local_info,attenuate,depth);
  local_info=(*segment);
  local_info.x1=(double) x_mid;
  local_info.y2=(double) y_mid;
  status&=(MagickStatusType) PlasmaImageProxy(context,&local_info,attenuate,depth);
  local_info=(*segment);
  local_info.x1=(double) x_mid;
  local_info.y1=(double) y_mid;
  status&=(MagickStatusType) PlasmaImageProxy(context,&local_info,attenuate,depth);
  return(status == 0 ? MagickFalse : MagickTrue);
}

typedef struct _PlasmaMidpoint
{
  const PlasmaContext
    *context;

  const SegmentInfo
    *segment;

  ssize_t
    x_mid,
    y_mid;

  double
    plasma;
} PlasmaMidpoint;

static MagickBooleanType SetPlasmaMiddlePixel(const PlasmaMidpoint *midpoint,
  MagickStatusType *status)
{
  const PlasmaContext
    *const context = midpoint->context;

  const SegmentInfo
    *const segment = midpoint->segment;

  const ssize_t
    x_mid = midpoint->x_mid,
    y_mid = midpoint->y_mid;

  const double
    plasma = midpoint->plasma;

  const Quantum
    *magick_restrict u,
    *magick_restrict v;

  Quantum
    *magick_restrict q;

  ssize_t
    x,
    y;

  if ((fabs(segment->x1-segment->x2) >= MagickEpsilon) ||
      (fabs(segment->y1-segment->y2) >= MagickEpsilon))
    {
      /*
        Middle pixel.
      */
      x=CastDoubleToSsizeT(ceil(segment->x1-0.5));
      y=CastDoubleToSsizeT(ceil(segment->y1-0.5));
      u=GetCacheViewVirtualPixels(context->u_view,x,y,1,1,context->exception);
      x=CastDoubleToSsizeT(ceil(segment->x2-0.5));
      y=CastDoubleToSsizeT(ceil(segment->y2-0.5));
      v=GetCacheViewVirtualPixels(context->v_view,x,y,1,1,context->exception);
      q=QueueCacheViewAuthenticPixels(context->image_view,x_mid,y_mid,1,1,
        context->exception);
      if (IsPlasmaPixelMissing(u,v,q) != MagickFalse)
        return(MagickFalse);
      SetPlasmaPixelChannels(context->image,context->random_info,u,v,q,plasma);
      *status=SyncCacheViewAuthenticPixels(context->image_view,context->exception);
    }
  return(MagickTrue);
}

typedef enum
{
  PlasmaColumnEdge,
  PlasmaRowEdge
} PlasmaEdgeSide;

static inline MagickBooleanType SetPlasmaEdgePixel(
  const PlasmaMidpoint *midpoint,const PlasmaEdgeSide side,const double edge,
  MagickStatusType *status)
{
  const PlasmaContext
    *const context = midpoint->context;

  const SegmentInfo
    *const segment = midpoint->segment;

  const ssize_t
    x_mid = midpoint->x_mid,
    y_mid = midpoint->y_mid;

  const double
    plasma = midpoint->plasma;

  const Quantum
    *magick_restrict u,
    *magick_restrict v;

  Quantum
    *magick_restrict q;

  ssize_t
    offset;

  offset=CastDoubleToSsizeT(ceil(edge-0.5));
  u=GetCacheViewVirtualPixels(context->u_view,((side == PlasmaColumnEdge) ?
    offset : CastDoubleToSsizeT(ceil(segment->x1-0.5))),((side ==
    PlasmaColumnEdge) ? CastDoubleToSsizeT(ceil(segment->y1-0.5)) : offset),1,1,
    context->exception);
  v=GetCacheViewVirtualPixels(context->v_view,((side == PlasmaColumnEdge) ?
    offset : CastDoubleToSsizeT(ceil(segment->x2-0.5))),((side ==
    PlasmaColumnEdge) ? CastDoubleToSsizeT(ceil(segment->y2-0.5)) : offset),1,1,
    context->exception);
  q=QueueCacheViewAuthenticPixels(context->image_view,((side ==
    PlasmaColumnEdge) ? offset : x_mid),((side == PlasmaColumnEdge) ? y_mid :
    offset),1,1,context->exception);
  if (IsPlasmaPixelMissing(u,v,q) != MagickFalse)
    return(MagickFalse);
  SetPlasmaPixelChannels(context->image,context->random_info,u,v,q,plasma);
  *status=SyncCacheViewAuthenticPixels(context->image_view,context->exception);
  return(MagickTrue);
}

static MagickBooleanType SetPlasmaRowPixels(const PlasmaMidpoint *midpoint,
  MagickStatusType *status)
{
  const SegmentInfo
    *const segment = midpoint->segment;

  const ssize_t
    x_mid = midpoint->x_mid,
    y_mid = midpoint->y_mid;

  if ((fabs(segment->y1-y_mid) >= MagickEpsilon) ||
      (fabs(segment->y2-y_mid) >= MagickEpsilon))
    {
      if ((fabs(segment->x1-x_mid) >= MagickEpsilon) ||
          (fabs(segment->y2-y_mid) >= MagickEpsilon))
        {
          /*
            Bottom pixel.
          */
          if (SetPlasmaEdgePixel(midpoint,PlasmaRowEdge,segment->y2,
              status) == MagickFalse)
            return(MagickFalse);
        }
      /*
        Top pixel.
      */
      if ((fabs(segment->y1-segment->y2) >= MagickEpsilon) &&
          (SetPlasmaEdgePixel(midpoint,PlasmaRowEdge,segment->y1,
              status) == MagickFalse))
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType FinishPlasmaSegment(const PlasmaMidpoint *midpoint,
  MagickStatusType status)
{
  const SegmentInfo
    *const segment = midpoint->segment;

  if (SetPlasmaMiddlePixel(midpoint,&status) == MagickFalse)
    return(MagickTrue);
  if ((fabs(segment->x2-segment->x1) < 3.0) &&
      (fabs(segment->y2-segment->y1) < 3.0))
    return(status == 0 ? MagickFalse : MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType PlasmaImageProxy(const PlasmaContext *context,
  const SegmentInfo *magick_restrict segment,size_t attenuate,size_t depth)
{
  double
    plasma;

  MagickStatusType
    status;

  PlasmaMidpoint
    midpoint;

  ssize_t
    x_mid,
    y_mid;

  if ((fabs(segment->x2-segment->x1) < MagickEpsilon) &&
      (fabs(segment->y2-segment->y1) < MagickEpsilon))
    return(MagickTrue);
  if (depth != 0)
    return(DividePlasmaSegment(context,segment,attenuate,depth));
  x_mid=CastDoubleToSsizeT(ceil((segment->x1+segment->x2)/2-0.5));
  y_mid=CastDoubleToSsizeT(ceil((segment->y1+segment->y2)/2-0.5));
  if (IsPlasmaSegmentAtMidpoint(segment,x_mid,y_mid) != MagickFalse)
    return(MagickFalse);
  /*
    Average pixels and apply plasma.
  */
  status=MagickTrue;
  plasma=(double) QuantumRange/(2.0*attenuate);
  midpoint.context=context;
  midpoint.segment=segment;
  midpoint.x_mid=x_mid;
  midpoint.y_mid=y_mid;
  midpoint.plasma=plasma;
  if ((fabs(segment->x1-x_mid) >= MagickEpsilon) ||
      (fabs(segment->x2-x_mid) >= MagickEpsilon))
    {
      /*
        Left pixel.
      */
      if (SetPlasmaEdgePixel(&midpoint,PlasmaColumnEdge,segment->x1,
          &status) == MagickFalse)
        return(MagickTrue);
      /*
        Right pixel.
      */
      if ((fabs(segment->x1-segment->x2) >= MagickEpsilon) &&
          (SetPlasmaEdgePixel(&midpoint,PlasmaColumnEdge,segment->x2,
              &status) == MagickFalse))
        return(MagickFalse);
    }
  if (SetPlasmaRowPixels(&midpoint,&status) == MagickFalse)
    return(MagickTrue);
  return(FinishPlasmaSegment(&midpoint,status));
}

MagickExport MagickBooleanType PlasmaImage(Image *image,
  const SegmentInfo *segment,size_t attenuate,size_t depth,
  ExceptionInfo *exception)
{
  CacheView
    *image_view,
    *u_view,
    *v_view;

  MagickBooleanType
    status;

  PlasmaContext
    plasma_context;

  RandomInfo
    *random_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  image_view=AcquireAuthenticCacheView(image,exception);
  u_view=AcquireVirtualCacheView(image,exception);
  v_view=AcquireVirtualCacheView(image,exception);
  random_info=AcquireRandomInfo();
  plasma_context.image=image;
  plasma_context.image_view=image_view;
  plasma_context.u_view=u_view;
  plasma_context.v_view=v_view;
  plasma_context.random_info=random_info;
  plasma_context.exception=exception;
  status=PlasmaImageProxy(&plasma_context,segment,attenuate,depth);
  random_info=DestroyRandomInfo(random_info);
  v_view=DestroyCacheView(v_view);
  u_view=DestroyCacheView(u_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}
