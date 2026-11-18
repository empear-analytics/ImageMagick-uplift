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

  MagickCore private methods of widget.c: the notice widget, the run between
  XMenuWidget and XNoticeWidget; included by that file only.
*/
#ifndef MAGICKCORE_WIDGET_NOTICE_PRIVATE_H
#define MAGICKCORE_WIDGET_NOTICE_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X N o t i c e W i d g e t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XNoticeWidget() displays a Notice widget with a notice to the user.  The
%  function returns when the user presses the "Dismiss" button.
%
%  The format of the XNoticeWidget method is:
%
%      void XNoticeWidget(Display *display,XWindows *windows,
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
typedef struct _NoticeTargets
{
  XWidgetInfo
    *dismiss_info;

  size_t
    *state;
} NoticeTargets;

static void ReleaseNoticeDismiss(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets *targets)
{
  XWidgetInfo
    *const dismiss_info = targets->dismiss_info;

  size_t
    *const state = targets->state;

  if ((*dismiss_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == windows->widget.id) &&
          (MatteIsActive((*dismiss_info),event.xbutton)))
        (*state)|=ExitState;
      (*dismiss_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
    }
}

static void PressNoticeDismiss(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets *targets)
{
  XWidgetInfo
    *const dismiss_info = targets->dismiss_info;

  if (MatteIsActive((*dismiss_info),event.xbutton))
    {
      /*
        User pressed Dismiss button.
      */
      (*dismiss_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      return;
    }
}

static void KeyNoticeDismiss(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets *targets)
{
  XWidgetInfo
    *const dismiss_info = targets->dismiss_info;

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
      (*dismiss_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,dismiss_info);
      (*state)|=ExitState;
      return;
    }
}

static void MoveNoticeDismiss(Display *display,XWindows *windows,XEvent event,
  const NoticeTargets *targets)
{
  XWidgetInfo
    *const dismiss_info = targets->dismiss_info;

  size_t
    *const state = targets->state;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*state) & InactiveWidgetState)
    return;
  if ((*dismiss_info).raised == MatteIsActive((*dismiss_info),event.xmotion))
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

#define DismissButtonText  "Dismiss"
#define Timeout  8

static unsigned int GetNoticeReasonWidth(XFontStruct *font_info,
  const char *reason)
{
  const char
    *text;

  unsigned int
    width;

  width=WidgetTextWidth(font_info,DismissButtonText);
  text=GetLocaleExceptionMessage(XServerError,reason);
  if (text != (char *) NULL)
    if (WidgetTextWidth(font_info,(char *) text) > width)
      width=WidgetTextWidth(font_info,(char *) text);
  return(width);
}

static unsigned int GetNoticeDescriptionWidth(XFontStruct *font_info,
  const char *description,unsigned int width)
{
  const char
    *text;

  if (description != (char *) NULL)
    {
      text=GetLocaleExceptionMessage(XServerError,description);
      if (text != (char *) NULL)
        if (WidgetTextWidth(font_info,(char *) text) > width)
          width=WidgetTextWidth(font_info,(char *) text);
    }
  return(width);
}

static void PositionNoticeWidth(XWindows *windows,XFontStruct *font_info,
  const unsigned int width)
{
  windows->widget.width=width+(unsigned int) (4*QuantumMargin);
  windows->widget.min_width=width+(unsigned int) QuantumMargin;
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
}

static void PositionNoticeHeight(Display *display,XWindows *windows,
  const unsigned int height)
{
  windows->widget.height=(unsigned int) (12*height);
  windows->widget.min_height=(unsigned int) (7*height);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
}

static void MapNoticeWidget(Display *display,XWindows *windows)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,"Notice",MagickPathExtent);
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

static void InitializeNoticeDismiss(XWindows *windows,XFontStruct *font_info,
  const unsigned int height,XWidgetInfo *dismiss_info)
{
  XGetWidgetInfo(DismissButtonText,dismiss_info);
  (*dismiss_info).width=(unsigned int) QuantumMargin+
    WidgetTextWidth(font_info,DismissButtonText);
  (*dismiss_info).height=(unsigned int) ((3*height) >> 1);
  (*dismiss_info).x=(int)
    ((windows->widget.width >> 1)-((*dismiss_info).width >> 1));
  (*dismiss_info).y=(int)
    (windows->widget.height-((*dismiss_info).height << 1));
}

static void ReleaseNoticeWidget(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets targets)
{
  if (windows->widget.mapped == MagickFalse)
    return;
  ReleaseNoticeDismiss(display,windows,event,&targets);
}

typedef struct _NoticeText
{
  Display
    *display;

  XWindows
    *windows;

  XFontStruct
    *font_info;

  unsigned int
    height;

  const char
    *reason,
    *description;
} NoticeText;

static void DrawNoticeText(const NoticeText *notice)
{
  Display
    *const display = notice->display;

  XWindows
    *const windows = notice->windows;

  XFontStruct
    *const font_info = notice->font_info;

  const unsigned int
    height = notice->height;

  const char
    *const reason = notice->reason,
    *const description = notice->description;

  int
    x,
    y;

  unsigned int
    width;

  width=WidgetTextWidth(font_info,(char *) reason);
  x=(int) ((windows->widget.width >> 1)-(width >> 1));
  y=(int) ((windows->widget.height >> 1)-(height << 1));
  (void) XDrawString(display,windows->widget.id,
    windows->widget.annotate_context,x,y,(char *) reason,Extent(reason));
  if (description != (char *) NULL)
    {
      width=WidgetTextWidth(font_info,(char *) description);
      x=(int) ((windows->widget.width >> 1)-(width >> 1));
      y+=(int) height;
      (void) XDrawString(display,windows->widget.id,
        windows->widget.annotate_context,x,y,(char *) description,
        Extent(description));
    }
}

static void RespondToNoticePointerEvents(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets *targets)
{
  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      KeyNoticeDismiss(display,windows,event,targets);
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      MoveNoticeDismiss(display,windows,event,targets);
      break;
    }
    default:
      break;
  }
}

static void RespondToNoticeWindowEvents(Display *display,XWindows *windows,
  const XEvent event,const NoticeTargets *targets)
{
  size_t
    *const state = targets->state;

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
      RespondToNoticePointerEvents(display,windows,event,targets);
      break;
    }
  }
}

static void DispatchNoticeEvent(Display *display,XWindows *windows,
  const NoticeTargets targets)
{
  XEvent
    event;

  /*
    Wait for next event.
  */
  if (XCheckIfEvent(display,&event,XScreenEvent,(char *) windows) == MagickFalse)
    {
      /*
        Do not block if delay > 0.
      */
      XDelay(display,SuspendTime << 2);
      return;
    }
  switch (event.type)
  {
    case ButtonPress:
    {
      PressNoticeDismiss(display,windows,event,&targets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseNoticeWidget(display,windows,event,targets);
      break;
    }
    default:
    {
      RespondToNoticeWindowEvents(display,windows,event,&targets);
      break;
    }
  }
}

#endif
