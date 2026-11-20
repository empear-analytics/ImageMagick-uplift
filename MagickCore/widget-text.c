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
#include "MagickCore/widget-text-view-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

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
%      void XTextViewWidget(const XHelpView *view,const char *title,
%        const char **textlist)
%
%  A description of each parameter follows:
%
%    o view: where and how the text is shown, in these members:
%
%      o display: Specifies a connection to an X server;  returned from
%        XOpenDisplay.
%
%      o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%      o window: Specifies a pointer to a XWindows structure.
%
%      o mono:  Use mono-spaced font when displaying text.
%
%    o title: This character string is displayed at the top of the widget
%      window.
%
%    o textlist: This string list is displayed within the Text View widget.
%
*/
MagickPrivate void XTextViewWidget(const XHelpView *view,const char *title,
  const char **textlist)
{
  Display
    *const display = view->display;

  XWindows
    *const windows = view->windows;

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
  assert(view->resource_info != (XResourceInfo *) NULL);
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
  text_info=SelectTextViewFont(display,view->resource_info,windows,view->mono);
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
