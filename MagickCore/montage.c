/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               M   M   OOO   N   N  TTTTT   AAA    GGGG  EEEEE               %
%               MM MM  O   O  NN  N    T    A   A  G      E                   %
%               M M M  O   O  N N N    T    AAAAA  G  GG  EEE                 %
%               M   M  O   O  N  NN    T    A   A  G   G  E                   %
%               M   M   OOO   N   N    T    A   A   GGG   EEEEE               %
%                                                                             %
%                                                                             %
%                MagickCore Methods to Create Image Thumbnails                %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
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
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/annotate.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/composite.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel.h"
#include "MagickCore/quantize.h"
#include "MagickCore/property.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/visual-effects.h"


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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o n t a g e I m a g e L i s t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MontageImageList() is a layout manager that lets you tile one or more
%  thumbnails across an image canvas.
%
%  The format of the MontageImageList method is:
%
%      Image *MontageImageList(const ImageInfo *image_info,
%        const MontageInfo *montage_info,Image *images,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o montage_info: Specifies a pointer to a MontageInfo structure.
%
%    o images: Specifies a pointer to an array of Image structures.
%
%    o exception: return any errors or warnings in this structure.
%
*/

typedef struct _MontageGeometryTargets
{
  ssize_t
    *x_offset,
    *y_offset;

  size_t
    *tiles_per_column,
    *tiles_per_row;
} MontageGeometryTargets;

static void GetMontageGeometry(char *geometry,const size_t number_images,
  const MontageGeometryTargets *targets)
{
  ssize_t
    *const x_offset = targets->x_offset,
    *const y_offset = targets->y_offset;

  size_t
    *const tiles_per_column = targets->tiles_per_column,
    *const tiles_per_row = targets->tiles_per_row;

  *tiles_per_column=0;
  *tiles_per_row=0;
  (void) GetGeometry(geometry,x_offset,y_offset,tiles_per_row,tiles_per_column);
  if ((*tiles_per_column == 0) && (*tiles_per_row == 0))
    *tiles_per_column=(size_t) sqrt((double) number_images);
  if ((*tiles_per_column == 0) && (*tiles_per_row != 0))
    *tiles_per_column=(size_t) ceil((double) number_images/(*tiles_per_row));
  if ((*tiles_per_row == 0) && (*tiles_per_column != 0))
    *tiles_per_row=(size_t) ceil((double) number_images/(*tiles_per_column));
}

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int SceneCompare(const void *x,const void *y)
{
  Image
    **image_1,
    **image_2;

  image_1=(Image **) x;
  image_2=(Image **) y;
  return((int) ((*image_1)->scene-(*image_2)->scene));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport Image *MontageImages(const Image *images,
  const MontageInfo *montage_info,ExceptionInfo *exception)
{
  Image
    *montage_image;

  ImageInfo
    *image_info;

  image_info=AcquireImageInfo();
  montage_image=MontageImageList(image_info,montage_info,images,exception);
  image_info=DestroyImageInfo(image_info);
  return(montage_image);
}

static void SortImageListByScene(Image **image_list,
  const size_t number_images)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) number_images; i++)
    if (image_list[i]->scene == 0)
      break;
  if (i == (ssize_t) number_images)
    qsort((void *) image_list,(size_t) number_images,sizeof(*image_list),
      SceneCompare);
}

static RectangleInfo GetLargestTileExtent(RectangleInfo extract_info,
  Image **image_list,const size_t number_images)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) number_images; i++)
  {
    if (image_list[i]->columns > extract_info.width)
      extract_info.width=image_list[i]->columns;
    if (image_list[i]->rows > extract_info.height)
      extract_info.height=image_list[i]->rows;
  }
  return(extract_info);
}

static DrawInfo *AcquireMontageDrawInfo(const ImageInfo *clone_info,
  const MontageInfo *montage_info)
{
  DrawInfo
    *draw_info;

  draw_info=CloneDrawInfo(clone_info,(DrawInfo *) NULL);
  if (montage_info->font != (char *) NULL)
    (void) CloneString(&draw_info->font,montage_info->font);
  if (montage_info->pointsize != 0.0)
    draw_info->pointsize=montage_info->pointsize;
  draw_info->gravity=CenterGravity;
  draw_info->stroke=montage_info->stroke;
  draw_info->fill=montage_info->fill;
  draw_info->text=AcquireString("");
  return(draw_info);
}

static size_t GetMaximumLabelLines(Image **image_list,
  const size_t number_images,ExceptionInfo *exception)
{
  const char
    *value;

  size_t
    number_lines;

  ssize_t
    i;

  number_lines=0;
  for (i=0; i < (ssize_t) number_images; i++)
  {
    value=GetImageProperty(image_list[i],"label",exception);
    if (value == (const char *) NULL)
      continue;
    if (MultilineCensus(value) > number_lines)
      number_lines=MultilineCensus(value);
  }
  return(number_lines);
}

static size_t GetMontageDirectoryExtent(Image **image_list,
  const size_t tiles_per_page,const size_t number_images)
{
  size_t
    extent;

  ssize_t
    tile;

  tile=0;
  extent=1;
  while (tile < MagickMin((ssize_t) tiles_per_page,(ssize_t) number_images))
  {
    extent+=strlen(image_list[tile]->filename)+1;
    tile++;
  }
  return(extent);
}

static Image *ReadMontageTexture(ImageInfo *clone_info,
  const MontageInfo *montage_info,ExceptionInfo *exception)
{
  Image
    *texture;

  texture=NewImageList();
  if (montage_info->texture != (char *) NULL)
    {
      (void) CopyMagickString(clone_info->filename,montage_info->texture,
        MagickPathExtent);
      texture=ReadImage(clone_info,exception);
    }
  return(texture);
}

typedef struct _MontageTileInfo
{
  const MontageInfo
    *montage_info;

  Image
    *montage;

  DrawInfo
    *draw_info;

  ssize_t
    x_offset,
    y_offset,
    x,
    y,
    bevel_width;

  size_t
    width,
    height,
    border_width,
    max_height;

  MagickBooleanType
    concatenate;

  RectangleInfo
    extract_info;

  TypeMetric
    metrics;
} MontageTileInfo;

static Image *CompositeMontageTile(Image *image,
  const MontageTileInfo *tile_info,ExceptionInfo *exception)
{
  char
    tile_geometry[MagickPathExtent];

  const char
    *value;

  /*
    Composite background with tile.
  */
  if (tile_info->montage_info->shadow != MagickFalse)
    image=AddTileShadow(image,exception);
  (void) CompositeImage(tile_info->montage,image,image->compose,MagickTrue,
    tile_info->x_offset+tile_info->x,tile_info->y_offset+tile_info->y,
    exception);
  value=GetImageProperty(image,"label",exception);
  if (value != (const char *) NULL)
    {
      /*
        Annotate composite tile with label.
      */
      (void) FormatLocaleString(tile_geometry,MagickPathExtent,
        "%.17gx%.17g%+.20g%+.20g",(double) ((tile_info->montage_info->frame ?
        image->columns : tile_info->width)-2*tile_info->border_width),(double)
        (tile_info->metrics.ascent-tile_info->metrics.descent+4)*
        MultilineCensus(value),(double) (tile_info->x_offset+(ssize_t)
        tile_info->border_width),(double) ((tile_info->montage_info->frame ?
        tile_info->y_offset+(ssize_t) tile_info->height+(ssize_t)
        tile_info->border_width+4 : tile_info->y_offset+(ssize_t)
        (tile_info->concatenate != MagickFalse ? tile_info->max_height :
        tile_info->extract_info.height)+(ssize_t) tile_info->border_width+
        (tile_info->montage_info->shadow != MagickFalse ? 4 : 0))+
        tile_info->bevel_width));
      (void) CloneString(&tile_info->draw_info->geometry,tile_geometry);
      (void) CloneString(&tile_info->draw_info->text,value);
      (void) AnnotateImage(tile_info->montage,tile_info->draw_info,exception);
    }
  return(image);
}

static void GetMontageOffsets(const MontageInfo *montage_info,
  const size_t number_images,ssize_t *x_offset,ssize_t *y_offset)
{
  size_t
    sans;

  const MontageGeometryTargets
    targets = { x_offset, y_offset, (&sans), (&sans) };

  *x_offset=0;
  *y_offset=0;
  if (montage_info->tile != (char *) NULL)
    GetMontageGeometry(montage_info->tile,number_images,&targets);
}

static ssize_t GetMontageRowOffset(const MontageInfo *montage_info,
  const size_t number_images)
{
  size_t
    sans;

  ssize_t
    x_offset,
    y;

  const MontageGeometryTargets
    targets = { (&x_offset), (&y), (&sans), (&sans) };

  x_offset=0;
  if (montage_info->tile != (char *) NULL)
    GetMontageGeometry(montage_info->tile,number_images,&targets);
  return(x_offset);
}

static void UpdateMontageBoundsHeight(const ssize_t y_offset,
  RectangleInfo *bounds)
{
  if (y_offset > (ssize_t) (*bounds).height)
    (*bounds).height=(size_t) y_offset;
}

static size_t GetMontageRowHeight(const MagickBooleanType concatenate,
  const size_t max_height,const RectangleInfo extract_info)
{
  size_t
    height;

  height=concatenate != MagickFalse ? max_height : extract_info.height;
  return(height);
}

static size_t GetMontageTileWidth(const MagickBooleanType concatenate,
  Image **image_list,const ssize_t tile,const RectangleInfo extract_info)
{
  size_t
    width;

  width=concatenate != MagickFalse ? image_list[tile]->columns :
    extract_info.width;
  return(width);
}

static size_t GetTallestTileHeight(Image **image_list,const ssize_t tile,
  size_t max_height)
{
  if (image_list[tile]->rows > max_height)
    max_height=image_list[tile]->rows;
  return(max_height);
}

typedef struct _MontageBoundsInfo
{
  size_t
    tiles_per_page,
    number_images,
    tiles_per_row,
    border_width,
    number_lines;

  MagickBooleanType
    concatenate;

  Image
    **image_list;

  const MontageInfo
    *montage_info;

  RectangleInfo
    extract_info;

  TypeMetric
    metrics;
} MontageBoundsInfo;

static void GetMontageBounds(ssize_t x_offset,ssize_t y_offset,
  const MontageBoundsInfo *bounds_info,RectangleInfo *bounds)
{
  size_t
    height,
    max_height,
    width;

  ssize_t
    tile;

  max_height=0;
  (*bounds).width=0;
  (*bounds).height=0;
  width=0;
  for (tile=0; tile < (ssize_t) bounds_info->tiles_per_page; tile++)
  {
    if (tile < (ssize_t) bounds_info->number_images)
      {
        width=GetMontageTileWidth(bounds_info->concatenate,
          bounds_info->image_list,tile,bounds_info->extract_info);
        max_height=GetTallestTileHeight(bounds_info->image_list,tile,
          max_height);
      }
    x_offset+=((ssize_t) width+2*(bounds_info->extract_info.x+(ssize_t)
      bounds_info->border_width));
    if (x_offset > (ssize_t) (*bounds).width)
      (*bounds).width=(size_t) x_offset;
    if (((tile+1) == (ssize_t) bounds_info->tiles_per_page) ||
        (((tile+1) % (ssize_t) bounds_info->tiles_per_row) == 0))
      {
        x_offset=GetMontageRowOffset(bounds_info->montage_info,
          bounds_info->number_images);
        height=GetMontageRowHeight(bounds_info->concatenate,max_height,
          bounds_info->extract_info);
        y_offset+=(ssize_t) ((ssize_t) height+(bounds_info->extract_info.y+
          (ssize_t) bounds_info->border_width)*2+(bounds_info->metrics.ascent-
          bounds_info->metrics.descent+4)*(ssize_t) bounds_info->number_lines+
          (bounds_info->montage_info->shadow != MagickFalse ? 4 : 0));
        UpdateMontageBoundsHeight(y_offset,bounds);
        max_height=0;
      }
  }
}

#define MontageImageTag  "Montage/Image"
#define TileImageTag  "Tile/Image"

static MagickBooleanType SetMontageTileProgress(Image *image,
  const MagickOffsetType tiles,const size_t total_tiles,
  MagickBooleanType status)
{
  MagickBooleanType
    proceed;

  proceed=SetImageProgress(image,MontageImageTag,tiles,total_tiles);
  if (proceed == MagickFalse)
    status=MagickFalse;
  return(status);
}

static void DestroyTileImages(Image **image_list,const ssize_t last)
{
  ssize_t
    i;

  for (i=0; i <= last; i++)
    if (image_list[i] != (Image *) NULL)
      image_list[i]=DestroyImage(image_list[i]);
}

static void DestroyTileResources(Image *image,const Image *thumbnail,
  Image **image_list,ssize_t i)
{
  if (image != (Image *) NULL)
    image=DestroyImage(image);
  if (thumbnail == (Image *) NULL)
    i--;
  DestroyTileImages(image_list,i);
}

static MagickBooleanType AcquireNextMontage(const ImageInfo *clone_info,
  Image *montage,ExceptionInfo *exception)
{
  AcquireNextImage(clone_info,montage,exception);
  if (GetNextImageInList(montage) == (Image *) NULL)
    {
      montage=DestroyImageList(montage);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void AppendMontageDirectoryEntry(Image *montage,const char *filename,
  const size_t extent,ExceptionInfo *exception)
{
  if (strchr(filename,(int) '\xff') == (char *) NULL)
    (void) ConcatenateMagickString(montage->directory,filename,extent);
  else
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
      "InvalidArgument","'%s'",filename);
  (void) ConcatenateMagickString(montage->directory,"\xff",extent);
}

static void RelinquishMontageStrings(Image *montage)
{
  if (montage->montage != (char *) NULL)
    montage->montage=(char *) RelinquishMagickMemory(montage->montage);
  if (montage->directory != (char *) NULL)
    montage->directory=(char *) RelinquishMagickMemory(montage->directory);
}

static MagickBooleanType AcquireMontageStrings(Image *montage,
  const size_t extent)
{
  montage->montage=AcquireString((char *) NULL);
  montage->directory=(char *) AcquireQuantumMemory(extent,
    sizeof(*montage->directory));
  if ((montage->montage == (char *) NULL) ||
      (montage->directory == (char *) NULL))
    {
      RelinquishMontageStrings(montage);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void GetMontageTileGeometry(const char *geometry,
  RectangleInfo *extract_info,MagickStatusType *flags,
  MagickBooleanType *concatenate)
{
  if (geometry != (char *) NULL)
    {
      /*
        Initialize tile geometry.
      */
      *flags=GetGeometry(geometry,&(*extract_info).x,&(*extract_info).y,
        &(*extract_info).width,&(*extract_info).height);
      *concatenate=((*flags & RhoValue) == 0) && ((*flags & SigmaValue) == 0) ?
        MagickTrue : MagickFalse;
    }
}

static void GetMontageTileCounts(const MontageInfo *montage_info,
  const size_t number_images,size_t *tiles_per_column,size_t *tiles_per_row)
{
  ssize_t
    x_offset,
    y_offset;

  const MontageGeometryTargets
    targets = { (&x_offset), (&y_offset), tiles_per_column, tiles_per_row };

  *tiles_per_column=(size_t) sqrt((double) number_images);
  *tiles_per_row=(size_t) ceil((double) number_images/(*tiles_per_column));
  x_offset=0;
  y_offset=0;
  if (montage_info->tile != (char *) NULL)
    GetMontageGeometry(montage_info->tile,number_images,&targets);
}

typedef struct _MontageGravityInfo
{
  Image
    *tile_image;

  size_t
    width,
    height;

  const MontageInfo
    *montage_info;

  Image
    *image;

  size_t
    border_width;

  ExceptionInfo
    *exception;
} MontageGravityInfo;

static void GravitateMontageTile(const MontageGravityInfo *gravity,
  MagickStatusType *flags,ssize_t *x,ssize_t *y)
{
  char
    tile_geometry[MagickPathExtent];

  RectangleInfo
    geometry;

  /*
    Gravitate as specified by the tile gravity.
  */
  gravity->tile_image->columns=gravity->width;
  gravity->tile_image->rows=gravity->height;
  gravity->tile_image->gravity=gravity->montage_info->gravity;
  if (gravity->image->gravity != UndefinedGravity)
    gravity->tile_image->gravity=gravity->image->gravity;
  (void) FormatLocaleString(tile_geometry,MagickPathExtent,
    "%.17gx%.17g+0+0",(double) gravity->image->columns,(double)
    gravity->image->rows);
  (*flags)=ParseGravityGeometry(gravity->tile_image,tile_geometry,&geometry,
    gravity->exception);
  (*x)=geometry.x+(ssize_t) gravity->border_width;
  (*y)=geometry.y+(ssize_t) gravity->border_width;
}

typedef struct _MontageTilePlacement
{
  size_t
    border_width;

  const MontageInfo
    *montage_info;

  size_t
    width,
    height;

  ExceptionInfo
    *exception;

  Image
    *tile_image;

  ssize_t
    bevel_width;

  FrameInfo
    frame_info;

  TypeMetric
    metrics;

  Image
    *montage;

  DrawInfo
    *draw_info;

  ssize_t
    x_offset,
    y_offset;

  size_t
    max_height;

  MagickBooleanType
    concatenate;

  RectangleInfo
    extract_info;
} MontageTilePlacement;

static Image *PlaceMontageTile(const MontageTilePlacement *placement,
  Image *image)
{
  MagickStatusType
    flags;

  MontageGravityInfo
    gravity_info;

  ssize_t
    x,
    y;

  if (placement->border_width != 0)
    {
      MontageTileExtent
        extent;

      extent.border_width=placement->border_width;
      extent.width=placement->width;
      extent.height=placement->height;
      image=AddTileBorder(image,placement->montage_info,&extent,
        placement->exception);
    }
  gravity_info.tile_image=placement->tile_image;
  gravity_info.width=placement->width;
  gravity_info.height=placement->height;
  gravity_info.montage_info=placement->montage_info;
  gravity_info.image=image;
  gravity_info.border_width=placement->border_width;
  gravity_info.exception=placement->exception;
  GravitateMontageTile(&gravity_info,&flags,&x,&y);
  if ((placement->montage_info->frame != (char *) NULL) &&
      (placement->bevel_width > 0))
    {
      MontageTileFrame
        frame;

      frame.frame_info=placement->frame_info;
      frame.width=placement->width;
      frame.height=placement->height;
      image=AddTileFrame(image,&frame,placement->metrics,placement->exception);
      x=0;
      y=0;
    }
  if (LocaleCompare(image->magick,"NULL") != 0)
    {
      MontageTileInfo
        tile_info;

      tile_info.montage_info=placement->montage_info;
      tile_info.montage=placement->montage;
      tile_info.draw_info=placement->draw_info;
      tile_info.x_offset=placement->x_offset;
      tile_info.y_offset=placement->y_offset;
      tile_info.x=x;
      tile_info.y=y;
      tile_info.bevel_width=placement->bevel_width;
      tile_info.width=placement->width;
      tile_info.height=placement->height;
      tile_info.border_width=placement->border_width;
      tile_info.max_height=placement->max_height;
      tile_info.concatenate=placement->concatenate;
      tile_info.extract_info=placement->extract_info;
      tile_info.metrics=placement->metrics;
      image=CompositeMontageTile(image,&tile_info,placement->exception);
    }
  return(image);
}

typedef struct _MontagePageInfo
{
  Image
    *montage;

  size_t
    tiles_per_page,
    number_images;

  Image
    **image_list;

  size_t
    extent;

  ExceptionInfo
    *exception;

  Image
    *texture;

  const MontageInfo
    *montage_info;

  const ImageInfo
    *image_info;

  DrawInfo
    *draw_info;

  RectangleInfo
    extract_info;

  char
    *title;

  size_t
    title_offset,
    border_width;

  TypeMetric
    metrics;

  size_t
    number_lines;
} MontagePageInfo;

static void DecorateMontagePage(const MontagePageInfo *page)
{
  MagickProgressMonitor
    progress_monitor;

  ssize_t
    tile;

  *page->montage->directory='\0';
  tile=0;
  while (tile < MagickMin((ssize_t) page->tiles_per_page,(ssize_t)
         page->number_images))
  {
    AppendMontageDirectoryEntry(page->montage,page->image_list[tile]->filename,
      page->extent,page->exception);
    tile++;
  }
  progress_monitor=SetImageProgressMonitor(page->montage,(MagickProgressMonitor)
    NULL,page->montage->client_data);
  if (page->texture != (Image *) NULL)
    (void) TextureImage(page->montage,page->texture,page->exception);
  if (page->montage_info->title != (char *) NULL)
    {
      MontageTitleInfo
        title_info;

      title_info.image_info=page->image_info;
      title_info.draw_info=page->draw_info;
      title_info.extract_info=page->extract_info;
      title_info.title=page->title;
      AnnotateMontageTitle(&title_info,page->image_list,page->montage,
        page->exception);
    }
  (void) SetImageProgressMonitor(page->montage,progress_monitor,
    page->montage->client_data);
}

static size_t GetMontageTitleOffset(const MontageInfo *montage_info,
  const TypeMetric metrics,const char *title,const RectangleInfo extract_info)
{
  size_t
    title_offset;

  title_offset=0;
  if (montage_info->title != (char *) NULL)
    title_offset=(size_t) (2*(metrics.ascent-metrics.descent)*
      MultilineCensus(title)+2*extract_info.y);
  return(title_offset);
}

static void InitializeMontageCanvas(const MontageInfo *montage_info,
  RectangleInfo *bounds,Image *montage,ExceptionInfo *exception)
{
  if (montage_info->shadow != MagickFalse)
    (*bounds).width+=4;
  /*
    Initialize montage image.
  */
  (void) CopyMagickString(montage->filename,montage_info->filename,
    MagickPathExtent);
  montage->columns=(size_t) MagickMax((ssize_t) (*bounds).width,1);
  montage->rows=(size_t) MagickMax((ssize_t) (*bounds).height,1);
  (void) SetImageBackgroundColor(montage,exception);
}

static void SetMontagePage(const MontagePageInfo *page)
{
  ssize_t
    x_offset,
    y_offset;

  GetMontageOffsets(page->montage_info,page->number_images,&x_offset,
    &y_offset);
  y_offset+=(ssize_t) page->title_offset;
  (void) FormatLocaleString(page->montage->montage,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) ((ssize_t) page->extract_info.width+
    (page->extract_info.x+(ssize_t) page->border_width)*2),(double) ((ssize_t)
    page->extract_info.height+(page->extract_info.y+(ssize_t)
    page->border_width)*2+(double) ((page->metrics.ascent-
    page->metrics.descent+4)*page->number_lines+
    (page->montage_info->shadow != MagickFalse ? 4 : 0))),(double) x_offset,
    (double) y_offset);
  DecorateMontagePage(page);
}

MagickExport Image *MontageImageList(const ImageInfo *image_info,
  const MontageInfo *montage_info,const Image *images,ExceptionInfo *exception)
{
  char
    *title;

  DrawInfo
    *draw_info;

  FrameInfo
    frame_info;

  Image
    *image,
    **image_list,
    **primary_list,
    *montage,
    *texture,
    *tile_image,
    *thumbnail;

  ImageInfo
    *clone_info;

  MagickBooleanType
    concatenate,
    proceed,
    status;

  MagickOffsetType
    tiles;

  MagickProgressMonitor
    progress_monitor;

  MagickStatusType
    flags;

  ssize_t
    i;

  RectangleInfo
    bounds,
    geometry,
    extract_info;

  size_t
    border_width,
    extent,
    height,
    images_per_page,
    max_height,
    number_images,
    number_lines,
    tiles_per_column,
    tiles_per_page,
    tiles_per_row,
    title_offset,
    total_tiles,
    width;

  ssize_t
    bevel_width,
    tile,
    x_offset,
    y_offset;

  TypeMetric
    metrics;

  /*
    Create image tiles.
  */
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  assert(montage_info != (MontageInfo *) NULL);
  assert(montage_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  number_images=GetImageListLength(images);
  primary_list=ImageListToArray(images,exception);
  if (primary_list == (Image **) NULL)
    return((Image *) NULL);
  image_list=primary_list;
  image=image_list[0];
  thumbnail=NewImageList();
  for (i=0; i < (ssize_t) number_images; i++)
  {
    image=CloneImage(image_list[i],0,0,MagickTrue,exception);
    if (image == (Image *) NULL)
      break;
    (void) ParseAbsoluteGeometry("0x0+0+0",&image->page);
    progress_monitor=SetImageProgressMonitor(image,(MagickProgressMonitor) NULL,
      image->client_data);
    flags=ParseRegionGeometry(image,montage_info->geometry,&geometry,exception);
    thumbnail=ThumbnailImage(image,geometry.width,geometry.height,exception);
    if (thumbnail == (Image *) NULL)
      break;
    image_list[i]=thumbnail;
    (void) SetImageProgressMonitor(image,progress_monitor,image->client_data);
    proceed=SetImageProgress(image,TileImageTag,(MagickOffsetType) i,
      number_images);
    if (proceed == MagickFalse)
      break;
    image=DestroyImage(image);
  }
  if (i < (ssize_t) number_images)
    {
      DestroyTileResources(image,thumbnail,image_list,i);
      primary_list=(Image **) RelinquishMagickMemory(primary_list);
      return((Image *) NULL);
    }
  /*
    Sort image list by increasing tile number.
  */
  SortImageListByScene(image_list,number_images);
  /*
    Determine tiles per row and column.
  */
  GetMontageTileCounts(montage_info,number_images,&tiles_per_column,
    &tiles_per_row);
  /*
    Determine tile sizes.
  */
  concatenate=MagickFalse;
  SetGeometry(image_list[0],&extract_info);
  extract_info.x=(ssize_t) montage_info->border_width;
  extract_info.y=(ssize_t) montage_info->border_width;
  GetMontageTileGeometry(montage_info->geometry,&extract_info,&flags,
    &concatenate);
  border_width=montage_info->border_width;
  bevel_width=0;
  (void) memset(&frame_info,0,sizeof(frame_info));
  if (montage_info->frame != (char *) NULL)
    {
      frame_info=ParseMontageFrame(montage_info,frame_info,extract_info);
      bevel_width=(ssize_t) MagickMax(frame_info.inner_bevel,
        frame_info.outer_bevel);
      border_width=(size_t) MagickMax((ssize_t) frame_info.width,
        (ssize_t) frame_info.height);
    }
  extract_info=GetLargestTileExtent(extract_info,image_list,number_images);
  /*
    Initialize draw attributes.
  */
  clone_info=CloneImageInfo(image_info);
  clone_info->background_color=montage_info->background_color;
  clone_info->border_color=montage_info->border_color;
  draw_info=AcquireMontageDrawInfo(clone_info,montage_info);
  (void) GetTypeMetrics(image_list[0],draw_info,&metrics,exception);
  texture=ReadMontageTexture(clone_info,montage_info,exception);
  /*
    Determine the number of lines in an next label.
  */
  title=InterpretImageProperties(clone_info,image_list[0],montage_info->title,
    exception);
  title_offset=GetMontageTitleOffset(montage_info,metrics,title,extract_info);
  number_lines=GetMaximumLabelLines(image_list,number_images,exception);
  /*
    Allocate next structure.
  */
  tile_image=AcquireImage((ImageInfo *) NULL,exception);
  montage=AcquireImage(clone_info,exception);
  montage->background_color=montage_info->background_color;
  montage->scene=0;
  images_per_page=(number_images-1)/(tiles_per_row*tiles_per_column)+1;
  tiles=0;
  total_tiles=(size_t) number_images;
  for (i=0; i < (ssize_t) images_per_page; i++)
  {
    MontageBoundsInfo
      bounds_info;

    MontagePageInfo
      page_info;

    /*
      Determine bounding box.
    */
    tiles_per_page=tiles_per_row*tiles_per_column;
    GetMontageOffsets(montage_info,number_images,&x_offset,&y_offset);
    tiles_per_page=tiles_per_row*tiles_per_column;
    y_offset+=(ssize_t) title_offset;
    bounds_info.tiles_per_page=tiles_per_page;
    bounds_info.number_images=number_images;
    bounds_info.tiles_per_row=tiles_per_row;
    bounds_info.border_width=border_width;
    bounds_info.number_lines=number_lines;
    bounds_info.concatenate=concatenate;
    bounds_info.image_list=image_list;
    bounds_info.montage_info=montage_info;
    bounds_info.extract_info=extract_info;
    bounds_info.metrics=metrics;
    GetMontageBounds(x_offset,y_offset,&bounds_info,&bounds);
    InitializeMontageCanvas(montage_info,&bounds,montage,exception);
    /*
      Set montage geometry.
    */
    extent=GetMontageDirectoryExtent(image_list,tiles_per_page,number_images);
    if (AcquireMontageStrings(montage,extent) == MagickFalse)
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    page_info.montage=montage;
    page_info.tiles_per_page=tiles_per_page;
    page_info.number_images=number_images;
    page_info.image_list=image_list;
    page_info.extent=extent;
    page_info.exception=exception;
    page_info.texture=texture;
    page_info.montage_info=montage_info;
    page_info.image_info=image_info;
    page_info.draw_info=draw_info;
    page_info.extract_info=extract_info;
    page_info.title=title;
    page_info.title_offset=title_offset;
    page_info.border_width=border_width;
    page_info.metrics=metrics;
    page_info.number_lines=number_lines;
    SetMontagePage(&page_info);
    /*
      Copy tile to the composite.
    */
    GetMontageOffsets(montage_info,number_images,&x_offset,&y_offset);
    x_offset+=extract_info.x;
    y_offset+=(ssize_t) title_offset+extract_info.y;
    max_height=0;
    status=MagickTrue;
    for (tile=0; tile < MagickMin((ssize_t) tiles_per_page,(ssize_t) number_images); tile++)
    {
      MontageTilePlacement
        placement;

      /*
        Copy this tile to the composite.
      */
      image=CloneImage(image_list[tile],0,0,MagickTrue,exception);
      if (image == (Image *) NULL)
        ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
      progress_monitor=SetImageProgressMonitor(image,
        (MagickProgressMonitor) NULL,image->client_data);
      width=concatenate != MagickFalse ? image->columns : extract_info.width;
      if (image->rows > max_height)
        max_height=image->rows;
      height=concatenate != MagickFalse ? max_height : extract_info.height;
      placement.border_width=border_width;
      placement.montage_info=montage_info;
      placement.width=width;
      placement.height=height;
      placement.exception=exception;
      placement.tile_image=tile_image;
      placement.bevel_width=bevel_width;
      placement.frame_info=frame_info;
      placement.metrics=metrics;
      placement.montage=montage;
      placement.draw_info=draw_info;
      placement.x_offset=x_offset;
      placement.y_offset=y_offset;
      placement.max_height=max_height;
      placement.concatenate=concatenate;
      placement.extract_info=extract_info;
      image=PlaceMontageTile(&placement,image);
      x_offset+=(ssize_t) width+2*(extract_info.x+(ssize_t) border_width);
      if (((tile+1) == (ssize_t) tiles_per_page) ||
          (((tile+1) % (ssize_t) tiles_per_row) == 0))
        {
          x_offset=extract_info.x;
          y_offset+=(ssize_t) ((ssize_t) height+(extract_info.y+
            (ssize_t) border_width)*2+(metrics.ascent-metrics.descent+4)*
            number_lines+(montage_info->shadow != MagickFalse ? 4 : 0));
          max_height=0;
        }
      if (images->progress_monitor != (MagickProgressMonitor) NULL)
        status=SetMontageTileProgress(image,tiles,total_tiles,status);
      image_list[tile]=DestroyImage(image_list[tile]);
      image=DestroyImage(image);
      tiles++;
    }
    (void) status;
    if ((i+1) < (ssize_t) images_per_page)
      {
        /*
          Allocate next image structure.
        */
        if (AcquireNextMontage(clone_info,montage,exception) == MagickFalse)
          return((Image *) NULL);
        montage=GetNextImageInList(montage);
        montage->background_color=montage_info->background_color;
        image_list+=tiles_per_page;
        number_images-=tiles_per_page;
      }
  }
  tile_image=DestroyImage(tile_image);
  if (texture != (Image *) NULL)
    texture=DestroyImage(texture);
  title=DestroyString(title);
  primary_list=(Image **) RelinquishMagickMemory(primary_list);
  draw_info=DestroyDrawInfo(draw_info);
  clone_info=DestroyImageInfo(clone_info);
  return(GetFirstImageInList(montage));
}
