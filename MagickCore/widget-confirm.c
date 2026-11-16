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
%                CCCC   OOO   N   N  FFFFF  IIIII  RRRR   M   M               %
%               C      O   O  NN  N  F        I    R   R  MM MM               %
%               C      O   O  N N N  FFF      I    RRRR   M M M               %
%               C      O   O  N  NN  F        I    R R    M   M               %
%                CCCC   OOO   N   N  F      IIIII  R  R   M   M               %
%                                                                             %
%                                                                             %
%                    MagickCore X11 Confirm Widget Methods                    %
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
%   X C o n f i r m W i d g e t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XConfirmWidget() displays a Confirm widget with a notice to the user. The
%  function returns -1 if Dismiss is pressed, 0 for Cancel, and 1 for Yes.
%
%  The format of the XConfirmWidget method is:
%
%      int XConfirmWidget(Display *display,XWindows *windows,
%        const char *reason,const char *description)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o reason: Specifies the message to display before terminating the
%      program.
%
%    o description: Specifies any description to the message.
%
*/
typedef struct _ConfirmRelease
{
  XWindows
    *windows;

  XEvent
    event;

  int
    *confirm;
} ConfirmRelease;

static size_t ReleaseConfirmChoice(const ConfirmRelease *release,
  const XWidgetInfo button_info,const int choice,size_t state)
{
  XWindows
    *const windows = release->windows;

  const XEvent
    event = release->event;

  int
    *const confirm = release->confirm;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(button_info,event.xbutton)))
    {
      (*confirm)=choice;
      state|=ExitState;
    }
  return(state);
}

typedef struct _ConfirmTargets
{
  XWidgetInfo
    *cancel_info,
    *dismiss_info,
    *yes_info;

  size_t
    *state;

  int
    *confirm;
} ConfirmTargets;

static void PressConfirmButton(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info,
    *const yes_info = targets->yes_info;

  if (MatteIsActive((*cancel_info),event.xbutton))
    {
      /*
        User pressed No button.
      */
      (*cancel_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
  if (MatteIsActive((*dismiss_info),event.xbutton))
    {
      /*
        User pressed Dismiss button.
      */
      (*dismiss_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
  if (MatteIsActive((*yes_info),event.xbutton))
    {
      /*
        User pressed Yes button.
      */
      (*yes_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,yes_info);
      return;
    }
}

static void ReleaseConfirmButtons(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info,
    *const yes_info = targets->yes_info;

  size_t
    *const state = targets->state;

  int
    *const confirm = targets->confirm;

  ConfirmRelease
    release;

  if (windows->widget.mapped == MagickFalse)
    return;
  release.windows=windows;
  release.event=event;
  release.confirm=(confirm);
  if ((*cancel_info).raised == MagickFalse)
    {
      (*state)=ReleaseConfirmChoice(&release,(*cancel_info),0,(*state));
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
    }
  if ((*dismiss_info).raised == MagickFalse)
    {
      (*state)=ReleaseConfirmChoice(&release,(*dismiss_info),(-1),(*state));
      (*dismiss_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
    }
  if ((*yes_info).raised == MagickFalse)
    {
      (*state)=ReleaseConfirmChoice(&release,(*yes_info),1,(*state));
      (*yes_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,yes_info);
    }
}

static void KeyConfirmYes(Display *display,XWindows *windows,const XEvent event,
  const ConfirmTargets *targets)
{
  XWidgetInfo
    *const yes_info = targets->yes_info;

  int
    *const confirm = targets->confirm;

  size_t
    *const state = targets->state;

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
      (*yes_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,yes_info);
      (*confirm)=1;
      (*state)|=ExitState;
      return;
    }
}

static void MoveConfirmYes(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  XWidgetInfo
    *const yes_info = targets->yes_info;

  if ((*yes_info).raised == MatteIsActive((*yes_info),event.xmotion))
    {
      /*
        Yes button status changed.
      */
      (*yes_info).raised=(*yes_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,yes_info);
      return;
    }
}

static void MoveConfirmButtons(Display *display,XWindows *windows,XEvent event,
  const ConfirmTargets *targets)
{
  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info;

  size_t
    *const state = targets->state;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*state) & InactiveWidgetState)
    return;
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
  if ((*dismiss_info).raised == MatteIsActive((*dismiss_info),event.xmotion))
    {
      /*
        Dismiss button status changed.
      */
      (*dismiss_info).raised=(*dismiss_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
  MoveConfirmYes(display,windows,event,targets);
}

#define CancelButtonText  "Cancel"
#define DismissButtonText  "Dismiss"
#define YesButtonText  "Yes"

typedef struct _ConfirmContext
{
  XWindows
    *windows;

  XFontStruct
    *font_info;

  unsigned int
    height;

  const char
    *description;

  Display
    *display;

  const char
    *reason;
} ConfirmContext;

static void SetConfirmYesButton(XFontStruct *font_info,
  const char *description,const XWidgetInfo *cancel_info,
  XWidgetInfo *yes_info)
{
  (*yes_info)=(*cancel_info);
  (*yes_info).text=(char *) YesButtonText;
  if (LocaleCompare(description,"Do you want to save it") == 0)
    (*yes_info).text=(char *) "Save";
  (*yes_info).width=(unsigned int) QuantumMargin+
    WidgetTextWidth(font_info,(*yes_info).text);
  if ((*yes_info).width < (*cancel_info).width)
    (*yes_info).width=(*cancel_info).width;
  (*yes_info).x=QuantumMargin;
}

static void UpdateConfirmButtons(const ConfirmContext *context,
  const ConfirmTargets *targets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  const char
    *const description = context->description;

  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info,
    *const yes_info = targets->yes_info;

  size_t
    *const state = targets->state;

  if ((*state) & UpdateConfigurationState)
    {
      /*
        Initialize button information.
      */
      XGetWidgetInfo(CancelButtonText,cancel_info);
      (*cancel_info).width=(unsigned int) QuantumMargin+
        WidgetTextWidth(font_info,CancelButtonText);
      (*cancel_info).height=(unsigned int) ((3*height) >> 1);
      (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
        QuantumMargin;
      (*cancel_info).y=(int) (windows->widget.height-
        ((*cancel_info).height << 1));
      (*dismiss_info)=(*cancel_info);
      (*dismiss_info).text=(char *) DismissButtonText;
      if (LocaleCompare(description,"Do you want to save it") == 0)
        (*dismiss_info).text=(char *) "Don't Save";
      (*dismiss_info).width=(unsigned int) QuantumMargin+
        WidgetTextWidth(font_info,(*dismiss_info).text);
      (*dismiss_info).x=(int)
        ((windows->widget.width >> 1)-((*dismiss_info).width >> 1));
      SetConfirmYesButton(font_info,description,cancel_info,yes_info);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawConfirmWidget(const ConfirmContext *context,
  const ConfirmTargets *targets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  const char
    *const description = context->description;

  Display
    *const display = context->display;

  const char
    *const reason = context->reason;

  XWidgetInfo
    *const cancel_info = targets->cancel_info,
    *const dismiss_info = targets->dismiss_info,
    *const yes_info = targets->yes_info;

  size_t
    *const state = targets->state;

  int
    x,
    y;

  unsigned int
    width;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Confirm widget.
      */
      width=WidgetTextWidth(font_info,(char *) reason);
      x=(int) ((windows->widget.width >> 1)-(width >> 1));
      y=(int) ((windows->widget.height >> 1)-(height << 1));
      (void) XDrawString(display,windows->widget.id,
        windows->widget.annotate_context,x,y,(char *) reason,Extent(reason));
      if (description != (char *) NULL)
        {
          char
            question[MagickPathExtent];

          (void) CopyMagickString(question,description,MagickPathExtent);
          (void) ConcatenateMagickString(question,"?",MagickPathExtent);
          width=WidgetTextWidth(font_info,question);
          x=((int) (windows->widget.width >> 1)-(int) (width >> 1));
          y+=(int) height;
          (void) XDrawString(display,windows->widget.id,
            windows->widget.annotate_context,x,y,question,Extent(question));
        }
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      XDrawBeveledButton(display,&windows->widget,yes_info);
      XHighlightWidget(display,&windows->widget,(*BorderOffsetReference()),(*BorderOffsetReference()));
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void HandleConfirmKeyEvent(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      KeyConfirmYes(display,windows,event,targets);
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      MoveConfirmButtons(display,windows,event,targets);
      break;
    }
    default:
      break;
  }
}

static void HandleConfirmEvent(Display *display,XWindows *windows,
  const XEvent event,const ConfirmTargets *targets)
{
  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case ButtonPress:
    {
      PressConfirmButton(display,windows,event,targets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseConfirmButtons(display,windows,event,targets);
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
      HandleConfirmKeyEvent(display,windows,event,targets);
      break;
    }
  }
}

static void MapConfirmWidget(Display *display,XWindows *windows)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,"Confirm",MagickPathExtent);
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

typedef struct _ConfirmLayout
{
  Display
    *display;

  XFontStruct
    *font_info;

  unsigned int
    width,
    height;
} ConfirmLayout;

static void PositionConfirmWidget(XWindows *windows,
  const ConfirmLayout *layout)
{
  Display
    *const display = layout->display;

  XFontStruct
    *const font_info = layout->font_info;

  const unsigned int
    width = layout->width,
    height = layout->height;

  /*
    Position Confirm widget.
  */
  windows->widget.width=(unsigned int) ((int) width+9*QuantumMargin);
  windows->widget.min_width=9*(unsigned int) QuantumMargin+
    WidgetTextWidth(font_info,CancelButtonText)+
    WidgetTextWidth(font_info,DismissButtonText)+
    WidgetTextWidth(font_info,YesButtonText);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (12*height);
  windows->widget.min_height=(unsigned int) (7*height);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
}

static unsigned int GetConfirmTextWidth(XFontStruct *font_info,
  const char *description)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,DismissButtonText) > width)
    width=WidgetTextWidth(font_info,DismissButtonText);
  if (WidgetTextWidth(font_info,YesButtonText) > width)
    width=WidgetTextWidth(font_info,YesButtonText);
  width<<=1;
  if (description != (char *) NULL)
    if (WidgetTextWidth(font_info,(char *) description) > width)
      width=WidgetTextWidth(font_info,(char *) description);
  return(width);
}

MagickPrivate int XConfirmWidget(Display *display,XWindows *windows,
  const char *reason,const char *description)
{
  int
    confirm;

  unsigned int
    height,
    width;

  size_t
    state;

  XEvent
    event;

  XFontStruct
    *font_info;

  XWidgetInfo
    cancel_info,
    dismiss_info,
    yes_info;

  const ConfirmTargets
    targets = { &cancel_info, &dismiss_info, &yes_info, &state, &confirm };

  ConfirmContext
    context;

  ConfirmLayout
    layout;
  /*
    Determine Confirm widget attributes.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(reason != (char *) NULL);
  assert(description != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",reason);
  XCheckRefreshWindows(display,windows);
  font_info=windows->widget.font_info;
  width=GetConfirmTextWidth(font_info,description);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  layout.display=display;
  layout.font_info=font_info;
  layout.width=width;
  layout.height=height;
  PositionConfirmWidget(windows,&layout);
  /*
    Map Confirm widget.
  */
  MapConfirmWidget(display,windows);
  /*
    Respond to X events.
  */
  confirm=0;
  state=UpdateConfigurationState;
  XSetCursorState(display,windows,MagickTrue);
  context.windows=windows;
  context.font_info=font_info;
  context.height=height;
  context.description=description;
  context.display=display;
  context.reason=reason;
  do
  {
    UpdateConfirmButtons(&context,&targets);
    RedrawConfirmWidget(&context,&targets);
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    HandleConfirmEvent(display,windows,event,&targets);
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  return(confirm);
}
#endif
