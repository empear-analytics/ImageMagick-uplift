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
%        PPPP   RRRR   IIIII  M   M  IIIII  TTTTT  IIIII  V   V  EEEEE        %
%        P   P  R   R    I    MM MM    I      T      I    V   V  E            %
%        PPPP   RRRR     I    M M M    I      T      I    V   V  EEE          %
%        P      R R      I    M   M    I      T      I     V V   E            %
%        P      R  R   IIIII  M   M  IIIII    T    IIIII    V    EEEEE        %
%                                                                             %
%                                                                             %
%                    MagickCore Primitive Drawing Methods                     %
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
#include "MagickCore/draw-core-private.h"
#include "MagickCore/draw-primitive-convert-private.h"
#include "MagickCore/draw-primitive-path-private.h"
#include "MagickCore/draw-primitive-fill-private.h"
#include "MagickCore/draw-primitive-render-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P r i m i t i v e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPrimitive() draws a primitive (line, rectangle, ellipse) on the image.
%
%  The format of the DrawPrimitive method is:
%
%      MagickBooleanType DrawPrimitive(Image *image,const DrawInfo *draw_info,
%        PrimitiveInfo *primitive_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o primitive_info: Specifies a pointer to a PrimitiveInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType DrawPrimitive(Image *image,
  const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  DrawPrimitiveContext
    context;

  MagickStatusType
    status;

  ssize_t
    x;

  ssize_t
    y;

  if (draw_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "  begin draw-primitive");
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "    affine: %g,%g,%g,%g,%g,%g",draw_info->affine.sx,
        draw_info->affine.rx,draw_info->affine.ry,draw_info->affine.sy,
        draw_info->affine.tx,draw_info->affine.ty);
    }
  status=MagickTrue;
  if (IsGrayImageWithColor(image,draw_info) != MagickFalse)
    status&=(MagickStatusType) SetImageColorspace(image,sRGBColorspace,
      exception);
  if (draw_info->compliance == SVGCompliance)
    {
      status&=(MagickStatusType) SetImageMask(image,WritePixelMask,
        draw_info->clipping_mask,exception);
      status&=(MagickStatusType) SetImageMask(image,CompositePixelMask,
        draw_info->composite_mask,exception);
    }
  x=CastDoubleToSsizeT(ceil(primitive_info->point.x-0.5));
  y=CastDoubleToSsizeT(ceil(primitive_info->point.y-0.5));
  image_view=AcquireAuthenticCacheView(image,exception);
  context.image=image;
  context.draw_info=draw_info;
  context.primitive_info=primitive_info;
  context.image_view=image_view;
  context.x=x;
  context.y=y;
  context.exception=exception;
  status=DrawPrimitiveByType(&context,status);
  image_view=DestroyCacheView(image_view);
  if (draw_info->compliance == SVGCompliance)
    {
      status&=(MagickStatusType) SetImageMask(image,WritePixelMask,
        (Image *) NULL,exception);
      status&=(MagickStatusType) SetImageMask(image,CompositePixelMask,
        (Image *) NULL,exception);
    }
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"  end draw-primitive");
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D r a w S t r o k e P o l y g o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawStrokePolygon() draws a stroked polygon (line, rectangle, ellipse) on
%  the image while respecting the line cap and join attributes.
%
%  The format of the DrawStrokePolygon method is:
%
%      MagickBooleanType DrawStrokePolygon(Image *image,
%        const DrawInfo *draw_info,const PrimitiveInfo *primitive_info)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o primitive_info: Specifies a pointer to a PrimitiveInfo structure.
%
%
*/

static MagickBooleanType DrawRoundLinecap(Image *image,
  const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  PrimitiveInfo
    linecap[5];

  ssize_t
    i;

  if (primitive_info->coordinates < 1)
    return(MagickFalse);
  for (i=0; i < 4; i++)
    linecap[i]=(*primitive_info);
  linecap[0].coordinates=4;
  linecap[1].point.x+=2.0*MagickEpsilon;
  linecap[2].point.x+=2.0*MagickEpsilon;
  linecap[2].point.y+=2.0*MagickEpsilon;
  linecap[3].point.y+=2.0*MagickEpsilon;
  linecap[4].primitive=UndefinedPrimitive;
  return(DrawPolygonPrimitive(image,draw_info,linecap,exception));
}

typedef struct _StrokeSubpathPaint
{
  Image
    *image;

  const DrawInfo
    *draw_info,
    *clone_info;

  ExceptionInfo
    *exception;
} StrokeSubpathPaint;

static MagickStatusType DrawStrokeSubpathPolygon(
  const StrokeSubpathPaint *paint,const PrimitiveInfo *p)
{
  Image
    *const image = paint->image;

  const DrawInfo
    *const draw_info = paint->draw_info,
    *const clone_info = paint->clone_info;

  ExceptionInfo
    *const exception = paint->exception;

  MagickStatusType
    status;

  PrimitiveInfo
    *stroke_polygon;

  stroke_polygon=TraceStrokePolygon(draw_info,p,exception);
  if (stroke_polygon == (PrimitiveInfo *) NULL)
    return(0);
  status=(MagickStatusType) DrawPolygonPrimitive(image,clone_info,
    stroke_polygon,exception);
  stroke_polygon=(PrimitiveInfo *) RelinquishMagickMemory(stroke_polygon);
  return(status);
}

static MagickStatusType DrawSubpathRoundLinecaps(Image *image,
  const DrawInfo *draw_info,const PrimitiveInfo *p,ExceptionInfo *exception)
{
  MagickBooleanType
    closed_path;

  MagickStatusType
    status;

  const PrimitiveInfo
    *q;

  status=MagickTrue;
  q=p+p->coordinates-1;
  closed_path=p->closed_subpath;
  if ((draw_info->linecap == RoundCap) && (closed_path == MagickFalse))
    {
      status&=(MagickStatusType) DrawRoundLinecap(image,draw_info,p,
        exception);
      status&=(MagickStatusType) DrawRoundLinecap(image,draw_info,q,
        exception);
    }
  return(status);
}

static DrawInfo *CloneStrokeFillInfo(const DrawInfo *draw_info,
  ExceptionInfo *exception)
{
  DrawInfo
    *clone_info;

  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  clone_info->fill=draw_info->stroke;
  if (clone_info->fill_pattern != (Image *) NULL)
    clone_info->fill_pattern=DestroyImage(clone_info->fill_pattern);
  if (clone_info->stroke_pattern != (Image *) NULL)
    clone_info->fill_pattern=CloneImage(clone_info->stroke_pattern,0,0,
      MagickTrue,exception);
  clone_info->stroke.alpha=(MagickRealType) TransparentAlpha;
  clone_info->stroke_width=0.0;
  clone_info->fill_rule=NonZeroRule;
  return(clone_info);
}

static MagickBooleanType DrawStrokePolygon(Image *image,
  const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  DrawInfo
    *clone_info;

  MagickStatusType
    status;

  const PrimitiveInfo
    *p;

  StrokeSubpathPaint
    paint;

  /*
    Draw stroked polygon.
  */
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),
      "    begin draw-stroke-polygon");
  clone_info=CloneStrokeFillInfo(draw_info,exception);
  status=MagickTrue;
  paint.image=image;
  paint.draw_info=draw_info;
  paint.clone_info=clone_info;
  paint.exception=exception;
  for (p=primitive_info; p->primitive != UndefinedPrimitive; p+=(ptrdiff_t) p->coordinates)
  {
    if (p->coordinates == 1)
      continue;
    status&=DrawStrokeSubpathPolygon(&paint,p);
    if (status == 0)
      break;
    status&=DrawSubpathRoundLinecaps(image,draw_info,p,exception);
  }
  clone_info=DestroyDrawInfo(clone_info);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),
      "    end draw-stroke-polygon");
  return(status != 0 ? MagickTrue : MagickFalse);
}

static ssize_t FindLinecapStartVertex(const PrimitiveInfo *primitive_info,
  const size_t number_vertices,double *dx,double *dy)
{
  ssize_t
    i;

  for (i=1; i < (ssize_t) number_vertices; i++)
  {
    (*dx)=primitive_info[0].point.x-primitive_info[i].point.x;
    (*dy)=primitive_info[0].point.y-primitive_info[i].point.y;
    if ((fabs((double) (*dx)) >= MagickEpsilon) ||
        (fabs((double) (*dy)) >= MagickEpsilon))
      break;
  }
  return(i);
}

static MagickBooleanType TraceSquareLinecap(PrimitiveInfo *primitive_info,
  const size_t number_vertices,const double offset)
{
  double
    distance;

  double
    dx,
    dy;

  ssize_t
    i;

  ssize_t
    j;

  dx=0.0;
  dy=0.0;
  i=FindLinecapStartVertex(primitive_info,number_vertices,&dx,&dy);
  if (i == (ssize_t) number_vertices)
    i=(ssize_t) number_vertices-1L;
  distance=hypot((double) dx,(double) dy);
  primitive_info[0].point.x=(double) (primitive_info[i].point.x+
    dx*(distance+offset)/distance);
  primitive_info[0].point.y=(double) (primitive_info[i].point.y+
    dy*(distance+offset)/distance);
  for (j=(ssize_t) number_vertices-2; j >= 0;  j--)
  {
    dx=primitive_info[number_vertices-1].point.x-primitive_info[j].point.x;
    dy=primitive_info[number_vertices-1].point.y-primitive_info[j].point.y;
    if ((fabs((double) dx) >= MagickEpsilon) ||
        (fabs((double) dy) >= MagickEpsilon))
      break;
  }
  distance=hypot((double) dx,(double) dy);
  primitive_info[number_vertices-1].point.x=(double) (primitive_info[j].point.x+
    dx*(distance+offset)/distance);
  primitive_info[number_vertices-1].point.y=(double) (primitive_info[j].point.y+
    dy*(distance+offset)/distance);
  return(MagickTrue);
}

static inline MagickBooleanType IsRoundStrokeJoin(const DrawInfo *draw_info,
  const MagickBooleanType closed_path)
{
  return(((draw_info->linejoin == RoundJoin) ||
    ((draw_info->linejoin == MiterJoin) &&
    (closed_path != MagickFalse))) ? MagickTrue : MagickFalse);
}

static void GetEpsilonSlope(const double dx,const double dy,double *slope)
{
  if (dx >= 0.0)
    *slope=dy < 0.0 ? -1.0/MagickEpsilon : 1.0/MagickEpsilon;
  else
    *slope=dy < 0.0 ? 1.0/MagickEpsilon : -1.0/MagickEpsilon;
}

static void GetStrokeSlope(const double dx,const double dy,double *slope,
  double *inverse_slope)
{
  *slope=0.0;
  *inverse_slope=0.0;
  if (fabs(dx) < MagickEpsilon)
    GetEpsilonSlope(dx,dy,slope);
  else
    if (fabs(dy) < MagickEpsilon)
      GetEpsilonSlope(dy,dx,inverse_slope);
    else
      {
        *slope=dy/dx;
        *inverse_slope=(-1.0*MagickSafeReciprocal(*slope));
      }
}

typedef struct _StrokeBoxes
{
  PointInfo
    *box_p,
    *box_q;
} StrokeBoxes;

static ssize_t TraceMiterPointP(const StrokeBoxes *boxes,PointInfo *stroke_p,
  ssize_t p,const double miterlimit)
{
  PointInfo
    *const box_p = boxes->box_p,
    *const box_q = boxes->box_q;

  double
    dot_product;

          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            stroke_p[p++]=box_p[4];
          else
            {
              stroke_p[p++]=box_p[1];
              stroke_p[p++]=box_p[2];
            }
  return(p);
}

static ssize_t TraceMiterPointQ(const StrokeBoxes *boxes,PointInfo *stroke_q,
  ssize_t q,const double miterlimit)
{
  PointInfo
    *const box_p = boxes->box_p,
    *const box_q = boxes->box_q;

  double
    dot_product;

          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            stroke_q[q++]=box_q[4];
          else
            {
              stroke_q[q++]=box_q[1];
              stroke_q[q++]=box_q[2];
            }
  return(q);
}

typedef struct _StrokeTargets
{
  PointInfo
    **stroke_p,
    **stroke_q;

  ssize_t
    *p,
    *q;
} StrokeTargets;

static void TraceMiterPoints(const StrokeBoxes *boxes,const double miterlimit,
  const StrokeTargets *targets)
{
  PointInfo
    *const box_p = boxes->box_p,
    *const box_q = boxes->box_q,
    **const stroke_p = targets->stroke_p,
    **const stroke_q = targets->stroke_q;

  double
    dot_product;

  ssize_t
    *const p = targets->p,
    *const q = targets->q;

          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            {
              (*stroke_q)[(*q)++]=box_q[4];
              (*stroke_p)[(*p)++]=box_p[4];
            }
          else
            {
              (*stroke_q)[(*q)++]=box_q[1];
              (*stroke_q)[(*q)++]=box_q[2];
              (*stroke_p)[(*p)++]=box_p[1];
              (*stroke_p)[(*p)++]=box_p[2];
            }
}

typedef struct _StrokeSegment
{
  double
    p,
    q;
} StrokeSegment;

static void WrapThetaQ(StrokeSegment *theta)
{
  if ((*theta).q < (*theta).p)
    (*theta).q+=2.0*MagickPI;
}

static void WrapThetaP(StrokeSegment *theta)
{
  if ((*theta).p < (*theta).q)
    (*theta).p+=2.0*MagickPI;
}

typedef struct _StrokeArc
{
  PointInfo
    center;

  StrokeSegment
    theta;

  double
    mid;

  size_t
    arc_segments;
} StrokeArc;

static void TraceArcPointsQ(const StrokeArc *arc,const StrokeTargets *targets)
{
  const PointInfo
    center = arc->center;

  const StrokeSegment
    theta = arc->theta;

  const double
    mid = arc->mid;

  const size_t
    arc_segments = arc->arc_segments;

  double
    delta_theta;

  PointInfo
    **const stroke_q = targets->stroke_q;

  ssize_t
    j,
    *const q = targets->q;

  for (j=1; j < (ssize_t) arc_segments; j++)
  {
    delta_theta=(double) (j*(theta.q-theta.p)/arc_segments);
    (*stroke_q)[*q].x=(double) (center.x+mid*cos(fmod((double)
      (theta.p+delta_theta),DegreesToRadians(360.0))));
    (*stroke_q)[*q].y=(double) (center.y+mid*sin(fmod((double)
      (theta.p+delta_theta),DegreesToRadians(360.0))));
    (*q)++;
  }
}

static void TraceArcPointsP(const StrokeArc *arc,const StrokeTargets *targets)
{
  const PointInfo
    center = arc->center;

  const StrokeSegment
    theta = arc->theta;

  const double
    mid = arc->mid;

  const size_t
    arc_segments = arc->arc_segments;

  double
    delta_theta;

  PointInfo
    **const stroke_p = targets->stroke_p;

  ssize_t
    j,
    *const p = targets->p;

  for (j=1; j < (ssize_t) arc_segments; j++)
  {
    delta_theta=(double) (j*(theta.q-theta.p)/arc_segments);
    (*stroke_p)[*p].x=(double) (center.x+mid*cos(fmod((double)
      (theta.p+delta_theta),DegreesToRadians(360.0))));
    (*stroke_p)[*p].y=(double) (center.y+mid*sin(fmod((double)
      (theta.p+delta_theta),DegreesToRadians(360.0))));
    (*p)++;
  }
}

typedef struct _StrokeSubpathInput
{
  const DrawInfo
    *draw_info;

  PrimitiveInfo
    *polygon_primitive;

  size_t
    number_vertices;

  MagickBooleanType
    closed_path;
} StrokeSubpathInput;

static MagickBooleanType TraceZeroLengthSubpath(
  const StrokeSubpathInput *subpath,ssize_t *n,PrimitiveInfo **result)
{
  const DrawInfo
    *draw_info = subpath->draw_info;

  PrimitiveInfo
    *polygon_primitive = subpath->polygon_primitive,
    *stroke_polygon;

  const size_t
    number_vertices = subpath->number_vertices;

  const MagickBooleanType
    closed_path = subpath->closed_path;

  if ((*n) == (ssize_t) number_vertices)
    {
      if ((draw_info->linecap != RoundCap) || (closed_path != MagickFalse))
        {
          /*
            Zero length subpath.
          */
          stroke_polygon=(PrimitiveInfo *) AcquireCriticalMemory(
            sizeof(*stroke_polygon));
          stroke_polygon[0]=polygon_primitive[0];
          stroke_polygon[0].coordinates=0;
          polygon_primitive=(PrimitiveInfo *) RelinquishMagickMemory(
            polygon_primitive);
          (*result)=stroke_polygon;
          return(MagickTrue);
        }
      (*n)=(ssize_t) number_vertices-1L;
    }
  return(MagickFalse);
}

static void SetStrokeIntersections(const StrokeBoxes *boxes,
  const StrokeSegment slope)
{
  PointInfo
    *const box_p = boxes->box_p,
    *const box_q = boxes->box_q;

  if (fabs((double) (slope.p-slope.q)) < MagickEpsilon)
    {
      box_p[4]=box_p[1];
      box_q[4]=box_q[1];
    }
  else
    {
      box_p[4].x=(double) ((slope.p*box_p[0].x-box_p[0].y-slope.q*box_p[3].x+
        box_p[3].y)/(slope.p-slope.q));
      box_p[4].y=(double) (slope.p*(box_p[4].x-box_p[0].x)+box_p[0].y);
      box_q[4].x=(double) ((slope.p*box_q[0].x-box_q[0].y-slope.q*box_q[3].x+
        box_q[3].y)/(slope.p-slope.q));
      box_q[4].y=(double) (slope.p*(box_q[4].x-box_q[0].x)+box_q[0].y);
    }
}

typedef struct _StrokeSegmentInput
{
  PrimitiveInfo
    *polygon_primitive;

  ssize_t
    n,
    i;

  PointInfo
    offset;

  StrokeSegment
    dx,
    dy;
} StrokeSegmentInput;

static void SetStrokeSegmentBoxes(const StrokeBoxes *boxes,
  const StrokeSegmentInput *segment)
{
  PointInfo
    *const box_p = boxes->box_p,
    *const box_q = boxes->box_q;

  PrimitiveInfo
    *const polygon_primitive = segment->polygon_primitive;

  const ssize_t
    n = segment->n,
    i = segment->i;

  const PointInfo
    offset = segment->offset;

  const StrokeSegment
    dx = segment->dx,
    dy = segment->dy;

  double
    dot_product;

  dot_product=dy.q*offset.x-dx.q*offset.y;
  if (dot_product > 0.0)
    {
      box_p[2].x=polygon_primitive[n].point.x-offset.x;
      box_p[2].y=polygon_primitive[n].point.y-offset.y;
      box_p[3].x=polygon_primitive[i].point.x-offset.x;
      box_p[3].y=polygon_primitive[i].point.y-offset.y;
      box_q[2].x=polygon_primitive[n].point.x+offset.x;
      box_q[2].y=polygon_primitive[n].point.y+offset.y;
      box_q[3].x=polygon_primitive[i].point.x+offset.x;
      box_q[3].y=polygon_primitive[i].point.y+offset.y;
    }
  else
    {
      box_p[2].x=polygon_primitive[n].point.x+offset.x;
      box_p[2].y=polygon_primitive[n].point.y+offset.y;
      box_p[3].x=polygon_primitive[i].point.x+offset.x;
      box_p[3].y=polygon_primitive[i].point.y+offset.y;
      box_q[2].x=polygon_primitive[n].point.x-offset.x;
      box_q[2].y=polygon_primitive[n].point.y-offset.y;
      box_q[3].x=polygon_primitive[i].point.x-offset.x;
      box_q[3].y=polygon_primitive[i].point.y-offset.y;
    }
}

typedef struct _StrokeFirstInput
{
  PrimitiveInfo
    *polygon_primitive;

  size_t
    number_vertices;
} StrokeFirstInput;

static void FindStrokeFirstSegment(const StrokeFirstInput *first,
  StrokeSegment *dx,StrokeSegment *dy,ssize_t *n)
{
  PrimitiveInfo
    *const polygon_primitive = first->polygon_primitive;

  const size_t
    number_vertices = first->number_vertices;

  (*dx).p=0.0;
  (*dy).p=0.0;
  for ((*n)=1; (*n) < (ssize_t) number_vertices; (*n)++)
  {
    (*dx).p=polygon_primitive[(*n)].point.x-polygon_primitive[0].point.x;
    (*dy).p=polygon_primitive[(*n)].point.y-polygon_primitive[0].point.y;
    if ((fabs((*dx).p) >= MagickEpsilon) || (fabs((*dy).p) >= MagickEpsilon))
      break;
  }
}

typedef struct _StrokeStartInput
{
  PrimitiveInfo
    *polygon_primitive;

  ssize_t
    n;

  PointInfo
    offset;

  StrokeSegment
    dx,
    dy,
    inverse_slope;
} StrokeStartInput;

static void SetStrokeStartBoxes(const StrokeBoxes *boxes,
  const StrokeStartInput *start)
{
  PointInfo
    *const box_p = boxes->box_p,
    *const box_q = boxes->box_q;

  PrimitiveInfo
    *const polygon_primitive = start->polygon_primitive;

  const ssize_t
    n = start->n;

  const PointInfo
    offset = start->offset;

  const StrokeSegment
    dx = start->dx,
    dy = start->dy,
    inverse_slope = start->inverse_slope;

  if ((dy.p*offset.x-dx.p*offset.y) > 0.0)
    {
      box_p[0].x=polygon_primitive[0].point.x-offset.x;
      box_p[0].y=polygon_primitive[0].point.y-offset.x*inverse_slope.p;
      box_p[1].x=polygon_primitive[n].point.x-offset.x;
      box_p[1].y=polygon_primitive[n].point.y-offset.x*inverse_slope.p;
      box_q[0].x=polygon_primitive[0].point.x+offset.x;
      box_q[0].y=polygon_primitive[0].point.y+offset.x*inverse_slope.p;
      box_q[1].x=polygon_primitive[n].point.x+offset.x;
      box_q[1].y=polygon_primitive[n].point.y+offset.x*inverse_slope.p;
    }
  else
    {
      box_p[0].x=polygon_primitive[0].point.x+offset.x;
      box_p[0].y=polygon_primitive[0].point.y+offset.y;
      box_p[1].x=polygon_primitive[n].point.x+offset.x;
      box_p[1].y=polygon_primitive[n].point.y+offset.y;
      box_q[0].x=polygon_primitive[0].point.x-offset.x;
      box_q[0].y=polygon_primitive[0].point.y-offset.y;
      box_q[1].x=polygon_primitive[n].point.x-offset.x;
      box_q[1].y=polygon_primitive[n].point.y-offset.y;
    }
}

typedef struct _StrokePolygonInput
{
  PrimitiveInfo
    *polygon_primitive;

  PointInfo
    *stroke_p,
    *stroke_q;

  ssize_t
    p,
    q;

  MagickBooleanType
    closed_path;
} StrokePolygonInput;

static void AssembleStrokePolygon(const StrokePolygonInput *assembly,
  PrimitiveInfo *stroke_polygon)
{
  PrimitiveInfo
    *const polygon_primitive = assembly->polygon_primitive;

  PointInfo
    *const stroke_p = assembly->stroke_p,
    *const stroke_q = assembly->stroke_q;

  const ssize_t
    p = assembly->p,
    q = assembly->q;

  const MagickBooleanType
    closed_path = assembly->closed_path;

  ssize_t
    i;

  for (i=0; i < (ssize_t) p; i++)
  {
    stroke_polygon[i]=polygon_primitive[0];
    stroke_polygon[i].point=stroke_p[i];
  }
  if (closed_path != MagickFalse)
    {
      stroke_polygon[i]=polygon_primitive[0];
      stroke_polygon[i].point=stroke_polygon[0].point;
      i++;
    }
  for ( ; i < (ssize_t) (p+q+closed_path); i++)
  {
    stroke_polygon[i]=polygon_primitive[0];
    stroke_polygon[i].point=stroke_q[p+q+closed_path-(i+1)];
  }
  if (closed_path != MagickFalse)
    {
      stroke_polygon[i]=polygon_primitive[0];
      stroke_polygon[i].point=stroke_polygon[p+closed_path].point;
      i++;
    }
  stroke_polygon[i]=polygon_primitive[0];
  stroke_polygon[i].point=stroke_polygon[0].point;
  i++;
  stroke_polygon[i].primitive=UndefinedPrimitive;
  stroke_polygon[0].coordinates=(size_t) (p+q+2*closed_path+1);
}

static PrimitiveInfo *TraceStrokePolygon(const DrawInfo *draw_info,
  const PrimitiveInfo *primitive_info,ExceptionInfo *exception)
{
#define MaxStrokePad  (6*BezierQuantum+360)
#define CheckPathExtent(pad_p,pad_q) \
{   \
  if ((pad_p) > MaxBezierCoordinates) \
    stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p); \
  else \
    if ((p+(ptrdiff_t) (pad_p)) >= (ssize_t) extent_p) \
      { \
        if (~extent_p < (pad_p)) \
          stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p); \
        else \
          { \
            extent_p+=(pad_p); \
            stroke_p=(PointInfo *) ResizeQuantumMemory(stroke_p,extent_p+ \
              MaxStrokePad,sizeof(*stroke_p)); \
          } \
      } \
  if ((pad_q) > MaxBezierCoordinates) \
    stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q); \
  else \
    if ((q+(ptrdiff_t) (pad_q)) >= (ssize_t) extent_q) \
      { \
        if (~extent_q < (pad_q)) \
          stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q); \
        else \
          { \
            extent_q+=(pad_q); \
            stroke_q=(PointInfo *) ResizeQuantumMemory(stroke_q,extent_q+ \
              MaxStrokePad,sizeof(*stroke_q)); \
          } \
      } \
  if ((stroke_p == (PointInfo *) NULL) || (stroke_q == (PointInfo *) NULL)) \
    { \
      if (stroke_p != (PointInfo *) NULL) \
        stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p); \
      if (stroke_q != (PointInfo *) NULL) \
        stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q); \
      polygon_primitive=(PrimitiveInfo *) \
        RelinquishMagickMemory(polygon_primitive); \
      (void) ThrowMagickException(exception,GetMagickModule(), \
        ResourceLimitError,"MemoryAllocationFailed","`%s'",""); \
      return((PrimitiveInfo *) NULL); \
    } \
}

  double
    dot_product,
    mid,
    miterlimit;

  MagickBooleanType
    closed_path;

  PointInfo
    box_p[5],
    box_q[5],
    center,
    offset,
    *stroke_p,
    *stroke_q;

  const StrokeBoxes
    boxes = { box_p, box_q };

  PrimitiveInfo
    *polygon_primitive,
    *returned,
    *stroke_polygon;

  ssize_t
    i;

  size_t
    arc_segments,
    extent_p,
    extent_q,
    number_vertices;

  ssize_t
    n,
    p,
    q;

  StrokeSegment
    dx = {0.0, 0.0},
    dy = {0.0, 0.0},
    inverse_slope = {0.0, 0.0},
    slope = {0.0, 0.0},
    theta = {0.0, 0.0};

  const StrokeTargets
    targets = { &stroke_p, &stroke_q, &p, &q };

  StrokeArc
    arc;

  StrokeFirstInput
    first;

  StrokeSegmentInput
    segment;

  /*
    Allocate paths.
  */
  number_vertices=primitive_info->coordinates;
  polygon_primitive=(PrimitiveInfo *) AcquireQuantumMemory((size_t)
    number_vertices+2UL,sizeof(*polygon_primitive));
  if (polygon_primitive == (PrimitiveInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return((PrimitiveInfo *) NULL);
    }
  (void) memcpy(polygon_primitive,primitive_info,(size_t) number_vertices*
    sizeof(*polygon_primitive));
  offset.x=primitive_info[number_vertices-1].point.x-primitive_info[0].point.x;
  offset.y=primitive_info[number_vertices-1].point.y-primitive_info[0].point.y;
  closed_path=(fabs(offset.x) < MagickEpsilon) &&
    (fabs(offset.y) < MagickEpsilon) ? MagickTrue : MagickFalse;
  if (IsRoundStrokeJoin(draw_info,closed_path) != MagickFalse)
    {
      polygon_primitive[number_vertices]=primitive_info[1];
      number_vertices++;
    }
  polygon_primitive[number_vertices].primitive=UndefinedPrimitive;
  /*
    Compute the slope for the first line segment, p.
  */
  closed_path=primitive_info[0].closed_subpath;
  first.polygon_primitive=polygon_primitive;
  first.number_vertices=number_vertices;
  FindStrokeFirstSegment(&first,&dx,&dy,&n);
  {
    const StrokeSubpathInput
      subpath = { draw_info, polygon_primitive, number_vertices, closed_path };

    if (TraceZeroLengthSubpath(&subpath,&n,&returned) != MagickFalse)
      return(returned);
  }
  extent_p=2*number_vertices;
  extent_q=2*number_vertices;
  stroke_p=(PointInfo *) AcquireQuantumMemory((size_t) extent_p+MaxStrokePad,
    sizeof(*stroke_p));
  stroke_q=(PointInfo *) AcquireQuantumMemory((size_t) extent_q+MaxStrokePad,
    sizeof(*stroke_q));
  if ((stroke_p == (PointInfo *) NULL) || (stroke_q == (PointInfo *) NULL))
    {
      stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p);
      stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q);
      polygon_primitive=(PrimitiveInfo *)
        RelinquishMagickMemory(polygon_primitive);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return((PrimitiveInfo *) NULL);
    }
  GetStrokeSlope(dx.p,dy.p,&slope.p,&inverse_slope.p);
  mid=ExpandAffine(&draw_info->affine)*draw_info->stroke_width/2.0;
  miterlimit=(double) (draw_info->miterlimit*draw_info->miterlimit*mid*mid);
  if ((draw_info->linecap == SquareCap) && (closed_path == MagickFalse))
    (void) TraceSquareLinecap(polygon_primitive,number_vertices,mid);
  offset.x=sqrt((double) (mid*mid/(inverse_slope.p*inverse_slope.p+1.0)));
  offset.y=(double) (offset.x*inverse_slope.p);
  {
    const StrokeStartInput
      start = { polygon_primitive, n, offset, dx, dy, inverse_slope };

    SetStrokeStartBoxes(&boxes,&start);
  }
  /*
    Create strokes for the line join attribute: bevel, miter, round.
  */
  p=0;
  q=0;
  stroke_q[p++]=box_q[0];
  stroke_p[q++]=box_p[0];
  for (i=(ssize_t) n+1; i < (ssize_t) number_vertices; i++)
  {
    /*
      Compute the slope for this line segment, q.
    */
    dx.q=polygon_primitive[i].point.x-polygon_primitive[n].point.x;
    dy.q=polygon_primitive[i].point.y-polygon_primitive[n].point.y;
    dot_product=dx.q*dx.q+dy.q*dy.q;
    if (dot_product < 0.25)
      continue;
    GetStrokeSlope(dx.q,dy.q,&slope.q,&inverse_slope.q);
    offset.x=sqrt((double) (mid*mid/(inverse_slope.q*inverse_slope.q+1.0)));
    offset.y=(double) (offset.x*inverse_slope.q);
    segment.polygon_primitive=polygon_primitive;
    segment.n=n;
    segment.i=i;
    segment.offset=offset;
    segment.dx=dx;
    segment.dy=dy;
    SetStrokeSegmentBoxes(&boxes,&segment);
    SetStrokeIntersections(&boxes,slope);
    DisableMSCWarning(4127)
    CheckPathExtent(MaxStrokePad,MaxStrokePad);
    RestoreMSCWarning
    dot_product=dx.q*dy.p-dx.p*dy.q;
    if (dot_product <= 0.0)
      switch (draw_info->linejoin)
      {
        case BevelJoin:
        {
          stroke_q[q++]=box_q[1];
          stroke_q[q++]=box_q[2];
          p=TraceMiterPointP(&boxes,stroke_p,p,miterlimit);
          break;
        }
        case MiterJoin:
        {
          TraceMiterPoints(&boxes,miterlimit,&targets);
          break;
        }
        case RoundJoin:
        {
          p=TraceMiterPointP(&boxes,stroke_p,p,miterlimit);
          center=polygon_primitive[n].point;
          theta.p=atan2(box_q[1].y-center.y,box_q[1].x-center.x);
          theta.q=atan2(box_q[2].y-center.y,box_q[2].x-center.x);
          WrapThetaQ(&theta);
          arc_segments=(size_t) CastDoubleToSsizeT(ceil((double) ((theta.q-
            theta.p)/(2.0*sqrt(MagickSafeReciprocal(mid))))));
          DisableMSCWarning(4127)
          CheckPathExtent(MaxStrokePad,arc_segments+MaxStrokePad);
          RestoreMSCWarning
          stroke_q[q].x=box_q[1].x;
          stroke_q[q].y=box_q[1].y;
          q++;
          arc.center=center;
          arc.theta=theta;
          arc.mid=mid;
          arc.arc_segments=arc_segments;
          TraceArcPointsQ(&arc,&targets);
          stroke_q[q++]=box_q[2];
          break;
        }
        default:
          break;
      }
    else
      switch (draw_info->linejoin)
      {
        case BevelJoin:
        {
          stroke_p[p++]=box_p[1];
          stroke_p[p++]=box_p[2];
          q=TraceMiterPointQ(&boxes,stroke_q,q,miterlimit);
          break;
        }
        case MiterJoin:
        {
          TraceMiterPoints(&boxes,miterlimit,&targets);
          break;
        }
        case RoundJoin:
        {
          q=TraceMiterPointQ(&boxes,stroke_q,q,miterlimit);
          center=polygon_primitive[n].point;
          theta.p=atan2(box_p[1].y-center.y,box_p[1].x-center.x);
          theta.q=atan2(box_p[2].y-center.y,box_p[2].x-center.x);
          WrapThetaP(&theta);
          arc_segments=(size_t) CastDoubleToSsizeT(ceil((double) ((theta.p-
            theta.q)/(2.0*sqrt((double) (MagickSafeReciprocal(mid)))))));
          DisableMSCWarning(4127)
          CheckPathExtent(arc_segments+MaxStrokePad,MaxStrokePad);
          RestoreMSCWarning
          stroke_p[p++]=box_p[1];
          arc.center=center;
          arc.theta=theta;
          arc.mid=mid;
          arc.arc_segments=arc_segments;
          TraceArcPointsP(&arc,&targets);
          stroke_p[p++]=box_p[2];
          break;
        }
        default:
          break;
      }
    slope.p=slope.q;
    inverse_slope.p=inverse_slope.q;
    box_p[0]=box_p[2];
    box_p[1]=box_p[3];
    box_q[0]=box_q[2];
    box_q[1]=box_q[3];
    dx.p=dx.q;
    dy.p=dy.q;
    n=i;
  }
  stroke_p[p++]=box_p[1];
  stroke_q[q++]=box_q[1];
  /*
    Trace stroked polygon.
  */
  stroke_polygon=(PrimitiveInfo *) AcquireQuantumMemory((size_t)
    (p+q+2L),(size_t) (closed_path+2L)*sizeof(*stroke_polygon));
  if (stroke_polygon == (PrimitiveInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p);
      stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q);
      polygon_primitive=(PrimitiveInfo *) RelinquishMagickMemory(
        polygon_primitive);
      return(stroke_polygon);
    }
  {
    const StrokePolygonInput
      assembly = { polygon_primitive, stroke_p, stroke_q, p, q, closed_path };

    AssembleStrokePolygon(&assembly,stroke_polygon);
  }
  stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p);
  stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q);
  polygon_primitive=(PrimitiveInfo *) RelinquishMagickMemory(polygon_primitive);
  return(stroke_polygon);
}
