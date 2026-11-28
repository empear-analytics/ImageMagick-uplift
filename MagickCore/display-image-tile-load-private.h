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

  MagickCore private methods of display-image.c: the tile fill, update and
  load helpers, from IsTileNameCharacter to XScaleTileEvent; included by that
  file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_TILE_LOAD_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_TILE_LOAD_PRIVATE_H

static inline MagickBooleanType IsTileNameCharacter(const char *q)
{
  return((MagickBooleanType) ((*q != '\xff') && (*q != '\0')));
}

typedef struct _XTileFillInfo
{
  Image
    *image;

  CacheView
    *image_view;

  int
    x_offset,
    y_offset;

  unsigned int
    height,
    width;

  PixelInfo
    pixel;

  ExceptionInfo
    *exception;
} XTileFillInfo;

static void XFillMissingTile(const XTileFillInfo *fill)
{
  Image
    *const image = fill->image;

  CacheView
    *const image_view = fill->image_view;

  const int
    x_offset = fill->x_offset,
    y_offset = fill->y_offset;

  const unsigned int
    height = fill->height,
    width = fill->width;

  const PixelInfo
    pixel = fill->pixel;

  ExceptionInfo
    *const exception = fill->exception;

  int
    i,
    j;

  Quantum
    *s;

  for (i=0; i < (int) height; i++)
  {
    s=GetCacheViewAuthenticPixels(image_view,(ssize_t) x_offset,(ssize_t)
      y_offset+i,width,1,exception);
    if (s == (Quantum *) NULL)
      break;
    for (j=0; j < (int) width; j++)
    {
      SetPixelViaPixelInfo(image,&pixel,s);
      s+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

typedef struct _XTileInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    *image;

  ExceptionInfo
    *exception;

  int
    x,
    y;

  unsigned int
    height,
    width;

  char
    *filename;

  const ModeType
    *TileCommands;

  int
    id;
} XTileInput;

static void XTileUpdateImage(const XTileInput *input)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    *const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  const int
    x = input->x,
    y = input->y;

  const unsigned int
    height = input->height,
    width = input->width;

  char
    filename[MagickPathExtent],
    *p,
    *q;

  int
    tile;

  int
    x_offset,
    y_offset;

  PixelInfo
    pixel;

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Ensure all the images exist.
  */
  tile=0;
  GetPixelInfo(image,&pixel);
  for (p=image->directory; *p != '\0'; p++)
  {
    CacheView
      *image_view;

    q=p;
    while ((IsTileNameCharacter(q) != MagickFalse) &&
           ((size_t) (q-p+1) < sizeof(filename)))
      q++;
    (void) CopyMagickString(filename,p,(size_t) (q-p+1));
    p=q;
    if (IsPathAccessible(filename) != MagickFalse)
      {
        tile++;
        continue;
      }
    /*
      Overwrite tile with background color.
    */
    x_offset=((int) width*(tile % (((int) image->columns-x)/(int) width))+
      x);
    y_offset=((int) height*(tile/(((int) image->columns-x)/(int) width))+
      y);
    image_view=AcquireAuthenticCacheView(image,exception);
    (void) GetOneCacheViewVirtualPixelInfo(image_view,0,0,&pixel,exception);
    {
      const XTileFillInfo
        fill = { image, image_view, x_offset, y_offset, height, width,
          pixel, exception };

      XFillMissingTile(&fill);
    }
    image_view=DestroyCacheView(image_view);
    tile++;
  }
  windows->image.window_changes.width=(int) image->columns;
  windows->image.window_changes.height=(int) image->rows;
  XConfigureImageColormap(&cmap_target,image,exception);
  (void) XConfigureImage(display,resource_info,windows,image,exception);
}

static Image *XTileLoadImage(const XTileInput *input)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  ExceptionInfo
    *const exception = input->exception;

  char
    *const filename = input->filename;

  Image
    *tile_image;

  /*
    Load tile image.
  */
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(resource_info->image_info->magick,"MIFF",
    MagickPathExtent);
  (void) CopyMagickString(resource_info->image_info->filename,filename,
    MagickPathExtent);
  tile_image=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  return(tile_image);
}

static void XScaleTileEvent(Image *image,XWindows *windows,XEvent *event)
{
  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  /*
    Tile image is relative to montage image configuration.
  */
  x=0;
  y=0;
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  event->xbutton.x+=windows->image.x;
  event->xbutton.x=(int) (scale_factor*event->xbutton.x+x+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  event->xbutton.y+=windows->image.y;
  event->xbutton.y=(int) (scale_factor*event->xbutton.y+y+0.5);
}

#endif
