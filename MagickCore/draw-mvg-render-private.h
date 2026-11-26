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

  MagickCore private methods of draw.c: RenderMVGContent and its helpers,
  SetMVGImageOpaque to ReleaseMVGRenderResources; included by that file only.
*/
#ifndef MAGICKCORE_DRAW_MVG_RENDER_PRIVATE_H
#define MAGICKCORE_DRAW_MVG_RENDER_PRIVATE_H

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

#endif
