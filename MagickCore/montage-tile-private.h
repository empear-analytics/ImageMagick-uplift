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

  MagickCore private methods of montage.c: the tile decoration helpers of
  MontageImageList (shadow, border, title, frame and the frame geometry
  parser); included by that file only.
*/
#ifndef MAGICKCORE_MONTAGE_TILE_PRIVATE_H
#define MAGICKCORE_MONTAGE_TILE_PRIVATE_H

static Image *AddTileShadow(Image *image,ExceptionInfo *exception)
{
  Image
    *shadow_image;

  /*
    Shadow image.
  */
  (void) QueryColorCompliance("#0000",AllCompliance,
    &image->background_color,exception);
  shadow_image=ShadowImage(image,30.0,5.0,5,5,exception);
  if (shadow_image != (Image *) NULL)
    {
      (void) CompositeImage(shadow_image,image,OverCompositeOp,
        MagickTrue,0,0,exception);
      image=DestroyImage(image);
      image=shadow_image;
    }
  return(image);
}

typedef struct _MontageTileExtent
{
  size_t
    border_width,
    width,
    height;
} MontageTileExtent;

static Image *AddTileBorder(Image *image,const MontageInfo *montage_info,
  const MontageTileExtent *extent,ExceptionInfo *exception)
{
  const size_t
    border_width = extent->border_width,
    width = extent->width,
    height = extent->height;

  Image
    *border_image;

  RectangleInfo
    border_info;

  /*
    Put a border around the image.
  */
  border_info.width=border_width;
  border_info.height=border_width;
  if (montage_info->frame != (char *) NULL)
    {
      border_info.width=(width-image->columns+1)/2;
      border_info.height=(height-image->rows+1)/2;
    }
  border_image=BorderImage(image,&border_info,image->compose,exception);
  if (border_image != (Image *) NULL)
    {
      image=DestroyImage(image);
      image=border_image;
    }
  if ((montage_info->frame != (char *) NULL) &&
      (image->compose == DstOutCompositeOp))
    {
      (void) SetPixelChannelMask(image,AlphaChannel);
      (void) NegateImage(image,MagickFalse,exception);
      (void) SetPixelChannelMask(image,DefaultChannels);
    }
  return(image);
}

typedef struct _MontageTitleInfo
{
  const ImageInfo
    *image_info;

  const DrawInfo
    *draw_info;

  RectangleInfo
    extract_info;

  const char
    *title;
} MontageTitleInfo;

static void AnnotateMontageTitle(const MontageTitleInfo *title_info,
  Image **image_list,Image *montage,ExceptionInfo *exception)
{
  const ImageInfo
    *const image_info = title_info->image_info;

  const DrawInfo
    *const draw_info = title_info->draw_info;

  const RectangleInfo
    extract_info = title_info->extract_info;

  const char
    *const title = title_info->title;

  DrawInfo
    *draw_clone_info;

  char
    tile_geometry[MagickPathExtent];

  TypeMetric
    tile_metrics;

  /*
    Annotate composite image with title.
  */
  draw_clone_info=CloneDrawInfo(image_info,draw_info);
  draw_clone_info->gravity=CenterGravity;
  draw_clone_info->pointsize*=2.0;
  (void) GetTypeMetrics(image_list[0],draw_clone_info,&tile_metrics,
    exception);
  (void) FormatLocaleString(tile_geometry,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) montage->columns,(double)
    (tile_metrics.ascent-tile_metrics.descent),0.0,
    (double) extract_info.y+4);
  (void) CloneString(&draw_clone_info->geometry,tile_geometry);
  (void) CloneString(&draw_clone_info->text,title);
  (void) AnnotateImage(montage,draw_clone_info,exception);
  draw_clone_info=DestroyDrawInfo(draw_clone_info);
}

typedef struct _MontageTileFrame
{
  FrameInfo
    frame_info;

  size_t
    width,
    height;
} MontageTileFrame;

static Image *AddTileFrame(Image *image,const MontageTileFrame *frame,
  const TypeMetric metrics,ExceptionInfo *exception)
{
  const FrameInfo
    frame_info = frame->frame_info;

  const size_t
    width = frame->width,
    height = frame->height;

  FrameInfo
    frame_clone;

  const char
    *value;

  Image
    *frame_image;

  /*
    Put an ornamental border around this tile.
  */
  frame_clone=frame_info;
  frame_clone.width=width+2*frame_info.width;
  frame_clone.height=height+2*frame_info.height;
  value=GetImageProperty(image,"label",exception);
  if (value != (const char *) NULL)
    frame_clone.height+=(size_t) ((metrics.ascent-metrics.descent+4)*
      MultilineCensus(value));
  frame_image=FrameImage(image,&frame_clone,image->compose,exception);
  if (frame_image != (Image *) NULL)
    {
      image=DestroyImage(image);
      image=frame_image;
    }
  return(image);
}

static FrameInfo ParseMontageFrame(const MontageInfo *montage_info,
  FrameInfo frame_info,const RectangleInfo extract_info)
{
  char
    absolute_geometry[MagickPathExtent];

  MagickStatusType
    flags;

  frame_info.width=extract_info.width;
  frame_info.height=extract_info.height;
  (void) FormatLocaleString(absolute_geometry,MagickPathExtent,"%s!",
    montage_info->frame);
  flags=ParseMetaGeometry(absolute_geometry,&frame_info.outer_bevel,
    &frame_info.inner_bevel,&frame_info.width,&frame_info.height);
  if ((flags & HeightValue) == 0)
    frame_info.height=frame_info.width;
  if ((flags & XiValue) == 0)
    frame_info.outer_bevel=(ssize_t) frame_info.width/2-1;
  if ((flags & PsiValue) == 0)
    frame_info.inner_bevel=frame_info.outer_bevel;
  frame_info.x=(ssize_t) frame_info.width;
  frame_info.y=(ssize_t) frame_info.height;
  return(frame_info);
}

#endif
