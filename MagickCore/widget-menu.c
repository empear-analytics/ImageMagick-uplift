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
%                         M   M  EEEEE  N   N  U   U                          %
%                         MM MM  E      NN  N  U   U                          %
%                         M M M  EEE    N N N  U   U                          %
%                         M   M  E      N  NN  U   U                          %
%                         M   M  EEEEE  N   N   UUU                           %
%                                                                             %
%                                                                             %
%                     MagickCore X11 Menu Widget Methods                      %
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
  Define declarations.
*/
#define WindowIsActive(window_info,position)  ( \
  ((position.x >= 0) && (position.y >= 0) &&  \
   (position.x < (int) window_info.width) &&  \
   (position.y < (int) window_info.height)) ? MagickTrue : MagickFalse)

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X M e n u W i d g e t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMenuWidget() maps a menu and returns the command pointed to by the user
%  when the button is released.
%
%  The format of the XMenuWidget method is:
%
%      int XMenuWidget(Display *display,XWindows *windows,const char *title,
%        const char *const *selections,char *item)
%
%  A description of each parameter follows:
%
%    o selection_number: Specifies the number of the selection that the
%      user choose.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o title: Specifies a character string that describes the menu selections.
%
%    o selections: Specifies a pointer to one or more strings that comprise
%      the choices in the menu.
%
%    o item: Specifies a character array.  The item selected from the menu
%      is returned here.
%
*/
static void PutBackMenuEvent(Display *display,XWindows *windows,
  XEvent event)
{
  if (event.xbutton.window == windows->command.id)
    (void) XPutBackEvent(display,&event);
}

typedef struct _MenuSelections
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  unsigned int
    number_selections;
} MenuSelections;

static void DrawMenuSelections(const MenuSelections *entries,
  XWidgetInfo *selection_info,XWidgetInfo *highlight_info)
{
  Display
    *const display = entries->display;

  XWindows
    *const windows = entries->windows;

  const char
    *const *const selections = entries->selections;

  const unsigned int
    number_selections = entries->number_selections;

  int
    id;

  for (id=0; id < (int) number_selections; id++)
  {
    (*selection_info).text=(char *) selections[id];
    XDrawWidgetText(display,&windows->widget,&(*selection_info));
    (*highlight_info).y=(*selection_info).y+(int) (*highlight_info).bevel_width;
    if (id == (*selection_info).id)
      XDrawBevel(display,&windows->widget,&(*highlight_info));
    (*selection_info).y+=(int) (*selection_info).height;
  }
}

typedef struct _MenuTargets
{
  XWidgetInfo
    *selection_info,
    *highlight_info;

  size_t
    *state;

  XWidgetInfo
    *menu_info;
} MenuTargets;

typedef struct _MenuPointer
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  unsigned int
    number_selections,
    top_offset;

  int
    id;
} MenuPointer;

static void EnterMenuSelection(const MenuPointer *pointer,
  const MenuTargets *targets)
{
  Display
    *const display = pointer->display;

  XWindows
    *const windows = pointer->windows;

  const char
    *const *const selections = pointer->selections;

  const unsigned int
    number_selections = pointer->number_selections,
    top_offset = pointer->top_offset;

  const int
    id = pointer->id;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  if (((*selection_info).id >= 0) &&
      ((*selection_info).id < (int) number_selections))
    {
      /*
        Unhighlight last selection.
      */
      if (id == (*selection_info).id)
        return;
      (*selection_info).y=((int) top_offset+(*selection_info).id*(int)
        (*selection_info).height);
      (*selection_info).text=(char *) selections[(*selection_info).id];
      XDrawWidgetText(display,&windows->widget,selection_info);
    }
  if ((id < 0) || (id >= (int) number_selections))
    return;
  /*
    Highlight this selection.
  */
  (*selection_info).id=id;
  (*selection_info).y=((int) top_offset+(*selection_info).id*(int)
    (*selection_info).height);
  (*selection_info).text=(char *) selections[(*selection_info).id];
  XDrawWidgetText(display,&windows->widget,selection_info);
  (*highlight_info).y=(*selection_info).y+(int) (*highlight_info).bevel_width;
  XDrawBevel(display,&windows->widget,highlight_info);
}

static void MoveMenuSelection(const MenuPointer *pointer,
  const MenuTargets *targets)
{
  Display
    *const display = pointer->display;

  XWindows
    *const windows = pointer->windows;

  const char
    *const *const selections = pointer->selections;

  const unsigned int
    number_selections = pointer->number_selections,
    top_offset = pointer->top_offset;

  const int
    id = pointer->id;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  if (((*selection_info).id >= 0) &&
      ((*selection_info).id < (int) number_selections))
    {
      /*
        Unhighlight last selection.
      */
      if (id == (*selection_info).id)
        return;
      (*selection_info).y=((int) top_offset+(*selection_info).id*(int)
        (*selection_info).height);
      (*selection_info).text=(char *) selections[(*selection_info).id];
      XDrawWidgetText(display,&windows->widget,selection_info);
    }
  (*selection_info).id=id;
  if ((id < 0) || (id >= (int) number_selections))
    return;
  /*
    Highlight this selection.
  */
  (*selection_info).y=((int) top_offset+id*(int) (*selection_info).height);
  (*selection_info).text=(char *) selections[id];
  XDrawWidgetText(display,&windows->widget,selection_info);
  (*highlight_info).y=(*selection_info).y+(int) (*highlight_info).bevel_width;
  XDrawBevel(display,&windows->widget,highlight_info);
}

typedef struct _MenuExitTargets
{
  XWidgetInfo
    *selection_info;

  size_t
    *state;
} MenuExitTargets;

static MagickBooleanType StayInMenuOnCommand(XWindows *windows,
  const XEvent event,char *item,const MenuExitTargets *targets)
{
  XWidgetInfo
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  if (((*SubmenuInfoReference()).active != 0) &&
      (event.xmotion.window == windows->command.id))
    {
      if (((*state) & InactiveWidgetState) == 0)
        {
          if (MatteIsActive((*SubmenuInfoReference()),event.xmotion) == MagickFalse)
            {
              (*selection_info).id=(~0);
              *item='\0';
              (*state)|=ExitState;
              return(MagickFalse);
            }
        }
      else
        if (WindowIsActive(windows->command,event.xmotion))
          {
            (*selection_info).id=(~0);
            *item='\0';
            (*state)|=ExitState;
            return(MagickFalse);
          }
    }
  return(MagickTrue);
}

static void MapMenuWidget(Display *display,XWindows *windows)
{
  XSetWindowAttributes
    window_attributes;

  XWindowChanges
    window_changes;

  window_attributes.override_redirect=MagickTrue;
  (void) XChangeWindowAttributes(display,windows->widget.id,
    (size_t) CWOverrideRedirect,&window_attributes);
  window_changes.width=(int) windows->widget.width;
  window_changes.height=(int) windows->widget.height;
  window_changes.x=windows->widget.x;
  window_changes.y=windows->widget.y;
  (void) XReconfigureWMWindow(display,windows->widget.id,windows->widget.screen,
    (unsigned int) (CWWidth | CWHeight | CWX | CWY),&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

typedef struct _MenuEventContext
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  unsigned int
    number_selections,
    top_offset;

  char
    *item;

  unsigned int
    height;

  const char
    *title;

  unsigned int
    title_height;
} MenuEventContext;

static void PressMenuSelection(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  char
    *const item = context->item;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  size_t
    *const state = targets->state;

  int
    id;

  if (event.xbutton.window != windows->widget.id)
    {
      /*
        exit menu.
      */
      PutBackMenuEvent(display,windows,event);
      (*selection_info).id=(~0);
      *item='\0';
      (*state)|=ExitState;
      return;
    }
  (*state)&=(unsigned int) (~InactiveWidgetState);
  if ((*selection_info).height == 0)
    return;
  id=(event.xbutton.y-(int) top_offset)/(int) (*selection_info).height;
  (*selection_info).id=id;
  if ((id < 0) || (id >= (int) number_selections))
    return;
  /*
    Highlight this selection.
  */
  (*selection_info).y=((int) top_offset+id*(int) (*selection_info).height);
  (*selection_info).text=(char *) selections[id];
  XDrawWidgetText(display,&windows->widget,selection_info);
  (*highlight_info).y=(*selection_info).y+(int) (*highlight_info).bevel_width;
  XDrawBevel(display,&windows->widget,highlight_info);
}


static void ReleaseMenuSelection(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const item = context->item;

  size_t
    *const state = targets->state;

  if (windows->widget.mapped == MagickFalse)
    return;
  if ((event.xbutton.window == windows->command.id) &&
      (((*state) & InactiveWidgetState) == 0))
    return;
  /*
    exit menu.
  */
  XSetCursorState(display,windows,MagickFalse);
  *item='\0';
  (*state)|=ExitState;
}

static void ConfigureMenuWindow(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  /*
    Update widget configuration.
  */
  if (event.xconfigure.window != windows->widget.id)
    return;
  if ((event.xconfigure.width == (int) windows->widget.width) &&
      (event.xconfigure.height == (int) windows->widget.height))
    return;
  windows->widget.width=(unsigned int)
    MagickMax(event.xconfigure.width,(int) windows->widget.min_width);
  windows->widget.height=(unsigned int)
    MagickMax(event.xconfigure.height,(int) windows->widget.min_height);
  (*state)|=UpdateConfigurationState;
}

static void ExposeMenuWindow(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  if (event.xexpose.window != windows->widget.id)
    return;
  if (event.xexpose.count != 0)
    return;
  (*state)|=RedrawWidgetState;
}

static void LeaveMenuWindow(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  int
    id;

  if (event.xcrossing.window != windows->widget.id)
    return;
  (*state)|=InactiveWidgetState;
  id=(*selection_info).id;
  if ((id < 0) || (id >= (int) number_selections))
    return;
  /*
    Unhighlight last selection.
  */
  (*selection_info).y=((int) top_offset+id*(int) (*selection_info).height);
  (*selection_info).id=(~0);
  (*selection_info).text=(char *) selections[id];
  XDrawWidgetText(display,&windows->widget,selection_info);
}

static void EnterMenuWindow(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  int
    id;

  MenuPointer
    pointer;

  if (event.xcrossing.window != windows->widget.id)
    return;
  if (event.xcrossing.state == 0)
    return;
  (*state)&=(unsigned int) (~InactiveWidgetState);
  if ((*selection_info).height == 0)
    return;
  id=((event.xcrossing.y-(int) top_offset)/(int) (*selection_info).height);
  pointer.display=display;
  pointer.windows=windows;
  pointer.selections=selections;
  pointer.number_selections=number_selections;
  pointer.top_offset=top_offset;
  pointer.id=id;
  EnterMenuSelection(&pointer,targets);
}

static void MoveMenuWindow(const MenuEventContext *context,
  XEvent event,const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  char
    *const item = context->item;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  size_t
    *const state = targets->state;

  int
    id;

  MenuExitTargets
    exit_targets;

  MenuPointer
    pointer;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  exit_targets.selection_info=(selection_info);
  exit_targets.state=(state);
  if (StayInMenuOnCommand(windows,event,item,&exit_targets) ==
      MagickFalse)
    return;
  if (event.xmotion.window != windows->widget.id)
    return;
  if ((*state) & InactiveWidgetState)
    return;
  if ((*selection_info).height == 0)
    return;
  id=(event.xmotion.y-(int) top_offset)/(int) (*selection_info).height;
  pointer.display=display;
  pointer.windows=windows;
  pointer.selections=selections;
  pointer.number_selections=number_selections;
  pointer.top_offset=top_offset;
  pointer.id=id;
  MoveMenuSelection(&pointer,targets);
}

static void UpdateMenuLayout(const MenuEventContext *context,
  const MenuTargets *targets)
{
  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const menu_info = targets->menu_info;

  if ((*state) & UpdateConfigurationState)
    {
      /*
        Initialize selection information.
      */
      XGetWidgetInfo((char *) NULL,menu_info);
      (*menu_info).bevel_width--;
      (*menu_info).width=windows->widget.width-
        (((*menu_info).bevel_width) << 1);
      (*menu_info).height=windows->widget.height-
        (((*menu_info).bevel_width) << 1);
      (*menu_info).x=(int) (*menu_info).bevel_width;
      (*menu_info).y=(int) (*menu_info).bevel_width;
      XGetWidgetInfo((char *) NULL,selection_info);
      (*selection_info).center=MagickFalse;
      (*selection_info).width=(*menu_info).width;
      (*selection_info).height=height;
      (*selection_info).x=(*menu_info).x;
      (*highlight_info)=(*selection_info);
      (*highlight_info).bevel_width--;
      (*highlight_info).width-=((*highlight_info).bevel_width << 1);
      (*highlight_info).height-=((*highlight_info).bevel_width << 1);
      (*highlight_info).x+=(int) (*highlight_info).bevel_width;
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawMenuWidget(const MenuEventContext *context,
  const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  const char
    *const title = context->title;

  const unsigned int
    title_height = context->title_height;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const highlight_info = targets->highlight_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const menu_info = targets->menu_info;

  int
    y;

  if ((*state) & RedrawWidgetState)
    {
      MenuSelections
        entries;

      /*
        Redraw Menu widget.
      */
      if ((*SubmenuInfoReference()).active == 0)
        {
          y=(int) title_height;
          XSetBevelColor(display,&windows->widget,MagickFalse);
          (void) XDrawLine(display,windows->widget.id,
            windows->widget.widget_context,(*selection_info).x,y-1,
            (int) (*selection_info).width,y-1);
          XSetBevelColor(display,&windows->widget,MagickTrue);
          (void) XDrawLine(display,windows->widget.id,
            windows->widget.widget_context,(*selection_info).x,y,
            (int) (*selection_info).width,y);
          (void) XSetFillStyle(display,windows->widget.widget_context,
            FillSolid);
        }
      /*
        Draw menu selections.
      */
      (*selection_info).center=MagickTrue;
      (*selection_info).y=(int) (*menu_info).bevel_width;
      (*selection_info).text=(char *) title;
      if ((*SubmenuInfoReference()).active == 0)
        XDrawWidgetText(display,&windows->widget,selection_info);
      (*selection_info).center=MagickFalse;
      (*selection_info).y=(int) top_offset;
      entries.display=display;
      entries.windows=windows;
      entries.selections=selections;
      entries.number_selections=number_selections;
      DrawMenuSelections(&entries,selection_info,highlight_info);
      XDrawBevel(display,&windows->widget,menu_info);
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void DrawMenuLine(const MenuEventContext *context,
  const MenuTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    number_selections = context->number_selections,
    top_offset = context->top_offset;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  int
    y;

  if (number_selections > 2)
    {
      /*
        Redraw Menu line.
      */
      y=((int) top_offset+(int) (*selection_info).height*(int)
        (number_selections-1));
      XSetBevelColor(display,&windows->widget,MagickFalse);
      (void) XDrawLine(display,windows->widget.id,
        windows->widget.widget_context,(*selection_info).x,y-1,
        (int) (*selection_info).width,y-1);
      XSetBevelColor(display,&windows->widget,MagickTrue);
      (void) XDrawLine(display,windows->widget.id,
        windows->widget.widget_context,(*selection_info).x,y,
        (int) (*selection_info).width,y);
      (void) XSetFillStyle(display,windows->widget.widget_context,FillSolid);
    }
}

static void HandleMenuPointerEvent(const MenuEventContext *context,
  const XEvent event,const MenuTargets *targets)
{
  switch (event.type)
  {
    case Expose:
    {
      ExposeMenuWindow(context,event,targets);
      break;
    }
    case LeaveNotify:
    {
      LeaveMenuWindow(context,event,targets);
      break;
    }
    case MotionNotify:
    {
      MoveMenuWindow(context,event,targets);
      break;
    }
    default:
      break;
  }
}

static void HandleMenuEvent(const MenuEventContext *context,const XEvent event,
  const MenuTargets *targets)
{
  switch (event.type)
  {
    case ButtonPress:
    {
      PressMenuSelection(context,event,targets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseMenuSelection(context,event,targets);
      break;
    }
    case ConfigureNotify:
    {
      ConfigureMenuWindow(context,event,targets);
      break;
    }
    case EnterNotify:
    {
      EnterMenuWindow(context,event,targets);
      break;
    }
    default:
    {
      HandleMenuPointerEvent(context,event,targets);
      break;
    }
  }
}

static int CloseMenuWidget(const MenuEventContext *context,
  const MenuTargets *targets,Cursor cursor)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const selections = context->selections;

  const unsigned int
    number_selections = context->number_selections;

  char
    *const item = context->item;

  XWidgetInfo
    *const selection_info = targets->selection_info;

  XSetWindowAttributes
    window_attributes;

  (void) XFreeCursor(display,cursor);
  window_attributes.override_redirect=MagickFalse;
  (void) XChangeWindowAttributes(display,windows->widget.id,
    (size_t) CWOverrideRedirect,&window_attributes);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  if ((*SubmenuInfoReference()).active != 0)
    {
      (*SubmenuInfoReference()).active=MagickFalse;
      (*ToggleInfoReference()).raised=MagickFalse;
      XDrawTriangleEast(display,&windows->command,&(*ToggleInfoReference()));
    }
  if (((*selection_info).id < 0) ||
    ((*selection_info).id >= (int) number_selections))
    return(~0);
  (void) CopyMagickString(item,selections[(*selection_info).id],
    MagickPathExtent);
  return((*selection_info).id);
}

static void RespondToMenuEvents(const MenuEventContext *context,
  const MenuTargets *targets)
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
    UpdateMenuLayout(context,targets);
    RedrawMenuWidget(context,targets);
    DrawMenuLine(context,targets);
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    HandleMenuEvent(context,event,targets);
  } while (((*state) & ExitState) == 0);
}

static void PlaceMenuWidget(Display *display,XWindows *windows,
  XFontStruct *font_info,const unsigned int title_height)
{
  int
    x,
    y;

  windows->widget.min_width=windows->widget.width;
  windows->widget.min_height=windows->widget.height;
  XQueryPosition(display,windows->widget.root,&x,&y);
  windows->widget.x=x-(int) (QuantumMargin >> 1);
  if ((*SubmenuInfoReference()).active != 0)
    {
      windows->widget.x=windows->command.x+(int) windows->command.width-
        QuantumMargin;
      (*ToggleInfoReference()).raised=MagickTrue;
      XDrawTriangleEast(display,&windows->command,&(*ToggleInfoReference()));
    }
  windows->widget.y=(*SubmenuInfoReference()).active == 0 ? y-(int) (title_height >> 2) : y;
  if ((*SubmenuInfoReference()).active != 0)
    windows->widget.y=windows->command.y+(*SubmenuInfoReference()).y;
  XConstrainWindowPosition(display,&windows->widget);
}

static Cursor DefineMenuCursor(Display *display,XWindows *windows)
{
  Cursor
    cursor;

  cursor=XCreateFontCursor(display,XC_right_ptr);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (void) XCheckDefineCursor(display,windows->command.id,cursor);
  (void) XCheckDefineCursor(display,windows->widget.id,cursor);
  return(cursor);
}

MagickPrivate int XMenuWidget(Display *display,XWindows *windows,
  const XMenuRequest *request)
{
  Cursor
    cursor;

  int
    id;

  unsigned int
    height,
    number_selections,
    title_height,
    top_offset,
    width;

  size_t
    state;

  XFontStruct
    *font_info;

  XWidgetInfo
    highlight_info,
    menu_info,
    selection_info;

  const MenuTargets
    targets = { &selection_info, &highlight_info, &state, &menu_info };

  MenuEventContext
    event_context;

  /*
    Determine Menu widget attributes.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(request->title != (char *) NULL);
  assert(request->selections != (const char **) NULL);
  assert(request->item != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",request->title);
  font_info=windows->widget.font_info;
  windows->widget.width=(*SubmenuInfoReference()).active == 0 ?
    WidgetTextWidth(font_info,(char *) request->title) : 0;
  for (id=0; request->selections[id] != (char *) NULL; id++)
  {
    width=WidgetTextWidth(font_info,(char *) request->selections[id]);
    if (width > windows->widget.width)
      windows->widget.width=width;
  }
  number_selections=(unsigned int) id;
  XGetWidgetInfo((char *) NULL,&menu_info);
  title_height=(unsigned int) ((*SubmenuInfoReference()).active == 0 ?
    (3*(font_info->descent+font_info->ascent) >> 1)+5 : 2);
  width=WidgetTextWidth(font_info,(char *) request->title);
  height=(unsigned int) ((3*(font_info->ascent+font_info->descent)) >> 1);
  /*
    Position Menu widget.
  */
  windows->widget.width+=(unsigned int) QuantumMargin+
    (menu_info.bevel_width << 1);
  top_offset=title_height+menu_info.bevel_width-1;
  windows->widget.height=top_offset+number_selections*height+4;
  PlaceMenuWidget(display,windows,font_info,title_height);
  /*
    Map Menu widget.
  */
  MapMenuWidget(display,windows);
  /*
    Respond to X events.
  */
  selection_info.height=height;
  cursor=DefineMenuCursor(display,windows);
  state=UpdateConfigurationState;
  event_context.display=display;
  event_context.windows=windows;
  event_context.selections=request->selections;
  event_context.number_selections=number_selections;
  event_context.top_offset=top_offset;
  event_context.item=request->item;
  event_context.height=height;
  event_context.title=request->title;
  event_context.title_height=title_height;
  RespondToMenuEvents(&event_context,&targets);
  return(CloseMenuWidget(&event_context,&targets,cursor));
}
#endif
