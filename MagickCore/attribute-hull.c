/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%         AAA   TTTTT  TTTTT  RRRR   IIIII  BBBB   U   U  TTTTT  EEEEE        %
%        A   A    T      T    R   R    I    B   B  U   U    T    E            %
%        AAAAA    T      T    RRRR     I    BBBB   U   U    T    EEE          %
%        A   A    T      T    R R      I    B   B  U   U    T    E            %
%        A   A    T      T    R  R   IIIII  BBBB    UUU     T    EEEEE        %
%                                                                             %
%                         H   H  U   U  L      L                              %
%                         H   H  U   U  L      L                              %
%                         HHHHH  U   U  L      L                              %
%                         H   H  U   U  L      L                              %
%                         H   H   UUU   LLLLL  LLLLL                          %
%                                                                             %
%                                                                             %
%                   MagickCore Minimum Bounding Box Method                    %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                October 2002                                 %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/blob-private.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/colormap-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/histogram.h"
#include "MagickCore/identify.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/memory_.h"
#include "MagickCore/magick.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/segment.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e M i n i m u m B o u n d i n g B o x                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageMinimumBoundingBox() returns the points that form the minimum
%  bounding box around the image foreground objects with the "Rotating
%  Calipers" algorithm.  The method also returns these properties:
%  minimum-bounding-box:area, minimum-bounding-box:width,
%  minimum-bounding-box:height, and minimum-bounding-box:angle.
%
%  The format of the GetImageMinimumBoundingBox method is:
%
%      PointInfo *GetImageMinimumBoundingBox(Image *image,
%        size_t number_vertices,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o number_vertices: the number of vertices in the bounding box.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _CaliperInfo
{
  double
    area,
    width,
    height,
    projection;

  ssize_t
    p,
    q,
    v;
} CaliperInfo;

static inline double getAngle(PointInfo *p,PointInfo *q)
{
  /*
    Get the angle between line (p,q) and horizontal axis, in degrees.
  */
  return(RadiansToDegrees(atan2(q->y-p->y,q->x-p->x)));
}

static inline double getDistance(PointInfo *p,PointInfo *q)
{
  double
    distance;

  distance=hypot(p->x-q->x,p->y-q->y);
  return(distance*distance);
}

static inline double getProjection(PointInfo *p,PointInfo *q,PointInfo *v)
{
  double
    distance;

  /*
    Projection of vector (x,y) - p into a line passing through p and q.
  */
  distance=getDistance(p,q);
  if (distance < MagickEpsilon)
    return(INFINITY);
  return((q->x-p->x)*(v->x-p->x)+(v->y-p->y)*(q->y-p->y))/sqrt(distance);
}

static inline double getFeretDiameter(PointInfo *p,PointInfo *q,PointInfo *v)
{
  double
    distance;

  /*
    Distance from a point (x,y) to a line passing through p and q.
  */
  distance=getDistance(p,q);
  if (distance < MagickEpsilon)
    return(INFINITY);
  return((q->x-p->x)*(v->y-p->y)-(v->x-p->x)*(q->y-p->y))/sqrt(distance);
}

static double GetMinimumBoxAngle(PointInfo *bounding_box)
{
  double
    angle,
    distance;

  ssize_t
    i;

  distance=hypot(bounding_box[0].x,bounding_box[0].y);
  angle=getAngle(&bounding_box[0],&bounding_box[1]);
  for (i=1; i < 4; i++)
  {
    double d = hypot(bounding_box[i].x,bounding_box[i].y);
    if (d < distance)
      {
        distance=d;
        angle=getAngle(&bounding_box[i],&bounding_box[(i+1) % 4]);
      }
  }
  return(angle);
}

static void MoveBoxToOrigin(PointInfo *bounding_box)
{
  PointInfo
    point;

  ssize_t
    i;

  point=bounding_box[0];
  for (i=1; i < 4; i++)
  {
    if (bounding_box[i].x < point.x)
      point.x=bounding_box[i].x;
    if (bounding_box[i].y < point.y)
      point.y=bounding_box[i].y;
  }
  for (i=0; i < 4; i++)
  {
    bounding_box[i].x-=point.x;
    bounding_box[i].y-=point.y;
  }
}

static void SetMinimumBoundingBoxProperties(Image *image,
  const CaliperInfo caliper_info,const PointInfo *vertices)
{
  (void) FormatImageProperty(image,"minimum-bounding-box:area","%.*g",
    GetMagickPrecision(),caliper_info.area);
  (void) FormatImageProperty(image,"minimum-bounding-box:width","%.*g",
    GetMagickPrecision(),caliper_info.width);
  (void) FormatImageProperty(image,"minimum-bounding-box:height","%.*g",
    GetMagickPrecision(),caliper_info.height);
  (void) FormatImageProperty(image,"minimum-bounding-box:_p","%.*g,%.*g",
    GetMagickPrecision(),vertices[caliper_info.p].x,
    GetMagickPrecision(),vertices[caliper_info.p].y);
  (void) FormatImageProperty(image,"minimum-bounding-box:_q","%.*g,%.*g",
    GetMagickPrecision(),vertices[caliper_info.q].x,
    GetMagickPrecision(),vertices[caliper_info.q].y);
  (void) FormatImageProperty(image,"minimum-bounding-box:_v","%.*g,%.*g",
    GetMagickPrecision(),vertices[caliper_info.v].x,
    GetMagickPrecision(),vertices[caliper_info.v].y);
}

MagickExport PointInfo *GetImageMinimumBoundingBox(Image *image,
  size_t *number_vertices,ExceptionInfo *exception)
{
  CaliperInfo
    caliper_info;

  const char
    *artifact;

  double
    angle,
    diameter,
    distance;

  PointInfo
    *bounding_box,
    *vertices;

  size_t
    number_hull_vertices;

  ssize_t
    i;

  /*
    Generate the minimum bounding box with the "Rotating Calipers" algorithm.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  *number_vertices=0;
  vertices=GetImageConvexHull(image,&number_hull_vertices,exception);
  if (vertices == (PointInfo *) NULL)
    return((PointInfo *) NULL);
  *number_vertices=4;
  bounding_box=(PointInfo *) AcquireQuantumMemory(*number_vertices,
    sizeof(*bounding_box));
  if (bounding_box == (PointInfo *) NULL)
    {
      vertices=(PointInfo *) RelinquishMagickMemory(vertices);
      return((PointInfo *) NULL);
    }
  caliper_info.area=2.0*image->columns*image->rows;
  caliper_info.width=(double) image->columns+image->rows;
  caliper_info.height=0.0;
  caliper_info.projection=0.0;
  caliper_info.p=(-1);
  caliper_info.q=(-1);
  caliper_info.v=(-1);
  for (i=0; i < (ssize_t) number_hull_vertices; i++)
  {
    double
      area = 0.0,
      max_projection = 0.0,
      min_diameter = -1.0,
      min_projection = 0.0;

    ssize_t
      j,
      k;

    ssize_t
      p = -1,
      q = -1,
      v = -1;

    for (j=0; j < (ssize_t) number_hull_vertices; j++)
    {
      diameter=fabs(getFeretDiameter(&vertices[i],
        &vertices[(i+1) % (ssize_t) number_hull_vertices],&vertices[j]));
      if (min_diameter < diameter)
        {
          min_diameter=diameter;
          p=i;
          q=(i+1) % (ssize_t) number_hull_vertices;
          v=j;
        }
    }
    for (k=0; k < (ssize_t) number_hull_vertices; k++)
    {
      double
        projection;

      /*
        Rotating calipers.
      */
      projection=getProjection(&vertices[p],&vertices[q],&vertices[k]);
      min_projection=MagickMin(min_projection,projection);
      max_projection=MagickMax(max_projection,projection);
    }
    area=min_diameter*(max_projection-min_projection);
    if (caliper_info.area > area)
      {
        caliper_info.area=area;
        caliper_info.width=min_diameter;
        caliper_info.height=max_projection-min_projection;
        caliper_info.projection=max_projection;
        caliper_info.p=p;
        caliper_info.q=q;
        caliper_info.v=v;
      }
  }
  /*
    Initialize minimum bounding box.
  */
  diameter=getFeretDiameter(&vertices[caliper_info.p],
    &vertices[caliper_info.q],&vertices[caliper_info.v]);
  angle=atan2(vertices[caliper_info.q].y-vertices[caliper_info.p].y,
    vertices[caliper_info.q].x-vertices[caliper_info.p].x);
  bounding_box[0].x=vertices[caliper_info.p].x+cos(angle)*
    caliper_info.projection;
  bounding_box[0].y=vertices[caliper_info.p].y+sin(angle)*
    caliper_info.projection;
  bounding_box[1].x=floor(bounding_box[0].x+cos(angle+MagickPI/2.0)*diameter+
    0.5);
  bounding_box[1].y=floor(bounding_box[0].y+sin(angle+MagickPI/2.0)*diameter+
    0.5);
  bounding_box[2].x=floor(bounding_box[1].x+cos(angle)*(-caliper_info.height)+
    0.5);
  bounding_box[2].y=floor(bounding_box[1].y+sin(angle)*(-caliper_info.height)+
    0.5);
  bounding_box[3].x=floor(bounding_box[2].x+cos(angle+MagickPI/2.0)*(-diameter)+
    0.5);
  bounding_box[3].y=floor(bounding_box[2].y+sin(angle+MagickPI/2.0)*(-diameter)+
    0.5);
  /*
    Export minimum bounding box properties.
  */
  SetMinimumBoundingBoxProperties(image,caliper_info,vertices);
  /*
    Find smallest angle to origin.
  */
  angle=GetMinimumBoxAngle(bounding_box);
  artifact=GetImageArtifact(image,"minimum-bounding-box:orientation");
  if (artifact != (const char *) NULL)
    {
      double
        length,
        q_length,
        p_length;

      PointInfo
        delta,
        point;

      /*
        Find smallest perpendicular distance from edge to origin.
      */
      MoveBoxToOrigin(bounding_box);
      for (i=0; i < 4; i++)
      {
        double
          d,
          intercept,
          slope;

        delta.x=bounding_box[(i+1) % 4].x-bounding_box[i].x;
        delta.y=bounding_box[(i+1) % 4].y-bounding_box[i].y;
        slope=delta.y*MagickSafeReciprocal(delta.x);
        intercept=bounding_box[(i+1) % 4].y-slope*bounding_box[i].x;
        d=fabs((slope*bounding_box[i].x-bounding_box[i].y+intercept)*
          MagickSafeReciprocal(sqrt(slope*slope+1.0)));
        if ((i == 0) || (d < distance))
          {
            distance=d;
            point=delta;
          }
      }
      angle=RadiansToDegrees(atan(point.y*MagickSafeReciprocal(point.x)));
      length=hypot(point.x,point.y);
      p_length=fabs((double) MagickMax(caliper_info.width,caliper_info.height)-
        length);
      q_length=fabs(length-(double) MagickMin(caliper_info.width,
        caliper_info.height));
      if (LocaleCompare(artifact,"landscape") == 0)
        {
          if (p_length > q_length)
            angle+=(angle < 0.0) ? 90.0 : -90.0;
        }
      else
        if (LocaleCompare(artifact,"portrait") == 0)
          {
            if (p_length < q_length)
              angle+=(angle >= 0.0) ? 90.0 : -90.0;
          }
    }
  (void) FormatImageProperty(image,"minimum-bounding-box:angle","%.*g",
    GetMagickPrecision(),angle);
  (void) FormatImageProperty(image,"minimum-bounding-box:unrotate","%.*g",
    GetMagickPrecision(),-angle);
  vertices=(PointInfo *) RelinquishMagickMemory(vertices);
  return(bounding_box);
}
