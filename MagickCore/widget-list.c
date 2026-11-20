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
%                         L      IIIII  SSSSS  TTTTT                          %
%                         L        I    SS       T                            %
%                         L        I     SSS     T                            %
%                         L        I       SS    T                            %
%                         LLLLL  IIIII  SSSSS    T                            %
%                                                                             %
%                                                                             %
%                 MagickCore X11 List Browser Widget Methods                  %
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
#include "MagickCore/widget-list-entries-private.h"
#include "MagickCore/widget-list-actions-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

static void RedrawListBrowserWidget(const ListBrowserContext *context,
  const ListBrowserWidgets *widgets)
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

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  XWidgetInfo
    *const list_info = widgets->list_info;

  XWidgetInfo
    *const text_info = widgets->text_info;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw List Browser window.
      */
      XDrawWidgetText(display,window_info,text_info);
      XDrawBeveledMatte(display,window_info,list_info);
      XDrawBeveledMatte(display,window_info,scroll_info);
      XDrawTriangleNorth(display,window_info,north_info);
      XDrawBeveledButton(display,window_info,slider_info);
      XDrawTriangleSouth(display,window_info,south_info);
      XDrawBeveledMatte(display,window_info,reply_info);
      XDrawMatteText(display,window_info,reply_info);
      XDrawBeveledButton(display,window_info,action_info);
      XDrawBeveledButton(display,window_info,cancel_info);
      XHighlightWidget(display,window_info,BorderOffset,BorderOffset);
      (*selection_info).id=(~0);
      (*state)|=RedrawActionState;
      (*state)|=RedrawListState;
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void RedrawListBrowserEntries(const ListBrowserContext *context,
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

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  unsigned int
    *const visible_entries = widgets->visible_entries;

  XWidgetInfo
    *const list_info = widgets->list_info;

  XWidgetInfo
    *const expose_info = widgets->expose_info;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  if ((*state) & RedrawListState)
    {
      /*
        Determine slider id and position.
      */
      if ((*slider_info).id >= (int) (entries-(*visible_entries)))
        (*slider_info).id=(int) (entries-(*visible_entries));
      if (((*slider_info).id < 0) || (entries <= (*visible_entries)))
        (*slider_info).id=0;
      (*slider_info).y=(*slider_info).min_y;
      if (entries > 0)
        (*slider_info).y+=(*slider_info).id*((*slider_info).max_y-
          (*slider_info).min_y+1)/(int) entries;
      if ((*slider_info).id != (*selection_info).id)
        {
          ListBrowserRedraw
            redraw;

          redraw.display=display;
          redraw.window_info=window_info;
          redraw.list=list;
          redraw.entries=entries;
          redraw.height=height;
          redraw.visible_entries=(*visible_entries);
          redraw.list_info=(list_info);
          redraw.north_info=(*north_info);
          redraw.slider_info=(*slider_info);
          redraw.south_info=(*south_info);
          RedrawListBrowserList(&redraw,selection_info,expose_info);
        }
      (*state)&=(unsigned int) (~RedrawListState);
    }
}

static MagickBooleanType WaitForListBrowserEvent(
  const ListBrowserContext *context,const ListBrowserWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    entries = context->entries;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  size_t
    *const delay = widgets->delay;

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
          ((*slider_info).id < (int) entries))
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

static unsigned int GetListBrowserWidth(XFontStruct *font_info,
  const char *action)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  width+=(unsigned int) QuantumMargin;
  return(width);
}

static void PositionListBrowserWidget(XWindowInfo *window_info,
  XFontStruct *font_info,unsigned int text_width,unsigned int height)
{
  window_info->width=MagickMin(text_width,MaxTextWidth)+(unsigned int)
    ((9*QuantumMargin) >> 1);
  window_info->min_width=(MinTextWidth+4*(unsigned int) QuantumMargin);
  if (window_info->width < window_info->min_width)
    window_info->width=window_info->min_width;
  window_info->height=(((81*height) >> 2)+(unsigned int)
    ((13*QuantumMargin) >> 1)+4);
  window_info->min_height=(((23*height) >> 1)+(unsigned int)
    ((13*QuantumMargin) >> 1)+4);
  if (window_info->height < window_info->min_height)
    window_info->height=window_info->min_height;
}

static void MapListBrowserWidget(Display *display,XWindows *windows,
  XWindowInfo *window_info,MagickStatusType mask)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(window_info->name,"Browse",MagickPathExtent);
  status=XStringListToTextProperty(&window_info->name,1,&window_name);
  if (status != False)
    {
      XSetWMName(display,window_info->id,&window_name);
      XSetWMIconName(display,windows->widget.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  window_changes.width=(int) window_info->width;
  window_changes.height=(int) window_info->height;
  window_changes.x=window_info->x;
  window_changes.y=window_info->y;
  (void) XReconfigureWMWindow(display,window_info->id,window_info->screen,mask,
    &window_changes);
  (void) XMapRaised(display,window_info->id);
  window_info->mapped=MagickFalse;
}

static void WithdrawListBrowserWidget(Display *display,XWindows *windows,
  XWindowInfo *window_info)
{
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,window_info->id,window_info->screen);
  XCheckRefreshWindows(display,windows);
}

static void RespondToListBrowserEvents(const ListBrowserContext *context,
  const ListBrowserTargets *targets,const ListBrowserWidgets *widgets)
{
  size_t
    *const state = widgets->state;

  XEvent
    *const event = widgets->event;

  do
  {
    UpdateListBrowserLayout(context,widgets);
    RedrawListBrowserWidget(context,widgets);
    RedrawListBrowserEntries(context,widgets);
    if (WaitForListBrowserEvent(context,widgets) == MagickFalse)
      continue;
    switch ((*event).type)
    {
      case ButtonPress:
      {
        PressListBrowserSlider(context,(*event),targets,widgets);
        break;
      }
      default:
      {
        HandleListBrowserWindowEvent(context,(*event),targets,widgets);
        break;
      }
    }
  } while (((*state) & ExitState) == 0);
}

static void RunListBrowser(const ListBrowserContext *context,
  MagickStatusType *mask)
{
  unsigned int
    visible_entries;

  size_t
    delay,
    state;

  XEvent
    event;

  XWidgetInfo
    action_info,
    cancel_info,
    expose_info,
    list_info,
    north_info,
    reply_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info;

  const ListBrowserTargets
    targets = { &reply_info, &selection_info, &action_info, &state,
      &list_info };

  const ListBrowserWidgets
    list_widgets = { &reply_info, &state, &slider_info, &action_info,
      &cancel_info, &north_info, &south_info, &delay, &scroll_info,
      &visible_entries, &event, &list_info, mask, &expose_info, &text_info,
      &selection_info };

  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_entries=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  RespondToListBrowserEvents(context,&targets,&list_widgets);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X L i s t B r o w s e r W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XListBrowserWidget() displays a List Browser widget with a query to the
%  user.  The user keys a reply or select a reply from the list.  Finally, the
%  user presses the Action or Cancel button to exit.  The typed text is
%  returned as the reply function parameter.
%
%  The format of the XListBrowserWidget method is:
%
%      void XListBrowserWidget(Display *display,XWindows *windows,
%        XWindowInfo *window_info,const char *const *list,const char *action,
%        const char *query,char *reply)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o list: Specifies a pointer to an array of strings.  The user can
%      select from these strings as a possible reply value.
%
%    o action: Specifies a pointer to the action of this widget.
%
%    o query: Specifies a pointer to the query to present to the user.
%
%    o reply: the response from the user is returned in this parameter.
%
*/
MagickPrivate void XListBrowserWidget(Display *display,XWindows *windows,
  XWindowInfo *window_info,const XListBrowserRequest *request)
{
  char
    primary_selection[MagickPathExtent];

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  unsigned int
    entries,
    height,
    text_width,
    width;

  XFontStruct
    *font_info;

  ListBrowserContext
    list_context;

  /*
    Count the number of entries in the list.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(window_info != (XWindowInfo *) NULL);
  assert(request->list != (const char **) NULL);
  assert(request->action != (char *) NULL);
  assert(request->query != (char *) NULL);
  assert(request->reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",request->action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (request->list == (const char **) NULL)
    {
      XNoticeWidget(display,windows,"No text to browse:",(char *) NULL);
      return;
    }
  entries=CountListBrowserEntries(request->list);
  /*
    Determine Font Browser widget attributes.
  */
  font_info=window_info->font_info;
  text_width=WidgetTextWidth(font_info,(char *) request->query);
  text_width=WidestListBrowserEntry(font_info,request->list,entries,text_width);
  width=GetListBrowserWidth(font_info,request->action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position List Browser widget.
  */
  PositionListBrowserWidget(window_info,font_info,text_width,height);
  XConstrainWindowPosition(display,window_info);
  /*
    Map List Browser widget.
  */
  MapListBrowserWidget(display,windows,window_info,mask);
  /*
    Respond to X events.
  */
  list_context.display=display;
  list_context.windows=windows;
  list_context.window_info=window_info;
  list_context.entries=entries;
  list_context.primary_selection=primary_selection;
  list_context.font_info=font_info;
  list_context.list=request->list;
  list_context.height=height;
  list_context.action=request->action;
  list_context.width=width;
  list_context.reply=request->reply;
  list_context.query=request->query;
  RunListBrowser(&list_context,&mask);
  WithdrawListBrowserWidget(display,windows,window_info);
}
#endif
