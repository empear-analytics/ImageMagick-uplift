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

  MagickCore private draw primitive path and polygon methods.
*/
#ifndef MAGICKCORE_DRAW_PRIMITIVE_PATH_PRIVATE_H
#define MAGICKCORE_DRAW_PRIMITIVE_PATH_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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

static PolygonInfo *ThrowPolygonPointsFailure(PolygonInfo *polygon_info,
  PointInfo *points,ExceptionInfo *exception)
{
  (void) ThrowMagickException(exception,GetMagickModule(),
    ResourceLimitError,"MemoryAllocationFailed","`%s'","");
  points=(PointInfo *) RelinquishMagickMemory(points);
  return(DestroyPolygonInfo(polygon_info));
}

static MagickBooleanType GrowEdgePoints(PointInfo **points,const ssize_t n,
  size_t *number_points)
{
  if (n == (ssize_t) (*number_points))
    {
      (*number_points)<<=1;
      (*points)=(PointInfo *) ResizeQuantumMemory((*points),(size_t)
        (*number_points),sizeof(*(*points)));
      if ((*points) == (PointInfo *) NULL)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType AcquireEdgePoints(PointInfo **points,
  size_t *number_points)
{
  if ((*points) == (PointInfo *) NULL)
    {
      (*number_points)=16;
      (*points)=(PointInfo *) AcquireQuantumMemory((size_t) (*number_points),
        sizeof(*(*points)));
    }
  if ((*points) == (PointInfo *) NULL)
    return(MagickFalse);
  return(MagickTrue);
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
          return(ThrowPolygonPointsFailure(polygon_info,points,exception));
        if (AcquireEdgePoints(&points,&number_points) == MagickFalse)
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
          return(ThrowPolygonPointsFailure(polygon_info,points,exception));
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
    if (GrowEdgePoints(&points,n,&number_points) == MagickFalse)
      return(ThrowPolygonAllocationFailure(polygon_info,exception));
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

static PathInfo *AcquirePrimitivePathInfo(const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  PathInfo
    *path_info;

  size_t
    i;

  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++) ;
  path_info=(PathInfo *) AcquireQuantumMemory((size_t) (3UL*i+1UL),
    sizeof(*path_info));
  if (path_info == (PathInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'","");
  return(path_info);
}

static ssize_t AppendNewSubpathPoint(const NewSubpathPointInput *subpath,
  PathInfo *path_info,ssize_t n,PointInfo *q)
{
  const PathInfoCode
    code = subpath->code;

  const PrimitiveInfo
    *const primitive_info = subpath->primitive_info;

  const size_t
    i = subpath->i;

  if (IsNewSubpathPoint(subpath) != MagickFalse)
    {
      /*
        Eliminate duplicate points.
      */
      path_info[n].code=code;
      path_info[n].point=primitive_info[i].point;
      *q=primitive_info[i].point;
      n++;
    }
  return(n);
}

typedef struct _OpenSubpathInput
{
  PathInfo
    *path_info;

  ssize_t
    start;

  PointInfo
    point,
    p;
} OpenSubpathInput;

static ssize_t AppendOpenSubpathEnd(const OpenSubpathInput *open_subpath,
  ssize_t n)
{
  PathInfo
    *const path_info = open_subpath->path_info;

  const ssize_t
    start = open_subpath->start;

  const PointInfo
    point = open_subpath->point,
    p = open_subpath->p;

  path_info[start].code=OpenCode;
  path_info[n].code=GhostlineCode;
  path_info[n].point=point;
  n++;
  path_info[n].code=LineToCode;
  path_info[n].point=p;
  n++;
  return(n);
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
  path_info=AcquirePrimitivePathInfo(primitive_info,exception);
  if (path_info == (PathInfo *) NULL)
    return((PathInfo *) NULL);
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
    {
      const NewSubpathPointInput
        subpath = { code, coordinates, q, primitive_info, i };

      n=AppendNewSubpathPoint(&subpath,path_info,n,&q);
    }
    if (coordinates > 0)
      continue;  /* next point in current subpath */
    if (closed_subpath != MagickFalse)
      {
        closed_subpath=MagickFalse;
      }
    else
      {
        const OpenSubpathInput
          open_subpath = { path_info, start, primitive_info[i].point, p };

        /*
          Mark the p point as open if the subpath is not closed.
        */
        n=AppendOpenSubpathEnd(&open_subpath,n);
      }
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

static MagickBooleanType AcquireDashPolygon(
  const PrimitiveInfo *primitive_info,size_t *number_vertices,
  PrimitiveInfo **dash_polygon,ExceptionInfo *exception)
{
  ssize_t
    i;

  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++) ;
  (*number_vertices)=(size_t) i;
  (*dash_polygon)=(PrimitiveInfo *) AcquireQuantumMemory((size_t)
    (2UL*(*number_vertices)+32UL),sizeof(*(*dash_polygon)));
  if ((*dash_polygon) == (PrimitiveInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return(MagickFalse);
    }
  (void) memset((*dash_polygon),0,(2UL*(*number_vertices)+32UL)*
    sizeof(*(*dash_polygon)));
  return(MagickTrue);
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

  const DashSegmentTargets
    targets = { (&j), (&status) };

  assert(draw_info != (const DrawInfo *) NULL);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    begin draw-dash");
  if (AcquireDashPolygon(primitive_info,&number_vertices,&dash_polygon,
      exception) == MagickFalse)
    return(MagickFalse);
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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
