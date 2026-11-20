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

  MagickCore private methods of display-image.c: the composite and configure
  commands, the crop command and the first draw helpers, from XCompositeEvent
  to XCreateStippleBitmapTail; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_COMPOSITE_CROP_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_COMPOSITE_CROP_PRIVATE_H

static void XCompositeEvent(const CompositeCommandInput *input,XEvent *event,
  const Cursor cursor,const CompositeEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    *const composite_info = targets->composite_info;

  int
    x,
    y;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XCompositePress(input,event,cursor,targets);
      break;
    }
    case ButtonRelease:
    {
      XCompositeRelease(input,event,targets);
      break;
    }
    case Expose:
      break;
    case KeyPress:
    {
      XCompositeKeyPress(input,event,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      x=(*event).xmotion.x;
      y=(*event).xmotion.y;
      XTrackInfoWidget(display,windows,x,y);
      (*composite_info).x=(ssize_t) windows->image.x+x;
      (*composite_info).y=(ssize_t) windows->image.y+y;
      break;
    }
    default:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"Event type: %d",
          (*event).type);
      break;
    }
  }
}

static void XShowCompositeOffset(XWindows *windows,char *text,
  RectangleInfo *composite_info,Display *display)
{
  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+ld%+ld ",
        (long) (*composite_info).x,(long) (*composite_info).y);
      XInfoWidget(display,windows,text);
    }
}

static void XCompositeStep(const CompositeCommandInput *input,
  const Cursor cursor,const CompositeEventTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  CompositeOperator
    *const compose = targets->compose;

  RectangleInfo
    *const composite_info = targets->composite_info;

  size_t
    *const state = targets->state;

  char
    text[MagickPathExtent];

  RectangleInfo
    highlight_info;

  XEvent
    event;

  XShowCompositeOffset(windows,text,composite_info,display);
  highlight_info=(*composite_info);
  highlight_info.x=(*composite_info).x-windows->image.x;
  highlight_info.y=(*composite_info).y-windows->image.y;
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  if (XCompositeCommandBlock(input,&event,compose,state) != MagickFalse)
    return;
  XCompositeEvent(input,&event,cursor,targets);
}

static void XCompositeScaleLocation(const Image *image,
  const XWindows *windows,RectangleInfo *composite_info)
{
  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  (*composite_info).x+=x;
  (*composite_info).x=(ssize_t) (scale_factor*(*composite_info).x+0.5);
  (*composite_info).width=(unsigned int) (scale_factor*
    (*composite_info).width+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  (*composite_info).y+=y;
  (*composite_info).y=(ssize_t) (scale_factor*(*composite_info).y+0.5);
  (*composite_info).height=(unsigned int) (scale_factor*
    (*composite_info).height+0.5);
}

static const char
  *const CompositeEditMenu[] =
  {
    "Operators",
    "Dissolve",
    "Displace",
    "Help",
    "Dismiss",
    (char *) NULL
  };

static const ModeType
  CompositeEditCommands[] =
  {
    CompositeOperatorsCommand,
    CompositeDissolveCommand,
    CompositeDisplaceCommand,
    CompositeHelpCommand,
    CompositeDismissCommand
  };

static MagickBooleanType XCompositeIntoImage(
  const CompositeCommandInput *input,Image *image,Image *composite_image,
  RectangleInfo composite_info)
{
  char
    *const displacement_geometry = input->displacement_geometry;

  CompositeOperator
    *const compose = input->targets.compose;

  Display
    *const display = input->display;

  double
    *const blend = input->targets.blend;

  ExceptionInfo
    *const exception = input->exception;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  /*
    Image compositing is relative to image configuration.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  XCompositeScaleLocation(image,windows,&composite_info);
  if ((composite_info.width != composite_image->columns) ||
      (composite_info.height != composite_image->rows))
    {
      Image
        *resize_image;

      /*
        Scale composite image.
      */
      resize_image=ResizeImage(composite_image,composite_info.width,
        composite_info.height,composite_image->filter,exception);
      composite_image=DestroyImage(composite_image);
      if (resize_image == (Image *) NULL)
        {
          XSetCursorState(display,windows,MagickFalse);
          return(MagickFalse);
        }
      composite_image=resize_image;
    }
  if ((*compose) == DisplaceCompositeOp)
    (void) SetImageArtifact(composite_image,"compose:args",
      displacement_geometry);
  if (XCreateBlendMattes(image,composite_image,(*blend),exception) == MagickFalse)
    return(MagickFalse);
  /*
    Composite image with X Image window.
  */
  (void) CompositeImage(image,composite_image,(*compose),MagickTrue,
    composite_info.x,composite_info.y,exception);
  composite_image=DestroyImage(composite_image);
  XSetCursorState(display,windows,MagickFalse);
  /*
    Update image configuration.
  */
  XConfigureImageColormap(display,resource_info,windows,image,exception);
  (void) XConfigureImage(display,resource_info,windows,image,exception);
  return(MagickTrue);
}

static Image *XReadCompositeImage(const CompositeCommandInput *input,
  const char *filename)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *composite_image;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(resource_info->image_info->filename,filename,
    MagickPathExtent);
  composite_image=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  return(composite_image);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C o m p o s i t e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XCompositeImage() requests an image name from the user, reads the image and
%  composites it with the X window image at a location the user chooses with
%  the pointer.
%
%  The format of the XCompositeImage method is:
%
%      MagickBooleanType XCompositeImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
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
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType XCompositeImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  static char
    displacement_geometry[MagickPathExtent] = "30x30",
    filename[MagickPathExtent] = "\0";

  static CompositeOperator
    compose = CopyCompositeOp;

  Cursor
    cursor;

  Image
    *composite_image;

  int
    x,
    y;

  double
    blend;

  RectangleInfo
    composite_info;

  size_t
    state;

  const CompositeTargets
    targets = { &blend, &compose };

  const CompositeEventTargets
    event_targets = { &composite_image, &state, &composite_info,
      &compose };

  const CompositeCommandInput
    command_input = { display, resource_info, windows, CompositeEditCommands,
      CompositeEditMenu, displacement_geometry, targets, exception };

  /*
    Request image file name from user.
  */
  XFileBrowserWidget(display,windows,"Composite",filename);
  if (*filename == '\0')
    return(MagickTrue);
  /*
    Read image.
  */
  composite_image=XReadCompositeImage(&command_input,filename);
  if (composite_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Composite");
  windows->command.data=1;
  (void) XCommandWidget(display,windows,CompositeEditMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  composite_info.x=(ssize_t) windows->image.x+x;
  composite_info.y=(ssize_t) windows->image.y+y;
  composite_info.width=0;
  composite_info.height=0;
  cursor=XCreateFontCursor(display,XC_ul_angle);
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  blend=0.0;
  state=DefaultState;
  do
  {
    XCompositeStep(&command_input,cursor,&event_targets);
  } while ((state & ExitState) == 0);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeCursor(display,cursor);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  return(XCompositeIntoImage(&command_input,image,composite_image,
    composite_info));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C o n f i g u r e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XConfigureImage() creates a new X image.  It also notifies the window
%  manager of the new image size and configures the transient widows.
%
%  The format of the XConfigureImage method is:
%
%      MagickBooleanType XConfigureImage(Display *display,
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
%    o exception: return any errors or warnings in this structure.
%
*/
static void XSetPanSizeHints(XSizeHints *size_hints,
  const XWindowChanges *window_changes,Display *display,XWindows *windows)
{
  if (size_hints != (XSizeHints *) NULL)
    {
      /*
        Set new size hints.
      */
      size_hints->flags=PSize | PMinSize | PMaxSize;
      size_hints->width=(*window_changes).width;
      size_hints->height=(*window_changes).height;
      size_hints->min_width=size_hints->width;
      size_hints->min_height=size_hints->height;
      size_hints->max_width=size_hints->width;
      size_hints->max_height=size_hints->height;
      (void) XSetNormalHints(display,windows->pan.id,size_hints);
      (void) XFree((void *) size_hints);
    }
}

static void XResizePanWindow(Display *display,XWindows *windows,Image *image)
{
  XSizeHints
    *size_hints;

  XWindowChanges
    window_changes;

  windows->pan.crop_geometry=windows->image.crop_geometry;
  XBestIconSize(display,&windows->pan,image);
  while (((windows->pan.width << 1) < MaxIconSize) &&
         ((windows->pan.height << 1) < MaxIconSize))
  {
    windows->pan.width<<=1;
    windows->pan.height<<=1;
  }
  if (windows->pan.geometry != (char *) NULL)
    (void) XParseGeometry(windows->pan.geometry,&windows->pan.x,&windows->pan.y,
      &windows->pan.width,&windows->pan.height);
  window_changes.width=(int) windows->pan.width;
  window_changes.height=(int) windows->pan.height;
  size_hints=XAllocSizeHints();
  XSetPanSizeHints(size_hints,&window_changes,display,windows);
  (void) XReconfigureWMWindow(display,windows->pan.id,windows->pan.screen,
    (unsigned int) (CWWidth | CWHeight),&window_changes);
}

typedef struct _ConfigureNotifyInput
{
  size_t
    width,
    height;

  ssize_t
    x,
    y;
} ConfigureNotifyInput;

static void XFormatWindowManagerGeometry(XResourceInfo *resource_info,
  char *geometry,Display *display,XWindows *windows)
{
  if (resource_info->image_geometry != (char *) NULL)
    (void) FormatLocaleString(geometry,MagickPathExtent,"%s>!",
      resource_info->image_geometry);
  else
    (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>!",
      XDisplayWidth(display,windows->image.screen),
      XDisplayHeight(display,windows->image.screen));
}

static void XNotifyWindowManager(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const ConfigureNotifyInput *input)
{
  char
    geometry[MagickPathExtent];

  size_t
    height = input->height,
    mask,
    width = input->width;

  ssize_t
    x = input->x,
    y = input->y;

  XWindowChanges
    window_changes;

  XFormatWindowManagerGeometry(resource_info,geometry,display,windows);
  (void) ParseMetaGeometry(geometry,&x,&y,&width,&height);
  window_changes.width=(int) width;
  if (window_changes.width > XDisplayWidth(display,windows->image.screen))
    window_changes.width=XDisplayWidth(display,windows->image.screen);
  window_changes.height=(int) height;
  if (window_changes.height > XDisplayHeight(display,windows->image.screen))
    window_changes.height=XDisplayHeight(display,windows->image.screen);
  mask=(size_t) (CWWidth | CWHeight);
  if (resource_info->backdrop)
    {
      mask|=CWX | CWY;
      window_changes.x=((XDisplayWidth(display,windows->image.screen)/2)-
        ((int) width/2));
      window_changes.y=((XDisplayHeight(display,windows->image.screen)/2)-
        ((int) height/2));
    }
  (void) XReconfigureWMWindow(display,windows->image.id,windows->image.screen,
    (unsigned int) mask,&window_changes);
}

static void XUpdateMagnifyConfiguration(XWindows *windows,Display *display,
  Image *image,ExceptionInfo *exception)
{
  if (windows->magnify.mapped != MagickFalse)
    XMakeMagnifyImage(display,windows,exception);
  XResizePanWindow(display,windows,image);
}

static MagickBooleanType XConfigureImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  ConfigureNotifyInput
    notify;

  MagickStatusType
    status;

  size_t
    height,
    width;

  ssize_t
    x,
    y;

  XWindowChanges
    window_changes;

  /*
    Dismiss if window dimensions are zero.
  */
  width=(unsigned int) windows->image.window_changes.width;
  height=(unsigned int) windows->image.window_changes.height;
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Configure Image: %dx%d=>%.17gx%.17g",windows->image.ximage->width,
      windows->image.ximage->height,(double) width,(double) height);
  if ((width*height) == 0)
    return(MagickTrue);
  x=0;
  y=0;
  /*
    Resize image to fit Image window dimensions.
  */
  XSetCursorState(display,windows,MagickTrue);
  (void) XFlush(display);
  if (((int) width != windows->image.ximage->width) ||
      ((int) height != windows->image.ximage->height))
    image->taint=MagickTrue;
  windows->magnify.x=(int)
    width*windows->magnify.x/windows->image.ximage->width;
  windows->magnify.y=(int)
    height*windows->magnify.y/windows->image.ximage->height;
  windows->image.x=((int) width*windows->image.x/windows->image.ximage->width);
  windows->image.y=((int) height*windows->image.y/
    windows->image.ximage->height);
  {
    const XMakeImageRequest
      image_request = { &windows->image, image, (unsigned int) width,
        (unsigned int) height };

    status=XMakeImage(display,resource_info,&image_request,exception);
  }
  if (status == MagickFalse)
    XNoticeWidget(display,windows,"Unable to configure X image:",
      windows->image.name);
  /*
    Notify window manager of the new configuration.
  */
  notify.width=width;
  notify.height=height;
  notify.x=x;
  notify.y=y;
  XNotifyWindowManager(display,resource_info,windows,&notify);
  (void) XClearWindow(display,windows->image.id);
  XRefreshWindow(display,&windows->image,(XEvent *) NULL);
  /*
    Update Magnify window configuration.
  */
  XUpdateMagnifyConfiguration(windows,display,image,exception);
  /*
    Update icon window configuration.
  */
  windows->icon.crop_geometry=windows->image.crop_geometry;
  XBestIconSize(display,&windows->icon,image);
  window_changes.width=(int) windows->icon.width;
  window_changes.height=(int) windows->icon.height;
  (void) XReconfigureWMWindow(display,windows->icon.id,windows->icon.screen,
    (unsigned int) (CWWidth | CWHeight),&window_changes);
  XSetCursorState(display,windows,MagickFalse);
  return((MagickBooleanType) (status != 0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C r o p I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XCropImage() allows the user to select a region of the image and crop, copy,
%  or cut it.  For copy or cut, the image can subsequently be composited onto
%  the image with XPasteImage.
%
%  The format of the XCropImage method is:
%
%      MagickBooleanType XCropImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        const ClipboardMode mode,ExceptionInfo *exception)
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
%    o image: the image; returned from ReadImage.
%
%    o mode: This unsigned value specified whether the image should be
%      cropped, copied, or cut.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsNearTopLeftCorner(const int x,const int y,
  const RectangleInfo rectangle)
{
  return((MagickBooleanType) ((x < (int) (rectangle.x+RoiDelta)) &&
    (x > (int) (rectangle.x-RoiDelta)) &&
    (y < (int) (rectangle.y+RoiDelta)) &&
    (y > (int) (rectangle.y-RoiDelta))));
}

static inline MagickBooleanType IsNearBottomLeftCorner(const int x,const int y,
  const RectangleInfo rectangle)
{
  return((MagickBooleanType) ((x < (int) (rectangle.x+RoiDelta)) &&
    (x > (int) (rectangle.x-RoiDelta)) &&
    (y < (int) (rectangle.y+(int) rectangle.height+RoiDelta)) &&
    (y > (int) (rectangle.y+(int) rectangle.height-RoiDelta))));
}

static inline MagickBooleanType IsNearTopRightCorner(const int x,const int y,
  const RectangleInfo rectangle)
{
  return((MagickBooleanType) ((x < (int) (rectangle.x+(int)
    rectangle.width+RoiDelta)) && (x > (int) (rectangle.x+(int)
    rectangle.width-RoiDelta)) && (y < (int) (rectangle.y+RoiDelta)) && (y >
    (int) (rectangle.y-RoiDelta))));
}

static inline MagickBooleanType IsNearBottomRightCorner(const int x,const int y,
  const RectangleInfo rectangle)
{
  return((MagickBooleanType) ((x < (int) (rectangle.x+(int)
    rectangle.width+RoiDelta)) && (x > (int) (rectangle.x+(int)
    rectangle.width-RoiDelta)) && (y < (int) (rectangle.y+(int)
    rectangle.height+RoiDelta)) && (y > (int) (rectangle.y+(int)
    rectangle.height-RoiDelta))));
}

static inline MagickBooleanType IsSelectionMovedOrDone(
  const RectangleInfo rectangle,const int x,const int y,const size_t state)
{
  return((MagickBooleanType) ((((int) rectangle.x != x) &&
    ((int) rectangle.y != y)) || ((state & ExitState) != 0)));
}

static void XShowCropHelp(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const ClipboardMode mode)
{
  switch (mode)
  {
    case CopyMode:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Copy",ImageCopyHelp);
      }
      break;
    }
    case CropMode:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Crop",ImageCropHelp);
      }
      break;
    }
    case CutMode:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Cut",ImageCutHelp);
      }
      break;
    }
  }
}

static void XMapInfoWidget(Display *display,XWindows *windows)
{
  if (windows->info.mapped == MagickFalse)
    (void) XMapWindow(display,windows->info.id);
}

static void XCheckSelectionBounds(XWindows *windows,const int x,const int y,
  RectangleInfo *crop_info)
{
  /*
    Check boundary conditions.
  */
  if ((*crop_info).x < 0)
    (*crop_info).x=0;
  else
    if ((*crop_info).x > (ssize_t) windows->image.ximage->width)
      (*crop_info).x=(ssize_t) windows->image.ximage->width;
  if ((int) (*crop_info).x < x)
    (*crop_info).width=(unsigned int) (x-(*crop_info).x);
  else
    {
      (*crop_info).width=(unsigned int) ((*crop_info).x-x);
      (*crop_info).x=(ssize_t) x;
    }
  if ((*crop_info).y < 0)
    (*crop_info).y=0;
  else
    if ((*crop_info).y > (ssize_t) windows->image.ximage->height)
      (*crop_info).y=(ssize_t) windows->image.ximage->height;
  if ((int) (*crop_info).y < y)
    (*crop_info).height=(unsigned int) (y-(*crop_info).y);
  else
    {
      (*crop_info).height=(unsigned int) ((*crop_info).y-y);
      (*crop_info).y=(ssize_t) y;
    }
}

typedef struct _CropCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ClipboardMode
    mode;

  RectangleInfo
    highlight_info;

  const char
    **RectifyModeMenu;

  const ModeType
    *RectifyCommands;
} CropCommandInput;

static void XCropRectifyCommand(const CropCommandInput *command_input,
  XEvent *event,size_t *state)
{
  Display
    *const display = command_input->display;

  XResourceInfo
    *const resource_info = command_input->resource_info;

  XWindows
    *const windows = command_input->windows;

  const ClipboardMode
    mode = command_input->mode;

  const RectangleInfo
    highlight_info = command_input->highlight_info;

  const char
    **const RectifyModeMenu = command_input->RectifyModeMenu;

  const ModeType
    *const RectifyCommands = command_input->RectifyCommands;

  int
    id;

  /*
    Select a command from the Command widget.
  */
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  id=XCommandWidget(display,windows,RectifyModeMenu,event);
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  if (id >= 0)
    switch (RectifyCommands[id])
    {
      case RectifyCopyCommand:
      {
        (*state)|=ExitState;
        break;
      }
      case RectifyHelpCommand:
      {
        (void) XSetFunction(display,windows->image.highlight_context,
          GXcopy);
        XShowCropHelp(display,resource_info,windows,mode);
        (void) XSetFunction(display,windows->image.highlight_context,
          GXinvert);
        break;
      }
      case RectifyDismissCommand:
      {
        /*
          Prematurely exit.
        */
        (*state)|=EscapeState;
        (*state)|=ExitState;
        break;
      }
      default:
        break;
    }
}

typedef struct _CropEventInput
{
  Display
    *display;

  XWindows
    *windows;

  const XEvent
    *event;

  RectangleInfo
    highlight_info;

  const char
    *text;

  XResourceInfo
    *resource_info;

  ClipboardMode
    mode;
} CropEventInput;

static void XCropButtonRelease(const CropEventInput *event_input,
  const RectangleInfo crop_info)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  const RectangleInfo
    highlight_info = event_input->highlight_info;

  if ((*event).xbutton.window == windows->pan.id)
    if ((highlight_info.x != crop_info.x-windows->image.x) ||
        (highlight_info.y != crop_info.y-windows->image.y))
      XHighlightRectangle(display,windows->image.id,
        windows->image.highlight_context,&highlight_info);
  (void) XSetSelectionOwner(display,XA_PRIMARY,windows->image.id,
    (*event).xbutton.time);
}

typedef struct _CropTargets
{
  int
    *x,
    *y;

  RectangleInfo
    *crop_info;

  size_t
    *state;

  XEvent
    *event;
} CropTargets;

static void XCropSelectionRequest(XEvent *event,const RectangleInfo crop_info,
  char *text)
{
  XSelectionEvent
    notify;

  XSelectionRequestEvent
    *request;

  /*
    Set primary selection.
  */
  (void) FormatLocaleString(text,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) crop_info.width,(double)
    crop_info.height,(double) crop_info.x,(double) crop_info.y);
  request=(&((*event).xselectionrequest));
  (void) XChangeProperty(request->display,request->requestor,
    request->property,request->target,8,PropModeReplace,
    (unsigned char *) text,(int) strlen(text));
  notify.type=SelectionNotify;
  notify.display=request->display;
  notify.requestor=request->requestor;
  notify.selection=request->selection;
  notify.target=request->target;
  notify.time=request->time;
  if (request->property == None)
    notify.property=request->target;
  else
    notify.property=request->property;
  (void) XSendEvent(request->display,request->requestor,False,0,
    (XEvent *) &notify);
}

static void XCropMotion(const CropEventInput *event_input,
  const CropTargets *targets)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  if ((*event).xmotion.window != windows->image.id)
    return;
  /*
    Map and unmap Info widget as text cursor crosses its boundaries.
  */
  (*x)=(*event).xmotion.x;
  (*y)=(*event).xmotion.y;
  XTrackInfoWidget(display,windows,(*x),(*y));
  (*crop_info).x=(ssize_t) windows->image.x+(*event).xmotion.x;
  (*crop_info).y=(ssize_t) windows->image.y+(*event).xmotion.y;
}

static void XCropExpose(const CropEventInput *event_input,XEvent *event)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  const RectangleInfo
    highlight_info = event_input->highlight_info;

  const char
    *const text = event_input->text;

  if (((*event).xexpose.window == windows->image.id) &&
      ((*event).xexpose.count == 0))
    {
      (*event).xexpose.x=(int) highlight_info.x;
      (*event).xexpose.y=(int) highlight_info.y;
      (*event).xexpose.width=(int) highlight_info.width;
      (*event).xexpose.height=(int) highlight_info.height;
      XRefreshWindow(display,&windows->image,event);
    }
  if (((*event).xexpose.window == windows->info.id) &&
      ((*event).xexpose.count == 0))
    XInfoWidget(display,windows,text);
}

static void XCropButtonPress(const CropEventInput *event_input,
  const CropTargets *targets)
{
  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  size_t
    *const state = targets->state;

  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  (*x)=windows->image.x+(*event).xbutton.x;
  (*y)=windows->image.y+(*event).xbutton.y;
  if (IsNearTopLeftCorner((*x),(*y),(*crop_info)) != MagickFalse)
    {
      (*crop_info).x=(*crop_info).x+(ssize_t) (*crop_info).width;
      (*crop_info).y=(*crop_info).y+(ssize_t) (*crop_info).height;
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearBottomLeftCorner((*x),(*y),(*crop_info)) != MagickFalse)
    {
      (*crop_info).x=((*crop_info).x+(int) (*crop_info).width);
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearTopRightCorner((*x),(*y),(*crop_info)) != MagickFalse)
    {
      (*crop_info).y=((*crop_info).y+(ssize_t) (*crop_info).height);
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearBottomRightCorner((*x),(*y),(*crop_info)) != MagickFalse)
    {
      (*state)|=UpdateConfigurationState;
      return;
    }
  XCropButtonRelease(event_input,(*crop_info));
}

static void XCropKeyHelp(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const ClipboardMode mode)
{
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  switch (mode)
  {
    case CopyMode:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Copy",ImageCopyHelp);
      }
      break;
    }
    case CropMode:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Cropg",ImageCropHelp);
      }
      break;
    }
    case CutMode:
    {
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Cutg",ImageCutHelp);
      }
      break;
    }
  }
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
}

static void XCropDownHelpKey(const CropEventInput *event_input,
  const CropTargets *targets,const KeySym key_symbol)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  XResourceInfo
    *const resource_info = event_input->resource_info;

  const ClipboardMode
    mode = event_input->mode;

  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_Down:
    case XK_KP_Down:
    {
      (*crop_info).y++;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XCropKeyHelp(display,resource_info,windows,mode);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static void XCropUpRightKey(const CropEventInput *event_input,
  const CropTargets *targets,const KeySym key_symbol)
{
  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((int) key_symbol)
  {
    case XK_Up:
    case XK_KP_Up:
    case XK_Next:
    {
      (*crop_info).y--;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      (*crop_info).x++;
      break;
    }
    default:
    {
      XCropDownHelpKey(event_input,targets,key_symbol);
      break;
    }
  }
}

static void XCropHomeLeftKey(const CropEventInput *event_input,
  const CropTargets *targets,const KeySym key_symbol)
{
  XWindows
    *const windows = event_input->windows;

  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      (*crop_info).x=(ssize_t) (windows->image.width/2L-(*crop_info).width/
        2L);
      (*crop_info).y=(ssize_t) (windows->image.height/2L-(*crop_info).height/
        2L);
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      (*crop_info).x--;
      break;
    }
    default:
    {
      XCropUpRightKey(event_input,targets,key_symbol);
      break;
    }
  }
}

static void XCropKeyPress(const CropEventInput *event_input,XEvent *event,
  const CropTargets *targets)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      (*state)|=EscapeState;
      magick_fallthrough;
    }
    case XK_Return:
    {
      (*state)|=ExitState;
      break;
    }
    default:
    {
      XCropHomeLeftKey(event_input,targets,key_symbol);
      break;
    }
  }
  (void) XSetSelectionOwner(display,XA_PRIMARY,windows->image.id,
    (*event).xkey.time);
}

static const ModeType
  CropCommands[] =
  {
    CropHelpCommand,
    CropDismissCommand
  },
  RectifyCommands[] =
  {
    RectifyCopyCommand,
    RectifyHelpCommand,
    RectifyDismissCommand
  };

typedef struct _CropContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ClipboardMode
    mode;

  const char
    *const *CropModeMenu;

  Cursor
    cursor;

  ExceptionInfo
    *exception;

  char
    *text;

  const char
    **RectifyModeMenu;
} CropContext;

static void XCropPointerCommand(const CropContext *context,
  const CropTargets *targets)
{
  int
    id;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(context->display,context->windows,context->CropModeMenu,
    event);
  if (id < 0)
    return;
  switch (CropCommands[id])
  {
    case CropHelpCommand:
    {
      XShowCropHelp(context->display,context->resource_info,context->windows,
        context->mode);
      break;
    }
    case CropDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static void XCropPointerPress(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const crop_info = targets->crop_info;

  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != context->windows->image.id)
    return;
  /*
    Note first corner of cropping rectangle-- exit loop.
  */
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
  (*crop_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
  (*crop_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
  (*state)|=ExitState;
}

static void XCropPointerKey(const CropContext *context,
  const CropTargets *targets)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  if ((*event).xkey.window != context->windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XShowCropHelp(context->display,context->resource_info,context->windows,
        context->mode);
      break;
    }
    default:
    {
      (void) XBell(context->display,0);
      break;
    }
  }
}

static void XCropPointerEvent(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XCropPointerPress(context,targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      XCropPointerKey(context,targets);
      break;
    }
    case MotionNotify:
    {
      if ((*event).xmotion.window != context->windows->image.id)
        break;
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(context->display,context->windows,(*x),(*y));
      (*crop_info).x=(ssize_t) context->windows->image.x+(*x);
      (*crop_info).y=(ssize_t) context->windows->image.y+(*y);
      break;
    }
    default:
      break;
  }
}

static void XCropTrackPointer(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const crop_info = targets->crop_info;

  do
  {
    if (context->windows->info.mapped != MagickFalse)
      {
        /*
          Display pointer position.
        */
        (void) FormatLocaleString(context->text,MagickPathExtent," %+ld%+ld ",
          (long) (*crop_info).x,(long) (*crop_info).y);
        XInfoWidget(context->display,context->windows,context->text);
      }
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XCropPointerCommand(context,targets);
        continue;
      }
    XCropPointerEvent(context,targets);
  } while (((*state) & ExitState) == 0);
}

static void XCropSizeEvent(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      (*crop_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
      (*crop_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
      break;
    }
    case ButtonRelease:
    {
      /*
        User has committed to cropping rectangle.
      */
      (*crop_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
      (*crop_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
      XSetCursorState(context->display,context->windows,MagickFalse);
      (*state)|=ExitState;
      context->windows->command.data=0;
      (void) XCommandWidget(context->display,context->windows,
        context->RectifyModeMenu,(XEvent *) NULL);
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      (*crop_info).x=(ssize_t) context->windows->image.x+(*event).xmotion.x;
      (*crop_info).y=(ssize_t) context->windows->image.y+(*event).xmotion.y;
    }
    default:
      break;
  }
}

static void XHighlightCropRegion(RectangleInfo *highlight_info,
  const CropContext *context,RectangleInfo *crop_info)
{
  if (((*highlight_info).width > 3) && ((*highlight_info).height > 3))
    {
      /*
        Display info and draw cropping rectangle.
      */
      XMapInfoWidget(context->display,context->windows);
      (void) FormatLocaleString(context->text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) (*crop_info).width,(double)
        (*crop_info).height,(double) (*crop_info).x,(double) (*crop_info).y);
      XInfoWidget(context->display,context->windows,context->text);
      XHighlightRectangle(context->display,context->windows->image.id,
        context->windows->image.highlight_context,highlight_info);
    }
  else
    if (context->windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(context->display,context->windows->info.id,
        context->windows->info.screen);
}

static void XCropDrawRegion(const CropContext *context,
  const CropTargets *targets,RectangleInfo *highlight_info)
{
  RectangleInfo
    *const crop_info = targets->crop_info;

  (*highlight_info)=(*crop_info);
  (*highlight_info).x=(*crop_info).x-context->windows->image.x;
  (*highlight_info).y=(*crop_info).y-context->windows->image.y;
  XHighlightCropRegion(highlight_info,context,crop_info);
}

static void XCropSizeRegion(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  RectangleInfo
    highlight_info;

  do
  {
    XCropDrawRegion(context,targets,&highlight_info);
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((highlight_info.width > 3) && (highlight_info.height > 3))
      XHighlightRectangle(context->display,context->windows->image.id,
        context->windows->image.highlight_context,&highlight_info);
    XCropSizeEvent(context,targets);
    if (IsSelectionMovedOrDone((*crop_info),(*x),(*y),(*state)) != MagickFalse)
      XCheckSelectionBounds(context->windows,(*x),(*y),crop_info);
  } while (((*state) & ExitState) == 0);
}

static void XCropWaitCommand(const CropContext *context,
  const CropTargets *targets,const RectangleInfo highlight_info)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  CropCommandInput
    command_input;

  command_input.display=context->display;
  command_input.resource_info=context->resource_info;
  command_input.windows=context->windows;
  command_input.mode=context->mode;
  command_input.highlight_info=highlight_info;
  command_input.RectifyModeMenu=context->RectifyModeMenu;
  command_input.RectifyCommands=RectifyCommands;
  XCropRectifyCommand(&command_input,event,state);
}

static void XCropWaitEvent(const CropContext *context,
  const CropTargets *targets,const RectangleInfo highlight_info)
{
  CropEventInput
    event_input;

  XEvent
    *const event = targets->event;

  RectangleInfo
    *const crop_info = targets->crop_info;

  event_input.display=context->display;
  event_input.windows=context->windows;
  event_input.event=(event);
  event_input.highlight_info=highlight_info;
  event_input.text=context->text;
  event_input.resource_info=context->resource_info;
  event_input.mode=context->mode;
  switch ((*event).type)
  {
    case ButtonPress:
    {
      XCropButtonPress(&event_input,targets);
      break;
    }
    case ButtonRelease:
    {
      XCropButtonRelease(&event_input,(*crop_info));
      break;
    }
    case Expose:
    {
      XCropExpose(&event_input,event);
      break;
    }
    case KeyPress:
    {
      XCropKeyPress(&event_input,event,targets);
      break;
    }
    case MotionNotify:
    {
      XCropMotion(&event_input,targets);
      break;
    }
    case SelectionRequest:
    {
      XCropSelectionRequest(event,(*crop_info),context->text);
    }
    default:
      break;
  }
}

static void XShowCropGeometryInfo(const CropContext *context,
  RectangleInfo *crop_info)
{
  if (context->windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(context->text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) (*crop_info).width,(double)
        (*crop_info).height,(double) (*crop_info).x,(double) (*crop_info).y);
      XInfoWidget(context->display,context->windows,context->text);
    }
}

static MagickBooleanType XCropWaitHead(const CropContext *context,
  const CropTargets *targets,RectangleInfo *highlight_info)
{
  size_t
    *const state = targets->state;

  RectangleInfo
    *const crop_info = targets->crop_info;

  XShowCropGeometryInfo(context,crop_info);
  (*highlight_info)=(*crop_info);
  (*highlight_info).x=(*crop_info).x-context->windows->image.x;
  (*highlight_info).y=(*crop_info).y-context->windows->image.y;
  if (((*highlight_info).width <= 3) || ((*highlight_info).height <= 3))
    {
      (*state)|=EscapeState;
      (*state)|=ExitState;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XCropWaitCorner(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    highlight_info;

  do
  {
    if (XCropWaitHead(context,targets,&highlight_info) == MagickFalse)
      break;
    XHighlightRectangle(context->display,context->windows->image.id,
      context->windows->image.highlight_context,&highlight_info);
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XCropWaitCommand(context,targets,highlight_info);
        continue;
      }
    XHighlightRectangle(context->display,context->windows->image.id,
      context->windows->image.highlight_context,&highlight_info);
    XCropWaitEvent(context,targets,highlight_info);
    if (((*state) & UpdateConfigurationState) != 0)
      {
        (void) XPutBackEvent(context->display,event);
        (void) XCheckDefineCursor(context->display,context->windows->image.id,
          context->cursor);
        break;
      }
  } while (((*state) & ExitState) == 0);
}

static void XCropSelectRegion(const CropContext *context,
  const CropTargets *targets)
{
  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  do
  {
    /*
      Size rectangle as pointer moves until the mouse button is released.
    */
    (*x)=(int) (*crop_info).x;
    (*y)=(int) (*crop_info).y;
    (*crop_info).width=0;
    (*crop_info).height=0;
    (*state)=DefaultState;
    XCropSizeRegion(context,targets);
    /*
      Wait for user to grab a corner of the rectangle or press return.
    */
    (*state)=DefaultState;
    (void) XMapWindow(context->display,context->windows->info.id);
    XCropWaitCorner(context,targets);
  } while (((*state) & ExitState) == 0);
}

static void XCropCutPixels(Image *image,const RectangleInfo crop_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  int
    x,
    y;

  Quantum
    *q;

  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; y < (int) crop_info.height; y++)
  {
    q=GetCacheViewAuthenticPixels(image_view,crop_info.x,y+crop_info.y,
      crop_info.width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) crop_info.width; x++)
    {
      SetPixelAlpha(image,TransparentAlpha,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
  image_view=DestroyCacheView(image_view);
}

static MagickBooleanType XCropCopyImage(const CropContext *context,Image *image,
  RectangleInfo crop_info)
{
  Image
    *crop_image;

  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  /*
    Copy image before applying image transforms.
  */
  XSetCursorState(context->display,context->windows,MagickTrue);
  XCheckRefreshWindows(context->display,context->windows);
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (context->windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(context->windows->image.crop_geometry,&x,&y,&width,
      &height);
  scale_factor=(double) width/context->windows->image.ximage->width;
  crop_info.x+=x;
  crop_info.x=(ssize_t) (scale_factor*crop_info.x+0.5);
  crop_info.width=(unsigned int) (scale_factor*crop_info.width+0.5);
  scale_factor=(double) height/context->windows->image.ximage->height;
  crop_info.y+=y;
  crop_info.y=(ssize_t) (scale_factor*crop_info.y+0.5);
  crop_info.height=(unsigned int) (scale_factor*crop_info.height+0.5);
  crop_info.x+=image->page.x;
  crop_info.y+=image->page.y;
  crop_image=CropImage(image,&crop_info,context->exception);
  XSetCursorState(context->display,context->windows,MagickFalse);
  if (crop_image == (Image *) NULL)
    return(MagickFalse);
  if (context->resource_info->copy_image != (Image *) NULL)
    context->resource_info->copy_image=DestroyImage(
      context->resource_info->copy_image);
  context->resource_info->copy_image=crop_image;
  if (context->mode == CopyMode)
    {
      (void) XConfigureImage(context->display,context->resource_info,
        context->windows,image,context->exception);
      return(MagickTrue);
    }
  /*
    Cut image.
  */
  if (SetImageStorageClass(image,DirectClass,context->exception) == MagickFalse)
    return(MagickFalse);
  image->alpha_trait=BlendPixelTrait;
  XCropCutPixels(image,crop_info,context->exception);
  /*
    Update image configuration.
  */
  XConfigureImageColormap(context->display,context->resource_info,
    context->windows,image,context->exception);
  (void) XConfigureImage(context->display,context->resource_info,
    context->windows,image,context->exception);
  return(MagickTrue);
}

static void XCropSelectArea(const CropContext *context,
  const CropTargets *targets)
{
  (void) XSetFunction(context->display,
    context->windows->image.highlight_context,GXinvert);
  XCropSelectRegion(context,targets);
  (void) XSetFunction(context->display,
    context->windows->image.highlight_context,GXcopy);
  XSetCursorState(context->display,context->windows,MagickFalse);
}

static MagickBooleanType XCropEscaped(const CropContext *context,
  const CropTargets *targets)
{
  size_t
    *const state = targets->state;

  if (((*state) & EscapeState) != 0)
    {
      /*
        User want to exit without cropping.
      */
      (void) XWithdrawWindow(context->display,context->windows->info.id,
        context->windows->info.screen);
      (void) XFreeCursor(context->display,context->cursor);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XMapCropCommand(Display *display,XWindows *windows,
  const char *const *CropModeMenu)
{
  windows->command.data=0;
  (void) XCommandWidget(display,windows,CropModeMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
}

static void XSetCropCommandName(XWindows *windows,const ClipboardMode mode)
{
  switch (mode)
  {
    case CopyMode:
    {
      (void) CloneString(&windows->command.name,"Copy");
      break;
    }
    case CropMode:
    {
      (void) CloneString(&windows->command.name,"Crop");
      break;
    }
    case CutMode:
    {
      (void) CloneString(&windows->command.name,"Cut");
      break;
    }
  }
}

static MagickBooleanType XCropReconfigure(const CropContext *context,
  Image *image,RectangleInfo crop_info)
{
  if (context->mode == CropMode)
    if (((int) crop_info.width != context->windows->image.ximage->width) ||
        ((int) crop_info.height != context->windows->image.ximage->height))
      {
        /*
          Reconfigure Image window as defined by cropping rectangle.
        */
        XSetCropGeometry(context->display,context->windows,&crop_info,image);
        context->windows->image.window_changes.width=(int) crop_info.width;
        context->windows->image.window_changes.height=(int) crop_info.height;
        (void) XConfigureImage(context->display,context->resource_info,
          context->windows,image,context->exception);
        return(MagickFalse);
      }
  return(MagickTrue);
}

static MagickBooleanType XCropImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  const ClipboardMode mode,ExceptionInfo *exception)
{
  const char
    *const CropModeMenu[] =
    {
      "Help",
      "Dismiss",
      (char *) NULL
    },
    *RectifyModeMenu[] =
    {
      "Crop",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  char
    text[MagickPathExtent];

  Cursor
    cursor;

  int
    x,
    y;

  RectangleInfo
    crop_info;

  size_t
    state;

  XEvent
    event;

  CropContext
    context;

  const CropTargets
    targets = { &x, &y, &crop_info, &state, &event };

  /*
    Map Command widget.
  */
  XSetCropCommandName(windows,mode);
  RectifyModeMenu[0]=windows->command.name;
  XMapCropCommand(display,windows,CropModeMenu);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  crop_info.x=(ssize_t) windows->image.x+x;
  crop_info.y=(ssize_t) windows->image.y+y;
  crop_info.width=0;
  crop_info.height=0;
  cursor=XCreateFontCursor(display,XC_fleur);
  state=DefaultState;
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.mode=mode;
  context.CropModeMenu=CropModeMenu;
  context.exception=exception;
  context.text=text;
  context.RectifyModeMenu=RectifyModeMenu;
  context.cursor=cursor;
  XCropTrackPointer(&context,&targets);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  if (XCropEscaped(&context,&targets) == MagickFalse)
    return(MagickTrue);
  XCropSelectArea(&context,&targets);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  if (XCropReconfigure(&context,image,crop_info) == MagickFalse)
    return(MagickTrue);
  return(XCropCopyImage(&context,image,crop_info));
}
static inline MagickBooleanType IsPolygonElement(const ElementType v)
{
  switch (v)
  {
    case PointElement:
    case PolygonElement:
    case FillPolygonElement:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static inline MagickBooleanType IsBoxedElement(const ElementType v)
{
  switch (v)
  {
    case RectangleElement:
    case CircleElement:
    case EllipseElement:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static inline MagickBooleanType IsElementComplete(const ElementType element,
  const unsigned int distance)
{
  return((MagickBooleanType) (((element != PolygonElement) &&
    (element != FillPolygonElement)) || (distance <= 9)));
}

typedef struct _DrawEditContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *DrawMenu;

  char
    *command;

  Window
    root_window;

  ExceptionInfo
    *exception;

  Image
    **image;

  Cursor
    cursor;

  const ModeType
    *DrawCommands;
} DrawEditContext;

typedef struct _DrawStyleTargets
{
  XDrawInfo
    *draw_info;

  unsigned int
    *pen_id;

  unsigned int
    *line_width;

  Pixmap
    *stipple;

  int
    *x;

  int
    *y;

  MagickStatusType
    *status;
} DrawStyleTargets;

static void XSelectDrawColor(const DrawEditContext *context,const int id,
  const DrawStyleTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const char
    *const *const DrawMenu = context->DrawMenu;

  char
    *const command = context->command;

  XDrawInfo
    *const draw_info = targets->draw_info;

  unsigned int
    *const pen_id = targets->pen_id;

  const char
    *ColorMenu[MaxNumberPens+1];

  int
    pen_number;

  MagickBooleanType
    transparent;

  XColor
    color;

  int
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="transparent";
  ColorMenu[MaxNumberPens-1]="Browser...";
  ColorMenu[MaxNumberPens]=(char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { DrawMenu[id], (const char **) ColorMenu, command };

    pen_number=XMenuWidget(display,windows,&menu_request);
  }
  if (pen_number < 0)
    return;
  transparent=(MagickBooleanType) (pen_number == (MaxNumberPens-2));
  if (transparent != MagickFalse)
    {
      (*draw_info).stencil=TransparentStencil;
      return;
    }
  if (pen_number == (MaxNumberPens-1))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set pen color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],&color);
  {
    const XBestPixelTarget
      pixel_target = { display, windows->map_info->colormap };

    XBestPixel(&pixel_target,(XColor *) NULL,(unsigned int) MaxColors,&color);
  }
  windows->pixel_info->pen_colors[pen_number]=color;
  (*pen_id)=(unsigned int) pen_number;
  (*draw_info).stencil=OpaqueStencil;
}

static void XSelectDrawWidth(const DrawEditContext *context,const int id,
  const DrawStyleTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const DrawMenu = context->DrawMenu;

  char
    *const command = context->command;

  unsigned int
    *const line_width = targets->line_width;

  const char
    *const WidthsMenu[] =
    {
      "1",
      "2",
      "4",
      "8",
      "16",
      "Dialog...",
      (char *) NULL,
    };

  static char
    width[MagickPathExtent] = "0";

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { DrawMenu[id], WidthsMenu, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry < 0)
    return;
  if (entry != 5)
    {
      (*line_width)=(unsigned int) StringToUnsignedLong(
        WidthsMenu[entry]);
      return;
    }
  {
    const XDialogRequest
      dialog_request = { "Ok", "Enter line width:", width };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
  if (*width == '\0')
    return;
  (*line_width)=(unsigned int) StringToUnsignedLong(width);
}

static void XCreateStippleBitmapTail(Display *display,const Window root_window,
  const int entry,Pixmap *stipple)
{
  switch (entry)
  {
    case 4:
    {
      (*stipple)=XCreateBitmapFromData(display,root_window,
        (char *) WavyBitmap,WavyWidth,WavyHeight);
      break;
    }
    case 5:
    {
      (*stipple)=XCreateBitmapFromData(display,root_window,
        (char *) HighlightBitmap,HighlightWidth,
        HighlightHeight);
      break;
    }
    case 6:
    default:
    {
      (*stipple)=XCreateBitmapFromData(display,root_window,
        (char *) OpaqueBitmap,OpaqueWidth,OpaqueHeight);
      break;
    }
  }
}

#endif
