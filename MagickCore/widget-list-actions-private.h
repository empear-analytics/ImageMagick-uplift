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

  MagickCore X11 methods of widget-list.c: the entry, slider, key, pointer,
  reply and scroll actions, the event handlers and the layout initialisers of
  XListBrowserWidget, from CountListBrowserEntries to UpdateListBrowserLayout;
  included by that file only.
*/
#ifndef MAGICKCORE_WIDGET_LIST_ACTIONS_PRIVATE_H
#define MAGICKCORE_WIDGET_LIST_ACTIONS_PRIVATE_H

#if defined(MAGICKCORE_X11_DELEGATE)

static unsigned int CountListBrowserEntries(const char *const *list)
{
  unsigned int
    entries;

  for (entries=0; ; entries++)
    if (list[entries] == (char *) NULL)
      break;
  return(entries);
}

static unsigned int WidestListBrowserEntry(XFontStruct *font_info,
  const char *const *list,const unsigned int entries,unsigned int text_width)
{
  int
    i;

  for (i=0; i < (int) entries; i++)
    if (WidgetTextWidth(font_info,(char *) list[i]) > text_width)
      text_width=WidgetTextWidth(font_info,(char *) list[i]);
  return(text_width);
}

typedef struct _ListBrowserReplyMatch
{
  const char
    *const *list;

  char
    *reply;

  unsigned int
    entries,
    visible_entries;
} ListBrowserReplyMatch;

static void MatchListBrowserReply(const ListBrowserReplyMatch *match,
  XWidgetInfo *list_info,XWidgetInfo *slider_info)
{
  const char
    *const *const list = match->list;

  char
    *const reply = match->reply;

  const unsigned int
    entries = match->entries,
    visible_entries = match->visible_entries;

  int
    i;

  for (i=0; i < (int) entries; i++)
    if (LocaleCompare(list[i],reply) == 0)
      {
        (*list_info).id=i;
        (*slider_info).id=i-(int) (visible_entries >> 1);
        if ((*slider_info).id < 0)
          (*slider_info).id=0;
      }
}

typedef struct _ListBrowserRedraw
{
  Display
    *display;

  XWindowInfo
    *window_info;

  const char
    *const *list;

  unsigned int
    entries,
    height,
    visible_entries;

  const XWidgetInfo
    *list_info;

  XWidgetInfo
    north_info,
    slider_info,
    south_info;
} ListBrowserRedraw;

static void RedrawListBrowserList(const ListBrowserRedraw *redraw,
  XWidgetInfo *selection_info,XWidgetInfo *expose_info)
{
  Display
    *const display = redraw->display;

  XWindowInfo
    *const window_info = redraw->window_info;

  const char
    *const *const list = redraw->list;

  const unsigned int
    entries = redraw->entries,
    height = redraw->height,
    visible_entries = redraw->visible_entries;

  const XWidgetInfo
    *const list_info = redraw->list_info;

  const XWidgetInfo
    north_info = redraw->north_info,
    slider_info = redraw->slider_info,
    south_info = redraw->south_info;

  int
    i;

  /*
    Redraw scroll bar and file names.
  */
  (*selection_info).id=slider_info.id;
  (*selection_info).y=(*list_info).y+(int) (height >> 3)+2;
  for (i=0; i < (int) visible_entries; i++)
  {
    (*selection_info).raised=(slider_info.id+i) != (*list_info).id ?
      MagickTrue : MagickFalse;
    (*selection_info).text=(char *) NULL;
    if ((slider_info.id+i) < (int) entries)
      (*selection_info).text=(char *) list[slider_info.id+i];
    XDrawWidgetText(display,window_info,&(*selection_info));
    (*selection_info).y+=(int) (*selection_info).height;
  }
  /*
    Update slider.
  */
  if (slider_info.y > (*expose_info).y)
    {
      (*expose_info).height=(unsigned int) (slider_info.y-(*expose_info).y);
      (*expose_info).y=slider_info.y-(int) (*expose_info).height-(int)
        slider_info.bevel_width-1;
    }
  else
    {
      (*expose_info).height=(unsigned int) ((*expose_info).y-slider_info.y);
      (*expose_info).y=slider_info.y+(int) slider_info.height+(int)
        slider_info.bevel_width+1;
    }
  XDrawTriangleNorth(display,window_info,&north_info);
  XDrawMatte(display,window_info,&(*expose_info));
  XDrawBeveledButton(display,window_info,&slider_info);
  XDrawTriangleSouth(display,window_info,&south_info);
  (*expose_info).y=slider_info.y;
}

typedef struct _ListBrowserTargets
{
  XWidgetInfo
    *reply_info,
    *selection_info,
    *action_info;

  size_t
    *state;

  XWidgetInfo
    *list_info;
} ListBrowserTargets;

typedef struct _ListBrowserPress
{
  Display
    *display;

  XWindowInfo
    *window_info;

  const char
    *const *list;

  unsigned int
    entries,
    height;

  XEvent
    event;

  XWidgetInfo
    slider_info;
} ListBrowserPress;

static void SelectListBrowserEntry(const ListBrowserPress *press,
  const ListBrowserTargets *targets)
{
  Display
    *const display = press->display;

  XWindowInfo
    *const window_info = press->window_info;

  const char
    *const *const list = press->list;

  const unsigned int
    entries = press->entries,
    height = press->height;

  const XEvent
    event = press->event;

  const XWidgetInfo
    slider_info = press->slider_info;

  XWidgetInfo
    *const reply_info = targets->reply_info,
    *const selection_info = targets->selection_info,
    *const action_info = targets->action_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const list_info = targets->list_info;

  int
    id;

  /*
    User pressed list matte.
  */
  id=slider_info.id+(event.xbutton.y-((*list_info).y+(int)
    (height >> 1))+1)/(int) (*selection_info).height;
  if (id >= (int) entries)
    return;
  (void) CopyMagickString((*reply_info).text,list[id],MagickPathExtent);
  (*reply_info).highlight=MagickFalse;
  (*reply_info).marker=(*reply_info).text;
  (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
  XDrawMatteText(display,window_info,reply_info);
  (*selection_info).id=(~0);
  if (id == (*list_info).id)
    {
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,window_info,action_info);
      (*state)|=ExitState;
    }
  (*list_info).id=id;
  (*state)|=RedrawListState;
}

static void MoveListBrowserSliderByPage(const KeySym key_symbol,
  const unsigned int visible_entries,const unsigned int entries,
  XWidgetInfo *slider_info)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_KP_Prior:
    {
      (*slider_info).id-=(int) visible_entries;
      break;
    }
    case XK_Next:
    case XK_KP_Next:
    {
      (*slider_info).id+=(int) visible_entries;
      break;
    }
    case XK_End:
    case XK_KP_End:
    {
      (*slider_info).id=(int) entries;
      break;
    }
    default:
      break;
  }
}

static XWidgetInfo MoveListBrowserSlider(XWidgetInfo slider_info,
  const KeySym key_symbol,const unsigned int visible_entries,
  const unsigned int entries)
{
  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      slider_info.id=0;
      break;
    }
    case XK_Up:
    case XK_KP_Up:
    {
      slider_info.id--;
      break;
    }
    case XK_Down:
    case XK_KP_Down:
    {
      slider_info.id++;
      break;
    }
    default:
    {
      MoveListBrowserSliderByPage(key_symbol,visible_entries,entries,
        &slider_info);
      break;
    }
  }
  return(slider_info);
}

static void ReadListBrowserEntry(Display *display,XWindowInfo *window_info,
  const XWidgetInfo reply_info,const ListBrowserTargets *targets)
{
  XWidgetInfo
    *const action_info = targets->action_info;

  size_t
    *const state = targets->state;

  /*
    Read new entry.
  */
  if (*reply_info.text == '\0')
    return;
  (*action_info).raised=MagickFalse;
  XDrawBeveledButton(display,window_info,action_info);
  (*state)|=ExitState;
}

#define CancelButtonText  "Cancel"

typedef struct _ListBrowserWidgets
{
  XWidgetInfo
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *slider_info;

  XWidgetInfo
    *action_info;

  XWidgetInfo
    *cancel_info;

  XWidgetInfo
    *north_info;

  XWidgetInfo
    *south_info;

  size_t
    *delay;

  XWidgetInfo
    *scroll_info;

  unsigned int
    *visible_entries;

  XEvent
    *event;

  XWidgetInfo
    *list_info;

  MagickStatusType
    *mask;

  XWidgetInfo
    *expose_info;

  XWidgetInfo
    *text_info;

  XWidgetInfo
    *selection_info;
} ListBrowserWidgets;

typedef struct _ListBrowserContext
{
  Display
    *display;

  XWindows
    *windows;

  XWindowInfo
    *window_info;

  unsigned int
    entries;

  char
    *primary_selection;

  XFontStruct
    *font_info;

  const char
    *const *list;

  unsigned int
    height;

  const char
    *action;

  unsigned int
    width;

  char
    *reply;

  const char
    *query;
} ListBrowserContext;

static void ClientListBrowserMessage(const ListBrowserContext *context,
  const XEvent event,const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWindowInfo
    *const window_info = context->window_info;

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
  if (event.xclient.window == window_info->id)
    {
      *(*reply_info).text='\0';
      (*state)|=ExitState;
      return;
    }
}

static void ReleaseListBrowserScroll(const ListBrowserContext *context,
  const XEvent event,const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindowInfo
    *const window_info = context->window_info;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const action_info = widgets->action_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  size_t
    *const delay = widgets->delay;

  ListBrowserRelease
    release;

  if (window_info->mapped == MagickFalse)
    return;
  if ((*north_info).raised == MagickFalse)
    {
      /*
        User released up button.
      */
      (*delay)=SuspendTime << 2;
      (*north_info).raised=MagickTrue;
      XDrawTriangleNorth(display,window_info,north_info);
    }
  if ((*south_info).raised == MagickFalse)
    {
      /*
        User released down button.
      */
      (*delay)=SuspendTime << 2;
      (*south_info).raised=MagickTrue;
      XDrawTriangleSouth(display,window_info,south_info);
    }
  if ((*slider_info).active)
    {
      /*
        Stop tracking slider.
      */
      (*slider_info).active=MagickFalse;
      return;
    }
  release.display=display;
  release.window_info=window_info;
  release.event=event;
  ReleaseListBrowserAction(&release,(*reply_info),action_info,state);
  release.display=display;
  release.window_info=window_info;
  release.event=event;
  ReleaseListBrowserCancel(&release,(*reply_info),cancel_info,state);
  if (MatteIsActive((*reply_info),event.xbutton) == MagickFalse)
    return;
}

static void PressListBrowserKey(const ListBrowserContext *context,
  const XEvent event,const ListBrowserTargets *targets,
  const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindowInfo
    *const window_info = context->window_info;

  const unsigned int
    entries = context->entries;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_entries = widgets->visible_entries;

  static char
    command[MagickPathExtent];

  static int
    length;

  static KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  if (event.xkey.window != window_info->id)
    return;
  length=XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (AreaIsActive((*scroll_info),event.xkey))
    {
      /*
        Move slider.
      */
      (*slider_info)=MoveListBrowserSlider((*slider_info),key_symbol,
        (*visible_entries),entries);
      (*state)|=RedrawListState;
      return;
    }
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      ReadListBrowserEntry(display,window_info,(*reply_info),targets);
      return;
    }
  if (key_symbol == XK_Control_L)
    {
      (*state)|=ControlState;
      return;
    }
  (*reply_info)=ApplyListBrowserControlKey((*reply_info),key_symbol,(*state));
  XEditText(display,reply_info,key_symbol,command,(*state));
  XDrawMatteText(display,window_info,reply_info);
}

static void MoveListBrowserPointer(const ListBrowserContext *context,
  const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindowInfo
    *const window_info = context->window_info;

  const unsigned int
    entries = context->entries;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const action_info = widgets->action_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XEvent
    *const event = widgets->event;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
  if ((*slider_info).active)
    {
      /*
        Move slider matte.
      */
      (*slider_info)=TrackListBrowserSlider((*slider_info),(*event),entries);
      (*state)|=RedrawListState;
      return;
    }
  if ((*state) & InactiveWidgetState)
    return;
  if ((*action_info).raised == MatteIsActive((*action_info),(*event).xmotion))
    {
      /*
        Action button status changed.
      */
      (*action_info).raised=(*action_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,window_info,action_info);
      return;
    }
  if ((*cancel_info).raised == MatteIsActive((*cancel_info),(*event).xmotion))
    {
      /*
        Cancel button status changed.
      */
      (*cancel_info).raised=(*cancel_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,window_info,cancel_info);
      return;
    }
}

static void InsertListBrowserSelection(const ListBrowserContext *context,
  const XEvent event,const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindowInfo
    *const window_info = context->window_info;

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
  status=XGetWindowProperty(display,
    event.xselection.requestor,event.xselection.property,0L,2047L,
    MagickTrue,XA_STRING,&type,&format,&length,&after,&data);
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
      XDrawMatteText(display,window_info,reply_info);
      (*state)|=RedrawActionState;
    }
  (void) XFree((void *) data);
}

static void ClickListBrowserReply(const ListBrowserContext *context,
  const XEvent event,const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindowInfo
    *const window_info = context->window_info;

  char
    *const primary_selection = context->primary_selection;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  int
    x;

  int
    i;

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
      click.window_info=window_info;
      click.event=event;
      click.click_time=click_time;
      (*reply_info)=HighlightListBrowserReply(&click,primary_selection,
        (*reply_info));
      XDrawMatteText(display,window_info,reply_info);
      click_time=event.xbutton.time;
      return;
    }
  /*
    Request primary selection.
  */
  (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
    window_info->id,event.xbutton.time);
}

static void PressListBrowserButtons(const ListBrowserContext *context,
  const XEvent event,const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindowInfo
    *const window_info = context->window_info;

  XWidgetInfo
    *const action_info = widgets->action_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  if (MatteIsActive((*action_info),event.xbutton))
    {
      /*
        User pressed action button.
      */
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,window_info,action_info);
      return;
    }
  if (MatteIsActive((*cancel_info),event.xbutton))
    {
      /*
        User pressed Cancel button.
      */
      (*cancel_info).raised=MagickFalse;
      XDrawBeveledButton(display,window_info,cancel_info);
      return;
    }
  ClickListBrowserReply(context,event,widgets);
}

static void PressListBrowserList(const ListBrowserContext *context,
  const XEvent event,const ListBrowserTargets *targets,
  const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindowInfo
    *const window_info = context->window_info;

  const unsigned int
    entries = context->entries;

  const char
    *const *const list = context->list;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const list_info = widgets->list_info;

  if (MatteIsActive((*list_info),event.xbutton))
    {
      ListBrowserPress
        press;

      press.display=display;
      press.window_info=window_info;
      press.list=list;
      press.entries=entries;
      press.height=height;
      press.event=event;
      press.slider_info=(*slider_info);
      SelectListBrowserEntry(&press,targets);
      return;
    }
  PressListBrowserButtons(context,event,widgets);
}

static void PressListBrowserScroll(const ListBrowserContext *context,
  const XEvent event,const ListBrowserTargets *targets,
  const ListBrowserWidgets *widgets)
{
  const unsigned int
    entries = context->entries;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_entries = widgets->visible_entries;

  if ((MatteIsActive((*south_info),event.xbutton)) &&
      ((*slider_info).id < (int) entries))
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
        (*visible_entries));
      (*state)|=RedrawListState;
      return;
    }
  PressListBrowserList(context,event,targets,widgets);
}

static void PressListBrowserSlider(const ListBrowserContext *context,
  const XEvent event,const ListBrowserTargets *targets,
  const ListBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const north_info = widgets->north_info;

  if (MatteIsActive((*slider_info),event.xbutton))
    {
      /*
        Track slider.
      */
      (*slider_info).active=MagickTrue;
      return;
    }
  if ((MatteIsActive((*north_info),event.xbutton)) &&
      ((*slider_info).id > 0))
    {
      /*
        Move slider up.
      */
      (*north_info).raised=MagickFalse;
      (*slider_info).id--;
      (*state)|=RedrawListState;
      return;
    }
  PressListBrowserScroll(context,event,targets,widgets);
}

static void ConfigureListBrowserWindow(const ListBrowserContext *context,
  const XEvent event,const ListBrowserWidgets *widgets)
{
  XWindowInfo
    *const window_info = context->window_info;

  size_t
    *const state = widgets->state;

  /*
    Update widget configuration.
  */
  if (event.xconfigure.window != window_info->id)
    return;
  if ((event.xconfigure.width == (int) window_info->width) &&
      (event.xconfigure.height == (int) window_info->height))
    return;
  window_info->width=(unsigned int)
    MagickMax(event.xconfigure.width,(int) window_info->min_width);
  window_info->height=(unsigned int)
    MagickMax(event.xconfigure.height,(int) window_info->min_height);
  (*state)|=UpdateConfigurationState;
}

static void ExposeListBrowserWindow(const ListBrowserContext *context,
  const XEvent event,const ListBrowserWidgets *widgets)
{
  XWindowInfo
    *const window_info = context->window_info;

  size_t
    *const state = widgets->state;

  if (event.xexpose.window != window_info->id)
    return;
  if (event.xexpose.count != 0)
    return;
  (*state)|=RedrawWidgetState;
}

static void ReleaseListBrowserKey(const ListBrowserContext *context,
  const XEvent event,const ListBrowserWidgets *widgets)
{
  XWindowInfo
    *const window_info = context->window_info;

  size_t
    *const state = widgets->state;

  static char
    command[MagickPathExtent];

  static KeySym
    key_symbol;

  /*
    Respond to a user key release.
  */
  if (event.xkey.window != window_info->id)
    return;
  (void) XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if (key_symbol == XK_Control_L)
    (*state)&=(unsigned int) (~ControlState);
}

static void HandleListBrowserSelectionEvent(const ListBrowserContext *context,
  XEvent event,const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindowInfo
    *const window_info = context->window_info;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  switch (event.type)
  {
    case MotionNotify:
    {
      MoveListBrowserPointer(context,widgets);
      break;
    }
    case SelectionClear:
    {
      (*reply_info).highlight=MagickFalse;
      XDrawMatteText(display,window_info,reply_info);
      break;
    }
    case SelectionNotify:
    {
      InsertListBrowserSelection(context,event,widgets);
      break;
    }
    case SelectionRequest:
    {
      XSelectionEvent
        notify;

      XSelectionRequestEvent
        *request;

      if ((*reply_info).highlight == MagickFalse)
        break;
      /*
        Set primary selection.
      */
      request=(&(event.xselectionrequest));
      (void) XChangeProperty(request->display,request->requestor,
        request->property,request->target,8,PropModeReplace,
        (unsigned char *) primary_selection,Extent(primary_selection));
      notify.type=SelectionNotify;
      notify.send_event=MagickTrue;
      notify.display=request->display;
      notify.requestor=request->requestor;
      notify.selection=request->selection;
      notify.target=request->target;
      notify.time=request->time;
      if (request->property == None)
        notify.property=request->target;
      else
        notify.property=request->property;
      (void) XSendEvent(request->display,request->requestor,False,NoEventMask,
        (XEvent *) &notify);
    }
    default:
      break;
  }
}

static void HandleListBrowserKeyEvent(const ListBrowserContext *context,
  const XEvent event,const ListBrowserTargets *targets,
  const ListBrowserWidgets *widgets)
{
  XWindowInfo
    *const window_info = context->window_info;

  size_t
    *const state = widgets->state;

  MagickStatusType
    *const mask = widgets->mask;

  switch (event.type)
  {
    case KeyPress:
    {
      PressListBrowserKey(context,event,targets,widgets);
      break;
    }
    case KeyRelease:
    {
      ReleaseListBrowserKey(context,event,widgets);
      break;
    }
    case LeaveNotify:
    {
      if (event.xcrossing.window != window_info->id)
        break;
      (*state)|=InactiveWidgetState;
      break;
    }
    case MapNotify:
    {
      (*mask)&=(unsigned int) (~CWX);
      (*mask)&=(unsigned int) (~CWY);
      break;
    }
    default:
    {
      HandleListBrowserSelectionEvent(context,event,widgets);
      break;
    }
  }
}

static void HandleListBrowserWindowEvent(const ListBrowserContext *context,
  const XEvent event,const ListBrowserTargets *targets,
  const ListBrowserWidgets *widgets)
{
  XWindowInfo
    *const window_info = context->window_info;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case ButtonRelease:
    {
      ReleaseListBrowserScroll(context,event,widgets);
      break;
    }
    case ClientMessage:
    {
      ClientListBrowserMessage(context,event,widgets);
      break;
    }
    case ConfigureNotify:
    {
      ConfigureListBrowserWindow(context,event,widgets);
      break;
    }
    case EnterNotify:
    {
      if (event.xcrossing.window != window_info->id)
        break;
      (*state)&=(unsigned int) (~InactiveWidgetState);
      break;
    }
    case Expose:
    {
      ExposeListBrowserWindow(context,event,widgets);
      break;
    }
    default:
    {
      HandleListBrowserKeyEvent(context,event,targets,widgets);
      break;
    }
  }
}

static void InitializeListBrowserButtons(const ListBrowserContext *context,
  const ListBrowserWidgets *widgets)
{
  XWindowInfo
    *const window_info = context->window_info;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  const char
    *const action = context->action;

  const unsigned int
    width = context->width;

  XWidgetInfo
    *const action_info = widgets->action_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  /*
    Initialize button information.
  */
  XGetWidgetInfo(CancelButtonText,cancel_info);
  (*cancel_info).width=width;
  (*cancel_info).height=(unsigned int) ((3*height) >> 1);
  (*cancel_info).x=(int) window_info->width-(int) (*cancel_info).width-
    QuantumMargin-2;
  (*cancel_info).y=(int) window_info->height-(int) (*cancel_info).height-
    QuantumMargin;
  XGetWidgetInfo(action,action_info);
  (*action_info).width=width;
  (*action_info).height=(unsigned int) ((3*height) >> 1);
  (*action_info).x=(*cancel_info).x-((int) (*cancel_info).width+
    (QuantumMargin >> 1)+(int) ((*action_info).bevel_width << 1));
  (*action_info).y=(*cancel_info).y;
}

static void InitializeListBrowserReply(const ListBrowserContext *context,
  const ListBrowserWidgets *widgets)
{
  XWindowInfo
    *const window_info = context->window_info;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  char
    *const reply = context->reply;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  XWidgetInfo
    *const action_info = widgets->action_info;

  /*
    Initialize reply information.
  */
  XGetWidgetInfo(reply,reply_info);
  (*reply_info).raised=MagickFalse;
  (*reply_info).bevel_width--;
  (*reply_info).width=(unsigned int) ((int) window_info->width-
    (((4*QuantumMargin) >> 1)));
  (*reply_info).height=height << 1;
  (*reply_info).x=QuantumMargin;
  (*reply_info).y=(*action_info).y-(int) (*reply_info).height-QuantumMargin;
}

static void InitializeListBrowserScroll(const ListBrowserContext *context,
  const ListBrowserWidgets *widgets)
{
  const unsigned int
    entries = context->entries;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_entries = widgets->visible_entries;

  XWidgetInfo
    *const expose_info = widgets->expose_info;

  int
    id;

  /*
    Initialize scroll information.
  */
  XGetWidgetInfo((char *) NULL,scroll_info);
  (*scroll_info).bevel_width--;
  (*scroll_info).width=height;
  (*scroll_info).height=(unsigned int)
    ((*reply_info).y-((6*QuantumMargin) >> 1)-(int) height);
  (*scroll_info).x=(*reply_info).x+(int) ((*reply_info).width-
    (*scroll_info).width);
  (*scroll_info).y=((5*QuantumMargin) >> 1)+(int) height-(int)
    (*reply_info).bevel_width;
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
  (*visible_entries)=(unsigned int) ((*scroll_info).height*
    MagickSafeReciprocal((double) height+(height >> 3)));
  if (entries > (*visible_entries))
    (*slider_info).height=((*visible_entries)*(*slider_info).height)/entries;
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-(int)
    (*slider_info).bevel_width-2;
  (*slider_info).x=(*scroll_info).x+(int) (*slider_info).bevel_width+1;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info)=(*scroll_info);
  (*expose_info).y=(*slider_info).y;
}

static void InitializeListBrowserList(const ListBrowserContext *context,
  const ListBrowserWidgets *widgets)
{
  XWindowInfo
    *const window_info = context->window_info;

  const unsigned int
    entries = context->entries;

  XFontStruct
    *const font_info = context->font_info;

  const char
    *const *const list = context->list;

  char
    *const reply = context->reply;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_entries = widgets->visible_entries;

  XWidgetInfo
    *const list_info = widgets->list_info;

  /*
    Initialize list information.
  */
  XGetWidgetInfo((char *) NULL,list_info);
  (*list_info).raised=MagickFalse;
  (*list_info).bevel_width--;
  (*list_info).width=(unsigned int)
    ((*scroll_info).x-(*reply_info).x-(int) (QuantumMargin >> 1));
  (*list_info).height=(*scroll_info).height;
  (*list_info).x=(*reply_info).x;
  (*list_info).y=(*scroll_info).y;
  if (window_info->mapped == MagickFalse)
    {
      ListBrowserReplyMatch
        match;

      match.list=list;
      match.reply=reply;
      match.entries=entries;
      match.visible_entries=(*visible_entries);
      MatchListBrowserReply(&match,list_info,slider_info);
    }
}

static void InitializeListBrowserText(const ListBrowserContext *context,
  const ListBrowserWidgets *widgets)
{
  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  const char
    *const query = context->query;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const list_info = widgets->list_info;

  XWidgetInfo
    *const text_info = widgets->text_info;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  /*
    Initialize text information.
  */
  XGetWidgetInfo(query,text_info);
  (*text_info).width=(*reply_info).width;
  (*text_info).height=height;
  (*text_info).x=(*list_info).x-(int) (QuantumMargin >> 1);
  (*text_info).y=QuantumMargin;
  /*
    Initialize selection information.
  */
  XGetWidgetInfo((char *) NULL,selection_info);
  (*selection_info).center=MagickFalse;
  (*selection_info).width=(*list_info).width;
  (*selection_info).height=(unsigned int) ((9*height) >> 3);
  (*selection_info).x=(*list_info).x;
  (*state)&=(unsigned int) (~UpdateConfigurationState);
}

static void UpdateListBrowserLayout(const ListBrowserContext *context,
  const ListBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  if ((*state) & UpdateConfigurationState)
    {
      InitializeListBrowserButtons(context,widgets);
      InitializeListBrowserReply(context,widgets);
      InitializeListBrowserScroll(context,widgets);
      InitializeListBrowserList(context,widgets);
      InitializeListBrowserText(context,widgets);
    }
}

#endif

#endif
