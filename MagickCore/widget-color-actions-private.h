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

  MagickCore X11 methods of widget-color.c: the reply, button, key and scroll
  actions and the layout initialisers of XColorBrowserWidget, from
  CopyColorBrowserMode to InitializeColorBrowserScroll; included by that file
  only.
*/
#ifndef MAGICKCORE_WIDGET_COLOR_ACTIONS_PRIVATE_H
#define MAGICKCORE_WIDGET_COLOR_ACTIONS_PRIVATE_H

#if defined(MAGICKCORE_X11_DELEGATE)

static void CopyColorBrowserMode(const XWidgetInfo mode_info,
  const XWidgetInfo reply_info)
{
  if (mode_info.text != (char *) NULL)
    (void) CopyMagickString(reply_info.text,mode_info.text,
      MagickPathExtent);
}

static XWidgetInfo TrackColorBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const size_t colors)
{
  slider_info.y=event.xmotion.y-(int)
    ((slider_info.height+slider_info.bevel_width) >> 1)+1;
  if (slider_info.y < slider_info.min_y)
    slider_info.y=slider_info.min_y;
  if (slider_info.y > slider_info.max_y)
    slider_info.y=slider_info.max_y;
  slider_info.id=0;
  if (slider_info.y != slider_info.min_y)
    slider_info.id=(int) (((int) colors*(slider_info.y-
      slider_info.min_y+1))/(slider_info.max_y-slider_info.min_y+1));
  return(slider_info);
}

static size_t ReadColorBrowserPattern(const XWidgetInfo reply_info,
  char *glob_pattern,size_t state)
{
  /*
    Read new color or glob pattern.
  */
  if (*reply_info.text == '\0')
    return(state);
  (void) CopyMagickString(glob_pattern,reply_info.text,MagickPathExtent);
  state|=UpdateListState;
  return(state);
}

typedef struct _ColorBrowserGrab
{
  Display
    *display;

  XWindows
    *windows;

  XEvent
    event;

  XWidgetInfo
    grab_info;

  ExceptionInfo
    *exception;
} ColorBrowserGrab;

static void ReleaseColorBrowserGrab(const ColorBrowserGrab *grab,
  XWidgetInfo *reply_info,size_t *state)
{
  Display
    *const display = grab->display;

  XWindows
    *const windows = grab->windows;

  const XEvent
    event = grab->event;

  const XWidgetInfo
    grab_info = grab->grab_info;

  ExceptionInfo
    *const exception = grab->exception;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(grab_info,event.xbutton)))
    {
      /*
        Select a fill color from the X server.
      */
      (void) XGetWindowColor(display,windows,(*reply_info).text,
        exception);
      (*reply_info).marker=(*reply_info).text;
      (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
      XDrawMatteText(display,&windows->widget,&(*reply_info));
      (*state)|=RedrawActionState;
    }
}

typedef struct _ColorBrowserUpdate
{
  Display
    *display;

  XWindows
    *windows;

  char
    *reply,
    *glob_pattern;

  const char
    *reset_pattern;

  ExceptionInfo
    *exception;
} ColorBrowserUpdate;

typedef struct _ColorBrowserUpdateTargets
{
  XWidgetInfo
    *action_info;

  char
    ***colorlist;

  size_t
    *colors;
} ColorBrowserUpdateTargets;

static MagickBooleanType UpdateColorBrowserList(
  const ColorBrowserUpdate *update,const ColorBrowserUpdateTargets *targets)
{
  Display
    *const display = update->display;

  XWindows
    *const windows = update->windows;

  char
    *const reply = update->reply,
    *const glob_pattern = update->glob_pattern;

  const char
    *const reset_pattern = update->reset_pattern;

  ExceptionInfo
    *const exception = update->exception;

  XWidgetInfo
    *const action_info = targets->action_info;

  char
    ***const colorlist = targets->colorlist;

  size_t
    *const colors = targets->colors;

  char
    **checklist;

  int
    i;

  size_t
    number_colors;

  Status
    status;

  XColor
    color;

  status=XParseColor(display,windows->widget.map_info->colormap,
    glob_pattern,&color);
  if ((status != False) || (strchr(glob_pattern,'-') != (char *) NULL))
    {
      /*
        Reply is a single color name-- exit.
      */
      (void) CopyMagickString(reply,glob_pattern,MagickPathExtent);
      (void) CopyMagickString(glob_pattern,reset_pattern,MagickPathExtent);
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      return(MagickFalse);
    }
  /*
    Update color list.
  */
  checklist=GetColorList(glob_pattern,&number_colors,exception);
  if (number_colors == 0)
    {
      (void) CopyMagickString(glob_pattern,reset_pattern,MagickPathExtent);
      (void) XBell(display,0);
    }
  else
    {
      for (i=0; i < (int) (*colors); i++)
        (*colorlist)[i]=DestroyString((*colorlist)[i]);
      if ((*colorlist) != (char **) NULL)
        (*colorlist)=(char **) RelinquishMagickMemory((*colorlist));
      (*colorlist)=checklist;
      (*colors)=number_colors;
    }
  return(MagickTrue);
}

typedef struct _ColorBrowserTargets
{
  XWidgetInfo
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *grab_info,
    *reset_info,
    *action_info,
    *cancel_info,
    *slider_info;

  size_t
    *colors;

  XWidgetInfo
    *scroll_info;

  unsigned int
    *visible_colors;

  XWidgetInfo
    *north_info;

  size_t
    *delay;

  XWidgetInfo
    *south_info,
    *mode_info;

  char
    ***colorlist;

  XWidgetInfo
    *list_info,
    *expose_info,
    *text_info,
    *selection_info;

  MagickStatusType
    *mask;
} ColorBrowserTargets;

typedef struct _ColorBrowserContext
{
  Display
    *display;

  XWindows
    *windows;

  char
    *glob_pattern,
    *reset_pattern;

  ExceptionInfo
    *exception;

  XFontStruct
    *font_info;

  char
    *primary_selection;

  unsigned int
    height;

  const char
    *action;

  unsigned int
    width;

  char
    *reply,
    *text;
} ColorBrowserContext;

static void ClientColorBrowserMessage(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  /*
    If client window delete message, exit.
  */
  if (event.xclient.message_type != windows->wm_protocols)
    return;
  if (*event.xclient.data.l == (int) windows->wm_take_focus)
    {
      (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
        (Time) event.xclient.data.l[1]);
      return;
    }
  if (*event.xclient.data.l != (int) windows->wm_delete_window)
    return;
  if (event.xclient.window == windows->widget.id)
    {
      *(*reply_info).text='\0';
      (*state)|=ExitState;
      return;
    }
}

static void SelectColorBrowserReply(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  Status
    status;

  Atom
    type;

  int
    format;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  /*
    Obtain response from primary selection.
  */
  if (event.xselection.property == (Atom) None)
    return;
  status=XGetWindowProperty(display,event.xselection.requestor,
    event.xselection.property,0L,2047L,MagickTrue,XA_STRING,&type,
    &format,&length,&after,&data);
  if (IsUnusableSelection(status,type,format,length) != MagickFalse)
    return;
  if ((Extent((*reply_info).text)+(int) length) >= (MagickPathExtent-1))
    (void) XBell(display,0);
  else
    {
      /*
        Insert primary selection in reply text.
      */
      *(data+length)='\0';
      XEditText(display,reply_info,(KeySym) XK_Insert,(char *) data,
        (*state));
      XDrawMatteText(display,&windows->widget,reply_info);
      (*state)|=JumpListState;
      (*state)|=RedrawActionState;
    }
  (void) XFree((void *) data);
}

static void ToggleColorBrowserButtons(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  if ((*grab_info).raised == MatteIsActive((*grab_info),event.xmotion))
    {
      /*
        Grab button status changed.
      */
      (*grab_info).raised=!(*grab_info).raised;
      XDrawBeveledButton(display,&windows->widget,grab_info);
      return;
    }
  if ((*reset_info).raised == MatteIsActive((*reset_info),event.xmotion))
    {
      /*
        Reset button status changed.
      */
      (*reset_info).raised=!(*reset_info).raised;
      XDrawBeveledButton(display,&windows->widget,reset_info);
      return;
    }
  if ((*action_info).raised == MatteIsActive((*action_info),event.xmotion))
    {
      /*
        Action button status changed.
      */
      (*action_info).raised=(*action_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      return;
    }
  if ((*cancel_info).raised == MatteIsActive((*cancel_info),event.xmotion))
    {
      /*
        Cancel button status changed.
      */
      (*cancel_info).raised=(*cancel_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
}

static void MoveColorBrowserButtons(const ColorBrowserContext *context,
  XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*slider_info).active)
    {
      /*
        Move slider matte.
      */
      (*slider_info)=TrackColorBrowserSlider((*slider_info),event,(*colors));
      (*state)|=RedrawListState;
      return;
    }
  if ((*state) & InactiveWidgetState)
    return;
  ToggleColorBrowserButtons(context,event,widgets);
}

static void PressColorBrowserKey(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XColor
    color;

  Status
    status;

  static char
    command[MagickPathExtent];

  static int
    length;

  static KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  if (event.xkey.window != windows->widget.id)
    return;
  length=XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (AreaIsActive((*scroll_info),event.xkey))
    {
      /*
        Move slider.
      */
      (*slider_info)=MoveFontBrowserSlider((*slider_info),key_symbol,
        (*visible_colors),(int) (*colors));
      (*state)|=RedrawListState;
      return;
    }
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      (*state)=ReadColorBrowserPattern((*reply_info),glob_pattern,(*state));
      return;
    }
  if (key_symbol == XK_Control_L)
    {
      (*state)|=ControlState;
      return;
    }
  (*reply_info)=ApplyListBrowserControlKey((*reply_info),key_symbol,(*state));
  XEditText(display,reply_info,key_symbol,command,(*state));
  XDrawMatteText(display,&windows->widget,reply_info);
  (*state)|=JumpListState;
  status=XParseColor(display,windows->widget.map_info->colormap,
    (*reply_info).text,&color);
  if (status != False)
    (*state)|=RedrawActionState;
}

static void ReleaseColorBrowserButtons(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const reset_pattern = context->reset_pattern;

  ExceptionInfo
    *const exception = context->exception;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  ListBrowserRelease
    release;

  if ((*grab_info).raised == MagickFalse)
    {
      ColorBrowserGrab
        grab;

      grab.display=display;
      grab.windows=windows;
      grab.event=event;
      grab.grab_info=(*grab_info);
      grab.exception=exception;
      ReleaseColorBrowserGrab(&grab,reply_info,state);
      (*grab_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,grab_info);
    }
  if ((*reset_info).raised == MagickFalse)
    {
      FontBrowserBack
        back;

      back.windows=windows;
      back.event=event;
      back.back_info=(*reset_info);
      back.glob_pattern=glob_pattern;
      back.back_pattern=reset_pattern;
      (*state)=ReleaseFontBrowserBack(&back,(*state));
      (*reset_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,reset_info);
    }
  release.display=display;
  release.window_info=(&windows->widget);
  release.event=event;
  ReleaseListBrowserAction(&release,(*reply_info),action_info,state);
  release.display=display;
  release.window_info=(&windows->widget);
  release.event=event;
  ReleaseListBrowserCancel(&release,(*reply_info),cancel_info,state);
  if (MatteIsActive((*reply_info),event.xbutton) == MagickFalse)
    return;
}

static void ReleaseColorBrowserScroll(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const slider_info = widgets->slider_info,
    *const north_info = widgets->north_info;

  size_t
    *const delay = widgets->delay;

  XWidgetInfo
    *const south_info = widgets->south_info;

  if (windows->widget.mapped == MagickFalse)
    return;
  if ((*north_info).raised == MagickFalse)
    {
      /*
        User released up button.
      */
      (*delay)=SuspendTime << 2;
      (*north_info).raised=MagickTrue;
      XDrawTriangleNorth(display,&windows->widget,north_info);
    }
  if ((*south_info).raised == MagickFalse)
    {
      /*
        User released down button.
      */
      (*delay)=SuspendTime << 2;
      (*south_info).raised=MagickTrue;
      XDrawTriangleSouth(display,&windows->widget,south_info);
    }
  if ((*slider_info).active)
    {
      /*
        Stop tracking slider.
      */
      (*slider_info).active=MagickFalse;
      return;
    }
  ReleaseColorBrowserButtons(context,event,widgets);
}

static void PressColorBrowserReply(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  int
    x;

  int
    i;

  if (MatteIsActive((*action_info),event.xbutton))
    {
      /*
        User pressed action button.
      */
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
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
  if (MatteIsActive((*reply_info),event.xbutton) == MagickFalse)
    return;
  if (event.xbutton.button != Button2)
    {
      static Time
        click_time;

      ListBrowserClick
        click;

      /*
        Move text cursor to position of button press.
      */
      x=event.xbutton.x-(*reply_info).x-(int) (QuantumMargin >> 2);
      i=FindListBrowserCursor(font_info,(*reply_info),x);
      (*reply_info).cursor=(*reply_info).marker+i-1;
      click.display=display;
      click.window_info=(&windows->widget);
      click.event=event;
      click.click_time=click_time;
      (*reply_info)=HighlightListBrowserReply(&click,primary_selection,
        (*reply_info));
      XDrawMatteText(display,&windows->widget,reply_info);
      click_time=event.xbutton.time;
      return;
    }
  /*
    Request primary selection.
  */
  (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
    windows->widget.id,event.xbutton.time);
}

static void PressColorBrowserButtons(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const mode_info = widgets->mode_info;

  if (MatteIsActive((*grab_info),event.xbutton))
    {
      /*
        User pressed Grab button.
      */
      (*grab_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,grab_info);
      return;
    }
  if (MatteIsActive((*reset_info),event.xbutton))
    {
      /*
        User pressed Reset button.
      */
      (*reset_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,reset_info);
      return;
    }
  if (MatteIsActive((*mode_info),event.xbutton))
    {
      /*
        User pressed mode button.
      */
      CopyColorBrowserMode((*mode_info),(*reply_info));
      (void) CopyMagickString(primary_selection,(*reply_info).text,
        MagickPathExtent);
      (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
        event.xbutton.time);
      (*reply_info).highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
        windows->widget.id ? MagickTrue : MagickFalse;
      (*reply_info).marker=(*reply_info).text;
      (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
      XDrawMatteText(display,&windows->widget,reply_info);
      return;
    }
  PressColorBrowserReply(context,event,widgets);
}

static void SelectColorBrowserEntry(const ColorBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  char
    ***const colorlist = widgets->colorlist;

  FontBrowserPress
    press;

  press.display=display;
  press.windows=windows;
  press.fontlist=(*colorlist);
  press.fonts=(*colors);
  press.height=height;
  press.event=event;
  press.slider_info=(*slider_info);
  SelectFontBrowserEntry(&press,glob_pattern,targets);
}

static void PressColorBrowserScroll(const ColorBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const ColorBrowserTargets *widgets)
{
  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info;

  if (MatteIsActive((*slider_info),event.xbutton))
    {
      /*
        Track slider.
      */
      (*slider_info).active=MagickTrue;
      return;
    }
  if ((MatteIsActive((*north_info),event.xbutton)) && ((*slider_info).id > 0))
    {
      /*
        Move slider up.
      */
      (*north_info).raised=MagickFalse;
      (*slider_info).id--;
      (*state)|=RedrawListState;
      return;
    }
  if ((MatteIsActive((*south_info),event.xbutton)) &&
      ((*slider_info).id < (int) (*colors)))
    {
      /*
        Move slider down.
      */
      (*south_info).raised=MagickFalse;
      (*slider_info).id++;
      (*state)|=RedrawListState;
      return;
    }
  if (MatteIsActive((*scroll_info),event.xbutton))
    {
      /*
        Move slider.
      */
      (*slider_info)=PageListBrowserSlider((*slider_info),event,
        (*visible_colors));
      (*state)|=RedrawListState;
      return;
    }
  if (MatteIsActive((*list_info),event.xbutton))
    {
      SelectColorBrowserEntry(context,event,targets,
        widgets);
      return;
    }
  PressColorBrowserButtons(context,event,widgets);
}

#define CancelButtonText  "Cancel"
#define ColornameText  "Name:"
#define ColorPatternText  "Pattern:"
#define GrabButtonText  "Grab"
#define ResetButtonText  "Reset"

static void InitializeColorBrowserButtons(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  const char
    *const action = context->action;

  const unsigned int
    width = context->width;

  XWidgetInfo
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  /*
    Initialize button information.
  */
  XGetWidgetInfo(CancelButtonText,cancel_info);
  (*cancel_info).width=width;
  (*cancel_info).height=(unsigned int) ((3*height) >> 1);
  (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
    QuantumMargin-2;
  (*cancel_info).y=(int) windows->widget.height-(int) (*cancel_info).height-
    QuantumMargin;
  XGetWidgetInfo(action,action_info);
  (*action_info).width=width;
  (*action_info).height=(unsigned int) ((3*height) >> 1);
  (*action_info).x=(int) windows->widget.width-(int) (*action_info).width-
    (int) (*cancel_info).width-2*QuantumMargin-2;
  (*action_info).y=(*cancel_info).y;
  XGetWidgetInfo(GrabButtonText,grab_info);
  (*grab_info).width=width;
  (*grab_info).height=(unsigned int) ((3*height) >> 1);
  (*grab_info).x=QuantumMargin;
  (*grab_info).y=((5*QuantumMargin) >> 1)+(int) height;
  XGetWidgetInfo(ResetButtonText,reset_info);
  (*reset_info).width=width;
  (*reset_info).height=(unsigned int) ((3*height) >> 1);
  (*reset_info).x=QuantumMargin;
  (*reset_info).y=(*grab_info).y+(int) (*grab_info).height+QuantumMargin;
}

static void InitializeColorBrowserReply(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height,
    width = context->width;

  char
    *const reply = context->reply;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const action_info = widgets->action_info,
    *const mode_info = widgets->mode_info;

  /*
    Initialize reply information.
  */
  XGetWidgetInfo(reply,reply_info);
  (*reply_info).raised=MagickFalse;
  (*reply_info).bevel_width--;
  (*reply_info).width=windows->widget.width-width-(unsigned int)
    ((6*QuantumMargin) >> 1);
  (*reply_info).height=height << 1;
  (*reply_info).x=(int) width+(QuantumMargin << 1);
  (*reply_info).y=(*action_info).y-(int) (*reply_info).height-QuantumMargin;
  /*
    Initialize mode information.
  */
  XGetWidgetInfo((char *) NULL,mode_info);
  (*mode_info).active=MagickTrue;
  (*mode_info).bevel_width=0;
  (*mode_info).width=(unsigned int) ((*action_info).x-
    (int) (QuantumMargin << 1));
  (*mode_info).height=(*action_info).height;
  (*mode_info).x=QuantumMargin;
  (*mode_info).y=(*action_info).y;
}

static void InitializeColorBrowserScroll(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const grab_info = widgets->grab_info,
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const expose_info = widgets->expose_info;

  int
    id;

  /*
    Initialize scroll information.
  */
  XGetWidgetInfo((char *) NULL,scroll_info);
  (*scroll_info).bevel_width--;
  (*scroll_info).width=height;
  (*scroll_info).height=(unsigned int) ((*reply_info).y-(*grab_info).y-
    (QuantumMargin >> 1));
  (*scroll_info).x=(*reply_info).x+(int) ((*reply_info).width-
    (*scroll_info).width);
  (*scroll_info).y=(*grab_info).y-(int) (*reply_info).bevel_width;
  (*scroll_info).raised=MagickFalse;
  (*scroll_info).trough=MagickTrue;
  (*north_info)=(*scroll_info);
  (*north_info).raised=MagickTrue;
  (*north_info).width-=((*north_info).bevel_width << 1);
  (*north_info).height=(*north_info).width-1;
  (*north_info).x+=(int) (*north_info).bevel_width;
  (*north_info).y+=(int) (*north_info).bevel_width;
  (*south_info)=(*north_info);
  (*south_info).y=(*scroll_info).y+(int) (*scroll_info).height-(int)
    (*scroll_info).bevel_width-(int) (*south_info).height;
  id=(*slider_info).id;
  (*slider_info)=(*north_info);
  (*slider_info).id=id;
  (*slider_info).width-=2;
  (*slider_info).min_y=(*north_info).y+(int) (*north_info).height+(int)
    (*north_info).bevel_width+(int) (*slider_info).bevel_width+2;
  (*slider_info).height=(unsigned int) ((int) (*scroll_info).height-
    (((*slider_info).min_y-(*scroll_info).y+1) << 1)+4);
  (*visible_colors)=(unsigned int) ((*scroll_info).height*
    MagickSafeReciprocal((double) height+(height >> 3)));
  if ((*colors) > (*visible_colors))
    (*slider_info).height=(unsigned int) (((*visible_colors)*
      (*slider_info).height)/(*colors));
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-
    (int) (*slider_info).bevel_width-2;
  (*slider_info).x=(*scroll_info).x+(int) (*slider_info).bevel_width+1;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info)=(*scroll_info);
  (*expose_info).y=(*slider_info).y;
}

#endif

#endif
