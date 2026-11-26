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

  MagickCore private methods of draw.c: the primitive tracers, Permutate to
  TraceRoundRectangle (arcs, Bezier curves, ellipses, lines and SVG paths);
  included by that file only.
*/
#ifndef MAGICKCORE_DRAW_PATH_TRACE_PRIVATE_H
#define MAGICKCORE_DRAW_PATH_TRACE_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   P e r m u t a t e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Permutate() returns the permutation of the (n,k).
%
%  The format of the Permutate method is:
%
%      void Permutate(ssize_t n,ssize_t k)
%
%  A description of each parameter follows:
%
%    o n:
%
%    o k:
%
%
*/
static inline double Permutate(const ssize_t n,const ssize_t k)
{
  double
    r;

  ssize_t
    i;

  r=1.0;
  for (i=k+1; i <= n; i++)
    r*=i;
  for (i=1; i <= (n-k); i++)
    r/=i;
  return(r);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   T r a c e P r i m i t i v e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TracePrimitive is a collection of methods for generating graphic
%  primitives such as arcs, ellipses, paths, etc.
%
*/

static MagickBooleanType TraceArc(MVGInfo *mvg_info,const PointInfo start,
  const PointInfo end,const PointInfo degrees)
{
  PointInfo
    center,
    radius;

  center.x=0.5*(end.x+start.x);
  center.y=0.5*(end.y+start.y);
  radius.x=fabs(center.x-start.x);
  radius.y=fabs(center.y-start.y);
  return(TraceEllipse(mvg_info,center,radius,degrees));
}

static double GetArcSweepTheta(double theta,const MagickBooleanType sweep)
{
  if ((theta < 0.0) && (sweep != MagickFalse))
    theta+=2.0*MagickPI;
  else
    if ((theta > 0.0) && (sweep == MagickFalse))
      theta-=2.0*MagickPI;
  return(theta);
}

static double GetArcCenterFactor(const double alpha,const double beta,
  const MagickBooleanType large_arc,const MagickBooleanType sweep)
{
  double
    factor;

  factor=MagickSafeReciprocal(alpha*alpha+beta*beta)-0.25;
  if (factor <= 0.0)
    factor=0.0;
  else
    {
      factor=sqrt((double) factor);
      if (sweep == large_arc)
        factor=(-factor);
    }
  return(factor);
}

static MagickBooleanType FinishArcPath(MVGInfo *mvg_info,
  const MagickStatusType status,const ssize_t offset,PrimitiveInfo *p)
{
  PrimitiveInfo
    *primitive_info;

  if (status == 0)
    return(MagickFalse);
  mvg_info->offset=offset;
  primitive_info=(*mvg_info->primitive_info)+mvg_info->offset;
  primitive_info->coordinates=(size_t) (p-primitive_info);
  primitive_info->closed_subpath=MagickFalse;
  SetTracedPrimitiveType(primitive_info,p);
  return(MagickTrue);
}

typedef struct _ArcSegmentInput
{
  double
    alpha,
    theta;

  size_t
    arc_segments;

  PointInfo
    center;

  double
    cosine,
    sine;

  PointInfo
    radii,
    start,
    end;

  PrimitiveInfo
    *primitive_info;
} ArcSegmentInput;

static void TraceArcSegmentPoints(const ArcSegmentInput *segment,
  const ssize_t i,PrimitiveInfo *p,PointInfo *points)
{
  const double
    alpha = segment->alpha,
    theta = segment->theta;

  const size_t
    arc_segments = segment->arc_segments;

  const PointInfo
    center = segment->center;

  const double
    cosine = segment->cosine,
    sine = segment->sine;

  const PointInfo
    radii = segment->radii,
    start = segment->start,
    end = segment->end;

  PrimitiveInfo
    *const primitive_info = segment->primitive_info;

  double
    beta,
    gamma;

  beta=0.5*((alpha+(i+1)*theta/arc_segments)-(alpha+i*theta/arc_segments));
  gamma=(8.0/3.0)*sin(fmod((double) (0.5*beta),DegreesToRadians(360.0)))*
    sin(fmod((double) (0.5*beta),DegreesToRadians(360.0)))/
    sin(fmod((double) beta,DegreesToRadians(360.0)));
  points[0].x=(double) (center.x+cos(fmod((double) (alpha+(double) i*theta/
    arc_segments),DegreesToRadians(360.0)))-gamma*sin(fmod((double) (alpha+
    (double) i*theta/arc_segments),DegreesToRadians(360.0))));
  points[0].y=(double) (center.y+sin(fmod((double) (alpha+(double) i*theta/
    arc_segments),DegreesToRadians(360.0)))+gamma*cos(fmod((double) (alpha+
    (double) i*theta/arc_segments),DegreesToRadians(360.0))));
  points[2].x=(double) (center.x+cos(fmod((double) (alpha+(double) (i+1)*
    theta/arc_segments),DegreesToRadians(360.0))));
  points[2].y=(double) (center.y+sin(fmod((double) (alpha+(double) (i+1)*
    theta/arc_segments),DegreesToRadians(360.0))));
  points[1].x=(double) (points[2].x+gamma*sin(fmod((double) (alpha+(double)
    (i+1)*theta/arc_segments),DegreesToRadians(360.0))));
  points[1].y=(double) (points[2].y-gamma*cos(fmod((double) (alpha+(double)
    (i+1)*theta/arc_segments),DegreesToRadians(360.0))));
  p->point.x=(p == primitive_info) ? start.x : (p-1)->point.x;
  p->point.y=(p == primitive_info) ? start.y : (p-1)->point.y;
  (p+1)->point.x=(double) (cosine*radii.x*points[0].x-sine*radii.y*
    points[0].y);
  (p+1)->point.y=(double) (sine*radii.x*points[0].x+cosine*radii.y*
    points[0].y);
  (p+2)->point.x=(double) (cosine*radii.x*points[1].x-sine*radii.y*
    points[1].y);
  (p+2)->point.y=(double) (sine*radii.x*points[1].x+cosine*radii.y*
    points[1].y);
  (p+3)->point.x=(double) (cosine*radii.x*points[2].x-sine*radii.y*
    points[2].y);
  (p+3)->point.y=(double) (sine*radii.x*points[2].x+cosine*radii.y*
    points[2].y);
  if (i == (ssize_t) (arc_segments-1))
    (p+3)->point=end;
}

static PointInfo GetArcMidpointOffset(const PointInfo start,
  const PointInfo end,const double cosine,const double sine)
{
  PointInfo
    center;

  center.x=(double) (cosine*(end.x-start.x)/2+sine*(end.y-start.y)/2);
  center.y=(double) (cosine*(end.y-start.y)/2-sine*(end.x-start.x)/2);
  return(center);
}

static double GetArcRadiiDelta(const PointInfo center,const PointInfo radii)
{
  double
    delta;

  delta=(center.x*center.x)/(radii.x*radii.x)+(center.y*center.y)/
    (radii.y*radii.y);
  return(delta);
}

static PointInfo GetArcUnitPoint(const PointInfo point,const double cosine,
  const double sine,const PointInfo radii)
{
  PointInfo
    unit_point;

  unit_point.x=(double) (cosine*point.x/radii.x+sine*point.y/radii.x);
  unit_point.y=(double) (cosine*point.y/radii.y-sine*point.x/radii.y);
  return(unit_point);
}

static PointInfo GetArcCenter(const PointInfo *points,const double factor,
  const double alpha,const double beta)
{
  PointInfo
    center;

  center.x=(double) ((points[0].x+points[1].x)/2-factor*beta);
  center.y=(double) ((points[0].y+points[1].y)/2+factor*alpha);
  return(center);
}

static size_t GetArcSegmentCount(const double theta)
{
  size_t
    arc_segments;

  arc_segments=(size_t) CastDoubleToSsizeT(ceil(fabs((double) (theta/(0.5*
    MagickPI+MagickEpsilon)))));
  return(arc_segments);
}

static PointInfo ScaleArcRadii(PointInfo radii,const double delta)
{
  if (delta > 1.0)
    {
      radii.x*=sqrt((double) delta);
      radii.y*=sqrt((double) delta);
    }
  return(radii);
}

static MagickStatusType TraceArcSegments(MVGInfo *mvg_info,
  const ArcSegmentInput *segment,PointInfo *points,PrimitiveInfo **p)
{
  const size_t
    arc_segments = segment->arc_segments;

  MagickStatusType
    status;

  ssize_t
    i;

  status=MagickTrue;
  for (i=0; i < (ssize_t) arc_segments; i++)
  {
    TraceArcSegmentPoints(segment,i,(*p),points);
    status&=(MagickStatusType) TraceBezier(mvg_info,4);
    if (status == 0)
      break;
    (*p)=(*mvg_info->primitive_info)+(ptrdiff_t) mvg_info->offset;
    mvg_info->offset+=(ssize_t) (*p)->coordinates;
    (*p)+=(ptrdiff_t) (*p)->coordinates;
  }
  return(status);
}

static MagickBooleanType TraceArcPath(MVGInfo *mvg_info,const PointInfo start,
  const PointInfo end,const ArcPathShape *shape)
{
  const PointInfo
    arc = shape->arc;

  const double
    angle = shape->angle;

  const MagickBooleanType
    large_arc = shape->large_arc,
    sweep = shape->sweep;

  double
    alpha,
    beta,
    delta,
    factor,
    theta,
    cosine,
    sine;

  MagickStatusType
    status;

  PointInfo
    center,
    points[3],
    radii;

  PrimitiveInfo
    *primitive_info,
    *p;

  size_t
    arc_segments;

  ssize_t
    offset;

  offset=mvg_info->offset;
  primitive_info=(*mvg_info->primitive_info)+mvg_info->offset;
  primitive_info->coordinates=0;
  if ((fabs(start.x-end.x) < MagickEpsilon) &&
      (fabs(start.y-end.y) < MagickEpsilon))
    return(TracePoint(primitive_info,end));
  radii.x=fabs(arc.x);
  radii.y=fabs(arc.y);
  if ((radii.x < MagickEpsilon) || (radii.y < MagickEpsilon))
    return(TraceLine(primitive_info,start,end));
  cosine=cos(DegreesToRadians(fmod((double) angle,360.0)));
  sine=sin(DegreesToRadians(fmod((double) angle,360.0)));
  center=GetArcMidpointOffset(start,end,cosine,sine);
  delta=GetArcRadiiDelta(center,radii);
  if (delta < MagickEpsilon)
    return(TraceLine(primitive_info,start,end));
  radii=ScaleArcRadii(radii,delta);
  points[0]=GetArcUnitPoint(start,cosine,sine,radii);
  points[1]=GetArcUnitPoint(end,cosine,sine,radii);
  alpha=points[1].x-points[0].x;
  beta=points[1].y-points[0].y;
  if (fabs(alpha*alpha+beta*beta) < MagickEpsilon)
    return(TraceLine(primitive_info,start,end));
  factor=GetArcCenterFactor(alpha,beta,large_arc,sweep);
  center=GetArcCenter(points,factor,alpha,beta);
  alpha=atan2(points[0].y-center.y,points[0].x-center.x);
  theta=atan2(points[1].y-center.y,points[1].x-center.x)-alpha;
  theta=GetArcSweepTheta(theta,sweep);
  arc_segments=GetArcSegmentCount(theta);
  p=primitive_info;
  {
    const ArcSegmentInput
      segment = { alpha, theta, arc_segments, center, cosine, sine, radii,
        start, end, primitive_info };

    status=TraceArcSegments(mvg_info,&segment,points,&p);
  }
  return(FinishArcPath(mvg_info,status,offset,p));
}

static MagickBooleanType GetBezierQuantum(const PrimitiveInfo *primitive_info,
  const size_t number_coordinates,size_t *quantum)
{
  double
    alpha;

  ssize_t
    i,
    j;

  for (i=0; i < (ssize_t) number_coordinates; i++)
  {
    for (j=i+1; j < (ssize_t) number_coordinates; j++)
    {
      alpha=fabs(primitive_info[j].point.x-primitive_info[i].point.x);
      if (alpha > (double) GetMaxMemoryRequest())
        return(MagickFalse);
      if (alpha > (double) (*quantum))
        (*quantum)=(size_t) alpha;
      alpha=fabs(primitive_info[j].point.y-primitive_info[i].point.y);
      if (alpha > (double) (*quantum))
        (*quantum)=(size_t) alpha;
    }
  }
  return(MagickTrue);
}

static PointInfo GetBezierPoint(const PrimitiveInfo *primitive_info,
  const double *coefficients,const size_t number_coordinates,
  const double weight)
{
  double
    alpha;

  PointInfo
    point;

  const PrimitiveInfo
    *p;

  ssize_t
    j;

  p=primitive_info;
  point.x=0.0;
  point.y=0.0;
  alpha=pow((double) (1.0-weight),(double) number_coordinates-1.0);
  for (j=0; j < (ssize_t) number_coordinates; j++)
  {
    point.x+=alpha*coefficients[j]*p->point.x;
    point.y+=alpha*coefficients[j]*p->point.y;
    alpha*=weight/(1.0-weight);
    p++;
  }
  return(point);
}

static MagickBooleanType TraceBezierControlPoints(
  PrimitiveInfo *primitive_info,const PointInfo *points,
  const size_t control_points,PrimitiveInfo **p)
{
  ssize_t
    i;

  *p=primitive_info;
  for (i=0; i < (ssize_t) control_points; i++)
  {
    if (TracePoint(*p,points[i]) == MagickFalse)
      return(MagickFalse);
    *p+=(ptrdiff_t) (*p)->coordinates;
  }
  return(MagickTrue);
}

static void SetBezierCoefficients(double *coefficients,
  const size_t number_coordinates)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) number_coordinates; i++)
    coefficients[i]=Permutate((ssize_t) number_coordinates-1,i);
}

static MagickBooleanType ReleaseBezierBuffers(PointInfo *points,
  double *coefficients)
{
  points=(PointInfo *) RelinquishMagickMemory(points);
  coefficients=(double *) RelinquishMagickMemory(coefficients);
  return(MagickFalse);
}

static MagickBooleanType TraceBezier(MVGInfo *mvg_info,
  const size_t number_coordinates)
{
  double
    *coefficients,
    weight;

  PointInfo
    end,
    point,
    *points;

  PrimitiveInfo
    *primitive_info;

  PrimitiveInfo
    *p;

  ssize_t
    i;

  size_t
    control_points,
    quantum;

  /*
    Allocate coefficients.
  */
  primitive_info=(*mvg_info->primitive_info)+mvg_info->offset;
  quantum=number_coordinates;
  if (GetBezierQuantum(primitive_info,number_coordinates,&quantum) ==
      MagickFalse)
    {
      (void) ThrowMagickException(mvg_info->exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return(MagickFalse);
    }
  primitive_info=(*mvg_info->primitive_info)+mvg_info->offset;
  quantum=MagickMin(quantum/number_coordinates,BezierQuantum);
  coefficients=(double *) AcquireQuantumMemory(number_coordinates,
    sizeof(*coefficients));
  points=(PointInfo *) AcquireQuantumMemory(quantum,number_coordinates*
    sizeof(*points));
  if ((coefficients == (double *) NULL) || (points == (PointInfo *) NULL))
    {
      points=(PointInfo *) RelinquishMagickMemory(points);
      coefficients=(double *) RelinquishMagickMemory(coefficients);
      (void) ThrowMagickException(mvg_info->exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return(MagickFalse);
    }
  control_points=quantum*number_coordinates;
  if (CheckPrimitiveExtent(mvg_info,(double) control_points+1) == MagickFalse)
    return(ReleaseBezierBuffers(points,coefficients));
  primitive_info=(*mvg_info->primitive_info)+mvg_info->offset;
  /*
    Compute bezier points.
  */
  end=primitive_info[number_coordinates-1].point;
  SetBezierCoefficients(coefficients,number_coordinates);
  weight=0.0;
  for (i=0; i < (ssize_t) control_points; i++)
  {
    point=GetBezierPoint(primitive_info,coefficients,number_coordinates,
      weight);
    points[i]=point;
    weight+=1.0/control_points;
  }
  /*
    Bezier curves are just short segmented polys.
  */
  if (TraceBezierControlPoints(primitive_info,points,control_points,&p) ==
      MagickFalse)
    return(ReleaseBezierBuffers(points,coefficients));
  if (TracePoint(p,end) == MagickFalse)
    return(ReleaseBezierBuffers(points,coefficients));
  p+=(ptrdiff_t) p->coordinates;
  primitive_info->coordinates=(size_t) (p-primitive_info);
  primitive_info->closed_subpath=MagickFalse;
  SetTracedPrimitiveType(primitive_info,p);
  points=(PointInfo *) RelinquishMagickMemory(points);
  coefficients=(double *) RelinquishMagickMemory(coefficients);
  return(MagickTrue);
}

static MagickBooleanType TraceCircle(MVGInfo *mvg_info,const PointInfo start,
  const PointInfo end)
{
  double
    alpha,
    beta,
    radius;

  PointInfo
    offset,
    degrees;

  alpha=end.x-start.x;
  beta=end.y-start.y;
  radius=hypot((double) alpha,(double) beta);
  offset.x=(double) radius;
  offset.y=(double) radius;
  degrees.x=0.0;
  degrees.y=360.0;
  return(TraceEllipse(mvg_info,start,offset,degrees));
}

static inline MagickBooleanType IsZeroEllipseRadii(const PointInfo radii)
{
  return(((fabs(radii.x) < MagickEpsilon) || (fabs(radii.y) < MagickEpsilon)) ?
    MagickTrue : MagickFalse);
}

static double GetEllipseEndAngle(const PointInfo arc)
{
  double
    y;

  y=arc.y;
  while (y < arc.x)
    y+=360.0;
  return(y);
}

static MagickBooleanType TraceEllipse(MVGInfo *mvg_info,const PointInfo center,
  const PointInfo radii,const PointInfo arc)
{
  double
    coordinates,
    delta,
    step,
    x,
    y;

  PointInfo
    angle,
    point;

  PrimitiveInfo
    *primitive_info;

  PrimitiveInfo
    *p;

  ssize_t
    i;

  /*
    Ellipses are just short segmented polys.
  */
  primitive_info=(*mvg_info->primitive_info)+mvg_info->offset;
  primitive_info->coordinates=0;
  if (IsZeroEllipseRadii(radii) != MagickFalse)
    return(MagickTrue);
  delta=MagickSafeReciprocal(MagickMax(radii.x,radii.y));
  step=MagickPI/(MagickPI*MagickSafeReciprocal(delta))/8.0;
  angle.x=DegreesToRadians(arc.x);
  y=GetEllipseEndAngle(arc);
  angle.y=DegreesToRadians(y);
  coordinates=ceil((angle.y-angle.x)/step+1.0);
  if (CheckPrimitiveExtent(mvg_info,coordinates+1) == MagickFalse)
    return(MagickFalse);
  i=0;
  primitive_info=(*mvg_info->primitive_info)+mvg_info->offset;
  for (p=primitive_info; angle.x < angle.y; angle.x+=step)
  {
    point.x=cos(fmod(angle.x,DegreesToRadians(360.0)))*radii.x+center.x;
    point.y=sin(fmod(angle.x,DegreesToRadians(360.0)))*radii.y+center.y;
    if (i++ >= (ssize_t) coordinates)
      break;
    if (TracePoint(p,point) == MagickFalse)
      return(MagickFalse);
    p+=(ptrdiff_t) p->coordinates;
  }
  point.x=cos(fmod(angle.y,DegreesToRadians(360.0)))*radii.x+center.x;
  point.y=sin(fmod(angle.y,DegreesToRadians(360.0)))*radii.y+center.y;
  if (TracePoint(p,point) == MagickFalse)
    return(MagickFalse);
  p+=(ptrdiff_t) p->coordinates;
  primitive_info->coordinates=(size_t) (p-primitive_info);
  primitive_info->closed_subpath=MagickFalse;
  x=fabs(primitive_info[0].point.x-
    primitive_info[primitive_info->coordinates-1].point.x);
  y=fabs(primitive_info[0].point.y-
    primitive_info[primitive_info->coordinates-1].point.y);
  if (IsClosedEllipse(x,y) != MagickFalse)
    primitive_info->closed_subpath=MagickTrue;
  SetTracedPrimitiveType(primitive_info,p);
  return(MagickTrue);
}

static MagickBooleanType TraceLine(PrimitiveInfo *primitive_info,
  const PointInfo start,const PointInfo end)
{
  if (TracePoint(primitive_info,start) == MagickFalse)
    return(MagickFalse);
  if (TracePoint(primitive_info+1,end) == MagickFalse)
    return(MagickFalse);
  (primitive_info+1)->primitive=primitive_info->primitive;
  primitive_info->coordinates=2;
  primitive_info->closed_subpath=MagickFalse;
  return(MagickTrue);
}

typedef struct _PathTrace
{
  MVGInfo
    *mvg_info;

  ExceptionInfo
    *exception;

  char
    *token;

  const char
    **p;

  int
    *attribute,
    *last_attribute;

  MagickBooleanType
    *status;

  PointInfo
    *end,
    *point,
    *points,
    *start;

  PrimitiveInfo
    **primitive_info,
    **q;

  ssize_t
    *subpath_offset;

  size_t
    *number_coordinates,
    *z_count;
} PathTrace;

static void SkipPathSeparators(const char **p)
{
  while (isspace((int) ((unsigned char) *(*p))) != 0)
    (*p)++;
  if (*(*p) == ',')
    (*p)++;
}

static MagickBooleanType SkipPathListSeparator(const char **p,char *token)
{
  if (IsValidListChar((int) ((unsigned char) *token)) == MagickFalse)
    return(MagickFalse);
  if (*token == ',')
    (void) GetNextToken((*p),p,MagickPathExtent,token);
  return(MagickTrue);
}

static PrimitiveInfo *AdvancePrimitiveOffset(MVGInfo *mvg_info)
{
  PrimitiveInfo
    *q;

  q=(*mvg_info->primitive_info)+mvg_info->offset;
  mvg_info->offset+=(ssize_t) q->coordinates;
  q+=(ptrdiff_t) q->coordinates;
  return(q);
}

static MagickBooleanType SkipPathListToken(const char **p,char *token)
{
  (void) GetNextToken((*p),p,MagickPathExtent,token);
  if (IsValidListChar((int) ((unsigned char) *token)) == MagickFalse)
    return(MagickFalse);
  if (*token == ',')
    (void) GetNextToken((*p),p,MagickPathExtent,token);
  return(MagickTrue);
}

static MagickBooleanType GetPathValue(const char **p,char *token,
  double *value)
{
  char
    *next_token;

  (void) GetNextToken((*p),p,MagickPathExtent,token);
  if (IsValidListChar((int) ((unsigned char) *token)) == MagickFalse)
    return(MagickFalse);
  if (*token == ',')
    (void) GetNextToken((*p),p,MagickPathExtent,token);
  (*value)=GetDrawValue(token,&next_token);
  if (token == next_token)
    return(MagickFalse);
  return(MagickTrue);
}

static inline double GetPathCoordinate(const int attribute,const int absolute,
  const double value,const double current)
{
  return((double) (attribute == absolute ? value : current+value));
}

static MagickBooleanType GetPathPoint(const char **p,char *token,double *x,
  double *y)
{
  if (GetPathValue(p,token,&(*x)) == MagickFalse)
    return(MagickFalse);
  if (GetPathValue(p,token,&(*y)) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType GetPathArcShape(const char **p,char *token,
  ArcPathShape *shape)
{
  if (GetPathPoint(p,token,&shape->arc.x,&shape->arc.y) == MagickFalse)
    return(MagickFalse);
  if (GetPathValue(p,token,&shape->angle) == MagickFalse)
    return(MagickFalse);
  if (SkipPathListToken(p,token) == MagickFalse)
    return(MagickFalse);
  shape->large_arc=StringToLong(token) != 0 ? MagickTrue : MagickFalse;
  if (SkipPathListToken(p,token) == MagickFalse)
    return(MagickFalse);
  shape->sweep=StringToLong(token) != 0 ? MagickTrue : MagickFalse;
  if (SkipPathListSeparator(p,token) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static ssize_t TracePathArc(const PathTrace *trace)
{
  ArcPathShape
    shape = { {0.0, 0.0}, 0.0, MagickFalse, MagickFalse };

  ExceptionInfo
    *exception = trace->exception;

  MVGInfo
    *mvg_info = trace->mvg_info;

  MagickBooleanType
    status = (*trace->status);

  PointInfo
    point = (*trace->point),
    end = (*trace->end);

  PrimitiveInfo
    *q = (*trace->q);

  char
    *token = trace->token;

  const char
    *p = (*trace->p);

  const int
    attribute = (*trace->attribute);

  double
    x,
    y;

  /*
    Elliptical arc.
  */
  do
  {
    if (GetPathArcShape(&p,token,&shape) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (GetPathPoint(&p,token,&x,&y) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    end.x=GetPathCoordinate(attribute,(int) 'A',x,point.x);
    end.y=GetPathCoordinate(attribute,(int) 'A',y,point.y);
    if (TraceArcPath(mvg_info,point,end,&shape) == MagickFalse)
      return(-1);
    q=AdvancePrimitiveOffset(mvg_info);
    point=end;
    SkipPathSeparators(&p);
  } while (IsValidPoint(p) != MagickFalse);
  (*trace->p)=p;
  (*trace->q)=q;
  (*trace->point)=point;
  (*trace->end)=end;
  (*trace->status)=status;
  return(0);
}

typedef enum
{
  CubicPathBezier,
  QuadraticPathBezier
} PathBezierSide;

static inline int GetBezierAbsoluteLetter(const PathBezierSide side)
{
  return((side == CubicPathBezier) ? 'C' : 'Q');
}

static inline int GetBezierPointCount(const PathBezierSide side)
{
  return((side == CubicPathBezier) ? 4 : 3);
}

static MagickBooleanType TraceBezierPoints(MVGInfo *mvg_info,
  const PointInfo *points,const PathBezierSide side,PrimitiveInfo **q)
{
  ssize_t
    i;

  for (i=0; i < ((side == CubicPathBezier) ? 4 : 3); i++)
    ((*q)+i)->point=points[i];
  if (TraceBezier(mvg_info,((side == CubicPathBezier) ? 4 : 3)) == MagickFalse)
    return(MagickFalse);
  (*q)=(*mvg_info->primitive_info)+mvg_info->offset;
  mvg_info->offset+=(ssize_t) (*q)->coordinates;
  (*q)+=(ptrdiff_t) (*q)->coordinates;
  return(MagickTrue);
}

static ssize_t TracePathBezierCurve(const PathTrace *trace,
  const PathBezierSide side)
{
  ExceptionInfo
    *exception = trace->exception;

  MVGInfo
    *mvg_info = trace->mvg_info;

  MagickBooleanType
    status = (*trace->status);

  PointInfo
    *points = trace->points,
    point = (*trace->point),
    end = (*trace->end);

  PrimitiveInfo
    *q = (*trace->q);

  char
    *token = trace->token;

  const char
    *p = (*trace->p);

  const int
    attribute = (*trace->attribute);

  double
    x,
    y;

  ssize_t
    i;

  /*
    Cubic Bézier curve.
  */
  do
  {
    points[0]=point;
    for (i=1; i < GetBezierPointCount(side); i++)
    {
      if (GetPathValue(&p,token,&x) == MagickFalse)
        ThrowPointExpectedException(token,exception);
      if (GetPathValue(&p,token,&y) == MagickFalse)
        ThrowPointExpectedException(token,exception);
      if ((side == QuadraticPathBezier) && (*p == ','))
        p++;
      end.x=GetPathCoordinate(attribute,(int) GetBezierAbsoluteLetter(side),x,point.x);
      end.y=GetPathCoordinate(attribute,(int) GetBezierAbsoluteLetter(side),y,point.y);
      points[i]=end;
    }
    if (TraceBezierPoints(mvg_info,points,side,&q) == MagickFalse)
      return(-1);
    point=end;
    SkipPathSeparators(&p);
  } while (IsValidPoint(p) != MagickFalse);
  (*trace->p)=p;
  (*trace->q)=q;
  (*trace->point)=point;
  (*trace->end)=end;
  (*trace->status)=status;
  return(0);
}

typedef enum
{
  HorizontalPathLine,
  VerticalPathLine
} PathLineSide;

static ssize_t TracePathAxisLine(const PathTrace *trace,
  const PathLineSide side)
{
  ExceptionInfo
    *exception = trace->exception;

  MVGInfo
    *mvg_info = trace->mvg_info;

  MagickBooleanType
    status = (*trace->status);

  PointInfo
    point = (*trace->point);

  PrimitiveInfo
    *q = (*trace->q);

  char
    *token = trace->token;

  const char
    *p = (*trace->p);

  const int
    attribute = (*trace->attribute);

  double
    x;

  do
  {
    if (GetPathValue(&p,token,&x) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (side == HorizontalPathLine)
      point.x=GetPathCoordinate(attribute,(int) 'H',x,point.x);
    else
      point.y=GetPathCoordinate(attribute,(int) 'V',x,point.y);
    if (CheckPrimitiveExtent(mvg_info,PrimitiveExtentPad) == MagickFalse)
      return(-1);
    q=(*mvg_info->primitive_info)+mvg_info->offset;
    if (TracePoint(q,point) == MagickFalse)
      return(-1);
    mvg_info->offset+=(ssize_t) q->coordinates;
    q+=(ptrdiff_t) q->coordinates;
    SkipPathSeparators(&p);
  } while (IsValidPoint(p) != MagickFalse);
  (*trace->p)=p;
  (*trace->q)=q;
  (*trace->point)=point;
  (*trace->status)=status;
  return(0);
}

static ssize_t TracePathLine(const PathTrace *trace)
{
  ExceptionInfo
    *exception = trace->exception;

  MVGInfo
    *mvg_info = trace->mvg_info;

  MagickBooleanType
    status = (*trace->status);

  PointInfo
    point = (*trace->point);

  PrimitiveInfo
    *q = (*trace->q);

  char
    *token = trace->token;

  const char
    *p = (*trace->p);

  const int
    attribute = (*trace->attribute);

  double
    x,
    y;

  /*
    Line to.
  */
  do
  {
    if (GetPathValue(&p,token,&x) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (GetPathValue(&p,token,&y) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    point.x=GetPathCoordinate(attribute,(int) 'L',x,point.x);
    point.y=GetPathCoordinate(attribute,(int) 'L',y,point.y);
    if (CheckPrimitiveExtent(mvg_info,PrimitiveExtentPad) == MagickFalse)
      return(-1);
    q=(*mvg_info->primitive_info)+mvg_info->offset;
    if (TracePoint(q,point) == MagickFalse)
      return(-1);
    mvg_info->offset+=(ssize_t) q->coordinates;
    q+=(ptrdiff_t) q->coordinates;
    SkipPathSeparators(&p);
  } while (IsValidPoint(p) != MagickFalse);
  (*trace->p)=p;
  (*trace->q)=q;
  (*trace->point)=point;
  (*trace->status)=status;
  return(0);
}

static ssize_t TracePathMove(const PathTrace *trace)
{
  ExceptionInfo
    *exception = trace->exception;

  MVGInfo
    *mvg_info = trace->mvg_info;

  MagickBooleanType
    status = (*trace->status);

  PointInfo
    point = (*trace->point),
    start = (*trace->start);

  PrimitiveInfo
    *q = (*trace->q),
    *primitive_info = (*trace->primitive_info);

  char
    *token = trace->token;

  const char
    *p = (*trace->p);

  const int
    attribute = (*trace->attribute);

  double
    x,
    y;

  size_t
    number_coordinates = (*trace->number_coordinates);

  ssize_t
    i,
    subpath_offset = (*trace->subpath_offset);

  /*
    Move to.
  */
  if (mvg_info->offset != subpath_offset)
    {
      primitive_info=(*mvg_info->primitive_info)+subpath_offset;
      primitive_info->coordinates=(size_t) (q-primitive_info);
      number_coordinates+=primitive_info->coordinates;
      primitive_info=q;
      subpath_offset=mvg_info->offset;
    }
  i=0;
  do
  {
    if (GetPathValue(&p,token,&x) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (GetPathValue(&p,token,&y) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    point.x=GetPathCoordinate(attribute,(int) 'M',x,point.x);
    point.y=GetPathCoordinate(attribute,(int) 'M',y,point.y);
    if (i == 0)
      start=point;
    i++;
    if (CheckPrimitiveExtent(mvg_info,PrimitiveExtentPad) == MagickFalse)
      return(-1);
    q=(*mvg_info->primitive_info)+mvg_info->offset;
    if (TracePoint(q,point) == MagickFalse)
      return(-1);
    mvg_info->offset+=(ssize_t) q->coordinates;
    q+=(ptrdiff_t) q->coordinates;
    SkipPathSeparators(&p);
  } while (IsValidPoint(p) != MagickFalse);
  (*trace->p)=p;
  (*trace->q)=q;
  (*trace->point)=point;
  (*trace->start)=start;
  (*trace->status)=status;
  (*trace->primitive_info)=primitive_info;
  (*trace->subpath_offset)=subpath_offset;
  (*trace->number_coordinates)=number_coordinates;
  return(0);
}


static void ReflectBezierControlPoint(PointInfo *points,
  const PathBezierSide side)
{
  points[0]=points[((side == CubicPathBezier) ? 3 : 2)];
  points[1].x=2.0*points[((side == CubicPathBezier) ? 3 : 2)].x-points[((side == CubicPathBezier) ? 2 : 1)].x;
  points[1].y=2.0*points[((side == CubicPathBezier) ? 3 : 2)].y-points[((side == CubicPathBezier) ? 2 : 1)].y;
}

static inline int GetBezierSmoothLetter(const PathBezierSide side)
{
  return((side == CubicPathBezier) ? 'S' : 'T');
}

static inline MagickBooleanType IsCubicPathComma(const PathBezierSide side,
  const char *p)
{
  return(((side == CubicPathBezier) && (*p == ',')) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsQuadraticPathFailed(
  const PathBezierSide side,const MagickBooleanType status)
{
  return(((side == QuadraticPathBezier) && (status == MagickFalse)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsSmoothPathRestart(const PathBezierSide side,
  const int last_attribute)
{
  return((strchr(((side == CubicPathBezier) ? "CcSs" : "QqTt"),last_attribute) ==
    (char *) NULL) ? MagickTrue : MagickFalse);
}

static ssize_t TracePathSmoothBezierCurve(const PathTrace *trace,
  const PathBezierSide side)
{
  ExceptionInfo
    *exception = trace->exception;

  MVGInfo
    *mvg_info = trace->mvg_info;

  MagickBooleanType
    status = (*trace->status);

  PointInfo
    *points = trace->points,
    point = (*trace->point),
    end = (*trace->end);

  PrimitiveInfo
    *q = (*trace->q);

  char
    *token = trace->token;

  const char
    *p = (*trace->p);

  const int
    attribute = (*trace->attribute);

  double
    x,
    y;

  int
    last_attribute = (*trace->last_attribute);

  ssize_t
    i;

  /*
    Cubic Bézier curve.
  */
  do
  {
    ReflectBezierControlPoint(points,side);
    for (i=2; i < GetBezierPointCount(side); i++)
    {
      if (GetPathPoint(&p,token,&x,&y) == MagickFalse)
        ThrowPointExpectedException(token,exception);
      if (IsCubicPathComma(side,p) != MagickFalse)
        p++;
      end.x=GetPathCoordinate(attribute,(int) GetBezierSmoothLetter(side),x,point.x);
      end.y=GetPathCoordinate(attribute,(int) GetBezierSmoothLetter(side),y,point.y);
      points[i]=end;
    }
    if (IsQuadraticPathFailed(side,status) != MagickFalse)
      break;
    if (IsSmoothPathRestart(side,last_attribute) != MagickFalse)
      {
        points[0]=point;
        points[1]=point;
      }
    if (TraceBezierPoints(mvg_info,points,side,&q) == MagickFalse)
      return(-1);
    point=end;
    last_attribute=attribute;
    SkipPathSeparators(&p);
  } while (IsValidPoint(p) != MagickFalse);
  (*trace->p)=p;
  (*trace->q)=q;
  (*trace->point)=point;
  (*trace->end)=end;
  (*trace->status)=status;
  (*trace->last_attribute)=last_attribute;
  return(0);
}



static ssize_t TracePathClose(const PathTrace *trace)
{
  MVGInfo
    *mvg_info = trace->mvg_info;

  PointInfo
    point = (*trace->point),
    start = (*trace->start);

  PrimitiveInfo
    *q = (*trace->q),
    *primitive_info = (*trace->primitive_info);

  size_t
    number_coordinates = (*trace->number_coordinates),
    z_count = (*trace->z_count);

  ssize_t
    subpath_offset = (*trace->subpath_offset);

  /*
    Close path.
  */
  point=start;
  if (CheckPrimitiveExtent(mvg_info,PrimitiveExtentPad) == MagickFalse)
    return(-1);
  q=(*mvg_info->primitive_info)+mvg_info->offset;
  if (TracePoint(q,point) == MagickFalse)
    return(-1);
  mvg_info->offset+=(ssize_t) q->coordinates;
  q+=(ptrdiff_t) q->coordinates;
  primitive_info=(*mvg_info->primitive_info)+subpath_offset;
  primitive_info->coordinates=(size_t) (q-primitive_info);
  primitive_info->closed_subpath=MagickTrue;
  number_coordinates+=primitive_info->coordinates;
  primitive_info=q;
  subpath_offset=mvg_info->offset;
  z_count++;
  (*trace->q)=q;
  (*trace->point)=point;
  (*trace->start)=start;
  (*trace->primitive_info)=primitive_info;
  (*trace->subpath_offset)=subpath_offset;
  (*trace->number_coordinates)=number_coordinates;
  (*trace->z_count)=z_count;
  return(0);
}

static ssize_t FinishTracedPath(const PathTrace *trace,
  const PrimitiveType primitive_type)
{
  MVGInfo
    *mvg_info = trace->mvg_info;

  PrimitiveInfo
    *primitive_info,
    *q = (*trace->q);

  size_t
    number_coordinates = (*trace->number_coordinates),
    z_count = (*trace->z_count);

  ssize_t
    i,
    subpath_offset = (*trace->subpath_offset);

  primitive_info=(*mvg_info->primitive_info)+subpath_offset;
  primitive_info->coordinates=(size_t) (q-primitive_info);
  number_coordinates+=primitive_info->coordinates;
  for (i=0; i < (ssize_t) number_coordinates; i++)
  {
    q--;
    q->primitive=primitive_type;
    if (z_count > 1)
      q->method=FillToBorderMethod;
  }
  q=primitive_info;
  return((ssize_t) number_coordinates);
}

typedef struct _PathCommandInfo
{
  int
    key;

  ssize_t
    (*trace)(const PathTrace *),
    (*trace_bezier)(const PathTrace *,const PathBezierSide),
    (*trace_line)(const PathTrace *,const PathLineSide);

  PathBezierSide
    bezier_side;

  PathLineSide
    line_side;
} PathCommandInfo;

static const PathCommandInfo
  PathCommands[] =
  {
    { 'a', TracePathArc, NULL, NULL, CubicPathBezier, HorizontalPathLine },
    { 'A', TracePathArc, NULL, NULL, CubicPathBezier, HorizontalPathLine },
    { 'c', NULL, TracePathBezierCurve, NULL, CubicPathBezier, HorizontalPathLine },
    { 'C', NULL, TracePathBezierCurve, NULL, CubicPathBezier, HorizontalPathLine },
    { 'H', NULL, NULL, TracePathAxisLine, CubicPathBezier, HorizontalPathLine },
    { 'h', NULL, NULL, TracePathAxisLine, CubicPathBezier, HorizontalPathLine },
    { 'l', TracePathLine, NULL, NULL, CubicPathBezier, HorizontalPathLine },
    { 'L', TracePathLine, NULL, NULL, CubicPathBezier, HorizontalPathLine },
    { 'M', TracePathMove, NULL, NULL, CubicPathBezier, HorizontalPathLine },
    { 'm', TracePathMove, NULL, NULL, CubicPathBezier, HorizontalPathLine },
    { 'q', NULL, TracePathBezierCurve, NULL, QuadraticPathBezier, HorizontalPathLine },
    { 'Q', NULL, TracePathBezierCurve, NULL, QuadraticPathBezier, HorizontalPathLine },
    { 's', NULL, TracePathSmoothBezierCurve, NULL, CubicPathBezier, HorizontalPathLine },
    { 'S', NULL, TracePathSmoothBezierCurve, NULL, CubicPathBezier, HorizontalPathLine },
    { 't', NULL, TracePathSmoothBezierCurve, NULL, QuadraticPathBezier, HorizontalPathLine },
    { 'T', NULL, TracePathSmoothBezierCurve, NULL, QuadraticPathBezier, HorizontalPathLine },
    { 'v', NULL, NULL, TracePathAxisLine, CubicPathBezier, VerticalPathLine },
    { 'V', NULL, NULL, TracePathAxisLine, CubicPathBezier, VerticalPathLine },
    { 'z', TracePathClose, NULL, NULL, CubicPathBezier, HorizontalPathLine },
    { 'Z', TracePathClose, NULL, NULL, CubicPathBezier, HorizontalPathLine }
  };

static const PathCommandInfo *GetPathCommandInfo(const int key)
{
  size_t
    i;

  for (i=0; i < (sizeof(PathCommands)/sizeof(*PathCommands)); i++)
    if (PathCommands[i].key == key)
      return(PathCommands+i);
  return((const PathCommandInfo *) NULL);
}

static ssize_t CallPathCommand(const PathCommandInfo *command,
  const PathTrace *trace)
{
  if (command->trace != (ssize_t (*)(const PathTrace *)) NULL)
    return(command->trace(trace));
  if (command->trace_bezier !=
      (ssize_t (*)(const PathTrace *,const PathBezierSide)) NULL)
    return(command->trace_bezier(trace,command->bezier_side));
  return(command->trace_line(trace,command->line_side));
}

static ssize_t TracePath(MVGInfo *mvg_info,const char *path,
  ExceptionInfo *exception)
{
  char
    token[MagickPathExtent] = "";

  const char
    *p;

  int
    attribute,
    last_attribute;

  MagickBooleanType
    status;

  PointInfo
    end = {0.0, 0.0},
    points[4] = { {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0} },
    point = {0.0, 0.0},
    start = {0.0, 0.0};

  PrimitiveInfo
    *primitive_info;

  PrimitiveType
    primitive_type;

  const PathCommandInfo
    *command;

  PrimitiveInfo
    *q;

  size_t
    number_coordinates,
    z_count;

  ssize_t
    subpath_offset;

  const PathTrace
    trace = { mvg_info, exception, token, &p, &attribute, &last_attribute,
      &status, &end, &point, points, &start, &primitive_info, &q,
      &subpath_offset, &number_coordinates, &z_count };

  subpath_offset=mvg_info->offset;
  primitive_info=(*mvg_info->primitive_info)+mvg_info->offset;
  status=MagickTrue;
  attribute=0;
  number_coordinates=0;
  z_count=0;
  primitive_type=primitive_info->primitive;
  q=primitive_info;
  for (p=path; *p != '\0'; )
  {
    if (status == MagickFalse)
      break;
    while (isspace((int) ((unsigned char) *p)) != 0)
      p++;
    if (*p == '\0')
      break;
    last_attribute=attribute;
    attribute=(int) (*p++);
    command=GetPathCommandInfo(attribute);
    if (command == (const PathCommandInfo *) NULL)
      {
        ThrowPointExpectedException(token,exception);
      }
    else
      if (CallPathCommand(command,&trace) == -1)
        return(-1);
  }
  if (status == MagickFalse)
    return(-1);
  return(FinishTracedPath(&trace,primitive_type));
}

static void ClampRoundCornerArc(double *arc,const double segment)
{
  if ((*arc) > (0.5*segment))
    (*arc)=0.5*segment;
}

static inline MagickBooleanType IsDegenerateSegment(const PointInfo segment)
{
  return(((segment.x < MagickEpsilon) || (segment.y < MagickEpsilon)) ?
    MagickTrue : MagickFalse);
}

static MagickBooleanType TraceRoundCorner(MVGInfo *mvg_info,
  const PointInfo point,const PointInfo arc,const PointInfo degrees)
{
  PrimitiveInfo
    *p;

  if (TraceEllipse(mvg_info,point,arc,degrees) == MagickFalse)
    return(MagickFalse);
  p=(*mvg_info->primitive_info)+(ptrdiff_t) mvg_info->offset;
  mvg_info->offset+=(ssize_t) p->coordinates;
  return(MagickTrue);
}

static MagickBooleanType TraceRoundRectangle(MVGInfo *mvg_info,
  const PointInfo start,const PointInfo end,PointInfo arc)
{
  PointInfo
    degrees,
    point,
    segment;

  PrimitiveInfo
    *primitive_info;

  PrimitiveInfo
    *p;


  ssize_t
    offset;

  offset=mvg_info->offset;
  segment.x=fabs(end.x-start.x);
  segment.y=fabs(end.y-start.y);
  if (IsDegenerateSegment(segment) != MagickFalse)
    {
      (*mvg_info->primitive_info+mvg_info->offset)->coordinates=0;
      return(MagickTrue);
    }
  ClampRoundCornerArc(&arc.x,segment.x);
  ClampRoundCornerArc(&arc.y,segment.y);
  point.x=start.x+segment.x-arc.x;
  point.y=start.y+arc.y;
  degrees.x=270.0;
  degrees.y=360.0;
  if (TraceRoundCorner(mvg_info,point,arc,degrees) == MagickFalse)
    return(MagickFalse);
  point.x=start.x+segment.x-arc.x;
  point.y=start.y+segment.y-arc.y;
  degrees.x=0.0;
  degrees.y=90.0;
  if (TraceRoundCorner(mvg_info,point,arc,degrees) == MagickFalse)
    return(MagickFalse);
  point.x=start.x+arc.x;
  point.y=start.y+segment.y-arc.y;
  degrees.x=90.0;
  degrees.y=180.0;
  if (TraceRoundCorner(mvg_info,point,arc,degrees) == MagickFalse)
    return(MagickFalse);
  point.x=start.x+arc.x;
  point.y=start.y+arc.y;
  degrees.x=180.0;
  degrees.y=270.0;
  if (TraceRoundCorner(mvg_info,point,arc,degrees) == MagickFalse)
    return(MagickFalse);
  if (CheckPrimitiveExtent(mvg_info,PrimitiveExtentPad) == MagickFalse)
    return(MagickFalse);
  p=(*mvg_info->primitive_info)+(ptrdiff_t) mvg_info->offset;
  if (TracePoint(p,(*mvg_info->primitive_info+offset)->point) == MagickFalse)
    return(MagickFalse);
  p+=(ptrdiff_t) p->coordinates;
  mvg_info->offset=offset;
  primitive_info=(*mvg_info->primitive_info)+offset;
  primitive_info->coordinates=(size_t) (p-primitive_info);
  primitive_info->closed_subpath=MagickTrue;
  SetTracedPrimitiveType(primitive_info,p);
  return(MagickTrue);
}

#endif
