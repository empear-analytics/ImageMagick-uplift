/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%                  W   W  IIIII  DDDD    GGGG  EEEEE  TTTTT                   %
%                  W   W    I    D   D  G      E        T                     %
%                  W W W    I    D   D  G  GG  EEE      T                     %
%                  WW WW    I    D   D  G   G  E        T                     %
%                  W   W  IIIII  DDDD    GGG   EEEEE    T                     %
%                                                                             %
%                         TTTTT  EEEEE  X   X  TTTTT                          %
%                           T    E       X X     T                            %
%                           T    EEE      X      T                            %
%                           T    E       X X     T                            %
%                           T    EEEEE  X   X    T                            %
%                                                                             %
%                                                                             %
%                   MagickCore X11 Text View Widget Methods                   %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                              September 1993                                 %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/image.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/string_.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/token.h"
#include "MagickCore/token-private.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/xwindow-private.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/widget-core-private.h"
#include "MagickCore/widget-browser-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X T e x t V i e w W i d g e t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTextViewWidget() displays text in a Text View widget.
%
%  The format of the XTextViewWidget method is:
%
%      void XTextViewWidget(Display *display,const XResourceInfo *resource_info,
%        XWindows *windows,const MagickBooleanType mono,const char *title,
%        const char **textlist)
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
%    o mono:  Use mono-spaced font when displaying text.
%
%    o title: This character string is displayed at the top of the widget
%      window.
%
%    o textlist: This string list is displayed within the Text View widget.
%
*/
typedef struct _TextViewLines
{
  Display
    *display;

  XWindows
    *windows;

  const char
    **textlist;

  unsigned int
    lines,
    visible_lines;

  const XWidgetInfo
    *list_info;

  XWidgetInfo
    slider_info;
} TextViewLines;

static void DrawTextViewLines(const TextViewLines *view,
  XWidgetInfo *selection_info)
{
  Display
    *const display = view->display;

  XWindows
    *const windows = view->windows;

  const char
    **const textlist = view->textlist;

  const unsigned int
    lines = view->lines,
    visible_lines = view->visible_lines;

  const XWidgetInfo
    *const list_info = view->list_info;

  const XWidgetInfo
    slider_info = view->slider_info;

  int
    i;

  for (i=0; i < (int) visible_lines; i++)
  {
    (*selection_info).raised=
      (slider_info.id+i) != (*list_info).id ? MagickTrue : MagickFalse;
    (*selection_info).text=(char *) NULL;
    if ((slider_info.id+i) < (int) lines)
      (*selection_info).text=(char *) textlist[slider_info.id+i];
    XDrawWidgetText(display,&windows->widget,&(*selection_info));
    (*selection_info).y+=(int) (*selection_info).height;
  }
}

typedef struct _TextViewTargets
{
  XWidgetInfo
    *list_info,
    *selection_info;

  size_t
    *state;
} TextViewTargets;

typedef struct _TextViewPress
{
  Display
    *display;

  XWindows
    *windows;

  const char
    **textlist;

  unsigned int
    lines,
    height;

  XEvent
    event;

  XWidgetInfo
    slider_info;
} TextViewPress;

static void SelectTextViewLine(const TextViewPress *press,
  char *primary_selection,const TextViewTargets *targets)
{
  Display
    *const display = press->display;

  XWindows
    *const windows = press->windows;

  const char
    **const textlist = press->textlist;

  const unsigned int
    lines = press->lines,
    height = press->height;

  const XEvent
    event = press->event;

  const XWidgetInfo
    slider_info = press->slider_info;

  XWidgetInfo
    *const list_info = targets->list_info,
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  int
    id;

  static Time
    click_time;

  /*
    User pressed list matte.
  */
  id=slider_info.id+(event.xbutton.y-((*list_info).y+(int)
    (height >> 1))+1)/(int) (*selection_info).height;
  if (id >= (int) lines)
    return;
  if (id != (*list_info).id)
    {
      (*list_info).id=id;
      click_time=event.xbutton.time;
      return;
    }
  (*list_info).id=id;
  if (event.xbutton.time >= (click_time+(unsigned long) DoubleClick))
    {
      click_time=event.xbutton.time;
      return;
    }
  click_time=event.xbutton.time;
  /*
    Become the XA_PRIMARY selection owner.
  */
  (void) CopyMagickString(primary_selection,textlist[(*list_info).id],
    MagickPathExtent);
  (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
    event.xbutton.time);
  if (XGetSelectionOwner(display,XA_PRIMARY) != windows->widget.id)
    return;
  (*selection_info).id=(~0);
  (*list_info).id=id;
  (*state)|=RedrawListState;
}

static void ReleaseTextViewDismiss(const DialogRelease *release,
  XWidgetInfo *dismiss_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindows
    *const windows = release->windows;

  const XEvent
    event = release->event;

  if ((*dismiss_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == windows->widget.id) &&
          (MatteIsActive((*dismiss_info),event.xbutton)))
        (*state)|=ExitState;
      (*dismiss_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,&(*dismiss_info));
    }
}

static void MoveTextViewSliderByPage(const KeySym key_symbol,
  const unsigned int visible_lines,const unsigned int lines,
  XWidgetInfo *slider_info)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_KP_Prior:
    {
      (*slider_info).id-=(int) visible_lines;
      break;
    }
    case XK_Next:
    case XK_KP_Next:
    {
      (*slider_info).id+=(int) visible_lines;
      break;
    }
    case XK_End:
    case XK_KP_End:
    {
      (*slider_info).id=(int) lines;
      break;
    }
    default:
      break;
  }
}

static XWidgetInfo MoveTextViewSlider(XWidgetInfo slider_info,
  const KeySym key_symbol,const unsigned int visible_lines,
  const unsigned int lines)
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
      MoveTextViewSliderByPage(key_symbol,visible_lines,lines,&slider_info);
      break;
    }
  }
  return(slider_info);
}

#define DismissButtonText  "Dismiss"

typedef struct _TextViewWidgets
{
  XWidgetInfo
    *north_info;

  XWidgetInfo
    *south_info;

  size_t
    *delay;

  XWidgetInfo
    *slider_info;

  XWidgetInfo
    *dismiss_info;

  size_t
    *state;

  XWidgetInfo
    *scroll_info;

  unsigned int
    *visible_lines;

  XEvent
    *event;

  XWidgetInfo
    *list_info;

  XWidgetInfo
    *selection_info;

  MagickStatusType
    *mask;

  XWidgetInfo
    *expose_info;
} TextViewWidgets;

typedef struct _TextViewContext
{
  Display
    *display;

  XWindows
    *windows;

  unsigned int
    lines;

  const char
    **textlist;

  unsigned int
    height;

  char
    *primary_selection;

  unsigned int
    width;

  XFontStruct
    *font_info;

  XFontStruct
    *text_info;
} TextViewContext;

static void ReleaseTextViewScroll(const TextViewContext *context,
  const XEvent event,const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  size_t
    *const delay = widgets->delay;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const dismiss_info = widgets->dismiss_info;

  size_t
    *const state = widgets->state;

  DialogRelease
    release;

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
  release.display=display;
  release.windows=windows;
  release.event=event;
  ReleaseTextViewDismiss(&release,dismiss_info,state);
}

static void PressTextViewKey(const TextViewContext *context,const XEvent event,
  const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    lines = context->lines;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const dismiss_info = widgets->dismiss_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_lines = widgets->visible_lines;

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
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      (*dismiss_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      (*state)|=ExitState;
      return;
    }
  if (AreaIsActive((*scroll_info),event.xkey))
    {
      /*
        Move slider.
      */
      (*slider_info)=MoveTextViewSlider((*slider_info),key_symbol,
        (*visible_lines),lines);
      (*state)|=RedrawListState;
      return;
    }
}

static void MoveTextViewPointer(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    lines = context->lines;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const dismiss_info = widgets->dismiss_info;

  size_t
    *const state = widgets->state;

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
      (*slider_info)=TrackListBrowserSlider((*slider_info),(*event),lines);
      (*state)|=RedrawListState;
      return;
    }
  if ((*state) & InactiveWidgetState)
    return;
  if ((*dismiss_info).raised == MatteIsActive((*dismiss_info),(*event).xmotion))
    {
      /*
        Dismiss button status changed.
      */
      (*dismiss_info).raised=
        (*dismiss_info).raised == MagickFalse ? MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
}

static void PressTextViewList(const TextViewContext *context,const XEvent event,
  const TextViewTargets *targets,const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    lines = context->lines;

  const char
    **const textlist = context->textlist;

  const unsigned int
    height = context->height;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const dismiss_info = widgets->dismiss_info;

  XWidgetInfo
    *const list_info = widgets->list_info;

  if (MatteIsActive((*dismiss_info),event.xbutton))
    {
      /*
        User pressed Dismiss button.
      */
      (*dismiss_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
  if (MatteIsActive((*list_info),event.xbutton))
    {
      TextViewPress
        press;

      press.display=display;
      press.windows=windows;
      press.textlist=textlist;
      press.lines=lines;
      press.height=height;
      press.event=event;
      press.slider_info=(*slider_info);
      SelectTextViewLine(&press,primary_selection,targets);
      return;
    }
}

static void PressTextViewScroll(const TextViewContext *context,
  const XEvent event,const TextViewTargets *targets,
  const TextViewWidgets *widgets)
{
  const unsigned int
    lines = context->lines;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_lines = widgets->visible_lines;

  if ((MatteIsActive((*south_info),event.xbutton)) &&
      ((*slider_info).id < (int) lines))
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
        (*visible_lines));
      (*state)|=RedrawListState;
      return;
    }
  PressTextViewList(context,event,targets,widgets);
}

static void PressTextViewSlider(const TextViewContext *context,
  const XEvent event,const TextViewTargets *targets,
  const TextViewWidgets *widgets)
{
  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const state = widgets->state;

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
  PressTextViewScroll(context,event,targets,widgets);
}

static void HandleTextViewSelectionEvent(const TextViewContext *context,
  XEvent event,const TextViewWidgets *widgets)
{
  char
    *const primary_selection = context->primary_selection;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const list_info = widgets->list_info;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  switch (event.type)
  {
    case MotionNotify:
    {
      MoveTextViewPointer(context,widgets);
      break;
    }
    case SelectionClear:
    {
      (*list_info).id=(~0);
      (*selection_info).id=(~0);
      (*state)|=RedrawListState;
      break;
    }
    case SelectionRequest:
    {
      XSelectionEvent
        notify;

      XSelectionRequestEvent
        *request;

      if ((*list_info).id == (~0))
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

static void HandleTextViewKeyEvent(const TextViewContext *context,
  const XEvent event,const TextViewWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  MagickStatusType
    *const mask = widgets->mask;

  switch (event.type)
  {
    case KeyPress:
    {
      PressTextViewKey(context,event,widgets);
      break;
    }
    case KeyRelease:
      break;
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
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
      HandleTextViewSelectionEvent(context,event,widgets);
      break;
    }
  }
}

static void HandleTextViewWindowEvent(const TextViewContext *context,
  const XEvent event,const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case ButtonRelease:
    {
      ReleaseTextViewScroll(context,event,widgets);
      break;
    }
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
      HandleTextViewKeyEvent(context,event,widgets);
      break;
    }
  }
}

static void InitializeTextViewButtons(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  const unsigned int
    width = context->width;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const dismiss_info = widgets->dismiss_info;

  /*
    Initialize button information.
  */
  XGetWidgetInfo(DismissButtonText,dismiss_info);
  (*dismiss_info).width=width;
  (*dismiss_info).height=(unsigned int) ((3*height) >> 1);
  (*dismiss_info).x=(int) windows->widget.width-(int) (*dismiss_info).width-
    QuantumMargin-2;
  (*dismiss_info).y=(int) windows->widget.height-(int) (*dismiss_info).height-
    QuantumMargin;
}

static void InitializeTextViewSlider(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  const unsigned int
    lines = context->lines;

  XFontStruct
    *const text_info = context->text_info;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_lines = widgets->visible_lines;

  XWidgetInfo
    *const expose_info = widgets->expose_info;

  int
    id;

  id=(*slider_info).id;
  (*slider_info)=(*north_info);
  (*slider_info).id=id;
  (*slider_info).width-=2;
  (*slider_info).min_y=(*north_info).y+(int) (*north_info).height+(int)
    (*north_info).bevel_width+(int) (*slider_info).bevel_width+2;
  (*slider_info).height=(unsigned int) ((int) (*scroll_info).height-
    (((*slider_info).min_y-(*scroll_info).y+1) << 1)+4);
  (*visible_lines)=(unsigned int) ((*scroll_info).height*MagickSafeReciprocal(
    (double) text_info->ascent+text_info->descent+((text_info->ascent+
    text_info->descent) >> 3)));
  if (lines > (*visible_lines))
    (*slider_info).height=(unsigned int)
      ((*visible_lines)*(*slider_info).height)/lines;
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-(int)
    (*slider_info).bevel_width-2;
  (*slider_info).x=(*scroll_info).x+(int) (*slider_info).bevel_width+1;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info)=(*scroll_info);
  (*expose_info).y=(*slider_info).y;
}

static void InitializeTextViewScroll(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const dismiss_info = widgets->dismiss_info;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  /*
    Initialize scroll information.
  */
  XGetWidgetInfo((char *) NULL,scroll_info);
  (*scroll_info).bevel_width--;
  (*scroll_info).width=height;
  (*scroll_info).height=(unsigned int) ((*dismiss_info).y-((5*QuantumMargin) >>
    1));
  (*scroll_info).x=(int) windows->widget.width-QuantumMargin-(int)
    (*scroll_info).width;
  (*scroll_info).y=(3*QuantumMargin) >> 1;
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
  InitializeTextViewSlider(context,widgets);
}

static void InitializeTextViewList(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  XWidgetInfo
    *const list_info = widgets->list_info;

  /*
    Initialize list information.
  */
  XGetWidgetInfo((char *) NULL,list_info);
  (*list_info).raised=MagickFalse;
  (*list_info).bevel_width--;
  (*list_info).width=(unsigned int) ((*scroll_info).x-((3*QuantumMargin) >> 1));
  (*list_info).height=(*scroll_info).height;
  (*list_info).x=QuantumMargin;
  (*list_info).y=(*scroll_info).y;
}

static void InitializeTextViewSelection(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  XFontStruct
    *const text_info = context->text_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const list_info = widgets->list_info;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  /*
    Initialize selection information.
  */
  XGetWidgetInfo((char *) NULL,selection_info);
  (*selection_info).center=MagickFalse;
  (*selection_info).width=(*list_info).width;
  (*selection_info).height=(unsigned int)
    (9*(text_info->ascent+text_info->descent)) >> 3;
  (*selection_info).x=(*list_info).x;
  (*state)&=(unsigned int) (~UpdateConfigurationState);
}

static void UpdateTextViewLayout(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  if ((*state) & UpdateConfigurationState)
    {
      InitializeTextViewButtons(context,widgets);
      InitializeTextViewScroll(context,widgets);
      InitializeTextViewList(context,widgets);
      InitializeTextViewSelection(context,widgets);
    }
}

static void RedrawTextViewWidget(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const dismiss_info = widgets->dismiss_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  XWidgetInfo
    *const list_info = widgets->list_info;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Text View window.
      */
      XDrawBeveledMatte(display,&windows->widget,list_info);
      XDrawBeveledMatte(display,&windows->widget,scroll_info);
      XDrawTriangleNorth(display,&windows->widget,north_info);
      XDrawBeveledButton(display,&windows->widget,slider_info);
      XDrawTriangleSouth(display,&windows->widget,south_info);
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
      (*selection_info).id=(~0);
      (*state)|=RedrawListState;
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void UpdateTextViewSlider(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const expose_info = widgets->expose_info;

  /*
    Update slider.
  */
  (*expose_info)=PlaceTextViewExpose((*expose_info),(*slider_info));
  XDrawTriangleNorth(display,&windows->widget,north_info);
  XDrawMatte(display,&windows->widget,expose_info);
  XDrawBeveledButton(display,&windows->widget,slider_info);
  XDrawTriangleSouth(display,&windows->widget,south_info);
  (*expose_info).y=(*slider_info).y;
}

static void RedrawTextViewText(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    lines = context->lines;

  const char
    **const textlist = context->textlist;

  const unsigned int
    height = context->height;

  XFontStruct
    *const font_info = context->font_info;

  XFontStruct
    *const text_info = context->text_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  unsigned int
    *const visible_lines = widgets->visible_lines;

  XWidgetInfo
    *const list_info = widgets->list_info;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  TextViewLines
    view;

  /*
    Redraw scroll bar and text.
  */
  windows->widget.font_info=text_info;
  (void) XSetFont(display,windows->widget.annotate_context,
    text_info->fid);
  (void) XSetFont(display,windows->widget.highlight_context,
    text_info->fid);
  (*selection_info).id=(*slider_info).id;
  (*selection_info).y=(*list_info).y+(int) (height >> 3)+2;
  view.display=display;
  view.windows=windows;
  view.textlist=textlist;
  view.lines=lines;
  view.visible_lines=(*visible_lines);
  view.list_info=(list_info);
  view.slider_info=(*slider_info);
  DrawTextViewLines(&view,selection_info);
  windows->widget.font_info=font_info;
  (void) XSetFont(display,windows->widget.annotate_context,
    font_info->fid);
  (void) XSetFont(display,windows->widget.highlight_context,
    font_info->fid);
  UpdateTextViewSlider(context,widgets);
}

static void RedrawTextViewLines(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  const unsigned int
    lines = context->lines;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const state = widgets->state;

  unsigned int
    *const visible_lines = widgets->visible_lines;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  if ((*state) & RedrawListState)
    {
      /*
        Determine slider id and position.
      */
      if ((*slider_info).id >= (int) (lines-(*visible_lines)))
        (*slider_info).id=(int) (lines-(*visible_lines));
      if (((*slider_info).id < 0) || (lines <= (*visible_lines)))
        (*slider_info).id=0;
      (*slider_info).y=(*slider_info).min_y;
      if (lines != 0)
        (*slider_info).y+=(*slider_info).id*((*slider_info).max_y-
          (*slider_info).min_y+1)/(int) lines;
      if ((*slider_info).id != (*selection_info).id)
        {
          RedrawTextViewText(context,widgets);
        }
      (*state)&=(unsigned int) (~RedrawListState);
    }
}

static MagickBooleanType WaitForTextViewEvent(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    lines = context->lines;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  size_t
    *const delay = widgets->delay;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const state = widgets->state;

  XEvent
    *const event = widgets->event;

  /*
    Wait for next event.
  */
  if ((*north_info).raised && (*south_info).raised)
    (void) XIfEvent(display,event,XScreenEvent,(char *) windows);
  else
    {
      /*
        Brief delay before advancing scroll bar.
      */
      XDelay(display,(*delay));
      (*delay)=SuspendTime;
      (void) XCheckIfEvent(display,event,XScreenEvent,(char *) windows);
      if (((*north_info).raised == MagickFalse) && ((*slider_info).id > 0))
        {
          /*
            Move slider up.
          */
          (*slider_info).id--;
          (*state)|=RedrawListState;
        }
      if (((*south_info).raised == MagickFalse) &&
          ((*slider_info).id < (int) lines))
        {
          /*
            Move slider down.
          */
          (*slider_info).id++;
          (*state)|=RedrawListState;
        }
      if ((*event).type != ButtonRelease)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static void RespondToTextViewEvents(const TextViewContext *context,
  const TextViewTargets *targets,const TextViewWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  XEvent
    *const event = widgets->event;

  do
  {
    UpdateTextViewLayout(context,widgets);
    RedrawTextViewWidget(context,widgets);
    RedrawTextViewLines(context,widgets);
    if (WaitForTextViewEvent(context,widgets) == MagickFalse)
      continue;
    switch ((*event).type)
    {
      case ButtonPress:
      {
        PressTextViewSlider(context,(*event),targets,widgets);
        break;
      }
      default:
      {
        HandleTextViewWindowEvent(context,(*event),widgets);
        break;
      }
    }
  } while (((*state) & ExitState) == 0);
}

static void WithdrawTextViewWidget(Display *display,XWindows *windows,
  XFontStruct *text_info)
{
  if (text_info != windows->widget.font_info)
    (void) XFreeFont(display,text_info);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
}

static void MapTextViewWidget(Display *display,XWindows *windows,
  const char *title,MagickStatusType mask)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,title,MagickPathExtent);
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
  (void) XReconfigureWMWindow(display,windows->widget.id,
    windows->widget.screen,(unsigned int) mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

static XFontStruct *SelectTextViewFont(Display *display,
  const XResourceInfo *resource_info,XWindows *windows,
  const MagickBooleanType mono)
{
  XFontStruct
    *text_info;

  text_info=(XFontStruct *) NULL;
  if (mono != MagickFalse)
    text_info=XBestFont(display,resource_info,MagickTrue);
  if (text_info == (XFontStruct *) NULL)
    text_info=windows->widget.font_info;
  return(text_info);
}

static void RunTextView(const TextViewContext *context,
  MagickStatusType *mask)
{
  unsigned int
    visible_lines;

  size_t
    delay,
    state;

  XEvent
    event;

  XWidgetInfo
    dismiss_info,
    expose_info,
    list_info,
    north_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info;

  const TextViewTargets
    targets = { &list_info, &selection_info, &state };

  const TextViewWidgets
    view_widgets = { &north_info, &south_info, &delay, &slider_info,
      &dismiss_info, &state, &scroll_info, &visible_lines, &event, &list_info,
      &selection_info, mask, &expose_info };

  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_lines=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  RespondToTextViewEvents(context,&targets,&view_widgets);
}

MagickPrivate void XTextViewWidget(Display *display,
  const XResourceInfo *resource_info,XWindows *windows,
  const MagickBooleanType mono,const char *title,const char **textlist)
{
  char
    primary_selection[MagickPathExtent];

  int
    i;

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  unsigned int
    height,
    lines,
    text_width,
    width;

  XFontStruct
    *font_info,
    *text_info;

  /*
    Convert text string to a text list.
  */
  assert(display != (Display *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(title != (const char *) NULL);
  assert(textlist != (const char **) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",title);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (textlist == (const char **) NULL)
    {
      XNoticeWidget(display,windows,"No text to view:",(char *) NULL);
      return;
    }
  /*
    Determine Text View widget attributes.
  */
  font_info=windows->widget.font_info;
  text_info=SelectTextViewFont(display,resource_info,windows,mono);
  text_width=0;
  for (i=0; textlist[i] != (char *) NULL; i++)
    if (WidgetTextWidth(text_info,(char *) textlist[i]) > text_width)
      text_width=(unsigned int) XTextWidth(text_info,(char *) textlist[i],
        MagickMin(Extent(textlist[i]),160));
  lines=(unsigned int) i;
  width=WidgetTextWidth(font_info,DismissButtonText);
  width+=(unsigned int) QuantumMargin;
  height=(unsigned int) (text_info->ascent+text_info->descent);
  /*
    Position Text View widget.
  */
  windows->widget.width=(unsigned int) (MagickMin((int) text_width,
    (int) MaxTextWidth)+5*QuantumMargin);
  windows->widget.min_width=(unsigned int) ((int) MinTextWidth+4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (MagickMin(MagickMax((int) lines,3),32)*
    (int) height+(int) ((13*height) >> 1)+((9*QuantumMargin) >> 1));
  windows->widget.min_height=(3*height+((13*height) >> 1)+(unsigned int) ((9*
    QuantumMargin) >> 1));
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Text View widget.
  */
  MapTextViewWidget(display,windows,title,mask);
  /*
    Respond to X events.
  */
  height=(unsigned int) (font_info->ascent+font_info->descent);
  {
    const TextViewContext
      view_context = { display, windows, lines, textlist, height,
        primary_selection, width, font_info, text_info };

    RunTextView(&view_context,&mask);
  }
  WithdrawTextViewWidget(display,windows,text_info);
}
#endif
