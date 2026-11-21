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
