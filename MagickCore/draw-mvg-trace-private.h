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
  graphic-context pop keywords, TraceMVGPrimitive to ParseMVGPopSymbol, with
  the clip masks, macro and extent helpers before them; included by that
  file only.
*/
#ifndef MAGICKCORE_DRAW_MVG_TRACE_PRIVATE_H
#define MAGICKCORE_DRAW_MVG_TRACE_PRIVATE_H

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

static const int
  ListSeparatorCharacters[] =
  {
    '.',
    '+',
    '-',
    ',',
    ' ',
    '\t',
    '\r',
    '\n'
  };

static inline MagickBooleanType IsValidListChar(int c)
{
  size_t
    i;

  if ((c >= '0') && (c <= '9'))
    return(MagickTrue);
  for (i=0; i < (sizeof(ListSeparatorCharacters)/
       sizeof(*ListSeparatorCharacters)); i++)
    if (ListSeparatorCharacters[i] == c)
      return(MagickTrue);
  return(MagickFalse);
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

#endif
