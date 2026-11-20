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

  MagickCore private methods of animate-window.c: the scene, colormap and
  window helpers of XAnimateImages, from IsColormapVisual to
  XHandleConfigureNotify; included by that file only.
*/
#ifndef MAGICKCORE_ANIMATE_WINDOW_SCENE_PRIVATE_H
#define MAGICKCORE_ANIMATE_WINDOW_SCENE_PRIVATE_H

static MagickBooleanType IsColormapVisual(const XVisualInfo *visual_info)
{
  return(((visual_info->klass == GrayScale) ||
    (visual_info->klass == PseudoColor) ||
    (visual_info->klass == DirectColor)) ? MagickTrue : MagickFalse);
}

static void XFocusBackdropImage(Display *display,XWindows *windows)
{
  if (windows->backdrop.id != (Window) NULL)
    (void) XSetInputFocus(display,windows->image.id,RevertToParent,
      CurrentTime);
}

static Image *XFindColormapMismatch(Image *images,
  const XVisualInfo *visual_info)
{
  Image
    *next;

  ssize_t
    i;

  for (next=images; next != (Image *) NULL; )
  {
    next->alpha_trait=UndefinedPixelTrait;
    if (IsColormapMismatch(next,images,visual_info) != MagickFalse)
      break;
    i=FindColormapDifference(next,images);
    if (i < (ssize_t) images->colors)
      break;
    next=GetNextImageInList(next);
  }
  return(next);
}

static void XPlaceTransientWindows(Display *display,XWindows *windows,
  const XEvent event)
{
  if (event.xconfigure.send_event != 0)
    {
      XWindowChanges
        window_changes;

      /*
        Position the transient windows relative of the Image window.
      */
      if ((windows->command.geometry == (char *) NULL) &&
          (windows->command.mapped == MagickFalse))
        {
            windows->command.x=event.xconfigure.x-
              (int) windows->command.width-25;
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
          windows->widget.x=
            event.xconfigure.x+event.xconfigure.width/10;
          windows->widget.y=
            event.xconfigure.y+event.xconfigure.height/10;
          XConstrainWindowPosition(display,&windows->widget);
          window_changes.x=windows->widget.x;
          window_changes.y=windows->widget.y;
          (void) XReconfigureWMWindow(display,windows->widget.id,
            windows->widget.screen,(unsigned int) (CWX | CWY),
            &window_changes);
        }
    }
}

static AnimateCommand XControlOpenCommand(const XEvent event,
  AnimateCommand animate_command)
{
  if ((event.xkey.state & ControlMask) == MagickFalse)
    return(animate_command);
  animate_command=OpenCommand;
  return(animate_command);
}

typedef struct _AnimateIconColormaps
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    *display_image;

  ExceptionInfo
    *exception;

  XStandardColormap
    *map_info;

  XStandardColormap
    *icon_map;

  XVisualInfo
    *icon_visual;

  XPixelInfo
    *icon_pixel;
} AnimateIconColormaps;

static void XReleaseIconColormap(const AnimateIconColormaps *icon_colormaps)
{
  Display
    *const display = icon_colormaps->display;

  XResourceInfo
    *const resource_info = icon_colormaps->resource_info;

  XWindows
    *const windows = icon_colormaps->windows;

  Image
    *const display_image = icon_colormaps->display_image;

  ExceptionInfo
    *const exception = icon_colormaps->exception;

  XStandardColormap
    *const map_info = icon_colormaps->map_info;

  XStandardColormap
    *const icon_map = icon_colormaps->icon_map;

  XVisualInfo
    *const icon_visual = icon_colormaps->icon_visual;

  XPixelInfo
    *const icon_pixel = icon_colormaps->icon_pixel;

  if (map_info->colormap == icon_map->colormap)
    XConfigureImageColormap(display,resource_info,windows,
      display_image,exception);
  (void) XFreeStandardColormap(display,icon_visual,icon_map,
    icon_pixel);
  windows->icon.mapped=MagickFalse;
}

typedef struct _AnimateImageWindow
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XStandardColormap
    *map_info;

  Image
    **image_list;

  Image
    *display_image;

  ExceptionInfo
    *exception;
} AnimateImageWindow;

static void XMapAnimateImageWindow(const AnimateImageWindow *image_window,
  Image **nexus,Image **image,MagickStatusType *state)
{
  Display
    *const display = image_window->display;

  XResourceInfo
    *const resource_info = image_window->resource_info;

  XWindows
    *const windows = image_window->windows;

  XStandardColormap
    *const map_info = image_window->map_info;

  Image
    **const image_list = image_window->image_list;

  Image
    *const display_image = image_window->display_image;

  ExceptionInfo
    *const exception = image_window->exception;

  const AnimateCommandTile
    tile = { display, resource_info, windows, image, state, exception };

  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
  if (LocaleCompare(image_list[0]->magick,"LOGO") == 0)
    {
      if (LocaleCompare(display_image->filename,"LOGO") == 0)
        (*nexus)=XMagickCommand(&tile,OpenCommand);
      else
        (*state)|=ExitState;
    }
  windows->image.mapped=MagickTrue;
}

typedef struct _AnimateCommandWindow
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *CommandMenu;
} AnimateCommandWindow;

static MagickStatusType XPressImageWindow(
  const AnimateCommandWindow *command_window,MagickStatusType state)
{
  Display
    *const display = command_window->display;

  XResourceInfo
    *const resource_info = command_window->resource_info;

  XWindows
    *const windows = command_window->windows;

  const char
    *const *const CommandMenu = command_window->CommandMenu;

  if (resource_info->immutable != MagickFalse)
    {
      state|=ExitState;
      return(state);
    }
  /*
    Map/unmap Command widget.
  */
  if (windows->command.mapped)
    (void) XWithdrawWindow(display,windows->command.id,
      windows->command.screen);
  else
    {
      (void) XCommandWidget(display,windows,CommandMenu,
        (XEvent *) NULL);
      (void) XMapRaised(display,windows->command.id);
    }
  return(state);
}

#define MagickMenus  4
#define MaXWindows  8
#define MagickTitle  "Commands"

typedef struct _AnimateMenuSelection
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *CommandMenu,
    *const **Menus;

  const AnimateCommand
    **Commands;

  int
    id;
} AnimateMenuSelection;

static MagickBooleanType XSelectMenuCommand(
  const AnimateMenuSelection *menu_selection,char *command,
  AnimateCommand *animate_command)
{
  Display
    *const display = menu_selection->display;

  XWindows
    *const windows = menu_selection->windows;

  const char
    *const *const CommandMenu = menu_selection->CommandMenu,
    *const **const Menus = menu_selection->Menus;

  const AnimateCommand
    **const Commands = menu_selection->Commands;

  const int
    id = menu_selection->id;

  if (id < MagickMenus)
    {
      int
        entry;

      /*
        Select a command from a pop-up menu.
      */
      entry=XMenuWidget(display,windows,CommandMenu[id],Menus[id],
        command);
      if (entry < 0)
        return(MagickFalse);
      (void) CopyMagickString(command,Menus[id][entry],MagickPathExtent);
      *animate_command=Commands[id][entry];
    }
  return(MagickTrue);
}

typedef struct _AnimateWindowColormaps
{
  Display
    *display;

  XWindows
    *windows;

  XStandardColormap
    *map_info;

  XPixelInfo
    *pixel;

  XWindowInfo
    **magick_windows;

  size_t
    number_windows;

  MagickStatusType
    context_mask;

  XEvent
    event;
} AnimateWindowColormaps;

static void XUpdateWindowColormaps(
  const AnimateWindowColormaps *window_colormaps,XGCValues *context_values)
{
  Display
    *const display = window_colormaps->display;

  XWindows
    *const windows = window_colormaps->windows;

  XStandardColormap
    *const map_info = window_colormaps->map_info;

  XPixelInfo
    *const pixel = window_colormaps->pixel;

  XWindowInfo
    **const magick_windows = window_colormaps->magick_windows;

  const size_t
    number_windows = window_colormaps->number_windows;

  const MagickStatusType
    context_mask = window_colormaps->context_mask;

  ssize_t
    i;

  /*
    Update graphic context and window colormap.
  */
  for (i=0; i < (ssize_t) number_windows; i++)
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
    (*context_values).plane_mask=
      (*context_values).background ^ (*context_values).foreground;
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
  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
}

typedef struct _AnimateDropSource
{
  Display
    *display;

  Window
    root_window;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  XEvent
    event;

  XWindows
    *windows;
} AnimateDropSource;

typedef struct _AnimateDropTargets
{
  Image
    **nexus;

  MagickStatusType
    *state;
} AnimateDropTargets;

static MagickBooleanType XReadDroppedImage(
  const AnimateDropSource *drop_source,const AnimateDropTargets *targets)
{
  Display
    *const display = drop_source->display;

  const Window
    root_window = drop_source->root_window;

  XResourceInfo
    *const resource_info = drop_source->resource_info;

  ExceptionInfo
    *const exception = drop_source->exception;

  const XEvent
    event = drop_source->event;

  Image
    **const nexus = targets->nexus;

  MagickStatusType
    *const state = targets->state;

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
    return(MagickFalse);
  selection=XInternAtom(display,"DndSelection",MagickFalse);
  status=XGetWindowProperty(display,root_window,selection,0L,2047L,
    MagickFalse,(Atom) AnyPropertyType,&type,&format,&length,&after,
    &data);
  if ((status != Success) || (length == 0))
    return(MagickFalse);
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
      if (LocaleNCompare((char *) data,"file:",5) != 0)
        {
          (void) XFree((void *) data);
          return(MagickFalse);
        }
      (void) CopyMagickString(resource_info->image_info->filename,
        ((char *) data)+5,MagickPathExtent);
    }
  *nexus=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if (*nexus != (Image *) NULL)
    *state|=ExitState;
  (void) XFree((void *) data);
  return(MagickTrue);
}

typedef struct _AnimateProtocolTargets
{
  MagickStatusType
    *state;

  XGCValues
    *context_values;
} AnimateProtocolTargets;

static MagickBooleanType XHandleMagickProtocol(
  const AnimateWindowColormaps *window_colormaps,
  const AnimateProtocolTargets *targets)
{
  XWindows
    *const windows = window_colormaps->windows;

  const XEvent
    event = window_colormaps->event;

  MagickStatusType
    *const state = targets->state;

  XGCValues
    *const context_values = targets->context_values;

  if (event.xclient.message_type == windows->im_protocols)
    {
      if (*event.xclient.data.l == (long) windows->im_update_colormap)
        {
          XUpdateWindowColormaps(window_colormaps,context_values);
          return(MagickFalse);
        }
      if (*event.xclient.data.l == (long) windows->im_exit)
        {
          *state|=ExitState;
          return(MagickFalse);
        }
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType XHandleDropProtocol(
  const AnimateDropSource *drop_source,const AnimateDropTargets *targets)
{
  XWindows
    *const windows = drop_source->windows;

  const XEvent
    event = drop_source->event;

  if (event.xclient.message_type == windows->dnd_protocols)
    {
      if (XReadDroppedImage(drop_source,targets) == MagickFalse)
        return(MagickFalse);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XPauseAnimation(Display *display,XResourceInfo *resource_info,
  XWindows *windows,XEvent *event)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  ssize_t
    i;

  for (i=0; i < (ssize_t) resource_info->pause; i++)
  {
    int
      status;

    status=XCheckTypedWindowEvent(display,windows->image.id,KeyPress,
      event);
    if (status != 0)
      {
        int
          length;

        length=XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
          sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if ((key_symbol == XK_q) || (key_symbol == XK_Escape))
          {
            XClientMessage(display,windows->image.id,
              windows->im_protocols,windows->im_exit,CurrentTime);
            break;
          }
      }
    MagickDelay(1000);
  }
}

typedef struct _AnimationTitle
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image_list;

  Image
    *image;

  ExceptionInfo
    *exception;

  ssize_t
    scene;

  size_t
    number_scenes;

  MagickStatusType
    state;
} AnimationTitle;

static void XUpdateAnimationTitle(const AnimationTitle *animation_title)
{
  Display
    *const display = animation_title->display;

  XResourceInfo
    *const resource_info = animation_title->resource_info;

  XWindows
    *const windows = animation_title->windows;

  Image
    **const image_list = animation_title->image_list;

  Image
    *const image = animation_title->image;

  ExceptionInfo
    *const exception = animation_title->exception;

  const ssize_t
    scene = animation_title->scene;

  const size_t
    number_scenes = animation_title->number_scenes;

  const MagickStatusType
    state = animation_title->state;

  char
    *p;

  int
    status;

  XTextProperty
    window_name;

  if ((state & StepAnimationState) ||
      (resource_info->title != (char *) NULL))
    {
      char
        name[MagickPathExtent];

      /*
        Update window title.
      */
      p=image_list[scene]->filename+
        strlen(image_list[scene]->filename)-1;
      while ((p > image_list[scene]->filename) && (*(p-1) != '/'))
        p--;
      (void) FormatLocaleString(name,MagickPathExtent,
        "%s: %s[%.17g of %.17g]",MagickPackageName,p,(double)
        scene+1,(double) number_scenes);
      (void) CloneString(&windows->image.name,name);
      if (resource_info->title != (char *) NULL)
        {
          char
            *title;

          title=InterpretImageProperties(resource_info->image_info,
            image,resource_info->title,exception);
          (void) CloneString(&windows->image.name,title);
          title=DestroyString(title);
        }
      status=XStringListToTextProperty(&windows->image.name,1,
        &window_name);
      if (status != Success)
        {
          XSetWMName(display,windows->image.id,&window_name);
          (void) XFree((void *) window_name.value);
        }
    }
}

typedef struct _AnimationSceneStep
{
  size_t
    number_scenes;

  Image
    **image_list;

  ssize_t
    first_scene;
} AnimationSceneStep;

typedef struct _AnimationSceneTargets
{
  ssize_t
    *scene,
    *iterations;

  MagickStatusType
    *state;

  MagickBooleanType
    *pause;
} AnimationSceneTargets;

static void XWrapSceneForward(const AnimationSceneStep *scene_step,
  const AnimationSceneTargets *targets)
{
  Image
    **const image_list = scene_step->image_list;

  const ssize_t
    first_scene = scene_step->first_scene;

  ssize_t
    *const scene = targets->scene,
    *const iterations = targets->iterations;

  MagickStatusType
    *const state = targets->state;

  MagickBooleanType
    *const pause = targets->pause;

  (*iterations)++;
  if ((*iterations) == (ssize_t) image_list[0]->iterations)
    {
      (*iterations)=0;
      (*state)|=ExitState;
    }
  if (((*state) & AutoReverseAnimationState) != 0)
    {
      (*state)&=(~ForwardAnimationState);
      (*scene)--;
    }
  else
    {
      if (((*state) & RepeatAnimationState) == 0)
        (*state)&=(~PlayAnimationState);
      (*scene)=first_scene;
      (*pause)=MagickTrue;
    }
}

static void XWrapSceneReverse(const AnimationSceneStep *scene_step,
  const AnimationSceneTargets *targets)
{
  const size_t
    number_scenes = scene_step->number_scenes;

  Image
    **const image_list = scene_step->image_list;

  const ssize_t
    first_scene = scene_step->first_scene;

  ssize_t
    *const scene = targets->scene,
    *const iterations = targets->iterations;

  MagickStatusType
    *const state = targets->state;

  MagickBooleanType
    *const pause = targets->pause;

  (*iterations)++;
  if ((*iterations) == (ssize_t) image_list[0]->iterations)
    {
      (*iterations)=0;
      (*state)&=(~RepeatAnimationState);
    }
  if ((*state) & AutoReverseAnimationState)
    {
      (*state)|=ForwardAnimationState;
      (*scene)=first_scene;
      (*pause)=MagickTrue;
    }
  else
    {
      if (((*state) & RepeatAnimationState) == MagickFalse)
        (*state)&=(~PlayAnimationState);
      (*scene)=(ssize_t) number_scenes-1;
    }
}

static inline MagickBooleanType IsAnimationPlaying(
  const MagickStatusType state)
{
  return(((state & PlayAnimationState) || (state & StepAnimationState)) ?
    MagickTrue : MagickFalse);
}

static void XAdvanceSceneForward(const AnimationSceneStep *scene_step,
  const AnimationSceneTargets *targets)
{
  const size_t
    number_scenes = scene_step->number_scenes;

  ssize_t
    *const scene = targets->scene;

  if ((*scene) < ((ssize_t) number_scenes-1))
    (*scene)++;
  else
    XWrapSceneForward(scene_step,targets);
}

static void XAdvanceScene(const AnimationSceneStep *scene_step,
  const AnimationSceneTargets *targets)
{
  const ssize_t
    first_scene = scene_step->first_scene;

  ssize_t
    *const scene = targets->scene;

  MagickStatusType
    *const state = targets->state;

  if ((*state) & ForwardAnimationState)
    {
      /*
        Forward animation:  increment scene number.
      */
      XAdvanceSceneForward(scene_step,targets);
    }
  else
    {
      /*
        Reverse animation:  decrement scene number.
      */
      if ((*scene) > first_scene)
        (*scene)--;
      else
        XWrapSceneReverse(scene_step,targets);
    }
}

typedef struct _AnimateSceneView
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
    *image;

  ssize_t
    scene;
} AnimateSceneView;

static void XShowAnimationScene(const AnimateSceneView *scene_view,
  const MagickStatusType state,XEvent *event)
{
  Display
    *const display = scene_view->display;

  XResourceInfo
    *const resource_info = scene_view->resource_info;

  XWindows
    *const windows = scene_view->windows;

  XVisualInfo
    *const visual_info = scene_view->visual_info;

  XStandardColormap
    *const map_info = scene_view->map_info;

  Image
    **const image_list = scene_view->image_list;

  Image
    *const image = scene_view->image;

  const ssize_t
    scene = scene_view->scene;

  XGetPixelInfo(display,visual_info,map_info,resource_info,
    image_list[scene],windows->image.pixel_info);
  if (image != (Image *) NULL)
    {
      windows->image.ximage->width=(int) image->columns;
      windows->image.ximage->height=(int) image->rows;
    }
  windows->image.pixmap=windows->image.pixmaps[scene];
  windows->image.matte_pixmap=windows->image.matte_pixmaps[scene];
  (*event).xexpose.x=0;
  (*event).xexpose.y=0;
  (*event).xexpose.width=(int) image->columns;
  (*event).xexpose.height=(int) image->rows;
  if ((state & ExitState) == 0)
    {
      XRefreshWindow(display,&windows->image,event);
      (void) XSync(display,MagickFalse);
    }
}

static void XPauseAnimationScene(const AnimateSceneView *scene_view,
  const MagickBooleanType pause,XEvent *event)
{
  Display
    *const display = scene_view->display;

  XResourceInfo
    *const resource_info = scene_view->resource_info;

  XWindows
    *const windows = scene_view->windows;

  if (pause != MagickFalse)
    XPauseAnimation(display,resource_info,windows,event);
}

static ssize_t XLoopStartScene(const Image *image,const ssize_t scene,
  ssize_t first_scene)
{
  if ((image != (Image *) NULL) && (image->start_loop != 0))
    first_scene=scene;
  return(first_scene);
}

static void XRemapColormapMismatch(XResourceInfo *resource_info,
  XVisualInfo *visual_info,Image *images,ExceptionInfo *exception)
{
  if (resource_info->map_type == (char *) NULL)
    if ((visual_info->klass != TrueColor) &&
        (visual_info->klass != DirectColor))
      {
        Image
          *next;

        /*
          Determine if the sequence of images has the identical colormap.
        */
        next=XFindColormapMismatch(images,visual_info);
        if (next != (Image *) NULL)
          (void) RemapImages(resource_info->quantize_info,images,
            (Image *) NULL,exception);
      }
}

static void XWithdrawColormapWindows(Display *display,XWindows *windows)
{
  if (IsColormapVisual(windows->visual_info) != MagickFalse)
    {
      /*
        Withdraw windows.
      */
      if (windows->info.mapped)
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      if (windows->command.mapped)
        (void) XWithdrawWindow(display,windows->command.id,
          windows->command.screen);
    }
}

static void XFreeScenePixmaps(Display *display,XWindows *windows,
  const size_t number_scenes)
{
  ssize_t
    scene;

  for (scene=1; scene < (ssize_t) number_scenes; scene++)
  {
    if (windows->image.pixmaps[scene] != (Pixmap) NULL)
      (void) XFreePixmap(display,windows->image.pixmaps[scene]);
    windows->image.pixmaps[scene]=(Pixmap) NULL;
    if (windows->image.matte_pixmaps[scene] != (Pixmap) NULL)
      (void) XFreePixmap(display,windows->image.matte_pixmaps[scene]);
    windows->image.matte_pixmaps[scene]=(Pixmap) NULL;
  }
}

typedef struct _AnimateX11Resources
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

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;
} AnimateX11Resources;

static void XReleaseAnimateResources(
  const AnimateX11Resources *x11_resources)
{
  Display
    *const display = x11_resources->display;

  XResourceInfo
    *const resource_info = x11_resources->resource_info;

  XWindows
    *const windows = x11_resources->windows;

  XVisualInfo
    *const visual_info = x11_resources->visual_info;

  XStandardColormap
    *const map_info = x11_resources->map_info;

  XPixelInfo
    *const pixel = x11_resources->pixel;

  XVisualInfo
    *const icon_visual = x11_resources->icon_visual;

  XStandardColormap
    *const icon_map = x11_resources->icon_map;

  XPixelInfo
    *const icon_pixel = x11_resources->icon_pixel;

  /*
    Free X resources.
  */
  if (windows->image.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->image.id,windows->image.screen);
  XDelay(display,SuspendTime);
  (void) XFreeStandardColormap(display,icon_visual,icon_map,icon_pixel);
  if (resource_info->map_type == (char *) NULL)
    (void) XFreeStandardColormap(display,visual_info,map_info,pixel);
  DestroyXResources();
}

static WarningHandler XSetAnimateWarningHandlers(
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

typedef struct _AnimateSceneList
{
  XResourceInfo
    *resource_info;

  XVisualInfo
    *visual_info;

  Image
    **image_list;

  size_t
    number_scenes;

  ExceptionInfo
    *exception;
} AnimateSceneList;

static Image *XPrepareSceneImages(const AnimateSceneList *scene_list)
{
  XResourceInfo
    *const resource_info = scene_list->resource_info;

  XVisualInfo
    *const visual_info = scene_list->visual_info;

  Image
    **const image_list = scene_list->image_list;

  const size_t
    number_scenes = scene_list->number_scenes;

  ExceptionInfo
    *const exception = scene_list->exception;

  Image
    *display_image;

  ssize_t
    scene;

  display_image=image_list[0];
  for (scene=0; scene < (ssize_t) number_scenes; scene++)
  {
    if (IsTrueColorMap(resource_info->map_type,visual_info) != MagickFalse)
      (void) SetImageType(image_list[scene],image_list[scene]->alpha_trait ==
        BlendPixelTrait ? TrueColorType : TrueColorAlphaType,exception);
    if ((display_image->columns < image_list[scene]->columns) &&
        (display_image->rows < image_list[scene]->rows))
      display_image=image_list[scene];
  }
  return(display_image);
}

static void XCenterImageOnBackdrop(Display *display,XWindows *windows,
  XVisualInfo *visual_info)
{
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

typedef struct _AnimateSceneWindow
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image_list;

  size_t
    number_scenes;

  ExceptionInfo
    *exception;

  ssize_t
    scene;
} AnimateSceneWindow;

static void XNameSceneWindow(const AnimateSceneWindow *scene_window)
{
  XResourceInfo
    *const resource_info = scene_window->resource_info;

  XWindows
    *const windows = scene_window->windows;

  Image
    **const image_list = scene_window->image_list;

  const size_t
    number_scenes = scene_window->number_scenes;

  ExceptionInfo
    *const exception = scene_window->exception;

  const ssize_t
    scene = scene_window->scene;

  char
    *p;

  if (resource_info->title != (char *) NULL)
    {
      char
        *title;

      title=InterpretImageProperties(resource_info->image_info,
        image_list[scene],resource_info->title,exception);
      (void) CloneString(&windows->image.name,title);
      title=DestroyString(title);
    }
  else
    {
      char
        window_name[MagickPathExtent];

      p=image_list[scene]->magick_filename+
        strlen(image_list[scene]->magick_filename)-1;
      while ((p > image_list[scene]->magick_filename) && (*(p-1) != '/'))
        p--;
      (void) FormatLocaleString(window_name,MagickPathExtent,
        "%s: %s[%.17g of %.17g]",MagickPackageName,p,(double) scene+1,
        (double) number_scenes);
      (void) CloneString(&windows->image.name,window_name);
    }
}

static void XStoreSceneWindow(const AnimateSceneWindow *scene_window,
  XEvent *event)
{
  Display
    *const display = scene_window->display;

  XWindows
    *const windows = scene_window->windows;

  Image
    **const image_list = scene_window->image_list;

  const ssize_t
    scene = scene_window->scene;

  int
    status;

  XTextProperty
    window_name;

  status=XStringListToTextProperty(&windows->image.name,1,&window_name);
  if (status != Success)
    {
      XSetWMName(display,windows->image.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  windows->image.pixmaps[scene]=windows->image.pixmap;
  windows->image.matte_pixmaps[scene]=windows->image.matte_pixmap;
  if (scene == 0)
    {
      (*event).xexpose.x=0;
      (*event).xexpose.y=0;
      (*event).xexpose.width=(int) image_list[scene]->columns;
      (*event).xexpose.height=(int) image_list[scene]->rows;
      XRefreshWindow(display,&windows->image,event);
      (void) XSync(display,MagickFalse);
  }
}

static void XCountSceneColors(Image *images,XResourceInfo *resource_info)
{
  Image
    *p;

  for (p=images; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (p->storage_class == DirectClass)
      {
        resource_info->colors=0;
        break;
      }
    if (p->colors > resource_info->colors)
      resource_info->colors=p->colors;
  }
}

static void XClearMagickWindowIds(XWindowInfo **magick_windows,
  const size_t number_windows)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) number_windows; i++)
    magick_windows[i]->id=(Window) NULL;
}

typedef struct _AnimateCommandWidget
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  const char
    *const *CommandMenu;

  const AnimateCommand
    *CommandMenus;

  const char
    *const **Menus;

  const AnimateCommand
    **Commands;

  char
    *command;
} AnimateCommandWidget;

typedef struct _AnimateCommandTargets
{
  XEvent
    *event;

  Image
    **image;

  MagickStatusType
    *state;

  Image
    **nexus;
} AnimateCommandTargets;

static void XRunCommandWidget(const AnimateCommandWidget *command_widget,
  const AnimateCommandTargets *targets)
{
  XEvent
    *const event = targets->event;

  Image
    **const image = targets->image;

  MagickStatusType
    *const state = targets->state;

  Image
    **const nexus = targets->nexus;

  AnimateCommand
    animate_command;

  AnimateCommandTile
    tile;

  AnimateMenuSelection
    menu_selection;

  int
    id;

  id=XCommandWidget(command_widget->display,command_widget->windows,
    command_widget->CommandMenu,&(*event));
  if (id < 0)
    return;
  (void) CopyMagickString(command_widget->command,
    command_widget->CommandMenu[id],MagickPathExtent);
  animate_command=command_widget->CommandMenus[id];
  menu_selection.display=command_widget->display;
  menu_selection.windows=command_widget->windows;
  menu_selection.CommandMenu=command_widget->CommandMenu;
  menu_selection.Menus=command_widget->Menus;
  menu_selection.Commands=command_widget->Commands;
  menu_selection.id=id;
  if (XSelectMenuCommand(&menu_selection,command_widget->command,
      &animate_command) == MagickFalse)
    return;
  if (animate_command != NullCommand)
    {
      tile.display=command_widget->display;
      tile.resource_info=command_widget->resource_info;
      tile.windows=command_widget->windows;
      tile.image=image;
      tile.state=state;
      tile.exception=command_widget->exception;
      (*nexus)=XMagickCommand(&tile,animate_command);
    }
}

static void XHandleConfigureNotify(Display *display,
  XResourceInfo *resource_info,XWindows *windows,const XEvent event)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Configure Notify: 0x%lx %dx%d+%d+%d %d",event.xconfigure.window,
      event.xconfigure.width,event.xconfigure.height,event.xconfigure.x,
      event.xconfigure.y,event.xconfigure.send_event);
  if (event.xconfigure.window == windows->image.id)
    {
      XPlaceTransientWindows(display,windows,event);
      /*
        Image window has a new configuration.
      */
      windows->image.width=(unsigned int) event.xconfigure.width;
      windows->image.height=(unsigned int) event.xconfigure.height;
      return;
    }
  if (event.xconfigure.window == windows->icon.id)
    {
      /*
        Icon window has a new configuration.
      */
      windows->icon.width=(unsigned int) event.xconfigure.width;
      windows->icon.height=(unsigned int) event.xconfigure.height;
      return;
    }
}

#endif
