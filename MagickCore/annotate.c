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

MagickExport MagickBooleanType AnnotateImage(Image *image,
  const DrawInfo *draw_info,ExceptionInfo *exception)
{
  char
    color[MagickPathExtent],
    primitive[MagickPathExtent],
    *text,
    **textlist;

  DrawInfo
    *annotate,
    *annotate_info;

  GeometryInfo
    geometry_info;

  MagickBooleanType
    status;

  PixelInfo
    pixel;

  PointInfo
    offset;

  RectangleInfo
    geometry;

  size_t
    height,
    number_lines;

  ssize_t
    i;

  TypeMetric
    metrics;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
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
  for (i=0; textlist[i] != (char *) NULL; i++)
  {
    const RenderTarget target = { image, annotate };

    TextDecorationInfo
      decoration;

    if (*textlist[i] == '\0')
      continue;
    /*
      Position text relative to image.
    */
    annotate_info->affine.tx=geometry_info.xi-(double) image->page.x;
    annotate_info->affine.ty=geometry_info.psi-(double) image->page.y;
    (void) CloneString(&annotate->text,textlist[i]);
    if ((metrics.width == 0) || (annotate->gravity != NorthWestGravity))
      (void) GetTypeMetrics(image,annotate,&metrics,exception);
    height=GetTextLineHeight(&metrics,draw_info);
    {
      const GravityTextInfo gravity = { annotate, annotate_info, geometry,
        height, i, metrics, number_lines };

      GetAnnotateTextOffset(&gravity,&offset);
    }
    {
      const TextAlignInfo text_align = { annotate, annotate_info, height, i,
        metrics };

      GetAnnotateAlignOffset(&text_align,&offset);
    }
    decoration.image=image;
    decoration.draw_info=draw_info;
    decoration.annotate=annotate;
    decoration.metrics=metrics;
    decoration.primitive=primitive;
    decoration.exception=exception;
    decoration.height=height;
    DrawTextUndercolor(&decoration,&offset);
    annotate_info->affine.tx=offset.x;
    annotate_info->affine.ty=offset.y;
    pixel=GetDecorationColor(annotate_info);
    (void) QueryColorname(image,&pixel,AllCompliance,color,exception);
    (void) FormatLocaleString(primitive,MagickPathExtent,"stroke %s "
      "stroke-width %g line 0,0 %g,0",color,(double)
      metrics.underline_thickness,(double) metrics.width);
    /*
      Annotate image with text.
    */
    DrawTextDecoration(&decoration,annotate_info);
    status=RenderType(&target,&offset,&metrics,exception);
    if (status == MagickFalse)
      break;

    decoration.metrics=metrics;
    DrawTextLineThrough(&decoration,annotate_info);
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e n d e r P o s t s c r i p t                                           %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderPostscript() renders text on the image with a Postscript font.  It
%  also returns the bounding box of the text relative to the image.
%
%  The format of the RenderPostscript method is:
%
%      MagickBooleanType RenderPostscript(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/

static inline MagickBooleanType IsPostscriptNameChar(const unsigned char *p)
{
  return(((isalnum(*p) != 0) || (*p == '-') || (*p == '_') ||
    (*p == '.')) ? MagickTrue : MagickFalse);
}

static MagickBooleanType IsValidPostscriptFontname(const char *font)
{
  const unsigned char
    *p;

  if ((font == (const char *) NULL) || (*font == '\0'))
    return(MagickFalse);
  for (p=(const unsigned char *) font; *p != '\0'; p++)
  {
    if (IsPostscriptNameChar(p) != MagickFalse)
      continue;
    return(MagickFalse);
  }
  return(MagickTrue);
}

static PointInfo GetPostscriptExtent(const DrawInfo *draw_info)
{
  PointInfo
    extent,
    point;

  ssize_t
    i;

  size_t
    length;

  extent.x=0.0;
  extent.y=0.0;
  length=strlen(draw_info->text);
  for (i=0; i <= (ssize_t) (length+2); i++)
  {
    point.x=fabs(draw_info->affine.sx*i*draw_info->pointsize+
      draw_info->affine.ry*2.0*draw_info->pointsize);
    point.y=fabs(draw_info->affine.rx*i*draw_info->pointsize+
      draw_info->affine.sy*2.0*draw_info->pointsize);
    if (point.x > extent.x)
      extent.x=point.x;
    if (point.y > extent.y)
      extent.y=point.y;
  }
  return(extent);
}

static void SetPostscriptMetrics(const DrawInfo *draw_info,
  const Image *annotate_image,const PointInfo resolution,TypeMetric *metrics)
{
  metrics->pixels_per_em.x=(resolution.y/DefaultResolution)*
    ExpandAffine(&draw_info->affine)*draw_info->pointsize;
  metrics->pixels_per_em.y=metrics->pixels_per_em.x;
  metrics->ascent=metrics->pixels_per_em.y;
  metrics->descent=metrics->pixels_per_em.y/-5.0;
  metrics->width=(double) annotate_image->columns/
    ExpandAffine(&draw_info->affine);
  metrics->height=floor(metrics->ascent-metrics->descent+0.5);
  metrics->max_advance=metrics->pixels_per_em.x;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=(-2.0);
  metrics->underline_thickness=1.0;
}

static void WritePostscriptPrologue(FILE *file)
{
  (void) FormatLocaleFile(file,"%%!PS-Adobe-3.0\n");
  (void) FormatLocaleFile(file,"/ReencodeType\n");
  (void) FormatLocaleFile(file,"{\n");
  (void) FormatLocaleFile(file,"  findfont dup length\n");
  (void) FormatLocaleFile(file,
    "  dict begin { 1 index /FID ne {def} {pop pop} ifelse } forall\n");
  (void) FormatLocaleFile(file,
    "  /Encoding ISOLatin1Encoding def currentdict end definefont pop\n");
  (void) FormatLocaleFile(file,"} bind def\n");
}

static void WritePostscriptFont(FILE *file,const DrawInfo *draw_info)
{
  if (IsValidPostscriptFontname(draw_info->font) == MagickFalse)
    (void) FormatLocaleFile(file,
      "/Times-Roman-ISO dup /Times-Roman ReencodeType findfont setfont\n");
  else
    (void) FormatLocaleFile(file,
      "/%s-ISO dup /%s ReencodeType findfont setfont\n",draw_info->font,
      draw_info->font);
}

static void WritePostscriptText(FILE *file,const DrawInfo *draw_info,
  const MagickBooleanType identity,ExceptionInfo *exception)
{
  char
    *text;

  text=EscapeParenthesis(draw_info->text,MAGICK_SIZE_MAX,exception);
  if (identity == MagickFalse)
    (void) FormatLocaleFile(file,"(%s) stringwidth pop -0.5 mul -0.5 rmoveto\n",
      text);
  (void) FormatLocaleFile(file,"(%s) show\n",text);
  text=DestroyString(text);
}

static Image *ReadPostscriptImage(const DrawInfo *draw_info,
  const PointInfo extent,const char *filename,ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  Image
    *annotate_image;

  ImageInfo
    *annotate_info;

  (void) FormatLocaleString(geometry,MagickPathExtent,"%.17gx%.17g+0+0!",
    floor(extent.x+0.5),floor(extent.y+0.5));
  annotate_info=AcquireImageInfo();
  (void) FormatLocaleString(annotate_info->filename,MagickPathExtent,"ps:%s",
    filename);
  (void) CloneString(&annotate_info->page,geometry);
  if (draw_info->density != (char *) NULL)
    (void) CloneString(&annotate_info->density,draw_info->density);
  annotate_info->antialias=draw_info->text_antialias;
  annotate_image=ReadImage(annotate_info,exception);
  CatchException(exception);
  annotate_info=DestroyImageInfo(annotate_info);
  (void) RelinquishUniqueFileResource(filename);
  return(annotate_image);
}

static MagickBooleanType IsIdentityAffine(const DrawInfo *draw_info)
{
  return((fabs(draw_info->affine.sx-draw_info->affine.sy) < MagickEpsilon) &&
    (fabs(draw_info->affine.rx) < MagickEpsilon) &&
    (fabs(draw_info->affine.ry) < MagickEpsilon) ? MagickTrue : MagickFalse);
}

static void SetPostscriptAlphaChannels(Image *image,Image *annotate_image,
  ExceptionInfo *exception)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
  if (annotate_image->alpha_trait == UndefinedPixelTrait)
    (void) SetImageAlphaChannel(annotate_image,OpaqueAlphaChannel,
      exception);
}

static MagickBooleanType RenderPostscript(Image *image,
  const DrawInfo *draw_info,const PointInfo *offset,TypeMetric *metrics,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    geometry[MagickPathExtent];

  FILE
    *file;

  Image
    *annotate_image;

  int
    unique_file;

  MagickBooleanType
    identity,
    status;

  PointInfo
    extent,
    resolution;

  ssize_t
    y;

  /*
    Render label with a Postscript font.
  */
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
      "Font %s; pointsize %g",draw_info->font != (char *) NULL ?
      draw_info->font : "none",draw_info->pointsize);
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"wb");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      ThrowFileException(exception,FileOpenError,"UnableToOpenFile",filename);
      return(MagickFalse);
    }
  WritePostscriptPrologue(file);
  /*
    Sample to compute bounding box.
  */
  identity=IsIdentityAffine(draw_info);
  extent=GetPostscriptExtent(draw_info);
  (void) FormatLocaleFile(file,"%g %g moveto\n",identity  != MagickFalse ? 0.0 :
    extent.x/2.0,extent.y/2.0);
  (void) FormatLocaleFile(file,"%g %g scale\n",draw_info->pointsize,
    draw_info->pointsize);
  WritePostscriptFont(file,draw_info);
  (void) FormatLocaleFile(file,"[%g %g %g %g 0 0] concat\n",
    draw_info->affine.sx,-draw_info->affine.rx,-draw_info->affine.ry,
    draw_info->affine.sy);
  WritePostscriptText(file,draw_info,identity,exception);
  (void) FormatLocaleFile(file,"showpage\n");
  (void) fclose(file);
  annotate_image=ReadPostscriptImage(draw_info,extent,filename,exception);
  if (annotate_image == (Image *) NULL)
    return(MagickFalse);
  (void) NegateImage(annotate_image,MagickFalse,exception);
  resolution=GetDensityResolution(draw_info);
  if (identity == MagickFalse)
    (void) TransformImage(&annotate_image,"0x0",(char *) NULL,exception);
  else
    {
      RectangleInfo
        crop_info;

      crop_info=GetImageBoundingBox(annotate_image,exception);
      crop_info.height=(size_t) ((resolution.y/DefaultResolution)*
        ExpandAffine(&draw_info->affine)*draw_info->pointsize+0.5);
      crop_info.y=CastDoubleToSsizeT(ceil((resolution.y/DefaultResolution)*
        extent.y/8.0-0.5));
      (void) FormatLocaleString(geometry,MagickPathExtent,
        "%.17gx%.17g%+.20g%+.20g",(double) crop_info.width,(double)
        crop_info.height,(double) crop_info.x,(double) crop_info.y);
      (void) TransformImage(&annotate_image,geometry,(char *) NULL,exception);
    }
  SetPostscriptMetrics(draw_info,annotate_image,resolution,metrics);
  if (draw_info->render == MagickFalse)
    {
      annotate_image=DestroyImage(annotate_image);
      return(MagickTrue);
    }
  if (draw_info->fill.alpha != (MagickRealType) TransparentAlpha)
    {
      CacheView
        *annotate_view;

      MagickBooleanType
        sync;

      PixelInfo
        fill_color;

      /*
        Render fill color.
      */
      SetPostscriptAlphaChannels(image,annotate_image,exception);
      fill_color=draw_info->fill;
      status=MagickTrue;
      annotate_view=AcquireAuthenticCacheView(annotate_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        magick_number_threads(annotate_image,annotate_image,annotate_image->rows,4)
#endif
      for (y=0; y < (ssize_t) annotate_image->rows; y++)
      {
        ssize_t
          x;

        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        q=GetCacheViewAuthenticPixels(annotate_view,0,y,annotate_image->columns,
          1,exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        for (x=0; x < (ssize_t) annotate_image->columns; x++)
        {
          GetFillColor(draw_info,x,y,&fill_color,exception);
          SetPixelAlpha(annotate_image,ClampToQuantum((((double) QuantumScale*
            GetPixelIntensity(annotate_image,q)*fill_color.alpha))),q);
          SetPixelRed(annotate_image,(const Quantum) fill_color.red,q);
          SetPixelGreen(annotate_image,(const Quantum) fill_color.green,q);
          SetPixelBlue(annotate_image,(const Quantum) fill_color.blue,q);
          q+=(ptrdiff_t) GetPixelChannels(annotate_image);
        }
        sync=SyncCacheViewAuthenticPixels(annotate_view,exception);
        if (sync == MagickFalse)
          status=MagickFalse;
      }
      annotate_view=DestroyCacheView(annotate_view);
      (void) CompositeImage(image,annotate_image,OverCompositeOp,MagickTrue,
        (ssize_t) ceil(offset->x-0.5),(ssize_t) ceil(offset->y-(metrics->ascent+
        metrics->descent)-0.5),exception);
    }
  annotate_image=DestroyImage(annotate_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e n d e r X 1 1                                                         %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderX11() renders text on the image with an X11 font.  It also returns the
%  bounding box of the text relative to the image.
%
%  The format of the RenderX11 method is:
%
%      MagickBooleanType RenderX11(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType RenderX11(Image *image,const DrawInfo *draw_info,
  const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
{
  const XRenderText
    text = { draw_info, offset, metrics };

  MagickBooleanType
    status;

  if (annotate_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&annotate_semaphore);
  LockSemaphoreInfo(annotate_semaphore);
  status=XRenderImage(image,&text,exception);
  UnlockSemaphoreInfo(annotate_semaphore);
  return(status);
}
