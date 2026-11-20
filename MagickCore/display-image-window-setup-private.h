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

  MagickCore private methods of display-image.c: the window positioning,
  configuration, button and key handlers and the image window set-up of
  XDisplayImage, from XPositionCommandAndWidget to IsDynamicVisualClass;
  included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_WINDOW_SETUP_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_WINDOW_SETUP_PRIVATE_H

static void XPositionCommandAndWidget(Display *display,XWindows *windows,
  const XEvent event)
{
  XWindowChanges
    window_changes;

  if ((windows->command.geometry == (char *) NULL) &&
      (windows->command.mapped == MagickFalse))
    {
      windows->command.x=event.xconfigure.x-(int)
        windows->command.width-25;
      windows->command.y=event.xconfigure.y;
      XConstrainWindowPosition(display,&windows->command);
      window_changes.x=windows->command.x;
      window_changes.y=windows->command.y;
      (void) XReconfigureWMWindow(display,windows->command.id,
        windows->command.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
  if ((windows->widget.geometry == (char *) NULL) &&
      (windows->widget.mapped == MagickFalse))
    {
      windows->widget.x=event.xconfigure.x+
        event.xconfigure.width/10;
      windows->widget.y=event.xconfigure.y+
        event.xconfigure.height/10;
      XConstrainWindowPosition(display,&windows->widget);
      window_changes.x=windows->widget.x;
      window_changes.y=windows->widget.y;
      (void) XReconfigureWMWindow(display,windows->widget.id,
        windows->widget.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
}

static void XPositionMagnifyAndPan(Display *display,XWindows *windows,
  const XEvent event)
{
  XWindowChanges
    window_changes;

  if ((windows->magnify.geometry == (char *) NULL) &&
      (windows->magnify.mapped == MagickFalse))
    {
      windows->magnify.x=event.xconfigure.x+
        event.xconfigure.width+25;
      windows->magnify.y=event.xconfigure.y;
      XConstrainWindowPosition(display,&windows->magnify);
      window_changes.x=windows->magnify.x;
      window_changes.y=windows->magnify.y;
      (void) XReconfigureWMWindow(display,windows->magnify.id,
        windows->magnify.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
  if ((windows->pan.geometry == (char *) NULL) &&
      (windows->pan.mapped == MagickFalse))
    {
      windows->pan.x=event.xconfigure.x+(int)
        event.xconfigure.width+25;
      windows->pan.y=event.xconfigure.y+(int)
        windows->magnify.height+50;
      XConstrainWindowPosition(display,&windows->pan);
      window_changes.x=windows->pan.x;
      window_changes.y=windows->pan.y;
      (void) XReconfigureWMWindow(display,windows->pan.id,
        windows->pan.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
}

static void XUpdatePanWindow(Display *display,XWindows *windows,
  const XEvent event)
{
  if ((event.xconfigure.width < windows->image.ximage->width) ||
      (event.xconfigure.height < windows->image.ximage->height))
    {
      (void) XMapRaised(display,windows->pan.id);
      XDrawPanRectangle(display,windows);
    }
  else
    if (windows->pan.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->pan.id,
        windows->pan.screen);
}

typedef struct _DisplayEventContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  XPixelInfo
    *pixel;

  XStandardColormap
    *map_info;

  XWindowInfo
    **magick_windows;

  unsigned int
    number_windows;

  MagickStatusType
    context_mask;

  size_t
    *state;

  Window
    root_window;

  const char
    *const *CommandMenu;

  char
    *command;

  XVisualInfo
    *visual_info;

  const char
    *const *VirtualMenu;

  DisplayCommand
    *VirtualCommands;

  const char
    *const *ShortCutsMenu;

  DisplayCommand
    *ShortCutsCommands;

  Image
    **image;

  time_t
    update_time;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;

  const char *const
    **Menus;

  DisplayCommand
    **Commands;

  XFontStruct
    *font_info;

  char
    **argv;

  int
    argc;

  XClassHint
    *class_hints;

  XWMHints
    *manager_hints;

  XResourceInfo
    *icon_resources;
} DisplayEventContext;

static void XUpdateStaticImageWindow(XWindows *windows,const XEvent event,
  Display *display,XResourceInfo *resource_info,Image *display_image,
  ExceptionInfo *exception)
{
  if (windows->image.mapped != MagickFalse &&
      windows->image.stasis != MagickFalse)
    {
      /*
        Update image window configuration.
      */
      windows->image.window_changes.width=event.xconfigure.width;
      windows->image.window_changes.height=event.xconfigure.height;
      (void) XConfigureImage(display,resource_info,windows,
        display_image,exception);
    }
}

static void XConfigureImageWindow(const DisplayEventContext *context,
  const XEvent event,Image *display_image,const XPoint vid_info)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  /*
    Image window has a new configuration.
  */
  if (event.xconfigure.send_event != 0)
    {
      /*
        Position the transient windows relative of the Image window.
      */
      XPositionCommandAndWidget(display,windows,event);
      XPositionMagnifyAndPan(display,windows,event);
    }
  if ((event.xconfigure.width == (int) windows->image.width) &&
      (event.xconfigure.height == (int) windows->image.height))
    return;
  windows->image.width=(unsigned int) event.xconfigure.width;
  windows->image.height=(unsigned int) event.xconfigure.height;
  windows->image.x=0;
  windows->image.y=0;
  if (display_image->montage != (char *) NULL)
    {
      windows->image.x=vid_info.x;
      windows->image.y=vid_info.y;
    }
  XUpdateStaticImageWindow(windows,event,display,resource_info,display_image,
    exception);
  /*
    Update pan window configuration.
  */
  XUpdatePanWindow(display,windows,event);
}

static MagickStatusType XConfigureMagnifyWindow(
  const DisplayEventContext *context,const XEvent event,Image *display_image,
  MagickStatusType status)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  unsigned int
    magnify;

  XWindowChanges
    window_changes;

  /*
    Magnify window has a new configuration.
  */
  windows->magnify.width=(unsigned int) event.xconfigure.width;
  windows->magnify.height=(unsigned int) event.xconfigure.height;
  if (windows->magnify.mapped == MagickFalse)
    return(status);
  magnify=1;
  while ((int) magnify <= event.xconfigure.width)
    magnify<<=1;
  while ((int) magnify <= event.xconfigure.height)
    magnify<<=1;
  magnify>>=1;
  if (((int) magnify != event.xconfigure.width) ||
      ((int) magnify != event.xconfigure.height))
    {
      window_changes.width=(int) magnify;
      window_changes.height=(int) magnify;
      (void) XReconfigureWMWindow(display,windows->magnify.id,
        windows->magnify.screen,(unsigned int) (CWWidth | CWHeight),
        &window_changes);
      return(status);
    }
  if (windows->magnify.mapped != MagickFalse &&
      windows->magnify.stasis != MagickFalse)
    {
      status=XMakeImage(display,resource_info,&windows->magnify,
        display_image,windows->magnify.width,windows->magnify.height,
        exception);
      XMakeMagnifyImage(display,windows,exception);
    }
  return(status);
}

static void XConfigurePanWindow(XWindows *windows,const XEvent event)
{
  /*
    Pan icon window has a new configuration.
  */
  if (event.xconfigure.send_event != 0)
    {
      windows->pan.x=event.xconfigure.x;
      windows->pan.y=event.xconfigure.y;
    }
  windows->pan.width=(unsigned int) event.xconfigure.width;
  windows->pan.height=(unsigned int) event.xconfigure.height;
}

static void XUpdateWindowColormaps(const DisplayEventContext *context,
  XGCValues *context_values)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  XWindowInfo
    **const magick_windows = context->magick_windows;

  const unsigned int
    number_windows = context->number_windows;

  const MagickStatusType
    context_mask = context->context_mask;

  int
    i;

  /*
    Update graphic context and window colormap.
  */
  for (i=0; i < (int) number_windows; i++)
  {
    if (magick_windows[i]->id == windows->icon.id)
      continue;
    (*context_values).background=pixel->background_color.pixel;
    (*context_values).foreground=pixel->foreground_color.pixel;
    (void) XChangeGC(display,magick_windows[i]->annotate_context,
      context_mask,context_values);
    (void) XChangeGC(display,magick_windows[i]->widget_context,
      context_mask,context_values);
    (*context_values).background=pixel->foreground_color.pixel;
    (*context_values).foreground=pixel->background_color.pixel;
    (*context_values).plane_mask=(*context_values).background ^
      (*context_values).foreground;
    (void) XChangeGC(display,magick_windows[i]->highlight_context,
      (size_t) (context_mask | GCPlaneMask),
      context_values);
    magick_windows[i]->attributes.background_pixel=
      pixel->background_color.pixel;
    magick_windows[i]->attributes.border_pixel=
      pixel->border_color.pixel;
    magick_windows[i]->attributes.colormap=map_info->colormap;
    (void) XChangeWindowAttributes(display,magick_windows[i]->id,
      (unsigned long) magick_windows[i]->mask,
      &magick_windows[i]->attributes);
  }
  if (windows->pan.mapped != MagickFalse)
    {
      (void) XSetWindowBackgroundPixmap(display,windows->pan.id,
        windows->pan.pixmap);
      (void) XClearWindow(display,windows->pan.id);
      XDrawPanRectangle(display,windows);
    }
  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
}

static Image *XDropImage(const DisplayEventContext *context,const XEvent event,
  Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  const Window
    root_window = context->root_window;

  Atom
    selection,
    type;

  int
    format,
    status;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  /*
    Display image named by the Drag-and-Drop selection.
  */
  if ((*event.xclient.data.l != 2) && (*event.xclient.data.l != 128))
    return(nexus);
  selection=XInternAtom(display,"DndSelection",MagickFalse);
  status=XGetWindowProperty(display,root_window,selection,0L,(long)
    MagickPathExtent,MagickFalse,(Atom) AnyPropertyType,&type,&format,
    &length,&after,&data);
  if ((status != Success) || (length == 0))
    return(nexus);
  if (*event.xclient.data.l == 2)
    {
      /*
        Offix DND.
      */
      (void) CopyMagickString(resource_info->image_info->filename,
        (char *) data,MagickPathExtent);
    }
  else
    {
      /*
        XDND.
      */
      if (strncmp((char *) data, "file:", 5) != 0)
        {
          (void) XFree((void *) data);
          return(nexus);
        }
      (void) CopyMagickString(resource_info->image_info->filename,
        ((char *) data)+5,MagickPathExtent);
    }
  nexus=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if (nexus != (Image *) NULL)
    *state|=NextImageState | ExitState;
  (void) XFree((void *) data);
  return(nexus);
}

typedef struct _DisplayEventTargets
{
  Image
    **nexus;

  Image
    **display_image;

  XEvent
    *event;

  XPoint
    *vid_info;

  time_t
    *timer;

  XGCValues
    *context_values;

  MagickStatusType
    *status;

  WarningHandler
    *warning_handler;

  XWindows
    **windows;

  unsigned int
    *number_windows;
} DisplayEventTargets;

static void XSelectVirtualCommand(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const command = context->command;

  const char
    *const *const VirtualMenu = context->VirtualMenu;

  DisplayCommand
    *const VirtualCommands = context->VirtualCommands;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  int
    entry;

  const XImageCacheContext
    display_context = { display, resource_info, windows, display_image,
      exception };

  /*
    Select a command from the Virtual menu.
  */
  {
    const XMenuRequest
      menu_request = { "Commands", VirtualMenu, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry >= 0)
    (*nexus)=XMagickCommand(&display_context,VirtualCommands[entry]);
}

static void XPressImageButton1(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const char
    *const *const CommandMenu = context->CommandMenu;

  if (resource_info->immutable)
    {
      XSelectVirtualCommand(context,targets);
      return;
    }
  /*
    Map/unmap Command widget.
  */
  if (windows->command.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->command.id,
      windows->command.screen);
  else
    {
      (void) XCommandWidget(display,windows,CommandMenu,
        (XEvent *) NULL);
      (void) XMapRaised(display,windows->command.id);
    }
}

static void XPressImageButton3(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  char
    *const command = context->command;

  const char
    *const *const ShortCutsMenu = context->ShortCutsMenu;

  DisplayCommand
    *const ShortCutsCommands = context->ShortCutsCommands;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  XPoint
    *const vid_info = targets->vid_info;

  int
    entry;

  const XImageCacheContext
    display_context = { display, resource_info, windows, display_image,
      exception };

  if (resource_info->immutable)
    {
      XSelectVirtualCommand(context,targets);
      return;
    }
  if ((*display_image)->montage != (char *) NULL)
    {
      /*
        Open or delete a tile from a visual image directory.
      */
      (*nexus)=XTileImage(display,resource_info,windows,
        (*display_image),event,exception);
      if ((*nexus) != (Image *) NULL)
        *state|=MontageImageState | NextImageState | ExitState;
      (*vid_info).x=(short int) windows->image.x;
      (*vid_info).y=(short int) windows->image.y;
      return;
    }
  /*
    Select a command from the Short Cuts menu.
  */
  {
    const XMenuRequest
      menu_request = { "Short Cuts", ShortCutsMenu, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry >= 0)
    (*nexus)=XMagickCommand(&display_context,ShortCutsCommands[entry]);
}

static void XPressImageWindow(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  const XImageCacheContext
    display_context = { display, resource_info, windows, display_image,
      exception };

  switch ((*event).xbutton.button)
  {
    case Button1:
    {
      XPressImageButton1(context,targets);
      break;
    }
    case Button2:
    {
      /*
        User pressed the image magnify button.
      */
      (void) XMagickCommand(&display_context,ZoomCommand);
      XMagnifyImage(display,windows,event,exception);
      break;
    }
    case Button3:
    {
      XPressImageButton3(context,targets);
      break;
    }
    case Button4:
    {
      /*
        Wheel up.
      */
      XTranslateImage(display,windows,*image,XK_Up);
      break;
    }
    case Button5:
    {
      /*
        Wheel down.
      */
      XTranslateImage(display,windows,*image,XK_Down);
      break;
    }
    default:
      break;
  }
}

static void XPressMagnifyWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const command = context->command;

  const char
    *const MagnifyMenu[] =
    {
      "2",
      "4",
      "5",
      "6",
      "7",
      "8",
      "9",
      "3",
      (char *) NULL,
    };

  int
    factor;

  const MagnifyWindowInput
    input = { display, windows, exception };

  static KeySym
    MagnifyCommands[] =
    {
      XK_2,
      XK_4,
      XK_5,
      XK_6,
      XK_7,
      XK_8,
      XK_9,
      XK_3
    };

  /*
    Select a magnify factor from the pop-up menu.
  */
  {
    const XMenuRequest
      menu_request = { "Magnify", MagnifyMenu, command };

    factor=XMenuWidget(display,windows,&menu_request);
  }
  if (factor >= 0)
    XMagnifyWindowCommand(&input,0,MagnifyCommands[factor]);
}

static void XPressPanWindow(const DisplayEventContext *context,XEvent *event)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  switch ((*event).xbutton.button)
  {
    case Button4:
    {
      /*
        Wheel up.
      */
      XTranslateImage(display,windows,*image,XK_Up);
      break;
    }
    case Button5:
    {
      /*
        Wheel down.
      */
      XTranslateImage(display,windows,*image,XK_Down);
      break;
    }
    default:
    {
      XPanImage(display,windows,event,exception);
      break;
    }
  }
}

static void XImageWindowKeyPress(const DisplayEventContext *context,
  const XEvent event,const KeySym key_symbol,const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  DisplayCommand
    display_command;

  const ImageWindowKeyInput
    input = { display, resource_info, windows, display_image, exception };

  const XImageCacheContext
    display_context = { display, resource_info, windows, display_image,
      exception };

  display_command=XImageWindowCommand(&input,event.xkey.state,key_symbol);
  if (display_command != NullCommand)
    (*nexus)=XMagickCommand(&display_context,display_command);
}

static void XPanWindowKeyPress(const DisplayEventContext *context,
  const KeySym key_symbol)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  if ((key_symbol == XK_q) || (key_symbol == XK_Escape))
    (void) XWithdrawWindow(display,windows->pan.id,
      windows->pan.screen);
  else
    if ((key_symbol == XK_F1) || (key_symbol == XK_Help))
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Pan",ImagePanHelp);
      }
    else
      XTranslateImage(display,windows,*image,key_symbol);
}

static Image *XMapImageWindow(const DisplayEventContext *context,
  Image *display_image,Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XStandardColormap
    *const map_info = context->map_info;

  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
  if ((LocaleCompare(display_image->magick,"LOGO") == 0) &&
      (LocaleCompare(display_image->filename,"LOGO") == 0))
    nexus=XOpenImage(display,resource_info,windows,MagickFalse);
  if (((int) windows->image.width < windows->image.ximage->width) ||
      ((int) windows->image.height < windows->image.ximage->height))
    (void) XMapRaised(display,windows->pan.id);
  windows->image.mapped=MagickTrue;
  return(nexus);
}

static void XFocusImageWindow(Display *display,XWindows *windows)
{
  if (windows->backdrop.id != (Window) NULL)
    (void) XSetInputFocus(display,windows->image.id,RevertToParent,
      CurrentTime);
}

static void XUnmapIconWindow(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XStandardColormap
    *const map_info = context->map_info;

  XVisualInfo
    *const icon_visual = context->icon_visual;

  XStandardColormap
    *const icon_map = context->icon_map;

  XPixelInfo
    *const icon_pixel = context->icon_pixel;

  if (map_info->colormap == icon_map->colormap)
    XConfigureImageColormap(display,resource_info,windows,
      display_image,exception);
  (void) XFreeStandardColormap(display,icon_visual,icon_map,
    icon_pixel);
  windows->icon.mapped=MagickFalse;
}

static void XLogDisplayImage(XResourceInfo *resource_info,Image *display_image)
{
  if (resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",display_image->filename,
        (double) display_image->scene,(double) display_image->columns,
        (double) display_image->rows);
      if (display_image->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          display_image->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        display_image->magick);
    }
}

static time_t XGetUpdateTime(XResourceInfo *resource_info,Image *display_image)
{
  struct stat
    attributes;

  time_t
    update_time;

  update_time=0;
  if (resource_info->update != MagickFalse)
    {
      MagickBooleanType
        status;

      /*
        Determine when file data was last modified.
      */
      status=GetPathAttributes(display_image->filename,&attributes);
      if (status != MagickFalse)
        update_time=attributes.st_mtime;
    }
  return(update_time);
}

static void XNameImageWindow(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  size_t
    *const state = context->state;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,resource_info,
    &windows->image);
  windows->image.shape=MagickTrue;  /* non-rectangular shape hint */
  if (resource_info->use_shared_memory == MagickFalse)
    windows->image.shared_memory=MagickFalse;
  if ((resource_info->title != (char *) NULL) && !(*state & MontageImageState))
    {
      char
        *title;

      title=InterpretImageProperties(resource_info->image_info,display_image,
        resource_info->title,exception);
      (void) CloneString(&windows->image.name,title);
      (void) CloneString(&windows->image.icon_name,title);
      title=DestroyString(title);
    }
  else
    {
      char
        filename[MagickPathExtent],
        window_name[MagickPathExtent];

      /*
        Window name is the base of the filename.
      */
      GetPathComponent(display_image->magick_filename,TailPath,filename);
      if (display_image->scene == 0)
        (void) FormatLocaleString(window_name,MagickPathExtent,"%s: %s",
          MagickPackageName,filename);
      else
        (void) FormatLocaleString(window_name,MagickPathExtent,
          "%s: %s[scene: %.17g frames: %.17g]",MagickPackageName,filename,
          (double) display_image->scene,(double) GetImageListLength(
          display_image));
      (void) CloneString(&windows->image.name,window_name);
      (void) CloneString(&windows->image.icon_name,filename);
    }
}

static void XMakeBackdropWindow(XResourceInfo *resource_info,XWindows *windows,
  Display *display,XVisualInfo *visual_info,XWMHints *manager_hints,
  Window root_window,char **argv,int argc,XClassHint *class_hints)
{
  if ((resource_info->backdrop) || (windows->backdrop.id != (Window) NULL))
    {
      /*
        Initialize backdrop window.
      */
      windows->backdrop.x=0;
      windows->backdrop.y=0;
      (void) CloneString(&windows->backdrop.name,"Backdrop");
      windows->backdrop.flags=(size_t) (USSize | USPosition);
      windows->backdrop.width=(unsigned int)
        XDisplayWidth(display,visual_info->screen);
      windows->backdrop.height=(unsigned int)
        XDisplayHeight(display,visual_info->screen);
      windows->backdrop.border_width=0;
      windows->backdrop.immutable=MagickTrue;
      windows->backdrop.attributes.do_not_propagate_mask=ButtonPressMask |
        ButtonReleaseMask;
      windows->backdrop.attributes.event_mask=ButtonPressMask | KeyPressMask |
        StructureNotifyMask;
      manager_hints->flags=IconWindowHint | InputHint | StateHint;
      manager_hints->icon_window=windows->icon.id;
      manager_hints->input=MagickTrue;
      manager_hints->initial_state=resource_info->iconic ? IconicState :
        NormalState;
      XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
        &windows->backdrop);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (backdrop)",windows->backdrop.id);
      (void) XMapWindow(display,windows->backdrop.id);
      (void) XClearWindow(display,windows->backdrop.id);
      if (windows->image.id != (Window) NULL)
        {
          (void) XDestroyWindow(display,windows->image.id);
          windows->image.id=(Window) NULL;
        }
      /*
        Position image in the center the backdrop.
      */
      windows->image.flags|=USPosition;
      windows->image.x=(XDisplayWidth(display,visual_info->screen)/2)-
        ((int) windows->image.width/2);
      windows->image.y=(XDisplayHeight(display,visual_info->screen)/2)-
        ((int) windows->image.height/2);
    }
}

static void XInitializeBackdropWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->backdrop);
  XMakeBackdropWindow(resource_info,windows,display,visual_info,manager_hints,
    root_window,argv,argc,class_hints);
}

static void XFollowGroupLeader(XWindows *windows,XWMHints *manager_hints,
  Display *display,XResourceInfo *resource_info)
{
  if (windows->group_leader.id != (Window) NULL)
    {
      /*
        Follow the leader.
      */
      manager_hints->flags|=WindowGroupHint;
      manager_hints->window_group=windows->group_leader.id;
      (void) XSelectInput(display,windows->group_leader.id,StructureNotifyMask);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (group leader)",windows->group_leader.id);
    }
}

static void XMakeImageWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const Window
    root_window = context->root_window;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  manager_hints->flags=IconWindowHint | InputHint | StateHint;
  manager_hints->icon_window=windows->icon.id;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=resource_info->iconic ? IconicState :
    NormalState;
  XFollowGroupLeader(windows,manager_hints,display,resource_info);
  XMakeWindow(display,
    (Window) (resource_info->backdrop ? windows->backdrop.id : root_window),
    argv,argc,class_hints,manager_hints,&windows->image);
  (void) XChangeProperty(display,windows->image.id,windows->im_protocols,
    XA_STRING,8,PropModeReplace,(unsigned char *) NULL,0);
  if (windows->group_leader.id != (Window) NULL)
    (void) XSetTransientForHint(display,windows->image.id,
      windows->group_leader.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (image)",
      windows->image.id);
}
static inline MagickBooleanType IsDynamicVisualClass(const int v)
{
  switch (v)
  {
    case GrayScale:
    case PseudoColor:
    case DirectColor:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

#endif
