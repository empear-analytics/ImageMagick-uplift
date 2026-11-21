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

  MagickCore private draw primitive logging and stroke predicates.
*/
#ifndef MAGICKCORE_DRAW_PRIMITIVE_LOG_PRIVATE_H
#define MAGICKCORE_DRAW_PRIMITIVE_LOG_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
