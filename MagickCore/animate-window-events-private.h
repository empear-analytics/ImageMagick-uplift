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

  MagickCore private methods of animate-window.c: the event handlers and the
  window creation of XAnimateImages, from XHandleDestroyNotify to
  XChangeToHomeDirectory; included by that file only.
*/
#ifndef MAGICKCORE_ANIMATE_WINDOW_EVENTS_PRIVATE_H
#define MAGICKCORE_ANIMATE_WINDOW_EVENTS_PRIVATE_H

static MagickStatusType XHandleDestroyNotify(XResourceInfo *resource_info,
  XWindows *windows,const XEvent event,MagickStatusType state)
{
  /*
    Group leader has exited.
  */
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Destroy Notify: 0x%lx",event.xdestroywindow.window);
  if (event.xdestroywindow.window == windows->group_leader.id)
    {
      state|=ExitState;
      return(state);
    }
  return(state);
}

static void XRepaintExposedWindows(Display *display,XWindows *windows,
  const ssize_t scene,const XEvent event)
{
  /*
    Repaint windows that are now exposed.
  */
  if (event.xexpose.window == windows->image.id)
    {
      windows->image.pixmap=windows->image.pixmaps[scene];
      windows->image.matte_pixmap=windows->image.matte_pixmaps[scene];
      XRefreshWindow(display,&windows->image,&event);
      return;
    }
  if ((event.xexpose.window == windows->icon.id) &&
      (event.xexpose.count == 0))
    {
      XRefreshWindow(display,&windows->icon,&event);
      return;
    }
}

static void XUnmapPanelWindows(Display *display,XWindows *windows,
  const XEvent event)
{
  if (event.xunmap.window == windows->command.id)
    {
      windows->command.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->popup.id)
    {
      XFocusBackdropImage(display,windows);
      windows->popup.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->widget.id)
    {
      XFocusBackdropImage(display,windows);
      windows->widget.mapped=MagickFalse;
      return;
    }
}

static void XMapPanelWindows(XWindows *windows,const XEvent event)
{
  if (event.xmap.window == windows->command.id)
    {
      windows->command.mapped=MagickTrue;
      return;
    }
  if (event.xmap.window == windows->popup.id)
    {
      windows->popup.mapped=MagickTrue;
      return;
    }
  if (event.xmap.window == windows->widget.id)
    {
      windows->widget.mapped=MagickTrue;
      return;
    }
}

typedef struct _AnimateEventContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;

  Image
    **image_list;

  Image
    *display_image;

  ExceptionInfo
    *exception;

  XResourceInfo
    *icon_resources;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;

  const char
    *const *CommandMenu;

  XPixelInfo
    *pixel;

  MagickStatusType
    context_mask;

  Window
    root_window;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;

  size_t
    number_scenes;
} AnimateEventContext;

static void XHandleUnmapNotify(const AnimateEventContext *event_context,
  const XEvent event)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  XStandardColormap
    *const map_info = event_context->map_info;

  Image
    *const display_image = event_context->display_image;

  ExceptionInfo
    *const exception = event_context->exception;

  XVisualInfo
    *const icon_visual = event_context->icon_visual;

  XStandardColormap
    *const icon_map = event_context->icon_map;

  XPixelInfo
    *const icon_pixel = event_context->icon_pixel;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Unmap Notify: 0x%lx",event.xunmap.window);
  if (event.xunmap.window == windows->backdrop.id)
    {
      windows->backdrop.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->image.id)
    {
      windows->image.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->info.id)
    {
      windows->info.mapped=MagickFalse;
      return;
    }
  if (event.xunmap.window == windows->icon.id)
    {
      AnimateIconColormaps
        icon_colormaps;

      icon_colormaps.display=display;
      icon_colormaps.resource_info=resource_info;
      icon_colormaps.windows=windows;
      icon_colormaps.display_image=display_image;
      icon_colormaps.exception=exception;
      icon_colormaps.map_info=map_info;
      icon_colormaps.icon_map=icon_map;
      icon_colormaps.icon_visual=icon_visual;
      icon_colormaps.icon_pixel=icon_pixel;
      XReleaseIconColormap(&icon_colormaps);
      return;
    }
  XUnmapPanelWindows(display,windows,event);
}

static void XHandleMapNotify(const AnimateEventContext *event_context,
  const XEvent event,const AnimateCommandTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  Image
    **const image = targets->image;

  MagickStatusType
    *const state = targets->state;

  Image
    **const nexus = targets->nexus;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Map Notify: 0x%lx",
      event.xmap.window);
  if (event.xmap.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,event.xmap.window,RevertToParent,
        CurrentTime);
      windows->backdrop.mapped=MagickTrue;
      return;
    }
  if (event.xmap.window == windows->image.id)
    {
      AnimateImageWindow
        image_window;

      image_window.display=display;
      image_window.resource_info=resource_info;
      image_window.windows=windows;
      image_window.map_info=event_context->map_info;
      image_window.image_list=event_context->image_list;
      image_window.display_image=event_context->display_image;
      image_window.exception=event_context->exception;
      XMapAnimateImageWindow(&image_window,nexus,image,state);
      return;
    }
  if (event.xmap.window == windows->info.id)
    {
      windows->info.mapped=MagickTrue;
      return;
    }
  if (event.xmap.window == windows->icon.id)
    {
      /*
        Create an icon image.
      */
      XMakeStandardColormap(display,event_context->icon_visual,
        event_context->icon_resources,event_context->display_image,
        event_context->icon_map,event_context->icon_pixel,
        event_context->exception);
      {
        const XMakeImageRequest
          image_request = { &windows->icon, event_context->display_image,
            windows->icon.width, windows->icon.height };

        (void) XMakeImage(display,event_context->icon_resources,&image_request,
          event_context->exception);
      }
      (void) XSetWindowBackgroundPixmap(display,windows->icon.id,
        windows->icon.pixmap);
      (void) XClearWindow(display,windows->icon.id);
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
      windows->icon.mapped=MagickTrue;
      return;
    }
  XMapPanelWindows(windows,event);
}

static void XHandlePropertyNotify(const AnimateEventContext *event_context,
  const XEvent event,const AnimateCommandTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  ExceptionInfo
    *const exception = event_context->exception;

  MagickStatusType
    *const state = targets->state;

  Image
    **const nexus = targets->nexus;

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
      "Property Notify: 0x%lx 0x%lx %d",(unsigned long)
      event.xproperty.window,(unsigned long) event.xproperty.atom,
      event.xproperty.state);
  if (event.xproperty.atom != windows->im_remote_command)
    return;
  /*
    Display image named by the remote command protocol.
  */
  status=XGetWindowProperty(display,event.xproperty.window,
    event.xproperty.atom,0L,(long) MagickPathExtent,MagickFalse,(Atom)
    AnyPropertyType,&type,&format,&length,&after,&data);
  if ((status != Success) || (length == 0))
    return;
  (void) CopyMagickString(resource_info->image_info->filename,
    (char *) data,MagickPathExtent);
  (*nexus)=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if ((*nexus) != (Image *) NULL)
    (*state)|=ExitState;
  (void) XFree((void *) data);
}

static MagickStatusType XHandleDeleteMessage(
  const AnimateEventContext *event_context,const XEvent event,
  MagickStatusType state)
{
  Display
    *const display = event_context->display;

  XWindows
    *const windows = event_context->windows;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  /*
    If client window delete message, exit.
  */
  if (event.xclient.message_type != windows->wm_protocols)
    return(state);
  if (*event.xclient.data.l == (long) windows->wm_take_focus)
    {
      (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
        (Time) event.xclient.data.l[1]);
      return(state);
    }
  if (*event.xclient.data.l != (long) windows->wm_delete_window)
    return(state);
  (void) XWithdrawWindow(display,event.xclient.window,
    visual_info->screen);
  if (event.xclient.window == windows->image.id)
    {
      state|=ExitState;
      return(state);
    }
  return(state);
}

static void XHelpKeyCommand(const KeySym key_symbol,
  AnimateCommand *animate_command)
{
  switch (key_symbol)
  {
    case XK_F1:
    {
      (*animate_command)=HelpCommand;
      break;
    }
    case XK_Find:
    {
      (*animate_command)=BrowseDocumentationCommand;
      break;
    }
    case XK_question:
    {
      (*animate_command)=InfoCommand;
      break;
    }
    case XK_q:
    case XK_Escape:
    {
      (*animate_command)=QuitCommand;
      break;
    }
    default:
      break;
  }
}

static void XPlaybackKeyCommand(const KeySym key_symbol,
  AnimateCommand *animate_command)
{
  switch (key_symbol)
  {
    case XK_BackSpace:
    {
      (*animate_command)=StepBackwardCommand;
      break;
    }
    case XK_space:
    {
      (*animate_command)=StepForwardCommand;
      break;
    }
    case XK_less:
    {
      (*animate_command)=FasterCommand;
      break;
    }
    case XK_greater:
    {
      (*animate_command)=SlowerCommand;
      break;
    }
    default:
    {
      XHelpKeyCommand(key_symbol,&(*animate_command));
      break;
    }
  }
}

static void XHandleKeyPress(const AnimateEventContext *event_context,
  XEvent event,const AnimateCommandTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  ExceptionInfo
    *const exception = event_context->exception;

  Image
    **const image = targets->image;

  MagickStatusType
    *const state = targets->state;

  Image
    **const nexus = targets->nexus;

  const AnimateCommandTile
    tile = { display, resource_info, windows, image, state, exception };

  AnimateCommand
    animate_command;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  static int
    length;

  /*
    Respond to a user key press.
  */
  length=XLookupString((XKeyEvent *) &event.xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key press: 0x%lx (%c)",(unsigned long) key_symbol,*command);
  animate_command=NullCommand;
  switch (key_symbol)
  {
    case XK_o:
    {
      animate_command=XControlOpenCommand(event,animate_command);
      break;
    }
    default:
    {
      XPlaybackKeyCommand(key_symbol,&animate_command);
      break;
    }
  }
  if (animate_command != NullCommand)
    (*nexus)=XMagickCommand(&tile,animate_command);
}

static void XLogButtonRelease(XResourceInfo *resource_info,const XEvent event)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Release: 0x%lx %u +%d+%d",event.xbutton.window,
      event.xbutton.button,event.xbutton.x,event.xbutton.y);
}

static void XLogReparentNotify(XResourceInfo *resource_info,const XEvent event)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Reparent Notify: 0x%lx=>0x%lx",event.xreparent.parent,
      event.xreparent.window);
}

static void XLogUnknownEvent(XResourceInfo *resource_info,const XEvent event)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Event type: %d",
      event.type);
}

static void XHandleKeyRelease(XResourceInfo *resource_info,XEvent event)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  /*
    Respond to a user key release.
  */
  (void) XLookupString((XKeyEvent *) &event.xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key release: 0x%lx (%c)",(unsigned long) key_symbol,*command);
}

static void XHandleExpose(const AnimateEventContext *event_context,
  const XEvent event,const ssize_t scene)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Expose: 0x%lx %dx%d+%d+%d",event.xexpose.window,
      event.xexpose.width,event.xexpose.height,event.xexpose.x,
      event.xexpose.y);
  XRepaintExposedWindows(display,windows,scene,event);
}

static void XHandleButtonPress(const AnimateEventContext *event_context,
  const AnimateCommandTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  XEvent
    *const event = targets->event;

  MagickStatusType
    *const state = targets->state;

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
      (*event).xbutton.state&=(~(unsigned int) Mod1Mask);
    }
  if ((*event).xbutton.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,(*event).xbutton.window,RevertToParent,
        (*event).xbutton.time);
      return;
    }
  if ((*event).xbutton.window == windows->image.id)
    {
      AnimateCommandWindow
        command_window;

      command_window.display=display;
      command_window.resource_info=resource_info;
      command_window.windows=windows;
      command_window.CommandMenu=event_context->CommandMenu;
      (*state)=XPressImageWindow(&command_window,(*state));
    }
}

typedef struct _AnimateClientTargets
{
  MagickStatusType
    *state;

  XGCValues
    *context_values;

  Image
    **nexus;
} AnimateClientTargets;

static void XHandleClientMessage(const AnimateEventContext *event_context,
  const XEvent event,const AnimateClientTargets *client_targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  ExceptionInfo
    *const exception = event_context->exception;

  MagickStatusType
    *const state = client_targets->state;

  XGCValues
    *const context_values = client_targets->context_values;

  Image
    **const nexus = client_targets->nexus;

  AnimateDropSource
    drop_source;

  AnimateDropTargets
    targets;

  AnimateProtocolTargets
    protocol_targets;

  AnimateWindowColormaps
    window_colormaps;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Client Message: 0x%lx 0x%lx %d 0x%lx",(unsigned long)
      event.xclient.window,(unsigned long) event.xclient.message_type,
      event.xclient.format,(unsigned long) event.xclient.data.l[0]);
  window_colormaps.display=display;
  window_colormaps.windows=windows;
  window_colormaps.map_info=event_context->map_info;
  window_colormaps.pixel=event_context->pixel;
  window_colormaps.magick_windows=event_context->magick_windows;
  window_colormaps.number_windows=event_context->number_windows;
  window_colormaps.context_mask=event_context->context_mask;
  window_colormaps.event=event;
  protocol_targets.state=(state);
  protocol_targets.context_values=(context_values);
  if (XHandleMagickProtocol(&window_colormaps,&protocol_targets) ==
      MagickFalse)
    return;
  drop_source.display=display;
  drop_source.root_window=event_context->root_window;
  drop_source.resource_info=resource_info;
  drop_source.exception=exception;
  drop_source.event=event;
  drop_source.windows=windows;
  targets.nexus=(nexus);
  targets.state=(state);
  if (XHandleDropProtocol(&drop_source,&targets) == MagickFalse)
    return;
  (*state)=XHandleDeleteMessage(event_context,event,(*state));
}

static void XHandleEnterNotify(const AnimateEventContext *event_context,
  const XEvent event)
{
  Display
    *const display = event_context->display;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XStandardColormap
    *const map_info = event_context->map_info;

  /*
    Selectively install colormap.
  */
  if ((map_info->colormap !=
       XDefaultColormap(display,visual_info->screen)) &&
      (event.xcrossing.mode != NotifyUngrab))
    XInstallColormap(display,map_info->colormap);
}

static void XLogDisplayImage(XResourceInfo *resource_info,
  const Image *display_image)
{
  if (resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",display_image->filename,(double)
        display_image->scene,(double) display_image->columns,(double)
        display_image->rows);
      if (display_image->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          display_image->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        display_image->magick);
    }
}

static void XLogSceneImage(Image **image_list,const ssize_t scene,
  const unsigned int columns,const unsigned int rows)
{
  if (image_list[scene]->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: [%.17g] %s %.17gx%.17g ",(double) image_list[scene]->scene,
        image_list[scene]->filename,(double) columns,(double) rows);
      if (image_list[scene]->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image_list[scene]->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        image_list[scene]->magick);
    }
}

static void XCreateSceneWindows(const AnimateEventContext *event_context,
  const Image *images,XEvent *event)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XStandardColormap
    *const map_info = event_context->map_info;

  Image
    **const image_list = event_context->image_list;

  ExceptionInfo
    *const exception = event_context->exception;

  const size_t
    number_scenes = event_context->number_scenes;

  int
    status;

  ssize_t
    scene;

  for (scene=0; scene < (ssize_t) number_scenes; scene++)
  {
    AnimateSceneWindow
      scene_window;

    unsigned int
      columns,
      rows;

    /*
      Create X image.
    */
    windows->image.pixmap=(Pixmap) NULL;
    windows->image.matte_pixmap=(Pixmap) NULL;
    if ((IsTrueColorMap(resource_info->map_type,visual_info) != MagickFalse) &&
        (image_list[scene]->storage_class == PseudoClass))
      {
        const XPixelInfoSource
          pixel_source = { visual_info, map_info, resource_info };

        XGetPixelInfo(display,&pixel_source,image_list[scene],
          windows->image.pixel_info);
      }
    columns=(unsigned int) image_list[scene]->columns;
    rows=(unsigned int) image_list[scene]->rows;
    if ((image_list[scene]->columns != columns) ||
        (image_list[scene]->rows != rows))
      ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
        image_list[scene]->filename);
    {
      const XMakeImageRequest
        image_request = { &windows->image, image_list[scene], columns, rows };

      status=XMakeImage(display,resource_info,&image_request,
        exception) == MagickFalse ? 0 : 1;
    }
    if (status == MagickFalse)
      ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
        images->filename);
    XLogSceneImage(image_list,scene,columns,rows);
    /*
      Window name is the base of the filename.
    */
    scene_window.display=display;
    scene_window.resource_info=resource_info;
    scene_window.windows=windows;
    scene_window.image_list=image_list;
    scene_window.number_scenes=number_scenes;
    scene_window.exception=exception;
    scene_window.scene=scene;
    XNameSceneWindow(&scene_window);
    XStoreSceneWindow(&scene_window,event);
  }
}

static WarningHandler XEnterWorkingDirectory(XResourceInfo *resource_info,
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

static const char
  *const CommandMenu[]=
  {
    "Animate",
    "Speed",
    "Direction",
    "Help",
    "Image Info",
    "Quit",
    (char *) NULL
  },
  *const AnimateMenu[]=
  {
    "Open...",
    "Play",
    "Step",
    "Repeat",
    "Auto Reverse",
    "Save...",
    (char *) NULL
  },
  *const SpeedMenu[]=
  {
    "Faster",
    "Slower",
    (char *) NULL
  },
  *const DirectionMenu[]=
  {
    "Forward",
    "Reverse",
    (char *) NULL
  },
  *const HelpMenu[]=
  {
    "Overview",
    "Browse Documentation",
    "About Animate",
    (char *) NULL
  };

static const char
  *const *Menus[MagickMenus]=
  {
    AnimateMenu,
    SpeedMenu,
    DirectionMenu,
    HelpMenu
  };

static const AnimateCommand
  CommandMenus[]=
  {
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    InfoCommand,
    QuitCommand
  },
  AnimateCommands[]=
  {
    OpenCommand,
    PlayCommand,
    StepCommand,
    RepeatCommand,
    AutoReverseCommand,
    SaveCommand
  },
  SpeedCommands[]=
  {
    FasterCommand,
    SlowerCommand
  },
  DirectionCommands[]=
  {
    ForwardCommand,
    ReverseCommand
  },
  HelpCommands[]=
  {
    HelpCommand,
    BrowseDocumentationCommand,
    VersionCommand
  };

static const AnimateCommand
  *Commands[MagickMenus]=
  {
    AnimateCommands,
    SpeedCommands,
    DirectionCommands,
    HelpCommands
  };

static XWindows *XCreateAnimateWindows(Display *display,
  XResourceInfo *resource_info,Image *images)
{
  XWindows
    *windows;

  /*
    Initialize window structure.
  */
  XCountSceneColors(images,resource_info);
  windows=XSetWindows(XInitializeWindows(display,resource_info));
  if (windows == (XWindows *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
      images->filename);
  return(windows);
}

static void XLoadAnimateFont(Display *display,XResourceInfo *resource_info,
  XWindows *windows)
{
  /*
    Initialize font info.
  */
  if (windows->font_info != (XFontStruct *) NULL)
    (void) XFreeFont(display,windows->font_info);
  windows->font_info=XBestFont(display,resource_info,MagickFalse);
  if (windows->font_info == (XFontStruct *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToLoadFont",
      resource_info->font);
}

static Image *XCoalesceAnimateImages(Image *images,ExceptionInfo *exception)
{
  Image
    *coalesce_image;

  coalesce_image=CoalesceImages(images,exception);
  if (coalesce_image == (Image *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
      images->filename);
  images=coalesce_image;
  return(images);
}

static Image **XSortAnimateScenes(Image *images,const size_t number_scenes,
  ExceptionInfo *exception)
{
  Image
    **image_list;

  image_list=ImageListToArray(images,exception);
  if (image_list == (Image **) NULL)
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      images->filename);
  XSortSceneList(image_list,number_scenes);
  return(image_list);
}

typedef struct _AnimateWindowSetup
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XVisualInfo
    *visual_info;

  XStandardColormap
    *map_info;

  XPixelInfo
    *pixel;

  XFontStruct
    *font_info;

  Window
    root_window;

  XClassHint
    *class_hints;

  XWMHints
    *manager_hints;

  char
    **argv;

  int
    argc;

  XResourceInfo
    *icon_resources;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;

  Image
    *display_image;

  size_t
    number_scenes;
} AnimateWindowSetup;

static void XCreateContextWindow(const AnimateWindowSetup *setup)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XStandardColormap
    *const map_info = setup->map_info;

  XPixelInfo
    *const pixel = setup->pixel;

  XFontStruct
    *const font_info = setup->font_info;

  const Window
    root_window = setup->root_window;

  XClassHint
    *const class_hints = setup->class_hints;

  XWMHints
    *const manager_hints = setup->manager_hints;

  char
    **const argv = setup->argv;

  const int
    argc = setup->argc;

  const XMakeWindowHints
    hints = { argv, argc, class_hints, manager_hints };

  /*
    Initialize graphic context.
  */
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
  XMakeWindow(display,root_window,&hints,&windows->context);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (context)",windows->context.id);
}

static void XCreateIconWindow(const AnimateWindowSetup *setup)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  const Window
    root_window = setup->root_window;

  XClassHint
    *const class_hints = setup->class_hints;

  XWMHints
    *const manager_hints = setup->manager_hints;

  char
    **const argv = setup->argv;

  const int
    argc = setup->argc;

  XResourceInfo
    *const icon_resources = setup->icon_resources;

  XVisualInfo
    *const icon_visual = setup->icon_visual;

  XStandardColormap
    *const icon_map = setup->icon_map;

  XPixelInfo
    *const icon_pixel = setup->icon_pixel;

  Image
    *const display_image = setup->display_image;

  const XMakeWindowHints
    hints = { argv, argc, class_hints, manager_hints };

  /*
    Initialize icon window.
  */
  XGetWindowInfo(display,icon_visual,icon_map,icon_pixel,(XFontStruct *) NULL,
    icon_resources,&windows->icon);
  windows->icon.geometry=resource_info->icon_geometry;
  XBestIconSize(display,&windows->icon,display_image);
  windows->icon.attributes.colormap=
    XDefaultColormap(display,icon_visual->screen);
  windows->icon.attributes.event_mask=ExposureMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=IconicState;
  XMakeWindow(display,root_window,&hints,&windows->icon);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (icon)",
      windows->icon.id);
}

static void XCreateInfoWindow(const AnimateWindowSetup *setup)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XStandardColormap
    *const map_info = setup->map_info;

  XPixelInfo
    *const pixel = setup->pixel;

  XFontStruct
    *const font_info = setup->font_info;

  XClassHint
    *const class_hints = setup->class_hints;

  XWMHints
    *const manager_hints = setup->manager_hints;

  char
    **const argv = setup->argv;

  const int
    argc = setup->argc;

  const XMakeWindowHints
    hints = { argv, argc, class_hints, manager_hints };

  /*
    Initialize Info widget.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->info);
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
  if (windows->image.mapped)
    (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (info)",
      windows->info.id);
}

static void XCreateCommandWindow(const AnimateWindowSetup *setup)
{
  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  XStandardColormap
    *const map_info = setup->map_info;

  XPixelInfo
    *const pixel = setup->pixel;

  XFontStruct
    *const font_info = setup->font_info;

  const Window
    root_window = setup->root_window;

  XClassHint
    *const class_hints = setup->class_hints;

  XWMHints
    *const manager_hints = setup->manager_hints;

  char
    **const argv = setup->argv;

  const int
    argc = setup->argc;

  char
    resource_name[MagickPathExtent];

  const XMakeWindowHints
    hints = { argv, argc, class_hints, manager_hints };

  /*
    Initialize Command widget.
  */
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
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (command)",windows->command.id);
}

static void XChangeToHomeDirectory(XResourceInfo *resource_info,
  char *working_directory,ExceptionInfo *exception)
{
  char
    *directory;

  int
    status;

  directory=getcwd(working_directory,MagickPathExtent);
  (void) directory;
  if (*resource_info->home_directory == '\0')
    (void) CopyMagickString(resource_info->home_directory,".",MagickPathExtent);
  status=chdir(resource_info->home_directory);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",resource_info->home_directory);
}

#endif
