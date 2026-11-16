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
%                  DDDD   IIIII   AAA   L       OOO    GGGG                   %
%                  D   D    I    A   A  L      O   O  G                       %
%                  D   D    I    AAAAA  L      O   O  G  GG                   %
%                  D   D    I    A   A  L      O   O  G   G                   %
%                  DDDD   IIIII  A   A  LLLLL   OOO    GGG                    %
%                                                                             %
%                                                                             %
%                    MagickCore X11 Dialog Widget Methods                     %
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
#if defined(MAGICKCORE_X11_DELEGATE)

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D i a l o g W i d g e t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDialogWidget() displays a Dialog widget with a query to the user.  The user
%  keys a reply and presses the Ok or Cancel button to exit.  The typed text is
%  returned as the reply function parameter.
%
%  The format of the XDialogWidget method is:
%
%      int XDialogWidget(Display *display,XWindows *windows,const char *action,
%        const char *query,char *reply)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o action: Specifies a pointer to the action of this widget.
%
%    o query: Specifies a pointer to the query to present to the user.
%
%    o reply: the response from the user is returned in this parameter.
%
*/

static void ReleaseDialogAction(const DialogRelease *release,
  XWidgetInfo *action_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindows
    *const windows = release->windows;

  const XEvent
    event = release->event;

  if ((*action_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == windows->widget.id) &&
          (MatteIsActive((*action_info),event.xbutton)))
        (*state)|=ExitState;
      (*action_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,&(*action_info));
    }
}

static void ReleaseDialogCancel(const DialogRelease *release,
  const XWidgetInfo *reply_info,XWidgetInfo *cancel_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindows
    *const windows = release->windows;

  const XEvent
    event = release->event;

  if ((*cancel_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == windows->widget.id) &&
          (MatteIsActive((*cancel_info),event.xbutton)))
        {
          *(*reply_info).text='\0';
          (*state)|=ExitState;
        }
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,&(*cancel_info));
    }
}

typedef struct _DialogPress
{
  Display
    *display;

  XWindows
    *windows;

  XEvent
    event;

  XFontStruct
    *font_info;
} DialogPress;

static void PressDialogReply(const DialogPress *press,char *primary_selection,
  XWidgetInfo *reply_info)
{
  Display
    *const display = press->display;

  XWindows
    *const windows = press->windows;

  const XEvent
    event = press->event;

  XFontStruct
    *const font_info = press->font_info;

  int
    i,
    x;

  static Time
    click_time;

  /*
    Move text cursor to position of button press.
  */
  x=event.xbutton.x-(*reply_info).x-(int) (QuantumMargin >> 2);
  for (i=1; i <= Extent((*reply_info).marker); i++)
    if (XTextWidth(font_info,(*reply_info).marker,i) > x)
      break;
  (*reply_info).cursor=(*reply_info).marker+i-1;
  if (event.xbutton.time > (click_time+(unsigned long) (*DoubleClickReference())))
    (*reply_info).highlight=MagickFalse;
  else
    {
      /*
        Become the XA_PRIMARY selection owner.
      */
      (void) CopyMagickString(primary_selection,(*reply_info).text,
        MagickPathExtent);
      (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
        event.xbutton.time);
      (*reply_info).highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
        windows->widget.id ? MagickTrue : MagickFalse;
    }
  XDrawMatteText(display,&windows->widget,reply_info);
  click_time=event.xbutton.time;
}

static void ApplyDialogControlKey(const KeySym key_symbol,const size_t state,
  XWidgetInfo *reply_info)
{
  if (state & ControlState)
    switch ((int) key_symbol)
    {
      case XK_u:
      case XK_U:
      {
        /*
          Erase the entire line of text.
        */
        *(*reply_info).text='\0';
        (*reply_info).cursor=(*reply_info).text;
        (*reply_info).marker=(*reply_info).text;
        (*reply_info).highlight=MagickFalse;
        break;
      }
      default:
        break;
    }
}

#define CancelButtonText  "Cancel"

typedef struct _DialogTargets
{
  XWidgetInfo
    *special_info,
    *action_info,
    *cancel_info,
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *text_info;
} DialogTargets;

typedef struct _DialogContext
{
  Display
    *display;

  XWindows
    *windows;

  XFontStruct
    *font_info;

  unsigned int
    anomaly;

  char
    *primary_selection;

  const char
    *action,
    *query;

  unsigned int
    height;

  MagickBooleanType
    raised;

  char
    *reply;

  unsigned int
    width;
} DialogContext;

static void PressDialogButton(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    anomaly = context->anomaly;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const special_info = targets->special_info,
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info,
    *const reply_info = targets->reply_info;

  if ((anomaly) && (MatteIsActive((*special_info),event.xbutton)))
    {
      /*
        Option button status changed.
      */
      (*special_info).raised=!(*special_info).raised;
      XDrawBeveledButton(display,&windows->widget,special_info);
      return;
    }
  if (MatteIsActive((*action_info),event.xbutton))
    {
      /*
        User pressed Action button.
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
      DialogPress
        press;

      press.display=display;
      press.windows=windows;
      press.event=event;
      press.font_info=font_info;
      PressDialogReply(&press,primary_selection,reply_info);
      return;
    }
  /*
    Request primary selection.
  */
  (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
    windows->widget.id,event.xbutton.time);
}

static void ReleaseDialogButtons(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info,
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

  DialogRelease
    release;

  if (windows->widget.mapped == MagickFalse)
    return;
  release.display=display;
  release.windows=windows;
  release.event=event;
  ReleaseDialogAction(&release,action_info,state);
  release.display=display;
  release.windows=windows;
  release.event=event;
  ReleaseDialogCancel(&release,reply_info,cancel_info,state);
}

static void ClientDialogMessage(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

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

static void KeyDialogPress(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const action_info = targets->action_info,
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

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
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      (*state)|=ExitState;
      return;
    }
  if (key_symbol == XK_Control_L)
    {
      (*state)|=ControlState;
      return;
    }
  ApplyDialogControlKey(key_symbol,(*state),reply_info);
  XEditText(display,reply_info,key_symbol,command,(*state));
  XDrawMatteText(display,&windows->widget,reply_info);
}

static void KeyDialogRelease(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  static char
    command[MagickPathExtent];

  static KeySym
    key_symbol;

  /*
    Respond to a user key release.
  */
  if (event.xkey.window != windows->widget.id)
    return;
  (void) XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if (key_symbol == XK_Control_L)
    (*state)&=(unsigned int) (~ControlState);
}

static void MoveDialogButtons(const DialogContext *context,XEvent event,
  const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info;

  size_t
    *const state = targets->state;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*state) & InactiveWidgetState)
    return;
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

static void SelectDialogReply(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

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
    }
  (void) XFree((void *) data);
}

static void SetDialogOptionText(const char *action,
  XWidgetInfo *special_info)
{
  if (LocaleCompare(action,"Background") == 0)
    (*special_info).text=(char *) "Backdrop";
  if (LocaleCompare(action,"New") == 0)
    (*special_info).text=(char *) "Gradation";
  if (LocaleCompare(action,"Resize") == 0)
    (*special_info).text=(char *) "Constrain ratio";
  if (LocaleCompare(action,"Save") == 0)
    (*special_info).text=(char *) "Non-progressive";
  if (LocaleCompare(action,"Shade") == 0)
    (*special_info).text=(char *) "Color shading";
}

static void InitializeDialogOptions(const DialogContext *context,
  const DialogTargets *targets)
{
  XFontStruct
    *const font_info = context->font_info;

  const char
    *const action = context->action,
    *const query = context->query;

  const unsigned int
    height = context->height;

  const MagickBooleanType
    raised = context->raised;

  XWidgetInfo
    *const special_info = targets->special_info,
    *const action_info = targets->action_info,
    *const reply_info = targets->reply_info,
    *const text_info = targets->text_info;

  /*
    Initialize option information.
  */
  XGetWidgetInfo("Dither",special_info);
  (*special_info).raised=raised;
  (*special_info).bevel_width--;
  (*special_info).width=(unsigned int) QuantumMargin >> 1;
  (*special_info).height=(unsigned int) QuantumMargin >> 1;
  (*special_info).x=(*reply_info).x;
  (*special_info).y=(*action_info).y+(int) (*action_info).height-(int)
    (*special_info).height;
  SetDialogOptionText(action,special_info);
  /*
    Initialize text information.
  */
  XGetWidgetInfo(query,text_info);
  (*text_info).width=(*reply_info).width;
  (*text_info).height=height;
  (*text_info).x=(*reply_info).x-(int) (QuantumMargin >> 1);
  (*text_info).y=QuantumMargin;
  (*text_info).center=MagickFalse;
}

static void UpdateDialogLayout(const DialogContext *context,
  const DialogTargets *targets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const char
    *const action = context->action;

  char
    *const reply = context->reply;

  const unsigned int
    width = context->width,
    height = context->height;

  XWidgetInfo
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info,
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

  if ((*state) & UpdateConfigurationState)
    {
      /*
        Initialize button information.
      */
      XGetWidgetInfo(CancelButtonText,cancel_info);
      (*cancel_info).width=width;
      (*cancel_info).height=(unsigned int) ((3*height) >> 1);
      (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
        ((3*QuantumMargin) >> 1);
      (*cancel_info).y=(int) windows->widget.height-(int) (*cancel_info).height-
        ((3*QuantumMargin) >> 1);
      XGetWidgetInfo(action,action_info);
      (*action_info).width=width;
      (*action_info).height=(unsigned int) ((3*height) >> 1);
      (*action_info).x=(*cancel_info).x-((int) (*cancel_info).width+QuantumMargin+
        (int) ((*action_info).bevel_width << 1));
      (*action_info).y=(*cancel_info).y;
      /*
        Initialize reply information.
      */
      XGetWidgetInfo(reply,reply_info);
      (*reply_info).raised=MagickFalse;
      (*reply_info).bevel_width--;
      (*reply_info).width=(unsigned int) ((int) windows->widget.width-
        (3*QuantumMargin));
      (*reply_info).height=height << 1;
      (*reply_info).x=(3*QuantumMargin) >> 1;
      (*reply_info).y=(*action_info).y-(int) (*reply_info).height-QuantumMargin;
      InitializeDialogOptions(context,targets);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawDialogWidget(const DialogContext *context,
  const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    anomaly = context->anomaly;

  XWidgetInfo
    *const special_info = targets->special_info,
    *const action_info = targets->action_info,
    *const cancel_info = targets->cancel_info,
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const text_info = targets->text_info;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Dialog widget.
      */
      XDrawWidgetText(display,&windows->widget,text_info);
      XDrawBeveledMatte(display,&windows->widget,reply_info);
      XDrawMatteText(display,&windows->widget,reply_info);
      if (anomaly)
        XDrawBeveledButton(display,&windows->widget,special_info);
      XDrawBeveledButton(display,&windows->widget,action_info);
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      XHighlightWidget(display,&windows->widget,(*BorderOffsetReference()),(*BorderOffsetReference()));
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void SendDialogSelection(const XSelectionRequestEvent *request,
  char *primary_selection)
{
  XSelectionEvent
    notify;

  (void) XChangeProperty(request->display,request->requestor,
    request->property,request->target,8,PropModeReplace,
    (unsigned char *) primary_selection,Extent(primary_selection));
  notify.type=SelectionNotify;
  notify.display=request->display;
  notify.requestor=request->requestor;
  notify.selection=request->selection;
  notify.target=request->target;
  notify.time=request->time;
  if (request->property == None)
    notify.property=request->target;
  else
    notify.property=request->property;
  (void) XSendEvent(request->display,request->requestor,False,0,
    (XEvent *) &notify);
}

static void HandleDialogSelectionEvent(const DialogContext *context,
  XEvent event,const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = targets->reply_info;

  switch (event.type)
  {
    case SelectionClear:
    {
      (*reply_info).highlight=MagickFalse;
      XDrawMatteText(display,&windows->widget,reply_info);
      break;
    }
    case SelectionNotify:
    {
      SelectDialogReply(context,event,targets);
      break;
    }
    case SelectionRequest:
    {
      XSelectionRequestEvent
        *request;

      if ((*reply_info).highlight == MagickFalse)
        break;
      /*
        Set primary selection.
      */
      request=(&(event.xselectionrequest));
      SendDialogSelection(request,primary_selection);
    }
    default:
      break;
  }
}

static void HandleDialogKeyEvent(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      KeyDialogPress(context,event,targets);
      break;
    }
    case KeyRelease:
    {
      KeyDialogRelease(context,event,targets);
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      MoveDialogButtons(context,event,targets);
      break;
    }
    default:
    {
      HandleDialogSelectionEvent(context,event,targets);
      break;
    }
  }
}

static void HandleDialogEvent(const DialogContext *context,
  const XEvent event,const DialogTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case ButtonPress:
    {
      PressDialogButton(context,event,targets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseDialogButtons(context,event,targets);
      break;
    }
    case ClientMessage:
    {
      ClientDialogMessage(context,event,targets);
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
      HandleDialogKeyEvent(context,event,targets);
      break;
    }
  }
}

typedef struct _DialogLayout
{
  Display
    *display;

  XFontStruct
    *font_info;

  unsigned int
    width,
    height;

  const char
    *query;

  char
    *reply;
} DialogLayout;

static void PositionDialogWidget(XWindows *windows,
  const DialogLayout *layout)
{
  Display
    *const display = layout->display;

  XFontStruct
    *const font_info = layout->font_info;

  const unsigned int
    width = layout->width,
    height = layout->height;

  const char
    *const query = layout->query;

  char
    *const reply = layout->reply;

  /*
    Position Dialog widget.
  */
  windows->widget.width=(unsigned int) MagickMax((int) (2*width),(int)
    WidgetTextWidth(font_info,(char *) query));
  if (windows->widget.width < WidgetTextWidth(font_info,reply))
    windows->widget.width=WidgetTextWidth(font_info,reply);
  windows->widget.width+=(unsigned int) (6*QuantumMargin);
  windows->widget.min_width=(unsigned int)
    ((int) width+28*XTextWidth(font_info,"#",1)+4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (7*(int) height+(QuantumMargin << 1));
  windows->widget.min_height=windows->widget.height;
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
}

static void SetDialogWidgetName(Display *display,XWindows *windows)
{
  Status
    status;

  XTextProperty
    window_name;

  (void) CopyMagickString(windows->widget.name,"Dialog",MagickPathExtent);
  status=XStringListToTextProperty(&windows->widget.name,1,&window_name);
  if (status != False)
    {
      XSetWMName(display,windows->widget.id,&window_name);
      XSetWMIconName(display,windows->widget.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
}

static void MapDialogWidget(Display *display,XWindows *windows)
{
  XWindowChanges
    window_changes;

  SetDialogWidgetName(display,windows);
  window_changes.width=(int) windows->widget.width;
  window_changes.height=(int) windows->widget.height;
  window_changes.x=windows->widget.x;
  window_changes.y=windows->widget.y;
  (void) XReconfigureWMWindow(display,windows->widget.id,windows->widget.screen,
    (unsigned int) (CWWidth | CWHeight | CWX | CWY),&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

static void RespondToDialogEvents(const DialogContext *context,
  const DialogTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  XEvent
    event;

  do
  {
    UpdateDialogLayout(context,targets);
    RedrawDialogWidget(context,targets);
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    HandleDialogEvent(context,event,targets);
  } while (((*state) & ExitState) == 0);
}

static unsigned int GetDialogButtonWidth(XFontStruct *font_info,
  const char *action)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  width+=(unsigned int) (3*QuantumMargin) >> 1;
  return(width);
}

static MagickBooleanType IsDialogOptionAction(const char *action)
{
  return(((LocaleCompare(action,"Background") == 0) ||
    (LocaleCompare(action,"New") == 0) ||
    (LocaleCompare(action,"Quantize") == 0) ||
    (LocaleCompare(action,"Resize") == 0) ||
    (LocaleCompare(action,"Save") == 0) ||
    (LocaleCompare(action,"Shade") == 0)) ? MagickTrue : MagickFalse);
}

MagickPrivate int XDialogWidget(Display *display,XWindows *windows,
  const char *action,const char *query,char *reply)
{
  char
    primary_selection[MagickPathExtent];

  static MagickBooleanType
    raised = MagickFalse;

  unsigned int
    anomaly,
    height,
    width;

  size_t
    state;

  XFontStruct
    *font_info;

  XWidgetInfo
    action_info,
    cancel_info,
    reply_info,
    special_info,
    text_info;

  const DialogTargets
    targets = { &special_info, &action_info, &cancel_info, &reply_info,
      &state, &text_info };

  DialogContext
    context;

  /*
    Determine Dialog widget attributes.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(query != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XCheckRefreshWindows(display,windows);
  font_info=windows->widget.font_info;
  width=GetDialogButtonWidth(font_info,action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  {
    const DialogLayout
      layout = { display, font_info, width, height, query, reply };

    PositionDialogWidget(windows,&layout);
  }
  /*
    Map Dialog widget.
  */
  MapDialogWidget(display,windows);
  /*
    Respond to X events.
  */
  anomaly=(IsDialogOptionAction(action) != MagickFalse);
  state=UpdateConfigurationState;
  XSetCursorState(display,windows,MagickTrue);
  context.display=display;
  context.windows=windows;
  context.font_info=font_info;
  context.anomaly=anomaly;
  context.primary_selection=primary_selection;
  context.action=action;
  context.query=query;
  context.height=height;
  context.raised=raised;
  context.reply=reply;
  context.width=width;
  RespondToDialogEvents(&context,&targets);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  if ((anomaly) && (special_info.raised))
    if (*reply != '\0')
      raised=MagickTrue;
  return(raised == MagickFalse);
}
#endif
