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

  MagickCore drawing core methods.
*/
#ifndef MAGICKCORE_DRAW_CORE_PRIVATE_H
#define MAGICKCORE_DRAW_CORE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Define declarations.
*/
#define BezierQuantum  200

/*
  Method prototypes.
*/
static MagickBooleanType
  TraceRectangle(PrimitiveInfo *,const PointInfo,const PointInfo);

/*
  Helper functions.
*/
static inline double GetDrawValue(const char *magick_restrict string,
  char **magick_restrict sentinel)
{
  char
    **magick_restrict q;

  double
    value;

  q=sentinel;
  value=InterpretLocaleValue(string,q);
  sentinel=q;
  return(value);
}

static inline MagickBooleanType TracePoint(PrimitiveInfo *primitive_info,
  const PointInfo point)
{
  primitive_info->point=point;
  primitive_info->coordinates=1;
  primitive_info->closed_subpath=MagickFalse;
  primitive_info->text=(char *) NULL;
  return(MagickTrue);
}

static inline MagickBooleanType IsClosedEllipse(const double x,const double y)
{
  return(((x < MagickEpsilon) && (y < MagickEpsilon)) ? MagickTrue :
    MagickFalse);
}

static inline void SetTracedPrimitiveType(const PrimitiveInfo *primitive_info,
  PrimitiveInfo *p)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) primitive_info->coordinates; i++)
  {
    p->primitive=primitive_info->primitive;
    p--;
  }
}

static inline MagickBooleanType TraceRectangle(PrimitiveInfo *primitive_info,
  const PointInfo start,const PointInfo end)
{
  PointInfo
    point;

  PrimitiveInfo
    *p;


  p=primitive_info;
  if (TracePoint(p,start) == MagickFalse)
    return(MagickFalse);
  p+=(ptrdiff_t) p->coordinates;
  point.x=start.x;
  point.y=end.y;
  if (TracePoint(p,point) == MagickFalse)
    return(MagickFalse);
  p+=(ptrdiff_t) p->coordinates;
  if (TracePoint(p,end) == MagickFalse)
    return(MagickFalse);
  p+=(ptrdiff_t) p->coordinates;
  point.x=end.x;
  point.y=start.y;
  if (TracePoint(p,point) == MagickFalse)
    return(MagickFalse);
  p+=(ptrdiff_t) p->coordinates;
  if (TracePoint(p,start) == MagickFalse)
    return(MagickFalse);
  p+=(ptrdiff_t) p->coordinates;
  primitive_info->coordinates=(size_t) (p-primitive_info);
  primitive_info->closed_subpath=MagickTrue;
  SetTracedPrimitiveType(primitive_info,p);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
