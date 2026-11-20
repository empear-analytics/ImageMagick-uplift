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

  MagickCore private draw primitive polygon fill methods.
*/
#ifndef MAGICKCORE_DRAW_PRIMITIVE_FILL_PRIVATE_H
#define MAGICKCORE_DRAW_PRIMITIVE_FILL_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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

static inline double ClampBoundsCoordinate(const double value,
  const double edge,const double limit)
{
  return(value < 0.0 ? 0.0 : value >= edge ? limit : value);
}

static SegmentInfo ClampPolygonBounds(const Image *image,SegmentInfo bounds)
{
  bounds.x1=ClampBoundsCoordinate(bounds.x1,(double) image->columns-1.0,
    (double) image->columns-1.0);
  bounds.y1=ClampBoundsCoordinate(bounds.y1,(double) image->rows-1.0,
    (double) image->rows-1.0);
  bounds.x2=ClampBoundsCoordinate(bounds.x2,(double) image->columns-1.0,
    (double) image->columns-1.0);
  bounds.y2=ClampBoundsCoordinate(bounds.y2,(double) image->rows-1.0,
    (double) image->rows-1.0);
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
        {
          const PolygonPointRow
            row = { image, draw_info, primitive_info, poly_extent, y,
              exception };

          DrawPolygonPointRow(&row,x,q);
        }
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


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
