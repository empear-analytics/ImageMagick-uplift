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

  MagickCore private methods of draw.c: the fill, font, image and transform
  keyword parsers, ParseMVGFill to ParseMVGStopColor; included by that file
  only.
*/
#ifndef MAGICKCORE_DRAW_MVG_ATTRIBUTE_PRIVATE_H
#define MAGICKCORE_DRAW_MVG_ATTRIBUTE_PRIVATE_H

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

#endif
