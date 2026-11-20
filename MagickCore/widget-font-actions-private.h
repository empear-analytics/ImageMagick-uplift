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

  MagickCore X11 methods of widget-font.c: the font comparison, the reply,
  button, key and scroll actions and the first layout initialisers of
  XFontBrowserWidget, from IsSameFontCharacter to InitializeFontBrowserReply;
  included by that file only.
*/
#ifndef MAGICKCORE_WIDGET_FONT_ACTIONS_PRIVATE_H
#define MAGICKCORE_WIDGET_FONT_ACTIONS_PRIVATE_H

#if defined(MAGICKCORE_X11_DELEGATE)

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline MagickBooleanType IsSameFontCharacter(const char *p,const char *q)
{
  return(((*p != '\0') && (*q != '\0') && (*p == *q)) ? MagickTrue :
    MagickFalse);
}

static int FontCompare(const void *x,const void *y)
{
  char
    *p,
    *q;

  p=(char *) *((char **) x);
  q=(char *) *((char **) y);
  while (IsSameFontCharacter(p,q) != MagickFalse)
  {
    p++;
    q++;
  }
  return(*p-(*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static int FindFontBrowserCursor(XFontStruct *font_info,
  const XWidgetInfo reply_info,const int x,int i)
{
  if (font_info != (XFontStruct *) NULL)
    for (i=1; i <= Extent(reply_info.marker); i++)
      if (XTextWidth(font_info,reply_info.marker,i) > x)
        break;
  return(i);
}

static XWidgetInfo PageFontBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const unsigned int visible_fonts)
{
  if (event.xbutton.y < slider_info.y)
    slider_info.id-=((int) visible_fonts-1);
  else
    slider_info.id+=((int) visible_fonts-1);
  return(slider_info);
}

static size_t ReadFontBrowserPattern(const XWidgetInfo reply_info,
  char *back_pattern,char *glob_pattern,size_t state)
{
  /*
    Read new font or glob pattern.
  */
  if (*reply_info.text == '\0')
    return(state);
  (void) CopyMagickString(back_pattern,glob_pattern,MagickPathExtent);
  (void) CopyMagickString(glob_pattern,reply_info.text,MagickPathExtent);
  state|=UpdateListState;
  return(state);
}

typedef struct _FontBrowserReset
{
  XWindows
    *windows;

  XEvent
    event;

  XWidgetInfo
    reset_info;

  char
    *back_pattern,
    *glob_pattern;

  const char
    *reset_pattern;
} FontBrowserReset;

static size_t ReleaseFontBrowserReset(const FontBrowserReset *reset,
  size_t state)
{
  XWindows
    *const windows = reset->windows;

  const XEvent
    event = reset->event;

  const XWidgetInfo
    reset_info = reset->reset_info;

  char
    *const back_pattern = reset->back_pattern,
    *const glob_pattern = reset->glob_pattern;

  const char
    *const reset_pattern = reset->reset_pattern;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(reset_info,event.xbutton)))
    {
      (void) CopyMagickString(back_pattern,glob_pattern,MagickPathExtent);
      (void) CopyMagickString(glob_pattern,reset_pattern,MagickPathExtent);
      state|=UpdateListState;
    }
  return(state);
}

typedef struct _FontBrowserUpdate
{
  Display
    *display;

  XWindows
    *windows;

  char
    *reply,
    *glob_pattern;

  const char
    *back_pattern;

  char
    **listhead;
} FontBrowserUpdate;

typedef struct _FontBrowserUpdateTargets
{
  XWidgetInfo
    *action_info;

  char
    ***fontlist;

  int
    *fonts;
} FontBrowserUpdateTargets;

static MagickBooleanType UpdateFontBrowserList(const FontBrowserUpdate *update,
  const FontBrowserUpdateTargets *targets)
{
  Display
    *const display = update->display;

  XWindows
    *const windows = update->windows;

  char
    *const reply = update->reply,
    *const glob_pattern = update->glob_pattern;

  const char
    *const back_pattern = update->back_pattern;

  char
    **const listhead = update->listhead;

  XWidgetInfo
    *const action_info = targets->action_info;

  char
    ***const fontlist = targets->fontlist;

  int
    *const fonts = targets->fonts;

  char
    **checklist;

  int
    number_fonts;

  checklist=XListFonts(display,glob_pattern,32767,&number_fonts);
  if (checklist == (char **) NULL)
    {
      if ((strchr(glob_pattern,'*') == (char *) NULL) &&
          (strchr(glob_pattern,'?') == (char *) NULL))
        {
          /*
            Might be a scaleable font-- exit.
          */
          (void) CopyMagickString(reply,glob_pattern,MagickPathExtent);
          (void) CopyMagickString(glob_pattern,back_pattern,MagickPathExtent);
          (*action_info).raised=MagickFalse;
          XDrawBeveledButton(display,&windows->widget,action_info);
          return(MagickFalse);
        }
      (void) CopyMagickString(glob_pattern,back_pattern,MagickPathExtent);
      (void) XBell(display,0);
    }
  else
    if (number_fonts == 1)
      {
        /*
          Reply is a single font name-- exit.
        */
        (void) CopyMagickString(reply,checklist[0],MagickPathExtent);
        (void) CopyMagickString(glob_pattern,back_pattern,MagickPathExtent);
        (void) XFreeFontNames(checklist);
        (*action_info).raised=MagickFalse;
        XDrawBeveledButton(display,&windows->widget,action_info);
        return(MagickFalse);
      }
    else
      {
        (void) XFreeFontNames(listhead);
        (*fontlist)=(char **) RelinquishMagickMemory((*fontlist));
        (*fontlist)=checklist;
        (*fonts)=number_fonts;
      }
  return(MagickTrue);
}

typedef struct _FontBrowserWidgets
{
  XWidgetInfo
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *back_info,
    *reset_info,
    *action_info,
    *cancel_info,
    *slider_info;

  int
    *fonts;

  XWidgetInfo
    *scroll_info;

  unsigned int
    *visible_fonts;

  XWidgetInfo
    *north_info;

  size_t
    *delay;

  XWidgetInfo
    *south_info;

  int
    *i;

  XFontStruct
    **font_info;

  char
    ***fontlist;

  XWidgetInfo
    *list_info,
    *mode_info,
    *expose_info,
    *text_info,
    *selection_info;

  char
    ***listhead;

  MagickStatusType
    *mask;
} FontBrowserWidgets;

typedef struct _FontBrowserContext
{
  Display
    *display;

  XWindows
    *windows;

  char
    *glob_pattern,
    *back_pattern,
    *reset_pattern,
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
} FontBrowserContext;

static void ClientFontBrowserMessage(const FontBrowserContext *context,
  const XEvent event,const FontBrowserWidgets *widgets)
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

static void SelectFontBrowserReply(const FontBrowserContext *context,
  const XEvent event,const FontBrowserWidgets *widgets)
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

static void ToggleFontBrowserButtons(const FontBrowserContext *context,
  const XEvent event,const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const back_info = widgets->back_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  if ((*back_info).raised == MatteIsActive((*back_info),event.xmotion))
    {
      /*
        Back button status changed.
      */
      (*back_info).raised=!(*back_info).raised;
      XDrawBeveledButton(display,&windows->widget,back_info);
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

static void MoveFontBrowserButtons(const FontBrowserContext *context,
  XEvent event,const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  int
    *const fonts = widgets->fonts;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*slider_info).active)
    {
      /*
        Move slider matte.
      */
      (*slider_info)=TrackFontBrowserSlider((*slider_info),event,(*fonts));
      (*state)|=RedrawListState;
      return;
    }
  if ((*state) & InactiveWidgetState)
    return;
  ToggleFontBrowserButtons(context,event,widgets);
}

static void PressFontBrowserKey(const FontBrowserContext *context,
  const XEvent event,const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const back_pattern = context->back_pattern;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  int
    *const fonts = widgets->fonts;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_fonts = widgets->visible_fonts;

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
        (*visible_fonts),(*fonts));
      (*state)|=RedrawListState;
      return;
    }
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      (*state)=ReadFontBrowserPattern((*reply_info),back_pattern,glob_pattern,
        (*state));
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
}

static void ReleaseFontBrowserButtons(const FontBrowserContext *context,
  const XEvent event,const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const back_pattern = context->back_pattern,
    *const reset_pattern = context->reset_pattern;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const back_info = widgets->back_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  ListBrowserRelease
    release;

  if ((*back_info).raised == MagickFalse)
    {
      FontBrowserBack
        back;

      back.windows=windows;
      back.event=event;
      back.back_info=(*back_info);
      back.glob_pattern=glob_pattern;
      back.back_pattern=back_pattern;
      (*state)=ReleaseFontBrowserBack(&back,(*state));
      (*back_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,back_info);
    }
  if ((*reset_info).raised == MagickFalse)
    {
      FontBrowserReset
        reset;

      reset.windows=windows;
      reset.event=event;
      reset.reset_info=(*reset_info);
      reset.back_pattern=back_pattern;
      reset.glob_pattern=glob_pattern;
      reset.reset_pattern=reset_pattern;
      (*state)=ReleaseFontBrowserReset(&reset,(*state));
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
}

static void ReleaseFontBrowserScroll(const FontBrowserContext *context,
  const XEvent event,const FontBrowserWidgets *widgets)
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
  ReleaseFontBrowserButtons(context,event,widgets);
}

static void PressFontBrowserReply(const FontBrowserContext *context,
  const XEvent event,const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  int
    *const i = widgets->i;

  XFontStruct
    *const font_info = *widgets->font_info;

  int
    x;

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
      (*i)=FindFontBrowserCursor(font_info,(*reply_info),x,(*i));
      (*reply_info).cursor=(*reply_info).marker+(*i)-1;
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

static void PressFontBrowserButtons(const FontBrowserContext *context,
  const XEvent event,const FontBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const back_info = widgets->back_info,
    *const reset_info = widgets->reset_info;

  if (MatteIsActive((*back_info),event.xbutton))
    {
      /*
        User pressed Back button.
      */
      (*back_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,back_info);
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
  PressFontBrowserReply(context,event,widgets);
}

static void ChooseFontBrowserEntry(const FontBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const FontBrowserWidgets *widgets)
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

  int
    *const fonts = widgets->fonts;

  char
    ***const fontlist = widgets->fontlist;

  FontBrowserPress
    press;

  press.display=display;
  press.windows=windows;
  press.fontlist=(*fontlist);
  press.fonts=(*fonts);
  press.height=height;
  press.event=event;
  press.slider_info=(*slider_info);
  SelectFontBrowserEntry(&press,glob_pattern,targets);
}

static void PressFontBrowserScroll(const FontBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const FontBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  int
    *const fonts = widgets->fonts;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_fonts = widgets->visible_fonts;

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
      ((*slider_info).id < (*fonts)))
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
      (*slider_info)=PageFontBrowserSlider((*slider_info),event,
        (*visible_fonts));
      (*state)|=RedrawListState;
      return;
    }
  if (MatteIsActive((*list_info),event.xbutton))
    {
      ChooseFontBrowserEntry(context,event,targets,widgets);
      return;
    }
  PressFontBrowserButtons(context,event,widgets);
}

#define BackButtonText  "Back"
#define CancelButtonText  "Cancel"
#define FontnameText  "Name:"
#define FontPatternText  "Pattern:"
#define ResetButtonText  "Reset"

static void InitializeFontBrowserButtons(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  const char
    *const action = context->action;

  const unsigned int
    width = context->width;

  XWidgetInfo
    *const back_info = widgets->back_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  XFontStruct
    *const font_info = *widgets->font_info;

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
  XGetWidgetInfo(BackButtonText,back_info);
  (*back_info).width=width;
  (*back_info).height=(unsigned int) ((3*height) >> 1);
  (*back_info).x=QuantumMargin;
  (*back_info).y=((5*QuantumMargin) >> 1)+(int) height;
  XGetWidgetInfo(ResetButtonText,reset_info);
  (*reset_info).width=width;
  (*reset_info).height=(unsigned int) ((3*height) >> 1);
  (*reset_info).x=QuantumMargin;
  (*reset_info).y=(*back_info).y+(int) (*back_info).height+QuantumMargin;
}

static void InitializeFontBrowserReply(const FontBrowserContext *context,
  const FontBrowserWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height,
    width = context->width;

  char
    *const reply = context->reply;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const action_info = widgets->action_info;

  XFontStruct
    *const font_info = *widgets->font_info;

  XWidgetInfo
    *const mode_info = widgets->mode_info;

  /*
    Initialize reply information.
  */
  XGetWidgetInfo(reply,reply_info);
  (*reply_info).raised=MagickFalse;
  (*reply_info).bevel_width--;
  (*reply_info).width=(unsigned int) ((int) windows->widget.width-(int)
    width-((6*QuantumMargin) >> 1));
  (*reply_info).height=height << 1;
  (*reply_info).x=(int) width+(QuantumMargin << 1);
  (*reply_info).y=(*action_info).y-(int) ((*action_info).height << 1)-
    QuantumMargin;
  /*
    Initialize mode information.
  */
  XGetWidgetInfo(reply,mode_info);
  (*mode_info).bevel_width=0;
  (*mode_info).width=(unsigned int)
    ((*action_info).x-(*reply_info).x-QuantumMargin);
  (*mode_info).height=(*action_info).height << 1;
  (*mode_info).x=(*reply_info).x;
  (*mode_info).y=(*action_info).y-(int) (*action_info).height+(int)
    (*action_info).bevel_width;
}

#endif

#endif
