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

  MagickCore private methods of display-image.c: the save, screen-event, tile,
  translate, trim and visual-directory commands and the background helpers,
  from IsPostscriptFormat to XDisplayBackgroundPixmap; included by that file
  only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_TILE_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_TILE_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSaveImage() saves an image to a file.
%
%  The format of the XSaveImage method is:
%
%      MagickBooleanType XSaveImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPostscriptFormat(
  const ImageInfo *image_info)
{
  return((MagickBooleanType) ((LocaleCompare(image_info->magick,"EPS") == 0) ||
    (LocaleCompare(image_info->magick,"PDF") == 0) ||
    (LocaleCompare(image_info->magick,"PS") == 0) ||
    (LocaleCompare(image_info->magick,"PS2") == 0)));
}

static void XSelectPageGeometry(Display *display,XWindows *windows,
  ImageInfo *image_info)
{
  if (IsPostscriptFormat(image_info) != MagickFalse)
    {
      char
        geometry[MagickPathExtent];

      const char
        *const PageSizes[] =
        {
          "Letter",
          "Tabloid",
          "Ledger",
          "Legal",
          "Statement",
          "Executive",
          "A3",
          "A4",
          "A5",
          "B4",
          "B5",
          "Folio",
          "Quarto",
          "10x14",
          (char *) NULL
        };

      /*
        Request page geometry from user.
      */
      (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
      if (LocaleCompare(image_info->magick,"PDF") == 0)
        (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
      if (image_info->page != (char *) NULL)
        (void) CopyMagickString(geometry,image_info->page,MagickPathExtent);
      {
        const XListBrowserRequest
          browse_request = { PageSizes, "Select", "Select page geometry:",
            geometry };

        XListBrowserWidget(display,windows,&windows->widget,&browse_request);
      }
      if (*geometry != '\0')
        image_info->page=GetPageGeometry(geometry);
    }
}

static MagickBooleanType XRequestJPEGQuality(Display *display,
  XWindows *windows,Image *image,ImageInfo *image_info)
{
  if ((LocaleCompare(image_info->magick,"JPEG") == 0) ||
      (LocaleCompare(image_info->magick,"JPG") == 0))
    {
      char
        quality[MagickPathExtent];

      int
        status;

      /*
        Request JPEG quality from user.
      */
      (void) FormatLocaleString(quality,MagickPathExtent,"%.17g",(double)
        image->quality);
      {
        const XDialogRequest
          dialog_request = { "Save", "Enter JPEG quality:", quality };

        status=XDialogWidget(display,windows,&dialog_request);
      }
      if (*quality == '\0')
        return(MagickFalse);
      image->quality=StringToUnsignedLong(quality);
      image_info->interlace=status != 0 ? NoInterlace : PlaneInterlace;
    }
  return(MagickTrue);
}

static MagickBooleanType XSeekOverwritePermission(Display *display,
  XWindows *windows,const char *filename)
{
  if (IsPathAccessible(filename) != MagickFalse)
    {
      int
        status;

      /*
        File exists-- seek user's permission before overwriting.
      */
      status=XConfirmWidget(display,windows,"Overwrite",filename);
      if (status <= 0)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static void XGetSaveFilename(const XResourceInfo *resource_info,
  const Image *image,char *filename,ExceptionInfo *exception)
{
  if (resource_info->write_filename != (char *) NULL)
    (void) CopyMagickString(filename,resource_info->write_filename,
      MagickPathExtent);
  else
    {
      char
        path[MagickPathExtent];

      int
        status;

      GetPathComponent(image->filename,HeadPath,path);
      GetPathComponent(image->filename,TailPath,filename);
      if (*path != '\0')
        {
          status=chdir(path);
          if (status == -1)
            (void) ThrowMagickException(exception,GetMagickModule(),
              FileOpenError,"UnableToOpenFile","%s",path);
        }
    }
}

static MagickBooleanType XSaveImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    geometry[MagickPathExtent];

  Image
    *save_image;

  ImageInfo
    *image_info;

  MagickStatusType
    status;

  /*
    Request file name from user.
  */
  XGetSaveFilename(resource_info,image,filename,exception);
  XFileBrowserWidget(display,windows,"Save",filename);
  if (*filename == '\0')
    return(MagickTrue);
  if (XSeekOverwritePermission(display,windows,filename) == MagickFalse)
    return(MagickTrue);
  image_info=CloneImageInfo(resource_info->image_info);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) SetImageInfo(image_info,1,exception);
  if (XRequestJPEGQuality(display,windows,image,image_info) == MagickFalse)
    return(MagickTrue);
  XSelectPageGeometry(display,windows,image_info);
  /*
    Apply image transforms.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  save_image=CloneImage(image,0,0,MagickTrue,exception);
  if (save_image == (Image *) NULL)
    return(MagickFalse);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(&save_image,windows->image.crop_geometry,geometry,
    exception);
  /*
    Write image.
  */
  (void) CopyMagickString(save_image->filename,filename,MagickPathExtent);
  status=WriteImage(image_info,save_image,exception);
  if (status != MagickFalse)
    image->taint=MagickFalse;
  save_image=DestroyImage(save_image);
  image_info=DestroyImageInfo(image_info);
  XSetCursorState(display,windows,MagickFalse);
  return((MagickBooleanType) (status != 0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S c r e e n E v e n t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XScreenEvent() handles global events associated with the Pan and Magnify
%  windows.
%
%  The format of the XScreenEvent function is:
%
%      void XScreenEvent(Display *display,XWindows *windows,XEvent *event,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o event: Specifies a pointer to a X11 XEvent structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int XPredicate(Display *magick_unused(display),XEvent *event,char *data)
{
  XWindows
    *windows;

  windows=(XWindows *) data;
  if ((event->type == ClientMessage) &&
      (event->xclient.window == windows->image.id))
    return(MagickFalse);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static void XSetMagnifyPosition(XWindows *windows,int x,int y)
{
  if (x < 0)
    x=0;
  else
    if (x >= (int) windows->image.width)
      x=(int) (windows->image.width-1);
  windows->magnify.x=(int) windows->image.x+x;
  if (y < 0)
    y=0;
  else
   if (y >= (int) windows->image.height)
     y=(int) (windows->image.height-1);
  windows->magnify.y=windows->image.y+y;
}

static void XMagnifyAtButton(Display *display,XWindows *windows,
  XEvent *event,ExceptionInfo *exception)
{
  int
    x,
    y;

  /*
    Update magnified image.
  */
  x=event->xbutton.x;
  y=event->xbutton.y;
  XSetMagnifyPosition(windows,x,y);
  if (windows->magnify.mapped == MagickFalse)
    (void) XMapRaised(display,windows->magnify.id);
  XMakeMagnifyImage(display,windows,exception);
  if (event->type == ButtonRelease)
    (void) XWithdrawWindow(display,windows->info.id,
      windows->info.screen);
}

static void XScreenButton(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if ((event->xbutton.button == Button3) &&
      (event->xbutton.state & Mod1Mask))
    {
      /*
        Convert Alt-Button3 to Button2.
      */
      event->xbutton.button=Button2;
      event->xbutton.state&=(unsigned int) (~Mod1Mask);
    }
  if (event->xbutton.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,event->xbutton.window,RevertToParent,
        event->xbutton.time);
      return;
    }
  if (event->xbutton.window == windows->pan.id)
    {
      XPanImage(display,windows,event,exception);
      return;
    }
  if ((event->xbutton.window == windows->image.id) &&
      (event->xbutton.button == Button2))
    {
      XMagnifyAtButton(display,windows,event,exception);
      return;
    }
}

static void XScreenClientMessage(Display *display,XWindows *windows,
  XEvent *event)
{
  /*
    If client window delete message, exit.
  */
  if (event->xclient.message_type != windows->wm_protocols)
    return;
  if (*event->xclient.data.l != (long) windows->wm_delete_window)
    return;
  if (event->xclient.window == windows->magnify.id)
    {
      (void) XWithdrawWindow(display,windows->magnify.id,
        windows->magnify.screen);
      return;
    }
}

static void XScreenConfigure(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if (event->xconfigure.window == windows->magnify.id)
    {
      unsigned int
        magnify;

      /*
        Magnify window has a new configuration.
      */
      windows->magnify.width=(unsigned int) event->xconfigure.width;
      windows->magnify.height=(unsigned int) event->xconfigure.height;
      if (windows->magnify.mapped == MagickFalse)
        return;
      magnify=1;
      while ((int) magnify <= event->xconfigure.width)
        magnify<<=1;
      while ((int) magnify <= event->xconfigure.height)
        magnify<<=1;
      magnify>>=1;
      if (((int) magnify != event->xconfigure.width) ||
          ((int) magnify != event->xconfigure.height))
        {
          XWindowChanges
            window_changes;

          window_changes.width=(int) magnify;
          window_changes.height=(int) magnify;
          (void) XReconfigureWMWindow(display,windows->magnify.id,
            windows->magnify.screen,(unsigned int) (CWWidth | CWHeight),
            &window_changes);
          return;
        }
      XMakeMagnifyImage(display,windows,exception);
      return;
    }
}

static void XScreenExpose(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if (event->xexpose.window == windows->image.id)
    {
      XRefreshWindow(display,&windows->image,event);
      return;
    }
  if ((event->xexpose.window == windows->pan.id) &&
      (event->xexpose.count == 0))
    {
      XDrawPanRectangle(display,windows);
      return;
    }
  if ((event->xexpose.window == windows->magnify.id) &&
      (event->xexpose.count == 0))
    {
      XMakeMagnifyImage(display,windows,exception);
      return;
    }
}

static void XScreenKeyPress(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  const MagnifyWindowInput
    input = { display, windows, exception };

  if (event->xkey.window != windows->magnify.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &event->xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  XMagnifyWindowCommand(&input,event->xkey.state,key_symbol);
}

static void XScreenMap(Display *display,XWindows *windows,XEvent *event)
{
  if (event->xmap.window == windows->magnify.id)
    {
      windows->magnify.mapped=MagickTrue;
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      return;
    }
  if (event->xmap.window == windows->info.id)
    {
      windows->info.mapped=MagickTrue;
      return;
    }
}

static void XTrackScreenMagnify(XEvent *event,XWindows *windows,
  Display *display,ExceptionInfo *exception)
{
  int
    x,
    y;

  if ((event->xmotion.window == windows->image.id) &&
      (windows->magnify.mapped != MagickFalse))
    {
      /*
        Update magnified image.
      */
      x=event->xmotion.x;
      y=event->xmotion.y;
      XSetMagnifyPosition(windows,x,y);
      XMakeMagnifyImage(display,windows,exception);
    }
}

static void XScreenMotion(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
  XTrackScreenMagnify(event,windows,display,exception);
}

static void XScreenUnmap(Display *magick_unused(display),XWindows *windows,
  XEvent *event)
{
  magick_unreferenced(display);
  if (event->xunmap.window == windows->magnify.id)
    {
      windows->magnify.mapped=MagickFalse;
      return;
    }
  if (event->xunmap.window == windows->info.id)
    {
      windows->info.mapped=MagickFalse;
      return;
    }
}

static void XScreenOtherEvent(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  switch (event->type)
  {
    case Expose:
    {
      XScreenExpose(display,windows,event,exception);
      break;
    }
    case KeyPress:
    {
      XScreenKeyPress(display,windows,event,exception);
      break;
    }
    case MapNotify:
    {
      XScreenMap(display,windows,event);
      break;
    }
    case MotionNotify:
    {
      XScreenMotion(display,windows,event,exception);
      break;
    }
    case UnmapNotify:
    {
      XScreenUnmap(display,windows,event);
      break;
    }
    default:
      break;
  }
}

static void XDispatchScreenEvent(XEvent *event,Display *display,
  XWindows *windows,ExceptionInfo *exception)
{
  switch (event->type)
  {
    case ButtonPress:
    case ButtonRelease:
    {
      XScreenButton(display,windows,event,exception);
      break;
    }
    case ClientMessage:
    {
      XScreenClientMessage(display,windows,event);
      break;
    }
    case ConfigureNotify:
    {
      XScreenConfigure(display,windows,event,exception);
      break;
    }
    default:
    {
      XScreenOtherEvent(display,windows,event,exception);
      break;
    }
  }
}

static void XScreenEvent(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  (void) XIfEvent(display,event,XPredicate,(char *) windows);
  if (event->xany.window == windows->command.id)
    return;
  XDispatchScreenEvent(event,display,windows,exception);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S e t C r o p G e o m e t r y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSetCropGeometry() accepts a cropping geometry relative to the Image window
%  and translates it to a cropping geometry relative to the image.
%
%  The format of the XSetCropGeometry method is:
%
%      void XSetCropGeometry(Display *display,XWindows *windows,
%        RectangleInfo *crop_info,Image *image)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o crop_info:  A pointer to a RectangleInfo that defines a region of the
%      Image window to crop.
%
%    o image: the image.
%
*/
static void XSetCropGeometry(Display *display,XWindows *windows,
  RectangleInfo *crop_info,Image *image)
{
  char
    text[MagickPathExtent];

  int
    x,
    y;

  double
    scale_factor;

  unsigned int
    height,
    width;

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display info on cropping rectangle.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %.17gx%.17g%+.20g%+.20g",
        (double) crop_info->width,(double) crop_info->height,(double)
        crop_info->x,(double) crop_info->y);
      XInfoWidget(display,windows,text);
    }
  /*
    Cropping geometry is relative to any previous crop geometry.
  */
  x=0;
  y=0;
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  else
    windows->image.crop_geometry=AcquireString((char *) NULL);
  /*
    Define the crop geometry string from the cropping rectangle.
  */
  scale_factor=(double) width/windows->image.ximage->width;
  if (crop_info->x > 0)
    x+=(int) (scale_factor*crop_info->x+0.5);
  width=(unsigned int) (scale_factor*crop_info->width+0.5);
  if (width == 0)
    width=1;
  scale_factor=(double) height/windows->image.ximage->height;
  if (crop_info->y > 0)
    y+=(int) (scale_factor*crop_info->y+0.5);
  height=(unsigned int) (scale_factor*crop_info->height+0.5);
  if (height == 0)
    height=1;
  (void) FormatLocaleString(windows->image.crop_geometry,MagickPathExtent,
    "%ux%u%+d%+d",width,height,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T i l e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTileImage() loads or deletes a selected tile from a visual image directory.
%  The load or delete command is chosen from a menu.
%
%  The format of the XTileImage method is:
%
%      Image *XTileImage(Display *display,XResourceInfo *resource_info,
%        XWindows *windows,Image *image,XEvent *event,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o tile_image:  XTileImage reads or deletes the tile image
%      and returns it.  A null image is returned if an error occurs.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
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
  XConfigureImageColormap(display,resource_info,windows,image,exception);
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

typedef struct _XTileLocationTargets
{
  unsigned int
    *width,
    *height;

  int
    *x,
    *y;

  char
    **p;
} XTileLocationTargets;

static MagickBooleanType XLocateTile(Display *display,Image *image,
  XEvent *event,const XTileLocationTargets *targets)
{
  int
    *const x = targets->x,
    *const y = targets->y;

  char
    **const p = targets->p;

  unsigned int
    *const width = targets->width,
    *const height = targets->height;

  int
    i,
    tile;

  /*
    Determine size and location of each tile in the visual image directory.
  */
  (*width)=(unsigned int) image->columns;
  (*height)=(unsigned int) image->rows;
  (*x)=0;
  (*y)=0;
  (void) XParseGeometry(image->montage,x,y,width,height);
  tile=((event->xbutton.y-(*y))/(int) (*height))*(((int) image->columns-(*x))/(int)
    (*width))+(event->xbutton.x-(*x))/(int) (*width);
  if (tile < 0)
    {
      /*
        Button press is outside any tile.
      */
      (void) XBell(display,0);
      return(MagickFalse);
    }
  /*
    Determine file name from the tile directory.
  */
  (*p)=image->directory;
  for (i=tile; (i != 0) && (*(*p) != '\0'); )
  {
    if (*(*p) == '\xff')
      i--;
    (*p)++;
  }
  if (*(*p) == '\0')
    {
      /*
        Button press is outside any tile.
      */
      (void) XBell(display,0);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XTileDeleteImage(const XTileInput *input)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  char
    *const filename = input->filename;

  int
    status;

  /*
    Delete tile image.
  */
  if (IsPathAccessible(filename) == MagickFalse)
    {
      XNoticeWidget(display,windows,"Image file does not exist:",filename);
      return;
    }
  status=XConfirmWidget(display,windows,"Really delete tile",filename);
  if (status <= 0)
    return;
  status=ShredFile(filename) == MagickFalse ? 0 : 1;
  status|=remove_utf8(filename);
  if (status != MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to delete image file:",
        filename);
      return;
    }
  XTileUpdateImage(input);
}

static Image *XTileCommand(const XTileInput *input)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const TileCommands = input->TileCommands;

  const int
    id = input->id;

  Image
    *tile_image;

  /*
    Perform command for the selected tile.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  tile_image=NewImageList();
  switch (TileCommands[id])
  {
    case TileLoadCommand:
    {
      tile_image=XTileLoadImage(input);
      break;
    }
    case TileNextCommand:
    {
      /*
        Display next image.
      */
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_next_image,CurrentTime);
      break;
    }
    case TileFormerCommand:
    {
      /*
        Display former image.
      */
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_former_image,CurrentTime);
      break;
    }
    case TileDeleteCommand:
    {
      XTileDeleteImage(input);
      break;
    }
    case TileUpdateCommand:
    {
      XTileUpdateImage(input);
      break;
    }
    default:
      break;
  }
  XSetCursorState(display,windows,MagickFalse);
  return(tile_image);
}

static Image *XTileImage(Display *display,XResourceInfo *resource_info,
  XWindows *windows,Image *image,XEvent *event,ExceptionInfo *exception)
{
  const char
    *const VerbMenu[] =
    {
      "Load",
      "Next",
      "Former",
      "Delete",
      "Update",
      (char *) NULL,
    };

  static const ModeType
    TileCommands[] =
    {
      TileLoadCommand,
      TileNextCommand,
      TileFormerCommand,
      TileDeleteCommand,
      TileUpdateCommand
    };

  char
    command[MagickPathExtent],
    filename[MagickPathExtent];

  int
    id,
    x,
    y;

  char
    *p,
    *q;

  unsigned int
    height,
    width;

  XTileInput
    input;

  const XTileLocationTargets
    location = { &width, &height, &x, &y, &p };

  XScaleTileEvent(image,windows,event);
  if (XLocateTile(display,image,event,&location) == MagickFalse)
    return((Image *) NULL);
  /*
    Select a command from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { "Tile Verb", VerbMenu, command };

    id=XMenuWidget(display,windows,&menu_request);
  }
  if (id < 0)
    return((Image *) NULL);
  q=p;
  while ((IsTileNameCharacter(q) != MagickFalse) &&
         ((size_t) (q-p+1) < sizeof(filename)))
    q++;
  (void) CopyMagickString(filename,p,(size_t) (q-p+1));
  input.display=display;
  input.resource_info=resource_info;
  input.windows=windows;
  input.image=image;
  input.exception=exception;
  input.x=x;
  input.y=y;
  input.height=height;
  input.width=width;
  input.filename=filename;
  input.TileCommands=TileCommands;
  input.id=id;
  return(XTileCommand(&input));
}

static void XClampImageOffset(XWindows *windows)
{
  if (windows->image.x < 0)
    windows->image.x=0;
  else
    if ((windows->image.x+(int) windows->image.width) > windows->image.ximage->width)
      windows->image.x=windows->image.ximage->width-(int) windows->image.width;
  if (windows->image.y < 0)
    windows->image.y=0;
  else
    if ((windows->image.y+(int) windows->image.height) > windows->image.ximage->height)
      windows->image.y=windows->image.ximage->height-(int)
        windows->image.height;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T r a n s l a t e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTranslateImage() translates the image within an Image window by one pixel
%  as specified by the key symbol.  If the image has a montage string the
%  translation is respect to the width and height contained within the string.
%
%  The format of the XTranslateImage method is:
%
%      void XTranslateImage(Display *display,XWindows *windows,
%        Image *image,const KeySym key_symbol)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o key_symbol: Specifies a KeySym which indicates which side of the image
%      to trim.
%
*/
static MagickBooleanType XOffsetImageWindowDown(XWindows *windows,
  const KeySym key_symbol,const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_Down:
    case XK_KP_Down:
    {
      windows->image.y+=(int) y_offset;
      break;
    }
    default:
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType XOffsetImageWindowUpRight(XWindows *windows,
  const KeySym key_symbol,const unsigned int x_offset,
  const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Next:
    case XK_Up:
    case XK_KP_Up:
    {
      windows->image.y-=(int) y_offset;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      windows->image.x+=(int) x_offset;
      break;
    }
    default:
    {
      return(XOffsetImageWindowDown(windows,key_symbol,y_offset));
    }
  }
  return(MagickTrue);
}

static MagickBooleanType XOffsetImageWindow(XWindows *windows,
  const KeySym key_symbol,const unsigned int x_offset,
  const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      windows->image.x=(int) windows->image.width/2;
      windows->image.y=(int) windows->image.height/2;
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      windows->image.x-=(int) x_offset;
      break;
    }
    default:
    {
      return(XOffsetImageWindowUpRight(windows,key_symbol,x_offset,y_offset));
    }
  }
  return(MagickTrue);
}

static void XTranslateImage(Display *display,XWindows *windows,
  Image *image,const KeySym key_symbol)
{
  char
    text[MagickPathExtent];

  int
    x,
    y;

  unsigned int
    x_offset,
    y_offset;

  /*
    User specified a pan position offset.
  */
  x_offset=windows->image.width;
  y_offset=windows->image.height;
  if (image->montage != (char *) NULL)
    (void) XParseGeometry(image->montage,&x,&y,&x_offset,&y_offset);
  if (XOffsetImageWindow(windows,key_symbol,x_offset,y_offset) == MagickFalse)
    return;
  /*
    Check boundary conditions.
  */
  XClampImageOffset(windows);
  /*
    Refresh Image window.
  */
  (void) FormatLocaleString(text,MagickPathExtent," %ux%u%+d%+d ",
    windows->image.width,windows->image.height,windows->image.x,
    windows->image.y);
  XInfoWidget(display,windows,text);
  XCheckRefreshWindows(display,windows);
  XDrawPanRectangle(display,windows);
  XRefreshWindow(display,&windows->image,(XEvent *) NULL);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T r i m I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTrimImage() trims the edges from the Image window.
%
%  The format of the XTrimImage method is:
%
%      MagickBooleanType XTrimImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XTrimLeftEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the left edge.
  */
  background=XGetPixel(windows->image.ximage,0,0);
  (*trim_info).width=(size_t) windows->image.ximage->width;
  for (x=0; x < windows->image.ximage->width; x++)
  {
    for (y=0; y < windows->image.ximage->height; y++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (y < windows->image.ximage->height)
      break;
  }
  (*trim_info).x=(ssize_t) x;
}

static void XTrimRightEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the right edge.
  */
  background=XGetPixel(windows->image.ximage,windows->image.ximage->width-1,0);
  for (x=windows->image.ximage->width-1; x != 0; x--)
  {
    for (y=0; y < windows->image.ximage->height; y++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (y < windows->image.ximage->height)
      break;
  }
  (*trim_info).width=(size_t) (x-(*trim_info).x+1);
}

static void XTrimTopEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the top edge.
  */
  background=XGetPixel(windows->image.ximage,0,0);
  (*trim_info).height=(size_t) windows->image.ximage->height;
  for (y=0; y < windows->image.ximage->height; y++)
  {
    for (x=0; x < windows->image.ximage->width; x++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (x < windows->image.ximage->width)
      break;
  }
  (*trim_info).y=(ssize_t) y;
}

static MagickBooleanType XTrimImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  RectangleInfo
    trim_info;

  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Trim edges from image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  /*
    Crop the left edge.
  */
  XTrimLeftEdge(windows,&trim_info);
  if (trim_info.x == (ssize_t) windows->image.ximage->width)
    {
      XSetCursorState(display,windows,MagickFalse);
      return(MagickFalse);
    }
  /*
    Crop the right edge.
  */
  XTrimRightEdge(windows,&trim_info);
  /*
    Crop the top edge.
  */
  XTrimTopEdge(windows,&trim_info);
  /*
    Crop the bottom edge.
  */
  background=XGetPixel(windows->image.ximage,0,windows->image.ximage->height-1);
  for (y=windows->image.ximage->height-1; y != 0; y--)
  {
    for (x=0; x < windows->image.ximage->width; x++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (x < windows->image.ximage->width)
      break;
  }
  trim_info.height=(size_t) (y-trim_info.y+1);
  if (((unsigned int) trim_info.width != windows->image.width) ||
      ((unsigned int) trim_info.height != windows->image.height))
    {
      /*
        Reconfigure Image window as defined by the trimming rectangle.
      */
      XSetCropGeometry(display,windows,&trim_info,image);
      windows->image.window_changes.width=(int) trim_info.width;
      windows->image.window_changes.height=(int) trim_info.height;
      (void) XConfigureImage(display,resource_info,windows,image,exception);
    }
  XSetCursorState(display,windows,MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X V i s u a l D i r e c t o r y I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XVisualDirectoryImage() creates a Visual Image Directory.
%
%  The format of the XVisualDirectoryImage method is:
%
%      Image *XVisualDirectoryImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _DirectoryInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  char
    *filename;

  ExceptionInfo
    *exception;
} DirectoryInput;

static Image *XMontageImageDirectory(const DirectoryInput *input,
  ImageInfo *read_info,Image *images)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *montage_image;

  MontageInfo
    *montage_info;

  char
    *const filename = input->filename;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  /*
    Create the Visual Image Directory.
  */
  montage_info=CloneMontageInfo(read_info,(MontageInfo *) NULL);
  montage_info->pointsize=10;
  if (resource_info->font != (char *) NULL)
    (void) CloneString(&montage_info->font,resource_info->font);
  (void) CopyMagickString(montage_info->filename,filename,MagickPathExtent);
  montage_image=MontageImageList(read_info,montage_info,GetFirstImageInList(
    images),exception);
  images=DestroyImageList(images);
  montage_info=DestroyMontageInfo(montage_info);
  read_info=DestroyImageInfo(read_info);
  XSetCursorState(display,windows,MagickFalse);
  if (montage_image == (Image *) NULL)
    return(montage_image);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_next_image,CurrentTime);
  return(montage_image);
}

typedef struct _DirectoryLoadInput
{
  Display
    *display;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  ImageInfo
    *read_info;

  char
    **filelist;

  int
    number_files;

  MagickBooleanType
    backdrop;

  XResourceInfo
    *background_resources;
} DirectoryLoadInput;

static void XLoadBackdropImage(const DirectoryLoadInput *load_input,
  Image *next_image)
{
  Display
    *const display = load_input->display;

  ExceptionInfo
    *const exception = load_input->exception;

  XWindows
    *const windows = load_input->windows;

  XResourceInfo
    *const background_resources = load_input->background_resources;

  (void) XDisplayBackgroundImage(display,background_resources,next_image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
}

static MagickBooleanType IsDirectoryLoadCancelled(Image *images,const int i,
  const int number_files)
{
  if (images->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(images,LoadImageTag,(MagickOffsetType) i,
        (MagickSizeType) number_files);
      if (proceed == MagickFalse)
        return(MagickTrue);
    }
  return(MagickFalse);
}

static Image *XLoadDirectoryImages(const DirectoryLoadInput *load_input,
  Image *images)
{
  ExceptionInfo
    *const exception = load_input->exception;

  ImageInfo
    *const read_info = load_input->read_info;

  char
    **const filelist = load_input->filelist;

  const int
    number_files = load_input->number_files;

  const MagickBooleanType
    backdrop = load_input->backdrop;

  Image
    *next_image,
    *thumbnail_image;

  RectangleInfo
    geometry;

  int
    i;

  for (i=0; i < (int) number_files; i++)
  {
    (void) CopyMagickString(read_info->filename,filelist[i],MagickPathExtent);
    filelist[i]=DestroyString(filelist[i]);
    *read_info->magick='\0';
    next_image=ReadImage(read_info,exception);
    CatchException(exception);
    if (next_image == (Image *) NULL)
      continue;
    (void) DeleteImageProperty(next_image,"label");
    (void) SetImageProperty(next_image,"label",InterpretImageProperties(
      read_info,next_image,DefaultTileLabel,exception),exception);
    (void) ParseRegionGeometry(next_image,read_info->size,&geometry,
      exception);
    thumbnail_image=ThumbnailImage(next_image,geometry.width,
      geometry.height,exception);
    if (thumbnail_image != (Image *) NULL)
      {
        next_image=DestroyImage(next_image);
        next_image=thumbnail_image;
      }
    if (backdrop)
      XLoadBackdropImage(load_input,next_image);
    AppendImageToList(&images,next_image);
    if (IsDirectoryLoadCancelled(images,i,number_files) != MagickFalse)
      break;
  }
  return(images);
}

typedef struct _DirectoryListTargets
{
  char
    ***filelist;

  int
    *number_files;
} DirectoryListTargets;

static MagickBooleanType XExpandDirectoryFiles(char *filenames,
  const DirectoryListTargets *targets)
{
  char
    ***const filelist = targets->filelist;

  int
    *const number_files = targets->number_files;

  MagickStatusType
    status;

  /*
    Expand the filenames.
  */
  (*filelist)=(char **) AcquireMagickMemory(sizeof(**filelist));
  if ((*filelist) == (char **) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        filenames);
      return(MagickFalse);
    }
  (*number_files)=1;
  (*filelist)[0]=filenames;
  status=ExpandFilenames(number_files,filelist);
  if ((status == MagickFalse) || ((*number_files) == 0))
    {
      if ((*number_files) == 0)
        ThrowXWindowException(ImageError,"NoImagesWereFound",filenames)
      else
        ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
          filenames);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static Image *XVisualDirectoryImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,ExceptionInfo *exception)
{
#define TileImageTag  "Scale/Image"
#define XClientName  "montage"

  char
    **filelist;

  Image
    *images;

  ImageInfo
    *read_info;

  int
    number_files;

  MagickBooleanType
    backdrop;

  static char
    filename[MagickPathExtent] = "\0",
    filenames[MagickPathExtent] = "*";

  XResourceInfo
    background_resources;

  const DirectoryInput
    input = { display, resource_info, windows, filename, exception };

  const DirectoryListTargets
    list_targets = { &filelist, &number_files };

  /*
    Request file name from user.
  */
  XFileBrowserWidget(display,windows,"Directory",filenames);
  if (*filenames == '\0')
    return((Image *) NULL);
  if (XExpandDirectoryFiles(filenames,&list_targets) == MagickFalse)
    return((Image *) NULL);
  /*
    Set image background resources.
  */
  background_resources=(*resource_info);
  background_resources.window_id=AcquireString("");
  (void) FormatLocaleString(background_resources.window_id,MagickPathExtent,
    "0x%lx",windows->image.id);
  background_resources.backdrop=MagickTrue;
  /*
    Read each image and convert them to a tile.
  */
  backdrop=(MagickBooleanType) ((windows->visual_info->klass == TrueColor) ||
    (windows->visual_info->klass == DirectColor));
  read_info=CloneImageInfo(resource_info->image_info);
  (void) SetImageOption(read_info,"jpeg:size","120x120");
  (void) CloneString(&read_info->size,DefaultTileGeometry);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  images=NewImageList();
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  {
    const DirectoryLoadInput
      load_input = { display, windows, exception, read_info, filelist,
        number_files, backdrop, &background_resources };

    images=XLoadDirectoryImages(&load_input,images);
  }
  filelist=(char **) RelinquishMagickMemory(filelist);
  if (images == (Image *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      XSetCursorState(display,windows,MagickFalse);
      ThrowXWindowException(ImageError,"NoImagesWereLoaded",filenames);
      return((Image *) NULL);
    }
  /*
    Create the Visual Image Directory.
  */
  return(XMontageImageDirectory(&input,read_info,images));
}
static void XLookupBackgroundWindowId(char *window_id,XWindowInfo *window_info,
  const Window root_window,Display *display)
{
  if (LocaleCompare(window_id,"root") == 0)
    (*window_info).id=root_window;
  else
    {
      if (isdigit((int) ((unsigned char) *window_id)) != 0)
        (*window_info).id=XWindowByID(display,root_window,
          (Window) strtol((char *) window_id,(char **) NULL,0));
      if ((*window_info).id == (Window) NULL)
        (*window_info).id=XWindowByName(display,root_window,window_id);
    }
}

static MagickBooleanType XFindBackgroundWindow(Display *display,
  char *window_id,const Window root_window,XWindowInfo *window_info)
{
  XLookupBackgroundWindowId(window_id,window_info,root_window,display);
  if ((*window_info).id == (Window) NULL)
    {
      ThrowXWindowException(XServerError,"NoWindowWithSpecifiedIDExists",
        window_id);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XGetBackgroundVisualType(Display *display,
  const XWindowInfo *window_info,XWindowAttributes *window_attributes,
  char *visual_type)
{
  int
    status;

  (*window_attributes).width=XDisplayWidth(display,XDefaultScreen(display));
  (*window_attributes).height=XDisplayHeight(display,XDefaultScreen(display));
  (void) CopyMagickString(visual_type,"default",MagickPathExtent);
  status=XGetWindowAttributes(display,(*window_info).id,window_attributes);
  if (status != 0)
    (void) FormatLocaleString(visual_type,MagickPathExtent,"0x%lx",
      XVisualIDFromVisual((*window_attributes).visual));
}

static void XFitBackgroundWindow(const XWindowAttributes *window_attributes,
  XWindowInfo *window_info)
{
  char
    geometry[MagickPathExtent];

  RectangleInfo
    geometry_info;

  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>",
    (*window_attributes).width,(*window_attributes).height);
  geometry_info.width=(*window_info).width;
  geometry_info.height=(*window_info).height;
  geometry_info.x=(ssize_t) (*window_info).x;
  geometry_info.y=(ssize_t) (*window_info).y;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  (*window_info).width=(unsigned int) geometry_info.width;
  (*window_info).height=(unsigned int) geometry_info.height;
  (*window_info).x=(int) geometry_info.x;
  (*window_info).y=(int) geometry_info.y;
}

typedef struct _BackgroundTargets
{
  XStandardColormap
    **map_info;

  XPixelInfo
    *pixel;

  XResourceInfo
    *resources;

  XVisualInfo
    **visual_info;

  XWindowInfo
    *window_info;
} BackgroundTargets;

static void XInitializeBackgroundVisual(Display *display,const Image *image,
  char *visual_type,const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Allocate standard colormap.
  */
  (*map_info)=XAllocStandardColormap();
  if ((*map_info) == (XStandardColormap *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
      image->filename);
  (*map_info)->colormap=(Colormap) NULL;
  (*pixel).pixels=(unsigned long *) NULL;
  /*
    Initialize visual info.
  */
  (*resources).map_type=(char *) NULL;
  (*resources).visual_type=visual_type;
  (*visual_info)=XBestVisualInfo(display,(*map_info),resources);
  if ((*visual_info) == (XVisualInfo *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToGetVisual",
      (*resources).visual_type);
  /*
    Initialize window info.
  */
  (*window_info).ximage=(XImage *) NULL;
  (*window_info).matte_image=(XImage *) NULL;
  (*window_info).pixmap=(Pixmap) NULL;
  (*window_info).matte_pixmap=(Pixmap) NULL;
}

static void XInitializeBackgroundColormap(XResourceInfo *resources,
  Display *display,XVisualInfo **visual_info,Image *image,
  XStandardColormap **map_info,XPixelInfo *pixel,ExceptionInfo *exception)
{
  (*resources).colormap=SharedColormap;
  XMakeStandardColormap(display,(*visual_info),resources,image,(*map_info),
    pixel,exception);
}

static void XInitializeBackgroundContext(XPixelInfo *pixel,Display *display,
  XWindowInfo *window_info,Image *image)
{
  XGCValues
    context_values;

  context_values.background=(*pixel).foreground_color.pixel;
  context_values.foreground=(*pixel).background_color.pixel;
  (*pixel).annotate_context=XCreateGC(display,(*window_info).id,
    (size_t) (GCBackground | GCForeground),&context_values);
  if ((*pixel).annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      image->filename);
}

static void XInitializeBackgroundAttributes(XWindowInfo *window_info,
  Display *display,XVisualInfo **visual_info,XStandardColormap **map_info,
  XPixelInfo *pixel,XResourceInfo *resources)
{
  (*window_info).name=AcquireString("\0");
  (*window_info).icon_name=AcquireString("\0");
  XGetWindowInfo(display,(*visual_info),(*map_info),pixel,(XFontStruct *) NULL,
    resources,window_info);
}

static void XCreateBackgroundWindowInfo(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Initialize Standard Colormap.
  */
  XInitializeBackgroundColormap(resources,display,visual_info,image,map_info,
    pixel,exception);
  /*
    Graphic context superclass.
  */
  XInitializeBackgroundContext(pixel,display,window_info,image);
  /*
    Initialize Image window attributes.
  */
  XInitializeBackgroundAttributes(window_info,display,visual_info,map_info,
    pixel,resources);
}

static void XSizeBackgroundWindow(const Image *image,XWindowInfo *window_info)
{
  (*window_info).width=(unsigned int) image->columns;
  (*window_info).height=(unsigned int) image->rows;
  if ((image->columns != (*window_info).width) ||
      (image->rows != (*window_info).height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image->filename);
}

static void XMakeBackgroundXImage(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  int
    status;

  {
    const XMakeImageRequest
      image_request = { window_info, image, (*window_info).width,
        (*window_info).height };

    status=XMakeImage(display,resources,&image_request,
      exception) == MagickFalse ? 0 : 1;
  }
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image->filename);
  (*window_info).x=0;
  (*window_info).y=0;
}

typedef struct _BackgroundInput
{
  Display
    *display;

  Image
    *image;

  const XWindowAttributes
    *window_attributes;
} BackgroundInput;

static void XCenterBackdropImage(XResourceInfo *resources,
  XWindowInfo *window_info,const XWindowAttributes *window_attributes,
  int *width,int *height)
{
  if ((*resources).backdrop != MagickFalse)
    {
      /*
        Center image on window.
      */
      (*window_info).x=((*window_attributes).width/2)-
        ((*window_info).ximage->width/2);
      (*window_info).y=((*window_attributes).height/2)-
        ((*window_info).ximage->height/2);
      (*width)=(*window_attributes).width;
      (*height)=(*window_attributes).height;
    }
}

static void XAdjustBackgroundSize(const BackgroundInput *input,
  const BackgroundTargets *targets,int *width,int *height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  const XWindowAttributes
    *const window_attributes = input->window_attributes;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  (*width)=(int) (*window_info).width;
  (*height)=(int) (*window_info).height;
  XCenterBackdropImage(resources,window_info,window_attributes,width,height);
  if (((*resources).image_geometry != (char *) NULL) &&
      (*(*resources).image_geometry != '\0'))
    {
      char
        default_geometry[MagickPathExtent];

      int
        flags,
        gravity;

      XSizeHints
        *size_hints;

      /*
        User specified geometry.
      */
      size_hints=XAllocSizeHints();
      if (size_hints == (XSizeHints *) NULL)
        ThrowXWindowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",image->filename);
      size_hints->flags=0L;
      (void) FormatLocaleString(default_geometry,MagickPathExtent,"%dx%d",
        (*width),(*height));
      flags=XWMGeometry(display,(*visual_info)->screen,
        (*resources).image_geometry,default_geometry,
        (*window_info).border_width,size_hints,&(*window_info).x,
        &(*window_info).y,width,height,&gravity);
      if (flags & (XValue | YValue))
        {
          (*width)=(*window_attributes).width;
          (*height)=(*window_attributes).height;
        }
      (void) XFree((void *) size_hints);
    }
}

static void XCreateBackgroundPixmap(XWindowInfo *window_info,Display *display,
  const int width,const int height,Image *image)
{
  (*window_info).pixmap=XCreatePixmap(display,(*window_info).id,
    (unsigned int) width,(unsigned int) height,(*window_info).depth);
  if ((*window_info).pixmap == (Pixmap) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXPixmap",
      image->filename);
}

static void XShowBackgroundPixmap(const int width,XWindowInfo *window_info,
  const int height,Display *display,Image *image)
{
  size_t
    delay;

  if (((unsigned int) width > (*window_info).width) ||
      ((unsigned int) height > (*window_info).height))
    (void) XFillRectangle(display,(*window_info).pixmap,
      (*window_info).annotate_context,0,0,(unsigned int) width,
      (unsigned int) height);
  (void) XPutImage(display,(*window_info).pixmap,
    (*window_info).annotate_context,(*window_info).ximage,0,0,
    (*window_info).x,(*window_info).y,(unsigned int) (*window_info).width,
    (unsigned int) (*window_info).height);
  (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
    (*window_info).pixmap);
  (void) XClearWindow(display,(*window_info).id);
  delay=1000*image->delay/(size_t) MagickMax(image->ticks_per_second,1L);
  XDelay(display,delay == 0UL ? 10UL : delay);
  (void) XSync(display,MagickFalse);
}

static void XDisplayBackgroundPixmap(const BackgroundInput *input,
  const BackgroundTargets *targets,const int width,const int height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Create the X pixmap.
  */
  XCreateBackgroundPixmap(window_info,display,width,height,image);
  /*
    Display pixmap on the window.
  */
  XShowBackgroundPixmap(width,window_info,height,display,image);
}

#endif
