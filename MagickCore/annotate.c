/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           AAA   N   N  N   N   OOO   TTTTT   AAA   TTTTT  EEEEE             %
%          A   A  NN  N  NN  N  O   O    T    A   A    T    E                 %
%          AAAAA  N N N  N N N  O   O    T    AAAAA    T    EEE               %
%          A   A  N  NN  N  NN  O   O    T    A   A    T    E                 %
%          A   A  N   N  N   N   OOO     T    A   A    T    EEEEE             %
%                                                                             %
%                                                                             %
%                   MagickCore Image Annotation Methods                       %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
% Digital Applications (www.digapp.com) contributed the stroked text algorithm.
% It was written by Leonard Rosenthol.
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/annotate.h"
#include "MagickCore/annotate-private.h"
#include "MagickCore/attribute.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/log.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/policy.h"
#include "MagickCore/property.h"
#include "MagickCore/resource_.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/type.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"
#include "coders/ghostscript-private.h"
#if defined(MAGICKCORE_FREETYPE_DELEGATE)
#if defined(__MINGW32__)
#  undef interface
#endif
#include <ft2build.h>
#if defined(FT_FREETYPE_H)
#  include FT_FREETYPE_H
#else
#  include <freetype/freetype.h>
#endif
#if defined(FT_GLYPH_H)
#  include FT_GLYPH_H
#else
#  include <freetype/ftglyph.h>
#endif
#if defined(FT_OUTLINE_H)
#  include FT_OUTLINE_H
#else
#  include <freetype/ftoutln.h>
#endif
#if defined(FT_BBOX_H)
#  include FT_BBOX_H
#else
#  include <freetype/ftbbox.h>
#endif
#if defined(FT_MODULE_H)
#  include FT_MODULE_H
#else
#  include <freetype/ftmodapi.h>
#endif
#endif
#if defined(MAGICKCORE_RAQM_DELEGATE)
#include <raqm.h>
#endif
typedef struct _GraphemeInfo
{
  ssize_t
    index,
    x_offset,
    x_advance,
    y_offset,
    y_advance;

  size_t
    cluster;
} GraphemeInfo;

typedef struct _FreetypeTarget
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  const char
    *encoding;
} FreetypeTarget;

typedef struct _RenderTarget
{
  Image
    *image;

  const DrawInfo
    *draw_info;
} RenderTarget;

/*
  Annotate semaphores.
*/
static SemaphoreInfo
  *annotate_semaphore = (SemaphoreInfo *) NULL;

/*
  Forward declarations.
*/
static MagickBooleanType
  RenderType(const RenderTarget *,const PointInfo *,TypeMetric *,
    ExceptionInfo *),
  RenderPostscript(Image *,const DrawInfo *,const PointInfo *,TypeMetric *,
    ExceptionInfo *),
  RenderFreetype(const FreetypeTarget *,const PointInfo *,TypeMetric *,
    ExceptionInfo *),
  RenderX11(Image *,const DrawInfo *,const PointInfo *,TypeMetric *,
    ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   A n n o t a t e C o m p o n e n t G e n e s i s                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnnotateComponentGenesis() instantiates the annotate component.
%
%  The format of the AnnotateComponentGenesis method is:
%
%      MagickBooleanType AnnotateComponentGenesis(void)
%
*/
MagickPrivate MagickBooleanType AnnotateComponentGenesis(void)
{
  if (annotate_semaphore == (SemaphoreInfo *) NULL)
    annotate_semaphore=AcquireSemaphoreInfo();
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   A n n o t a t e C o m p o n e n t T e r m i n u s                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnnotateComponentTerminus() destroys the annotate component.
%
%  The format of the AnnotateComponentTerminus method is:
%
%      AnnotateComponentTerminus(void)
%
*/
MagickPrivate void AnnotateComponentTerminus(void)
{
  if (annotate_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&annotate_semaphore);
  RelinquishSemaphoreInfo(&annotate_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   A n n o t a t e I m a g e                                                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnnotateImage() annotates an image with text.
%
%  The format of the AnnotateImage method is:
%
%      MagickBooleanType AnnotateImage(Image *image,DrawInfo *draw_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline double GravityOffsetSign(const size_t extent)
{
  return(extent == 0 ? -1.0 : 1.0);
}

static inline double GravityOffsetFlip(const size_t extent)
{
  return(extent == 0 ? 1.0 : -1.0);
}

static void SplitTextLines(char *text,char **textlist,
  const size_t number_lines)
{
  char
    *p;

  ssize_t
    i;

  p=text;
  for (i=0; i < (ssize_t) number_lines; i++)
  {
    char
      *q;

    textlist[i]=p;
    for (q=p; *q != '\0'; q++)
      if ((*q == '\r') || (*q == '\n'))
        break;
    if (*q == '\r')
      {
        *q='\0';
        q++;
      }
    *q='\0';
    p=q+1;
  }
  textlist[i]=(char *) NULL;
}

typedef struct _GravityTextInfo
{
  DrawInfo
    *annotate,
    *annotate_info;

  RectangleInfo
    geometry;

  size_t
    height;

  ssize_t
    i;

  TypeMetric
    metrics;

  size_t
    number_lines;
} GravityTextInfo;

static void GetGravityTextOffset(const GravityTextInfo *gravity,
  PointInfo *offset)
{
  DrawInfo
    *const annotate = gravity->annotate,
    *const annotate_info = gravity->annotate_info;

  const RectangleInfo
    geometry = gravity->geometry;

  const size_t
    height = gravity->height;

  const ssize_t
    i = gravity->i;

  const TypeMetric
    metrics = gravity->metrics;

  const size_t
    number_lines = gravity->number_lines;

  switch (annotate->gravity)
  {
    case UndefinedGravity:
    default:
    {
      (*offset).x=annotate_info->affine.tx+i*annotate_info->affine.ry*height;
      (*offset).y=annotate_info->affine.ty+i*annotate_info->affine.sy*height;
      break;
    }
    case (GravityType) SouthWestGravity:
    {
      (*offset).x=GravityOffsetSign(geometry.width)*annotate_info->affine.tx+i*
        annotate_info->affine.ry*height-annotate_info->affine.ry*
        (number_lines-1.0)*height;
      (*offset).y=GravityOffsetFlip(geometry.height)*annotate_info->affine.ty+
        geometry.height+i*annotate_info->affine.sy*height-
        annotate_info->affine.sy*(number_lines-1.0)*height+metrics.descent;
      break;
    }
    case (GravityType) SouthGravity:
    {
      (*offset).x=GravityOffsetSign(geometry.width)*annotate_info->affine.tx+
        geometry.width/2.0+i*annotate_info->affine.ry*height-
        annotate_info->affine.sx*metrics.width/2.0-
        annotate_info->affine.ry*(number_lines-1.0)*height/2.0;
      (*offset).y=GravityOffsetFlip(geometry.height)*annotate_info->affine.ty+
        geometry.height+i*annotate_info->affine.sy*height-
        annotate_info->affine.rx*metrics.width/2.0-
        annotate_info->affine.sy*(number_lines-1.0)*height+metrics.descent;
      break;
    }
    case (GravityType) SouthEastGravity:
    {
      (*offset).x=GravityOffsetFlip(geometry.width)*annotate_info->affine.tx+
        geometry.width+i*annotate_info->affine.ry*height-
        annotate_info->affine.sx*metrics.width-
        annotate_info->affine.ry*(number_lines-1.0)*height-1.0;
      (*offset).y=GravityOffsetFlip(geometry.height)*annotate_info->affine.ty+
        geometry.height+i*annotate_info->affine.sy*height-
        annotate_info->affine.rx*metrics.width-
        annotate_info->affine.sy*(number_lines-1.0)*height+metrics.descent;
      break;
    }
  }
}

typedef struct _TextAlignInfo
{
  DrawInfo
    *annotate,
    *annotate_info;

  size_t
    height;

  ssize_t
    i;

  TypeMetric
    metrics;
} TextAlignInfo;

static void GetAnnotateAlignOffset(const TextAlignInfo *text_align,
  PointInfo *offset)
{
  DrawInfo
    *const annotate = text_align->annotate,
    *const annotate_info = text_align->annotate_info;

  const size_t
    height = text_align->height;

  const ssize_t
    i = text_align->i;

  const TypeMetric
    metrics = text_align->metrics;

  switch (annotate->align)
  {
    case LeftAlign:
    {
      (*offset).x=annotate_info->affine.tx+i*annotate_info->affine.ry*height;
      (*offset).y=annotate_info->affine.ty+i*annotate_info->affine.sy*height;
      break;
    }
    case CenterAlign:
    {
      (*offset).x=annotate_info->affine.tx+i*annotate_info->affine.ry*height-
        annotate_info->affine.sx*metrics.width/2.0;
      (*offset).y=annotate_info->affine.ty+i*annotate_info->affine.sy*height-
        annotate_info->affine.rx*metrics.width/2.0;
      break;
    }
    case RightAlign:
    {
      (*offset).x=annotate_info->affine.tx+i*annotate_info->affine.ry*height-
        annotate_info->affine.sx*metrics.width;
      (*offset).y=annotate_info->affine.ty+i*annotate_info->affine.sy*height-
        annotate_info->affine.rx*metrics.width;
      break;
    }
    default:
      break;
  }
}

static size_t CountTextLines(const char *text)
{
  const char
    *p;

  size_t
    number_lines;

  number_lines=1;
  for (p=text; *p != '\0'; p++)
    if (*p == '\n')
      number_lines++;
  return(number_lines);
}

static size_t GetTextLineHeight(const TypeMetric *metrics,
  const DrawInfo *draw_info)
{
  size_t
    height;

  height=CastDoubleToSizeT(metrics->ascent-metrics->descent+0.5);
  if (height == 0)
    height=(size_t) draw_info->pointsize;
  height=CastDoubleToSizeT(floor((double) height+
    draw_info->interline_spacing+0.5));
  return(height);
}

static PixelInfo GetDecorationColor(const DrawInfo *annotate_info)
{
  PixelInfo
    pixel;

  pixel=annotate_info->fill;
  if (annotate_info->stroke.alpha != (MagickRealType) TransparentAlpha)
    pixel=annotate_info->stroke;
  return(pixel);
}

static void GetUpperGravityTextOffset(const GravityTextInfo *gravity,
  PointInfo *offset)
{
  DrawInfo
    *const annotate = gravity->annotate,
    *const annotate_info = gravity->annotate_info;

  const RectangleInfo
    geometry = gravity->geometry;

  const size_t
    height = gravity->height;

  const ssize_t
    i = gravity->i;

  const TypeMetric
    metrics = gravity->metrics;

  const size_t
    number_lines = gravity->number_lines;

  switch (annotate->gravity)
  {
    default:
    {
      GetGravityTextOffset(gravity,offset);
      break;
    }
    case (GravityType) NorthGravity:
    {
      (*offset).x=GravityOffsetSign(geometry.width)*annotate_info->affine.tx+
        geometry.width/2.0+i*annotate_info->affine.ry*height-
        annotate_info->affine.sx*metrics.width/2.0+annotate_info->affine.ry*
        (metrics.ascent+metrics.descent);
      (*offset).y=GravityOffsetSign(geometry.height)*annotate_info->affine.ty+
        (metrics.bounds.y2-metrics.ascent)+i*annotate_info->affine.sy*height+
        annotate_info->affine.sy*metrics.ascent-annotate_info->affine.rx*
        metrics.width/2.0;
      break;
    }
    case (GravityType) NorthEastGravity:
    {
      (*offset).x=GravityOffsetFlip(geometry.width)*annotate_info->affine.tx+
        geometry.width+i*annotate_info->affine.ry*height-
        annotate_info->affine.sx*metrics.width+annotate_info->affine.ry*
        (metrics.ascent+metrics.descent)-1.0;
      (*offset).y=GravityOffsetSign(geometry.height)*annotate_info->affine.ty+
        (metrics.bounds.y2-metrics.ascent)+i*annotate_info->affine.sy*height+
        annotate_info->affine.sy*metrics.ascent-annotate_info->affine.rx*
        metrics.width;
      break;
    }
    case (GravityType) WestGravity:
    {
      (*offset).x=GravityOffsetSign(geometry.width)*annotate_info->affine.tx+i*
        annotate_info->affine.ry*height+annotate_info->affine.ry*
        (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
      (*offset).y=GravityOffsetSign(geometry.height)*annotate_info->affine.ty+
        geometry.height/2.0+i*annotate_info->affine.sy*height+
        annotate_info->affine.sy*(metrics.ascent+metrics.descent-
        (number_lines-1.0)*height)/2.0;
      break;
    }
  }
}

static void GetCenterGravityTextOffset(const GravityTextInfo *gravity,
  PointInfo *offset)
{
  DrawInfo
    *const annotate = gravity->annotate,
    *const annotate_info = gravity->annotate_info;

  const RectangleInfo
    geometry = gravity->geometry;

  const size_t
    height = gravity->height;

  const ssize_t
    i = gravity->i;

  const TypeMetric
    metrics = gravity->metrics;

  const size_t
    number_lines = gravity->number_lines;

  switch (annotate->gravity)
  {
    default:
    {
      GetUpperGravityTextOffset(gravity,offset);
      break;
    }
    case (GravityType) CenterGravity:
    {
      (*offset).x=GravityOffsetSign(geometry.width)*annotate_info->affine.tx+
        geometry.width/2.0+i*annotate_info->affine.ry*height-
        annotate_info->affine.sx*metrics.width/2.0+annotate_info->affine.ry*
        (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
      (*offset).y=GravityOffsetSign(geometry.height)*annotate_info->affine.ty+
        geometry.height/2.0+i*annotate_info->affine.sy*height-
        annotate_info->affine.rx*metrics.width/2.0+annotate_info->affine.sy*
        (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
      break;
    }
    case (GravityType) EastGravity:
    {
      (*offset).x=GravityOffsetFlip(geometry.width)*annotate_info->affine.tx+
        geometry.width+i*annotate_info->affine.ry*height-
        annotate_info->affine.sx*metrics.width+
        annotate_info->affine.ry*(metrics.ascent+metrics.descent-
        (number_lines-1.0)*height)/2.0-1.0;
      (*offset).y=GravityOffsetSign(geometry.height)*annotate_info->affine.ty+
        geometry.height/2.0+i*annotate_info->affine.sy*height-
        annotate_info->affine.rx*metrics.width+
        annotate_info->affine.sy*(metrics.ascent+metrics.descent-
        (number_lines-1.0)*height)/2.0;
      break;
    }
  }
}

static void GetAnnotateTextOffset(const GravityTextInfo *gravity,
  PointInfo *offset)
{
  DrawInfo
    *const annotate = gravity->annotate,
    *const annotate_info = gravity->annotate_info;

  const RectangleInfo
    geometry = gravity->geometry;

  const size_t
    height = gravity->height;

  const ssize_t
    i = gravity->i;

  const TypeMetric
    metrics = gravity->metrics;

  switch (annotate->gravity)
  {
    default:
    {
      GetCenterGravityTextOffset(gravity,offset);
      break;
    }
    case (GravityType) NorthWestGravity:
    {
      (*offset).x=GravityOffsetSign(geometry.width)*annotate_info->affine.tx+i*
        annotate_info->affine.ry*height+annotate_info->affine.ry*
        (metrics.ascent+metrics.descent);
      (*offset).y=GravityOffsetSign(geometry.height)*annotate_info->affine.ty+
        (metrics.bounds.y2-metrics.ascent)+i*annotate_info->affine.sy*height+
        annotate_info->affine.sy*metrics.ascent;
      break;
    }
  }
}

typedef struct _TextDecorationInfo
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  DrawInfo
    *annotate;

  TypeMetric
    metrics;

  char
    *primitive;

  size_t
    height;

  ExceptionInfo
    *exception;
} TextDecorationInfo;

static void DrawTextDecoration(const TextDecorationInfo *decoration,
  DrawInfo *annotate_info)
{
  Image
    *const image = decoration->image;

  const DrawInfo
    *const draw_info = decoration->draw_info;

  DrawInfo
    *const annotate = decoration->annotate;

  const TypeMetric
    metrics = decoration->metrics;

  const char
    *const primitive = decoration->primitive;

  ExceptionInfo
    *const exception = decoration->exception;

  switch (annotate->decorate)
  {
    case OverlineDecoration:
    {
      annotate_info->affine.ty-=(draw_info->affine.sy*(metrics.ascent+
        metrics.descent-metrics.underline_position));
      (void) CloneString(&annotate_info->primitive,primitive);
      (void) DrawImage(image,annotate_info,exception);
      break;
    }
    case UnderlineDecoration:
    {
      annotate_info->affine.ty-=(draw_info->affine.sy*
        metrics.underline_position);
      (void) CloneString(&annotate_info->primitive,primitive);
      (void) DrawImage(image,annotate_info,exception);
      break;
    }
    default:
      break;
  }
}

static void DrawTextUndercolor(const TextDecorationInfo *decoration,
  const PointInfo *offset)
{
  Image
    *const image = decoration->image;

  const DrawInfo
    *const draw_info = decoration->draw_info;

  const TypeMetric
    metrics = decoration->metrics;

  char
    *const primitive = decoration->primitive;

  const size_t
    height = decoration->height;

  ExceptionInfo
    *const exception = decoration->exception;

  if (draw_info->undercolor.alpha != (MagickRealType) TransparentAlpha)
    {
      DrawInfo
        *undercolor_info;

      /*
        Text box.
      */
      undercolor_info=CloneDrawInfo((ImageInfo *) NULL,(DrawInfo *) NULL);
      undercolor_info->fill=draw_info->undercolor;
      undercolor_info->affine=draw_info->affine;
      undercolor_info->affine.tx=(*offset).x-draw_info->affine.ry*
        metrics.ascent;
      undercolor_info->affine.ty=(*offset).y-draw_info->affine.sy*
        metrics.ascent;
      (void) FormatLocaleString(primitive,MagickPathExtent,
        "rectangle 0.0,0.0 %g,%g",metrics.origin.x,(double) height);
      (void) CloneString(&undercolor_info->primitive,primitive);
      (void) DrawImage(image,undercolor_info,exception);
      (void) DestroyDrawInfo(undercolor_info);
    }
}

static void DrawTextLineThrough(const TextDecorationInfo *decoration,
  DrawInfo *annotate_info)
{
  Image
    *const image = decoration->image;

  const DrawInfo
    *const draw_info = decoration->draw_info;

  DrawInfo
    *const annotate = decoration->annotate;

  const TypeMetric
    metrics = decoration->metrics;

  const char
    *const primitive = decoration->primitive;

  const size_t
    height = decoration->height;

  ExceptionInfo
    *const exception = decoration->exception;

  if (annotate->decorate == LineThroughDecoration) {
      annotate_info->affine.ty-=(draw_info->affine.sy*(height+
        metrics.underline_position+metrics.descent*2)/2.0);
      (void) CloneString(&annotate_info->primitive,primitive);
      (void) DrawImage(image,annotate_info,exception);
  }
}

static void DestroyAnnotateText(DrawInfo *annotate_info,DrawInfo *annotate,
  char **textlist,char *text)
{
  annotate_info=DestroyDrawInfo(annotate_info);
  annotate=DestroyDrawInfo(annotate);
  textlist=(char **) RelinquishMagickMemory(textlist);
  text=DestroyString(text);
}

typedef struct _AnnotateLineInfo
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  DrawInfo
    *annotate,
    *annotate_info;

  RectangleInfo
    geometry;

  GeometryInfo
    geometry_info;

  char
    **textlist;

  size_t
    number_lines;

  ExceptionInfo
    *exception;
} AnnotateLineInfo;

typedef struct _AnnotateLineTargets
{
  TypeMetric
    *metrics;

  MagickBooleanType
    *status;
} AnnotateLineTargets;

static void AnnotateTextLines(const AnnotateLineInfo *line,
  const AnnotateLineTargets *targets)
{
  TypeMetric
    *const metrics = targets->metrics;

  MagickBooleanType
    *const status = targets->status;

  char
    color[MagickPathExtent],
    primitive[MagickPathExtent];

  PixelInfo
    pixel;

  PointInfo
    offset;

  size_t
    height;

  ssize_t
    i;

  for (i=0; line->textlist[i] != (char *) NULL; i++)
  {
    const RenderTarget target = { line->image, line->annotate };

    TextDecorationInfo
      decoration;

    if (*line->textlist[i] == '\0')
      continue;
    /*
      Position text relative to image.
    */
    line->annotate_info->affine.tx=line->geometry_info.xi-(double)
      line->image->page.x;
    line->annotate_info->affine.ty=line->geometry_info.psi-(double)
      line->image->page.y;
    (void) CloneString(&line->annotate->text,line->textlist[i]);
    if (((*metrics).width == 0) ||
        (line->annotate->gravity != NorthWestGravity))
      (void) GetTypeMetrics(line->image,line->annotate,&(*metrics),
        line->exception);
    height=GetTextLineHeight(&(*metrics),line->draw_info);
    {
      const GravityTextInfo gravity = { line->annotate, line->annotate_info,
        line->geometry, height, i, (*metrics), line->number_lines };

      GetAnnotateTextOffset(&gravity,&offset);
    }
    {
      const TextAlignInfo text_align = { line->annotate, line->annotate_info,
        height, i, (*metrics) };

      GetAnnotateAlignOffset(&text_align,&offset);
    }
    decoration.image=line->image;
    decoration.draw_info=line->draw_info;
    decoration.annotate=line->annotate;
    decoration.metrics=(*metrics);
    decoration.primitive=primitive;
    decoration.exception=line->exception;
    decoration.height=height;
    DrawTextUndercolor(&decoration,&offset);
    line->annotate_info->affine.tx=offset.x;
    line->annotate_info->affine.ty=offset.y;
    pixel=GetDecorationColor(line->annotate_info);
    (void) QueryColorname(line->image,&pixel,AllCompliance,color,
      line->exception);
    (void) FormatLocaleString(primitive,MagickPathExtent,"stroke %s "
      "stroke-width %g line 0,0 %g,0",color,(double)
      (*metrics).underline_thickness,(double) (*metrics).width);
    /*
      Annotate image with text.
    */
    DrawTextDecoration(&decoration,line->annotate_info);
    (*status)=RenderType(&target,&offset,&(*metrics),line->exception);
    if ((*status) == MagickFalse)
      break;

    decoration.metrics=(*metrics);
    DrawTextLineThrough(&decoration,line->annotate_info);
  }
}

static void LogAnnotateImage(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
}

MagickExport MagickBooleanType AnnotateImage(Image *image,
  const DrawInfo *draw_info,ExceptionInfo *exception)
{
  char
    *text,
    **textlist;

  DrawInfo
    *annotate,
    *annotate_info;

  GeometryInfo
    geometry_info;

  MagickBooleanType
    status;

  RectangleInfo
    geometry;

  size_t
    number_lines;

  TypeMetric
    metrics;

  const AnnotateLineTargets
    targets = { &metrics, &status };

  LogAnnotateImage(image);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  if (draw_info->text == (char *) NULL)
    return(MagickFalse);
  if (*draw_info->text == '\0')
    return(MagickTrue);
  annotate=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  text=annotate->text;
  annotate->text=(char *) NULL;
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  number_lines=CountTextLines(text);
  textlist=(char **) AcquireQuantumMemory(number_lines+1,sizeof(*textlist));
  if (textlist == (char **) NULL)
    {
      annotate_info=DestroyDrawInfo(annotate_info);
      annotate=DestroyDrawInfo(annotate);
      text=DestroyString(text);
      return(MagickFalse);
    }
  SplitTextLines(text,textlist,number_lines);
  SetGeometry(image,&geometry);
  SetGeometryInfo(&geometry_info);
  if (annotate_info->geometry != (char *) NULL)
    {
      (void) ParsePageGeometry(image,annotate_info->geometry,&geometry,
        exception);
      (void) ParseGeometry(annotate_info->geometry,&geometry_info);
    }
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    {
      DestroyAnnotateText(annotate_info,annotate,textlist,text);
      return(MagickFalse);
    }
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  status=MagickTrue;
  (void) memset(&metrics,0,sizeof(metrics));
  {
    const AnnotateLineInfo line = { image, draw_info, annotate, annotate_info,
      geometry, geometry_info, textlist, number_lines, exception };

    AnnotateTextLines(&line,&targets);
  }
  /*
    Relinquish resources.
  */
  DestroyAnnotateText(annotate_info,annotate,textlist,text);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t M a g i c k C a p t i o n                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatMagickCaption() formats a caption so that it fits within the image
%  width.  It returns the number of lines in the formatted caption.
%
%  The format of the FormatMagickCaption method is:
%
%      ssize_t FormatMagickCaption(Image *image,DrawInfo *draw_info,
%        const MagickBooleanType split,TypeMetric *metrics,char **caption,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image:  The image.
%
%    o draw_info: the draw info.
%
%    o split: when no convenient line breaks-- insert newline.
%
%    o metrics: Return the font metrics in this structure.
%
%    o caption: the caption.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline char *ReplaceSpaceWithNewline(char **caption,char *space)
{
  size_t
    octets;

  if ((caption == (char **) NULL) || (space == (char *) NULL))
    return((char *) NULL);
  octets=(size_t) GetUTFOctets(space);
  if (octets == 1)
    *space='\n';
  else
    {
      char
        *target;

      size_t
        length;

      ssize_t
        offset;

      length=strlen(*caption);
      *space='\n';
      offset=space-(*caption);
      if (offset >= 0)
        {
          target=AcquireString(*caption);
          (void) CopyMagickString(target,*caption,(size_t) offset+2);
          (void) ConcatenateMagickString(target,space+octets,length);
          (void) DestroyString(*caption);
          *caption=target;
          space=(*caption)+offset;
        }
    }
  return(space);
}

static inline MagickBooleanType IsBreakableSpace(const DrawInfo *draw_info,
  const int code)
{
  return(((draw_info->word_break != BreakWordBreakType) &&
    (IsUTFSpace(code) != MagickFalse) &&
    (IsNonBreakingUTFSpace(code) == MagickFalse)) ? MagickTrue : MagickFalse);
}

static inline char *ReplaceWideSpace(char **caption,char *p,
  const size_t width,const size_t columns)
{
  if (width > columns)
    p=ReplaceSpaceWithNewline(caption,p);
  return(p);
}

static inline char *InsertNewlineBeforeSpace(char **caption,char *p)
{
  ssize_t
    n;

  n=p-(*caption);
  if ((n > 0) && ((*caption)[n-1] != '\n'))
    {
      char
        *target;

      target=AcquireString(*caption);
      (void) CopyMagickString(target,*caption,(size_t) n+1);
      (void) ConcatenateMagickString(target,"\n",strlen(*caption)+1);
      (void) ConcatenateMagickString(target,p,strlen(*caption)+2);
      (void) DestroyString(*caption);
      *caption=target;
      p=(*caption)+n;
    }
  return(p);
}

static inline ssize_t CountCaptionNewlines(char *caption)
{
  char
    *p;

  ssize_t
    n;

  n=0;
  for (p=caption; GetUTFCode(p) != 0; p+=(ptrdiff_t) GetUTFOctets(p))
    if (GetUTFCode(p) == '\n')
      n++;
  return(n);
}

static inline char *AppendUTFCharacter(char *q,const char *p)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetUTFOctets(p); i++)
    *q++=(*(p+i));
  return(q);
}

static inline char *BreakCaptionLine(char **caption,char *p,char *s,
  const MagickBooleanType split)
{
  if (s != (char *) NULL)
    p=ReplaceSpaceWithNewline(caption,s);
  else
    if ((split != MagickFalse) || (GetUTFOctets(p) > 2))
      {
        /*
          No convenient line breaks-- insert newline.
        */
        p=InsertNewlineBeforeSpace(caption,p);
      }
  return(p);
}

MagickExport ssize_t FormatMagickCaption(Image *image,DrawInfo *draw_info,
  const MagickBooleanType split,TypeMetric *metrics,char **caption,
  ExceptionInfo *exception)
{
  char
    *p,
    *q,
    *s;

  MagickBooleanType
    status;

  size_t
    width;

  ssize_t
    n;

  q=draw_info->text;
  s=(char *) NULL;
  width=0;
  for (p=(*caption); GetUTFCode(p) != 0; p+=(ptrdiff_t) GetUTFOctets(p))
  {
    int
      code;

    code=GetUTFCode(p);
    if (code == '\n')
      {
        q=draw_info->text;
        continue;
      }
    if (IsBreakableSpace(draw_info,code) != MagickFalse)
      {
        p=ReplaceWideSpace(caption,p,width,image->columns);
        s=p;
      }
    q=AppendUTFCharacter(q,p);
    *q='\0';
    status=GetTypeMetrics(image,draw_info,metrics,exception);
    if (status == MagickFalse)
      break;
    width=CastDoubleToSizeT(metrics->width+draw_info->stroke_width+0.5);
    if (width <= image->columns)
      continue;
    p=BreakCaptionLine(caption,p,s,split);
    q=draw_info->text;
    s=(char *) NULL;
  }
  n=CountCaptionNewlines(*caption);
  return(n);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%   G e t T y p e M e t r i c s                                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetTypeMetrics() returns the following information for the specified font
%  and text:
%
%    character width
%    character height
%    ascender
%    descender
%    text width
%    text height
%    maximum horizontal advance
%    bounds: x1
%    bounds: y1
%    bounds: x2
%    bounds: y2
%    origin: x
%    origin: y
%    underline position
%    underline thickness
%
%  The format of the GetTypeMetrics method is:
%
%      MagickBooleanType GetTypeMetrics(Image *image,const DrawInfo *draw_info,
%        TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o metrics: Return the font metrics in this structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType GetTypeMetrics(Image *image,
  const DrawInfo *draw_info,TypeMetric *metrics,ExceptionInfo *exception)
{
  DrawInfo
    *annotate_info;

  MagickBooleanType
    status;

  PointInfo
    offset;

  RenderTarget target;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->text != (char *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  annotate_info->render=MagickFalse;
  annotate_info->direction=UndefinedDirection;
  (void) memset(metrics,0,sizeof(*metrics));
  offset.x=0.0;
  offset.y=0.0;
  target.image=image;
  target.draw_info=annotate_info;
  status=RenderType(&target,&offset,metrics,exception);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),"Metrics: text: %s; "
      "width: %g; height: %g; ascent: %g; descent: %g; max advance: %g; "
      "bounds: %g,%g  %g,%g; origin: %g,%g; pixels per em: %g,%g; "
      "underline position: %g; underline thickness: %g",annotate_info->text,
      metrics->width,metrics->height,metrics->ascent,metrics->descent,
      metrics->max_advance,metrics->bounds.x1,metrics->bounds.y1,
      metrics->bounds.x2,metrics->bounds.y2,metrics->origin.x,metrics->origin.y,
      metrics->pixels_per_em.x,metrics->pixels_per_em.y,
      metrics->underline_position,metrics->underline_thickness);
  annotate_info=DestroyDrawInfo(annotate_info);
  return(status);
}

#include "MagickCore/annotate-type-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e n d e r F r e e t y p e                                               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderFreetype() renders text on the image with a Truetype font.  It also
%  returns the bounding box of the text relative to the image.
%
%  The format of the RenderFreetype method is:
%
%      MagickBooleanType RenderFreetype(const FreetypeTarget *target,
%        const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o target: the image, the draw info and the font encoding.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(MAGICKCORE_FREETYPE_DELEGATE)

#include "MagickCore/annotate-raqm-private.h"
#include "MagickCore/annotate-freetype-private.h"

static FT_Outline_Funcs
  OutlineMethods =
  {
    (FT_Outline_MoveTo_Func) TraceMoveTo,
    (FT_Outline_LineTo_Func) TraceLineTo,
    (FT_Outline_ConicTo_Func) TraceQuadraticBezier,
    (FT_Outline_CubicTo_Func) TraceCubicBezier,
    0, 0
  };

#include "MagickCore/annotate-freetype-render-private.h"
#else
static MagickBooleanType RenderFreetype(const FreetypeTarget *target,
  const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
{
  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  (void) ThrowMagickException(exception,GetMagickModule(),
    MissingDelegateWarning,"DelegateLibrarySupportNotBuiltIn","'%s' (Freetype)",
    draw_info->font != (char *) NULL ? draw_info->font : "none");
  return(RenderPostscript(image,draw_info,offset,metrics,exception));
}
#endif

#include "MagickCore/annotate-render-private.h"
