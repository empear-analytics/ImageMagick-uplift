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

  MagickCore private methods of draw.c: the stroke and text keyword parsers,
  ParseMVGStroke to ParseMVGWordSpacing, the keyword tables and
  ParseMVGKeyword; included by that file only.
*/
#ifndef MAGICKCORE_DRAW_MVG_KEYWORD_PRIVATE_H
#define MAGICKCORE_DRAW_MVG_KEYWORD_PRIVATE_H

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

#endif
