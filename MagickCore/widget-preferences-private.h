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

  MagickCore private methods of widget.c: the preferences widget, the run
  between XNoticeWidget and XPreferencesWidget; included by that file only.
*/
#ifndef MAGICKCORE_WIDGET_PREFERENCES_PRIVATE_H
#define MAGICKCORE_WIDGET_PREFERENCES_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X P r e f e r e n c e s W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPreferencesWidget() displays a Preferences widget with program preferences.
%  If the user presses the Apply button, the preferences are stored in a
%  configuration file in the users' home directory.
%
%  The format of the XPreferencesWidget method is:
%
%      MagickBooleanType XPreferencesWidget(Display *display,
%        XResourceInfo *resource_info,XWindows *windows)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o window: Specifies a pointer to a XWindows structure.
%
*/
#define ApplyButtonText  "Apply"
#define CacheButtonText  "%lu mega-bytes of memory in the undo edit cache   "
#define CancelButtonText  "Cancel"
#define NumberPreferences  8

static size_t ReleasePreferencesButton(XWindows *windows,const XEvent event,
  const XWidgetInfo button_info,size_t state)
{
  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(button_info,event.xbutton)))
    state|=ExitState;
  return(state);
}

static void TogglePreferencesButton(Display *display,XWindows *windows,
  const XEvent event,XWidgetInfo *preferences_info)
{
  int
    i;

  for (i=0; i < NumberPreferences; i++)
    if (MatteIsActive(preferences_info[i],event.xbutton))
      {
        /*
          User pressed a Preferences button.
        */
        preferences_info[i].raised=preferences_info[i].raised ==
          MagickFalse ? MagickTrue : MagickFalse;
        XDrawBeveledButton(display,&windows->widget,&preferences_info[i]);
        break;
      }
}

static void DoublePreferencesUndoCache(XResourceInfo *resource_info)
{
  resource_info->undo_cache<<=1;
  if (resource_info->undo_cache > 256)
    resource_info->undo_cache=1;
}

static const char
  *Preferences[] =
  {
    "display image centered on a backdrop",
    "confirm on program exit",
    "confirm on image edits",
    "correct image for display gamma",
    "display warning messages",
    "apply Floyd/Steinberg error diffusion to image",
    "use a shared colormap for colormapped X visuals",
    "display images as an X server pixmap"
  };

static void MapPreferencesWidget(Display *display,XWindows *windows)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,"Preferences",MagickPathExtent);
  status=XStringListToTextProperty(&windows->widget.name,1,&window_name);
  if (status != False)
    {
      XSetWMName(display,windows->widget.id,&window_name);
      XSetWMIconName(display,windows->widget.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  window_changes.width=(int) windows->widget.width;
  window_changes.height=(int) windows->widget.height;
  window_changes.x=windows->widget.x;
  window_changes.y=windows->widget.y;
  (void) XReconfigureWMWindow(display,windows->widget.id,windows->widget.screen,
    (unsigned int) (CWWidth | CWHeight | CWX | CWY),&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

static unsigned int GetPreferencesTextWidth(XFontStruct *font_info)
{
  int
    i;

  unsigned int
    text_width;

  text_width=WidgetTextWidth(font_info,CacheButtonText);
  for (i=0; i < NumberPreferences; i++)
    if (WidgetTextWidth(font_info,(char *) Preferences[i]) > text_width)
      text_width=WidgetTextWidth(font_info,(char *) Preferences[i]);
  return(text_width);
}

static unsigned int GetPreferencesButtonWidth(XFontStruct *font_info)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,ApplyButtonText);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  width+=(unsigned int) QuantumMargin;
  return(width);
}

static void PositionPreferencesWidth(XWindows *windows,XFontStruct *font_info,
  unsigned int width,unsigned int text_width)
{
  windows->widget.width=(unsigned int) (MagickMax((int) (width << 1),
    (int) text_width)+6*QuantumMargin);
  windows->widget.min_width=(width << 1)+(unsigned int) QuantumMargin;
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
}

static void PositionPreferencesHeight(XWindows *windows,
  XFontStruct *font_info,unsigned int height)
{
  windows->widget.height=(unsigned int) (7*(int) height+NumberPreferences*
    ((int) height+(QuantumMargin >> 1)));
  windows->widget.min_height=(unsigned int) (7*(int) height+NumberPreferences*
    ((int) height+(QuantumMargin >> 1)));
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
}

static void SavePreferences(XResourceInfo *resource_info,
  const XWidgetInfo *preferences_info)
{
  /*
    Save user preferences to the client configuration file.
  */
  resource_info->backdrop=preferences_info[0].raised == MagickFalse;
  resource_info->confirm_exit=preferences_info[1].raised == MagickFalse;
  resource_info->confirm_edit=preferences_info[2].raised == MagickFalse;
  resource_info->gamma_correct=preferences_info[3].raised == MagickFalse;
  resource_info->display_warnings=preferences_info[4].raised == MagickFalse;
  resource_info->quantize_info->dither_method=
    preferences_info[5].raised == MagickFalse ?
    RiemersmaDitherMethod : NoDitherMethod;
  resource_info->colormap=SharedColormap;
  if (preferences_info[6].raised)
    resource_info->colormap=PrivateColormap;
  resource_info->use_pixmap=preferences_info[7].raised == MagickFalse;
  XUserPreferences(resource_info);
}

typedef struct _PreferencesWidgets
{
  XWidgetInfo
    *cache_info;

  char
    *cache;
  XWidgetInfo
    *preferences_info;
  XWidgetInfo
    *cancel_info;

  XWidgetInfo
    *apply_info;

  unsigned int
    *width;

  size_t
    *state;
  XEvent
    *event;
} PreferencesWidgets;

typedef struct _PreferencesContext
{
  XResourceInfo
    *resource_info;

  XFontStruct
    *font_info;
  XWindows
    *windows;

  unsigned int
    height;
  Display
    *display;
} PreferencesContext;

static void InitializePreferencesCache(const PreferencesContext *context,
  const PreferencesWidgets *widgets,int y)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const cache_info = widgets->cache_info;

  char
    *const cache = widgets->cache;

  (void) FormatLocaleString(cache,MagickPathExtent,CacheButtonText,
    (unsigned long) resource_info->undo_cache);
  XGetWidgetInfo(cache,cache_info);
  (*cache_info).bevel_width--;
  (*cache_info).width=(unsigned int) QuantumMargin >> 1;
  (*cache_info).height=(unsigned int) QuantumMargin >> 1;
  (*cache_info).x=QuantumMargin << 1;
  (*cache_info).y=y;
}

static void SetPreferencesRaised(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  XWidgetInfo
    *const preferences_info = widgets->preferences_info;

  preferences_info[0].raised=resource_info->backdrop == MagickFalse;
  preferences_info[1].raised=resource_info->confirm_exit == MagickFalse;
  preferences_info[2].raised=resource_info->confirm_edit == MagickFalse;
  preferences_info[3].raised=resource_info->gamma_correct ==
    MagickFalse;
  preferences_info[4].raised=resource_info->display_warnings ==
    MagickFalse;
  preferences_info[5].raised=
    resource_info->quantize_info->dither_method == NoDitherMethod;
  preferences_info[6].raised=resource_info->colormap != SharedColormap;
  preferences_info[7].raised=resource_info->use_pixmap == MagickFalse;
}

static void UpdatePreferencesButtons(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  XFontStruct
    *const font_info = context->font_info;

  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const preferences_info = widgets->preferences_info;

  unsigned int
    *const width = widgets->width;

  size_t
    *const state = widgets->state;

  int
    i,
    y;

  if ((*state) & UpdateConfigurationState)
    {
      /*
        Initialize button information.
      */
      XGetWidgetInfo(CancelButtonText,cancel_info);
      (*cancel_info).width=(*width);
      (*cancel_info).height=(unsigned int) (3*height) >> 1;
      (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
        (QuantumMargin << 1);
      (*cancel_info).y=(int) windows->widget.height-(int) (*cancel_info).height-
        QuantumMargin;
      XGetWidgetInfo(ApplyButtonText,apply_info);
      (*apply_info).width=(*width);
      (*apply_info).height=(unsigned int) (3*height) >> 1;
      (*apply_info).x=QuantumMargin << 1;
      (*apply_info).y=(*cancel_info).y;
      y=(int) (height << 1);
      for (i=0; i < NumberPreferences; i++)
      {
        XGetWidgetInfo(Preferences[i],&preferences_info[i]);
        preferences_info[i].bevel_width--;
        preferences_info[i].width=(unsigned int) QuantumMargin >> 1;
        preferences_info[i].height=(unsigned int) QuantumMargin >> 1;
        preferences_info[i].x=QuantumMargin << 1;
        preferences_info[i].y=y;
        y+=(int) height+(QuantumMargin >> 1);
      }
      SetPreferencesRaised(context,widgets);
      InitializePreferencesCache(context,widgets,y);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawPreferencesWidget(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const preferences_info = widgets->preferences_info;

  XWidgetInfo
    *const cache_info = widgets->cache_info;

  size_t
    *const state = widgets->state;

  int
    i;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Preferences widget.
      */
      XDrawBeveledButton(display,&windows->widget,apply_info);
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      for (i=0; i < NumberPreferences; i++)
        XDrawBeveledButton(display,&windows->widget,&preferences_info[i]);
      XDrawTriangleEast(display,&windows->widget,cache_info);
      XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void PressPreferencesButton(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XResourceInfo
    *const resource_info = context->resource_info;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const preferences_info = widgets->preferences_info;

  XWidgetInfo
    *const cache_info = widgets->cache_info;

  unsigned int
    *const width = widgets->width;

  char
    *const cache = widgets->cache;

  int
    x,
    y;

  if (MatteIsActive((*apply_info),event.xbutton))
    {
      /*
        User pressed Apply button.
      */
      (*apply_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,apply_info);
      return;
    }
  if (MatteIsActive((*cancel_info),event.xbutton))
    {
      /*
        User pressed Cancel button.
      */
      (*cancel_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
  TogglePreferencesButton(display,windows,event,preferences_info);
  if (MatteIsActive((*cache_info),event.xbutton))
    {
      /*
        User pressed Cache button.
      */
      x=(*cache_info).x+(int) (*cache_info).width+
        (int) (*cache_info).bevel_width+(QuantumMargin >> 1);
      y=(*cache_info).y+(int) (((*cache_info).height-height) >> 1);
      (*width)=WidgetTextWidth(font_info,cache);
      (void) XClearArea(display,windows->widget.id,x,y,(*width),height,
        False);
      DoublePreferencesUndoCache(resource_info);
      (void) FormatLocaleString(cache,MagickPathExtent,CacheButtonText,
        (unsigned long) resource_info->undo_cache);
      (*cache_info).raised=MagickFalse;
      XDrawTriangleEast(display,&windows->widget,cache_info);
      return;
    }
}

static void ReleasePreferencesButtons(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const cache_info = widgets->cache_info;

  size_t
    *const state = widgets->state;

  if (windows->widget.mapped == MagickFalse)
    return;
  if ((*apply_info).raised == MagickFalse)
    {
      (*state)=ReleasePreferencesButton(windows,event,(*apply_info),(*state));
      (*apply_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,apply_info);
      (*apply_info).raised=MagickFalse;
    }
  if ((*cancel_info).raised == MagickFalse)
    {
      (*state)=ReleasePreferencesButton(windows,event,(*cancel_info),(*state));
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
    }
  if ((*cache_info).raised == MagickFalse)
    {
      (*cache_info).raised=MagickTrue;
      XDrawTriangleEast(display,&windows->widget,cache_info);
    }
}

static void PressPreferencesKey(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  size_t
    *const state = widgets->state;

  static char
    command[MagickPathExtent];

  static KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  if (event.xkey.window != windows->widget.id)
    return;
  (void) XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      (*apply_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,apply_info);
      (*state)|=ExitState;
      return;
    }
}

static void MovePreferencesPointer(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  size_t
    *const state = widgets->state;

  XEvent
    *const event = widgets->event;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
  if ((*state) & InactiveWidgetState)
    return;
  if ((*apply_info).raised == MatteIsActive((*apply_info),(*event).xmotion))
    {
      /*
        Apply button status changed.
      */
      (*apply_info).raised=(*apply_info).raised == MagickFalse;
      XDrawBeveledButton(display,&windows->widget,apply_info);
      return;
    }
  if ((*cancel_info).raised == MatteIsActive((*cancel_info),(*event).xmotion))
    {
      /*
        Cancel button status changed.
      */
      (*cancel_info).raised=(*cancel_info).raised == MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
}

static void HandlePreferencesKeyEvent(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      PressPreferencesKey(context,event,widgets);
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      MovePreferencesPointer(context,widgets);
      break;
    }
    default:
      break;
  }
}

static void HandlePreferencesWindowEvent(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case ClientMessage:
    {
      (*state)=ClientWidgetMessage(display,windows,event,(*state));
      break;
    }
    case ConfigureNotify:
    {
      (*state)=ConfigureWidgetWindow(windows,event,(*state));
      break;
    }
    case EnterNotify:
    {
      (*state)=EnterWidgetWindow(windows,event,(*state));
      break;
    }
    case Expose:
    {
      (*state)=ExposeWidgetWindow(windows,event,(*state));
      break;
    }
    default:
    {
      HandlePreferencesKeyEvent(context,event,widgets);
      break;
    }
  }
}

static void RespondToPreferencesEvents(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  XEvent
    *const event = widgets->event;

  do
  {
    UpdatePreferencesButtons(context,widgets);
    RedrawPreferencesWidget(context,widgets);
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,event,XScreenEvent,(char *) windows);
    switch ((*event).type)
    {
      case ButtonPress:
      {
        PressPreferencesButton(context,(*event),widgets);
        break;
      }
      case ButtonRelease:
      {
        ReleasePreferencesButtons(context,(*event),widgets);
        break;
      }
      default:
      {
        HandlePreferencesWindowEvent(context,(*event),widgets);
        break;
      }
    }
  } while (((*state) & ExitState) == 0);
}

#endif
