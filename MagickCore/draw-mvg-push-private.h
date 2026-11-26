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

  MagickCore private methods of draw.c: the graphic-context push keywords and
  the keyword parsers from ParseMVGPushClass to ParseMVGEncoding; included by
  that file only.
*/
#ifndef MAGICKCORE_DRAW_MVG_PUSH_PRIVATE_H
#define MAGICKCORE_DRAW_MVG_PUSH_PRIVATE_H

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

#endif
