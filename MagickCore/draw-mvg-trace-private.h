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

  MagickCore private methods of draw.c: the MVG primitive tracing and the
  graphic-context pop keywords, TraceMVGPrimitive to ParseMVGPopSymbol;
  included by that file only.
*/
#ifndef MAGICKCORE_DRAW_MVG_TRACE_PRIVATE_H
#define MAGICKCORE_DRAW_MVG_TRACE_PRIVATE_H

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

#endif
