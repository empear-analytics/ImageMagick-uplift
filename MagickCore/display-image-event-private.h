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

  MagickCore private methods of display-image.c: the event dispatchers, window
  builders and set-up helpers of XDisplayImage, from XGetDisplayTimer to
  XLookupKeyRelease; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_EVENT_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_EVENT_PRIVATE_H

static time_t XGetDisplayTimer(const Image *display_image)
{
  size_t
    delay;

  delay=display_image->delay/(size_t)
    MagickMax(display_image->ticks_per_second,1L);
  return(GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1);
}

static void XHandleButtonPress(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  time_t
    *const timer = targets->timer;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Press: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if (((*event).xbutton.button == Button3) &&
      ((*event).xbutton.state & Mod1Mask))
    {
      /*
        Convert Alt-Button3 to Button2.
      */
      (*event).xbutton.button=Button2;
      (*event).xbutton.state&=(unsigned int) (~Mod1Mask);
    }
  if ((*event).xbutton.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,(*event).xbutton.window,RevertToParent,
        (*event).xbutton.time);
      return;
    }
  if ((*event).xbutton.window == windows->image.id)
    {
      XPressImageWindow(context,targets);
      return;
    }
  if ((*event).xbutton.window == windows->magnify.id)
    {
      XPressMagnifyWindow(context);
      return;
    }
  if ((*event).xbutton.window == windows->pan.id)
    {
      XPressPanWindow(context,event);
      return;
    }
  (*timer)=XGetDisplayTimer((*display_image));
}

static void XHandleClientMessage(const DisplayEventContext *context,
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

  XVisualInfo
    *const visual_info = context->visual_info;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  XGCValues
    *const context_values = targets->context_values;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Client Message: 0x%lx 0x%lx %d 0x%lx",(*event).xclient.window,
      (*event).xclient.message_type,(*event).xclient.format,(unsigned long)
      (*event).xclient.data.l[0]);
  if ((*event).xclient.message_type == windows->im_protocols)
    {
      XHandleProtocolMessage(context,(*event),context_values);
      return;
    }
  if ((*event).xclient.message_type == windows->dnd_protocols)
    {
      (*nexus)=XDropImage(context,(*event),(*nexus));
      return;
    }
  /*
    If client window delete message, exit.
  */
  if ((*event).xclient.message_type != windows->wm_protocols)
    return;
  if (*(*event).xclient.data.l != (long) windows->wm_delete_window)
    return;
  (void) XWithdrawWindow(display,(*event).xclient.window,
    visual_info->screen);
  if ((*event).xclient.window == windows->image.id)
    {
      *state|=ExitState;
      return;
    }
  if ((*event).xclient.window == windows->pan.id)
    {
      /*
        Restore original image size when pan window is deleted.
      */
      windows->image.window_changes.width=windows->image.ximage->width;
      windows->image.window_changes.height=windows->image.ximage->height;
      (void) XConfigureImage(display,resource_info,windows,
        (*display_image),exception);
    }
}

static void XHandleConfigureNotify(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  XPoint
    *const vid_info = targets->vid_info;

  MagickStatusType
    *const status = targets->status;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Configure Notify: 0x%lx %dx%d+%d+%d %d",(*event).xconfigure.window,
      (*event).xconfigure.width,(*event).xconfigure.height,(*event).xconfigure.x,
      (*event).xconfigure.y,(*event).xconfigure.send_event);
  if ((*event).xconfigure.window == windows->image.id)
    {
      XConfigureImageWindow(context,(*event),(*display_image),(*vid_info));
      return;
    }
  if ((*event).xconfigure.window == windows->magnify.id)
    {
      (*status)=XConfigureMagnifyWindow(context,(*event),(*display_image),
        (*status));
      return;
    }
  if (windows->magnify.mapped != MagickFalse &&
      ((*event).xconfigure.window == windows->pan.id))
    {
      XConfigurePanWindow(windows,(*event));
      return;
    }
  if ((*event).xconfigure.window == windows->icon.id)
    {
      /*
        Icon window has a new configuration.
      */
      windows->icon.width=(unsigned int) (*event).xconfigure.width;
      windows->icon.height=(unsigned int) (*event).xconfigure.height;
      return;
    }
}

static void XHandleExpose(const DisplayEventContext *context,
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
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  time_t
    *const timer = targets->timer;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Expose: 0x%lx %dx%d+%d+%d",(*event).xexpose.window,
      (*event).xexpose.width,(*event).xexpose.height,(*event).xexpose.x,
      (*event).xexpose.y);
  /*
    Refresh windows that are now exposed.
  */
  if (((*event).xexpose.window == windows->image.id) &&
      windows->image.mapped != MagickFalse)
    {
      XRefreshWindow(display,&windows->image,event);
      (*timer)=XGetDisplayTimer((*display_image));
      return;
    }
  if (((*event).xexpose.window == windows->magnify.id) &&
      windows->magnify.mapped != MagickFalse)
    {
      XMakeMagnifyImage(display,windows,exception);
      return;
    }
  if ((*event).xexpose.window == windows->pan.id)
    {
      XDrawPanRectangle(display,windows);
      return;
    }
  if ((*event).xexpose.window == windows->icon.id)
    {
      XRefreshWindow(display,&windows->icon,event);
      return;
    }
}

static void XHandleKeyPress(const DisplayEventContext *context,
  const KeySym key_symbol,const DisplayEventTargets *targets)
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
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  time_t
    *const timer = targets->timer;

  const MagnifyWindowInput
    input = { display, windows, exception };

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key press: %d 0x%lx (%s)",(*event).xkey.state,(unsigned long)
      key_symbol,command);
  if ((*event).xkey.window == windows->image.id)
    {
      XImageWindowKeyPress(context,(*event),key_symbol,
        targets);
    }
  if ((*event).xkey.window == windows->magnify.id)
    XMagnifyWindowCommand(&input,(*event).xkey.state,key_symbol);
  if ((*event).xkey.window == windows->pan.id)
    {
      XPanWindowKeyPress(context,key_symbol);
    }
  (*timer)=XGetDisplayTimer((*display_image));
}

static void XRecordMappedWindow(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XVisualInfo
    *const icon_visual = context->icon_visual;

  XStandardColormap
    *const icon_map = context->icon_map;

  XPixelInfo
    *const icon_pixel = context->icon_pixel;

  XResourceInfo
    *const icon_resources = context->icon_resources;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  if ((*event).xmap.window == windows->info.id)
    {
      windows->info.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->icon.id)
    {
      MagickBooleanType
        taint;

      /*
        Create an icon image.
      */
      taint=(*display_image)->taint;
      XMakeStandardColormap(display,icon_visual,icon_resources,
        (*display_image),icon_map,icon_pixel,exception);
      (void) XMakeImage(display,icon_resources,&windows->icon,
        (*display_image),windows->icon.width,windows->icon.height,
        exception);
      (*display_image)->taint=taint;
      (void) XSetWindowBackgroundPixmap(display,windows->icon.id,
        windows->icon.pixmap);
      (void) XClearWindow(display,windows->icon.id);
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
      windows->icon.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->command.id)
    {
      windows->command.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->popup.id)
    {
      windows->popup.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->widget.id)
    {
      windows->widget.mapped=MagickTrue;
      return;
    }
}

static void XHandleMapNotify(const DisplayEventContext *context,
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
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Map Notify: 0x%lx",
      (*event).xmap.window);
  if ((*event).xmap.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,(*event).xmap.window,RevertToParent,
        CurrentTime);
      windows->backdrop.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->image.id)
    {
      (*nexus)=XMapImageWindow(context,(*display_image),(*nexus));
      return;
    }
  if ((*event).xmap.window == windows->magnify.id)
    {
      XMakeMagnifyImage(display,windows,exception);
      windows->magnify.mapped=MagickTrue;
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
      return;
    }
  if ((*event).xmap.window == windows->pan.id)
    {
      XMakePanImage(display,resource_info,windows,(*display_image),
        exception);
      windows->pan.mapped=MagickTrue;
      return;
    }
  XRecordMappedWindow(context,targets);
}

static void XRecordUnmappedWindow(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  if ((*event).xunmap.window == windows->info.id)
    {
      windows->info.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->icon.id)
    {
      XUnmapIconWindow(context,(*display_image));
      return;
    }
  if ((*event).xunmap.window == windows->command.id)
    {
      windows->command.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->popup.id)
    {
      XFocusImageWindow(display,windows);
      windows->popup.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->widget.id)
    {
      XFocusImageWindow(display,windows);
      windows->widget.mapped=MagickFalse;
      return;
    }
}

static void XHandleUnmapNotify(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XEvent
    *const event = targets->event;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Unmap Notify: 0x%lx",(*event).xunmap.window);
  if ((*event).xunmap.window == windows->backdrop.id)
    {
      windows->backdrop.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->image.id)
    {
      windows->image.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->magnify.id)
    {
      windows->magnify.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->pan.id)
    {
      windows->pan.mapped=MagickFalse;
      return;
    }
  XRecordUnmappedWindow(context,targets);
}

static void XHandlePropertyNotify(const DisplayEventContext *context,
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

  Image
    **const nexus = targets->nexus;

  XEvent
    *const event = targets->event;

  Atom
    type;

  int
    format,
    status;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Property Notify: 0x%lx 0x%lx %d",(*event).xproperty.window,
      (*event).xproperty.atom,(*event).xproperty.state);
  if ((*event).xproperty.atom != windows->im_remote_command)
    return;
  /*
    Display image named by the remote command protocol.
  */
  status=XGetWindowProperty(display,(*event).xproperty.window,
    (*event).xproperty.atom,0L,(long) MagickPathExtent,MagickFalse,(Atom)
    AnyPropertyType,&type,&format,&length,&after,&data);
  if ((status != Success) || (length == 0))
    return;
  if (LocaleCompare((char *) data,"-quit") == 0)
    {
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_exit,CurrentTime);
      (void) XFree((void *) data);
      return;
    }
  (void) CopyMagickString(resource_info->image_info->filename,
    (char *) data,MagickPathExtent);
  (void) XFree((void *) data);
  (*nexus)=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if ((*nexus) != (Image *) NULL)
    *state|=NextImageState | ExitState;
}

static void XMakeInfoWindow(const DisplayEventContext *context)
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

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,resource_info,
    &windows->info);
  (void) CloneString(&windows->info.name,"Info");
  (void) CloneString(&windows->info.icon_name,"Info");
  windows->info.border_width=1;
  windows->info.x=2;
  windows->info.y=2;
  windows->info.flags|=PPosition;
  windows->info.attributes.win_gravity=UnmapGravity;
  windows->info.attributes.event_mask=ButtonPressMask | ExposureMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,windows->image.id,argv,argc,class_hints,manager_hints,
    &windows->info);
  windows->info.highlight_stipple=XCreateBitmapFromData(display,
    windows->info.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->info.shadow_stipple=XCreateBitmapFromData(display,
    windows->info.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->info.id,windows->image.id);
  if (windows->image.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (info)",
      windows->info.id);
}

static void XMakeCommandWindow(const DisplayEventContext *context)
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

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->command);
  windows->command.data=MagickMenus;
  (void) XCommandWidget(display,windows,CommandMenu,(XEvent *) NULL);
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.command",
    resource_info->client_name);
  windows->command.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) CloneString(&windows->command.name,MagickTitle);
  windows->command.border_width=0;
  windows->command.flags|=PPosition;
  windows->command.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | LeaveWindowMask |
    OwnerGrabButtonMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->command);
  windows->command.highlight_stipple=XCreateBitmapFromData(display,
    windows->command.id,(char *) HighlightBitmap,HighlightWidth,
    HighlightHeight);
  windows->command.shadow_stipple=XCreateBitmapFromData(display,
    windows->command.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->command.id,windows->image.id);
  if (windows->command.mapped != MagickFalse)
    (void) XMapRaised(display,windows->command.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (command)",windows->command.id);
}

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
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->widget);
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
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->popup);
  windows->popup.highlight_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->popup.shadow_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->popup.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (pop up)",windows->popup.id);
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
  windows->magnify.cursor=XMakeCursor(display,windows->image.id,
    map_info->colormap,resource_info->background_color,
    resource_info->foreground_color);
  if (windows->magnify.cursor == (Cursor) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateCursor",
      display_image->filename);
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
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->magnify);
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
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->pan);
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
        (*nexus)=XMagickCommand(display,resource_info,windows,display_command,
          display_image,exception);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static void XUpdateWindowStasis(XWindows *windows,const time_t timestamp)
{
  if ((windows->image.stasis == MagickFalse) ||
      (windows->magnify.stasis == MagickFalse))
    {
      if ((GetMagickTime()-timestamp) > 0)
        {
          windows->image.stasis=MagickTrue;
          windows->magnify.stasis=MagickTrue;
        }
    }
}

static inline MagickBooleanType IsSelectiveColormapCrossing(Display *display,
  const XStandardColormap *map_info,const XVisualInfo *visual_info,
  const int mode)
{
  return((MagickBooleanType) ((map_info->colormap !=
    XDefaultColormap(display,visual_info->screen)) && (mode != NotifyUngrab)));
}

static void XDispatchWindowStateEvent(const DisplayEventContext *event_context,
  const DisplayEventTargets *event_targets)
{
  XResourceInfo
    *const resource_info = event_context->resource_info;

  XEvent
    *const event = event_targets->event;

  switch ((*event).type)
  {
    case PropertyNotify:
    {
      XHandlePropertyNotify(event_context,event_targets);
      break;
    }
    case ReparentNotify:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Reparent Notify: 0x%lx=>0x%lx",(*event).xreparent.parent,
          (*event).xreparent.window);
      break;
    }
    case UnmapNotify:
    {
      XHandleUnmapNotify(event_context,event_targets);
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

static void XDispatchMapEvent(const DisplayEventContext *event_context,
  const DisplayEventTargets *event_targets)
{
  Display
    *const display = event_context->display;

  XStandardColormap
    *const map_info = event_context->map_info;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XEvent
    *const event = event_targets->event;

  switch ((*event).type)
  {
    case LeaveNotify:
    {
      /*
        Selectively uninstall colormap.
      */
      if (IsSelectiveColormapCrossing(display,map_info,visual_info,
            (*event).xcrossing.mode) != MagickFalse)
        XUninstallColormap(display,map_info->colormap);
      break;
    }
    case MapNotify:
    {
      XHandleMapNotify(event_context,event_targets);
      break;
    }
    case MappingNotify:
    {
      (void) XRefreshKeyboardMapping(&(*event).xmapping);
      break;
    }
    case NoExpose:
      break;
    default:
    {
      XDispatchWindowStateEvent(event_context,event_targets);
      break;
    }
  }
}

static void XDispatchExposeEvent(const DisplayEventContext *event_context,
  const DisplayEventTargets *event_targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  XStandardColormap
    *const map_info = event_context->map_info;

  size_t
    *const state = event_context->state;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XEvent
    *const event = event_targets->event;

  switch ((*event).type)
  {
    case DestroyNotify:
    {
      /*
        Group leader has exited.
      */
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Destroy Notify: 0x%lx",(*event).xdestroywindow.window);
      if ((*event).xdestroywindow.window == windows->group_leader.id)
        {
          *state|=ExitState;
          break;
        }
      break;
    }
    case EnterNotify:
    {
      /*
        Selectively install colormap.
      */
      if (IsSelectiveColormapCrossing(display,map_info,visual_info,
            (*event).xcrossing.mode) != MagickFalse)
        XInstallColormap(display,map_info->colormap);
      break;
    }
    case Expose:
    {
      XHandleExpose(event_context,event_targets);
      break;
    }
    default:
    {
      XDispatchMapEvent(event_context,event_targets);
      break;
    }
  }
}

static void XDispatchButtonEvent(const DisplayEventContext *event_context,
  const DisplayEventTargets *event_targets)
{
  XResourceInfo
    *const resource_info = event_context->resource_info;

  XEvent
    *const event = event_targets->event;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XHandleButtonPress(event_context,event_targets);
      break;
    }
    case ButtonRelease:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Button Release: 0x%lx %u +%d+%d",(*event).xbutton.window,
          (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
      break;
    }
    case ClientMessage:
    {
      XHandleClientMessage(event_context,event_targets);
      break;
    }
    case ConfigureNotify:
    {
      XHandleConfigureNotify(event_context,event_targets);
      break;
    }
    default:
    {
      XDispatchExposeEvent(event_context,event_targets);
      break;
    }
  }
}

static void XWithdrawDynamicWindows(Display *display,XWindows *windows)
{
  if (IsDynamicVisualClass(windows->visual_info->klass) != MagickFalse)
    {
      /*
        Withdraw pan and Magnify window.
      */
      if (windows->info.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      if (windows->magnify.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->magnify.id,
          windows->magnify.screen);
      if (windows->command.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->command.id,
          windows->command.screen);
    }
}

static void XFreeDisplayResources(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XPixelInfo
    *const icon_pixel = context->icon_pixel,
    *const pixel = context->pixel;

  XStandardColormap
    *const icon_map = context->icon_map,
    *const map_info = context->map_info;

  XVisualInfo
    *const icon_visual = context->icon_visual,
    *const visual_info = context->visual_info;

  size_t
    *const state = context->state;

  if (*state & ExitState)
    {
      /*
        Free Standard Colormap.
      */
      (void) XFreeStandardColormap(display,icon_visual,icon_map,icon_pixel);
      if (resource_info->map_type == (char *) NULL)
        (void) XFreeStandardColormap(display,visual_info,map_info,pixel);
      /*
        Free X resources.
      */
      if (resource_info->copy_image != (Image *) NULL)
        {
          resource_info->copy_image=DestroyImage(resource_info->copy_image);
          resource_info->copy_image=NewImageList();
        }
      DestroyXResources();
    }
}

static void XLogKeyRelease(const XResourceInfo *resource_info,
  const KeySym key_symbol,const char *command)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key release: 0x%lx (%c)",(unsigned long) key_symbol,*command);
}

static WarningHandler XRestoreWorkingDirectory(XResourceInfo *resource_info,
  char *working_directory,ExceptionInfo *exception)
{
  WarningHandler
    warning_handler;

  int
    status;

  if (*working_directory == '\0')
    (void) CopyMagickString(working_directory,".",MagickPathExtent);
  status=chdir(working_directory);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",working_directory);
  warning_handler=resource_info->display_warnings ?
    SetErrorHandler(XWarning) : SetErrorHandler((ErrorHandler) NULL);
  warning_handler=resource_info->display_warnings ?
    SetWarningHandler(XWarning) : SetWarningHandler((WarningHandler) NULL);
  return(warning_handler);
}

static XWindows *XAllocateDisplayWindows(Display *display,
  XResourceInfo *resource_info,const Image *display_image,Image **image)
{
  XWindows
    *windows;

  resource_info->colors=display_image->colors;
  windows=XSetWindows(XInitializeWindows(display,resource_info));
  if (windows == (XWindows *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateWindow",
      (*image)->filename);
  return(windows);
}

static void XInitializeWindowIds(XWindows *windows,
  XWindowInfo **magick_windows,unsigned int *number_windows)
{
  int
    i;

  (*number_windows)=0;
  magick_windows[(*number_windows)++]=(&windows->icon);
  magick_windows[(*number_windows)++]=(&windows->backdrop);
  magick_windows[(*number_windows)++]=(&windows->image);
  magick_windows[(*number_windows)++]=(&windows->info);
  magick_windows[(*number_windows)++]=(&windows->command);
  magick_windows[(*number_windows)++]=(&windows->widget);
  magick_windows[(*number_windows)++]=(&windows->popup);
  magick_windows[(*number_windows)++]=(&windows->magnify);
  magick_windows[(*number_windows)++]=(&windows->pan);
  for (i=0; i < (int) (*number_windows); i++)
    magick_windows[i]->id=(Window) NULL;
}

typedef struct _DisplayExitInputs
{
  size_t
    *state;

  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;
} DisplayExitInputs;

static void XConfirmDisplayExit(const DisplayExitInputs *inputs,
  Image **display_image)
{
  size_t
    *const state = inputs->state;

  Display
    *const display = inputs->display;

  XResourceInfo
    *const resource_info = inputs->resource_info;

  XWindows
    *const windows = inputs->windows;

  ExceptionInfo
    *const exception = inputs->exception;

  if ((*state & ExitState) == 0)
    (void) XMagickCommand(display,resource_info,windows,FreeBuffersCommand,
      &(*display_image),exception);
  else
    if (resource_info->confirm_edit != MagickFalse)
      {
        /*
          Query user if image has changed.
        */
        if ((resource_info->immutable == MagickFalse) &&
            (*display_image)->taint != MagickFalse)
          {
            int
              status;

            status=XConfirmWidget(display,windows,"Your image changed.",
              "Do you want to save it");
            if (status == 0)
              *state&=(unsigned int) (~ExitState);
            else
              if (status > 0)
                (void) XMagickCommand(display,resource_info,windows,SaveCommand,
                  &(*display_image),exception);
          }
      }
}

static void XWithdrawBackdropWindow(XResourceInfo *resource_info,
  XWindows *windows,Display *display)
{
  if ((resource_info->backdrop == MagickFalse) && (windows->backdrop.mapped))
    {
      (void) XWithdrawWindow(display,windows->backdrop.id,
        windows->backdrop.screen);
      (void) XDestroyWindow(display,windows->backdrop.id);
      windows->backdrop.id=(Window) NULL;
      (void) XWithdrawWindow(display,windows->image.id,
        windows->image.screen);
      (void) XDestroyWindow(display,windows->image.id);
      windows->image.id=(Window) NULL;
    }
}

static void XCloseDisplayWindows(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = context->state;

  XWithdrawDynamicWindows(display,windows);
  if (windows->pan.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->pan.id,windows->pan.screen);
  XWithdrawBackdropWindow(resource_info,windows,display);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (((*state & FormerImageState) != 0) || ((*state & NextImageState) != 0))
    *state&=(unsigned int) (~ExitState);
  XFreeDisplayResources(context);
  (void) XSync(display,MagickFalse);
}

static void XChangeToHomeDirectory(XResourceInfo *resource_info,
  ExceptionInfo *exception)
{
  int
    status;

  if (*resource_info->home_directory == '\0')
    (void) CopyMagickString(resource_info->home_directory,".",MagickPathExtent);
  status=chdir(resource_info->home_directory);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",resource_info->home_directory);
}

static MagickStatusType XMakeDisplayXImages(const DisplayEventContext *context,
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

  MagickStatusType
    status;

  unsigned int
    height,
    width;

  windows->image.x=0;
  windows->image.y=0;
  windows->magnify.shape=MagickFalse;
  width=(unsigned int) display_image->columns;
  height=(unsigned int) display_image->rows;
  if ((display_image->columns != width) || (display_image->rows != height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  status=XMakeImage(display,resource_info,&windows->image,display_image,
    width,height,exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  status=XMakeImage(display,resource_info,&windows->magnify,(Image *) NULL,
    windows->magnify.width,windows->magnify.height,exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  if (windows->magnify.mapped != MagickFalse)
    (void) XMapRaised(display,windows->magnify.id);
  if (windows->pan.mapped != MagickFalse)
    (void) XMapRaised(display,windows->pan.id);
  windows->image.window_changes.width=(int) display_image->columns;
  windows->image.window_changes.height=(int) display_image->rows;
  (void) XConfigureImage(display,resource_info,windows,display_image,exception);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  (void) XSync(display,MagickFalse);
  return(status);
}

static void XInitializePanWindow(const DisplayEventContext *context,
  XWindows *windows,Display *display)
{
  XMakePanWindow(context);
  if (windows->info.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if ((windows->image.mapped == MagickFalse) ||
      (windows->backdrop.id != (Window) NULL))
    (void) XMapWindow(display,windows->image.id);
}

static void XMakeDisplayWindows(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XVisualInfo
    *const visual_info = context->visual_info;

  char
    geometry[MagickPathExtent];

  RectangleInfo
    geometry_info;

  XNameImageWindow(context,display_image);
  if (resource_info->immutable)
    windows->image.immutable=MagickTrue;
  windows->image.use_pixmap=resource_info->use_pixmap;
  windows->image.geometry=resource_info->image_geometry;
  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>!",
    XDisplayWidth(display,visual_info->screen),
    XDisplayHeight(display,visual_info->screen));
  geometry_info.width=display_image->columns;
  geometry_info.height=display_image->rows;
  geometry_info.x=0;
  geometry_info.y=0;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  windows->image.width=(unsigned int) geometry_info.width;
  windows->image.height=(unsigned int) geometry_info.height;
  windows->image.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | OwnerGrabButtonMask |
    PropertyChangeMask | StructureNotifyMask | SubstructureNotifyMask;
  XInitializeBackdropWindow(context);
  XMakeImageWindow(context);
  /*
    Initialize Info widget.
  */
  XMakeInfoWindow(context);
  /*
    Initialize Command widget.
  */
  XMakeCommandWindow(context);
  /*
    Initialize Widget window.
  */
  XMakeWidgetWindow(context);
  /*
    Initialize popup window.
  */
  XMakePopupWindow(context);
  /*
    Initialize Magnify window and cursor.
  */
  XMakeMagnifyWindow(context,display_image);
  (void) XSetTransientForHint(display,windows->magnify.id,windows->image.id);
  /*
    Initialize panning window.
  */
  XInitializePanWindow(context,windows,display);
}

static void XInitializeIconContext(XPixelInfo *icon_pixel,Display *display,
  XGCValues *context_values,XWindows *windows,Image *display_image)
{
  if (icon_pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,icon_pixel->annotate_context);
  (*context_values).background=icon_pixel->background_color.pixel;
  (*context_values).foreground=icon_pixel->foreground_color.pixel;
  icon_pixel->annotate_context=XCreateGC(display,windows->icon.id,
    (size_t) (GCBackground | GCForeground),context_values);
  if (icon_pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  windows->icon.annotate_context=icon_pixel->annotate_context;
}

static void XMakeIconWindow(const DisplayEventContext *context,
  Image *display_image,XGCValues *context_values)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info,
    *const icon_resources = context->icon_resources;

  XWindows
    *const windows = context->windows;

  XVisualInfo
    *const icon_visual = context->icon_visual;

  XStandardColormap
    *const icon_map = context->icon_map;

  XPixelInfo
    *const icon_pixel = context->icon_pixel;

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

  XGetWindowInfo(display,icon_visual,icon_map,icon_pixel,(XFontStruct *) NULL,
    icon_resources,&windows->icon);
  windows->icon.geometry=resource_info->icon_geometry;
  XBestIconSize(display,&windows->icon,display_image);
  windows->icon.attributes.colormap=XDefaultColormap(display,
    icon_visual->screen);
  windows->icon.attributes.event_mask=ExposureMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=IconicState;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->icon);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (icon)",
      windows->icon.id);
  /*
    Initialize graphic context for icon window.
  */
  XInitializeIconContext(icon_pixel,display,context_values,windows,
    display_image);
}

static void XMakeContextWindow(const DisplayEventContext *context)
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

  windows->context.id=(Window) NULL;
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->context);
  (void) CloneString(&class_hints->res_name,resource_info->client_name);
  (void) CloneString(&class_hints->res_class,resource_info->client_name);
  class_hints->res_class[0]=(char) LocaleToUppercase((int)
    class_hints->res_class[0]);
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=WithdrawnState;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->context);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (context)",windows->context.id);
}

static void XCreateDisplayGCs(const DisplayEventContext *context,
  const Image *display_image,const MagickStatusType context_mask,
  XGCValues *context_values)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  if (pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,pixel->annotate_context);
  pixel->annotate_context=XCreateGC(display,windows->context.id,
    context_mask,context_values);
  if (pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  (*context_values).background=pixel->depth_color.pixel;
  if (pixel->widget_context != (GC) NULL)
    (void) XFreeGC(display,pixel->widget_context);
  pixel->widget_context=XCreateGC(display,windows->context.id,context_mask,
    context_values);
  if (pixel->widget_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  (*context_values).background=pixel->foreground_color.pixel;
  (*context_values).foreground=pixel->background_color.pixel;
  (*context_values).plane_mask=(*context_values).background ^
    (*context_values).foreground;
  if (pixel->highlight_context != (GC) NULL)
    (void) XFreeGC(display,pixel->highlight_context);
  pixel->highlight_context=XCreateGC(display,windows->context.id,
    (size_t) (context_mask | GCPlaneMask),context_values);
  if (pixel->highlight_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  (void) XDestroyWindow(display,windows->context.id);
}

static WarningHandler XInstallWarningHandlers(
  const XResourceInfo *resource_info,WarningHandler warning_handler)
{
  if (warning_handler == (WarningHandler) NULL)
    {
      warning_handler=resource_info->display_warnings ?
        SetErrorHandler(XWarning) : SetErrorHandler((ErrorHandler) NULL);
      warning_handler=resource_info->display_warnings ?
        SetWarningHandler(XWarning) : SetWarningHandler((WarningHandler) NULL);
    }
  return(warning_handler);
}

static void XLoadDisplayFont(Display *display,XResourceInfo *resource_info,
  XWindows *windows)
{
  if (windows->font_info != (XFontStruct *) NULL)
    (void) XFreeFont(display,windows->font_info);
  windows->font_info=XBestFont(display,resource_info,MagickFalse);
  if (windows->font_info == (XFontStruct *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToLoadFont",
      resource_info->font);
}

static MagickBooleanType XCheckMappedImageTimer(
  const DisplayEventContext *context,const DisplayEventTargets *targets)
{
  XWindows
    *const windows = context->windows;

  Image
    **const nexus = targets->nexus,
    **const display_image = targets->display_image;

  time_t
    *const timer = targets->timer;

  if (windows->image.mapped != MagickFalse)
    if (XCheckImageTimer(context,(*display_image),nexus,timer) !=
        MagickFalse)
      return(MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType XNextDisplayEvent(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XEvent
    *const event = targets->event;

  time_t
    timestamp;

  if (XCheckMappedImageTimer(context,targets) != MagickFalse)
    return(MagickTrue);
  timestamp=GetMagickTime();
  (void) XNextEvent(display,event);
  XUpdateWindowStasis(windows,timestamp);
  if (XSelectCommandWidgetEvent(context,targets) !=
      MagickFalse)
    return(MagickTrue);
  return(MagickFalse);
}

static void XTraceDisplayImage(Image **image)
{
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
}

static void XReturnToHomeDirectory(char *working_directory,
  XResourceInfo *resource_info,ExceptionInfo *exception)
{
  char
    *directory;

  directory=getcwd(working_directory,MagickPathExtent);
  (void) directory;
  XChangeToHomeDirectory(resource_info,exception);
}

static void XRestoreDisplayState(const WarningHandler warning_handler,
  char *working_directory,XResourceInfo *resource_info,
  ExceptionInfo *exception)
{
  (void) SetErrorHandler(warning_handler);
  (void) SetWarningHandler(warning_handler);
  /*
    Change to home directory.
  */
  XReturnToHomeDirectory(working_directory,resource_info,exception);
}

typedef struct _DisplayContextInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  XWindowInfo
    **magick_windows;

  unsigned int
    number_windows;

  size_t
    *state;

  char
    *command;

  Image
    **image;

  char
    **argv;

  int
    argc;
} DisplayContextInput;

static void XInitializeDisplayColormap(const DisplayContextInput *input,
  DisplayEventContext *event_context)
{
  Window
    root_window;

  XClassHint
    *class_hints;

  XFontStruct
    *font_info;

  XPixelInfo
    *icon_pixel,
    *pixel;

  XResourceInfo
    *icon_resources;

  XStandardColormap
    *icon_map,
    *map_info;

  XVisualInfo
    *icon_visual,
    *visual_info;

  XWMHints
    *manager_hints;

  map_info=input->windows->map_info;
  icon_map=input->windows->icon_map;
  visual_info=input->windows->visual_info;
  icon_visual=input->windows->icon_visual;
  pixel=input->windows->pixel_info;
  icon_pixel=input->windows->icon_pixel;
  font_info=input->windows->font_info;
  icon_resources=input->windows->icon_resources;
  class_hints=input->windows->class_hints;
  manager_hints=input->windows->manager_hints;
  root_window=XRootWindow(input->display,visual_info->screen);
  (*event_context).display=input->display;
  (*event_context).resource_info=input->resource_info;
  (*event_context).windows=input->windows;
  (*event_context).exception=input->exception;
  (*event_context).pixel=pixel;
  (*event_context).map_info=map_info;
  (*event_context).magick_windows=input->magick_windows;
  (*event_context).number_windows=input->number_windows;
  (*event_context).state=input->state;
  (*event_context).root_window=root_window;
  (*event_context).CommandMenu=CommandMenu;
  (*event_context).command=input->command;
  (*event_context).visual_info=visual_info;
  (*event_context).VirtualMenu=VirtualMenu;
  (*event_context).VirtualCommands=VirtualCommands;
  (*event_context).ShortCutsMenu=ShortCutsMenu;
  (*event_context).ShortCutsCommands=ShortCutsCommands;
  (*event_context).image=input->image;
  (*event_context).icon_visual=icon_visual;
  (*event_context).icon_map=icon_map;
  (*event_context).icon_pixel=icon_pixel;
  (*event_context).Menus=Menus;
  (*event_context).Commands=Commands;
  (*event_context).font_info=font_info;
  (*event_context).argv=input->argv;
  (*event_context).argc=input->argc;
  (*event_context).class_hints=class_hints;
  (*event_context).manager_hints=manager_hints;
  (*event_context).icon_resources=icon_resources;
}

static void XFillDisplayEventContext(const DisplayContextInput *input,
  DisplayEventContext *event_context)
{
  /*
    Initialize font info.
  */
  XLoadDisplayFont(input->display,input->resource_info,input->windows);
  /*
    Initialize Standard Colormap.
  */
  XInitializeDisplayColormap(input,event_context);
}

typedef struct _DisplayWindowsInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  Image
    **image;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;
} DisplayWindowsInput;

static void XOpenDisplayWindows(const DisplayWindowsInput *input,
  const DisplayEventTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *const display_image = *targets->display_image;

  Image
    **const image = input->image;

  XPoint
    *const vid_info = targets->vid_info;

  WarningHandler
    *const warning_handler = targets->warning_handler;

  XWindows
    **const windows = targets->windows;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindowInfo
    **const magick_windows = input->magick_windows;

  char
    *const working_directory = input->working_directory;

  unsigned int
    *const number_windows = targets->number_windows;

  (*warning_handler)=(WarningHandler) NULL;
  (*windows)=XSetWindows((XWindows *) ~0);
  if ((*windows) != (XWindows *) NULL)
    {
      (*warning_handler)=XRestoreWorkingDirectory(resource_info,
        working_directory,exception);
    }
  else
    {
      /*
        Allocate windows structure.
      */
      (*windows)=XAllocateDisplayWindows(display,resource_info,display_image,
        image);
      /*
        Initialize window id's.
      */
      XInitializeWindowIds((*windows),magick_windows,number_windows);
      (*vid_info).x=0;
      (*vid_info).y=0;
    }
}

typedef struct _DisplaySetUpInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  size_t
    *state;
} DisplaySetUpInput;

static void XSetUpDisplay(const DisplaySetUpInput *input,
  DisplayEventContext *event_context,const DisplayEventTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *const display_image = *targets->display_image;

  Image
    **const nexus = targets->nexus;

  MagickStatusType
    *const status = targets->status;

  size_t
    *const state = input->state;

  time_t
    *const timer = targets->timer;

  WarningHandler
    *const warning_handler = targets->warning_handler;

  XFontStruct
    *const font_info = event_context->font_info;

  XGCValues
    *const context_values = targets->context_values;

  XPixelInfo
    *const pixel = event_context->pixel;

  XResourceInfo
    *const resource_info = input->resource_info;

  XStandardColormap
    *const map_info = event_context->map_info;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  MagickStatusType
    context_mask;

  time_t
    update_time;

  (*nexus)=NewImageList();
  XLogDisplayImage(resource_info,display_image);
  XMakeStandardColormap(display,visual_info,resource_info,display_image,
    map_info,pixel,exception);
  display_image->taint=MagickFalse;
  /*
    Initialize graphic context.
  */
  XMakeContextWindow(event_context);
  (*context_values).background=pixel->background_color.pixel;
  (*context_values).font=font_info->fid;
  (*context_values).foreground=pixel->foreground_color.pixel;
  (*context_values).graphics_exposures=MagickFalse;
  context_mask=(MagickStatusType)
    (GCBackground | GCFont | GCForeground | GCGraphicsExposures);
  XCreateDisplayGCs(event_context,display_image,context_mask,
    context_values);
  /*
    Initialize icon window.
  */
  XMakeIconWindow(event_context,display_image,context_values);
  /*
    Initialize Image window.
  */
  XMakeDisplayWindows(event_context,display_image);
  /*
    Set our progress monitor and warning handlers.
  */
  (*warning_handler)=XInstallWarningHandlers(resource_info,(*warning_handler));
  /*
    Initialize Image and Magnify X images.
  */
  (*status)=XMakeDisplayXImages(event_context,display_image);
  /*
    Respond to events.
  */
  (*timer)=XGetDisplayTimer(display_image);
  update_time=XGetUpdateTime(resource_info,display_image);
  *state&=(unsigned int) (~FormerImageState);
  *state&=(unsigned int) (~MontageImageState);
  *state&=(unsigned int) (~NextImageState);
  (*event_context).context_mask=context_mask;
  (*event_context).update_time=update_time;
}

typedef struct _DisplayFinishInput
{
  size_t
    *state;

  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  char
    *working_directory;

  Image
    **image;
} DisplayFinishInput;

static Image *XFinishDisplayImage(const DisplayFinishInput *finish_input,
  const DisplayEventContext *event_context,const DisplayEventTargets *targets)
{
  Display
    *const display = finish_input->display;

  ExceptionInfo
    *const exception = finish_input->exception;

  Image
    **const display_image = targets->display_image;

  Image
    **const image = finish_input->image;

  Image
    **const nexus = targets->nexus;

  XResourceInfo
    *const resource_info = finish_input->resource_info;

  XWindows
    *const windows = *targets->windows;

  char
    *const working_directory = finish_input->working_directory;

  size_t
    *const state = finish_input->state;

  const WarningHandler
    warning_handler = *targets->warning_handler;

  {
    const DisplayExitInputs inputs = { state, display, resource_info, windows,
      exception };

    XConfirmDisplayExit(&inputs,display_image);
  }
  XCloseDisplayWindows(event_context);
  /*
    Restore our progress monitor and warning handlers.
  */
  XRestoreDisplayState(warning_handler,working_directory,resource_info,
    exception);
  *image=(*display_image);
  return((*nexus));
}

typedef struct _DisplayImageInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  Image
    **image;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;

  size_t
    *state;

  char
    *command;

  char
    **argv;

  int
    argc;
} DisplayImageInput;

static void XInitializeDisplayImage(const DisplayImageInput *image_input,
  DisplayEventContext *event_context,const DisplayEventTargets *targets)
{
  Display
    *const display = image_input->display;

  ExceptionInfo
    *const exception = image_input->exception;

  Image
    **const image = image_input->image;

  XResourceInfo
    *const resource_info = image_input->resource_info;

  XWindowInfo
    **const magick_windows = image_input->magick_windows;

  XWindows
    **const windows = targets->windows;

  char
    *const command = image_input->command;

  char
    *const working_directory = image_input->working_directory;

  char
    **const argv = image_input->argv;

  const int
    argc = image_input->argc;

  size_t
    *const state = image_input->state;

  unsigned int
    *const number_windows = targets->number_windows;

  {
    const DisplayWindowsInput input = { display, resource_info, exception,
      image, working_directory, magick_windows };

    XOpenDisplayWindows(&input,targets);
  }
  {
    const DisplayContextInput input = { display, resource_info, (*windows),
      exception, magick_windows, (*number_windows), state, command, image, argv,
      argc };

    XFillDisplayEventContext(&input,event_context);
  }
  {
    const DisplaySetUpInput input = { display, resource_info, exception,
      state };

    XSetUpDisplay(&input,event_context,targets);
  }
}

static void XLookupKeyPress(const DisplayEventContext *context,
  const DisplayEventTargets *targets,char *command,const size_t extent)
{
  XEvent
    *const event = targets->event;

  int
    length;

  KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  length=XLookupString((XKeyEvent *) &event->xkey,command,(int)
    extent,&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  XHandleKeyPress(context,key_symbol,targets);
}

static void XLookupKeyRelease(const XResourceInfo *resource_info,
  XEvent *event,char *command,const size_t extent)
{
  KeySym
    key_symbol;

  /*
    Respond to a user key release.
  */
  (void) XLookupString((XKeyEvent *) &event->xkey,command,(int)
    extent,&key_symbol,(XComposeStatus *) NULL);
  XLogKeyRelease(resource_info,key_symbol,command);
}

#endif
