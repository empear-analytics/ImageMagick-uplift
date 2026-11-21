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

  MagickCore private draw primitive render methods.
*/
#ifndef MAGICKCORE_DRAW_PRIMITIVE_RENDER_PRIVATE_H
#define MAGICKCORE_DRAW_PRIMITIVE_RENDER_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _PrimitiveScanInfo
{
  Image
    *image;

  CacheView
    *image_view;

  const DrawInfo
    *draw_info;

  ExceptionInfo
    *exception;
} PrimitiveScanInfo;

static MagickStatusType ReplaceAlphaPixels(const PrimitiveScanInfo *scan,
  const PixelInfo target,MagickStatusType status)
{
  Image
    *const image = scan->image;

  CacheView
    *const image_view = scan->image_view;

  const DrawInfo
    *const draw_info = scan->draw_info;

  ExceptionInfo
    *const exception = scan->exception;

  PixelInfo
    pixel;

  ssize_t
    x,
    y;

  GetPixelInfo(image,&pixel);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
        {
          q+=(ptrdiff_t) GetPixelChannels(image);
          continue;
        }
      GetFillColor(draw_info,x,y,&pixel,exception);
      SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
      exception);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

static MagickStatusType ReplaceColorPixels(const PrimitiveScanInfo *scan,
  const PixelInfo target,MagickStatusType status)
{
  Image
    *const image = scan->image;

  CacheView
    *const image_view = scan->image_view;

  const DrawInfo
    *const draw_info = scan->draw_info;

  ExceptionInfo
    *const exception = scan->exception;

  PixelInfo
    pixel;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
        {
          q+=(ptrdiff_t) GetPixelChannels(image);
          continue;
        }
      GetFillColor(draw_info,x,y,&pixel,exception);
      SetPixelViaPixelInfo(image,&pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
      exception);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

static MagickStatusType ResetAlphaPixels(const PrimitiveScanInfo *scan,
  MagickStatusType status)
{
  Image
    *const image = scan->image;

  CacheView
    *const image_view = scan->image_view;

  const DrawInfo
    *const draw_info = scan->draw_info;

  ExceptionInfo
    *const exception = scan->exception;

  PixelInfo
    pixel;

  ssize_t
    x,
    y;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetFillColor(draw_info,x,y,&pixel,exception);
      SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
      exception);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

static MagickStatusType ResetColorPixels(const PrimitiveScanInfo *scan,
  MagickStatusType status)
{
  Image
    *const image = scan->image;

  CacheView
    *const image_view = scan->image_view;

  const DrawInfo
    *const draw_info = scan->draw_info;

  ExceptionInfo
    *const exception = scan->exception;

  PixelInfo
    pixel;

  ssize_t
    x,
    y;

  GetPixelInfo(image,&pixel);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      GetFillColor(draw_info,x,y,&pixel,exception);
      SetPixelViaPixelInfo(image,&pixel,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
      exception);
    if (status == MagickFalse)
      break;
  }
  return(status);
}

typedef struct _DrawPrimitiveContext
{
  Image
    *image;

  const DrawInfo
    *draw_info;

  const PrimitiveInfo
    *primitive_info;

  CacheView
    *image_view;

  ssize_t
    x,
    y;

  ExceptionInfo
    *exception;
} DrawPrimitiveContext;

static void SetFillToBorderTarget(const PrimitiveInfo *primitive_info,
  const DrawInfo *draw_info,PixelInfo *target)
{
  if (primitive_info->method == FillToBorderMethod)
    {
      (*target).red=(double) draw_info->border_color.red;
      (*target).green=(double) draw_info->border_color.green;
      (*target).blue=(double) draw_info->border_color.blue;
    }
}

static MagickStatusType EnsureBlendAlphaChannel(Image *image,
  MagickStatusType status,ExceptionInfo *exception)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    status&=(MagickStatusType) SetImageAlphaChannel(image,
      OpaqueAlphaChannel,exception);
  return(status);
}

static MagickStatusType DrawAlphaPoint(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  PixelInfo
    pixel;

  Quantum
    *q;

  q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
  if (q == (Quantum *) NULL)
    return(status);
  GetFillColor(draw_info,x,y,&pixel,exception);
  SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
  status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
    exception);
  return(status);
}

static MagickStatusType DrawAlphaPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  status=EnsureBlendAlphaChannel(image,status,exception);
  switch (primitive_info->method)
  {
    case PointMethod:
    default:
    {
      status=DrawAlphaPoint(context,status);
      break;
    }
    case ReplaceMethod:
    {
      PixelInfo
        target;

      PrimitiveScanInfo
        scan;

      status&=(MagickStatusType) GetOneCacheViewVirtualPixelInfo(image_view,
        x,y,&target,exception);
      scan.image=image;
      scan.image_view=image_view;
      scan.draw_info=draw_info;
      scan.exception=exception;
      status=ReplaceAlphaPixels(&scan,target,status);
      break;
    }
    case FloodfillMethod:
    case FillToBorderMethod:
    {
      ChannelType
        channel_mask;

      PixelInfo
        target;

      status&=(MagickStatusType) GetOneVirtualPixelInfo(image,
        TileVirtualPixelMethod,x,y,&target,exception);
      SetFillToBorderTarget(primitive_info,draw_info,&target);
      channel_mask=SetImageChannelMask(image,AlphaChannel);
      status&=(MagickStatusType) FloodfillPaintImage(image,draw_info,
        &target,x,y,primitive_info->method == FloodfillMethod ?
        MagickFalse : MagickTrue,exception);
      (void) SetImageChannelMask(image,channel_mask);
      break;
    }
    case ResetMethod:
    {
      const PrimitiveScanInfo
        scan = { image, image_view, draw_info, exception };

      status=ResetAlphaPixels(&scan,status);
      break;
    }
  }
  return(status);
}

static MagickStatusType DrawColorPoint(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  PixelInfo
    pixel;

  Quantum
    *q;

  q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
  if (q == (Quantum *) NULL)
    return(status);
  GetPixelInfo(image,&pixel);
  GetFillColor(draw_info,x,y,&pixel,exception);
  SetPixelViaPixelInfo(image,&pixel,q);
  status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
    exception);
  return(status);
}

static MagickStatusType DrawColorPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  switch (primitive_info->method)
  {
    case PointMethod:
    default:
    {
      status=DrawColorPoint(context,status);
      break;
    }
    case ReplaceMethod:
    {
      PixelInfo
        target;

      PrimitiveScanInfo
        scan;

      status&=(MagickStatusType) GetOneCacheViewVirtualPixelInfo(image_view,
        x,y,&target,exception);
      scan.image=image;
      scan.image_view=image_view;
      scan.draw_info=draw_info;
      scan.exception=exception;
      status=ReplaceColorPixels(&scan,target,status);
      break;
    }
    case FloodfillMethod:
    case FillToBorderMethod:
    {
      PixelInfo
        target;

      status&=(MagickStatusType) GetOneVirtualPixelInfo(image,
        TileVirtualPixelMethod,x,y,&target,exception);
      SetFillToBorderTarget(primitive_info,draw_info,&target);
      status&=(MagickStatusType) FloodfillPaintImage(image,draw_info,
        &target,x,y,primitive_info->method == FloodfillMethod ?
        MagickFalse : MagickTrue,exception);
      break;
    }
    case ResetMethod:
    {
      PrimitiveScanInfo
        scan;

      scan.image=image;
      scan.image_view=image_view;
      scan.draw_info=draw_info;
      scan.exception=exception;
      status=ResetColorPixels(&scan,status);
      break;
    }
  }
  return(status);
}

static Image *ReadCompositeImages(const PrimitiveInfo *primitive_info,
  const DrawInfo *draw_info,ExceptionInfo *exception)
{
  Image
    *composite_images;

  ImageInfo
    *clone_info;

  clone_info=AcquireImageInfo();
  composite_images=(Image *) NULL;
  if (LocaleNCompare(primitive_info->text,"data:",5) == 0)
    composite_images=ReadInlineImage(clone_info,primitive_info->text,
      exception);
  else
    if (*primitive_info->text != '\0')
      {
        /*
          Read composite image.
        */
        (void) CopyMagickString(clone_info->filename,primitive_info->text,
          MagickPathExtent);
        (void) SetImageInfo(clone_info,1,exception);
        (void) CopyMagickString(clone_info->filename,primitive_info->text,
          MagickPathExtent);
        clone_info->size=DestroyString(clone_info->size);
        clone_info->extract=DestroyString(clone_info->extract);
        clone_info->coder_depth=draw_info->image_info->coder_depth;
        composite_images=StrictReadImage(clone_info,exception);
        draw_info->image_info->coder_depth=clone_info->coder_depth;
      }
  clone_info=DestroyImageInfo(clone_info);
  return(composite_images);
}

static MagickStatusType CompositeDrawImage(const DrawPrimitiveContext *context,
  Image *composite_image,MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  AffineMatrix
    affine;

  char
    composite_geometry[MagickPathExtent];

  RectangleInfo
    geometry;

  SetGeometry(image,&geometry);
  image->gravity=draw_info->gravity;
  geometry.x=x;
  geometry.y=y;
  (void) FormatLocaleString(composite_geometry,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) composite_image->columns,(double)
    composite_image->rows,(double) geometry.x,(double) geometry.y);
  (void) ParseGravityGeometry(image,composite_geometry,&geometry,exception);
  affine=draw_info->affine;
  affine.tx=(double) geometry.x;
  affine.ty=(double) geometry.y;
  composite_image->interpolate=image->interpolate;
  if ((draw_info->compose == OverCompositeOp) ||
      (draw_info->compose == SrcOverCompositeOp))
    status&=(MagickStatusType) DrawAffineImage(image,composite_image,
      &affine,exception);
  else
    status&=(MagickStatusType) CompositeImage(image,composite_image,
      draw_info->compose,MagickTrue,geometry.x,geometry.y,exception);
  return(status);
}

static MagickStatusType DrawImagePrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  char
    composite_geometry[MagickPathExtent];

  Image
    *composite_image,
    *composite_images;

  ssize_t
    x1,
    y1;

  if (primitive_info->text == (char *) NULL)
    return(status);
  composite_images=ReadCompositeImages(primitive_info,draw_info,exception);
  if (composite_images == (Image *) NULL)
    {
      status=MagickFalse;
      return(status);
    }
  composite_image=RemoveFirstImageFromList(&composite_images);
  composite_images=DestroyImageList(composite_images);
  (void) SetImageProgressMonitor(composite_image,(MagickProgressMonitor)
    NULL,(void *) NULL);
  x1=CastDoubleToSsizeT(ceil(primitive_info[1].point.x-0.5));
  y1=CastDoubleToSsizeT(ceil(primitive_info[1].point.y-0.5));
  if (IsCompositeOffset(x1,y1,composite_image) != MagickFalse)
    {
      /*
        Resize image.
      */
      (void) FormatLocaleString(composite_geometry,MagickPathExtent,
        "%gx%g!",primitive_info[1].point.x,primitive_info[1].point.y);
      composite_image->filter=image->filter;
      status&=(MagickStatusType) TransformImage(&composite_image,
        (char *) NULL,composite_geometry,exception);
    }
  if (composite_image->alpha_trait == UndefinedPixelTrait)
    status&=(MagickStatusType) SetImageAlphaChannel(composite_image,
      OpaqueAlphaChannel,exception);
  if (draw_info->alpha != OpaqueAlpha)
    status&=(MagickStatusType) SetImageAlpha(composite_image,
      draw_info->alpha,exception);
  status=CompositeDrawImage(context,composite_image,status);
  composite_image=DestroyImage(composite_image);
  return(status);
}

static MagickStatusType DrawPointPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  CacheView
    *image_view = context->image_view;

  const ssize_t
    x = context->x,
    y = context->y;

  ExceptionInfo
    *exception = context->exception;

  PixelInfo
    fill_color;

  Quantum
    *q;

  if ((y < 0) || (y >= (ssize_t) image->rows))
    return(status);
  if ((x < 0) || (x >= (ssize_t) image->columns))
    return(status);
  q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
  if (q == (Quantum *) NULL)
    return(status);
  GetFillColor(draw_info,x,y,&fill_color,exception);
  CompositePixelOver(image,&fill_color,(double) fill_color.alpha,q,(double)
    GetPixelAlpha(image,q),q);
  status&=(MagickStatusType) SyncCacheViewAuthenticPixels(image_view,
    exception);
  return(status);
}

static MagickStatusType DrawUnstrokedPolygon(
  const DrawPrimitiveContext *context,MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  DrawInfo
    *clone_info;

  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  clone_info->stroke_width=0.0;
  clone_info->stroke.alpha=(MagickRealType) TransparentAlpha;
  status&=(MagickStatusType) DrawPolygonPrimitive(image,clone_info,
    primitive_info,exception);
  clone_info=DestroyDrawInfo(clone_info);
  return(status);
}

static MagickStatusType DrawTextPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  char
    geometry[MagickPathExtent];

  DrawInfo
    *clone_info;

  if (primitive_info->text == (char *) NULL)
    return(status);
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  (void) CloneString(&clone_info->text,primitive_info->text);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%+f%+f",
    primitive_info->point.x,primitive_info->point.y);
  (void) CloneString(&clone_info->geometry,geometry);
  status&=(MagickStatusType) AnnotateImage(image,clone_info,exception);
  clone_info=DestroyDrawInfo(clone_info);
  return(status);
}

static MagickStatusType DrawWideStrokedPrimitive(
  const DrawPrimitiveContext *context,MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  double
    point_x,
    point_y;

  MagickBooleanType
    closed_path;

  ssize_t
    i;

  /*
    Draw strokes while respecting line cap/join attributes.
  */
  closed_path=primitive_info[0].closed_subpath;
  i=(ssize_t) primitive_info[0].coordinates;
  point_x=fabs(primitive_info[i-1].point.x-primitive_info[0].point.x);
  point_y=fabs(primitive_info[i-1].point.y-primitive_info[0].point.y);
  if (IsClosedEllipse(point_x,point_y) != MagickFalse)
    closed_path=MagickTrue;
  if (IsRoundStrokeOrPrimitive(draw_info,closed_path,primitive_info,i) !=
      MagickFalse)
    {
      status&=(MagickStatusType) DrawPolygonPrimitive(image,draw_info,
        primitive_info,exception);
      return(status);
    }
  status=DrawUnstrokedPolygon(context,status);
  if (status != MagickFalse)
    status&=(MagickStatusType) DrawStrokePolygon(image,draw_info,
      primitive_info,exception);
  return(status);
}

static MagickStatusType DrawStrokedPrimitive(const DrawPrimitiveContext *context,
  MagickStatusType status)
{
  Image
    *image = context->image;

  const DrawInfo
    *draw_info = context->draw_info;

  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  ExceptionInfo
    *exception = context->exception;

  double
    mid,
    scale;


  if (IsEventLogging() != MagickFalse)
    LogPrimitiveInfo(primitive_info);
  scale=ExpandAffine(&draw_info->affine);
  if (IsVisibleDashedStroke(draw_info,scale) != MagickFalse)
    {
      /*
        Draw dash polygon.
      */
      status=DrawUnstrokedPolygon(context,status);
      if (status != MagickFalse)
        status&=(MagickStatusType) DrawDashPolygon(draw_info,primitive_info,
          image,exception);
      return(status);
    }
  mid=ExpandAffine(&draw_info->affine)*draw_info->stroke_width/2.0;
  if (IsVisibleWideStroke(mid,draw_info) != MagickFalse)
    return(DrawWideStrokedPrimitive(context,status));
  status&=(MagickStatusType) DrawPolygonPrimitive(image,draw_info,
    primitive_info,exception);
  return(status);
}

static MagickStatusType DrawPrimitiveByType(
  const DrawPrimitiveContext *context,MagickStatusType status)
{
  const PrimitiveInfo
    *primitive_info = context->primitive_info;

  switch (primitive_info->primitive)
  {
    case AlphaPrimitive:
    {
      status=DrawAlphaPrimitive(context,status);
      break;
    }
    case ColorPrimitive:
    {
      status=DrawColorPrimitive(context,status);
      break;
    }
    case ImagePrimitive:
    {
      status=DrawImagePrimitive(context,status);
      break;
    }
    case PointPrimitive:
    {
      status=DrawPointPrimitive(context,status);
      break;
    }
    case TextPrimitive:
    {
      status=DrawTextPrimitive(context,status);
      break;
    }
    default:
    {
      status=DrawStrokedPrimitive(context,status);
      break;
    }
  }
  return(status);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
