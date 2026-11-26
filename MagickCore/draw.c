/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        DDDD   RRRR    AAA   W   W                           %
%                        D   D  R   R  A   A  W   W                           %
%                        D   D  RRRR   AAAAA  W W W                           %
%                        D   D  R RN   A   A  WW WW                           %
%                        DDDD   R  R   A   A  W   W                           %
%                                                                             %
%                                                                             %
%                     MagickCore Image Drawing Methods                        %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1998                                   %
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
% Bill Radcliffe of Corbis (www.corbis.com) contributed the polygon
% rendering code based on Paul Heckbert's "Concave Polygon Scan Conversion",
% Graphics Gems, 1990.  Leonard Rosenthal and David Harr of Appligent
% (www.appligent.com) contributed the dash pattern, linecap stroking
% algorithm, and minor rendering improvements.
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/annotate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/channel.h"
#include "MagickCore/color.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/composite.h"
#include "MagickCore/composite-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/constitute-private.h"
#include "MagickCore/draw.h"
#include "MagickCore/draw-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/resample.h"
#include "MagickCore/resample-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/thread-private.h"
#include "MagickCore/token.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/draw-core-private.h"

/*
  Define declarations.
*/
#define PrimitiveExtentPad  4296.0
#define MacroExpansionLimit  262144
#define ThrowPointExpectedException(token,exception) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),DrawError, \
    "NonconformingDrawingPrimitiveDefinition","`%s'",token); \
  status=MagickFalse; \
  break; \
}

typedef struct _ElementInfo
{
  double
    cx,
    cy,
    major,
    minor,
    angle;
} ElementInfo;

typedef struct _MVGInfo
{
  PrimitiveInfo
    **primitive_info;

  size_t
    *extent;

  ssize_t
    offset;

  PointInfo
    point;

  ExceptionInfo
    *exception;
} MVGInfo;

typedef struct _ArcPathShape
{
  PointInfo
    arc;

  double
    angle;

  MagickBooleanType
    large_arc,
    sweep;
} ArcPathShape;

typedef struct _CompositeMaskPath
{
  const char
    *id,
    *mask_path;
} CompositeMaskPath;

typedef struct _ClippingMaskPath
{
  const char
    *id,
    *clip_path;
} ClippingMaskPath;


/*
  Forward declarations.
*/
static Image
  *DrawClippingMask(Image *,const DrawInfo *,const ClippingMaskPath *,
    ExceptionInfo *);

static MagickBooleanType
  RenderMVGContent(Image *,const DrawInfo *,const size_t,ExceptionInfo *),
  TraceArc(MVGInfo *,const PointInfo,const PointInfo,const PointInfo),
  TraceArcPath(MVGInfo *,const PointInfo,const PointInfo,const ArcPathShape *),
  TraceBezier(MVGInfo *,const size_t),
  TraceCircle(MVGInfo *,const PointInfo,const PointInfo),
  TraceEllipse(MVGInfo *,const PointInfo,const PointInfo,const PointInfo),
  TraceLine(PrimitiveInfo *,const PointInfo,const PointInfo),
  TraceRoundRectangle(MVGInfo *,const PointInfo,const PointInfo,PointInfo);

static ssize_t
  TracePath(MVGInfo *,const char *,ExceptionInfo *);



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w C l i p P a t h                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawClipPath() draws the clip path on the image mask.
%
%  The format of the DrawClipPath method is:
%
%      MagickBooleanType DrawClipPath(Image *image,const DrawInfo *draw_info,
%        const char *id,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o id: the clip path id.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType DrawClipPath(Image *image,
  const DrawInfo *draw_info,const char *id,ExceptionInfo *exception)
{
  ClippingMaskPath
    clip_input;

  const char
    *clip_path;

  Image
    *clipping_mask;

  MagickBooleanType
    status;

  clip_path=GetImageArtifact(image,id);
  if (clip_path == (const char *) NULL)
    return(MagickFalse);
  clip_input.id=draw_info->clip_mask;
  clip_input.clip_path=clip_path;
  clipping_mask=DrawClippingMask(image,draw_info,&clip_input,exception);
  if (clipping_mask == (Image *) NULL)
    return(MagickFalse);
  status=SetImageMask(image,WritePixelMask,clipping_mask,exception);
  clipping_mask=DestroyImage(clipping_mask);
  return(status);
}

#include "MagickCore/draw-mvg-trace-private.h"

#include "MagickCore/draw-mvg-push-private.h"

#include "MagickCore/draw-mvg-keyword-private.h"

#include "MagickCore/draw-mvg-render-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawImage() draws a graphic primitive on your image.  The primitive
%  may be represented as a string or filename.  Precede the filename with an
%  "at" sign (@) and the contents of the file are drawn on the image.  You
%  can affect how text is drawn by setting one or more members of the draw
%  info structure.
%
%  The format of the DrawImage method is:
%
%      MagickBooleanType DrawImage(Image *image,const DrawInfo *draw_info,
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
MagickExport MagickBooleanType DrawImage(Image *image,const DrawInfo *draw_info,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  size_t
    macro_expansion = 0;

  ((DrawInfo *) draw_info)->macro_expansion=(&macro_expansion);
  status=RenderMVGContent(image,draw_info,0,exception);
  ((DrawInfo *) draw_info)->macro_expansion=(size_t *) NULL;
  return(status);
}

static DrawInfo *ClonePatternDrawInfo(const Image *image,
  const DrawInfo *draw_info,const char *name)
{
  char
    property[MagickPathExtent];

  const char
    *type;

  DrawInfo
    *clone_info;

  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  if (clone_info->fill_pattern != (Image *) NULL)
    clone_info->fill_pattern=DestroyImage(clone_info->fill_pattern);
  if (clone_info->stroke_pattern != (Image *) NULL)
    clone_info->stroke_pattern=DestroyImage(clone_info->stroke_pattern);
  (void) FormatLocaleString(property,MagickPathExtent,"%s-type",name);
  type=GetImageArtifact(image,property);
  if (type != (const char *) NULL)
    clone_info->gradient.type=(GradientType) ParseCommandOption(
      MagickGradientOptions,MagickFalse,type);
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t t e r n P a t h                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPatternPath() draws a pattern.
%
%  The format of the DrawPatternPath method is:
%
%      MagickBooleanType DrawPatternPath(Image *image,const DrawInfo *draw_info,
%        const char *name,Image **pattern,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o name: the pattern name.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType DrawPatternPath(Image *image,
  const DrawInfo *draw_info,const char *name,Image **pattern,
  ExceptionInfo *exception)
{
  char
    property[MagickPathExtent];

  const char
    *geometry,
    *path;

  DrawInfo
    *clone_info;

  ImageInfo
    *image_info;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (const DrawInfo *) NULL);
  assert(name != (const char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  (void) FormatLocaleString(property,MagickPathExtent,"%s",name);
  path=GetImageArtifact(image,property);
  if (path == (const char *) NULL)
    return(MagickFalse);
  (void) FormatLocaleString(property,MagickPathExtent,"%s-geometry",name);
  geometry=GetImageArtifact(image,property);
  if (geometry == (const char *) NULL)
    return(MagickFalse);
  if ((*pattern) != (Image *) NULL)
    *pattern=DestroyImage(*pattern);
  image_info=AcquireImageInfo();
  image_info->size=AcquireString(geometry);
  *pattern=AcquireImage(image_info,exception);
  image_info=DestroyImageInfo(image_info);
  (void) QueryColorCompliance("#00000000",AllCompliance,
    &(*pattern)->background_color,exception);
  (void) SetImageBackgroundColor(*pattern,exception);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),
      "begin pattern-path %s %s",name,geometry);
  clone_info=ClonePatternDrawInfo(image,draw_info,name);
  (void) CloneString(&clone_info->primitive,path);
  status=RenderMVGContent(*pattern,clone_info,0,exception);
  clone_info=DestroyDrawInfo(clone_info);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"end pattern-path");
  return(status);
}



#include "MagickCore/draw-path-trace-private.h"

