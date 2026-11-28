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

  MagickCore private methods of display-image.c: the widget, popup, magnify
  and pan windows of XDisplayImage, from XMakeWidgetWindow to
  XSelectCommandWidgetEvent; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_EVENT_WINDOWS_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_EVENT_WINDOWS_PRIVATE_H

static void XMakeWidgetWindow(const DisplayEventContext *context)
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

  char
    resource_name[MagickPathExtent];

  const XMakeWindowHints hints = { argv, argc, class_hints, manager_hints };

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->widget);
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.widget",
    resource_info->client_name);
  windows->widget.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  windows->widget.border_width=0;
  windows->widget.flags|=PPosition;
  windows->widget.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | OwnerGrabButtonMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,&hints,&windows->widget);
  windows->widget.highlight_stipple=XCreateBitmapFromData(display,
    windows->widget.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->widget.shadow_stipple=XCreateBitmapFromData(display,
    windows->widget.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->widget.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (widget)",windows->widget.id);
}

static void XMakePopupWindow(const DisplayEventContext *context)
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

  const XMakeWindowHints hints = { argv, argc, class_hints, manager_hints };

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->popup);
  windows->popup.border_width=0;
  windows->popup.flags|=PPosition;
  windows->popup.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,&hints,&windows->popup);
  windows->popup.highlight_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->popup.shadow_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->popup.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (pop up)",windows->popup.id);
}

static void XSetMagnifyWindowAttributes(XWindows *windows,
  XResourceInfo *resource_info)
{
  windows->magnify.width=MagnifySize;
  windows->magnify.height=MagnifySize;
  windows->magnify.flags|=PPosition;
  windows->magnify.min_width=MagnifySize;
  windows->magnify.min_height=MagnifySize;
  windows->magnify.width_inc=MagnifySize;
  windows->magnify.height_inc=MagnifySize;
  windows->magnify.data=resource_info->magnify;
  windows->magnify.attributes.cursor=windows->magnify.cursor;
  windows->magnify.attributes.event_mask=ButtonPressMask | ButtonReleaseMask |
    ExposureMask | KeyPressMask | KeyReleaseMask | OwnerGrabButtonMask |
    StructureNotifyMask;
}

static void XMakeMagnifyWindow(const DisplayEventContext *context,
  const Image *display_image)
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

  char
    resource_name[MagickPathExtent];

  const XMakeWindowHints hints = { argv, argc, class_hints, manager_hints };

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->magnify);
  if (resource_info->use_shared_memory == MagickFalse)
    windows->magnify.shared_memory=MagickFalse;
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.magnify",
    resource_info->client_name);
  windows->magnify.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) FormatLocaleString(windows->magnify.name,MagickPathExtent,
    "Magnify %uX",resource_info->magnify);
  if (windows->magnify.cursor != (Cursor) NULL)
    (void) XFreeCursor(display,windows->magnify.cursor);
  {
    const XCursorColors
      cursor_colors = { resource_info->background_color,
        resource_info->foreground_color };

    windows->magnify.cursor=XMakeCursor(display,windows->image.id,
      map_info->colormap,&cursor_colors);
  }
  if (windows->magnify.cursor == (Cursor) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateCursor",
      display_image->filename);
  XSetMagnifyWindowAttributes(windows,resource_info);
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,&hints,&windows->magnify);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (magnify)",windows->magnify.id);
}

static void XMakePanWindow(const DisplayEventContext *context)
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

  char
    resource_name[MagickPathExtent];

  const XMakeWindowHints hints = { argv, argc, class_hints, manager_hints };

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->pan);
  (void) CloneString(&windows->pan.name,"Pan Icon");
  windows->pan.width=windows->icon.width;
  windows->pan.height=windows->icon.height;
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.pan",
    resource_info->client_name);
  windows->pan.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) XParseGeometry(windows->pan.geometry,&windows->pan.x,&windows->pan.y,
    &windows->pan.width,&windows->pan.height);
  windows->pan.flags|=PPosition;
  windows->pan.immutable=MagickTrue;
  windows->pan.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | ExposureMask | KeyPressMask | KeyReleaseMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,&hints,&windows->pan);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (pan)",
      windows->pan.id);
  (void) XSetTransientForHint(display,windows->pan.id,windows->image.id);
}

static MagickBooleanType XSelectCommandWidgetEvent(
  const DisplayEventContext *context,const DisplayEventTargets *targets)
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

  Image
    **const nexus = targets->nexus,
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  DisplayCommand
    display_command;

  int
    id;

  const XImageCacheContext
    display_context = { display, resource_info, windows, display_image,
      exception };

  if ((*event).xany.window == windows->command.id)
    {
      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,CommandMenu,event);
      if (id < 0)
        return(MagickTrue);
      (void) CopyMagickString(command,CommandMenu[id],MagickPathExtent);
      display_command=CommandMenus[id];
      if (XSelectCommandMenuEntry(context,id,&display_command) !=
          MagickFalse)
        return(MagickTrue);
      if (display_command != NullCommand)
        (*nexus)=XMagickCommand(&display_context,display_command);
      return(MagickTrue);
    }
  return(MagickFalse);
}

#endif
