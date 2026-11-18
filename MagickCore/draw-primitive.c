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

/*
  Typedef declarations.
*/
typedef struct _EdgeInfo
{
  SegmentInfo
    bounds;

  double
    scanline;

  PointInfo
    *points;

  size_t
    number_points;

  ssize_t
    direction;

  MagickBooleanType
    ghostline;

  size_t
    highwater;
} EdgeInfo;

typedef struct _PolygonInfo
{
  EdgeInfo
    *edges;

  size_t
    number_edges;
} PolygonInfo;

typedef enum
{
  MoveToCode,
  OpenCode,
  GhostlineCode,
  LineToCode,
  EndCode
} PathInfoCode;

typedef struct _PathInfo
{
  PointInfo
    point;

  PathInfoCode
    code;
} PathInfo;

typedef struct _FillAlphaSample
{
  double
    mid;

  MagickBooleanType
    fill;

  ssize_t
    x,
    y;
} FillAlphaSample;

/*
  Define declarations.
*/
#define AntialiasThreshold  (1.0/3.0)
#define MaxBezierCoordinates  67108864

/*
  Forward declarations.
*/
static MagickBooleanType
  DrawStrokePolygon(Image *,const DrawInfo *,const PrimitiveInfo *,
    ExceptionInfo *),
  TraceSquareLinecap(PrimitiveInfo *,const size_t,const double);
static PrimitiveInfo
  *TraceStrokePolygon(const DrawInfo *,const PrimitiveInfo *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C o n v e r t P a t h T o P o l y g o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvertPathToPolygon() converts a path to the more efficient sorted
%  rendering form.
%
%  The format of the ConvertPathToPolygon method is:
%
%      PolygonInfo *ConvertPathToPolygon(const PathInfo *path_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o ConvertPathToPolygon() returns the path in a more efficient sorted
%      rendering form of type PolygonInfo.
%
%    o draw_info: Specifies a pointer to an DrawInfo structure.
%
%    o path_info: Specifies a pointer to an PathInfo structure.
%
%
*/

static PolygonInfo *DestroyPolygonInfo(PolygonInfo *polygon_info)
{
  ssize_t
    i;

  if (polygon_info->edges != (EdgeInfo *) NULL)
    {
      for (i=0; i < (ssize_t) polygon_info->number_edges; i++)
        if (polygon_info->edges[i].points != (PointInfo *) NULL)
          polygon_info->edges[i].points=(PointInfo *)
            RelinquishMagickMemory(polygon_info->edges[i].points);
      polygon_info->edges=(EdgeInfo *) RelinquishMagickMemory(
        polygon_info->edges);
    }
  return((PolygonInfo *) RelinquishMagickMemory(polygon_info));
}
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int DrawCompareEdges(const void *p_edge,const void *q_edge)
{
#define DrawCompareEdge(p,q) \
{ \
  if (((p)-(q)) < 0.0) \
    return(-1); \
  if (((p)-(q)) > 0.0) \
    return(1); \
}

  const PointInfo
    *p,
    *q;

  /*
    Edge sorting for right-handed coordinate system.
  */
  p=((const EdgeInfo *) p_edge)->points;
  q=((const EdgeInfo *) q_edge)->points;
  DrawCompareEdge(p[0].y,q[0].y);
  DrawCompareEdge(p[0].x,q[0].x);
  DrawCompareEdge((p[1].x-p[0].x)*(q[1].y-q[0].y),(p[1].y-p[0].y)*
    (q[1].x-q[0].x));
  DrawCompareEdge(p[1].y,q[1].y);
  DrawCompareEdge(p[1].x,q[1].x);
  return(0);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static void LogPolygonInfo(const PolygonInfo *polygon_info)
{
  EdgeInfo
    *p;

  ssize_t
    i,
    j;

  (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    begin active-edge");
  p=polygon_info->edges;
  for (i=0; i < (ssize_t) polygon_info->number_edges; i++)
  {
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"      edge %.17g:",
      (double) i);
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"      direction: %s",
      p->direction != MagickFalse ? "down" : "up");
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"      ghostline: %s",
      p->ghostline != MagickFalse ? "transparent" : "opaque");
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),
      "      bounds: %g,%g - %g,%g",p->bounds.x1,p->bounds.y1,
      p->bounds.x2,p->bounds.y2);
    for (j=0; j < (ssize_t) p->number_points; j++)
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),"        %g,%g",
        p->points[j].x,p->points[j].y);
    p++;
  }
  (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    end active-edge");
}

static void ReversePoints(PointInfo *points,const size_t number_points)
{
  PointInfo
    point;

  size_t
    i;

  for (i=0; i < (number_points >> 1); i++)
  {
    point=points[i];
    points[i]=points[number_points-(i+1)];
    points[number_points-(i+1)]=point;
  }
}

static inline MagickBooleanType IsDirectionChange(const PointInfo *points,
  const long direction,const long next_direction)
{
  return(((points != (PointInfo *) NULL) && (direction != 0) &&
    (direction != next_direction)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsSubpathStart(const PathInfo *path_info,
  const ssize_t i)
{
  return(((path_info[i].code == MoveToCode) ||
    (path_info[i].code == OpenCode) ||
    (path_info[i].code == GhostlineCode)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType GrowPolygonEdges(PolygonInfo *polygon_info,
  const size_t edge,size_t *number_edges)
{
  if (edge == (*number_edges))
    {
      (*number_edges)<<=1;
      polygon_info->edges=(EdgeInfo *) ResizeQuantumMemory(
        polygon_info->edges,(size_t) (*number_edges),
        sizeof(*polygon_info->edges));
      if (polygon_info->edges == (EdgeInfo *) NULL)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _PolygonEdgeInput
{
  ssize_t
    n;

  long
    direction;

  SegmentInfo
    bounds;
} PolygonEdgeInput;

typedef struct _PolygonEdgeTargets
{
  size_t
    *number_edges;

  PointInfo
    **points;

  MagickBooleanType
    *ghostline;

  size_t
    *edge;
} PolygonEdgeTargets;

static MagickBooleanType FlushSubpathEdge(PolygonInfo *polygon_info,
  const PolygonEdgeInput *input,const PolygonEdgeTargets *targets)
{
  size_t
    *const number_edges = targets->number_edges;

  PointInfo
    **const points = targets->points;

  MagickBooleanType
    *const ghostline = targets->ghostline;

  size_t
    *const edge = targets->edge;

  const ssize_t
    n = input->n;

  const long
    direction = input->direction;

  const SegmentInfo
    bounds = input->bounds;

  if (((*points) != (PointInfo *) NULL) && (n >= 2))
    {
      if (GrowPolygonEdges(polygon_info,(*edge),number_edges) == MagickFalse)
        return(MagickFalse);
      polygon_info->edges[(*edge)].number_points=(size_t) n;
      polygon_info->edges[(*edge)].scanline=(-1.0);
      polygon_info->edges[(*edge)].highwater=0;
      polygon_info->edges[(*edge)].ghostline=(*ghostline);
      polygon_info->edges[(*edge)].direction=(ssize_t) (direction > 0);
      if (direction < 0)
        ReversePoints((*points),(size_t) n);
      polygon_info->edges[(*edge)].points=(*points);
      polygon_info->edges[(*edge)].bounds=bounds;
      polygon_info->edges[(*edge)].bounds.y1=(*points)[0].y;
      polygon_info->edges[(*edge)].bounds.y2=(*points)[n-1].y;
      (*points)=(PointInfo *) NULL;
      (*ghostline)=MagickFalse;
      (*edge)++;
      polygon_info->number_edges=(*edge);
    }
  return(MagickTrue);
}

static MagickBooleanType FlushFinalEdge(PolygonInfo *polygon_info,
  const PolygonEdgeInput *input,const PolygonEdgeTargets *targets)
{
  size_t
    *const number_edges = targets->number_edges;

  PointInfo
    **const points = targets->points;

  MagickBooleanType
    *const ghostline = targets->ghostline;

  size_t
    *const edge = targets->edge;

  const ssize_t
    n = input->n;

  const long
    direction = input->direction;

  const SegmentInfo
    bounds = input->bounds;

  if ((*points) == (PointInfo *) NULL)
    return(MagickTrue);
  if (n < 2)
    (*points)=(PointInfo *) RelinquishMagickMemory((*points));
  else
    {
      if (GrowPolygonEdges(polygon_info,(*edge),number_edges) == MagickFalse)
        return(MagickFalse);
      polygon_info->edges[(*edge)].number_points=(size_t) n;
      polygon_info->edges[(*edge)].scanline=(-1.0);
      polygon_info->edges[(*edge)].highwater=0;
      polygon_info->edges[(*edge)].ghostline=(*ghostline);
      polygon_info->edges[(*edge)].direction=(ssize_t) (direction > 0);
      if (direction < 0)
        ReversePoints((*points),(size_t) n);
      polygon_info->edges[(*edge)].points=(*points);
      polygon_info->edges[(*edge)].bounds=bounds;
      polygon_info->edges[(*edge)].bounds.y1=(*points)[0].y;
      polygon_info->edges[(*edge)].bounds.y2=(*points)[n-1].y;
      (*points)=(PointInfo *) NULL;
      (*ghostline)=MagickFalse;
      (*edge)++;
      polygon_info->number_edges=(*edge);
    }
  return(MagickTrue);
}

static MagickBooleanType ShrinkPolygonEdgePoints(PolygonInfo *polygon_info)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) polygon_info->number_edges; i++)
  {
    EdgeInfo
      *edge_info;

    edge_info=polygon_info->edges+i;
    edge_info->points=(PointInfo *) ResizeQuantumMemory(edge_info->points,
      edge_info->number_points,sizeof(*edge_info->points));
    if (edge_info->points == (PointInfo *) NULL)
      return(MagickFalse);
  }
  return(MagickTrue);
}

static void WidenEdgeBounds(const PointInfo point,SegmentInfo *bounds)
{
  if (point.x < (*bounds).x1)
    (*bounds).x1=point.x;
  if (point.x > (*bounds).x2)
    (*bounds).x2=point.x;
}

static PolygonInfo *ThrowPolygonAllocationFailure(PolygonInfo *polygon_info,
  ExceptionInfo *exception)
{
  (void) ThrowMagickException(exception,GetMagickModule(),
    ResourceLimitError,"MemoryAllocationFailed","`%s'","");
  return(DestroyPolygonInfo(polygon_info));
}

static PolygonInfo *ConvertPathToPolygon(const PathInfo *path_info,
  ExceptionInfo *exception)
{
  long
    direction,
    next_direction;

  PointInfo
    point,
    *points;

  PolygonInfo
    *polygon_info;

  SegmentInfo
    bounds;

  ssize_t
    i,
    n;

  MagickBooleanType
    ghostline;

  size_t
    edge,
    number_edges,
    number_points;

  PolygonEdgeInput
    input;

  const PolygonEdgeTargets
    targets = { &number_edges, &points, &ghostline, &edge };

  /*
    Convert a path to the more efficient sorted rendering form.
  */
  polygon_info=(PolygonInfo *) AcquireMagickMemory(sizeof(*polygon_info));
  if (polygon_info == (PolygonInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return((PolygonInfo *) NULL);
    }
  number_edges=16;
  polygon_info->edges=(EdgeInfo *) AcquireQuantumMemory(number_edges,
    sizeof(*polygon_info->edges));
  if (polygon_info->edges == (EdgeInfo *) NULL)
    return(ThrowPolygonAllocationFailure(polygon_info,exception));
  (void) memset(polygon_info->edges,0,number_edges*
    sizeof(*polygon_info->edges));
  direction=0;
  edge=0;
  ghostline=MagickFalse;
  n=0;
  number_points=0;
  points=(PointInfo *) NULL;
  (void) memset(&point,0,sizeof(point));
  (void) memset(&bounds,0,sizeof(bounds));
  polygon_info->edges[edge].number_points=(size_t) n;
  polygon_info->edges[edge].scanline=0.0;
  polygon_info->edges[edge].highwater=0;
  polygon_info->edges[edge].ghostline=ghostline;
  polygon_info->edges[edge].direction=(ssize_t) direction;
  polygon_info->edges[edge].points=points;
  polygon_info->edges[edge].bounds=bounds;
  polygon_info->number_edges=0;
  for (i=0; path_info[i].code != EndCode; i++)
  {
    if (IsSubpathStart(path_info,i) != MagickFalse)
      {
        /*
          Move to.
        */
        input.n=n;
        input.direction=direction;
        input.bounds=bounds;
        if (FlushSubpathEdge(polygon_info,&input,&targets) == MagickFalse)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'","");
            points=(PointInfo *) RelinquishMagickMemory(points);
            return(DestroyPolygonInfo(polygon_info));
          }
        if (points == (PointInfo *) NULL)
          {
            number_points=16;
            points=(PointInfo *) AcquireQuantumMemory((size_t) number_points,
              sizeof(*points));
          }
        if (points == (PointInfo *) NULL)
          return(ThrowPolygonAllocationFailure(polygon_info,exception));
        ghostline=path_info[i].code == GhostlineCode ? MagickTrue : MagickFalse;
        point=path_info[i].point;
        points[0]=point;
        bounds.x1=point.x;
        bounds.x2=point.x;
        direction=0;
        n=1;
        continue;
      }
    /*
      Line to.
    */
    next_direction=((path_info[i].point.y > point.y) ||
      ((fabs(path_info[i].point.y-point.y) < MagickEpsilon) &&
       (path_info[i].point.x > point.x))) ? 1 : -1;
    if (IsDirectionChange(points,direction,next_direction) != MagickFalse)
      {
        /*
          New edge.
        */
        point=points[n-1];
        if (GrowPolygonEdges(polygon_info,edge,&number_edges) == MagickFalse)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'","");
            points=(PointInfo *) RelinquishMagickMemory(points);
            return(DestroyPolygonInfo(polygon_info));
          }
        polygon_info->edges[edge].number_points=(size_t) n;
        polygon_info->edges[edge].scanline=(-1.0);
        polygon_info->edges[edge].highwater=0;
        polygon_info->edges[edge].ghostline=ghostline;
        polygon_info->edges[edge].direction=(ssize_t) (direction > 0);
        if (direction < 0)
          ReversePoints(points,(size_t) n);
        polygon_info->edges[edge].points=points;
        polygon_info->edges[edge].bounds=bounds;
        polygon_info->edges[edge].bounds.y1=points[0].y;
        polygon_info->edges[edge].bounds.y2=points[n-1].y;
        polygon_info->number_edges=edge+1;
        points=(PointInfo *) NULL;
        number_points=16;
        points=(PointInfo *) AcquireQuantumMemory((size_t) number_points,
          sizeof(*points));
        if (points == (PointInfo *) NULL)
          return(ThrowPolygonAllocationFailure(polygon_info,exception));
        n=1;
        ghostline=MagickFalse;
        points[0]=point;
        bounds.x1=point.x;
        bounds.x2=point.x;
        edge++;
      }
    direction=next_direction;
    if (points == (PointInfo *) NULL)
      continue;
    if (n == (ssize_t) number_points)
      {
        number_points<<=1;
        points=(PointInfo *) ResizeQuantumMemory(points,(size_t) number_points,
          sizeof(*points));
        if (points == (PointInfo *) NULL)
          return(ThrowPolygonAllocationFailure(polygon_info,exception));
      }
    point=path_info[i].point;
    points[n]=point;
    WidenEdgeBounds(point,&bounds);
    n++;
  }
  input.n=n;
  input.direction=direction;
  input.bounds=bounds;
  if (FlushFinalEdge(polygon_info,&input,&targets) == MagickFalse)
    return(ThrowPolygonAllocationFailure(polygon_info,exception));
  polygon_info->number_edges=edge;
  polygon_info->edges=(EdgeInfo *) ResizeQuantumMemory(polygon_info->edges,
    polygon_info->number_edges,sizeof(*polygon_info->edges));
  if (polygon_info->edges == (EdgeInfo *) NULL)
    return(ThrowPolygonAllocationFailure(polygon_info,exception));
  if (ShrinkPolygonEdgePoints(polygon_info) == MagickFalse)
    return(ThrowPolygonAllocationFailure(polygon_info,exception));
  qsort(polygon_info->edges,(size_t) polygon_info->number_edges,
    sizeof(*polygon_info->edges),DrawCompareEdges);
  if ((GetLogEventMask() & DrawEvent) != 0)
    LogPolygonInfo(polygon_info);
  return(polygon_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C o n v e r t P r i m i t i v e T o P a t h                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvertPrimitiveToPath() converts a PrimitiveInfo structure into a vector
%  path structure.
%
%  The format of the ConvertPrimitiveToPath method is:
%
%      PathInfo *ConvertPrimitiveToPath(const DrawInfo *draw_info,
%        const PrimitiveInfo *primitive_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o ConvertPrimitiveToPath() returns a vector path structure of type
%      PathInfo.
%
%    o draw_info: a structure of type DrawInfo.
%
%    o primitive_info: Specifies a pointer to an PrimitiveInfo structure.
%
*/

static void LogPathInfo(const PathInfo *path_info)
{
  const PathInfo
    *p;

  (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    begin vector-path");
  for (p=path_info; p->code != EndCode; p++)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),
      "      %g,%g %s",p->point.x,p->point.y,p->code == GhostlineCode ?
      "moveto ghostline" : p->code == OpenCode ? "moveto open" :
      p->code == MoveToCode ? "moveto" : p->code == LineToCode ? "lineto" :
      "?");
  (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    end vector-path");
}

typedef struct _NewSubpathPointInput
{
  PathInfoCode
    code;

  size_t
    coordinates;

  PointInfo
    q;

  const PrimitiveInfo
    *primitive_info;

  size_t
    i;
} NewSubpathPointInput;

static inline MagickBooleanType IsNewSubpathPoint(
  const NewSubpathPointInput *input)
{
  const PathInfoCode
    code = input->code;

  const size_t
    coordinates = input->coordinates,
    i = input->i;

  const PointInfo
    q = input->q;

  const PrimitiveInfo
    *const primitive_info = input->primitive_info;

  return(((code == MoveToCode) || (coordinates <= 0) ||
    (fabs(q.x-primitive_info[i].point.x) >= MagickEpsilon) ||
    (fabs(q.y-primitive_info[i].point.y) >= MagickEpsilon)) ? MagickTrue :
    MagickFalse);
}

static PathInfo *TerminatePathInfo(PathInfo *path_info,const ssize_t n)
{
  path_info[n].code=EndCode;
  path_info[n].point.x=0.0;
  path_info[n].point.y=0.0;
  if (IsEventLogging() != MagickFalse)
    LogPathInfo(path_info);
  path_info=(PathInfo *) ResizeQuantumMemory(path_info,(size_t) (n+1),
    sizeof(*path_info));
  return(path_info);
}

static MagickBooleanType IsNonPathPrimitive(const PrimitiveType primitive)
{
  switch (primitive)
  {
    case AlphaPrimitive:
    case ColorPrimitive:
    case ImagePrimitive:
    case PointPrimitive:
    case TextPrimitive:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static PathInfo *ConvertPrimitiveToPath(const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  MagickBooleanType
    closed_subpath;

  PathInfo
    *path_info;

  PathInfoCode
    code;

  PointInfo
    p,
    q;

  ssize_t
    n,
    start;

  size_t
    coordinates,
    i;

  /*
    Converts a PrimitiveInfo structure into a vector path structure.
  */
  if (IsNonPathPrimitive(primitive_info->primitive) != MagickFalse)
    return((PathInfo *) NULL);
  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++) ;
  path_info=(PathInfo *) AcquireQuantumMemory((size_t) (3UL*i+1UL),
    sizeof(*path_info));
  if (path_info == (PathInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return((PathInfo *) NULL);
    }
  coordinates=0;
  closed_subpath=MagickFalse;
  n=0;
  p.x=(-1.0);
  p.y=(-1.0);
  q.x=(-1.0);
  q.y=(-1.0);
  start=0;
  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++)
  {
    NewSubpathPointInput
      subpath;

    code=LineToCode;
    if (coordinates <= 0)
      {
        /*
          New subpath.
        */
        coordinates=primitive_info[i].coordinates;
        p=primitive_info[i].point;
        start=n;
        code=MoveToCode;
        closed_subpath=primitive_info[i].closed_subpath;
      }
    coordinates--;
    subpath.code=code;
    subpath.coordinates=coordinates;
    subpath.q=q;
    subpath.primitive_info=primitive_info;
    subpath.i=i;
    if (IsNewSubpathPoint(&subpath) != MagickFalse)
      {
        /*
          Eliminate duplicate points.
        */
        path_info[n].code=code;
        path_info[n].point=primitive_info[i].point;
        q=primitive_info[i].point;
        n++;
      }
    if (coordinates > 0)
      continue;  /* next point in current subpath */
    if (closed_subpath != MagickFalse)
      {
        closed_subpath=MagickFalse;
        continue;
      }
    /*
      Mark the p point as open if the subpath is not closed.
    */
    path_info[start].code=OpenCode;
    path_info[n].code=GhostlineCode;
    path_info[n].point=primitive_info[i].point;
    n++;
    path_info[n].code=LineToCode;
    path_info[n].point=p;
    n++;
  }
  return(TerminatePathInfo(path_info,n));
}



static PointInfo GetDensityResolution(const DrawInfo *clone_info)
{
  PointInfo
    resolution;

  resolution.x=96.0;
  resolution.y=96.0;
  if (clone_info->density != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(clone_info->density,&geometry_info);
      if ((flags & RhoValue) != 0)
        resolution.x=geometry_info.rho;
      resolution.y=resolution.x;
      if ((flags & SigmaValue) != 0)
        resolution.y=geometry_info.sigma;
    }
  return(resolution);
}

static SegmentInfo GetPolygonBounds(const Image *image,
  const PolygonInfo *polygon_info,const double mid)
{
  SegmentInfo
    bounds;

  ssize_t
    i;

  bounds=polygon_info->edges[0].bounds;
  for (i=1; i < (ssize_t) polygon_info->number_edges; i++)
  {
    if (polygon_info->edges[i].bounds.x1 < (double) bounds.x1)
      bounds.x1=polygon_info->edges[i].bounds.x1;
    if (polygon_info->edges[i].bounds.y1 < (double) bounds.y1)
      bounds.y1=polygon_info->edges[i].bounds.y1;
    if (polygon_info->edges[i].bounds.x2 > (double) bounds.x2)
      bounds.x2=polygon_info->edges[i].bounds.x2;
    if (polygon_info->edges[i].bounds.y2 > (double) bounds.y2)
      bounds.y2=polygon_info->edges[i].bounds.y2;
  }
  bounds.x1-=mid;
  bounds.x1=bounds.x1 < 0.0 ? 0.0 : bounds.x1 >= (double)
    image->columns ? (double) image->columns-1 : bounds.x1;
  bounds.y1-=mid;
  bounds.y1=bounds.y1 < 0.0 ? 0.0 : bounds.y1 >= (double)
    image->rows ? (double) image->rows-1 : bounds.y1;
  bounds.x2+=mid;
  bounds.x2=bounds.x2 < 0.0 ? 0.0 : bounds.x2 >= (double)
    image->columns ? (double) image->columns-1 : bounds.x2;
  bounds.y2+=mid;
  bounds.y2=bounds.y2 < 0.0 ? 0.0 : bounds.y2 >= (double)
    image->rows ? (double) image->rows-1 : bounds.y2;
  return(bounds);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D r a w B o u n d i n g R e c t a n g l e s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawBoundingRectangles() draws the bounding rectangles on the image.  This
%  is only useful for developers debugging the rendering algorithm.
%
%  The format of the DrawBoundingRectangles method is:
%
%      MagickBooleanType DrawBoundingRectangles(Image *image,
%        const DrawInfo *draw_info,PolygonInfo *polygon_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o polygon_info: Specifies a pointer to a PolygonInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static MagickBooleanType DrawBoundingRectangles(Image *image,
  const DrawInfo *draw_info,const PolygonInfo *polygon_info,
  ExceptionInfo *exception)
{
  double
    mid;

  DrawInfo
    *clone_info;

  MagickStatusType
    status;

  PointInfo
    end,
    resolution,
    start;

  PrimitiveInfo
    primitive_info[6];

  ssize_t
    i;

  SegmentInfo
    bounds;

  ssize_t
    coordinates;

  (void) memset(primitive_info,0,sizeof(primitive_info));
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  status=QueryColorCompliance("#000F",AllCompliance,&clone_info->fill,
    exception);
  if (status == MagickFalse)
    {
      clone_info=DestroyDrawInfo(clone_info);
      return(MagickFalse);
    }
  resolution=GetDensityResolution(clone_info);
  mid=(resolution.x/96.0)*ExpandAffine(&clone_info->affine)*
    clone_info->stroke_width/2.0;
  bounds.x1=0.0;
  bounds.y1=0.0;
  bounds.x2=0.0;
  bounds.y2=0.0;
  if (polygon_info != (PolygonInfo *) NULL)
    {
      bounds=GetPolygonBounds(image,polygon_info,mid);
      for (i=0; i < (ssize_t) polygon_info->number_edges; i++)
      {
        if (polygon_info->edges[i].direction != 0)
          status=QueryColorCompliance("#f00",AllCompliance,&clone_info->stroke,
            exception);
        else
          status=QueryColorCompliance("#0f0",AllCompliance,&clone_info->stroke,
            exception);
        if (status == MagickFalse)
          break;
        start.x=(double) (polygon_info->edges[i].bounds.x1-mid);
        start.y=(double) (polygon_info->edges[i].bounds.y1-mid);
        end.x=(double) (polygon_info->edges[i].bounds.x2+mid);
        end.y=(double) (polygon_info->edges[i].bounds.y2+mid);
        primitive_info[0].primitive=RectanglePrimitive;
        status&=(MagickStatusType) TraceRectangle(primitive_info,start,end);
        primitive_info[0].method=ReplaceMethod;
        coordinates=(ssize_t) primitive_info[0].coordinates;
        primitive_info[coordinates].primitive=UndefinedPrimitive;
        status=DrawPrimitive(image,clone_info,primitive_info,exception);
        if (status == MagickFalse)
          break;
      }
      if (i < (ssize_t) polygon_info->number_edges)
        {
          clone_info=DestroyDrawInfo(clone_info);
          return(status == 0 ? MagickFalse : MagickTrue);
        }
    }
  status=QueryColorCompliance("#00f",AllCompliance,&clone_info->stroke,
    exception);
  if (status == MagickFalse)
    {
      clone_info=DestroyDrawInfo(clone_info);
      return(MagickFalse);
    }
  start.x=(double) (bounds.x1-mid);
  start.y=(double) (bounds.y1-mid);
  end.x=(double) (bounds.x2+mid);
  end.y=(double) (bounds.y2+mid);
  primitive_info[0].primitive=RectanglePrimitive;
  status&=(MagickStatusType) TraceRectangle(primitive_info,start,end);
  primitive_info[0].method=ReplaceMethod;
  coordinates=(ssize_t) primitive_info[0].coordinates;
  primitive_info[coordinates].primitive=UndefinedPrimitive;
  status=DrawPrimitive(image,clone_info,primitive_info,exception);
  clone_info=DestroyDrawInfo(clone_info);
  return(status == 0 ? MagickFalse : MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D r a w D a s h P o l y g o n                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawDashPolygon() draws a dashed polygon (line, rectangle, ellipse) on the
%  image while respecting the dash offset and dash pattern attributes.
%
%  The format of the DrawDashPolygon method is:
%
%      MagickBooleanType DrawDashPolygon(const DrawInfo *draw_info,
%        const PrimitiveInfo *primitive_info,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o draw_info: the draw info.
%
%    o primitive_info: Specifies a pointer to a PrimitiveInfo structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _DashSegmentPendingInput
{
  MagickStatusType
    status;

  double
    total_length;

  double
    maximum_length;

  ssize_t
    n;

  ssize_t
    j;
} DashSegmentPendingInput;

static inline MagickBooleanType IsDashSegmentPending(
  const DashSegmentPendingInput *input)
{
  const MagickStatusType
    status = input->status;

  const double
    total_length = input->total_length,
    maximum_length = input->maximum_length;

  const ssize_t
    n = input->n,
    j = input->j;

  return(((status != MagickFalse) && (total_length < maximum_length) &&
    ((n & 0x01) == 0) && (j > 1)) ? MagickTrue : MagickFalse);
}

static ssize_t GetNextDashIndex(const DrawInfo *draw_info,ssize_t n)
{
  if (fabs(draw_info->dash_pattern[n]) >= MagickEpsilon)
    n++;
  if (fabs(draw_info->dash_pattern[n]) < MagickEpsilon)
    n=0;
  return(n);
}

typedef struct _DashSegmentInfo
{
  size_t
    number_vertices;

  PrimitiveInfo
    *dash_polygon;

  const PrimitiveInfo
    *primitive_info;

  ssize_t
    i,
    n;

  double
    dx,
    dy,
    total_length,
    maximum_length;

  Image
    *image;

  DrawInfo
    *clone_info;

  ExceptionInfo
    *exception;
} DashSegmentInfo;

typedef struct _DashSegmentTargets
{
  ssize_t
    *j;

  MagickStatusType
    *status;
} DashSegmentTargets;

static MagickBooleanType DrawDashSegment(const DashSegmentInfo *segment,
  const DashSegmentTargets *targets)
{
  const size_t
    number_vertices = segment->number_vertices;

  PrimitiveInfo
    *const dash_polygon = segment->dash_polygon;

  const PrimitiveInfo
    *const primitive_info = segment->primitive_info;

  const ssize_t
    i = segment->i,
    n = segment->n;

  const double
    dx = segment->dx,
    dy = segment->dy,
    total_length = segment->total_length,
    maximum_length = segment->maximum_length;

  Image
    *const image = segment->image;

  DrawInfo
    *const clone_info = segment->clone_info;

  ExceptionInfo
    *const exception = segment->exception;

  ssize_t
    *const j = targets->j;

  MagickStatusType
    *const status = targets->status;

  if ((n & 0x01) != 0)
    {
      dash_polygon[0]=primitive_info[0];
      dash_polygon[0].closed_subpath=MagickFalse;
      dash_polygon[0].point.x=(double) (primitive_info[i-1].point.x+dx*
        total_length*MagickSafeReciprocal(maximum_length));
      dash_polygon[0].point.y=(double) (primitive_info[i-1].point.y+dy*
        total_length*MagickSafeReciprocal(maximum_length));
      *j=1;
    }
  else
    {
      if ((*j+1) > (ssize_t) number_vertices)
        return(MagickFalse);
      dash_polygon[*j]=primitive_info[i-1];
      dash_polygon[*j].closed_subpath=MagickFalse;
      dash_polygon[*j].point.x=(double) (primitive_info[i-1].point.x+dx*
        total_length*MagickSafeReciprocal(maximum_length));
      dash_polygon[*j].point.y=(double) (primitive_info[i-1].point.y+dy*
        total_length*MagickSafeReciprocal(maximum_length));
      dash_polygon[*j].coordinates=1;
      (*j)++;
      dash_polygon[0].coordinates=(size_t) *j;
      dash_polygon[*j].primitive=UndefinedPrimitive;
      *status&=(MagickStatusType) DrawStrokePolygon(image,clone_info,
        dash_polygon,exception);
      if (*status == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _DashOffsetInput
{
  const DrawInfo
    *draw_info;

  double
    scale,
    offset;
} DashOffsetInput;

static void SkipDashOffset(const DashOffsetInput *dash,double *length,
  ssize_t *n,ssize_t *j)
{
  const DrawInfo
    *const draw_info = dash->draw_info;

  const double
    scale = dash->scale;

  double
    offset = dash->offset;

  (*j)=1;
  for ((*n)=0; offset > 0.0; (*j)=0)
  {
    if (draw_info->dash_pattern[(*n)] <= 0.0)
      break;
    (*length)=scale*(draw_info->dash_pattern[(*n)]+((*n) == 0 ? -0.5 : 0.5));
    if (offset > (*length))
      {
        offset-=(*length);
        (*n)++;
        (*length)=scale*draw_info->dash_pattern[(*n)];
        continue;
      }
    if (offset < (*length))
      {
        (*length)-=offset;
        offset=0.0;
        break;
      }
    offset=0.0;
    (*n)++;
  }
}

static MagickBooleanType DrawDashPolygon(const DrawInfo *draw_info,
  const PrimitiveInfo *primitive_info,Image *image,ExceptionInfo *exception)
{
  double
    dx,
    dy,
    length,
    maximum_length,
    offset,
    scale,
    total_length;

  DrawInfo
    *clone_info;

  MagickStatusType
    status;

  PrimitiveInfo
    *dash_polygon;

  ssize_t
    i;

  DashOffsetInput
    dash;

  DashSegmentPendingInput
    pending;

  size_t
    number_vertices;

  ssize_t
    j,
    n;

  assert(draw_info != (const DrawInfo *) NULL);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    begin draw-dash");
  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++) ;
  number_vertices=(size_t) i;
  dash_polygon=(PrimitiveInfo *) AcquireQuantumMemory((size_t)
    (2UL*number_vertices+32UL),sizeof(*dash_polygon));
  if (dash_polygon == (PrimitiveInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return(MagickFalse);
    }
  (void) memset(dash_polygon,0,(2UL*number_vertices+32UL)*
    sizeof(*dash_polygon));
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  clone_info->miterlimit=0;
  dash_polygon[0]=primitive_info[0];
  dash_polygon[0].closed_subpath=MagickFalse;
  scale=ExpandAffine(&draw_info->affine);
  length=scale*draw_info->dash_pattern[0];
  offset=fabs(draw_info->dash_offset) >= MagickEpsilon ?
    scale*draw_info->dash_offset : 0.0;
  dash.draw_info=draw_info;
  dash.scale=scale;
  dash.offset=offset;
  SkipDashOffset(&dash,&length,&n,&j);
  status=MagickTrue;
  maximum_length=0.0;
  total_length=0.0;
  for (i=1; (i < (ssize_t) number_vertices) && (length >= 0.0); i++)
  {
    dx=primitive_info[i].point.x-primitive_info[i-1].point.x;
    dy=primitive_info[i].point.y-primitive_info[i-1].point.y;
    maximum_length=hypot(dx,dy);
    if (maximum_length > (double) (MaxBezierCoordinates >> 2))
      continue;
    if (fabs(length) < MagickEpsilon)
      {
        n=GetNextDashIndex(draw_info,n);
        length=scale*draw_info->dash_pattern[n];
      }
    for (total_length=0.0; (length >= 0.0) && (maximum_length >= (total_length+length)); )
    {
      DashSegmentInfo
        segment;

      DashSegmentTargets
        targets;

      total_length+=length;
      segment.number_vertices=number_vertices;
      segment.dash_polygon=dash_polygon;
      segment.primitive_info=primitive_info;
      segment.i=i;
      segment.n=n;
      segment.dx=dx;
      segment.dy=dy;
      segment.total_length=total_length;
      segment.maximum_length=maximum_length;
      segment.image=image;
      segment.clone_info=clone_info;
      segment.exception=exception;
      targets.j=(&j);
      targets.status=(&status);
      if (DrawDashSegment(&segment,&targets) == MagickFalse)
        break;
      n=GetNextDashIndex(draw_info,n);
      length=scale*draw_info->dash_pattern[n];
    }
    length-=(maximum_length-total_length);
    if ((n & 0x01) != 0)
      continue;
    dash_polygon[j]=primitive_info[i];
    dash_polygon[j].coordinates=1;
    j++;
  }
  pending.status=status;
  pending.total_length=total_length;
  pending.maximum_length=maximum_length;
  pending.n=n;
  pending.j=j;
  if (IsDashSegmentPending(&pending) != MagickFalse)
    {
      dash_polygon[j]=primitive_info[i-1];
      dash_polygon[j].closed_subpath=MagickFalse;
      dash_polygon[j].point.x+=MagickEpsilon;
      dash_polygon[j].point.y+=MagickEpsilon;
      dash_polygon[j].coordinates=1;
      j++;
      dash_polygon[0].coordinates=(size_t) j;
      dash_polygon[j].primitive=UndefinedPrimitive;
      status&=(MagickStatusType) DrawStrokePolygon(image,clone_info,
        dash_polygon,exception);
    }
  dash_polygon=(PrimitiveInfo *) RelinquishMagickMemory(dash_polygon);
  clone_info=DestroyDrawInfo(clone_info);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    end draw-dash");
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D r a w P o l y g o n P r i m i t i v e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPolygonPrimitive() draws a polygon on the image.
%
%  The format of the DrawPolygonPrimitive method is:
%
%      MagickBooleanType DrawPolygonPrimitive(Image *image,
%        const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
%        ExceptionInfo *exception)
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

static PolygonInfo **DestroyPolygonTLS(PolygonInfo **polygon_info)
{
  ssize_t
    i;

  assert(polygon_info != (PolygonInfo **) NULL);
  for (i=0; i < (ssize_t) GetMagickResourceLimit(ThreadResource); i++)
    if (polygon_info[i] != (PolygonInfo *) NULL)
      polygon_info[i]=DestroyPolygonInfo(polygon_info[i]);
  polygon_info=(PolygonInfo **) RelinquishMagickMemory(polygon_info);
  return(polygon_info);
}

static PolygonInfo **AcquirePolygonTLS(const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  PathInfo
    *magick_restrict path_info;

  PolygonInfo
    **polygon_info;

  size_t
    number_threads;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  polygon_info=(PolygonInfo **) AcquireQuantumMemory(number_threads,
    sizeof(*polygon_info));
  if (polygon_info == (PolygonInfo **) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return((PolygonInfo **) NULL);
    }
  (void) memset(polygon_info,0,number_threads*sizeof(*polygon_info));
  path_info=ConvertPrimitiveToPath(primitive_info,exception);
  if (path_info == (PathInfo *) NULL)
    return(DestroyPolygonTLS(polygon_info));
  polygon_info[0]=ConvertPathToPolygon(path_info,exception);
  if (polygon_info[0] == (PolygonInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return(DestroyPolygonTLS(polygon_info));
    }
  path_info=(PathInfo *) RelinquishMagickMemory(path_info);
  return(polygon_info);
}

static MagickBooleanType ClonePolygonEdgePoints(PolygonInfo **polygon_info,
  const ssize_t i)
{
  EdgeInfo
    *edge_info;

  ssize_t
    j;

  for (j=0; j < (ssize_t) polygon_info[i]->number_edges; j++)
  {
    edge_info=polygon_info[0]->edges+j;
    polygon_info[i]->edges[j].points=(PointInfo *) AcquireQuantumMemory(
      edge_info->number_points,sizeof(*edge_info));
    if (polygon_info[i]->edges[j].points == (PointInfo *) NULL)
      return(MagickFalse);
    (void) memcpy(polygon_info[i]->edges[j].points,edge_info->points,
      edge_info->number_points*sizeof(*edge_info->points));
  }
  return(MagickTrue);
}

static MagickBooleanType ClonePolygonEdgesTLS(PolygonInfo **polygon_info,
  const size_t number_threads,ExceptionInfo *exception)
{
  ssize_t
    i;

  for (i=1; i < (ssize_t) number_threads; i++)
  {
    EdgeInfo
      *edge_info;

    ssize_t
      j;

    polygon_info[i]=(PolygonInfo *) AcquireMagickMemory(
      sizeof(*polygon_info[i]));
    if (polygon_info[i] == (PolygonInfo *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'","");
        return(MagickFalse);
      }
    polygon_info[i]->number_edges=0;
    edge_info=polygon_info[0]->edges;
    polygon_info[i]->edges=(EdgeInfo *) AcquireQuantumMemory(
      polygon_info[0]->number_edges,sizeof(*edge_info));
    if (polygon_info[i]->edges == (EdgeInfo *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'","");
        return(MagickFalse);
      }
    (void) memcpy(polygon_info[i]->edges,edge_info,
      polygon_info[0]->number_edges*sizeof(*edge_info));
    for (j=0; j < (ssize_t) polygon_info[i]->number_edges; j++)
      polygon_info[i]->edges[j].points=(PointInfo *) NULL;
    polygon_info[i]->number_edges=polygon_info[0]->number_edges;
    if (ClonePolygonEdgePoints(polygon_info,i) == MagickFalse)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'","");
        return(MagickFalse);
      }
  }
  return(MagickTrue);
}

static size_t DestroyEdge(PolygonInfo *polygon_info,const ssize_t edge)
{
  assert(edge < (ssize_t) polygon_info->number_edges);
  polygon_info->edges[edge].points=(PointInfo *) RelinquishMagickMemory(
    polygon_info->edges[edge].points);
  polygon_info->number_edges--;
  if (edge < (ssize_t) polygon_info->number_edges)
    (void) memmove(polygon_info->edges+edge,polygon_info->edges+edge+1,
      (polygon_info->number_edges-(size_t) edge)*sizeof(*polygon_info->edges));
  return(polygon_info->number_edges);
}

static inline MagickBooleanType IsFillAlphaSkipped(const MagickBooleanType fill,
  const double distance,const double subpath_alpha)
{
  return(((fill == MagickFalse) || (distance > 1.0) ||
    (subpath_alpha >= 1.0)) ? MagickTrue : MagickFalse);
}

static double GetEdgeDistance(const EdgeInfo *p,const ssize_t i,
  const ssize_t x,const ssize_t y)
{
  const PointInfo
    *q;

  double
    alpha,
    beta,
    distance;

  PointInfo
    delta;

  /*
    Compute distance between a point and an edge.
  */
  q=p->points+i-1;
  delta.x=(q+1)->x-q->x;
  delta.y=(q+1)->y-q->y;
  beta=delta.x*(x-q->x)+delta.y*(y-q->y);  /* segLen*point-cos(theta) */
  if (beta <= 0.0)
    {
      /*
        Cosine <= 0, point is closest to q.
      */
      delta.x=(double) x-q->x;
      delta.y=(double) y-q->y;
      distance=delta.x*delta.x+delta.y*delta.y;
    }
  else
    {
      alpha=delta.x*delta.x+delta.y*delta.y;  /* segLen*segLen */
      if (beta >= alpha)
        {
          /*
            Point is closest to q+1.
          */
          delta.x=(double) x-(q+1)->x;
          delta.y=(double) y-(q+1)->y;
          distance=delta.x*delta.x+delta.y*delta.y;
        }
      else
        {
          /*
            Point is closest to point between q & q+1.
          */
          alpha=MagickSafeReciprocal(alpha);
          beta=delta.x*(y-q->y)-delta.y*(x-q->x);
          distance=alpha*beta*beta;
        }
    }
  return(distance);
}

static inline ssize_t GetEdgeDirectionSign(const ssize_t direction)
{
  return(direction != 0 ? 1 : -1);
}

static inline MagickBooleanType IsWindingEdgeSkipped(const EdgeInfo *p,
  const ssize_t x,const ssize_t y)
{
  return((((double) y > p->bounds.y2) || ((double) x <= p->bounds.x1)) ?
    MagickTrue : MagickFalse);
}

static ssize_t GetWindingEdgeIndex(EdgeInfo *p,const ssize_t y)
{
  ssize_t
    i;

  i=(ssize_t) MagickMax((double) p->highwater,1.0);
  for ( ; i < (ssize_t) (p->number_points-1); i++)
    if ((double) y <= p->points[i].y)
      break;
  return(i);
}

static ssize_t GetWindingNumber(PolygonInfo *polygon_info,const ssize_t x,
  const ssize_t y)
{
  const PointInfo
    *q;

  EdgeInfo
    *p;

  ssize_t
    i,
    j,
    winding_number;

  /*
    Determine winding number.
  */
  winding_number=0;
  p=polygon_info->edges;
  for (j=0; j < (ssize_t) polygon_info->number_edges; j++, p++)
  {
    if ((double) y <= p->bounds.y1)
      break;
    if (IsWindingEdgeSkipped(p,x,y) != MagickFalse)
      continue;
    if ((double) x > p->bounds.x2)
      {
        winding_number+=GetEdgeDirectionSign(p->direction);
        continue;
      }
    i=GetWindingEdgeIndex(p,y);
    q=p->points+i-1;
    if ((((q+1)->x-q->x)*(y-q->y)) <= (((q+1)->y-q->y)*(x-q->x)))
      winding_number+=GetEdgeDirectionSign(p->direction);
  }
  return(winding_number);
}

static double GetStrokeEdgeBeta(const double distance,const double mid,
  double *stroke_alpha)
{
  double
    alpha,
    beta;

  beta=1.0;
  if (fabs(distance-1.0) >= MagickEpsilon)
    beta=sqrt((double) distance);
  alpha=beta-mid-0.5;
  if (*stroke_alpha < ((alpha-0.25)*(alpha-0.25)))
    *stroke_alpha=(alpha-0.25)*(alpha-0.25);
  return(beta);
}

static double GetStrokeBeta(const EdgeInfo *p,const double distance,
  const double mid,double *stroke_alpha)
{
  double
    alpha,
    beta;

  beta=0.0;
  if (p->ghostline == MagickFalse)
    {
      alpha=mid+0.5;
      if ((*stroke_alpha < 1.0) &&
          (distance <= ((alpha+0.25)*(alpha+0.25))))
        {
          alpha=mid-0.5;
          if (distance <= ((alpha+0.25)*(alpha+0.25)))
            *stroke_alpha=1.0;
          else
            beta=GetStrokeEdgeBeta(distance,mid,stroke_alpha);
        }
    }
  return(beta);
}

static double GetSubpathBeta(double beta,const double distance)
{
  if (fabs(beta) < MagickEpsilon)
    {
      beta=1.0;
      if (fabs(distance-1.0) >= MagickEpsilon)
        beta=sqrt(distance);
    }
  return(beta);
}

static double GetFillOpacity(PolygonInfo *polygon_info,
  const FillAlphaSample *sample,const FillRule fill_rule,
  const double subpath_alpha)
{
  const MagickBooleanType
    fill = sample->fill;

  const ssize_t
    x = sample->x,
    y = sample->y;

  ssize_t
    winding_number;

  if (fill == MagickFalse)
    return(0.0);
  if (subpath_alpha >= 1.0)
    return(1.0);
  winding_number=GetWindingNumber(polygon_info,x,y);
  if (fill_rule != NonZeroRule)
    {
      if ((MagickAbsoluteValue(winding_number) & 0x01) != 0)
        return(1.0);
    }
  else
    if (MagickAbsoluteValue(winding_number) != 0)
      return(1.0);
  return(subpath_alpha);
}

static double GetFillAlpha(PolygonInfo *polygon_info,
  const FillAlphaSample *sample,const FillRule fill_rule,double *stroke_alpha)
{
  const double
    mid = sample->mid;

  const MagickBooleanType
    fill = sample->fill;

  const ssize_t
    x = sample->x,
    y = sample->y;

  double
    alpha,
    beta,
    distance,
    subpath_alpha;

  EdgeInfo
    *p;

  ssize_t
    i,
    j;

  /*
    Compute fill & stroke opacity for this (x,y) point.
  */
  *stroke_alpha=0.0;
  subpath_alpha=0.0;
  p=polygon_info->edges;
  for (j=0; j < (ssize_t) polygon_info->number_edges; j++, p++)
  {
    if ((double) y <= (p->bounds.y1-mid-0.5))
      break;
    if ((double) y > (p->bounds.y2+mid+0.5))
      {
        p--;
        (void) DestroyEdge(polygon_info,j--);
        continue;
      }
    if (((double) x <= (p->bounds.x1-mid-0.5)) ||
        ((double) x > (p->bounds.x2+mid+0.5)))
      continue;
    i=(ssize_t) MagickMax((double) p->highwater,1.0);
    for ( ; i < (ssize_t) p->number_points; i++)
    {
      if ((double) y <= (p->points[i-1].y-mid-0.5))
        break;
      if ((double) y > (p->points[i].y+mid+0.5))
        continue;
      if (p->scanline != (double) y)
        {
          p->scanline=(double) y;
          p->highwater=(size_t) i;
        }
      distance=GetEdgeDistance(p,i,x,y);
      /*
        Compute stroke & subpath opacity.
      */
      beta=GetStrokeBeta(p,distance,mid,stroke_alpha);
      if (IsFillAlphaSkipped(fill,distance,subpath_alpha) != MagickFalse)
        continue;
      if (distance <= 0.0)
        {
          subpath_alpha=1.0;
          continue;
        }
      if (distance > 1.0)
        continue;
      beta=GetSubpathBeta(beta,distance);
      alpha=beta-1.0;
      if (subpath_alpha < (alpha*alpha))
        subpath_alpha=alpha*alpha;
    }
  }
  /*
    Compute fill opacity.
  */
  return(GetFillOpacity(polygon_info,sample,fill_rule,subpath_alpha));
}

static inline MagickBooleanType IsBoundsOutsideImage(const Image *image,
  const SegmentInfo bounds)
{
  return(((bounds.x1 >= (double) image->columns) ||
    (bounds.y1 >= (double) image->rows) ||
    (bounds.x2 <= 0.0) || (bounds.y2 <= 0.0)) ? MagickTrue : MagickFalse);
}

static SegmentInfo GetPolygonEdgesExtent(PolygonInfo **polygon_info,
  SegmentInfo bounds)
{
  EdgeInfo
    *p;

  ssize_t
    i;

  for (i=1; i < (ssize_t) polygon_info[0]->number_edges; i++)
  {
    p=polygon_info[0]->edges+i;
    if (p->bounds.x1 < bounds.x1)
      bounds.x1=p->bounds.x1;
    if (p->bounds.y1 < bounds.y1)
      bounds.y1=p->bounds.y1;
    if (p->bounds.x2 > bounds.x2)
      bounds.x2=p->bounds.x2;
    if (p->bounds.y2 > bounds.y2)
      bounds.y2=p->bounds.y2;
  }
  return(bounds);
}

static SegmentInfo ClampPolygonBounds(const Image *image,SegmentInfo bounds)
{
  bounds.x1=bounds.x1 < 0.0 ? 0.0 : bounds.x1 >= (double) image->columns-1.0 ?
    (double) image->columns-1.0 : bounds.x1;
  bounds.y1=bounds.y1 < 0.0 ? 0.0 : bounds.y1 >= (double) image->rows-1.0 ?
    (double) image->rows-1.0 : bounds.y1;
  bounds.x2=bounds.x2 < 0.0 ? 0.0 : bounds.x2 >= (double) image->columns-1.0 ?
    (double) image->columns-1.0 : bounds.x2;
  bounds.y2=bounds.y2 < 0.0 ? 0.0 : bounds.y2 >= (double) image->rows-1.0 ?
    (double) image->rows-1.0 : bounds.y2;
  return(bounds);
}

typedef struct _ExtentInfo
{
  ssize_t
    x1,
    y1,
    x2,
    y2;
} ExtentInfo;

typedef struct _PolygonPointRow
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  const PrimitiveInfo
    *primitive_info;

  ExtentInfo
    poly_extent;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} PolygonPointRow;

static void DrawPolygonPointRow(const PolygonPointRow *row,ssize_t x,
  Quantum *magick_restrict q)
{
  Image
    *const image = row->image;

  const DrawInfo
    *const draw_info = row->draw_info;

  const PrimitiveInfo
    *const primitive_info = row->primitive_info;

  const ExtentInfo
    poly_extent = row->poly_extent;

  const ssize_t
    y = row->y;

  ExceptionInfo
    *const exception = row->exception;

  PixelInfo
    pixel;

  GetPixelInfo(image,&pixel);
  for ( ; x <= poly_extent.x2; x++)
  {
    if ((x == CastDoubleToSsizeT(ceil(primitive_info->point.x-0.5))) &&
        (y == CastDoubleToSsizeT(ceil(primitive_info->point.y-0.5))))
      {
        GetFillColor(draw_info,x-poly_extent.x1,y-poly_extent.y1,&pixel,
          exception);
        SetPixelViaPixelInfo(image,&pixel,q);
      }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

static MagickBooleanType DrawPolygonPrimitive(Image *image,
  const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  const char
    *artifact;

  double
    mid;

  ExtentInfo
    poly_extent;

  MagickBooleanType
    fill,
    status;

  PolygonInfo
    **magick_restrict polygon_info;

  SegmentInfo
    bounds;

  size_t
    number_threads;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  assert(primitive_info != (PrimitiveInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (primitive_info->coordinates <= 1)
    return(MagickTrue);
  /*
    Compute bounding box.
  */
  polygon_info=AcquirePolygonTLS(primitive_info,exception);
  if (polygon_info == (PolygonInfo **) NULL)
    return(MagickFalse);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    begin draw-polygon");
  fill=(primitive_info->method == FillToBorderMethod) ||
    (primitive_info->method == FloodfillMethod) ? MagickTrue : MagickFalse;
  mid=ExpandAffine(&draw_info->affine)*draw_info->stroke_width/2.0;
  bounds=polygon_info[0]->edges[0].bounds;
  artifact=GetImageArtifact(image,"draw:render-bounding-rectangles");
  if (IsStringTrue(artifact) != MagickFalse)
    (void) DrawBoundingRectangles(image,draw_info,polygon_info[0],exception);
  bounds=GetPolygonEdgesExtent(polygon_info,bounds);
  bounds.x1-=(mid+1.0);
  bounds.y1-=(mid+1.0);
  bounds.x2+=(mid+1.0);
  bounds.y2+=(mid+1.0);
  if (IsBoundsOutsideImage(image,bounds) != MagickFalse)
    {
      polygon_info=DestroyPolygonTLS(polygon_info);
      return(MagickTrue);  /* virtual polygon */
    }
  bounds=ClampPolygonBounds(image,bounds);
  poly_extent.x1=CastDoubleToSsizeT(ceil(bounds.x1-0.5));
  poly_extent.y1=CastDoubleToSsizeT(ceil(bounds.y1-0.5));
  poly_extent.x2=CastDoubleToSsizeT(floor(bounds.x2+0.5));
  poly_extent.y2=CastDoubleToSsizeT(floor(bounds.y2+0.5));
  number_threads=(size_t) GetMagickNumberThreads(image,image,(size_t)
    (poly_extent.y2-poly_extent.y1+1),1);
  status=ClonePolygonEdgesTLS(polygon_info,number_threads,exception);
  if (status == MagickFalse)
    {
      polygon_info=DestroyPolygonTLS(polygon_info);
      return(status);
    }
  image_view=AcquireAuthenticCacheView(image,exception);
  if ((primitive_info->coordinates == 1) ||
      (polygon_info[0]->number_edges == 0))
    {
      /*
        Draw point.
      */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        num_threads((int) number_threads)
#endif
      for (y=poly_extent.y1; y <= poly_extent.y2; y++)
      {
        PolygonPointRow
          row;

        ssize_t
          x;

        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        x=poly_extent.x1;
        q=GetCacheViewAuthenticPixels(image_view,x,y,(size_t) (poly_extent.x2-
          x+1),1,exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        row.image=image;
        row.draw_info=draw_info;
        row.primitive_info=primitive_info;
        row.poly_extent=poly_extent;
        row.y=y;
        row.exception=exception;
        DrawPolygonPointRow(&row,x,q);
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      polygon_info=DestroyPolygonTLS(polygon_info);
      if (draw_info->debug != MagickFalse)
        (void) LogMagickEvent(DrawEvent,GetMagickModule(),
          "    end draw-polygon");
      return(status);
    }
  /*
    Draw polygon or line.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    num_threads((int) number_threads)
#endif
  for (y=poly_extent.y1; y <= poly_extent.y2; y++)
  {
    const int
      id = GetOpenMPThreadId();

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,poly_extent.x1,y,(size_t)
      (poly_extent.x2-poly_extent.x1+1),1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=poly_extent.x1; x <= poly_extent.x2; x++)
    {
      double
        fill_alpha,
        stroke_alpha;

      PixelInfo
        fill_color,
        stroke_color;

      /*
        Fill and/or stroke.
      */
      const FillAlphaSample sample = { mid, fill, x, y };
      fill_alpha=GetFillAlpha(polygon_info[id],&sample,draw_info->fill_rule,
        &stroke_alpha);
      if (draw_info->stroke_antialias == MagickFalse)
        {
          fill_alpha=fill_alpha >= AntialiasThreshold ? 1.0 : 0.0;
          stroke_alpha=stroke_alpha >= AntialiasThreshold ? 1.0 : 0.0;
        }
      GetFillColor(draw_info,x-poly_extent.x1,y-poly_extent.y1,&fill_color,
        exception);
      CompositePixelOver(image,&fill_color,fill_alpha*fill_color.alpha,q,
        (double) GetPixelAlpha(image,q),q);
      GetStrokeColor(draw_info,x-poly_extent.x1,y-poly_extent.y1,&stroke_color,
        exception);
      CompositePixelOver(image,&stroke_color,stroke_alpha*stroke_color.alpha,q,
        (double) GetPixelAlpha(image,q),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  polygon_info=DestroyPolygonTLS(polygon_info);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    end draw-polygon");
  return(status);
}

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
static void LogPrimitiveInfo(const PrimitiveInfo *primitive_info)
{
  const char
    *methods[] =
    {
      "point",
      "replace",
      "floodfill",
      "filltoborder",
      "reset",
      "?"
    };

  PointInfo
    p,
    point,
    q;

  ssize_t
    i,
    x;

  ssize_t
    coordinates,
    y;

  x=CastDoubleToSsizeT(ceil(primitive_info->point.x-0.5));
  y=CastDoubleToSsizeT(ceil(primitive_info->point.y-0.5));
  switch (primitive_info->primitive)
  {
    case AlphaPrimitive:
    {
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "AlphaPrimitive %.17g,%.17g %s",(double) x,(double) y,
        methods[primitive_info->method]);
      return;
    }
    case ColorPrimitive:
    {
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "ColorPrimitive %.17g,%.17g %s",(double) x,(double) y,
        methods[primitive_info->method]);
      return;
    }
    case ImagePrimitive:
    {
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "ImagePrimitive %.17g,%.17g",(double) x,(double) y);
      return;
    }
    case PointPrimitive:
    {
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "PointPrimitive %.17g,%.17g %s",(double) x,(double) y,
        methods[primitive_info->method]);
      return;
    }
    case TextPrimitive:
    {
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "TextPrimitive %.17g,%.17g",(double) x,(double) y);
      return;
    }
    default:
      break;
  }
  coordinates=0;
  p=primitive_info[0].point;
  q.x=(-1.0);
  q.y=(-1.0);
  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++)
  {
    point=primitive_info[i].point;
    if (coordinates <= 0)
      {
        coordinates=(ssize_t) primitive_info[i].coordinates;
        (void) LogMagickEvent(DrawEvent,GetMagickModule(),
          "    begin open (%.17g)",(double) coordinates);
        p=point;
      }
    point=primitive_info[i].point;
    if ((fabs(q.x-point.x) >= MagickEpsilon) ||
        (fabs(q.y-point.y) >= MagickEpsilon))
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "      %.17g: %.18g,%.18g",(double) coordinates,point.x,point.y);
    else
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "      %.17g: %g %g (duplicate)",(double) coordinates,point.x,point.y);
    q=point;
    coordinates--;
    if (coordinates > 0)
      continue;
    if ((fabs(p.x-point.x) >= MagickEpsilon) ||
        (fabs(p.y-point.y) >= MagickEpsilon))
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    end last (%.17g)",
        (double) coordinates);
    else
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    end open (%.17g)",
        (double) coordinates);
  }
}

static inline MagickBooleanType IsGrayImageWithColor(const Image *image,
  const DrawInfo *draw_info)
{
  return(((IsGrayColorspace(image->colorspace) != MagickFalse) &&
    ((IsPixelInfoGray(&draw_info->fill) == MagickFalse) ||
    (IsPixelInfoGray(&draw_info->stroke) == MagickFalse))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsVisibleWideStroke(const double mid,
  const DrawInfo *draw_info)
{
  return(((mid > 1.0) &&
    ((draw_info->stroke.alpha != (double) TransparentAlpha) ||
    (draw_info->stroke_pattern != (Image *) NULL))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsRoundStrokeOrPrimitive(
  const DrawInfo *draw_info,const MagickBooleanType closed_path,
  const PrimitiveInfo *primitive_info,const ssize_t i)
{
  return(((((draw_info->linecap == RoundCap) ||
    (closed_path != MagickFalse)) &&
    (draw_info->linejoin == RoundJoin)) ||
    (primitive_info[i].primitive != UndefinedPrimitive)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsCompositeOffset(const ssize_t x1,
  const ssize_t y1,const Image *composite_image)
{
  return((((x1 != 0L) && (x1 != (ssize_t) composite_image->columns)) ||
    ((y1 != 0L) && (y1 != (ssize_t) composite_image->rows))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsVisibleDashedStroke(const DrawInfo *draw_info,
  const double scale)
{
  return(((draw_info->dash_pattern != (double *) NULL) &&
    (fabs(draw_info->dash_pattern[0]) >= MagickEpsilon) &&
    (fabs(scale*draw_info->stroke_width) >= MagickEpsilon) &&
    (draw_info->stroke.alpha != (double) TransparentAlpha)) ? MagickTrue :
    MagickFalse);
}

typedef struct _PrimitiveScanInfo
{
  Image
    *image;

  CacheView
    *image_view;

  const DrawInfo
    *draw_info;

  ExceptionInfo
    *exception;
} PrimitiveScanInfo;

static MagickStatusType ReplaceAlphaPixels(const PrimitiveScanInfo *scan,
  const PixelInfo target,MagickStatusType status)
{
  Image
    *const image = scan->image;

  CacheView
    *const image_view = scan->image_view;

  const DrawInfo
    *const draw_info = scan->draw_info;

  ExceptionInfo
    *const exception = scan->exception;

  PixelInfo
    pixel;

  ssize_t
    x,
    y;

  GetPixelInfo(image,&pixel);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
        {
          q+=(ptrdiff_t) GetPixelChannels(image);
          continue;
        }
      GetFillColor(draw_info,x,y,&pixel,exception);
      SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
      exception);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

static MagickStatusType ReplaceColorPixels(const PrimitiveScanInfo *scan,
  const PixelInfo target,MagickStatusType status)
{
  Image
    *const image = scan->image;

  CacheView
    *const image_view = scan->image_view;

  const DrawInfo
    *const draw_info = scan->draw_info;

  ExceptionInfo
    *const exception = scan->exception;

  PixelInfo
    pixel;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
        {
          q+=(ptrdiff_t) GetPixelChannels(image);
          continue;
        }
      GetFillColor(draw_info,x,y,&pixel,exception);
      SetPixelViaPixelInfo(image,&pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
      exception);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

static MagickStatusType ResetAlphaPixels(const PrimitiveScanInfo *scan,
  MagickStatusType status)
{
  Image
    *const image = scan->image;

  CacheView
    *const image_view = scan->image_view;

  const DrawInfo
    *const draw_info = scan->draw_info;

  ExceptionInfo
    *const exception = scan->exception;

  PixelInfo
    pixel;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetFillColor(draw_info,x,y,&pixel,exception);
      SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
      exception);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

static MagickStatusType ResetColorPixels(const PrimitiveScanInfo *scan,
  MagickStatusType status)
{
  Image
    *const image = scan->image;

  CacheView
    *const image_view = scan->image_view;

  const DrawInfo
    *const draw_info = scan->draw_info;

  ExceptionInfo
    *const exception = scan->exception;

  PixelInfo
    pixel;

  ssize_t
    x,
    y;

  GetPixelInfo(image,&pixel);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetFillColor(draw_info,x,y,&pixel,exception);
      SetPixelViaPixelInfo(image,&pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
      exception);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

typedef struct _DrawPrimitiveContext
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  const PrimitiveInfo
    *primitive_info;

  CacheView
    *image_view;

  ssize_t
    x,
    y;

  ExceptionInfo
    *exception;
} DrawPrimitiveContext;

static void SetFillToBorderTarget(const PrimitiveInfo *primitive_info,
  const DrawInfo *draw_info,PixelInfo *target)
{
  if (primitive_info->method == FillToBorderMethod)
    {
      (*target).red=(double) draw_info->border_color.red;
      (*target).green=(double) draw_info->border_color.green;
      (*target).blue=(double) draw_info->border_color.blue;
    }
}

static MagickStatusType EnsureBlendAlphaChannel(Image *image,
  MagickStatusType status,ExceptionInfo *exception)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    status&=(MagickStatusType) SetImageAlphaChannel(image,
      OpaqueAlphaChannel,exception);
  return(status);
}

static MagickStatusType DrawAlphaPoint(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  PixelInfo
    pixel;

  Quantum
    *q;

  q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
  if (q == (Quantum *) NULL)
    return(status);
  GetFillColor(draw_info,x,y,&pixel,exception);
  SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
  status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
    exception);
  return(status);
}

static MagickStatusType DrawAlphaPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  status=EnsureBlendAlphaChannel(image,status,exception);
  switch (primitive_info->method)
  {
    case PointMethod:
    default:
    {
      status=DrawAlphaPoint(context,status);
      break;
    }
    case ReplaceMethod:
    {
      PixelInfo
        target;

      PrimitiveScanInfo
        scan;

      status&=(MagickStatusType) GetOneCacheViewVirtualPixelInfo(image_view,
        x,y,&target,exception);
      scan.image=image;
      scan.image_view=image_view;
      scan.draw_info=draw_info;
      scan.exception=exception;
      status=ReplaceAlphaPixels(&scan,target,status);
      break;
    }
    case FloodfillMethod:
    case FillToBorderMethod:
    {
      ChannelType
        channel_mask;

      PixelInfo
        target;

      status&=(MagickStatusType) GetOneVirtualPixelInfo(image,
        TileVirtualPixelMethod,x,y,&target,exception);
      SetFillToBorderTarget(primitive_info,draw_info,&target);
      channel_mask=SetImageChannelMask(image,AlphaChannel);
      status&=(MagickStatusType) FloodfillPaintImage(image,draw_info,
        &target,x,y,primitive_info->method == FloodfillMethod ?
        MagickFalse : MagickTrue,exception);
      (void) SetImageChannelMask(image,channel_mask);
      break;
    }
    case ResetMethod:
    {
      const PrimitiveScanInfo
        scan = { image, image_view, draw_info, exception };

      status=ResetAlphaPixels(&scan,status);
      break;
    }
  }
  return(status);
}

static MagickStatusType DrawColorPoint(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  PixelInfo
    pixel;

  Quantum
    *q;

  q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
  if (q == (Quantum *) NULL)
    return(status);
  GetPixelInfo(image,&pixel);
  GetFillColor(draw_info,x,y,&pixel,exception);
  SetPixelViaPixelInfo(image,&pixel,q);
  status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
    exception);
  return(status);
}

static MagickStatusType DrawColorPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  switch (primitive_info->method)
  {
    case PointMethod:
    default:
    {
      status=DrawColorPoint(context,status);
      break;
    }
    case ReplaceMethod:
    {
      PixelInfo
        target;

      PrimitiveScanInfo
        scan;

      status&=(MagickStatusType) GetOneCacheViewVirtualPixelInfo(image_view,
        x,y,&target,exception);
      scan.image=image;
      scan.image_view=image_view;
      scan.draw_info=draw_info;
      scan.exception=exception;
      status=ReplaceColorPixels(&scan,target,status);
      break;
    }
    case FloodfillMethod:
    case FillToBorderMethod:
    {
      PixelInfo
        target;

      status&=(MagickStatusType) GetOneVirtualPixelInfo(image,
        TileVirtualPixelMethod,x,y,&target,exception);
      SetFillToBorderTarget(primitive_info,draw_info,&target);
      status&=(MagickStatusType) FloodfillPaintImage(image,draw_info,
        &target,x,y,primitive_info->method == FloodfillMethod ?
        MagickFalse : MagickTrue,exception);
      break;
    }
    case ResetMethod:
    {
      PrimitiveScanInfo
        scan;

      scan.image=image;
      scan.image_view=image_view;
      scan.draw_info=draw_info;
      scan.exception=exception;
      status=ResetColorPixels(&scan,status);
      break;
    }
  }
  return(status);
}

static Image *ReadCompositeImages(const PrimitiveInfo *primitive_info,
  const DrawInfo *draw_info,ExceptionInfo *exception)
{
  Image
    *composite_images;

  ImageInfo
    *clone_info;

  clone_info=AcquireImageInfo();
  composite_images=(Image *) NULL;
  if (LocaleNCompare(primitive_info->text,"data:",5) == 0)
    composite_images=ReadInlineImage(clone_info,primitive_info->text,
      exception);
  else
    if (*primitive_info->text != '\0')
      {
        /*
          Read composite image.
        */
        (void) CopyMagickString(clone_info->filename,primitive_info->text,
          MagickPathExtent);
        (void) SetImageInfo(clone_info,1,exception);
        (void) CopyMagickString(clone_info->filename,primitive_info->text,
          MagickPathExtent);
        clone_info->size=DestroyString(clone_info->size);
        clone_info->extract=DestroyString(clone_info->extract);
        clone_info->coder_depth=draw_info->image_info->coder_depth;
        composite_images=StrictReadImage(clone_info,exception);
        draw_info->image_info->coder_depth=clone_info->coder_depth;
      }
  clone_info=DestroyImageInfo(clone_info);
  return(composite_images);
}

static MagickStatusType CompositeDrawImage(const DrawPrimitiveContext *context,
  Image *composite_image,MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  AffineMatrix
    affine;

  char
    composite_geometry[MagickPathExtent];

  RectangleInfo
    geometry;

  SetGeometry(image,&geometry);
  image->gravity=draw_info->gravity;
  geometry.x=x;
  geometry.y=y;
  (void) FormatLocaleString(composite_geometry,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) composite_image->columns,(double)
    composite_image->rows,(double) geometry.x,(double) geometry.y);
  (void) ParseGravityGeometry(image,composite_geometry,&geometry,exception);
  affine=draw_info->affine;
  affine.tx=(double) geometry.x;
  affine.ty=(double) geometry.y;
  composite_image->interpolate=image->interpolate;
  if ((draw_info->compose == OverCompositeOp) ||
      (draw_info->compose == SrcOverCompositeOp))
    status&=(MagickStatusType) DrawAffineImage(image,composite_image,
      &affine,exception);
  else
    status&=(MagickStatusType) CompositeImage(image,composite_image,
      draw_info->compose,MagickTrue,geometry.x,geometry.y,exception);
  return(status);
}

static MagickStatusType DrawImagePrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  char
    composite_geometry[MagickPathExtent];

  Image
    *composite_image,
    *composite_images;

  ssize_t
    x1,
    y1;

  if (primitive_info->text == (char *) NULL)
    return(status);
  composite_images=ReadCompositeImages(primitive_info,draw_info,exception);
  if (composite_images == (Image *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  composite_image=RemoveFirstImageFromList(&composite_images);
  composite_images=DestroyImageList(composite_images);
  (void) SetImageProgressMonitor(composite_image,(MagickProgressMonitor)
    NULL,(void *) NULL);
  x1=CastDoubleToSsizeT(ceil(primitive_info[1].point.x-0.5));
  y1=CastDoubleToSsizeT(ceil(primitive_info[1].point.y-0.5));
  if (IsCompositeOffset(x1,y1,composite_image) != MagickFalse)
    {
      /*
        Resize image.
      */
      (void) FormatLocaleString(composite_geometry,MagickPathExtent,
        "%gx%g!",primitive_info[1].point.x,primitive_info[1].point.y);
      composite_image->filter=image->filter;
      status&=(MagickStatusType) TransformImage(&composite_image,
        (char *) NULL,composite_geometry,exception);
    }
  if (composite_image->alpha_trait == UndefinedPixelTrait)
    status&=(MagickStatusType) SetImageAlphaChannel(composite_image,
      OpaqueAlphaChannel,exception);
  if (draw_info->alpha != OpaqueAlpha)
    status&=(MagickStatusType) SetImageAlpha(composite_image,
      draw_info->alpha,exception);
  status=CompositeDrawImage(context,composite_image,status);
  composite_image=DestroyImage(composite_image);
  return(status);
}

static MagickStatusType DrawPointPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  PixelInfo
    fill_color;

  Quantum
    *q;

  if ((y < 0) || (y >= (ssize_t) image->rows))
    return(status);
  if ((x < 0) || (x >= (ssize_t) image->columns))
    return(status);
  q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
  if (q == (Quantum *) NULL)
    return(status);
  GetFillColor(draw_info,x,y,&fill_color,exception);
  CompositePixelOver(image,&fill_color,(double) fill_color.alpha,q,(double)
    GetPixelAlpha(image,q),q);
  status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
    exception);
  return(status);
}

static MagickStatusType DrawUnstrokedPolygon(
  const DrawPrimitiveContext *context,MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  DrawInfo
    *clone_info;

  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  clone_info->stroke_width=0.0;
  clone_info->stroke.alpha=(MagickRealType) TransparentAlpha;
  status&=(MagickStatusType) DrawPolygonPrimitive(image,clone_info,
    primitive_info,exception);
  clone_info=DestroyDrawInfo(clone_info);
  return(status);
}

static MagickStatusType DrawTextPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  char
    geometry[MagickPathExtent];

  DrawInfo
    *clone_info;

  if (primitive_info->text == (char *) NULL)
    return(status);
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  (void) CloneString(&clone_info->text,primitive_info->text);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%+f%+f",
    primitive_info->point.x,primitive_info->point.y);
  (void) CloneString(&clone_info->geometry,geometry);
  status&=(MagickStatusType) AnnotateImage(image,clone_info,exception);
  clone_info=DestroyDrawInfo(clone_info);
  return(status);
}

static MagickStatusType DrawWideStrokedPrimitive(
  const DrawPrimitiveContext *context,MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  double
    point_x,
    point_y;

  MagickBooleanType
    closed_path;

  ssize_t
    i;

  /*
    Draw strokes while respecting line cap/join attributes.
  */
  closed_path=primitive_info[0].closed_subpath;
  i=(ssize_t) primitive_info[0].coordinates;
  point_x=fabs(primitive_info[i-1].point.x-primitive_info[0].point.x);
  point_y=fabs(primitive_info[i-1].point.y-primitive_info[0].point.y);
  if (IsClosedEllipse(point_x,point_y) != MagickFalse)
    closed_path=MagickTrue;
  if (IsRoundStrokeOrPrimitive(draw_info,closed_path,primitive_info,i) !=
      MagickFalse)
    {
      status&=(MagickStatusType) DrawPolygonPrimitive(image,draw_info,
        primitive_info,exception);
      return(status);
    }
  status=DrawUnstrokedPolygon(context,status);
  if (status != MagickFalse)
    status&=(MagickStatusType) DrawStrokePolygon(image,draw_info,
      primitive_info,exception);
  return(status);
}

static MagickStatusType DrawStrokedPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  double
    mid,
    scale;


  if (IsEventLogging() != MagickFalse)
    LogPrimitiveInfo(primitive_info);
  scale=ExpandAffine(&draw_info->affine);
  if (IsVisibleDashedStroke(draw_info,scale) != MagickFalse)
    {
      /*
        Draw dash polygon.
      */
      status=DrawUnstrokedPolygon(context,status);
      if (status != MagickFalse)
        status&=(MagickStatusType) DrawDashPolygon(draw_info,primitive_info,
          image,exception);
      return(status);
    }
  mid=ExpandAffine(&draw_info->affine)*draw_info->stroke_width/2.0;
  if (IsVisibleWideStroke(mid,draw_info) != MagickFalse)
    return(DrawWideStrokedPrimitive(context,status));
  status&=(MagickStatusType) DrawPolygonPrimitive(image,draw_info,
    primitive_info,exception);
  return(status);
}

static MagickStatusType DrawPrimitiveByType(
  const DrawPrimitiveContext *context,MagickStatusType status)
{
  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  switch (primitive_info->primitive)
  {
    case AlphaPrimitive:
    {
      status=DrawAlphaPrimitive(context,status);
      break;
    }
    case ColorPrimitive:
    {
      status=DrawColorPrimitive(context,status);
      break;
    }
    case ImagePrimitive:
    {
      status=DrawImagePrimitive(context,status);
      break;
    }
    case PointPrimitive:
    {
      status=DrawPointPrimitive(context,status);
      break;
    }
    case TextPrimitive:
    {
      status=DrawTextPrimitive(context,status);
      break;
    }
    default:
    {
      status=DrawStrokedPrimitive(context,status);
      break;
    }
  }
  return(status);
}

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
  for (i=1; i < (ssize_t) number_vertices; i++)
  {
    dx=primitive_info[0].point.x-primitive_info[i].point.x;
    dy=primitive_info[0].point.y-primitive_info[i].point.y;
    if ((fabs((double) dx) >= MagickEpsilon) ||
        (fabs((double) dy) >= MagickEpsilon))
      break;
  }
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

  StrokeSubpathInput
    subpath;

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

  StrokePolygonInput
    assembly;

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
  dx.p=0.0;
  dy.p=0.0;
  for (n=1; n < (ssize_t) number_vertices; n++)
  {
    dx.p=polygon_primitive[n].point.x-polygon_primitive[0].point.x;
    dy.p=polygon_primitive[n].point.y-polygon_primitive[0].point.y;
    if ((fabs(dx.p) >= MagickEpsilon) || (fabs(dy.p) >= MagickEpsilon))
      break;
  }
  subpath.draw_info=draw_info;
  subpath.polygon_primitive=polygon_primitive;
  subpath.number_vertices=number_vertices;
  subpath.closed_path=closed_path;
  if (TraceZeroLengthSubpath(&subpath,&n,&returned) != MagickFalse)
    return(returned);
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
  assembly.polygon_primitive=polygon_primitive;
  assembly.stroke_p=stroke_p;
  assembly.stroke_q=stroke_q;
  assembly.p=p;
  assembly.q=q;
  assembly.closed_path=closed_path;
  AssembleStrokePolygon(&assembly,stroke_polygon);
  stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p);
  stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q);
  polygon_primitive=(PrimitiveInfo *) RelinquishMagickMemory(polygon_primitive);
  return(stroke_polygon);
}
