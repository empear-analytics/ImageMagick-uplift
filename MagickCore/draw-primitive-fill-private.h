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

  MagickCore private draw primitive polygon fill methods.
*/
#ifndef MAGICKCORE_DRAW_PRIMITIVE_FILL_PRIVATE_H
#define MAGICKCORE_DRAW_PRIMITIVE_FILL_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _ExtentInfo
{
  ssize_t
    x1,
    y1,
    x2,
    y2;
} ExtentInfo;

typedef struct _PolygonPointRow
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  const PrimitiveInfo
    *primitive_info;

  ExtentInfo
    poly_extent;

  ssize_t
    y;

  ExceptionInfo
    *exception;
} PolygonPointRow;

static void DrawPolygonPointRow(const PolygonPointRow *row,ssize_t x,
  Quantum *magick_restrict q)
{
  Image
    *const image = row->image;

  const DrawInfo
    *const draw_info = row->draw_info;

  const PrimitiveInfo
    *const primitive_info = row->primitive_info;

  const ExtentInfo
    poly_extent = row->poly_extent;

  const ssize_t
    y = row->y;

  ExceptionInfo
    *const exception = row->exception;

  PixelInfo
    pixel;

  GetPixelInfo(image,&pixel);
  for ( ; x <= poly_extent.x2; x++)
  {
    if ((x == CastDoubleToSsizeT(ceil(primitive_info->point.x-0.5))) &&
        (y == CastDoubleToSsizeT(ceil(primitive_info->point.y-0.5))))
      {
        GetFillColor(draw_info,x-poly_extent.x1,y-poly_extent.y1,&pixel,
          exception);
        SetPixelViaPixelInfo(image,&pixel,q);
      }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D r a w P o l y g o n P r i m i t i v e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPolygonPrimitive() draws a polygon on the image.
%
%  The format of the DrawPolygonPrimitive method is:
%
%      MagickBooleanType DrawPolygonPrimitive(Image *image,
%        const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o draw_info: the draw info.
%
%    o primitive_info: Specifies a pointer to a PrimitiveInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType DrawPolygonPrimitive(Image *image,
  const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  const char
    *artifact;

  double
    mid;

  ExtentInfo
    poly_extent;

  MagickBooleanType
    fill,
    status;

  PolygonInfo
    **magick_restrict polygon_info;

  SegmentInfo
    bounds;

  size_t
    number_threads;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  assert(primitive_info != (PrimitiveInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (primitive_info->coordinates <= 1)
    return(MagickTrue);
  /*
    Compute bounding box.
  */
  polygon_info=AcquirePolygonTLS(primitive_info,exception);
  if (polygon_info == (PolygonInfo **) NULL)
    return(MagickFalse);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    begin draw-polygon");
  fill=(primitive_info->method == FillToBorderMethod) ||
    (primitive_info->method == FloodfillMethod) ? MagickTrue : MagickFalse;
  mid=ExpandAffine(&draw_info->affine)*draw_info->stroke_width/2.0;
  bounds=polygon_info[0]->edges[0].bounds;
  artifact=GetImageArtifact(image,"draw:render-bounding-rectangles");
  if (IsStringTrue(artifact) != MagickFalse)
    (void) DrawBoundingRectangles(image,draw_info,polygon_info[0],exception);
  bounds=GetPolygonEdgesExtent(polygon_info,bounds);
  bounds.x1-=(mid+1.0);
  bounds.y1-=(mid+1.0);
  bounds.x2+=(mid+1.0);
  bounds.y2+=(mid+1.0);
  if (IsBoundsOutsideImage(image,bounds) != MagickFalse)
    {
      polygon_info=DestroyPolygonTLS(polygon_info);
      return(MagickTrue);  /* virtual polygon */
    }
  bounds=ClampPolygonBounds(image,bounds);
  poly_extent.x1=CastDoubleToSsizeT(ceil(bounds.x1-0.5));
  poly_extent.y1=CastDoubleToSsizeT(ceil(bounds.y1-0.5));
  poly_extent.x2=CastDoubleToSsizeT(floor(bounds.x2+0.5));
  poly_extent.y2=CastDoubleToSsizeT(floor(bounds.y2+0.5));
  number_threads=(size_t) GetMagickNumberThreads(image,image,(size_t)
    (poly_extent.y2-poly_extent.y1+1),1);
  status=ClonePolygonEdgesTLS(polygon_info,number_threads,exception);
  if (status == MagickFalse)
    {
      polygon_info=DestroyPolygonTLS(polygon_info);
      return(status);
    }
  image_view=AcquireAuthenticCacheView(image,exception);
  if ((primitive_info->coordinates == 1) ||
      (polygon_info[0]->number_edges == 0))
    {
      /*
        Draw point.
      */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      #pragma omp parallel for schedule(static) shared(status) \
        num_threads((int) number_threads)
#endif
      for (y=poly_extent.y1; y <= poly_extent.y2; y++)
      {
        ssize_t
          x;

        Quantum
          *magick_restrict q;

        if (status == MagickFalse)
          continue;
        x=poly_extent.x1;
        q=GetCacheViewAuthenticPixels(image_view,x,y,(size_t) (poly_extent.x2-
          x+1),1,exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        {
          const PolygonPointRow
            row = { image, draw_info, primitive_info, poly_extent, y,
              exception };

          DrawPolygonPointRow(&row,x,q);
        }
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          status=MagickFalse;
      }
      image_view=DestroyCacheView(image_view);
      polygon_info=DestroyPolygonTLS(polygon_info);
      if (draw_info->debug != MagickFalse)
        (void) LogMagickEvent(DrawEvent,GetMagickModule(),
          "    end draw-polygon");
      return(status);
    }
  /*
    Draw polygon or line.
  */
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    num_threads((int) number_threads)
#endif
  for (y=poly_extent.y1; y <= poly_extent.y2; y++)
  {
    const int
      id = GetOpenMPThreadId();

    Quantum
      *magick_restrict q;

    ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,poly_extent.x1,y,(size_t)
      (poly_extent.x2-poly_extent.x1+1),1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=poly_extent.x1; x <= poly_extent.x2; x++)
    {
      double
        fill_alpha,
        stroke_alpha;

      PixelInfo
        fill_color,
        stroke_color;

      /*
        Fill and/or stroke.
      */
      const FillAlphaSample sample = { mid, fill, x, y };
      fill_alpha=GetFillAlpha(polygon_info[id],&sample,draw_info->fill_rule,
        &stroke_alpha);
      if (draw_info->stroke_antialias == MagickFalse)
        {
          fill_alpha=fill_alpha >= AntialiasThreshold ? 1.0 : 0.0;
          stroke_alpha=stroke_alpha >= AntialiasThreshold ? 1.0 : 0.0;
        }
      GetFillColor(draw_info,x-poly_extent.x1,y-poly_extent.y1,&fill_color,
        exception);
      CompositePixelOver(image,&fill_color,fill_alpha*fill_color.alpha,q,
        (double) GetPixelAlpha(image,q),q);
      GetStrokeColor(draw_info,x-poly_extent.x1,y-poly_extent.y1,&stroke_color,
        exception);
      CompositePixelOver(image,&stroke_color,stroke_alpha*stroke_color.alpha,q,
        (double) GetPixelAlpha(image,q),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  polygon_info=DestroyPolygonTLS(polygon_info);
  if (draw_info->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    end draw-polygon");
  return(status);
}


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
