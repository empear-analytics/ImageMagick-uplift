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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w C l i p p i n g M a s k                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawClippingMask() draws the clip path and returns it as an image clipping
%  mask.
%
%  The format of the DrawClippingMask method is:
%
%      Image *DrawClippingMask(Image *image,const DrawInfo *draw_info,
%        const char *id,const char *clip_path,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o id: the clip path id.
%
%    o clip_path: the clip path.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *DrawClippingMask(Image *image,const DrawInfo *draw_info,
  const ClippingMaskPath *path,ExceptionInfo *exception)
{
  const char
    *const id = path->id,
    *const clip_path = path->clip_path;

  DrawInfo
    *clone_info;

  Image
    *clip_mask,
    *separate_mask;

  MagickStatusType
    status;

  /*
    Draw a clip path.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (const DrawInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  clip_mask=AcquireImage((const ImageInfo *) NULL,exception);
  status=SetImageExtent(clip_mask,image->columns,image->rows,exception);
  if (status == MagickFalse)
    return(DestroyImage(clip_mask));
  status=SetImageMask(clip_mask,WritePixelMask,(Image *) NULL,exception);
  status=QueryColorCompliance("#0000",AllCompliance,
    &clip_mask->background_color,exception);
  clip_mask->background_color.alpha=(MagickRealType) TransparentAlpha;
  clip_mask->background_color.alpha_trait=BlendPixelTrait;
  status=SetImageBackgroundColor(clip_mask,exception);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"\nbegin clip-path %s",
      id);
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  (void) CloneString(&clone_info->primitive,clip_path);
  status=QueryColorCompliance("#ffffff",AllCompliance,&clone_info->fill,
    exception);
  if (clone_info->clip_mask != (char *) NULL)
    clone_info->clip_mask=DestroyString(clone_info->clip_mask);
  status=QueryColorCompliance("#00000000",AllCompliance,&clone_info->stroke,
    exception);
  clone_info->stroke_width=0.0;
  clone_info->alpha=OpaqueAlpha;
  clone_info->clip_path=MagickTrue;
  status=RenderMVGContent(clip_mask,clone_info,0,exception);
  clone_info=DestroyDrawInfo(clone_info);
  separate_mask=SeparateImage(clip_mask,AlphaChannel,exception);
  if (separate_mask == (Image *) NULL)
    status=MagickFalse;
  else
    {
      clip_mask=DestroyImage(clip_mask);
      clip_mask=separate_mask;
      status&=(MagickStatusType) NegateImage(clip_mask,MagickFalse,exception);
    }
  if (status == MagickFalse)
    clip_mask=DestroyImage(clip_mask);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"end clip-path");
  return(clip_mask);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w C o m p o s i t e M a s k                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawCompositeMask() draws the mask path and returns it as an image mask.
%
%  The format of the DrawCompositeMask method is:
%
%      Image *DrawCompositeMask(Image *image,const DrawInfo *draw_info,
%        const char *id,const char *mask_path,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o id: the mask path id.
%
%    o mask_path: the mask path.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *DrawCompositeMask(Image *image,const DrawInfo *draw_info,
  const CompositeMaskPath *path,ExceptionInfo *exception)
{
  const char
    *const id = path->id,
    *const mask_path = path->mask_path;

  Image
    *composite_mask,
    *separate_mask;

  DrawInfo
    *clone_info;

  MagickStatusType
    status;

  /*
    Draw a mask path.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (const DrawInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  composite_mask=AcquireImage((const ImageInfo *) NULL,exception);
  status=SetImageExtent(composite_mask,image->columns,image->rows,exception);
  if (status == MagickFalse)
    return(DestroyImage(composite_mask));
  status=SetImageMask(composite_mask,CompositePixelMask,(Image *) NULL,
    exception);
  status=QueryColorCompliance("#0000",AllCompliance,
    &composite_mask->background_color,exception);
  composite_mask->background_color.alpha=(MagickRealType) TransparentAlpha;
  composite_mask->background_color.alpha_trait=BlendPixelTrait;
  (void) SetImageBackgroundColor(composite_mask,exception);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"\nbegin mask-path %s",
      id);
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  (void) CloneString(&clone_info->primitive,mask_path);
  status=QueryColorCompliance("#ffffff",AllCompliance,&clone_info->fill,
    exception);
  status=QueryColorCompliance("#00000000",AllCompliance,&clone_info->stroke,
    exception);
  clone_info->stroke_width=0.0;
  clone_info->alpha=OpaqueAlpha;
  status=RenderMVGContent(composite_mask,clone_info,0,exception);
  clone_info=DestroyDrawInfo(clone_info);
  separate_mask=SeparateImage(composite_mask,AlphaChannel,exception);
  if (separate_mask != (Image *) NULL)
    {
      composite_mask=DestroyImage(composite_mask);
      composite_mask=separate_mask;
      status=NegateImage(composite_mask,MagickFalse,exception);
      if (status == MagickFalse)
        composite_mask=DestroyImage(composite_mask);
    }
  if (status == MagickFalse)
    composite_mask=DestroyImage(composite_mask);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"end mask-path");
  return(composite_mask);
}



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

static inline MagickBooleanType IsMVGInfoMissing(const MVGInfo *mvg_info)
{
  return(((mvg_info == (MVGInfo *) NULL) ||
    (mvg_info->primitive_info == (PrimitiveInfo **) NULL) ||
    (*mvg_info->primitive_info == (PrimitiveInfo *) NULL) ||
    (mvg_info->extent == (size_t *) NULL)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType CheckPrimitiveExtent(MVGInfo *mvg_info,
  const double pad)
{
  double
    proposed_extent;

  PrimitiveInfo
    *primitive_info;

  size_t
    extent;

  ssize_t
    i;

  if (IsMVGInfoMissing(mvg_info) != MagickFalse)
    return(MagickFalse);
  proposed_extent=mvg_info->offset+pad+PrimitiveExtentPad+1.0;
  if ((proposed_extent <= 0.0) || (proposed_extent > (double) MAGICK_SIZE_MAX))
    return(MagickFalse);
  extent=CastDoubleToSizeT(ceil(proposed_extent));
  if (extent <= *mvg_info->extent)
    return(MagickTrue);
  if (extent > (GetMaxMemoryRequest()/sizeof(PrimitiveInfo)))
    return(MagickFalse);
  primitive_info=(PrimitiveInfo *) ResizeQuantumMemory(
    *mvg_info->primitive_info,extent+1,sizeof(PrimitiveInfo));
  if (primitive_info == (PrimitiveInfo *) NULL)
    {
      /*
        Create a stack to unwind; report failure.
      */
      extent=(size_t) PrimitiveExtentPad;
      primitive_info=(PrimitiveInfo *) AcquireCriticalMemory(extent*
        sizeof(*primitive_info));
      (void) memset(primitive_info,0,extent*sizeof(*primitive_info));
      *mvg_info->primitive_info=primitive_info;
      *mvg_info->extent=extent;
      mvg_info->offset=0;
      ThrowMagickException(mvg_info->exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return(MagickFalse);
    }
  primitive_info[extent].primitive=UndefinedPrimitive;
  primitive_info[extent].text=(char *) NULL;
  /*
    Commit updated buffer.
  */
  for (i=(ssize_t) *mvg_info->extent; i < (ssize_t) extent; i++)
  {
    primitive_info[i].primitive=UndefinedPrimitive;
    primitive_info[i].text=(char *) NULL;
  }
  *mvg_info->primitive_info=primitive_info;
  *mvg_info->extent=extent;
  return(MagickTrue);
}

static int MVGMacroCompare(const void *target,const void *source)
{
  const char
    *p,
    *q;

  p=(const char *) target;
  q=(const char *) source;
  return(strcmp(p,q));
}

static MagickBooleanType GetMVGMacroToken(const char **p,const size_t extent,
  char *token)
{
  if (GetNextToken((*p),p,extent,token) < 1)
    return(MagickFalse);
  if (*token == '\0')
    return(MagickFalse);
  return(MagickTrue);
}

typedef struct _MVGMacroExtract
{
  const char
    *start,
    *end,
    *p;

  size_t
    extent;

  char
    *token,
    *macro;

  SplayTreeInfo
    *macros;

  char
    *name;
} MVGMacroExtract;

static void AddMVGMacro(const MVGMacroExtract *extract)
{
  const char
    *const start = extract->start,
    *const end = extract->end;

  const char
    *p = extract->p;

  const size_t
    extent = extract->extent;

  char
    *const token = extract->token,
    *const macro = extract->macro;

  SplayTreeInfo
    *const macros = extract->macros;

  char
    *const name = extract->name;

  size_t
    length;

  length=(size_t) (end-start);
  (void) GetNextToken(p,&p,extent,token);
  if (length > 0)
    {
      (void) CopyMagickString(macro,start,length);
      (void) AddValueToSplayTree(macros,ConstantString(name),
        ConstantString(macro));
    }
}

static void MatchMVGMacroPop(const char *token,const char *p,
  const char **end,ssize_t *n)
{
  if (LocaleCompare(token,"pop") == 0)
    {
      (*end)=p-strlen(token)-1;
      (*n)--;
    }
}

typedef struct _MVGMacroState
{
  char
    **macro,
    **token;

  const char
    **q;

  ExceptionInfo
    *exception;

  size_t
    *extent;

  SplayTreeInfo
    **macros;
} MVGMacroState;

static void ScanMVGMacro(const MVGMacroState *state)
{
  char
    *macro = (*state->macro),
    *token = (*state->token);

  const char
    *q = (*state->q);

  ExceptionInfo
    *exception = state->exception;

  size_t
    extent = (*state->extent);

  SplayTreeInfo
    *macros = (*state->macros);

  char
    name[MagickPathExtent];

  const char
    *end,
    *p,
    *start;

  ssize_t
    n;

  start=q;
  end=q;
  (void) CopyMagickString(name,token,MagickPathExtent);
  n=1;
  for (p=q; *p != '\0'; )
  {
    MVGMacroExtract
      extract;

    if (GetMVGMacroToken(&p,extent,token) == MagickFalse)
      break;
    MatchMVGMacroPop(token,p,&end,&n);
    if ((LocaleCompare(token,"push") == 0) &&
        (n++ >= MagickMaxRecursionDepth))
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          DrawError,"VectorGraphicsNestedTooDeeply","`%s'",token);
        break;
      }
    if ((n != 0) || (end < start))
      continue;
    /*
      Extract macro.
    */
    extract.start=start;
    extract.end=end;
    extract.p=p;
    extract.extent=extent;
    extract.token=token;
    extract.macro=macro;
    extract.macros=macros;
    extract.name=name;
    AddMVGMacro(&extract);
    break;
  }
}

static SplayTreeInfo *GetMVGMacros(const char *primitive,
  ExceptionInfo *exception)
{
  char
    *macro,
    *token;

  const char
    *q;

  size_t
    extent;

  SplayTreeInfo
    *macros;

  const MVGMacroState
    macro_state = { &macro, &token, &q, exception, &extent, &macros };

  /*
    Scan graphic primitives for definitions and classes.
  */
  if (primitive == (const char *) NULL)
    return((SplayTreeInfo *) NULL);
  macros=NewSplayTree(MVGMacroCompare,RelinquishMagickMemory,
    RelinquishMagickMemory);
  macro=AcquireString(primitive);
  token=AcquireString(primitive);
  extent=strlen(token)+MagickPathExtent;
  for (q=primitive; *q != '\0'; )
  {
    if (GetMVGMacroToken(&q,extent,token) == MagickFalse)
      break;
    if (LocaleCompare("push",token) != 0)
      continue;
    (void) GetNextToken(q,&q,extent,token);
    if (*q != '"')
      continue;
    /*
      Named macro (e.g. push graphic-context "wheel").
    */
    (void) GetNextToken(q,&q,extent,token);
    ScanMVGMacro(&macro_state);
  }
  token=DestroyString(token);
  macro=DestroyString(macro);
  return(macros);
}

static inline MagickBooleanType IsValidListChar(int c)
{
  if ((c >= '0') && (c <= '9'))
    return(MagickTrue);
  switch (c)
  {
    case '.':
    case '+':
    case '-':
    case ',':
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      break;
    default:
      return(MagickFalse);
  }
  return(MagickTrue);
}

static inline MagickBooleanType IsValidPoint(const char *point)
{
  char
    *p;

  double
    value;

  value=GetDrawValue(point,&p);
  return((fabs(value) < MagickEpsilon) && (p == point) ? MagickFalse :
    MagickTrue);
}

static inline MagickBooleanType IsNonIdentityAffine(const AffineMatrix affine)
{
  return(((fabs(affine.sx-1.0) >= MagickEpsilon) ||
    (fabs(affine.rx) >= MagickEpsilon) || (fabs(affine.ry) >= MagickEpsilon) ||
    (fabs(affine.sy-1.0) >= MagickEpsilon) ||
    (fabs(affine.tx) >= MagickEpsilon) ||
    (fabs(affine.ty) >= MagickEpsilon)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsClipMaskChanged(DrawInfo **graphic_context,
  const ssize_t n)
{
  return(((n != 0) && (graphic_context[n]->compliance != SVGCompliance) &&
    (graphic_context[n]->clip_mask != (char *) NULL) &&
    (LocaleCompare(graphic_context[n]->clip_mask,
    graphic_context[n-1]->clip_mask) != 0)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsMVGFileReference(const DrawInfo *draw_info,
  const size_t depth)
{
  return(((*draw_info->primitive == '@') &&
    (strlen(draw_info->primitive) > 1) &&
    (*(draw_info->primitive+1) != '-') && (depth == 0)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsMVGBlockEndMissing(const char *q,
  const char *p)
{
  return(((q == (char *) NULL) || (p == (char *) NULL) ||
    ((q-4) < p)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsMVGBlockEndInvalid(const char *q,
  const char *p,const size_t extent)
{
  return(((q == (char *) NULL) || (*q == '\0') ||
    (p == (char *) NULL) || ((q-4) < p) ||
    ((size_t) (q-p+4+1) > extent)) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsMVGBlockEndOutside(const char *q,
  const char *p,const size_t extent)
{
  return(((q == (char *) NULL) || (p == (char *) NULL) ||
    ((q-4) < p) || ((size_t) (q-p+4+1) > extent)) ? MagickTrue : MagickFalse);
}

typedef struct _RenderableClassInput
{
  DrawInfo
    **graphic_context;

  ssize_t
    n;

  const char
    *mvg_class;

  const char
    *p;

  const char
    *primitive;
} RenderableClassInput;

static inline MagickBooleanType IsRenderableClass(
  const RenderableClassInput *input)
{
  DrawInfo
    **const graphic_context = input->graphic_context;

  const ssize_t
    n = input->n;

  const char
    *const mvg_class = input->mvg_class,
    *const p = input->p,
    *const primitive = input->primitive;

  return(((graphic_context[n]->render != MagickFalse) &&
    (mvg_class != (const char *) NULL) && (p > primitive)) ? MagickTrue :
    MagickFalse);
}

static void DestroyMVGPrimitiveText(PrimitiveInfo *primitive_info)
{
  ssize_t
    i;

  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++)
    if (primitive_info[i].text != (char *) NULL)
      primitive_info[i].text=DestroyString(primitive_info[i].text);
}

typedef struct _MVGRenderState
{
  AffineMatrix
    *affine,
    *current;

  char
    *keyword,
    **primitive,
    **token;

  const char
    **p,
    **q;

  double
    *coordinates,
    *cursor;

  DrawInfo
    ***graphic_context;

  const DrawInfo
    *draw_info;

  ExceptionInfo
    *exception;

  Image
    *image;

  MagickStatusType
    *status;

  MVGInfo
    *mvg_info;

  PrimitiveInfo
    **primitive_info;

  PrimitiveType
    *primitive_type;

  size_t
    depth,
    *extent,
    *number_points,
    *number_stops;

  SplayTreeInfo
    **macros;

  ssize_t
    *classDepth,
    *defsDepth,
    *i,
    *j,
    *n,
    *symbolDepth,
    *x;

  StopInfo
    **stops;
} MVGRenderState;

static MagickBooleanType SkipMVGListToken(const char **q,const size_t extent,
  char *token)
{
  (void) GetNextToken((*q),q,extent,token);
  if (IsValidListChar((int) ((unsigned char) *token)) == MagickFalse)
    return(MagickFalse);
  if (*token == ',')
    (void) GetNextToken((*q),q,extent,token);
  return(MagickTrue);
}

static MagickBooleanType GetMVGValue(const char **q,const size_t extent,
  char *token,double *value)
{
  char
    *next_token;

  (void) GetNextToken((*q),q,extent,token);
  (*value)=GetDrawValue(token,&next_token);
  if (token == next_token)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType GetMVGListValue(const char **q,const size_t extent,
  char *token,double *value)
{
  char
    *next_token;

  (void) GetNextToken((*q),q,extent,token);
  if (IsValidListChar((int) ((unsigned char) *token)) == MagickFalse)
    return(MagickFalse);
  if (*token == ',')
    (void) GetNextToken((*q),q,extent,token);
  (*value)=GetDrawValue(token,&next_token);
  if (token == next_token)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType GetMVGPoint(const char **q,const size_t extent,
  char *token,PointInfo *point)
{
  if (GetMVGValue(q,extent,token,&(*point).x) == MagickFalse)
    return(MagickFalse);
  if (GetMVGListValue(q,extent,token,&(*point).y) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void ParseMVGPrimitivePoints(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  MVGInfo
    mvg_info = (*state->mvg_info);

  PointInfo
    point;

  PrimitiveInfo
    *primitive_info = (*state->primitive_info);

  PrimitiveType
    primitive_type = (*state->primitive_type);

  size_t
    extent = (*state->extent),
    *number_points = state->number_points;

  ssize_t
    i = (*state->i),
    j = (*state->j),
    x = (*state->x);

  DestroyMVGPrimitiveText(primitive_info);
  i=0;
  mvg_info.offset=i;
  j=0;
  primitive_info[0].primitive=primitive_type;
  primitive_info[0].point.x=0.0;
  primitive_info[0].point.y=0.0;
  primitive_info[0].coordinates=0;
  primitive_info[0].method=FloodfillMethod;
  primitive_info[0].closed_subpath=MagickFalse;
  for (x=0; *q != '\0'; x++)
  {
    /*
      Define points.
    */
    if (IsValidPoint(q) == MagickFalse)
      break;
    if (GetMVGPoint(&q,extent,token,&point) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    (void) GetNextToken(q,(const char **) NULL,extent,token);
    if (*token == ',')
      (void) GetNextToken(q,&q,extent,token);
    primitive_info[i].primitive=primitive_type;
    primitive_info[i].point=point;
    primitive_info[i].coordinates=0;
    primitive_info[i].method=FloodfillMethod;
    primitive_info[i].closed_subpath=MagickFalse;
    i++;
    mvg_info.offset=i;
    if (i < (ssize_t) (*number_points))
      continue;
    status&=(MagickStatusType) CheckPrimitiveExtent(&mvg_info,(double)
      (*number_points));
    primitive_info=(*mvg_info.primitive_info);
    if (status == MagickFalse)
      break;
  }
  (*state->q)=q;
  (*state->status)=status;
  (*state->mvg_info)=mvg_info;
  (*state->primitive_info)=primitive_info;
  (*state->i)=i;
  (*state->j)=j;
  (*state->x)=x;
}

static double EstimatePathCoordinates(const char **q,const size_t extent,
  char *token)
{
  char
    *s,
    *t;

  double
    coordinates;

  (void) GetNextToken((*q),q,extent,token);
  coordinates=1.0;
  t=token;
  for (s=token; *s != '\0'; s=t)
  {
    double
      value;

    value=GetDrawValue(s,&t);
    (void) value;
    if (s == t)
      {
        t++;
        continue;
      }
    coordinates++;
  }
  return(coordinates);
}

static void AddPathCurveCoordinates(char *token,double *coordinates)
{
  char
    *s;

  for (s=token; *s != '\0'; s++)
    if (strspn(s,"AaCcQqSsTt") != 0)
      (*coordinates)+=(20.0*BezierQuantum)+360.0;
}

static SegmentInfo GetPrimitiveBounds(const PrimitiveInfo *primitive_info,
  const ssize_t j)
{
  PointInfo
    point;

  SegmentInfo
    bounds;

  ssize_t
    k;

  bounds.x1=primitive_info[j].point.x;
  bounds.y1=primitive_info[j].point.y;
  bounds.x2=primitive_info[j].point.x;
  bounds.y2=primitive_info[j].point.y;
  for (k=1; k < (ssize_t) primitive_info[j].coordinates; k++)
  {
    point=primitive_info[j+k].point;
    if (point.x < bounds.x1)
      bounds.x1=point.x;
    if (point.y < bounds.y1)
      bounds.y1=point.y;
    if (point.x > bounds.x2)
      bounds.x2=point.x;
    if (point.y > bounds.y2)
      bounds.y2=point.y;
  }
  return(bounds);
}

static void EstimateMVGPrimitiveExtent(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    coordinates = (*state->coordinates);


  PrimitiveInfo
    *primitive_info = (*state->primitive_info);

  PrimitiveType
    primitive_type = (*state->primitive_type);

  SegmentInfo
    bounds;

  size_t
    extent = (*state->extent);

  ssize_t
    j = (*state->j),
    x = (*state->x);

  primitive_info[j].text=DestroyString(primitive_info[j].text);
  primitive_info[j].primitive=primitive_type;
  primitive_info[j].coordinates=(size_t) x;
  primitive_info[j].method=FloodfillMethod;
  primitive_info[j].closed_subpath=MagickFalse;
  /*
    Circumscribe primitive within a circle.
  */
  bounds=GetPrimitiveBounds(primitive_info,j);
  /*
    Speculate how many points our primitive might consume.
  */
  coordinates=(double) primitive_info[j].coordinates;
  switch (primitive_type)
  {
    case RectanglePrimitive:
    {
      coordinates*=5.0;
      break;
    }
    case RoundRectanglePrimitive:
    {
      double
        alpha,
        beta,
        radius;

      alpha=bounds.x2-bounds.x1;
      beta=bounds.y2-bounds.y1;
      radius=hypot(alpha,beta);
      coordinates*=5.0;
      coordinates+=2.0*((size_t) ceil((double) MagickPI*radius))+6.0*
        BezierQuantum+360.0;
      break;
    }
    case BezierPrimitive:
    {
      coordinates=(BezierQuantum*(double) primitive_info[j].coordinates);
      break;
    }
    case PathPrimitive:
    {
      coordinates=EstimatePathCoordinates(&q,extent,token);
      AddPathCurveCoordinates(token,&coordinates);
      break;
    }
    default:
      break;
  }
  (*state->q)=q;
  (*state->coordinates)=coordinates;
}

static void ReserveMVGPrimitiveExtent(const MVGRenderState *state)
{
  double
    coordinates = (*state->coordinates);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  MVGInfo
    mvg_info = (*state->mvg_info);

  PrimitiveInfo
    *primitive_info = (*state->primitive_info);

  size_t
    *number_points = state->number_points;

  ssize_t
    i = (*state->i);

  do
  {
    if (((size_t) (i+coordinates)) >= (*number_points))
      {
        /*
          Resize based on speculative points required by primitive.
        */
        (*number_points)+=(size_t) coordinates+1;
        if ((*number_points) < (size_t) coordinates)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'",
              image->filename);
            status=MagickFalse;
            break;
          }
        mvg_info.offset=i;
        status&=CheckPrimitiveExtent(&mvg_info,(double) (*number_points)+
          (double) coordinates+1.0);
        primitive_info=(*mvg_info.primitive_info);
        if (status == MagickFalse)
          break;
      }
    status&=(MagickStatusType) CheckPrimitiveExtent(&mvg_info,
      PrimitiveExtentPad);
    primitive_info=(*mvg_info.primitive_info);
  } while (0);
  (*state->status)=status;
  (*state->mvg_info)=mvg_info;
  (*state->primitive_info)=primitive_info;
}

static ssize_t GetTracedPrimitiveEnd(const PrimitiveInfo *primitive_info,
  const MagickStatusType status,const ssize_t j,ssize_t i)
{
  if (status == MagickFalse)
    return(i);
  i=j+(ssize_t) primitive_info[j].coordinates;
  return(i);
}

static MagickBooleanType IsInvalidRoundRectanglePoints(
  const PrimitiveInfo *primitive_info,const ssize_t j)
{
  if (primitive_info[j].coordinates != 3)
    return(MagickTrue);
  if ((primitive_info[j+2].point.x < 0.0) ||
      (primitive_info[j+2].point.y < 0.0))
    return(MagickTrue);
  if ((primitive_info[j+1].point.x-primitive_info[j].point.x) < 0.0)
    return(MagickTrue);
  if ((primitive_info[j+1].point.y-primitive_info[j].point.y) < 0.0)
    return(MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType IsInvalidEllipsePoints(const PrimitiveInfo *primitive_info,
  const ssize_t j)
{
  if (primitive_info[j].coordinates != 3)
    return(MagickTrue);
  if ((primitive_info[j+1].point.x < 0.0) ||
      (primitive_info[j+1].point.y < 0.0))
    return(MagickTrue);
  return(MagickFalse);
}

typedef struct _MVGTextInput
{
  DrawInfo
    **graphic_context;

  ssize_t
    n;

  PrimitiveInfo
    *primitive_info;

  char
    *token;

  Image
    *image;

  ExceptionInfo
    *exception;
} MVGTextInput;

static void AdvanceMVGTextCursor(const MVGTextInput *text,MVGInfo *mvg_info,
  double *cursor,MagickStatusType *status)
{
  DrawInfo
    **const graphic_context = text->graphic_context;

  const ssize_t
    n = text->n;

  PrimitiveInfo
    *const primitive_info = text->primitive_info;

  char
    *const token = text->token;

  Image
    *const image = text->image;

  ExceptionInfo
    *const exception = text->exception;

  DrawInfo
    *clone_info;

  TypeMetric
    metrics;

  clone_info=CloneDrawInfo((ImageInfo *) NULL,graphic_context[n]);
  if ((fabs((*mvg_info).point.x-primitive_info->point.x) < MagickEpsilon) &&
      (fabs((*mvg_info).point.y-primitive_info->point.y) < MagickEpsilon))
    {
      (*mvg_info).point=primitive_info->point;
      primitive_info->point.x+=(*cursor);
    }
  else
    {
      (*mvg_info).point=primitive_info->point;
      (*cursor)=0.0;
    }
  clone_info->render=MagickFalse;
  clone_info->text=AcquireString(token);
  (*status)&=(MagickStatusType) GetTypeMetrics(image,clone_info,
    &metrics,exception);
  clone_info=DestroyDrawInfo(clone_info);
  (*cursor)+=metrics.width;
  if (graphic_context[n]->compliance != SVGCompliance)
    (*cursor)=0.0;
}

static void TraceMVGPrimitive(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    coordinates = (*state->coordinates),
    cursor = (*state->cursor);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  MVGInfo
    mvg_info = (*state->mvg_info);

  PrimitiveInfo
    *primitive_info = (*state->primitive_info);

  PrimitiveType
    primitive_type = (*state->primitive_type);

  size_t
    extent = (*state->extent);

  ssize_t
    i = (*state->i),
    j = (*state->j),
    n = (*state->n);

  MVGTextInput
    text;

  mvg_info.offset=j;
  switch (primitive_type)
  {
    case PointPrimitive:
    default:
    {
      if (primitive_info[j].coordinates != 1)
        {
          status=MagickFalse;
          break;
        }
      status&=(MagickStatusType) TracePoint(primitive_info+j,
        primitive_info[j].point);
      primitive_info=(*mvg_info.primitive_info);
      i=GetTracedPrimitiveEnd(primitive_info,status,j,i);
      break;
    }
    case LinePrimitive:
    {
      if (primitive_info[j].coordinates != 2)
        {
          status=MagickFalse;
          break;
        }
      status&=(MagickStatusType) TraceLine(primitive_info+j,
        primitive_info[j].point,primitive_info[j+1].point);
      primitive_info=(*mvg_info.primitive_info);
      i=GetTracedPrimitiveEnd(primitive_info,status,j,i);
      break;
    }
    case RectanglePrimitive:
    {
      if (primitive_info[j].coordinates != 2)
        {
          status=MagickFalse;
          break;
        }
      status&=(MagickStatusType) TraceRectangle(primitive_info+j,
        primitive_info[j].point,primitive_info[j+1].point);
      primitive_info=(*mvg_info.primitive_info);
      i=GetTracedPrimitiveEnd(primitive_info,status,j,i);
      break;
    }
    case RoundRectanglePrimitive:
    {
      if (IsInvalidRoundRectanglePoints(primitive_info,j) != MagickFalse)
        {
          status=MagickFalse;
          break;
        }
      status&=(MagickStatusType) TraceRoundRectangle(&mvg_info,
        primitive_info[j].point,primitive_info[j+1].point,
        primitive_info[j+2].point);
      primitive_info=(*mvg_info.primitive_info);
      i=GetTracedPrimitiveEnd(primitive_info,status,j,i);
      break;
    }
    case ArcPrimitive:
    {
      if (primitive_info[j].coordinates != 3)
        {
          status=MagickFalse;
          break;
        }
      status&=(MagickStatusType) TraceArc(&mvg_info,primitive_info[j].point,
        primitive_info[j+1].point,primitive_info[j+2].point);
      primitive_info=(*mvg_info.primitive_info);
      i=GetTracedPrimitiveEnd(primitive_info,status,j,i);
      break;
    }
    case EllipsePrimitive:
    {
      if (IsInvalidEllipsePoints(primitive_info,j) != MagickFalse)
        {
          status=MagickFalse;
          break;
        }
      status&=(MagickStatusType) TraceEllipse(&mvg_info,
        primitive_info[j].point,primitive_info[j+1].point,
        primitive_info[j+2].point);
      primitive_info=(*mvg_info.primitive_info);
      i=GetTracedPrimitiveEnd(primitive_info,status,j,i);
      break;
    }
    case CirclePrimitive:
    {
      if (primitive_info[j].coordinates != 2)
        {
          status=MagickFalse;
          break;
        }
      status&=(MagickStatusType) TraceCircle(&mvg_info,
        primitive_info[j].point,primitive_info[j+1].point);
      primitive_info=(*mvg_info.primitive_info);
      i=GetTracedPrimitiveEnd(primitive_info,status,j,i);
      break;
    }
    case PolylinePrimitive:
    {
      if (primitive_info[j].coordinates < 1)
        {
          status=MagickFalse;
          break;
        }
      break;
    }
    case PolygonPrimitive:
    {
      if (primitive_info[j].coordinates < 3)
        {
          status=MagickFalse;
          break;
        }
      primitive_info[i]=primitive_info[j];
      primitive_info[i].coordinates=0;
      primitive_info[j].coordinates++;
      primitive_info[j].closed_subpath=MagickTrue;
      i++;
      break;
    }
    case BezierPrimitive:
    {
      if (primitive_info[j].coordinates < 3)
        {
          status=MagickFalse;
          break;
        }
      status&=(MagickStatusType) TraceBezier(&mvg_info,
        primitive_info[j].coordinates);
      primitive_info=(*mvg_info.primitive_info);
      i=GetTracedPrimitiveEnd(primitive_info,status,j,i);
      break;
    }
    case PathPrimitive:
    {
      coordinates=(double) TracePath(&mvg_info,token,exception);
      primitive_info=(*mvg_info.primitive_info);
      if (status == MagickFalse)
        break;
      if (coordinates < 0.0)
        {
          status=MagickFalse;
          break;
        }
      i=(ssize_t) (j+coordinates);
      break;
    }
    case AlphaPrimitive:
    case ColorPrimitive:
    {
      ssize_t
        method;

      if (primitive_info[j].coordinates != 1)
        {
          status=MagickFalse;
          break;
        }
      (void) GetNextToken(q,&q,extent,token);
      method=ParseCommandOption(MagickMethodOptions,MagickFalse,token);
      if (method == -1)
        {
          status=MagickFalse;
          break;
        }
      primitive_info[j].method=(PaintMethod) method;
      break;
    }
    case TextPrimitive:
    {
      if (primitive_info[j].coordinates != 1)
        {
          status=MagickFalse;
          break;
        }
      if (*token != ',')
        (void) GetNextToken(q,&q,extent,token);
      (void) CloneString(&primitive_info[j].text,token);
      /*
        Compute text cursor offset.
      */
      text.graphic_context=graphic_context;
      text.n=n;
      text.primitive_info=primitive_info;
      text.token=token;
      text.image=image;
      text.exception=exception;
      AdvanceMVGTextCursor(&text,&mvg_info,&cursor,&status);
      break;
    }
    case ImagePrimitive:
    {
      if (primitive_info[j].coordinates != 2)
        {
          status=MagickFalse;
          break;
        }
      (void) GetNextToken(q,&q,extent,token);
      (void) CloneString(&primitive_info[j].text,token);
      break;
    }
  }
  mvg_info.offset=i;
  (*state->q)=q;
  (*state->coordinates)=coordinates;
  (*state->cursor)=cursor;
  (*state->status)=status;
  (*state->mvg_info)=mvg_info;
  (*state->primitive_info)=primitive_info;
  (*state->i)=i;
}

static void CheckMVGPrimitiveExtent(const MVGRenderState *state)
{
  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  MVGInfo
    mvg_info = (*state->mvg_info);

  PrimitiveInfo
    *primitive_info = (*state->primitive_info);

  ssize_t
    n = (*state->n);

  do
  {
    /*
      Sanity check.
    */
    status&=(MagickStatusType) CheckPrimitiveExtent(&mvg_info,ExpandAffine(
      &graphic_context[n]->affine));
    primitive_info=(*mvg_info.primitive_info);
    if (status == 0)
      break;
    status&=(MagickStatusType) CheckPrimitiveExtent(&mvg_info,(double)
      graphic_context[n]->stroke_width);
    primitive_info=(*mvg_info.primitive_info);
  } while (0);
  (*state->status)=status;
  (*state->mvg_info)=mvg_info;
  (*state->primitive_info)=primitive_info;
}

static void TransformMVGPrimitive(const MVGRenderState *state)
{
  DrawInfo
    **graphic_context = (*state->graphic_context);

  PointInfo
    point;

  PrimitiveInfo
    *primitive_info = (*state->primitive_info);

  size_t
    *number_points = state->number_points;

  ssize_t
    i = (*state->i),
    n = (*state->n);

  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++)
  {
    point=primitive_info[i].point;
    primitive_info[i].point.x=graphic_context[n]->affine.sx*point.x+
      graphic_context[n]->affine.ry*point.y+graphic_context[n]->affine.tx;
    primitive_info[i].point.y=graphic_context[n]->affine.rx*point.x+
      graphic_context[n]->affine.sy*point.y+graphic_context[n]->affine.ty;
    point=primitive_info[i].point;
    if (point.x < graphic_context[n]->bounds.x1)
      graphic_context[n]->bounds.x1=point.x;
    if (point.y < graphic_context[n]->bounds.y1)
      graphic_context[n]->bounds.y1=point.y;
    if (point.x > graphic_context[n]->bounds.x2)
      graphic_context[n]->bounds.x2=point.x;
    if (point.y > graphic_context[n]->bounds.y2)
      graphic_context[n]->bounds.y2=point.y;
    if (primitive_info[i].primitive == ImagePrimitive)
      break;
    if (i >= (ssize_t) (*number_points))
      ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  }
  (*state->i)=i;
}

static void DrawMVGPrimitive(const MVGRenderState *state)
{
  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  PrimitiveInfo
    *primitive_info = (*state->primitive_info);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  if (graphic_context[n]->render != MagickFalse)
    {
      if (IsClipMaskChanged(graphic_context,n) != MagickFalse)
        {
          const char
            *clip_path;

          clip_path=(const char *) GetValueFromSplayTree(macros,
            graphic_context[n]->clip_mask);
          if (clip_path != (const char *) NULL)
            (void) SetImageArtifact(image,graphic_context[n]->clip_mask,
              clip_path);
          status&=(MagickStatusType) DrawClipPath(image,graphic_context[n],
            graphic_context[n]->clip_mask,exception);
        }
      status&=(MagickStatusType) DrawPrimitive(image,graphic_context[n],
        primitive_info,exception);
    }
  (*state->status)=status;
}

static void ApplyMVGAffine(const MVGRenderState *state)
{
  AffineMatrix
    affine = (*state->affine),
    current = (*state->current);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ssize_t
    n = (*state->n);

  if (IsNonIdentityAffine(affine) != MagickFalse)
    {
      graphic_context[n]->affine.sx=current.sx*affine.sx+current.ry*affine.rx;
      graphic_context[n]->affine.rx=current.rx*affine.sx+current.sy*affine.rx;
      graphic_context[n]->affine.ry=current.sx*affine.ry+current.ry*affine.sy;
      graphic_context[n]->affine.sy=current.rx*affine.ry+current.sy*affine.sy;
      graphic_context[n]->affine.tx=current.sx*affine.tx+current.ry*affine.ty+
        current.tx;
      graphic_context[n]->affine.ty=current.rx*affine.tx+current.sy*affine.ty+
        current.ty;
    }
}

static void FinishMVGGradient(const MVGRenderState *state)
{
  const char
    *q = (*state->q);

  const DrawInfo
    *draw_info = state->draw_info;

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  size_t
    number_stops = (*state->number_stops);

  StopInfo
    *stops = (*state->stops);

  if (*q == '\0')
    {
      if (number_stops > 1)
        {
          GradientType
            type;

          type=LinearGradient;
          if (draw_info->gradient.type == RadialGradient)
            type=RadialGradient;
          (void) GradientImage(image,type,PadSpread,stops,number_stops,
            exception);
        }
     if (number_stops > 0)
       stops=(StopInfo *) RelinquishMagickMemory(stops);
    }
  (*state->stops)=stops;
}

static void ApplyMVGClipPath(const MVGRenderState *state)
{
  const char
    *clip_path;

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  clip_path=(const char *) GetValueFromSplayTree(macros,
    graphic_context[n]->clip_mask);
  if (clip_path != (const char *) NULL)
    (void) SetImageArtifact(image,
      graphic_context[n]->clip_mask,clip_path);
  status&=(MagickStatusType) DrawClipPath(image,
    graphic_context[n],graphic_context[n]->clip_mask,
    exception);
  (*state->status)=status;
}

static ssize_t RepeatOddDashPattern(DrawInfo **graphic_context,
  const ssize_t n,const ssize_t x,ssize_t j)
{
  if (((x & 0x01) != 0) && (j == x))
    for ( ; j < (2*x); j++)
      graphic_context[n]->dash_pattern[j]=
        graphic_context[n]->dash_pattern[j-x];
  return(j);
}

static void ParseMVGDashPattern(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    j = (*state->j),
    n = (*state->n),
    x = (*state->x);

  for (j=0; j < x; j++)
  {
    if (GetMVGListValue(&q,extent,token,
        &graphic_context[n]->dash_pattern[j]) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (graphic_context[n]->dash_pattern[j] <= 0.0)
      status=MagickFalse;
  }
  j=RepeatOddDashPattern(graphic_context,n,x,j);
  graphic_context[n]->dash_pattern[j]=0.0;
  (*state->q)=q;
  (*state->status)=status;
  (*state->j)=j;
}

static ssize_t CountMVGDashValues(const char *r,const size_t extent,
  char *token)
{
  ssize_t
    x;

  for (x=0; IsValidPoint(token) != MagickFalse; x++)
  {
    (void) GetNextToken(r,&r,extent,token);
    if (*token == ',')
      (void) GetNextToken(r,&r,extent,token);
  }
  return(x);
}

typedef MagickBooleanType
  (*ParseMVGKeywordMethod)(const MVGRenderState *);

typedef struct _MVGKeywordInfo
{
  const char
    *keyword;

  PrimitiveType
    primitive;

  ParseMVGKeywordMethod
    parse;
} MVGKeywordInfo;

static const MVGKeywordInfo *GetMVGKeywordInfo(const MVGKeywordInfo *keywords,
  const size_t number_keywords,const char *name)
{
  size_t
    i;

  for (i=0; i < number_keywords; i++)
    if (LocaleCompare(keywords[i].keyword,name) == 0)
      return(keywords+i);
  return((const MVGKeywordInfo *) NULL);
}

static MagickBooleanType ParseMVGPopDefs(const MVGRenderState *state)
{
  DrawInfo
    **graphic_context = (*state->graphic_context);

  ssize_t
    defsDepth = (*state->defsDepth),
    n = (*state->n);

  defsDepth--;
  graphic_context[n]->render=defsDepth > 0 ? MagickFalse :
    MagickTrue;
  (*state->defsDepth)=defsDepth;
  return(MagickTrue);
}

static inline MagickBooleanType IsNonSVGClipMask(const DrawInfo *draw_info)
{
  return(((draw_info->clip_mask != (char *) NULL) &&
    (draw_info->compliance != SVGCompliance)) ? MagickTrue : MagickFalse);
}

static MagickBooleanType ParseMVGPopGraphicContext(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  ssize_t
    n = (*state->n);

  do
  {
    if (n <= 0)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          DrawError,"UnbalancedGraphicContextPushPop","`%s'",token);
        status=MagickFalse;
        n=0;
        break;
      }
    if ((IsNonSVGClipMask(graphic_context[n]) != MagickFalse) &&
        (LocaleCompare(graphic_context[n]->clip_mask,
         graphic_context[n-1]->clip_mask) != 0))
      status=SetImageMask(image,WritePixelMask,(Image *) NULL,
        exception);
    graphic_context[n]=DestroyDrawInfo(graphic_context[n]);
    n--;
    break;
  } while (0);
  (*state->status)=status;
  (*state->n)=n;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGPopSymbol(const MVGRenderState *state)
{
  DrawInfo
    **graphic_context = (*state->graphic_context);

  ssize_t
    n = (*state->n),
    symbolDepth = (*state->symbolDepth);

  symbolDepth--;
  graphic_context[n]->render=symbolDepth > 0 ? MagickFalse :
    MagickTrue;
  (*state->symbolDepth)=symbolDepth;
  return(MagickTrue);
}

static const char *FindMVGPopKeyword(const char **q,const size_t extent,
  char *token,const char *keyword)
{
  const char
    *p;

  for (p=(*q); *(*q) != '\0'; )
  {
    if (GetNextToken((*q),q,extent,token) < 1)
      break;
    if (LocaleCompare(token,"pop") != 0)
      continue;
    (void) GetNextToken((*q),(const char **) NULL,extent,token);
    if (LocaleCompare(token,keyword) != 0)
      continue;
    break;
  }
  return(p);
}

static MagickBooleanType ParseMVGPushClass(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *p = (*state->p),
    *q = (*state->q);

  size_t
    extent = (*state->extent);

  do
  {
    /*
      Class context.
    */
    p=FindMVGPopKeyword(&q,extent,token,"class");
    (void) GetNextToken(q,&q,extent,token);
    break;
  } while (0);
  (*state->p)=p;
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGPushClipPath(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *p = (*state->p),
    *q = (*state->q);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    p=FindMVGPopKeyword(&q,extent,token,"clip-path");
    if (IsMVGBlockEndMissing(q,p) != MagickFalse)
      {
        status=MagickFalse;
        break;
      }
    (void) GetNextToken(q,&q,extent,token);
    break;
  } while (0);
  (*state->p)=p;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGPushDefs(const MVGRenderState *state)
{
  DrawInfo
    **graphic_context = (*state->graphic_context);

  ssize_t
    defsDepth = (*state->defsDepth),
    n = (*state->n);

  defsDepth++;
  graphic_context[n]->render=defsDepth > 0 ? MagickFalse :
    MagickTrue;
  (*state->defsDepth)=defsDepth;
  return(MagickTrue);
}

static MagickBooleanType GetMVGSegment(const char **q,const size_t extent,
  char *token,SegmentInfo *segment)
{
  if (GetMVGValue(q,extent,token,&(*segment).x1) == MagickFalse)
    return(MagickFalse);
  if (GetMVGListValue(q,extent,token,&(*segment).y1) == MagickFalse)
    return(MagickFalse);
  if (GetMVGListValue(q,extent,token,&(*segment).x2) == MagickFalse)
    return(MagickFalse);
  if (GetMVGListValue(q,extent,token,&(*segment).y2) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static SegmentInfo GetGradientBounds(DrawInfo **graphic_context,
  const ssize_t n,const SegmentInfo segment)
{
  SegmentInfo
    bounds;

  bounds.x1=graphic_context[n]->affine.sx*segment.x1+
    graphic_context[n]->affine.ry*segment.y1+
    graphic_context[n]->affine.tx;
  bounds.y1=graphic_context[n]->affine.rx*segment.x1+
    graphic_context[n]->affine.sy*segment.y1+
    graphic_context[n]->affine.ty;
  bounds.x2=graphic_context[n]->affine.sx*segment.x2+
    graphic_context[n]->affine.ry*segment.y2+
    graphic_context[n]->affine.tx;
  bounds.y2=graphic_context[n]->affine.rx*segment.x2+
    graphic_context[n]->affine.sy*segment.y2+
    graphic_context[n]->affine.ty;
  return(bounds);
}

static MagickBooleanType ParseMVGPushGradient(const MVGRenderState *state)
{
  char
    geometry[MagickPathExtent],
    *token = (*state->token);

  const char
    *p = (*state->p),
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  SegmentInfo
    bounds;

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    char
      key[2*MagickPathExtent],
      name[MagickPathExtent],
      type[MagickPathExtent];

    SegmentInfo
      segment;

    (void) GetNextToken(q,&q,extent,token);
    (void) CopyMagickString(name,token,MagickPathExtent);
    (void) GetNextToken(q,&q,extent,token);
    (void) CopyMagickString(type,token,MagickPathExtent);
    if (GetMVGSegment(&q,extent,token,&segment) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if ((LocaleCompare(type,"radial") == 0) &&
        (SkipMVGListToken(&q,extent,token) == MagickFalse))
      ThrowPointExpectedException(token,exception);
    p=FindMVGPopKeyword(&q,extent,token,"gradient");
    if (IsMVGBlockEndInvalid(q,p,extent) != MagickFalse)
      {
        status=MagickFalse;
        break;
      }
    (void) CopyMagickString(token,p,(size_t) (q-p-4+1));
    bounds=GetGradientBounds(graphic_context,n,segment);
    (void) FormatLocaleString(key,MagickPathExtent,"%s",name);
    (void) SetImageArtifact(image,key,token);
    (void) FormatLocaleString(key,MagickPathExtent,"%s-type",name);
    (void) SetImageArtifact(image,key,type);
    (void) FormatLocaleString(key,MagickPathExtent,"%s-geometry",
      name);
    (void) FormatLocaleString(geometry,MagickPathExtent,
      "%gx%g%+.15g%+.15g",
      MagickMax(fabs(bounds.x2-bounds.x1+1.0),1.0),
      MagickMax(fabs(bounds.y2-bounds.y1+1.0),1.0),
      bounds.x1,bounds.y1);
    (void) SetImageArtifact(image,key,geometry);
    (void) GetNextToken(q,&q,extent,token);
    break;
  } while (0);
  (*state->p)=p;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGPushGraphicContext(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    n++;
    graphic_context=(DrawInfo **) ResizeQuantumMemory(
      graphic_context,(size_t) (n+1),sizeof(*graphic_context));
    if (graphic_context == (DrawInfo **) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'",
          image->filename);
        status=MagickFalse;
        break;
      }
    graphic_context[n]=CloneDrawInfo((ImageInfo *) NULL,
      graphic_context[n-1]);
    if (*q == '"')
      {
        (void) GetNextToken(q,&q,extent,token);
        (void) CloneString(&graphic_context[n]->id,token);
      }
    if (n >= MagickMaxRecursionDepth)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          DrawError,"VectorGraphicsNestedTooDeeply","`%s'",
          image->filename);
        status=MagickFalse;
      }
    break;
  } while (0);
  (*state->q)=q;
  (*state->graphic_context)=graphic_context;
  (*state->status)=status;
  (*state->n)=n;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGPushMask(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  size_t
    extent = (*state->extent);

  (void) GetNextToken(q,&q,extent,token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGPushPattern(const MVGRenderState *state)
{
  char
    geometry[MagickPathExtent],
    *next_token,
    *token = (*state->token);

  const char
    *p = (*state->p),
    *q = (*state->q);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  do
  {
    char
      key[2*MagickPathExtent],
      name[MagickPathExtent];

    double
      value;

    RectangleInfo
      region;

    (void) GetNextToken(q,&q,extent,token);
    (void) CopyMagickString(name,token,MagickPathExtent);
    (void) GetNextToken(q,&q,extent,token);
    region.x=CastDoubleToSsizeT(ceil(GetDrawValue(token,
      &next_token)-0.5));
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    if (GetMVGListValue(&q,extent,token,&value) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    region.y=CastDoubleToSsizeT(ceil(value-0.5));
    if (GetMVGListValue(&q,extent,token,&value) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    region.width=CastDoubleToSizeT(floor(value+0.5));
    if (GetMVGListValue(&q,extent,token,&value) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    region.height=CastDoubleToSizeT(value+0.5);
    p=FindMVGPopKeyword(&q,extent,token,"pattern");
    if (IsMVGBlockEndOutside(q,p,extent) != MagickFalse)
      {
        status=MagickFalse;
        break;
      }
    (void) CopyMagickString(token,p,(size_t) (q-p-4+1));
    (void) FormatLocaleString(key,MagickPathExtent,"%s",name);
    (void) SetImageArtifact(image,key,token);
    (void) FormatLocaleString(key,MagickPathExtent,"%s-geometry",
      name);
    (void) FormatLocaleString(geometry,MagickPathExtent,
      "%.17gx%.17g%+.20g%+.20g",(double) region.width,(double)
      region.height,(double) region.x,(double) region.y);
    (void) SetImageArtifact(image,key,geometry);
    (void) GetNextToken(q,&q,extent,token);
    break;
  } while (0);
  (*state->p)=p;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGPushSymbol(const MVGRenderState *state)
{
  DrawInfo
    **graphic_context = (*state->graphic_context);

  ssize_t
    n = (*state->n),
    symbolDepth = (*state->symbolDepth);

  symbolDepth++;
  graphic_context[n]->render=symbolDepth > 0 ? MagickFalse :
    MagickTrue;
  (*state->symbolDepth)=symbolDepth;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGAffine(const MVGRenderState *state)
{
  AffineMatrix
    affine = (*state->affine);

  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    affine.sx=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    if (GetMVGListValue(&q,extent,token,&affine.ry) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (GetMVGListValue(&q,extent,token,&affine.rx) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (GetMVGListValue(&q,extent,token,&affine.sy) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (GetMVGListValue(&q,extent,token,&affine.tx) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    if (GetMVGListValue(&q,extent,token,&affine.ty) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->affine)=affine;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGBorderColor(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  status&=(MagickStatusType) QueryColorCompliance(token,AllCompliance,
    &graphic_context[n]->border_color,exception);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static char *InjectMVGClassElements(char *primitive,const char *p,
  const char *mvg_class,const char **q)
{
  char
    *elements;

  ssize_t
    offset;

  offset=(ssize_t) (p-primitive);
  elements=AcquireString(primitive);
  elements[offset]='\0';
  (void) ConcatenateString(&elements,mvg_class);
  (void) ConcatenateString(&elements,"\n");
  (void) ConcatenateString(&elements,(*q));
  primitive=DestroyString(primitive);
  primitive=elements;
  (*q)=primitive+offset;
  return(primitive);
}

static MagickBooleanType IsNewGraphicContextId(DrawInfo **graphic_context,
  const ssize_t n,const char *token)
{
  ssize_t
    i;

  for (i=0; i <= n; i++)
    if (LocaleCompare(token,graphic_context[i]->id) == 0)
      return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType ParseMVGClass(const MVGRenderState *state)
{
  char
    *primitive = (*state->primitive),
    *token = (*state->token);

  const char
    *p = (*state->p),
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    classDepth = (*state->classDepth),
    n = (*state->n);

  do
  {
    const char
      *mvg_class;

    (void) GetNextToken(q,&q,extent,token);
    if ((*token == '\0') || (*token == ';'))
      {
        status=MagickFalse;
        break;
      }
    /*
      Identify recursion.
    */
    if (IsNewGraphicContextId(graphic_context,n,token) == MagickFalse)
      break;
    if (classDepth++ >= MagickMaxRecursionDepth)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          DrawError,"VectorGraphicsNestedTooDeeply","`%s'",token);
        status=MagickFalse;
        break;
      }
    mvg_class=(const char *) GetValueFromSplayTree(macros,token);
    {
      const RenderableClassInput
        renderable = { graphic_context, n, mvg_class, p, primitive };

      if (IsRenderableClass(&renderable) != MagickFalse)
        {
          /*
            Inject class elements in stream.
          */
          (void) CloneString(&graphic_context[n]->id,token);
          primitive=InjectMVGClassElements(primitive,p,mvg_class,&q);
        }
    }
    break;
  } while (0);
  (*state->primitive)=primitive;
  (*state->q)=q;
  (*state->status)=status;
  (*state->classDepth)=classDepth;
  return(MagickTrue);
}

static void DrawMVGClipPathNode(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  const char
    *clip_path;

  clip_path=(const char *) GetValueFromSplayTree(macros,token);
  if (clip_path != (const char *) NULL)
    {
      ClippingMaskPath
        clip_input;

      if (graphic_context[n]->clipping_mask != (Image *) NULL)
        graphic_context[n]->clipping_mask=
          DestroyImage(graphic_context[n]->clipping_mask);
      clip_input.id=token;
      clip_input.clip_path=clip_path;
      graphic_context[n]->clipping_mask=DrawClippingMask(image,
        graphic_context[n],&clip_input,exception);
      if (graphic_context[n]->compliance != SVGCompliance)
        {
          ApplyMVGClipPath(state);
        }
    }
}

static MagickBooleanType ParseMVGClipPath(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  const MVGRenderState
    arm_state = { state->affine, state->current, state->keyword,
      state->primitive, &token, state->p, &q, state->coordinates,
      state->cursor, &graphic_context, state->draw_info, state->exception,
      state->image, &status, state->mvg_info, state->primitive_info,
      state->primitive_type, state->depth, &extent, state->number_points,
      state->number_stops, &macros, state->classDepth, state->defsDepth,
      state->i, state->j, &n, state->symbolDepth, state->x, state->stops };

  do
  {
    /*
      Take a node from within the MVG document, and duplicate it here.
    */
    (void) GetNextToken(q,&q,extent,token);
    if (*token == '\0')
      {
        status=MagickFalse;
        break;
      }
    (void) CloneString(&graphic_context[n]->clip_mask,token);
    DrawMVGClipPathNode(&arm_state);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGClipRule(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      fill_rule;

    (void) GetNextToken(q,&q,extent,token);
    fill_rule=ParseCommandOption(MagickFillRuleOptions,MagickFalse,
      token);
    if (fill_rule == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->fill_rule=(FillRule) fill_rule;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGClipUnits(const MVGRenderState *state)
{
  AffineMatrix
    affine = (*state->affine),
    current = (*state->current);

  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  const DrawInfo
    *draw_info = state->draw_info;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      clip_units;

    (void) GetNextToken(q,&q,extent,token);
    clip_units=ParseCommandOption(MagickClipPathOptions,MagickFalse,
      token);
    if (clip_units == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->clip_units=(ClipPathUnits) clip_units;
    if (clip_units == ObjectBoundingBox)
      {
        GetAffineMatrix(&current);
        affine.sx=draw_info->bounds.x2;
        affine.sy=draw_info->bounds.y2;
        affine.tx=draw_info->bounds.x1;
        affine.ty=draw_info->bounds.y1;
        break;
      }
    break;
  } while (0);
  (*state->affine)=affine;
  (*state->current)=current;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGCompliance(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  /*
    MVG compliance associates a clipping mask with an image; SVG
    compliance associates a clipping mask with a graphics context.
  */
  (void) GetNextToken(q,&q,extent,token);
  graphic_context[n]->compliance=(ComplianceType) ParseCommandOption(
    MagickComplianceOptions,MagickFalse,token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGCurrentColor(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  size_t
    extent = (*state->extent);

  (void) GetNextToken(q,&q,extent,token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGDecorate(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      decorate;

    (void) GetNextToken(q,&q,extent,token);
    decorate=ParseCommandOption(MagickDecorateOptions,MagickFalse,
      token);
    if (decorate == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->decorate=(DecorationType) decorate;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGDensity(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  (void) CloneString(&graphic_context[n]->density,token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGDirection(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  ssize_t
    direction;

  (void) GetNextToken(q,&q,extent,token);
  direction=ParseCommandOption(MagickDirectionOptions,MagickFalse,
    token);
  if (direction == -1)
    status=MagickFalse;
  else
    graphic_context[n]->direction=(DirectionType) direction;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGEncoding(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  (void) CloneString(&graphic_context[n]->encoding,token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFill(const MVGRenderState *state)
{
  char
    pattern[MagickPathExtent],
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  const DrawInfo
    *draw_info = state->draw_info;

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  do
  {
    const char
      *mvg_class;

    (void) GetNextToken(q,&q,extent,token);
    if (graphic_context[n]->clip_path != MagickFalse)
      break;
    mvg_class=(const char *) GetValueFromSplayTree(macros,token);
    if (mvg_class != (const char *) NULL)
      {
        (void) DrawPatternPath(image,draw_info,mvg_class,
          &graphic_context[n]->fill_pattern,exception);
        break;
      }
    (void) FormatLocaleString(pattern,MagickPathExtent,"%s",token);
    if (GetImageArtifact(image,pattern) != (const char *) NULL)
      {
        (void) DrawPatternPath(image,draw_info,token,
          &graphic_context[n]->fill_pattern,exception);
        break;
      }
    status&=(MagickStatusType) QueryColorCompliance(token,AllCompliance,
      &graphic_context[n]->fill,exception);
    if (graphic_context[n]->fill_alpha != (double) OpaqueAlpha)
      graphic_context[n]->fill.alpha=graphic_context[n]->fill_alpha;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFillOpacity(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    factor;

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    double
      opacity;

    (void) GetNextToken(q,&q,extent,token);
    if (graphic_context[n]->clip_path != MagickFalse)
      break;
    factor=strchr(token,'%') != (char *) NULL ? 0.01 : 1.0;
    opacity=MagickMin(MagickMax(factor*
      GetDrawValue(token,&next_token),0.0),1.0);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    if (graphic_context[n]->compliance == SVGCompliance)
      graphic_context[n]->fill_alpha*=opacity;
    else
      graphic_context[n]->fill_alpha=(double) QuantumRange*opacity;
    if (graphic_context[n]->fill.alpha != (double) TransparentAlpha)
      graphic_context[n]->fill.alpha=graphic_context[n]->fill_alpha;
    else
      graphic_context[n]->fill.alpha=(MagickRealType)
        ClampToQuantum((double) QuantumRange*opacity);
    graphic_context[n]->fill.alpha_trait=BlendPixelTrait;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFillRule(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      fill_rule;

    (void) GetNextToken(q,&q,extent,token);
    fill_rule=ParseCommandOption(MagickFillRuleOptions,MagickFalse,
      token);
    if (fill_rule == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->fill_rule=(FillRule) fill_rule;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFont(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  (void) CloneString(&graphic_context[n]->font,token);
  if (LocaleCompare("none",token) == 0)
    graphic_context[n]->font=(char *) RelinquishMagickMemory(
      graphic_context[n]->font);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFontFamily(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  (void) CloneString(&graphic_context[n]->family,token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFontSize(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    graphic_context[n]->pointsize=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFontStretch(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      stretch;

    (void) GetNextToken(q,&q,extent,token);
    stretch=ParseCommandOption(MagickStretchOptions,MagickFalse,token);
    if (stretch == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->stretch=(StretchType) stretch;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFontStyle(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      style;

    (void) GetNextToken(q,&q,extent,token);
    style=ParseCommandOption(MagickStyleOptions,MagickFalse,token);
    if (style == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->style=(StyleType) style;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGFontWeight(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  ssize_t
    weight;

  (void) GetNextToken(q,&q,extent,token);
  weight=ParseCommandOption(MagickWeightOptions,MagickFalse,token);
  if (weight == -1)
    weight=(ssize_t) StringToUnsignedLong(token);
  graphic_context[n]->weight=(size_t) weight;
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGGradientUnits(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  size_t
    extent = (*state->extent);

  (void) GetNextToken(q,&q,extent,token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGGravity(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      gravity;

    (void) GetNextToken(q,&q,extent,token);
    gravity=ParseCommandOption(MagickGravityOptions,MagickFalse,token);
    if (gravity == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->gravity=(GravityType) gravity;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGImage(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  PrimitiveType
    primitive_type = (*state->primitive_type);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      compose;

    primitive_type=ImagePrimitive;
    (void) GetNextToken(q,&q,extent,token);
    compose=ParseCommandOption(MagickComposeOptions,MagickFalse,token);
    if (compose == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->compose=(CompositeOperator) compose;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  (*state->primitive_type)=primitive_type;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGInterlineSpacing(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    graphic_context[n]->interline_spacing=GetDrawValue(token,
      &next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGInterwordSpacing(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    graphic_context[n]->interword_spacing=GetDrawValue(token,
      &next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGKerning(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    graphic_context[n]->kerning=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGLetterSpacing(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    *clone_info,
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  TypeMetric
    metrics;

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    if (IsValidPoint(token) == MagickFalse)
      break;
    clone_info=CloneDrawInfo((ImageInfo *) NULL,graphic_context[n]);
    clone_info->text=AcquireString(" ");
    status&=(MagickStatusType) GetTypeMetrics(image,clone_info,&metrics,
      exception);
    graphic_context[n]->kerning=metrics.width*
      GetDrawValue(token,&next_token);
    clone_info=DestroyDrawInfo(clone_info);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGMask(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  const char
    *mask_path;

  /*
    Take a node from within the MVG document, and duplicate it here.
  */
  (void) GetNextToken(q,&q,extent,token);
  mask_path=(const char *) GetValueFromSplayTree(macros,token);
  if (mask_path != (const char *) NULL)
    {
      CompositeMaskPath
        mask_input;

      if (graphic_context[n]->composite_mask != (Image *) NULL)
        graphic_context[n]->composite_mask=
          DestroyImage(graphic_context[n]->composite_mask);
      mask_input.id=token;
      mask_input.mask_path=mask_path;
      graphic_context[n]->composite_mask=DrawCompositeMask(image,
        graphic_context[n],&mask_input,exception);
      if (graphic_context[n]->compliance != SVGCompliance)
        status=SetImageMask(image,CompositePixelMask,
          graphic_context[n]->composite_mask,exception);
    }
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGOffset(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  size_t
    extent = (*state->extent);

  (void) GetNextToken(q,&q,extent,token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGOpacity(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    factor;

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    double
      opacity;

    (void) GetNextToken(q,&q,extent,token);
    if (graphic_context[n]->clip_path != MagickFalse)
      break;
    factor=strchr(token,'%') != (char *) NULL ? 0.01 : 1.0;
    opacity=MagickMin(MagickMax(factor*
      GetDrawValue(token,&next_token),0.0),1.0);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    if (graphic_context[n]->compliance == SVGCompliance)
      {
        graphic_context[n]->fill_alpha*=opacity;
        graphic_context[n]->stroke_alpha*=opacity;
      }
    else
      {
        graphic_context[n]->fill_alpha=(double) QuantumRange*opacity;
        graphic_context[n]->stroke_alpha=(double) QuantumRange*opacity;
      }
    if (graphic_context[n]->fill.alpha != (double) TransparentAlpha)
      {
        graphic_context[n]->fill.alpha=graphic_context[n]->fill_alpha;
        graphic_context[n]->stroke.alpha=
          graphic_context[n]->stroke_alpha;
      }
    else
      {
        graphic_context[n]->fill.alpha=(MagickRealType)
          ClampToQuantum((double) QuantumRange*opacity);
        graphic_context[n]->stroke.alpha=(MagickRealType)
          ClampToQuantum((double) QuantumRange*opacity);
      }
    graphic_context[n]->fill.alpha_trait=BlendPixelTrait;
    graphic_context[n]->stroke.alpha_trait=BlendPixelTrait;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGRotate(const MVGRenderState *state)
{
  AffineMatrix
    affine = (*state->affine);

  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    angle;

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    angle=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    affine.sx=cos(DegreesToRadians(fmod((double) angle,360.0)));
    affine.rx=sin(DegreesToRadians(fmod((double) angle,360.0)));
    affine.ry=(-sin(DegreesToRadians(fmod((double) angle,360.0))));
    affine.sy=cos(DegreesToRadians(fmod((double) angle,360.0)));
    break;
  } while (0);
  (*state->affine)=affine;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGScale(const MVGRenderState *state)
{
  AffineMatrix
    affine = (*state->affine);

  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    affine.sx=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    if (GetMVGListValue(&q,extent,token,&affine.sy) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->affine)=affine;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGSkewX(const MVGRenderState *state)
{
  AffineMatrix
    affine = (*state->affine);

  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    angle;

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    angle=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    affine.ry=sin(DegreesToRadians(angle));
    break;
  } while (0);
  (*state->affine)=affine;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGSkewY(const MVGRenderState *state)
{
  AffineMatrix
    affine = (*state->affine);

  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    angle;

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    angle=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    affine.rx=(-tan(DegreesToRadians(angle)/2.0));
    break;
  } while (0);
  (*state->affine)=affine;
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStopColor(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    factor;

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent),
    number_stops = (*state->number_stops);

  StopInfo
    *stops = (*state->stops);

  do
  {
    PixelInfo
      stop_color;

    number_stops++;
    if (number_stops == 1)
      stops=(StopInfo *) AcquireQuantumMemory(2,sizeof(*stops));
    else
      if (number_stops > 2)
        stops=(StopInfo *) ResizeQuantumMemory(stops,number_stops,
          sizeof(*stops));
    if (stops == (StopInfo *) NULL)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'",
          image->filename);
        status=MagickFalse;
        break;
      }
    (void) GetNextToken(q,&q,extent,token);
    status&=(MagickStatusType) QueryColorCompliance(token,AllCompliance,
      &stop_color,exception);
    stops[number_stops-1].color=stop_color;
    (void) GetNextToken(q,&q,extent,token);
    factor=strchr(token,'%') != (char *) NULL ? 0.01 : 1.0;
    stops[number_stops-1].offset=factor*GetDrawValue(token,
      &next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  (*state->number_stops)=number_stops;
  (*state->stops)=stops;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStroke(const MVGRenderState *state)
{
  char
    pattern[MagickPathExtent],
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  const DrawInfo
    *draw_info = state->draw_info;

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  do
  {
    const char
      *mvg_class;

    (void) GetNextToken(q,&q,extent,token);
    if (graphic_context[n]->clip_path != MagickFalse)
      break;
    mvg_class=(const char *) GetValueFromSplayTree(macros,token);
    if (mvg_class != (const char *) NULL)
      {
        (void) DrawPatternPath(image,draw_info,mvg_class,
          &graphic_context[n]->stroke_pattern,exception);
        break;
      }
    (void) FormatLocaleString(pattern,MagickPathExtent,"%s",token);
    if (GetImageArtifact(image,pattern) != (const char *) NULL)
      {
        (void) DrawPatternPath(image,draw_info,token,
          &graphic_context[n]->stroke_pattern,exception);
        break;
      }
    status&=(MagickStatusType) QueryColorCompliance(token,AllCompliance,
      &graphic_context[n]->stroke,exception);
    if (graphic_context[n]->stroke_alpha != (double) OpaqueAlpha)
      graphic_context[n]->stroke.alpha=
        graphic_context[n]->stroke_alpha;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStrokeAntialias(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  graphic_context[n]->stroke_antialias=StringToLong(token) != 0 ?
    MagickTrue : MagickFalse;
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStrokeDasharray(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n),
    x;

  const MVGRenderState
    arm_state = { state->affine, state->current, state->keyword,
      state->primitive, &token, state->p, &q, state->coordinates,
      state->cursor, &graphic_context, state->draw_info, state->exception,
      state->image, &status, state->mvg_info, state->primitive_info,
      state->primitive_type, state->depth, &extent, state->number_points,
      state->number_stops, state->macros, state->classDepth, state->defsDepth,
      state->i, state->j, &n, state->symbolDepth, &x, state->stops };

  do
  {
    graphic_context[n]->dash_pattern=(double *)
      RelinquishMagickMemory(graphic_context[n]->dash_pattern);
    if (IsValidPoint(q) != MagickFalse)
      {
        const char
          *r;

        r=q;
        (void) GetNextToken(r,&r,extent,token);
        if (IsValidListChar((int) ((unsigned char) *token)) == MagickFalse)
          ThrowPointExpectedException(token,exception);
        if (*token == ',')
          (void) GetNextToken(r,&r,extent,token);
        x=CountMVGDashValues(r,extent,token);
        graphic_context[n]->dash_pattern=(double *)
          AcquireQuantumMemory((size_t) (2*x+2),
          sizeof(*graphic_context[n]->dash_pattern));
        if (graphic_context[n]->dash_pattern == (double *) NULL)
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","`%s'",
              image->filename);
            status=MagickFalse;
            break;
          }
        (void) memset(graphic_context[n]->dash_pattern,0,(size_t)
          (2*x+2)*sizeof(*graphic_context[n]->dash_pattern));
        ParseMVGDashPattern(&arm_state);
        break;
      }
    (void) GetNextToken(q,&q,extent,token);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStrokeDashoffset(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    graphic_context[n]->dash_offset=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStrokeLinecap(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      linecap;

    (void) GetNextToken(q,&q,extent,token);
    linecap=ParseCommandOption(MagickLineCapOptions,MagickFalse,token);
    if (linecap == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->linecap=(LineCap) linecap;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStrokeLinejoin(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      linejoin;

    (void) GetNextToken(q,&q,extent,token);
    linejoin=ParseCommandOption(MagickLineJoinOptions,MagickFalse,
      token);
    if (linejoin == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->linejoin=(LineJoin) linejoin;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStrokeMiterlimit(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  graphic_context[n]->miterlimit=StringToUnsignedLong(token);
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStrokeOpacity(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    factor;

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    double
      opacity;

    (void) GetNextToken(q,&q,extent,token);
    if (graphic_context[n]->clip_path != MagickFalse)
      break;
    factor=strchr(token,'%') != (char *) NULL ? 0.01 : 1.0;
    opacity=MagickMin(MagickMax(factor*GetDrawValue(token,&next_token),
      0.0),1.0);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    if (graphic_context[n]->compliance == SVGCompliance)
      graphic_context[n]->stroke_alpha*=opacity;
    else
      graphic_context[n]->stroke_alpha=(double) QuantumRange*opacity;
    if (graphic_context[n]->stroke.alpha != (double) TransparentAlpha)
      graphic_context[n]->stroke.alpha=graphic_context[n]->stroke_alpha;
    else
      graphic_context[n]->stroke.alpha=(MagickRealType)
        ClampToQuantum((double) QuantumRange*opacity);
    graphic_context[n]->stroke.alpha_trait=BlendPixelTrait;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGStrokeWidth(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    if (graphic_context[n]->clip_path != MagickFalse)
      break;
    graphic_context[n]->stroke_width=GetDrawValue(token,&next_token);
    if ((token == next_token) ||
        (graphic_context[n]->stroke_width < 0.0))
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGText(const MVGRenderState *state)
{
  double
    cursor = (*state->cursor);

  PrimitiveType
    primitive_type = (*state->primitive_type);

  primitive_type=TextPrimitive;
  cursor=0.0;
  (*state->cursor)=cursor;
  (*state->primitive_type)=primitive_type;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGTextAlign(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      align;

    (void) GetNextToken(q,&q,extent,token);
    align=ParseCommandOption(MagickAlignOptions,MagickFalse,token);
    if (align == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->align=(AlignType) align;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGTextAnchor(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    ssize_t
      align;

    (void) GetNextToken(q,&q,extent,token);
    align=ParseCommandOption(MagickAlignOptions,MagickFalse,token);
    if (align == -1)
      {
        status=MagickFalse;
        break;
      }
    graphic_context[n]->align=(AlignType) align;
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGTextAntialias(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  graphic_context[n]->text_antialias=StringToLong(token) != 0 ?
    MagickTrue : MagickFalse;
  (*state->q)=q;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGTextUndercolor(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  (void) GetNextToken(q,&q,extent,token);
  status&=(MagickStatusType) QueryColorCompliance(token,AllCompliance,
    &graphic_context[n]->undercolor,exception);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGTranslate(const MVGRenderState *state)
{
  AffineMatrix
    affine = (*state->affine);

  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  double
    cursor = (*state->cursor);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    affine.tx=GetDrawValue(token,&next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    if (GetMVGListValue(&q,extent,token,&affine.ty) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    cursor=0.0;
    break;
  } while (0);
  (*state->affine)=affine;
  (*state->q)=q;
  (*state->cursor)=cursor;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGUse(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    *clone_info,
    **graphic_context = (*state->graphic_context);

  const DrawInfo
    *draw_info = state->draw_info;

  ExceptionInfo
    *exception = state->exception;

  Image
    *image = state->image;

  MagickStatusType
    status = (*state->status);

  size_t
    depth = state->depth,
    extent = (*state->extent);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  const char
    *use;

  /*
    Get a macro from the MVG document, and "use" it here.
  */
  (void) GetNextToken(q,&q,extent,token);
  use=(const char *) GetValueFromSplayTree(macros,token);
  if (use != (const char *) NULL)
    {
      clone_info=CloneDrawInfo((ImageInfo *) NULL,graphic_context[n]);
      if (draw_info->macro_expansion != (size_t *) NULL)
        {
          if (*draw_info->macro_expansion >= MacroExpansionLimit)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                DrawError,"NonconformingDrawingPrimitiveDefinition",
                "`%s'",token);
              clone_info=DestroyDrawInfo(clone_info);
              return(MagickFalse);
            }
          (*draw_info->macro_expansion)++;
        }
      (void) CloneString(&clone_info->primitive,use);
      status=RenderMVGContent(image,clone_info,depth+1,exception);
      clone_info=DestroyDrawInfo(clone_info);
    }
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGViewbox(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    double
      value;

    if (GetMVGValue(&q,extent,token,&value) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    graphic_context[n]->viewbox.x=CastDoubleToSsizeT(ceil(value-0.5));
    if (GetMVGListValue(&q,extent,token,&value) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    graphic_context[n]->viewbox.y=CastDoubleToSsizeT(ceil(value-0.5));
    if (GetMVGListValue(&q,extent,token,&value) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    graphic_context[n]->viewbox.width=CastDoubleToSizeT(floor(value+0.5));
    if (GetMVGListValue(&q,extent,token,&value) == MagickFalse)
      ThrowPointExpectedException(token,exception);
    graphic_context[n]->viewbox.height=CastDoubleToSizeT(floor(value+0.5));
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static MagickBooleanType ParseMVGWordSpacing(const MVGRenderState *state)
{
  char
    *next_token,
    *token = (*state->token);

  const char
    *q = (*state->q);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  ExceptionInfo
    *exception = state->exception;

  MagickStatusType
    status = (*state->status);

  size_t
    extent = (*state->extent);

  ssize_t
    n = (*state->n);

  do
  {
    (void) GetNextToken(q,&q,extent,token);
    graphic_context[n]->interword_spacing=GetDrawValue(token,
      &next_token);
    if (token == next_token)
      ThrowPointExpectedException(token,exception);
    break;
  } while (0);
  (*state->q)=q;
  (*state->status)=status;
  return(MagickTrue);
}

static const MVGKeywordInfo
  MVGPopKeywords[] =
  {
    { "class", UndefinedPrimitive, (ParseMVGKeywordMethod) NULL },
    { "clip-path", UndefinedPrimitive, (ParseMVGKeywordMethod) NULL },
    { "defs", UndefinedPrimitive, ParseMVGPopDefs },
    { "gradient", UndefinedPrimitive, (ParseMVGKeywordMethod) NULL },
    { "graphic-context", UndefinedPrimitive, ParseMVGPopGraphicContext },
    { "mask", UndefinedPrimitive, (ParseMVGKeywordMethod) NULL },
    { "pattern", UndefinedPrimitive, (ParseMVGKeywordMethod) NULL },
    { "symbol", UndefinedPrimitive, ParseMVGPopSymbol }
  };

static const MVGKeywordInfo
  MVGPushKeywords[] =
  {
    { "class", UndefinedPrimitive, ParseMVGPushClass },
    { "clip-path", UndefinedPrimitive, ParseMVGPushClipPath },
    { "defs", UndefinedPrimitive, ParseMVGPushDefs },
    { "gradient", UndefinedPrimitive, ParseMVGPushGradient },
    { "graphic-context", UndefinedPrimitive, ParseMVGPushGraphicContext },
    { "mask", UndefinedPrimitive, ParseMVGPushMask },
    { "pattern", UndefinedPrimitive, ParseMVGPushPattern },
    { "symbol", UndefinedPrimitive, ParseMVGPushSymbol }
  };

static MagickBooleanType ParseMVGPop(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  const MVGKeywordInfo
    *keyword_info;

  size_t
    extent = (*state->extent);

  if (GetNextToken(q,&q,extent,token) < 1)
    {
      (*state->q)=q;
      return(MagickTrue);
    }
  (*state->q)=q;
  keyword_info=GetMVGKeywordInfo(MVGPopKeywords,sizeof(MVGPopKeywords)/
    sizeof(*MVGPopKeywords),token);
  if (keyword_info == (const MVGKeywordInfo *) NULL)
    {
      (*state->status)=MagickFalse;
      return(MagickTrue);
    }
  if (keyword_info->parse == (ParseMVGKeywordMethod) NULL)
    return(MagickTrue);
  return(keyword_info->parse(state));
}

static MagickBooleanType ParseMVGPush(const MVGRenderState *state)
{
  char
    *token = (*state->token);

  const char
    *q = (*state->q);

  const MVGKeywordInfo
    *keyword_info;

  size_t
    extent = (*state->extent);

  if (GetNextToken(q,&q,extent,token) < 1)
    {
      (*state->q)=q;
      return(MagickTrue);
    }
  (*state->q)=q;
  keyword_info=GetMVGKeywordInfo(MVGPushKeywords,sizeof(MVGPushKeywords)/
    sizeof(*MVGPushKeywords),token);
  if (keyword_info == (const MVGKeywordInfo *) NULL)
    {
      (*state->status)=MagickFalse;
      return(MagickTrue);
    }
  if (keyword_info->parse == (ParseMVGKeywordMethod) NULL)
    return(MagickTrue);
  return(keyword_info->parse(state));
}

static const MVGKeywordInfo
  MVGKeywords[] =
  {
    { "affine", UndefinedPrimitive, ParseMVGAffine },
    { "alpha", AlphaPrimitive, (ParseMVGKeywordMethod) NULL },
    { "arc", ArcPrimitive, (ParseMVGKeywordMethod) NULL },
    { "bezier", BezierPrimitive, (ParseMVGKeywordMethod) NULL },
    { "border-color", UndefinedPrimitive, ParseMVGBorderColor },
    { "class", UndefinedPrimitive, ParseMVGClass },
    { "clip-path", UndefinedPrimitive, ParseMVGClipPath },
    { "clip-rule", UndefinedPrimitive, ParseMVGClipRule },
    { "clip-units", UndefinedPrimitive, ParseMVGClipUnits },
    { "circle", CirclePrimitive, (ParseMVGKeywordMethod) NULL },
    { "color", ColorPrimitive, (ParseMVGKeywordMethod) NULL },
    { "compliance", UndefinedPrimitive, ParseMVGCompliance },
    { "currentColor", UndefinedPrimitive, ParseMVGCurrentColor },
    { "decorate", UndefinedPrimitive, ParseMVGDecorate },
    { "density", UndefinedPrimitive, ParseMVGDensity },
    { "direction", UndefinedPrimitive, ParseMVGDirection },
    { "ellipse", EllipsePrimitive, (ParseMVGKeywordMethod) NULL },
    { "encoding", UndefinedPrimitive, ParseMVGEncoding },
    { "fill", UndefinedPrimitive, ParseMVGFill },
    { "fill-opacity", UndefinedPrimitive, ParseMVGFillOpacity },
    { "fill-rule", UndefinedPrimitive, ParseMVGFillRule },
    { "font", UndefinedPrimitive, ParseMVGFont },
    { "font-family", UndefinedPrimitive, ParseMVGFontFamily },
    { "font-size", UndefinedPrimitive, ParseMVGFontSize },
    { "font-stretch", UndefinedPrimitive, ParseMVGFontStretch },
    { "font-style", UndefinedPrimitive, ParseMVGFontStyle },
    { "font-weight", UndefinedPrimitive, ParseMVGFontWeight },
    { "gradient-units", UndefinedPrimitive, ParseMVGGradientUnits },
    { "gravity", UndefinedPrimitive, ParseMVGGravity },
    { "image", UndefinedPrimitive, ParseMVGImage },
    { "interline-spacing", UndefinedPrimitive, ParseMVGInterlineSpacing },
    { "interword-spacing", UndefinedPrimitive, ParseMVGInterwordSpacing },
    { "kerning", UndefinedPrimitive, ParseMVGKerning },
    { "letter-spacing", UndefinedPrimitive, ParseMVGLetterSpacing },
    { "line", LinePrimitive, (ParseMVGKeywordMethod) NULL },
    { "mask", UndefinedPrimitive, ParseMVGMask },
    { "offset", UndefinedPrimitive, ParseMVGOffset },
    { "opacity", UndefinedPrimitive, ParseMVGOpacity },
    { "path", PathPrimitive, (ParseMVGKeywordMethod) NULL },
    { "point", PointPrimitive, (ParseMVGKeywordMethod) NULL },
    { "polyline", PolylinePrimitive, (ParseMVGKeywordMethod) NULL },
    { "polygon", PolygonPrimitive, (ParseMVGKeywordMethod) NULL },
    { "pop", UndefinedPrimitive, ParseMVGPop },
    { "push", UndefinedPrimitive, ParseMVGPush },
    { "rectangle", RectanglePrimitive, (ParseMVGKeywordMethod) NULL },
    { "rotate", UndefinedPrimitive, ParseMVGRotate },
    { "roundRectangle", RoundRectanglePrimitive, (ParseMVGKeywordMethod) NULL },
    { "scale", UndefinedPrimitive, ParseMVGScale },
    { "skewX", UndefinedPrimitive, ParseMVGSkewX },
    { "skewY", UndefinedPrimitive, ParseMVGSkewY },
    { "stop-color", UndefinedPrimitive, ParseMVGStopColor },
    { "stroke", UndefinedPrimitive, ParseMVGStroke },
    { "stroke-antialias", UndefinedPrimitive, ParseMVGStrokeAntialias },
    { "stroke-dasharray", UndefinedPrimitive, ParseMVGStrokeDasharray },
    { "stroke-dashoffset", UndefinedPrimitive, ParseMVGStrokeDashoffset },
    { "stroke-linecap", UndefinedPrimitive, ParseMVGStrokeLinecap },
    { "stroke-linejoin", UndefinedPrimitive, ParseMVGStrokeLinejoin },
    { "stroke-miterlimit", UndefinedPrimitive, ParseMVGStrokeMiterlimit },
    { "stroke-opacity", UndefinedPrimitive, ParseMVGStrokeOpacity },
    { "stroke-width", UndefinedPrimitive, ParseMVGStrokeWidth },
    { "text", UndefinedPrimitive, ParseMVGText },
    { "text-align", UndefinedPrimitive, ParseMVGTextAlign },
    { "text-anchor", UndefinedPrimitive, ParseMVGTextAnchor },
    { "text-antialias", UndefinedPrimitive, ParseMVGTextAntialias },
    { "text-undercolor", UndefinedPrimitive, ParseMVGTextUndercolor },
    { "translate", UndefinedPrimitive, ParseMVGTranslate },
    { "use", UndefinedPrimitive, ParseMVGUse },
    { "viewbox", UndefinedPrimitive, ParseMVGViewbox },
    { "word-spacing", UndefinedPrimitive, ParseMVGWordSpacing }
  };

static MagickBooleanType ParseMVGKeyword(const MVGRenderState *state)
{
  const MVGKeywordInfo
    *keyword_info;

  keyword_info=GetMVGKeywordInfo(MVGKeywords,sizeof(MVGKeywords)/
    sizeof(*MVGKeywords),state->keyword);
  if (keyword_info == (const MVGKeywordInfo *) NULL)
    {
      if (*state->keyword != ';')
        (*state->status)=MagickFalse;
      return(MagickTrue);
    }
  if (keyword_info->primitive != UndefinedPrimitive)
    (*state->primitive_type)=keyword_info->primitive;
  if (keyword_info->parse == (ParseMVGKeywordMethod) NULL)
    return(MagickTrue);
  return(keyword_info->parse(state));
}

static MagickBooleanType SetMVGImageOpaque(Image *image,
  ExceptionInfo *exception)
{
  MagickStatusType
    status;

  if ((image->alpha_trait & BlendPixelTrait) == 0)
    {
      status=SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
      if (status == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static DrawInfo **DestroyMVGGraphicContexts(DrawInfo **graphic_context,
  ssize_t n)
{
  for ( ; n >= 0; n--)
    graphic_context[n]=DestroyDrawInfo(graphic_context[n]);
  graphic_context=(DrawInfo **) RelinquishMagickMemory(graphic_context);
  return(graphic_context);
}

static const char *SkipMVGComment(const char *q)
{
  while ((*q != '\n') && (*q != '\0'))
    q++;
  return(q);
}

static void PrepareMVGPrimitive(const MVGRenderState *state)
{
  MagickStatusType
    *status = state->status;

  do
  {
    ParseMVGPrimitivePoints(state);
    if ((*status) == MagickFalse)
      break;
    EstimateMVGPrimitiveExtent(state);
    if ((*status) == MagickFalse)
      break;
    ReserveMVGPrimitiveExtent(state);
    if ((*status) == MagickFalse)
      break;
    TraceMVGPrimitive(state);
  } while (0);
}

static inline MagickBooleanType IsMVGRenderStopped(
  const MagickBooleanType proceed,const MagickStatusType status)
{
  if (proceed == MagickFalse)
    return(MagickTrue);
  if (status == 0)
    return(MagickTrue);
  return(MagickFalse);
}

static inline MagickBooleanType IsMVGDebugSpan(const DrawInfo *draw_info,
  const char *p,const char *q)
{
  return(((draw_info->debug != MagickFalse) && (q > p)) ? MagickTrue :
    MagickFalse);
}

static MagickBooleanType FinishMVGRender(const MagickStatusType status,
  const char *keyword,ExceptionInfo *exception)
{
  if ((status == MagickFalse) && (exception->severity < ErrorException))
    ThrowBinaryException(DrawError,"NonconformingDrawingPrimitiveDefinition",
      keyword);
  return(status != 0 ? MagickTrue : MagickFalse);
}

static char *AcquireMVGPrimitive(const DrawInfo *draw_info,const size_t depth,
  ExceptionInfo *exception)
{
  char
    *primitive;

  if (IsMVGFileReference(draw_info,depth) != MagickFalse)
    primitive=FileToString(draw_info->primitive,~0UL,exception);
  else
    primitive=AcquireString(draw_info->primitive);
  return(primitive);
}

static MagickBooleanType PrepareMVGImage(Image *image,
  ExceptionInfo *exception)
{
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if (SetMVGImageOpaque(image,exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType IsMVGRenderable(const Image *image,
  const DrawInfo *draw_info,const size_t depth,ExceptionInfo *exception)
{
  if (depth >= MagickMaxRecursionDepth)
    ThrowBinaryException(DrawError,"VectorGraphicsNestedTooDeeply",
      image->filename);
  if ((draw_info->primitive == (char *) NULL) ||
      (*draw_info->primitive == '\0'))
    return(MagickFalse);
  return(MagickTrue);
}

static void InitializeMVGGraphicContext(const MVGRenderState *state)
{
  const DrawInfo
    *draw_info = state->draw_info;

  DrawInfo
    **graphic_context = (*state->graphic_context);

  Image
    *image = state->image;

  ssize_t
    n = (*state->n);

  graphic_context[n]=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  graphic_context[n]->primitive=DestroyString(graphic_context[n]->primitive);
  graphic_context[n]->viewbox=image->page;
  if ((image->page.width == 0) || (image->page.height == 0))
    {
      graphic_context[n]->viewbox.width=image->columns;
      graphic_context[n]->viewbox.height=image->rows;
    }
}

static void ReleaseMVGRenderResources(const MVGRenderState *state)
{
  char
    *primitive = (*state->primitive),
    *token = (*state->token);

  DrawInfo
    **graphic_context = (*state->graphic_context);

  PrimitiveInfo
    *primitive_info = (*state->primitive_info);

  SplayTreeInfo
    *macros = (*state->macros);

  ssize_t
    n = (*state->n);

  StopInfo
    *stops = (*state->stops);

  macros=DestroySplayTree(macros);
  token=DestroyString(token);
  if (primitive_info != (PrimitiveInfo *) NULL)
    {
      DestroyMVGPrimitiveText(primitive_info);
      primitive_info=(PrimitiveInfo *) RelinquishMagickMemory(primitive_info);
    }
  primitive=DestroyString(primitive);
  stops=(StopInfo *) RelinquishMagickMemory(stops);
  graphic_context=DestroyMVGGraphicContexts(graphic_context,n);
  (*state->primitive)=primitive;
  (*state->token)=token;
  (*state->graphic_context)=graphic_context;
  (*state->primitive_info)=primitive_info;
  (*state->macros)=macros;
  (*state->stops)=stops;
}

static MagickBooleanType RenderMVGContent(Image *image,
  const DrawInfo *draw_info,const size_t depth,ExceptionInfo *exception)
{
#define RenderImageTag  "Render/Image"

  AffineMatrix
    affine,
    current;

  char
    keyword[MagickPathExtent],
    *primitive,
    *token;

  const char
    *p,
    *q;

  double
    coordinates,
    cursor,
    primitive_extent;

  DrawInfo
    **graphic_context;

  MagickBooleanType
    proceed;

  MagickStatusType
    status;

  MVGInfo
    mvg_info;

  PrimitiveInfo
    *primitive_info;

  PrimitiveType
    primitive_type;

  size_t
    extent,
    number_points,
    number_stops;

  SplayTreeInfo
    *macros;

  ssize_t
    classDepth = 0,
    defsDepth,
    i,
    j,
    n,
    symbolDepth,
    x;

  StopInfo
    *stops;

  const MVGRenderState
    render_state = { &affine, &current, keyword, &primitive, &token, &p, &q,
      &coordinates, &cursor, &graphic_context, draw_info, exception, image,
      &status, &mvg_info, &primitive_info, &primitive_type, depth, &extent,
      &number_points, &number_stops, &macros, &classDepth, &defsDepth, &i, &j,
      &n, &symbolDepth, &x, &stops };

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (IsMVGRenderable(image,draw_info,depth,exception) == MagickFalse)
    return(MagickFalse);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"begin draw-image");
  if (PrepareMVGImage(image,exception) == MagickFalse)
    return(MagickFalse);
  primitive=AcquireMVGPrimitive(draw_info,depth,exception);
  if (primitive == (char *) NULL)
    return(MagickFalse);
  primitive_extent=(double) strlen(primitive);
  (void) SetImageArtifact(image,"mvg:vector-graphics",primitive);
  n=0;
  number_stops=0;
  stops=(StopInfo *) NULL;
  /*
    Allocate primitive info memory.
  */
  graphic_context=(DrawInfo **) AcquireMagickMemory(sizeof(*graphic_context));
  if (graphic_context == (DrawInfo **) NULL)
    {
      primitive=DestroyString(primitive);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  number_points=(size_t) PrimitiveExtentPad;
  primitive_info=(PrimitiveInfo *) AcquireQuantumMemory((size_t)
    (number_points+1),sizeof(*primitive_info));
  if (primitive_info == (PrimitiveInfo *) NULL)
    {
      primitive=DestroyString(primitive);
      graphic_context=DestroyMVGGraphicContexts(graphic_context,n);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  (void) memset(primitive_info,0,(size_t) (number_points+1)*
    sizeof(*primitive_info));
  (void) memset(&mvg_info,0,sizeof(mvg_info));
  mvg_info.primitive_info=(&primitive_info);
  mvg_info.extent=(&number_points);
  mvg_info.exception=exception;
  InitializeMVGGraphicContext(&render_state);
  token=AcquireString(primitive);
  extent=strlen(token)+MagickPathExtent;
  defsDepth=0;
  symbolDepth=0;
  cursor=0.0;
  macros=GetMVGMacros(primitive,exception);
  status=MagickTrue;
  for (q=primitive; *q != '\0'; )
  {
    /*
      Interpret graphic primitive.
    */
    if (GetMVGMacroToken(&q,MagickPathExtent,keyword) == MagickFalse)
      break;
    if (*keyword == '#')
      {
        /*
          Comment.
        */
        q=SkipMVGComment(q);
        continue;
      }
    p=q-strlen(keyword)-1;
    primitive_type=UndefinedPrimitive;
    current=graphic_context[n]->affine;
    GetAffineMatrix(&affine);
    *token='\0';
    if (ParseMVGKeyword(&render_state) == MagickFalse)
      return(MagickFalse);
    if (status == MagickFalse)
      break;
    ApplyMVGAffine(&render_state);
    if (primitive_type == UndefinedPrimitive)
      {
        FinishMVGGradient(&render_state);
        if (IsMVGDebugSpan(draw_info,p,q) != MagickFalse)
          (void) LogMagickEvent(DrawEvent,GetMagickModule(),"  %.*s",(int)
            (q-p-1),p);
        continue;
      }
    /*
      Parse the primitive attributes.
    */
    PrepareMVGPrimitive(&render_state);
    if (status == 0)
      break;
    primitive_info[i].primitive=UndefinedPrimitive;
    if (IsMVGDebugSpan(draw_info,p,q) != MagickFalse)
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),"  %.*s",(int) (q-p),p);
    CheckMVGPrimitiveExtent(&render_state);
    if (status == 0)
      break;
    if (i == 0)
      continue;
    /*
      Transform points.
    */
    TransformMVGPrimitive(&render_state);
    DrawMVGPrimitive(&render_state);
    proceed=SetImageProgress(image,RenderImageTag,q-primitive,(MagickSizeType)
      primitive_extent);
    if (IsMVGRenderStopped(proceed,status) != MagickFalse)
      break;
  }
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"end draw-image");
  /*
    Relinquish resources.
  */
  ReleaseMVGRenderResources(&render_state);
  return(FinishMVGRender(status,keyword,exception));
}

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

