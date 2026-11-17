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

static const TypeInfo *GetDefaultTypeInfo(const DrawInfo *draw_info,
  const TypeInfo *type_info)
{
  ExceptionInfo
    *sans_exception;

  /*
    Search for a default font.
  */
  sans_exception=AcquireExceptionInfo();
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfoByFamily((const char *) NULL,draw_info->style,
      draw_info->stretch,draw_info->weight,sans_exception);
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfo("*",sans_exception);
  sans_exception=DestroyExceptionInfo(sans_exception);
  return(type_info);
}

static const TypeInfo *GetTypeInfoByFamilyList(const DrawInfo *draw_info,
  const TypeInfo *type_info,ExceptionInfo *exception)
{
  char
    **family;

  int
    number_families;

  ssize_t
    i;

  /*
    Parse font family list.
  */
  family=StringToArgv(draw_info->family,&number_families);
  for (i=1; i < (ssize_t) number_families; i++)
  {
    (void) SubstituteString(&family[i],",","");
    type_info=GetTypeInfoByFamily(family[i],draw_info->style,
      draw_info->stretch,draw_info->weight,exception);
    if ((type_info != (const TypeInfo *) NULL) &&
        (LocaleCompare(family[i],type_info->family) == 0))
      break;
  }
  for (i=0; i < (ssize_t) number_families; i++)
    family[i]=DestroyString(family[i]);
  family=(char **) RelinquishMagickMemory(family);
  if (type_info == (const TypeInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),TypeWarning,
      "UnableToReadFont","`%s'",draw_info->family);
  return(type_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
+   R e n d e r T y p e                                                       %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderType() renders text on the image.  It also returns the bounding box of
%  the text relative to the image.
%
%  The format of the RenderType method is:
%
%      MagickBooleanType RenderType(const RenderTarget *target,
%        const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o target: the image and the draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static const TypeInfo *GetFamilyTypeInfo(const DrawInfo *draw_info,
  const TypeInfo *type_info,ExceptionInfo *exception)
{
  if (strpbrk(draw_info->family,",'\"") == (char *) NULL)
    type_info=GetTypeInfoByFamily(draw_info->family,draw_info->style,
      draw_info->stretch,draw_info->weight,exception);
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfoByFamilyList(draw_info,type_info,exception);
  return(type_info);
}

static void SetTypeInfoFace(DrawInfo *annotate_info,
  const TypeInfo *type_info)
{
  annotate_info->face=type_info->face;
  if (type_info->metrics != (char *) NULL)
    (void) CloneString(&annotate_info->metrics,type_info->metrics);
  if (type_info->glyphs != (char *) NULL)
    (void) CloneString(&annotate_info->font,type_info->glyphs);
}

static MagickBooleanType RenderTargetFreetype(const RenderTarget *target,
  const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
{
  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  MagickBooleanType
    status;

  const FreetypeTarget ft_font = { image, draw_info, draw_info->encoding };
  status=RenderFreetype(&ft_font,offset,metrics,exception);
  return(status);
}

static MagickBooleanType RenderType(const RenderTarget *target,
  const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
{
  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  char
    *font;

  const TypeInfo
    *type_info;

  DrawInfo
    *annotate_info;

  MagickBooleanType
    status;

  FreetypeTarget freetype;

  type_info=(const TypeInfo *) NULL;
  if (draw_info->font != (char *) NULL)
    {
      if (*draw_info->font == '@')
        return(RenderTargetFreetype(target,offset,metrics,exception));
      if (*draw_info->font == '-')
        return(RenderX11(image,draw_info,offset,metrics,exception));
      if (*draw_info->font == '^')
        return(RenderPostscript(image,draw_info,offset,metrics,exception));
      if (IsPathAccessible(draw_info->font) != MagickFalse)
        return(RenderTargetFreetype(target,offset,metrics,exception));
      type_info=GetTypeInfo(draw_info->font,exception);
      if (type_info == (const TypeInfo *) NULL)
        (void) ThrowMagickException(exception,GetMagickModule(),TypeWarning,
          "UnableToReadFont","`%s'",draw_info->font);
    }
  if ((type_info == (const TypeInfo *) NULL) &&
      (draw_info->family != (const char *) NULL))
    type_info=GetFamilyTypeInfo(draw_info,type_info,exception);
  font=GetPolicyValue("system:font");
  if (font != (char *) NULL)
    {
      if (IsPathAccessible(font) != MagickFalse)
        {
          /*
            Render with default system font.
          */
          annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
          annotate_info->font=font;
          freetype.image=image;
          freetype.draw_info=annotate_info;
          freetype.encoding=annotate_info->encoding;
          status=RenderFreetype(&freetype,offset,metrics,exception);
          annotate_info=DestroyDrawInfo(annotate_info);
          return(status);
        }
      font=DestroyString(font);
    }
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetDefaultTypeInfo(draw_info,type_info);
  if (type_info == (const TypeInfo *) NULL)
    return(RenderTargetFreetype(target,offset,metrics,exception));
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  SetTypeInfoFace(annotate_info,type_info);
  freetype.image=image;
  freetype.draw_info=annotate_info;
  freetype.encoding=type_info->encoding;
  status=RenderFreetype(&freetype,offset,metrics,exception);
  annotate_info=DestroyDrawInfo(annotate_info);
  return(status);
}

static PointInfo ParseDensityResolution(const DrawInfo *draw_info,
  PointInfo resolution)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    geometry_flags;

  geometry_flags=ParseGeometry(draw_info->density,&geometry_info);
  if ((geometry_flags & RhoValue) != 0)
    resolution.x=geometry_info.rho;
  resolution.y=resolution.x;
  if ((geometry_flags & SigmaValue) != 0)
    resolution.y=geometry_info.sigma;
  return(resolution);
}

static PointInfo GetDensityResolution(const DrawInfo *draw_info)
{
  PointInfo
    resolution;

  resolution.x=DefaultResolution;
  resolution.y=DefaultResolution;
  if (draw_info->density != (char *) NULL)
    resolution=ParseDensityResolution(draw_info,resolution);
  return(resolution);
}

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

#if defined(MAGICKCORE_RAQM_DELEGATE)
static MagickBooleanType ContainsRaqmParagraphSeparator(const unsigned char *p,
  const size_t length)
{
  size_t 
    i;

  if ((p == (const unsigned char*) NULL) || (length == 0))
    return(MagickFalse);

  for (i = 0; i < length; i++)
  {
    /* LF or CR */
    if ((p[i] == '\n') || (p[i] == '\r'))
      return(MagickTrue);

    /* UTF-8 NEL U+0085 => C2 85 */
    if ((p[i] == 0xC2) && (i + 1 < length) && (p[i + 1] == 0x85))
      return(MagickTrue);

    /* UTF-8 LS/PS U+2028/U+2029 => E2 80 A8/A9 */
    if ((p[i] == 0xE2) && (i + 2 < length) && (p[i + 1] == 0x80) &&
        ((p[i + 2] == 0xA8) || (p[i + 2] == 0xA9)))
      return(MagickTrue);
  }

  return(MagickFalse);
}

static size_t ComplexRaqmTextLayout(const Image *image,
  const DrawInfo *draw_info,const char *text,const size_t length,
  const FT_Face face,GraphemeInfo **grapheme,ExceptionInfo *exception)
{
  const char
    *features;

  raqm_t
    *rq;

  raqm_glyph_t
    *glyphs;

  size_t
    extent;

  ssize_t
    i;

  extent=0;
  rq=raqm_create();
  if (rq == (raqm_t *) NULL)
    goto cleanup;
  if (raqm_set_text_utf8(rq,text,length) == 0)
    goto cleanup;
  if (raqm_set_par_direction(rq,(raqm_direction_t) draw_info->direction) == 0)
    goto cleanup;
  if (raqm_set_freetype_face(rq,face) == 0)
    goto cleanup;
  features=GetImageProperty(image,"type:features",exception);
  if (features != (const char *) NULL)
    {
      char
        breaker,
        quote,
        *token;

      int
        next,
        status_token;

      TokenInfo
        *token_info;

      next=0;
      token_info=AcquireTokenInfo();
      token=AcquireString("");
      status_token=Tokenizer(token_info,0,token,50,features,"",",","",'\0',
        &breaker,&next,&quote);
      while (status_token == 0)
      {
        raqm_add_font_feature(rq,token,(int) strlen(token));
        status_token=Tokenizer(token_info,0,token,50,features,"",",","",'\0',
          &breaker,&next,&quote);
      }
      token_info=DestroyTokenInfo(token_info);
      token=DestroyString(token);
    }
  if (raqm_layout(rq) == 0)
    goto cleanup;
  glyphs=raqm_get_glyphs(rq,&extent);
  if (glyphs == (raqm_glyph_t *) NULL)
    {
      extent=0;
      goto cleanup;
    }
  *grapheme=(GraphemeInfo *) AcquireQuantumMemory(extent,sizeof(**grapheme));
  if (*grapheme == (GraphemeInfo *) NULL)
    {
      extent=0;
      goto cleanup;
    }
  for (i=0; i < (ssize_t) extent; i++)
  {
    (*grapheme)[i].index=glyphs[i].index;
    (*grapheme)[i].x_offset=glyphs[i].x_offset;
    (*grapheme)[i].x_advance=glyphs[i].x_advance;
    (*grapheme)[i].y_offset=glyphs[i].y_offset;
    (*grapheme)[i].y_advance=glyphs[i].y_advance;
    (*grapheme)[i].cluster=glyphs[i].cluster;
  }

cleanup:
  raqm_destroy(rq);
  return(extent);
}
#endif

static inline MagickBooleanType IsKernedGrapheme(const ssize_t index,
  const ssize_t last_glyph)
{
  return(((index != 0) && (last_glyph != 0)) ? MagickTrue : MagickFalse);
}

static size_t ComplexTextLayout(const DrawInfo *draw_info,const char *text,
  const size_t length,const FT_Face face,const FT_Int32 flags,
  GraphemeInfo **grapheme)
{
  const char
    *p;

  ssize_t
    i;

  ssize_t
    last_glyph;

  /*
    Simple layout for bi-directional text (right-to-left or left-to-right).
  */
  *grapheme=(GraphemeInfo *) AcquireQuantumMemory(length+1,sizeof(**grapheme));
  if (*grapheme == (GraphemeInfo *) NULL)
    return(0);
  last_glyph=0;
  p=text;
  for (i=0; GetUTFCode(p) != 0; p+=(ptrdiff_t) GetUTFOctets(p), i++)
  {
    (*grapheme)[i].index=(ssize_t) FT_Get_Char_Index(face,(FT_ULong)
      GetUTFCode(p));
    (*grapheme)[i].x_offset=0;
    (*grapheme)[i].y_offset=0;
    if ((IsKernedGrapheme((*grapheme)[i].index,last_glyph) != MagickFalse) &&
        (FT_HAS_KERNING(face)))
      {
        FT_Error
          ft_status;

        FT_Vector
          kerning;

        ft_status=FT_Get_Kerning(face,(FT_UInt) last_glyph,(FT_UInt)
          (*grapheme)[i].index,ft_kerning_default,&kerning);
        if (ft_status == 0)
          (*grapheme)[i-1].x_advance+=(FT_Pos) ((draw_info->direction ==
            RightToLeftDirection ? -1.0 : 1.0)*kerning.x);
      }
    (void) FT_Load_Glyph(face,(FT_UInt) (*grapheme)[i].index,flags);
    (*grapheme)[i].x_advance=face->glyph->advance.x;
    (*grapheme)[i].y_advance=face->glyph->advance.y;
    (*grapheme)[i].cluster=(size_t) (p-text);
    last_glyph=(*grapheme)[i].index;
  }
  return((size_t) i);
}

static void FreetypeCloseStream(FT_Stream stream)
{
  FILE
    *file;

  file=(FILE *) stream->descriptor.pointer;
  if (file != (FILE *) NULL)
    (void) fclose(file);
  stream->descriptor.pointer=NULL;
}

static unsigned long FreetypeReadStream(FT_Stream stream,unsigned long offset,
  unsigned char *buffer,unsigned long count)
{
  FILE
    *file;

  unsigned long
    result;

  file=(FILE *) stream->descriptor.pointer;
  if (file == (FILE *) NULL)
    return(0);
  if (offset > stream->size)
    result=1;
  else
    result=(unsigned long) fseek(file,(off_t) offset,SEEK_SET);
  if (count == 0) /* seek operation */
    return(result);
  if (result != 0)
    return(0);
  return((unsigned long) fread(buffer,1,count,file));
}

static inline MagickBooleanType IsEmptyOutline(FT_Outline outline)
{
  return((outline.n_points == 0) || (outline.n_contours <= 0) ? MagickTrue :
    MagickFalse);
}

static int TraceCubicBezier(FT_Vector *p,FT_Vector *q,FT_Vector *to,
  DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MagickPathExtent];

  affine=draw_info->affine;
  (void) FormatLocaleString(path,MagickPathExtent,"C%g,%g %g,%g %g,%g",
    affine.tx+p->x/64.0,affine.ty-p->y/64.0,affine.tx+q->x/64.0,affine.ty-
    q->y/64.0,affine.tx+to->x/64.0,affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceLineTo(FT_Vector *to,DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MagickPathExtent];

  affine=draw_info->affine;
  (void) FormatLocaleString(path,MagickPathExtent,"L%g,%g",affine.tx+to->x/64.0,
    affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceMoveTo(FT_Vector *to,DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MagickPathExtent];

  affine=draw_info->affine;
  (void) FormatLocaleString(path,MagickPathExtent,"M%g,%g",affine.tx+to->x/64.0,
    affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceQuadraticBezier(FT_Vector *control,FT_Vector *to,
  DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MagickPathExtent];

  affine=draw_info->affine;
  (void) FormatLocaleString(path,MagickPathExtent,"Q%g,%g %g,%g",affine.tx+
    control->x/64.0,affine.ty-control->y/64.0,affine.tx+to->x/64.0,affine.ty-
    to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR >= 10
static inline const char *FreetypeErrorMessage(FT_Error ft_status)
{
  return(FT_Error_String(ft_status));
#else
static inline const char *FreetypeErrorMessage(
  FT_Error magick_unused(ft_status))
{
  magick_unreferenced(ft_status);
  return((const char *) NULL);
#endif
}

static void *FreetypeAlloc(FT_Memory magick_unused(memory),long extent)
{
  magick_unreferenced(memory);
  if (extent <= 0)
    return((void *) NULL);
  return(AcquireQuantumMemory((size_t) extent,1));
}

static void *FreetypeRealloc(FT_Memory magick_unused(memory),long length,
  long extent,void *block)
{
  void
    *cloned_block,
    *new_block;

  magick_unreferenced(memory);
  if (extent <= 0)
    return(RelinquishMagickMemory(block));
  if (block == (void *) NULL)
    return(AcquireQuantumMemory((size_t) extent,1));
  cloned_block=AcquireQuantumMemory((size_t) length,1);
  if (cloned_block == (void *) NULL)
    return((void *) NULL);
  (void) memcpy(cloned_block,block,(size_t) length);
  new_block=ResizeQuantumMemory(cloned_block,(size_t) extent,1);
  if (new_block == (void *) NULL)
    return((void *) NULL);
  block=RelinquishMagickMemory(block);
  return(new_block);
}

static void FreetypeFree(FT_Memory magick_unused(memory),void *block)
{
  magick_unreferenced(memory);
  (void) RelinquishMagickMemory(block);
}

static FT_Memory FreetypeAcquireMemoryManager(void)
{
  FT_Memory
    memory;

  memory=(FT_Memory) AcquireCriticalMemory(sizeof(*memory));
  memset(memory,0,sizeof(*memory));
  memory->alloc=(&FreetypeAlloc);
  memory->realloc=(&FreetypeRealloc);
  memory->free=(&FreetypeFree);
  return(memory);
}

static void FreetypeDone(FT_Memory memory,FT_Library library,
  FT_StreamRec *stream)
{
  (void) FT_Done_Library(library);
  stream=(FT_StreamRec *) RelinquishMagickMemory(stream);
  memory=(FT_Memory) RelinquishMagickMemory(memory);
}

static FT_Error FreetypeInit(FT_Memory memory,FT_Library *alibrary)
{
  FT_Error
    ft_status;

  ft_status=FT_New_Library(memory,alibrary);
  if (ft_status != 0)
    RelinquishMagickMemory(memory);
  else
    {
      FT_Add_Default_Modules(*alibrary);
      FT_Set_Default_Properties(*alibrary);
    }
  return(ft_status);
}

static inline MagickBooleanType IsUnauthorizedFontFile(
  const DrawInfo *draw_info)
{
  return(((draw_info->font != (char *) NULL) && (*draw_info->font == '@') &&
    (IsRightsAuthorized(PathPolicyDomain,ReadPolicyRights,draw_info->font) ==
    MagickFalse)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsTextWithoutGlyphs(const DrawInfo *draw_info,
  const FT_UInt first_glyph_id)
{
  return(((draw_info->text == (char *) NULL) || (*draw_info->text == '\0') ||
    (first_glyph_id == 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsRenderedStroke(const DrawInfo *draw_info,
  const MagickBooleanType status)
{
  return((((draw_info->stroke.alpha != (MagickRealType) TransparentAlpha) ||
    (draw_info->stroke_pattern != (Image *) NULL)) &&
    ((status != MagickFalse) &&
    (draw_info->render != MagickFalse))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsVisibleStroke(const DrawInfo *draw_info,
  const MagickBooleanType status)
{
  return((((draw_info->stroke.alpha != (MagickRealType) TransparentAlpha) ||
    (draw_info->stroke_pattern != (Image *) NULL)) &&
    (status != MagickFalse)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsInterwordSpace(const DrawInfo *draw_info,
  const char *text,const ssize_t code)
{
  return(((fabs(draw_info->interword_spacing) >= MagickEpsilon) &&
    (IsUTFSpace(GetUTFCode(text)) != MagickFalse) &&
    (IsUTFSpace((int) code) == MagickFalse)) ? MagickTrue : MagickFalse);
}

static void SetFreetypeMetrics(const FT_Face face,TypeMetric *metrics)
{
  metrics->pixels_per_em.x=face->size->metrics.x_ppem;
  metrics->pixels_per_em.y=face->size->metrics.y_ppem;
  metrics->ascent=(double) face->size->metrics.ascender/64.0;
  metrics->descent=(double) face->size->metrics.descender/64.0;
  if (face->size->metrics.ascender == 0)
    {
      /*
        Sanitize buggy ascender and descender values.
      */
      metrics->ascent=face->size->metrics.y_ppem;
      if (face->size->metrics.descender == 0)
        metrics->descent=face->size->metrics.y_ppem/-3.5;
    }
  metrics->width=0;
  metrics->origin.x=0;
  metrics->origin.y=0;
  metrics->height=(double) face->size->metrics.height/64.0;
  metrics->max_advance=0.0;
  if (face->size->metrics.max_advance > MagickEpsilon)
    metrics->max_advance=(double) face->size->metrics.max_advance/64.0;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=face->underline_position*
    (metrics->pixels_per_em.x*MagickSafeReciprocal(face->units_per_EM));
  metrics->underline_thickness=face->underline_thickness*
    (metrics->pixels_per_em.x*MagickSafeReciprocal(face->units_per_EM));
}

static void SetFreetypeCanvas(Image *image,ExceptionInfo *exception)
{
  if (image->storage_class != DirectClass)
    (void) SetImageStorageClass(image,DirectClass,exception);
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    (void) SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
}

static void DrawFreetypeStroke(Image *image,DrawInfo *annotate_info,
  const PointInfo *offset,ExceptionInfo *exception)
{
  /*
    Draw text stroke.
  */
  annotate_info->linejoin=RoundJoin;
  annotate_info->affine.tx=offset->x;
  annotate_info->affine.ty=offset->y;
  (void) ConcatenateString(&annotate_info->primitive,"'");
  if (strlen(annotate_info->primitive) > 7)
    (void) DrawImage(image,annotate_info,exception);
  (void) CloneString(&annotate_info->primitive,"path '");
}

typedef struct _GlyphRasterInfo
{
  FT_BitmapGlyph
    bitmap;

  unsigned char
    *r;
} GlyphRasterInfo;

static double GetGlyphFillOpacity(const GlyphRasterInfo *raster,
  const ssize_t n,const ssize_t x,const ssize_t y)
{
  const FT_BitmapGlyph
    bitmap = raster->bitmap;

  unsigned char
    *const r = raster->r;

  double
    fill_opacity;

  fill_opacity=1.0;
  if (bitmap->bitmap.buffer != (unsigned char *) NULL)
    {
      if (bitmap->bitmap.pixel_mode == ft_pixel_mode_grays)
        fill_opacity=(double) (r[n])/(bitmap->bitmap.num_grays-1);
      else
        if (bitmap->bitmap.pixel_mode == ft_pixel_mode_mono)
          fill_opacity=((r[(x >> 3)+y*bitmap->bitmap.pitch] &
            (1 << (~x & 0x07)))) == 0 ? 0.0 : 1.0;
    }
  return(fill_opacity);
}

static Quantum *SkipGlyphPixel(const Image *image,Quantum *q)
{
  if (q != (Quantum *) NULL)
    q+=(ptrdiff_t) GetPixelChannels(image);
  return(q);
}

static MagickBooleanType SyncGlyphPixel(const MagickBooleanType active,
  CacheView *image_view,ExceptionInfo *exception)
{
  MagickBooleanType
    status,
    sync;

  status=MagickTrue;
  if (active == MagickFalse)
    {
      sync=SyncCacheViewAuthenticPixels(image_view,exception);
      if (sync == MagickFalse)
        status=MagickFalse;
    }
  return(status);
}

static char *FindInvalidUTFCode(const DrawInfo *draw_info)
{
  char
    *p;

  for (p=draw_info->text; GetUTFCode(p) != 0; p+=(ptrdiff_t) GetUTFOctets(p))
    if (GetUTFCode(p) < 0)
      break;
  return(p);
}

static void SelectFreetypeText(const DrawInfo *draw_info,
  unsigned char **utf8,char **p)
{
  *utf8=(unsigned char *) NULL;
  if (GetUTFCode(*p) == 0)
    *p=draw_info->text;
  else
    {
      *utf8=ConvertLatin1ToUTF8((unsigned char *) draw_info->text);
      if (*utf8 != (unsigned char *) NULL)
        *p=(char *) *utf8;
    }
}

static FT_Matrix GetFreetypeAffine(const DrawInfo *draw_info)
{
  FT_Matrix
    affine;

  affine.xx=65536L;
  affine.yx=0L;
  affine.xy=0L;
  affine.yy=65536L;
  if (draw_info->render != MagickFalse)
    {
      affine.xx=(FT_Fixed) (65536L*draw_info->affine.sx+0.5);
      affine.yx=(FT_Fixed) (-65536L*draw_info->affine.rx+0.5);
      affine.xy=(FT_Fixed) (-65536L*draw_info->affine.ry+0.5);
      affine.yy=(FT_Fixed) (65536L*draw_info->affine.sy+0.5);
    }
  return(affine);
}

static void RelinquishFreetypeText(GraphemeInfo *grapheme,unsigned char *utf8)
{
  if (grapheme != (GraphemeInfo *) NULL)
    grapheme=(GraphemeInfo *) RelinquishMagickMemory(grapheme);
  if (utf8 != (unsigned char *) NULL)
    utf8=(unsigned char *) RelinquishMagickMemory(utf8);
}

static FT_Int32 GetFreetypeHintingFlags(Image *image,FT_Int32 flags,
  ExceptionInfo *exception)
{
  const char
    *value;

  value=GetImageProperty(image,"type:hinting",exception);
  if ((value != (const char *) NULL) && (LocaleCompare(value,"off") == 0))
    flags|=FT_LOAD_NO_HINTING;
  return(flags);
}

static inline MagickBooleanType IsTransparentFill(const DrawInfo *draw_info)
{
  return(((draw_info->fill.alpha == (MagickRealType) TransparentAlpha) &&
    (draw_info->fill_pattern == (Image *) NULL) &&
    (draw_info->stroke.alpha == (MagickRealType) TransparentAlpha) &&
    (draw_info->stroke_pattern == (Image *) NULL)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsOffsetOutside(const ssize_t offset,
  const size_t extent)
{
  return(((offset < 0) || (offset >= (ssize_t) extent)) ?
    MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNonOutlineGlyph(const FT_Glyph glyph_image,
  FT_Outline outline)
{
  return(((glyph_image->format != FT_GLYPH_FORMAT_OUTLINE) &&
    (IsEmptyOutline(outline) == MagickFalse)) ? MagickTrue : MagickFalse);
}

static void ExtendGlyphMinBounds(const FT_BBox bounds,TypeMetric *metrics)
{
  if ((bounds.xMin < metrics->bounds.x1) && (bounds.xMin != 0))
    metrics->bounds.x1=(double) bounds.xMin;
  if ((bounds.yMin < metrics->bounds.y1) && (bounds.yMin != 0))
    metrics->bounds.y1=(double) bounds.yMin;
}

static void ExtendGlyphMaxBounds(const FT_BBox bounds,TypeMetric *metrics)
{
  if ((bounds.xMax > metrics->bounds.x2) && (bounds.xMax != 0))
    metrics->bounds.x2=(double) bounds.xMax;
  if ((bounds.yMax > metrics->bounds.y2) && (bounds.yMax != 0))
    metrics->bounds.y2=(double) bounds.yMax;
}

static FT_Encoding SelectAdobeBig5Encoding(const char *encoding,
  FT_Encoding encoding_type)
{
  if (LocaleCompare(encoding,"AdobeCustom") == 0)
    encoding_type=FT_ENCODING_ADOBE_CUSTOM;
  if (LocaleCompare(encoding,"AdobeExpert") == 0)
    encoding_type=FT_ENCODING_ADOBE_EXPERT;
  if (LocaleCompare(encoding,"AdobeStandard") == 0)
    encoding_type=FT_ENCODING_ADOBE_STANDARD;
  if (LocaleCompare(encoding,"AppleRoman") == 0)
    encoding_type=FT_ENCODING_APPLE_ROMAN;
  if (LocaleCompare(encoding,"BIG5") == 0)
    encoding_type=FT_ENCODING_BIG5;
  return(encoding_type);
}

static FT_Encoding SelectCjkLatinEncoding(const char *encoding,
  FT_Encoding encoding_type)
{
  (void) encoding;
#if defined(FT_ENCODING_PRC)
  if (LocaleCompare(encoding,"GB2312") == 0)
    encoding_type=FT_ENCODING_PRC;
#endif
#if defined(FT_ENCODING_JOHAB)
  if (LocaleCompare(encoding,"Johab") == 0)
    encoding_type=FT_ENCODING_JOHAB;
#endif
#if defined(FT_ENCODING_ADOBE_LATIN_1)
  if (LocaleCompare(encoding,"Latin-1") == 0)
    encoding_type=FT_ENCODING_ADOBE_LATIN_1;
#endif
#if defined(FT_ENCODING_ADOBE_LATIN_2)
  if (LocaleCompare(encoding,"Latin-2") == 0)
    encoding_type=FT_ENCODING_OLD_LATIN_2;
#endif
  return(encoding_type);
}

static void BlendGlyphAlpha(const Image *image,double fill_opacity,
  Quantum *q)
{
  double
    Sa,
    Da;

  Da=1.0-(QuantumScale*(double) GetPixelAlpha(image,q));
  Sa=fill_opacity;
  fill_opacity=(1.0-RoundToUnity(Sa+Da-Sa*Da))*(double) QuantumRange;
  SetPixelAlpha(image,(const Quantum) fill_opacity,q);
}

static void ScaleFreetypeMetrics(TypeMetric *metrics)
{
  /*
    Determine font metrics.
  */
  metrics->bounds.x1/=64.0;
  metrics->bounds.y1/=64.0;
  metrics->bounds.x2/=64.0;
  metrics->bounds.y2/=64.0;
  metrics->origin.x/=64.0;
  metrics->origin.y/=64.0;
  metrics->width=ceil(metrics->width/64.0);
}

static void SetFreetypeMetricsOrigin(TypeMetric *metrics,
  const FT_Vector origin)
{
  metrics->origin.x=(double) origin.x;
  metrics->origin.y=(double) origin.y;
  if (metrics->origin.x > metrics->width)
    metrics->width=metrics->origin.x;
}

static FT_StreamRec *AcquireFreetypeStream(const char *pathname)
{
  FT_StreamRec
    *stream;

  struct stat
    attributes;

  stream=(FT_StreamRec *) AcquireCriticalMemory(sizeof(*stream));
  (void) memset(stream,0,sizeof(*stream));
  if (stat(pathname,&attributes) == 0)
    stream->size=attributes.st_size >= 0 ? (unsigned long)
      attributes.st_size : 0;
  stream->descriptor.pointer=fopen_utf8(pathname,"rb");
  stream->read=(&FreetypeReadStream);
  stream->close=(&FreetypeCloseStream);
  return(stream);
}

static PointInfo GetGlyphBitmapPoint(const PointInfo *offset,
  const FT_BitmapGlyph bitmap,const FT_Vector origin)
{
  PointInfo
    point;

  point.x=offset->x+bitmap->left;
  if (bitmap->bitmap.pixel_mode == ft_pixel_mode_mono)
    point.x+=(origin.x/64.0);
  point.y=offset->y-bitmap->top;
  return(point);
}

static FT_Glyph DestroyFreetypeGlyph(FT_Glyph glyph_image)
{
  if (glyph_image != (FT_Glyph) NULL)
    {
      FT_Done_Glyph(glyph_image);
      glyph_image=(FT_Glyph) NULL;
    }
  return(glyph_image);
}

static void TraceGlyphOutline(DrawInfo *annotate_info,const FT_Vector origin,
  FT_Outline outline,const FT_Outline_Funcs *outline_methods)
{
  /*
    Trace the glyph.
  */
  annotate_info->affine.tx=origin.x/64.0;
  annotate_info->affine.ty=(-origin.y/64.0);
  if (IsEmptyOutline(outline) == MagickFalse)
    (void) FT_Outline_Decompose(&outline,outline_methods,annotate_info);
}

typedef struct _GlyphRows
{
  Image
    *image;

  CacheView
    *image_view;

  FT_BitmapGlyph
    bitmap;

  PointInfo
    point;

  unsigned char
    *r;

  MagickBooleanType
    transparent_fill;

  const DrawInfo
    *draw_info;

  ExceptionInfo
    *exception;
} GlyphRows;

static Quantum *GetGlyphRowPixels(const GlyphRows *rows,
  const ssize_t x_offset,const ssize_t y_offset,MagickBooleanType *active)
{
  Image
    *const image = rows->image;

  CacheView
    *const image_view = rows->image_view;

  const FT_BitmapGlyph
    bitmap = rows->bitmap;

  ExceptionInfo
    *const exception = rows->exception;

  Quantum
    *magick_restrict q;

  q=(Quantum *) NULL;
  if (IsOffsetOutside(x_offset,image->columns) != MagickFalse)
    *active=MagickFalse;
  else
    {
      q=GetCacheViewAuthenticPixels(image_view,x_offset,y_offset,
        bitmap->bitmap.width,1,exception);
      *active=q != (Quantum *) NULL ? MagickTrue : MagickFalse;
    }
  return(q);
}

static double GetGlyphPixelOpacity(const GlyphRows *rows,const ssize_t n,
  const ssize_t x,const ssize_t y)
{
  const FT_BitmapGlyph
    bitmap = rows->bitmap;

  unsigned char
    *const r = rows->r;

  const DrawInfo
    *const draw_info = rows->draw_info;

  double
    fill_opacity;

  GlyphRasterInfo
    raster;

  raster.bitmap=bitmap;
  raster.r=r;
  fill_opacity=GetGlyphFillOpacity(&raster,n,x,y);
  if (draw_info->text_antialias == MagickFalse)
    fill_opacity=fill_opacity >= 0.5 ? 1.0 : 0.0;
  return(fill_opacity);
}

static void RasterizeGlyphRow(const GlyphRows *rows,const ssize_t y,
  MagickBooleanType *status)
{
  Image
    *const image = rows->image;

  CacheView
    *const image_view = rows->image_view;

  const FT_BitmapGlyph
    bitmap = rows->bitmap;

  const PointInfo
    point = rows->point;

  const MagickBooleanType
    transparent_fill = rows->transparent_fill;

  const DrawInfo
    *const draw_info = rows->draw_info;

  ExceptionInfo
    *const exception = rows->exception;

  double
    fill_opacity;

  MagickBooleanType
    active,
    sync;

  PixelInfo
    fill_color;

  Quantum
    *magick_restrict q;

  ssize_t
    x;

  ssize_t
    n,
    x_offset,
    y_offset;

  if ((*status) == MagickFalse)
    return;
  x_offset=CastDoubleToSsizeT(ceil(point.x-0.5));
  y_offset=CastDoubleToSsizeT(ceil(point.y+y-0.5));
  if (IsOffsetOutside(y_offset,image->rows) != MagickFalse)
    return;
  q=GetGlyphRowPixels(rows,x_offset,y_offset,&active);
  n=y*bitmap->bitmap.pitch;
  for (x=0; x < (ssize_t) bitmap->bitmap.width; x++, n++)
  {
    x_offset=CastDoubleToSsizeT(ceil(point.x+x-0.5));
    if (IsOffsetOutside(x_offset,image->columns) != MagickFalse)
      {
        q=SkipGlyphPixel(image,q);
        continue;
      }
    fill_opacity=GetGlyphPixelOpacity(rows,n,x,y);
    if (active == MagickFalse)
      q=GetCacheViewAuthenticPixels(image_view,x_offset,y_offset,1,1,
        exception);
    if (q == (Quantum *) NULL)
      continue;
    if (transparent_fill == MagickFalse)
      {
        GetPixelInfo(image,&fill_color);
        GetFillColor(draw_info,x_offset,y_offset,&fill_color,exception);
        fill_opacity=fill_opacity*fill_color.alpha;
        CompositePixelOver(image,&fill_color,fill_opacity,q,
          GetPixelAlpha(image,q),q);
      }
    else
      BlendGlyphAlpha(image,fill_opacity,q);
    if (SyncGlyphPixel(active,image_view,exception) == MagickFalse)
      (*status)=MagickFalse;
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  sync=SyncCacheViewAuthenticPixels(image_view,exception);
  if (sync == MagickFalse)
    (*status)=MagickFalse;
}

static void SelectFreetypeCharmap(FT_Face face,FT_Encoding encoding_type)
{
  FT_Error
    ft_status;

  ft_status=FT_Select_Charmap(face,encoding_type);
  if ((ft_status != 0) && (face->num_charmaps != 0))
    (void) FT_Set_Charmap(face,face->charmaps[0]);
}

typedef struct _GlyphBitmapTarget
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  FT_BitmapGlyph
    bitmap;

  PointInfo
    point;

  DrawInfo
    *annotate_info;

  const PointInfo
    *offset;

  ExceptionInfo
    *exception;
} GlyphBitmapTarget;

static MagickBooleanType RasterizeGlyphBitmap(const GlyphBitmapTarget *target,
  MagickBooleanType status)
{
  Image
    *const image = target->image;

  const DrawInfo
    *const draw_info = target->draw_info;

  const FT_BitmapGlyph
    bitmap = target->bitmap;

  const PointInfo
    point = target->point;

  DrawInfo
    *const annotate_info = target->annotate_info;

  const PointInfo
    *const offset = target->offset;

  ExceptionInfo
    *const exception = target->exception;

  ssize_t
    y;

  CacheView
    *image_view;

  MagickBooleanType
    transparent_fill;

  unsigned char
    *r;

  /*
    Rasterize the glyph.
  */
  transparent_fill=IsTransparentFill(draw_info);
  r=bitmap->bitmap.buffer;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,bitmap->bitmap.rows,4)
#endif
  for (y=0; y < (ssize_t) bitmap->bitmap.rows; y++)
  {
    const GlyphRows rows = { image, image_view, bitmap, point, r,
      transparent_fill, draw_info, exception };

    RasterizeGlyphRow(&rows,y,&status);
  }
  image_view=DestroyCacheView(image_view);
  if (IsVisibleStroke(draw_info,status) != MagickFalse)
    DrawFreetypeStroke(image,annotate_info,offset,exception);
  return(status);
}

static FT_Int32 GetFreetypeLoadFlags(const DrawInfo *draw_info)
{
  FT_Int32
    flags;

  flags=FT_LOAD_DEFAULT;
  if (draw_info->render == MagickFalse)
    flags=FT_LOAD_NO_BITMAP;
  if (draw_info->text_antialias == MagickFalse)
    flags|=FT_LOAD_TARGET_MONO;
  else
    {
#if defined(FT_LOAD_TARGET_LIGHT)
      flags|=FT_LOAD_TARGET_LIGHT;
#elif defined(FT_LOAD_TARGET_LCD)
      flags|=FT_LOAD_TARGET_LCD;
#endif
    }
  return(flags);
}

static DrawInfo *CloneFreetypeAnnotateInfo(const DrawInfo *draw_info)
{
  DrawInfo
    *annotate_info;

  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  if (annotate_info->dash_pattern != (double *) NULL)
    annotate_info->dash_pattern[0]=0.0;
  (void) CloneString(&annotate_info->primitive,"path '");
  return(annotate_info);
}

static FT_Encoding SelectFreetypeEncoding(const char *encoding,
  FT_Encoding encoding_type)
{
  encoding_type=SelectAdobeBig5Encoding(encoding,encoding_type);
  encoding_type=SelectCjkLatinEncoding(encoding,encoding_type);
  if (LocaleCompare(encoding,"None") == 0)
    encoding_type=FT_ENCODING_NONE;
  if (LocaleCompare(encoding,"SJIScode") == 0)
    encoding_type=FT_ENCODING_SJIS;
  if (LocaleCompare(encoding,"Symbol") == 0)
    encoding_type=FT_ENCODING_MS_SYMBOL;
  if (LocaleCompare(encoding,"Unicode") == 0)
    encoding_type=FT_ENCODING_UNICODE;
  if (LocaleCompare(encoding,"Wansung") == 0)
    encoding_type=FT_ENCODING_WANSUNG;
  return(encoding_type);
}

static void SelectFreetypeFontPath(const DrawInfo *draw_info,
  FT_Open_Args *args,FT_Long *face_index,ExceptionInfo *exception)
{
  if (draw_info->font == (char *) NULL)
    {
      const TypeInfo *type_info = GetTypeInfo("*",exception);
      if (type_info != (const TypeInfo *) NULL)
        args->pathname=ConstantString(type_info->glyphs);
    }
  else
    if (*draw_info->font != '@')
      args->pathname=ConstantString(draw_info->font);
    else
      {
        /*
          Extract face index, e.g. @msgothic[1].
        */
        ImageInfo *image_info = AcquireImageInfo();
        (void) CopyMagickString(image_info->filename,draw_info->font+1,
          MagickPathExtent);
        (void) SetImageInfo(image_info,0,exception);
        *face_index=(FT_Long) image_info->scene;
        args->pathname=ConstantString(image_info->filename);
        image_info=DestroyImageInfo(image_info);
     }
}

static FT_UInt GetFreetypeGlyphId(const GraphemeInfo *grapheme,
  const FT_UInt missing_glyph_id)
{
  FT_UInt
    id;

  id=(FT_UInt) grapheme->index;
  if (id == 0)
    id=missing_glyph_id;
  return(id);
}

#define ThrowFreetypeErrorException(tag,ft_status,value) \
{ \
  const char *error_string = FreetypeErrorMessage(ft_status); \
  if (error_string != (const char *) NULL) \
    (void) ThrowMagickException(exception,GetMagickModule(),TypeError, \
      tag,"`%s (%s)'",value, error_string); \
  else \
    (void) ThrowMagickException(exception,GetMagickModule(),TypeError, \
      tag,"`%s'",value); \
}

typedef struct _FreetypeFaceTargets
{
  FT_Memory
    *memory;

  FT_Library
    *library;

  FT_StreamRec
    **stream;

  FT_Face
    *face;
} FreetypeFaceTargets;

static MagickBooleanType ConfigureFreetypeFace(const FreetypeTarget *target,
  const FreetypeFaceTargets *targets,ExceptionInfo *exception)
{
  FT_Memory
    *const memory = targets->memory;

  FT_Library
    *const library = targets->library;

  FT_StreamRec
    **const stream = targets->stream;

  FT_Face
    *const face = targets->face;

  const DrawInfo
    *draw_info = target->draw_info;

  const char
    *encoding = target->encoding;

  FT_Encoding
    encoding_type;

  FT_Error
    ft_status;

  PointInfo
    resolution;

  if ((draw_info->metrics != (char *) NULL) &&
      (IsPathAccessible(draw_info->metrics) != MagickFalse))
    (void) FT_Attach_File(*face,draw_info->metrics);
  encoding_type=FT_ENCODING_UNICODE;
  SelectFreetypeCharmap(*face,encoding_type);
  if (encoding != (const char *) NULL)
    {
      encoding_type=SelectFreetypeEncoding(encoding,encoding_type);
      ft_status=FT_Select_Charmap(*face,encoding_type);
      if (ft_status != 0)
        {
          (void) FT_Done_Face(*face);
          FreetypeDone(*memory,*library,*stream);
          ThrowFreetypeErrorException("UnrecognizedFontEncoding",ft_status,
            encoding);
          return(MagickFalse);
        }
    }
  /*
    Set text size.
  */
  resolution=GetDensityResolution(draw_info);
  ft_status=FT_Set_Char_Size(*face,(FT_F26Dot6) (64.0*draw_info->pointsize),
    (FT_F26Dot6) (64.0*draw_info->pointsize),(FT_UInt) resolution.x,
    (FT_UInt) resolution.y);
  if (ft_status != 0)
    {
      (void) FT_Done_Face(*face);
      FreetypeDone(*memory,*library,*stream);
      ThrowFreetypeErrorException("UnableToReadFont",ft_status,
        draw_info->font);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType OpenFreetypeFace(const FreetypeTarget *target,
  const FreetypeFaceTargets *targets,ExceptionInfo *exception)
{
  FT_Memory
    *const memory = targets->memory;

  FT_Library
    *const library = targets->library;

  FT_StreamRec
    **const stream = targets->stream;

  FT_Face
    *const face = targets->face;

  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  FT_Error
    ft_status;

  FT_Long
    face_index;

  FT_Open_Args
    args;

  /*
    Initialize Truetype library.
  */
  if (IsUnauthorizedFontFile(draw_info) != MagickFalse)
    ThrowPolicyException(draw_info->font,MagickFalse);
  *memory=FreetypeAcquireMemoryManager();
  if (*memory == (FT_Memory) NULL)
    ThrowBinaryException(ResourceLimitError,"UnableToInitializeFreetypeLibrary",
      image->filename);
  ft_status=FreetypeInit(*memory,library);
  if (ft_status != 0)
    {
      ThrowFreetypeErrorException("UnableToInitializeFreetypeLibrary",ft_status,
        image->filename);
      return(MagickFalse);
    }
  /*
    Open font face.
  */
  face_index=(FT_Long) draw_info->face;
  (void) memset(&args,0,sizeof(args));
  SelectFreetypeFontPath(draw_info,&args,&face_index,exception);
  /*
    Configure streaming interface.
  */
  *stream=AcquireFreetypeStream(args.pathname);
  args.flags=FT_OPEN_STREAM;
  args.stream=*stream;
  *face=(FT_Face) NULL;
  ft_status=FT_Open_Face(*library,&args,face_index,face);
  if (ft_status != 0)
    {
      FreetypeDone(*memory,*library,*stream);
      ThrowFreetypeErrorException("UnableToReadFont",ft_status,args.pathname);
      args.pathname=DestroyString(args.pathname);
      return(MagickFalse);
    }
  args.pathname=DestroyString(args.pathname);
  return(ConfigureFreetypeFace(target,targets,exception));
}

typedef struct _GlyphInfo
{
  FT_UInt
    id;

  FT_Vector
    origin;

  FT_Glyph
    image;
} GlyphInfo;

static FT_Outline_Funcs
  OutlineMethods =
  {
    (FT_Outline_MoveTo_Func) TraceMoveTo,
    (FT_Outline_LineTo_Func) TraceLineTo,
    (FT_Outline_ConicTo_Func) TraceQuadraticBezier,
    (FT_Outline_CubicTo_Func) TraceCubicBezier,
    0, 0
  };

typedef struct _GlyphLoadInput
{
  FT_Face
    face;

  FT_Int32
    flags;

  const DrawInfo
    *draw_info;

  MagickBooleanType
    status;

  DrawInfo
    *annotate_info;

  FT_Matrix
    affine;

  TypeMetric
    *metrics;
} GlyphLoadInput;

static MagickBooleanType LoadFreetypeGlyph(const GlyphLoadInput *glyph_input,
  GlyphInfo *glyph,FT_BBox *bounds)
{
  const FT_Face
    face = glyph_input->face;

  const FT_Int32
    flags = glyph_input->flags;

  const DrawInfo
    *const draw_info = glyph_input->draw_info;

  const MagickBooleanType
    status = glyph_input->status;

  DrawInfo
    *const annotate_info = glyph_input->annotate_info;

  const FT_Matrix
    affine = glyph_input->affine;

  TypeMetric
    *const metrics = glyph_input->metrics;

  FT_Error
    ft_status;

  FT_Outline
    outline;

    (*glyph).image=DestroyFreetypeGlyph((*glyph).image);
    ft_status=FT_Load_Glyph(face,(*glyph).id,flags);
    if (ft_status != 0)
      return(MagickTrue);
    ft_status=FT_Get_Glyph(face->glyph,&(*glyph).image);
    if (ft_status != 0)
      return(MagickTrue);
    outline=((FT_OutlineGlyph) (*glyph).image)->outline;
    if (IsNonOutlineGlyph((*glyph).image,outline) != MagickFalse)
      return(MagickTrue);
    ft_status=FT_Outline_Get_BBox(&outline,bounds);
    if (ft_status != 0)
      return(MagickTrue);
    ExtendGlyphMinBounds((*bounds),metrics);
    ExtendGlyphMaxBounds((*bounds),metrics);
    if (IsRenderedStroke(draw_info,status) != MagickFalse)
      TraceGlyphOutline(annotate_info,(*glyph).origin,outline,&OutlineMethods);
    FT_Vector_Transform(&(*glyph).origin,&affine);
    (void) FT_Glyph_Transform((*glyph).image,&affine,&(*glyph).origin);
    ft_status=FT_Glyph_To_Bitmap(&(*glyph).image,FT_RENDER_MODE_NORMAL,
      (FT_Vector *) NULL,MagickTrue);
    if (ft_status != 0)
      return(MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType RenderFreetype(const FreetypeTarget *target,
  const PointInfo *offset,TypeMetric *metrics,ExceptionInfo *exception)
{
  Image
    *image = target->image;

  const DrawInfo
    *draw_info = target->draw_info;

  const char
    *encoding = target->encoding;

#if !defined(FT_OPEN_PATHNAME)
#define FT_OPEN_PATHNAME  ft_open_pathname
#endif

  char
    *p;

  DrawInfo
    *annotate_info;

  FT_BBox
    bounds;

  FT_BitmapGlyph
    bitmap;

  FT_Face
    face;

  FT_Int32
    flags;

  FT_Library
    library;

  FT_Matrix
    affine;

  FT_Memory
    memory;

  FT_StreamRec
    *stream;

  FT_UInt
    first_glyph_id,
    last_glyph_id,
    missing_glyph_id;

  FT_Vector
    origin;

  GlyphInfo
    glyph;

  GraphemeInfo
    *grapheme;

  FreetypeFaceTargets
    targets;

  MagickBooleanType
    status;

  PointInfo
    point;

  ssize_t
    i;

  size_t
    length;

  ssize_t
    code,
    last_character;

  unsigned char
    *utf8;

  targets.memory=(&memory);
  targets.library=(&library);
  targets.stream=(&stream);
  targets.face=(&face);
  if (OpenFreetypeFace(target,&targets,exception) == MagickFalse)
    return(MagickFalse);
  SetFreetypeMetrics(face,metrics);
  first_glyph_id=0;
  FT_Get_First_Char(face,&first_glyph_id);
  if (IsTextWithoutGlyphs(draw_info,first_glyph_id) != MagickFalse)
    {
      (void) FT_Done_Face(face);
      FreetypeDone(memory,library,stream);
      return(MagickTrue);
    }
  /*
    Compute bounding box.
  */
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),"Font %s; "
      "font-encoding %s; text-encoding %s; pointsize %g",
      draw_info->font != (char *) NULL ? draw_info->font : "none",
      encoding != (char *) NULL ? encoding : "none",
      draw_info->encoding != (char *) NULL ? draw_info->encoding : "none",
      draw_info->pointsize);
  flags=GetFreetypeLoadFlags(draw_info);
  flags=GetFreetypeHintingFlags(image,flags,exception);
  glyph.id=0;
  glyph.image=(FT_Glyph) NULL;
  last_glyph_id=0;
  origin.x=0;
  origin.y=0;
  affine=GetFreetypeAffine(draw_info);
  annotate_info=CloneFreetypeAnnotateInfo(draw_info);
  status=MagickTrue;
  if (draw_info->render != MagickFalse)
    SetFreetypeCanvas(image,exception);
  p=FindInvalidUTFCode(draw_info);
  SelectFreetypeText(draw_info,&utf8,&p);
  grapheme=(GraphemeInfo *) NULL;
  length=strlen(p);
#if defined(MAGICKCORE_RAQM_DELEGATE)
  if (ContainsRaqmParagraphSeparator((const unsigned char *) p,length) == MagickFalse)
    length=ComplexRaqmTextLayout(image,draw_info,p,length,face,&grapheme,
      exception);
  else
#endif
  length=ComplexTextLayout(draw_info,p,length,face,flags,&grapheme);
  missing_glyph_id=FT_Get_Char_Index(face,' ');
  code=0;
  last_character=(ssize_t) length-1;
  for (i=0; i < (ssize_t) length; i++)
  {
    GlyphLoadInput
      glyph_input;

    /*
      Render UTF-8 sequence.
    */
    glyph.id=GetFreetypeGlyphId(grapheme+i,missing_glyph_id);
    if ((glyph.id != 0) && (last_glyph_id != 0))
      origin.x+=(FT_Pos) (64.0*draw_info->kerning);
    glyph.origin=origin;
    glyph.origin.x+=(FT_Pos) grapheme[i].x_offset;
    glyph.origin.y+=(FT_Pos) grapheme[i].y_offset;
    glyph_input.face=face;
    glyph_input.flags=flags;
    glyph_input.draw_info=draw_info;
    glyph_input.status=status;
    glyph_input.annotate_info=annotate_info;
    glyph_input.affine=affine;
    glyph_input.metrics=metrics;
    if (LoadFreetypeGlyph(&glyph_input,&glyph,&bounds) != MagickFalse)
      continue;
    bitmap=(FT_BitmapGlyph) glyph.image;
    point=GetGlyphBitmapPoint(offset,bitmap,origin);
    if (draw_info->render != MagickFalse)
      {
        GlyphBitmapTarget
          glyph_target;

        glyph_target.image=image;
        glyph_target.draw_info=draw_info;
        glyph_target.bitmap=bitmap;
        glyph_target.point=point;
        glyph_target.annotate_info=annotate_info;
        glyph_target.offset=offset;
        glyph_target.exception=exception;
        status=RasterizeGlyphBitmap(&glyph_target,status);
      }
    if (IsInterwordSpace(draw_info,p+grapheme[i].cluster,code) != MagickFalse)
      origin.x+=(FT_Pos) (64.0*draw_info->interword_spacing);
    else
      if (i == last_character)
        origin.x+=MagickMax((FT_Pos) grapheme[i].x_advance,bounds.xMax);
      else
        origin.x+=(FT_Pos) grapheme[i].x_advance;
    origin.y+=(FT_Pos) grapheme[i].y_advance;
    SetFreetypeMetricsOrigin(metrics,origin);
    last_glyph_id=glyph.id;
    code=GetUTFCode(p+grapheme[i].cluster);
  }
  RelinquishFreetypeText(grapheme,utf8);
  glyph.image=DestroyFreetypeGlyph(glyph.image);
  ScaleFreetypeMetrics(metrics);
  /*
    Relinquish resources.
  */
  annotate_info=DestroyDrawInfo(annotate_info);
  (void) FT_Done_Face(face);
  FreetypeDone(memory,library,stream);
  return(status);
}
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
  MagickBooleanType
    status;

  if (annotate_semaphore == (SemaphoreInfo *) NULL)
    ActivateSemaphoreInfo(&annotate_semaphore);
  LockSemaphoreInfo(annotate_semaphore);
  status=XRenderImage(image,draw_info,offset,metrics,exception);
  UnlockSemaphoreInfo(annotate_semaphore);
  return(status);
}
