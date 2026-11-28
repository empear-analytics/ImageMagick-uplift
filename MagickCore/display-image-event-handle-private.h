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

  MagickCore private methods of display-image.c: the event handlers of
  XDisplayImage and the info and command windows, from XGetDisplayTimer to
  XMakeCommandWindow; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_EVENT_HANDLE_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_EVENT_HANDLE_PRIVATE_H

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
      {
        const XStandardColormapRequest
          colormap_request = { icon_visual, icon_resources, (*display_image),
            icon_map, icon_pixel };

        XMakeStandardColormap(display,&colormap_request,exception);
      }
      {
        const XMakeImageRequest
          image_request = { &windows->icon, (*display_image),
            windows->icon.width, windows->icon.height };

        (void) XMakeImage(display,icon_resources,&image_request,exception);
      }
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
      {
        const XClientReason
          reason = { windows->im_exit, CurrentTime };

        XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
      }
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

  const XMakeWindowHints hints = { argv, argc, class_hints, manager_hints };

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
  XMakeWindow(display,windows->image.id,&hints,&windows->info);
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

  const XMakeWindowHints hints = { argv, argc, class_hints, manager_hints };

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
  XMakeWindow(display,root_window,&hints,&windows->command);
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

#endif
