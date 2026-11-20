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

  MagickCore private methods of animate-window.c: the window set-up, the event
  loop and the session of XAnimateImages, from XReleaseAnimateWindows to
  XBindAnimateWindows; included by that file only.
*/
#ifndef MAGICKCORE_ANIMATE_WINDOW_SESSION_PRIVATE_H
#define MAGICKCORE_ANIMATE_WINDOW_SESSION_PRIVATE_H

static void XReleaseAnimateWindows(const AnimateWindowSetup *setup,
  const Image *nexus)
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

  XVisualInfo
    *const icon_visual = setup->icon_visual;

  XStandardColormap
    *const icon_map = setup->icon_map;

  XPixelInfo
    *const icon_pixel = setup->icon_pixel;

  if (nexus == (Image *) NULL)
    {
      AnimateX11Resources
        x11_resources;

      x11_resources.display=display;
      x11_resources.resource_info=resource_info;
      x11_resources.windows=windows;
      x11_resources.visual_info=visual_info;
      x11_resources.map_info=map_info;
      x11_resources.pixel=pixel;
      x11_resources.icon_visual=icon_visual;
      x11_resources.icon_map=icon_map;
      x11_resources.icon_pixel=icon_pixel;
      XReleaseAnimateResources(&x11_resources);
    }
}

static void XWithdrawBackdropWindows(Display *display,
  XResourceInfo *resource_info,XWindows *windows)
{
  if ((resource_info->backdrop == MagickFalse) &&
      (windows->backdrop.mapped))
    {
      (void) XWithdrawWindow(display,windows->backdrop.id,\
        windows->backdrop.screen);
      (void) XDestroyWindow(display,windows->backdrop.id);
      windows->backdrop.id=(Window) NULL;
      (void) XWithdrawWindow(display,windows->image.id,windows->image.screen);
      (void) XDestroyWindow(display,windows->image.id);
      windows->image.id=(Window) NULL;
    }
}

static void XCreateGraphicContexts(const AnimateWindowSetup *setup,
  const char *filename,XGCValues *context_values,
  MagickStatusType *context_mask)
{
  Display
    *const display = setup->display;

  XWindows
    *const windows = setup->windows;

  XPixelInfo
    *const pixel = setup->pixel;

  XFontStruct
    *const font_info = setup->font_info;

  /*
    Initialize graphic context.
  */
  (*context_values).background=pixel->background_color.pixel;
  (*context_values).font=font_info->fid;
  (*context_values).foreground=pixel->foreground_color.pixel;
  (*context_values).graphics_exposures=MagickFalse;
  *context_mask=(MagickStatusType)
    (GCBackground | GCFont | GCForeground | GCGraphicsExposures);
  if (pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,pixel->annotate_context);
  pixel->annotate_context=
    XCreateGC(display,windows->context.id,*context_mask,context_values);
  if (pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      filename);
  (*context_values).background=pixel->depth_color.pixel;
  if (pixel->widget_context != (GC) NULL)
    (void) XFreeGC(display,pixel->widget_context);
  pixel->widget_context=
    XCreateGC(display,windows->context.id,*context_mask,context_values);
  if (pixel->widget_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      filename);
  (*context_values).background=pixel->foreground_color.pixel;
  (*context_values).foreground=pixel->background_color.pixel;
  (*context_values).plane_mask=
    (*context_values).background ^ (*context_values).foreground;
  if (pixel->highlight_context != (GC) NULL)
    (void) XFreeGC(display,pixel->highlight_context);
  pixel->highlight_context=XCreateGC(display,windows->context.id,
    (size_t) (*context_mask | GCPlaneMask),context_values);
  if (pixel->highlight_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      filename);
  (void) XDestroyWindow(display,windows->context.id);
}

static void XCreateIconContext(const AnimateWindowSetup *setup,
  const char *filename,XGCValues *context_values)
{
  Display
    *const display = setup->display;

  XWindows
    *const windows = setup->windows;

  XPixelInfo
    *const icon_pixel = setup->icon_pixel;

  if (icon_pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,icon_pixel->annotate_context);
  (*context_values).background=icon_pixel->background_color.pixel;
  (*context_values).foreground=icon_pixel->foreground_color.pixel;
  icon_pixel->annotate_context=XCreateGC(display,windows->icon.id,
    (size_t) (GCBackground | GCForeground),context_values);
  if (icon_pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      filename);
  windows->icon.annotate_context=icon_pixel->annotate_context;
}

static void XCreateImageWindowInfo(const AnimateWindowSetup *setup,
  ExceptionInfo *exception)
{
  char
    geometry[MagickPathExtent];

  Display
    *const display = setup->display;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  XVisualInfo
    *const visual_info = setup->visual_info;

  Image
    *const display_image = setup->display_image;

  RectangleInfo
    geometry_info;

  XGetWindowInfo(display,visual_info,setup->map_info,setup->pixel,
    setup->font_info,resource_info,&windows->image);
  windows->image.shape=MagickTrue;  /* non-rectangular shape hint */
  if (resource_info->use_shared_memory == MagickFalse)
    windows->image.shared_memory=MagickFalse;
  if (resource_info->title != (char *) NULL)
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
      (void) FormatLocaleString(window_name,MagickPathExtent,
        "%s: %s[scene: %.17g frames: %.17g]",MagickPackageName,filename,(double)
        display_image->scene,(double) setup->number_scenes);
      (void) CloneString(&windows->image.name,window_name);
      (void) CloneString(&windows->image.icon_name,filename);
    }
  if (resource_info->immutable != MagickFalse)
    windows->image.immutable=MagickTrue;
  windows->image.shape=MagickTrue;
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
}

static void XCreateBackdropWindow(const AnimateWindowSetup *setup)
{
Display
    *const display = setup->display;

XResourceInfo
    *const resource_info = setup->resource_info;

XWindows
    *const windows = setup->windows;

XVisualInfo
    *const visual_info = setup->visual_info;

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

  XGetWindowInfo(display,visual_info,setup->map_info,setup->pixel,
    setup->font_info,resource_info,&windows->backdrop);
  if ((resource_info->backdrop) || (windows->backdrop.id != (Window) NULL))
    {
      /*
        Initialize backdrop window.
      */
      windows->backdrop.x=0;
      windows->backdrop.y=0;
      (void) CloneString(&windows->backdrop.name,"ImageMagick Backdrop");
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
      manager_hints->initial_state=
        resource_info->iconic ? IconicState : NormalState;
      XMakeWindow(display,root_window,&hints,&windows->backdrop);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (backdrop)",windows->backdrop.id);
      XCenterImageOnBackdrop(display,windows,visual_info);
    }
}

static void XCreateImageWindow(const AnimateWindowSetup *setup)
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

  const XMakeWindowHints
    hints = { argv, argc, class_hints, manager_hints };

  manager_hints->flags=IconWindowHint | InputHint | StateHint;
  manager_hints->icon_window=windows->icon.id;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=
    resource_info->iconic ? IconicState : NormalState;
  if (windows->group_leader.id != (Window) NULL)
    {
      /*
        Follow the leader.
      */
      manager_hints->flags|=(MagickStatusType) WindowGroupHint;
      manager_hints->window_group=windows->group_leader.id;
      (void) XSelectInput(display,windows->group_leader.id,StructureNotifyMask);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (group leader)",windows->group_leader.id);
    }
  XMakeWindow(display,
    (Window) (resource_info->backdrop ? windows->backdrop.id : root_window),
    &hints,&windows->image);
  (void) XChangeProperty(display,windows->image.id,windows->im_protocols,
    XA_STRING,8,PropModeReplace,(unsigned char *) NULL,0);
  if (windows->group_leader.id != (Window) NULL)
    (void) XSetTransientForHint(display,windows->image.id,
      windows->group_leader.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (image)",
      windows->image.id);
}

static void XCreatePanelWindows(const AnimateWindowSetup *setup)
{
  char
    resource_name[MagickPathExtent];

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

  const XMakeWindowHints
    hints = { argv, argc, class_hints, manager_hints };

  XGetWindowInfo(display,setup->visual_info,setup->map_info,setup->pixel,
    setup->font_info,resource_info,&windows->widget);
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
  /*
    Initialize popup window.
  */
  XGetWindowInfo(display,setup->visual_info,setup->map_info,setup->pixel,
    setup->font_info,resource_info,&windows->popup);
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

static void XHandleAnimateWindowEvent(
  const AnimateEventContext *event_context,XEvent event,
  const AnimateCommandTargets *command_targets)
{
  XResourceInfo
    *const resource_info = event_context->resource_info;

  switch (event.type)
  {
    case MapNotify:
    {
      XHandleMapNotify(event_context,event,command_targets);
      break;
    }
    case MappingNotify:
    {
      (void) XRefreshKeyboardMapping(&event.xmapping);
      break;
    }
    case NoExpose:
      break;
    case PropertyNotify:
    {
      XHandlePropertyNotify(event_context,event,command_targets);
      break;
    }
    case ReparentNotify:
    {
      XLogReparentNotify(resource_info,event);
      break;
    }
    case UnmapNotify:
    {
      XHandleUnmapNotify(event_context,event);
      break;
    }
    default:
    {
      XLogUnknownEvent(resource_info,event);
      break;
    }
  }
}

static void XHandleAnimateInputEvent(
  const AnimateEventContext *event_context,XEvent event,const ssize_t scene,
  const AnimateCommandTargets *command_targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XStandardColormap
    *const map_info = event_context->map_info;

  switch (event.type)
  {
    case Expose:
    {
      XHandleExpose(event_context,event,scene);
      break;
    }
    case KeyPress:
    {
      XHandleKeyPress(event_context,event,command_targets);
      break;
    }
    case KeyRelease:
    {
      XHandleKeyRelease(resource_info,event);
      break;
    }
    case LeaveNotify:
    {
      /*
        Selectively uninstall colormap.
      */
      if ((map_info->colormap !=
           XDefaultColormap(display,visual_info->screen)) &&
          (event.xcrossing.mode != NotifyUngrab))
        XUninstallColormap(display,map_info->colormap);
      break;
    }
    default:
    {
      XHandleAnimateWindowEvent(event_context,event,command_targets);
      break;
    }
  }
}

static void XCreateImagePixmaps(const AnimateWindowSetup *setup,
  const char *filename)
{
Display
    *const display = setup->display;

XWindows
    *const windows = setup->windows;

const size_t
    number_scenes = setup->number_scenes;

  XWindowChanges
    window_changes;

  windows->image.x=0;
  windows->image.y=0;
  /*
    Initialize image pixmaps structure.
  */
  window_changes.width=(int) windows->image.width;
  window_changes.height=(int) windows->image.height;
  (void) XReconfigureWMWindow(display,windows->image.id,windows->command.screen,
    (unsigned int) (CWWidth | CWHeight),&window_changes);
  windows->image.pixmaps=(Pixmap *) AcquireQuantumMemory(number_scenes,
    sizeof(*windows->image.pixmaps));
  windows->image.matte_pixmaps=(Pixmap *) AcquireQuantumMemory(number_scenes,
    sizeof(*windows->image.pixmaps));
  if ((windows->image.pixmaps == (Pixmap *) NULL) ||
      (windows->image.matte_pixmaps == (Pixmap *) NULL))
    ThrowXWindowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      filename);
  if ((windows->image.mapped == MagickFalse) ||
      (windows->backdrop.id != (Window) NULL))
    (void) XMapWindow(display,windows->image.id);
}

typedef struct _AnimatePlayTargets
{
  ssize_t
    *scene,
    *iterations;

  MagickStatusType
    *state;

  Image
    **image;

  ssize_t
    *first_scene;

  XEvent
    *event;
} AnimatePlayTargets;

static void XPlayNextScene(const AnimateEventContext *event_context,
  const AnimatePlayTargets *targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  Image
    **const image_list = event_context->image_list;

  const size_t
    number_scenes = event_context->number_scenes;

  size_t
    delay;

  ssize_t
    *const scene = targets->scene,
    *const iterations = targets->iterations,
    *const first_scene = targets->first_scene;

  MagickStatusType
    *const state = targets->state;

  Image
    **const image = targets->image;

  XEvent
    *const event = targets->event;

  AnimationTitle
    animation_title;

  MagickBooleanType
    pause;

  AnimationSceneStep
    scene_step;

  AnimateSceneView
    scene_view;

  const AnimationSceneTargets
    scene_targets = { scene, iterations, state, &pause };

  pause=MagickFalse;
  delay=1000*(*image)->delay/(size_t) MagickMax((*image)->ticks_per_second,
    1L);
  XDelay(display,resource_info->delay*(delay == 0 ? 10 : delay));
  scene_step.number_scenes=number_scenes;
  scene_step.image_list=image_list;
  scene_step.first_scene=(*first_scene);
  XAdvanceScene(&scene_step,&scene_targets);
  (*scene)=MagickMax((*scene),0);
  (*image)=image_list[(*scene)];
  (*first_scene)=XLoopStartScene((*image),(*scene),(*first_scene));
  animation_title.display=display;
  animation_title.resource_info=resource_info;
  animation_title.windows=windows;
  animation_title.image_list=image_list;
  animation_title.image=(*image);
  animation_title.exception=event_context->exception;
  animation_title.scene=(*scene);
  animation_title.number_scenes=number_scenes;
  animation_title.state=(*state);
  XUpdateAnimationTitle(&animation_title);
  /*
    Copy X pixmap to Image window.
  */
  scene_view.display=display;
  scene_view.resource_info=resource_info;
  scene_view.windows=windows;
  scene_view.visual_info=event_context->visual_info;
  scene_view.map_info=event_context->map_info;
  scene_view.image_list=image_list;
  scene_view.image=(*image);
  scene_view.scene=(*scene);
  XShowAnimationScene(&scene_view,(*state),event);
  (*state)&=(~StepAnimationState);
  XPauseAnimationScene(&scene_view,pause,event);
}

static MagickBooleanType IsAnimationStep(Display *display,
  const MagickStatusType state)
{
  return(((XEventsQueued(display,QueuedAfterFlush) == 0) &&
    (IsAnimationPlaying(state) != MagickFalse)) ? MagickTrue : MagickFalse);
}

static void XNextAnimateEvent(Display *display,XWindows *windows,
  XEvent *event)
{
  time_t
    timestamp;

  timestamp=GetMagickTime();
  (void) XNextEvent(display,event);
  if (windows->image.stasis == MagickFalse)
    windows->image.stasis=(GetMagickTime()-timestamp) > 0 ?
      MagickTrue : MagickFalse;
}

static void XHandleAnimateWindowChange(
  const AnimateEventContext *event_context,XEvent event,const ssize_t scene,
  const AnimateCommandTargets *command_targets)
{
  Display
    *const display = event_context->display;

  MagickStatusType
    *const state = command_targets->state;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  switch (event.type)
  {
    case ConfigureNotify:
    {
      XHandleConfigureNotify(display,resource_info,windows,event);
      break;
    }
    case DestroyNotify:
    {
      (*state)=XHandleDestroyNotify(resource_info,windows,event,(*state));
      break;
    }
    case EnterNotify:
    {
      XHandleEnterNotify(event_context,event);
      break;
    }
    default:
    {
      XHandleAnimateInputEvent(event_context,event,scene,command_targets);
      break;
    }
  }
}

static void XRunCommandWindow(const AnimateEventContext *event_context,
  char *command,const AnimateCommandTargets *command_targets)
{
  AnimateCommandWidget
    command_widget;

  /*
    Select a command from the Command widget.
  */
  command_widget.display=event_context->display;
  command_widget.resource_info=event_context->resource_info;
  command_widget.windows=event_context->windows;
  command_widget.exception=event_context->exception;
  command_widget.CommandMenu=CommandMenu;
  command_widget.CommandMenus=CommandMenus;
  command_widget.Menus=Menus;
  command_widget.Commands=Commands;
  command_widget.command=command;
  XRunCommandWidget(&command_widget,command_targets);
}

typedef struct _AnimateEventInput
{
  const AnimateCommandTargets
    *command_targets;

  const AnimateClientTargets
    *client_targets;

  ssize_t
    scene;
} AnimateEventInput;

static void XDispatchAnimateEvent(const AnimateEventContext *event_context,
  XEvent event,const AnimateEventInput *input)
{
  const AnimateClientTargets
    *const client_targets = input->client_targets;

  const AnimateCommandTargets
    *const command_targets = input->command_targets;

  const ssize_t
    scene = input->scene;

  XResourceInfo
    *const resource_info = event_context->resource_info;

switch (event.type)
{
  case ButtonPress:
  {
    XHandleButtonPress(event_context,command_targets);
    break;
  }
  case ButtonRelease:
  {
    XLogButtonRelease(resource_info,event);
    break;
  }
  case ClientMessage:
  {
    XHandleClientMessage(event_context,event,client_targets);
    break;
  }
  default:
  {
    XHandleAnimateWindowChange(event_context,event,scene,command_targets);
    break;
  }
}
}

static Image *XRunAnimateLoop(const AnimateEventContext *event_context,
  Image **images,XGCValues *context_values,XEvent *event)
{
  char
    command[MagickPathExtent];

  Display
    *const display = event_context->display;

  ExceptionInfo
    *const exception = event_context->exception;

  Image
    *image,
    **const image_list = event_context->image_list,
    *nexus;

  AnimateEventInput
    input;

  MagickStatusType
    state;

  ssize_t
    first_scene,
    iterations,
    scene;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  const AnimateCommandTargets
    command_targets = { event, &image, &state, &nexus };

  const AnimateClientTargets
    client_targets = { &state, context_values, &nexus };

  const AnimatePlayTargets
    play_targets = { &scene, &iterations, &state, &image, &first_scene,
      event };

  const AnimateCommandTile
    tile = { display, resource_info, windows, images, (&state), exception };

  nexus=NewImageList();
  scene=0;
  first_scene=0;
  iterations=0;
  image=image_list[0];
  state=(MagickStatusType) (ForwardAnimationState | RepeatAnimationState);
  (void) XMagickCommand(&tile,PlayCommand);
  do
  {
    if (IsAnimationStep(display,state) != MagickFalse)
      {
        XPlayNextScene(event_context,&play_targets);
        continue;
      }
    /*
      Handle a window event.
    */
    XNextAnimateEvent(display,windows,event);
    if ((*event).xany.window == windows->command.id)
      {
        XRunCommandWindow(event_context,command,&command_targets);
        continue;
      }
    input.command_targets=&command_targets;
    input.client_targets=&client_targets;
    input.scene=scene;
    XDispatchAnimateEvent(event_context,(*event),&input);
  }
  while (!(state & ExitState));
  return(nexus);
}

typedef struct _AnimateExitInfo
{
  Image
    *images;

  WarningHandler
    warning_handler;

  char
    *working_directory;
} AnimateExitInfo;

static Image *XFinishAnimation(const AnimateEventContext *event_context,
  const AnimateWindowSetup *setup,const AnimateExitInfo *exit_info,
  Image *nexus)
{
  Display
    *const display = event_context->display;

  ExceptionInfo
    *const exception = event_context->exception;

  Image
    *images = exit_info->images,
    **image_list = event_context->image_list;

  const size_t
    number_scenes = event_context->number_scenes;

  const WarningHandler
    warning_handler = exit_info->warning_handler;

  char
    *const working_directory = exit_info->working_directory;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  image_list=(Image **) RelinquishMagickMemory(image_list);
  images=DestroyImageList(images);
  XWithdrawColormapWindows(display,windows);
  XWithdrawBackdropWindows(display,resource_info,windows);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  XFreeScenePixmaps(display,windows,number_scenes);
  XSetCursorState(display,windows,MagickFalse);
  windows->image.pixmaps=(Pixmap *)
    RelinquishMagickMemory(windows->image.pixmaps);
  windows->image.matte_pixmaps=(Pixmap *)
    RelinquishMagickMemory(windows->image.matte_pixmaps);
  XReleaseAnimateWindows(setup,nexus);
  (void) XSync(display,MagickFalse);
  /*
    Restore our progress monitor and warning handlers.
  */
  (void) SetErrorHandler(warning_handler);
  (void) SetWarningHandler(warning_handler);
  /*
    Change to home directory.
  */
  XChangeToHomeDirectory(resource_info,working_directory,exception);
  return(nexus);
}

static void XInitializeMagickWindows(XWindows *windows,
  XWindowInfo **magick_windows,size_t *number_windows)
{
  (*number_windows)=0;
  magick_windows[(*number_windows)++]=(&windows->icon);
  magick_windows[(*number_windows)++]=(&windows->backdrop);
  magick_windows[(*number_windows)++]=(&windows->image);
  magick_windows[(*number_windows)++]=(&windows->info);
  magick_windows[(*number_windows)++]=(&windows->command);
  magick_windows[(*number_windows)++]=(&windows->widget);
  magick_windows[(*number_windows)++]=(&windows->popup);
  XClearMagickWindowIds(magick_windows,(*number_windows));
}

typedef struct _AnimateRunInputs
{
  Image
    **image_list;

  ExceptionInfo
    *exception;

  Image
    *images,
    *nexus;

  WarningHandler
    warning_handler;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;
} AnimateRunInputs;

static Image *XRunAnimateSession(const AnimateWindowSetup *setup,
  const AnimateRunInputs *inputs)
{
  Display
    *const display = setup->display;

  ExceptionInfo
    *const exception = inputs->exception;

  Image
    *images = inputs->images,
    *nexus = inputs->nexus;

  MagickStatusType
    context_mask;

  WarningHandler
    warning_handler = inputs->warning_handler;

  XEvent
    event;

  XGCValues
    context_values;

  XResourceInfo
    *const resource_info = setup->resource_info;

  XWindows
    *const windows = setup->windows;

  AnimateExitInfo
    exit_info;

  XCreateContextWindow(setup);
  XCreateGraphicContexts(setup,images->filename,&context_values,
    &context_mask);
  XCreateIconWindow(setup);
  /*
    Initialize graphic context for icon window.
  */
  XCreateIconContext(setup,images->filename,&context_values);
  /*
    Initialize Image window.
  */
  XCreateImageWindowInfo(setup,exception);
  XCreateBackdropWindow(setup);
  XCreateImageWindow(setup);
  XCreateInfoWindow(setup);
  XCreateCommandWindow(setup);
  /*
    Initialize Widget window.
  */
  XCreatePanelWindows(setup);
  /*
    Set out progress and warning handlers.
  */
  warning_handler=XSetAnimateWarningHandlers(resource_info,warning_handler);
  /*
    Initialize X image structure.
  */
  XCreateImagePixmaps(setup,images->filename);
  XSetCursorState(display,windows,MagickTrue);
  {
    const AnimateEventContext
      event_context = { display, resource_info, windows, setup->visual_info,
        setup->map_info, inputs->image_list, setup->display_image, exception,
        setup->icon_resources, setup->icon_visual, setup->icon_map,
        setup->icon_pixel, CommandMenu, setup->pixel, context_mask,
        setup->root_window, inputs->magick_windows, inputs->number_windows,
        setup->number_scenes };

    XCreateSceneWindows(&event_context,images,&event);
    XSetCursorState(display,windows,MagickFalse);
    if (windows->command.mapped)
      (void) XMapRaised(display,windows->command.id);
    /*
      Respond to events.
    */
    nexus=XRunAnimateLoop(&event_context,&images,&context_values,&event);
    exit_info.images=images;
    exit_info.warning_handler=warning_handler;
    exit_info.working_directory=inputs->working_directory;
    return(XFinishAnimation(&event_context,setup,&exit_info,nexus));
  }
}

typedef struct _AnimateStartInputs
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  char
    **argv;

  int
    argc;

  XWindows
    *windows;

  Image
    *images;

  ExceptionInfo
    *exception;

  WarningHandler
    warning_handler;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;

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

  XResourceInfo
    *icon_resources;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;
} AnimateStartInputs;

static Image *XStartAnimateSession(const AnimateStartInputs *start)
{
  Display
    *const display = start->display;

  ExceptionInfo
    *const exception = start->exception;

  Image
    *display_image,
    **image_list,
    *images = start->images,
    *nexus;

  size_t
    number_scenes;

  XResourceInfo
    *const resource_info = start->resource_info;

  AnimateSceneList
    scene_list;

  images=XCoalesceAnimateImages(images,exception);
  XRemapColormapMismatch(resource_info,start->visual_info,images,exception);
  /*
    Sort images by increasing scene number.
  */
  number_scenes=GetImageListLength(images);
  image_list=XSortAnimateScenes(images,number_scenes,exception);
  /*
    Initialize Standard Colormap.
  */
  nexus=NewImageList();
  scene_list.resource_info=resource_info;
  scene_list.visual_info=start->visual_info;
  scene_list.image_list=image_list;
  scene_list.number_scenes=number_scenes;
  scene_list.exception=exception;
  display_image=XPrepareSceneImages(&scene_list);
  XLogDisplayImage(resource_info,display_image);
  {
    const XStandardColormapRequest
      colormap_request = { start->visual_info, resource_info, display_image,
        start->map_info, start->pixel };

    XMakeStandardColormap(display,&colormap_request,exception);
  }
  {
    const AnimateWindowSetup
      setup = { display, resource_info, start->windows, start->visual_info,
        start->map_info, start->pixel, start->font_info, start->root_window,
        start->class_hints, start->manager_hints, start->argv, start->argc,
        start->icon_resources, start->icon_visual, start->icon_map,
        start->icon_pixel, display_image, number_scenes };

    const AnimateRunInputs
      inputs = { image_list, exception, images, nexus, start->warning_handler,
        start->working_directory, start->magick_windows,
        start->number_windows };

    return(XRunAnimateSession(&setup,&inputs));
  }
}

typedef struct _AnimateWindowInputs
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  char
    **argv;

  int
    argc;

  XWindows
    *windows;

  Image
    *images;

  ExceptionInfo
    *exception;

  WarningHandler
    warning_handler;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;
} AnimateWindowInputs;

static Image *XBindAnimateWindows(const AnimateWindowInputs *window_inputs)
{
  Display
    *const display = window_inputs->display;

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

  XWindows
    *const windows = window_inputs->windows;

  XWMHints
    *manager_hints;

  /*
    Initialize Standard Colormap.
  */
  map_info=windows->map_info;
  icon_map=windows->icon_map;
  visual_info=windows->visual_info;
  icon_visual=windows->icon_visual;
  pixel=windows->pixel_info;
  icon_pixel=windows->icon_pixel;
  font_info=windows->font_info;
  icon_resources=windows->icon_resources;
  class_hints=windows->class_hints;
  manager_hints=windows->manager_hints;
  root_window=XRootWindow(display,visual_info->screen);
  {
    const AnimateStartInputs
      start = { display, window_inputs->resource_info, window_inputs->argv,
        window_inputs->argc, windows, window_inputs->images,
        window_inputs->exception, window_inputs->warning_handler,
        window_inputs->working_directory, window_inputs->magick_windows,
        window_inputs->number_windows, visual_info, map_info, pixel, font_info,
        root_window, class_hints, manager_hints, icon_resources, icon_visual,
        icon_map, icon_pixel };

    return(XStartAnimateSession(&start));
  }
}

#endif
