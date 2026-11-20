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
%                  W   W  IIIII  DDDD    GGGG  EEEEE    T                     %
%                                                                             %
%                                                                             %
%                   MagickCore X11 User Interface Methods                     %
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
  Variable declarations.
*/
static XWidgetInfo
  monitor_info =
  {
    (char *) NULL, (char *) NULL, (char *) NULL, 0, 0, 0, 0, 0, 0, 0, 0,
    MagickFalse, MagickFalse, MagickFalse, MagickFalse, MagickFalse
  },
  submenu_info =
  {
    (char *) NULL, (char *) NULL, (char *) NULL, 0, 0, 0, 0, 0, 0, 0, 0,
    MagickFalse, MagickFalse, MagickFalse, MagickFalse, MagickFalse
  },
  *selection_info = (XWidgetInfo *) NULL,
  toggle_info =
  {
    (char *) NULL, (char *) NULL, (char *) NULL, 0, 0, 0, 0, 0, 0, 0, 0,
    MagickFalse, MagickFalse, MagickFalse, MagickFalse, MagickFalse
  };

MagickPrivate XWidgetInfo *SubmenuInfoReference(void)
{
  return(&submenu_info);
}

MagickPrivate XWidgetInfo *ToggleInfoReference(void)
{
  return(&toggle_info);
}

MagickPrivate XWidgetInfo **SelectionInfoReference(void)
{
  return(&selection_info);
}

static char
  release_key_command[MagickPathExtent];

static KeySym
  release_key_symbol;

MagickPrivate char (*ReleaseKeyCommandReference(void))[MagickPathExtent]
{
  return(&release_key_command);
}

MagickPrivate KeySym *ReleaseKeySymbolReference(void)
{
  return(&release_key_symbol);
}

/*
  Constant declarations.
*/
MagickPrivate const int *BorderOffsetReference(void)
{
  return(&BorderOffset);
}

MagickPrivate const int *DoubleClickReference(void)
{
  return(&DoubleClick);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y X W i d g e t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyXWidget() destroys resources associated with the X widget.
%
%  The format of the DestroyXWidget method is:
%
%      void DestroyXWidget()
%
%  A description of each parameter follows:
%
*/
MagickPrivate void DestroyXWidget(void)
{
  if (selection_info != (XWidgetInfo *) NULL)
    selection_info=(XWidgetInfo *) RelinquishMagickMemory(selection_info);
}













/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S c r e e n E v e n t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XScreenEvent() returns MagickTrue if the any event on the X server queue is
%  associated with the widget window.
%
%  The format of the XScreenEvent function is:
%
%      int XScreenEvent(Display *display,XEvent *event,char *data)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o event: Specifies a pointer to a X11 XEvent structure.
%
%    o data: Specifies a pointer to a XWindows structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif










/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X I n f o W i d g e t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XInfoWidget() displays text in the Info widget.  The purpose is to inform
%  the user that what activity is currently being performed (e.g. reading
%  an image, rotating an image, etc.).
%
%  The format of the XInfoWidget method is:
%
%      void XInfoWidget(Display *display,XWindows *windows,const char *activity)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o activity: This character string reflects the current activity and is
%      displayed in the Info widget.
%
*/
MagickPrivate void XInfoWidget(Display *display,XWindows *windows,
  const char *activity)
{
  unsigned int
    height,
    margin,
    width;

  XFontStruct
    *font_info;

  XWindowChanges
    window_changes;

  /*
    Map Info widget.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(activity != (char *) NULL);
  font_info=windows->info.font_info;
  width=WidgetTextWidth(font_info,(char *) activity)+(unsigned int)
    ((3*QuantumMargin) >> 1)+4;
  height=(unsigned int) (((6*(font_info->ascent+font_info->descent)) >> 2)+4);
  if ((windows->info.width != width) || (windows->info.height != height))
    {
      /*
        Size Info widget to accommodate the activity text.
      */
      windows->info.width=width;
      windows->info.height=height;
      window_changes.width=(int) width;
      window_changes.height=(int) height;
      (void) XReconfigureWMWindow(display,windows->info.id,windows->info.screen,
        (unsigned int) (CWWidth | CWHeight),&window_changes);
    }
  if (windows->info.mapped == MagickFalse)
    {
      (void) XMapRaised(display,windows->info.id);
      windows->info.mapped=MagickTrue;
    }
  /*
    Initialize Info matte information.
  */
  height=(unsigned int) (font_info->ascent+font_info->descent);
  XGetWidgetInfo(activity,&monitor_info);
  monitor_info.bevel_width--;
  margin=monitor_info.bevel_width+((windows->info.height-height) >> 1)-2;
  monitor_info.center=MagickFalse;
  monitor_info.x=(int) margin;
  monitor_info.y=(int) margin;
  monitor_info.width=windows->info.width-(margin << 1);
  monitor_info.height=windows->info.height-(margin << 1)+1;
  /*
    Draw Info widget.
  */
  monitor_info.raised=MagickFalse;
  XDrawBeveledMatte(display,&windows->info,&monitor_info);
  monitor_info.raised=MagickTrue;
  XDrawWidgetText(display,&windows->info,&monitor_info);
}



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

MagickPrivate void XNoticeWidget(Display *display,XWindows *windows,
  const char *reason,const char *description)
{
  time_t
    timer;

  unsigned int
    height,
    width;

  size_t
    state;

  XFontStruct
    *font_info;

  XWidgetInfo
    dismiss_info;

  const NoticeTargets
    targets = { &dismiss_info, &state };

  /*
    Determine Notice widget attributes.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(reason != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",reason);
  XDelay(display,SuspendTime << 3);  /* avoid surprise with delay */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  font_info=windows->widget.font_info;
  width=GetNoticeReasonWidth(font_info,reason);
  width=GetNoticeDescriptionWidth(font_info,description,width);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position Notice widget.
  */
  PositionNoticeWidth(windows,font_info,width);
  PositionNoticeHeight(display,windows,height);
  /*
    Map Notice widget.
  */
  MapNoticeWidget(display,windows);
  (void) XBell(display,0);
  /*
    Respond to X events.
  */
  timer=GetMagickTime()+Timeout;
  state=UpdateConfigurationState;
  do
  {
    if (GetMagickTime() > timer)
      break;
    if (state & UpdateConfigurationState)
      {
        /*
          Initialize Dismiss button information.
        */
        InitializeNoticeDismiss(windows,font_info,height,&dismiss_info);
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        const NoticeText
          notice = { display, windows, font_info, height, reason,
            description };

        /*
          Redraw Notice widget.
        */
        DrawNoticeText(&notice);
        XDrawBeveledButton(display,&windows->widget,&dismiss_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        state&=(unsigned int) (~RedrawWidgetState);
      }
    DispatchNoticeEvent(display,windows,targets);
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X P r o g r e s s M o n i t o r W i d g e t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XProgressMonitorWidget() displays the progress a task is making in
%  completing a task.  A span of zero toggles the active status.  An inactive
%  state disables the progress monitor.
%
%  The format of the XProgressMonitorWidget method is:
%
%      void XProgressMonitorWidget(Display *display,XWindows *windows,
%        const char *task,const XProgressMonitorSpan *progress)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o task: Identifies the task in progress.
%
%    o progress: the progress made, in these members:
%
%      o offset: Specifies the offset position within the span which represents
%        how much progress has been made in completing a task.
%
%      o span: Specifies the span relative to completing a task.
%
*/
static void XRefreshExposedWindows(Display *display,XWindows *windows)
{
  XEvent
    event;

  while (XCheckTypedWindowEvent(display,windows->command.id,Expose,&event))
    (void) XCommandWidget(display,windows,(const char *const *) NULL,&event);
  while (XCheckTypedWindowEvent(display,windows->image.id,Expose,&event))
    XRefreshWindow(display,&windows->image,&event);
  while (XCheckTypedWindowEvent(display,windows->info.id,Expose,&event))
    if (monitor_info.text != (char *) NULL)
      XInfoWidget(display,windows,monitor_info.text);
}

MagickPrivate void XProgressMonitorWidget(Display *display,XWindows *windows,
  const char *task,const XProgressMonitorSpan *progress)
{
  const MagickOffsetType
    offset = progress->offset;

  const MagickSizeType
    span = progress->span;

  unsigned int
    width;

  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(task != (const char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",task);
  if (span == 0)
    return;
  /*
    Update image windows if there is a pending expose event.
  */
  XRefreshExposedWindows(display,windows);
  /*
    Draw progress monitor bar to represent percent completion of a task.
  */
  if ((windows->info.mapped == MagickFalse) || (task != monitor_info.text))
    XInfoWidget(display,windows,task);
  width=(unsigned int) (((offset+1)*((int) windows->info.width-
    (2*monitor_info.x)))/(int) span);
  if (width < monitor_info.width)
    {
      monitor_info.raised=MagickTrue;
      XDrawWidgetText(display,&windows->info,&monitor_info);
      monitor_info.raised=MagickFalse;
    }
  monitor_info.width=width;
  XDrawWidgetText(display,&windows->info,&monitor_info);
  (void) XFlush(display);
}

#endif
