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

#if defined(MAGICKCORE_X11_DELEGATE)

/*
  Define declarations.
*/
#define AreaIsActive(matte_info,position)  ( \
  ((position.y >= (matte_info.y-(int) matte_info.bevel_width)) &&  \
   (position.y < (matte_info.y+(int) matte_info.height+(int) matte_info.bevel_width))) \
   ? MagickTrue : MagickFalse)
#define MaxTextWidth  ((unsigned int) (255*XTextWidth(font_info,"_",1)))
#define MinTextWidth  ((unsigned int) (26*XTextWidth(font_info,"_",1)))
#define WindowIsActive(window_info,position)  ( \
  ((position.x >= 0) && (position.y >= 0) &&  \
   (position.x < (int) window_info.width) &&  \
   (position.y < (int) window_info.height)) ? MagickTrue : MagickFalse)

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

/*
  Constant declarations.
*/
static const int
  BorderOffset = 4,
  DoubleClick = 250;

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






#include "MagickCore/widget-color-private.h"

MagickPrivate void XColorBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
  char
    **colorlist,
    primary_selection[MagickPathExtent],
    reset_pattern[MagickPathExtent],
    text[MagickPathExtent];

  ExceptionInfo
    *exception;

  static char
    glob_pattern[MagickPathExtent] = "*";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  unsigned int
    height,
    text_width,
    width;

  size_t
    colors;

  XFontStruct
    *font_info;

  /*
    Get color list and sort in ascending order.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(reset_pattern,"*",MagickPathExtent);
  exception=AcquireExceptionInfo();
  colorlist=GetColorList(glob_pattern,&colors,exception);
  if (colorlist == (char **) NULL)
    {
      /*
        Pattern failed, obtain all the colors.
      */
      (void) CopyMagickString(glob_pattern,"*",MagickPathExtent);
      colorlist=GetColorList(glob_pattern,&colors,exception);
      if (colorlist == (char **) NULL)
        {
          XNoticeWidget(display,windows,"Unable to obtain colors names:",
            glob_pattern);
          (void) XDialogWidget(display,windows,action,"Enter color name:",
            reply);
          return;
        }
    }
  /*
    Determine Color Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=WidestColorBrowserName(font_info,colorlist,colors);
  width=GetColorBrowserWidth(font_info,action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  {
    const ColorBrowserLayout
      color_layout = { display, font_info, width,
        height, text_width };

    PositionColorBrowserWidget(windows,&color_layout);
  }
  /*
    Map Color Browser widget.
  */
  MapColorBrowserWidget(display,windows,mask);
  {
    const ColorBrowserContext
      color_context = { display, windows,
        glob_pattern, reset_pattern, exception, font_info, primary_selection,
        height, action, width, reply, text };

    RunColorBrowser(&color_context,&colorlist,&colors,&mask);
  }
  WithdrawBrowserWidget(display,windows);
  FreeColorBrowserList(colorlist,colors,exception);
  CheckColorBrowserReply(display,windows,reply);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X C o m m a n d W i d g e t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XCommandWidget() maps a menu and returns the command pointed to by the user
%  when the button is released.
%
%  The format of the XCommandWidget method is:
%
%      int XCommandWidget(Display *display,XWindows *windows,
%        const char *const *selections,XEvent *event)
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
%    o selections: Specifies a pointer to one or more strings that comprise
%      the choices in the menu.
%
%    o event: Specifies a pointer to a X11 XEvent structure.
%
*/
typedef struct _CommandWidgetEvent
{
  Display
    *display;

  XWindows
    *windows;

  XEvent
    *event;

  unsigned int
    number_selections;
} CommandWidgetEvent;

static int PressCommandSelection(const CommandWidgetEvent *command_event,int id)
{
  Display
    *const display = command_event->display;

  XWindows
    *const windows = command_event->windows;

  XEvent
    *const event = command_event->event;

  const unsigned int
    number_selections = command_event->number_selections;

  int
    i;

  for (i=0; i < (int) number_selections; i++)
  {
    if (MatteIsActive(selection_info[i],event->xbutton) == MagickFalse)
      continue;
    if (i >= (int) windows->command.data)
      {
        selection_info[i].raised=MagickFalse;
        XDrawBeveledButton(display,&windows->command,&selection_info[i]);
        break;
      }
    submenu_info=selection_info[i];
    submenu_info.active=MagickTrue;
    toggle_info.y=submenu_info.y+(int) (submenu_info.height >> 1)-
      (int) (toggle_info.height >> 1);
    id=i;
    (void) XCheckWindowEvent(display,windows->widget.id,LeaveWindowMask,
      event);
    break;
  }
  return(id);
}

static int ReleaseCommandSelection(const CommandWidgetEvent *command_event,int id)
{
  Display
    *const display = command_event->display;

  XWindows
    *const windows = command_event->windows;

  XEvent
    *const event = command_event->event;

  const unsigned int
    number_selections = command_event->number_selections;

  int
    i;

  for (i=0; i < (int) number_selections; i++)
  {
    if (MatteIsActive(selection_info[i],event->xbutton) == MagickFalse)
      continue;
    id=i;
    if (id >= (int) windows->command.data)
      {
        selection_info[id].raised=MagickTrue;
        XDrawBeveledButton(display,&windows->command,&selection_info[id]);
        break;
      }
    break;
  }
  return(id);
}

static int TrackCommandSelections(const CommandWidgetEvent *command_event,int id)
{
  Display
    *const display = command_event->display;

  XWindows
    *const windows = command_event->windows;

  XEvent
    *const event = command_event->event;

  const unsigned int
    number_selections = command_event->number_selections;

  int
    i;

  for (i=0; i < (int) number_selections; i++)
  {
    if (i >= (int) windows->command.data)
      {
        if (selection_info[i].raised ==
            MatteIsActive(selection_info[i],event->xmotion))
          {
            /*
              Button status changed.
            */
            selection_info[i].raised=!selection_info[i].raised;
            XDrawBeveledButton(display,&windows->command,
              &selection_info[i]);
          }
        continue;
      }
    if (MatteIsActive(selection_info[i],event->xmotion) == MagickFalse)
      continue;
    submenu_info=selection_info[i];
    submenu_info.active=MagickTrue;
    toggle_info.raised=MagickTrue;
    toggle_info.y=submenu_info.y+(int) (submenu_info.height >> 1)-
      (int) (toggle_info.height >> 1);
    XDrawTriangleEast(display,&windows->command,&toggle_info);
    id=i;
  }
  return(id);
}

static int FollowCommandMotion(const CommandWidgetEvent *motion,int id)
{
  Display
    *const display = motion->display;

  XWindows
    *const windows = motion->windows;

  XEvent
    *const event = motion->event;

  const unsigned int
    number_selections = motion->number_selections;

  for ( ; ; )
  {
    CommandWidgetEvent
      command_event;

    command_event.display=display;
    command_event.windows=windows;
    command_event.event=event;
    command_event.number_selections=number_selections;
    id=TrackCommandSelections(&command_event,id);
    XDelay(display,SuspendTime);
    if (XCheckMaskEvent(display,ButtonMotionMask,event) == MagickFalse)
      break;
    while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
    toggle_info.raised=MagickFalse;
    if (windows->command.data != 0)
      XDrawTriangleEast(display,&windows->command,&toggle_info);
  }
  return(id);
}

static const unsigned char
  tile_bits[]=
  {
    0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1e, 0x38, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1e, 0xbc, 0x9f, 0x03, 0x00, 0x3e, 0x00, 0xc0,
    0x1f, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x1e, 0xfc, 0xff, 0x0f, 0x80, 0x3f,
    0x00, 0xf0, 0x1f, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0xfc, 0xff, 0x1f,
    0xe0, 0x3f, 0x00, 0xfc, 0x1f, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0xfc,
    0xff, 0x1f, 0xf0, 0x3f, 0x00, 0xfe, 0x1f, 0xf8, 0x0f, 0x00, 0x00, 0x00,
    0x1e, 0xfc, 0xfc, 0x3f, 0xf8, 0x3f, 0x00, 0xff, 0x1e, 0xfc, 0x0f, 0x00,
    0x00, 0x00, 0x1e, 0x7c, 0xfc, 0x3e, 0xf8, 0x3c, 0x80, 0x1f, 0x1e, 0x7c,
    0x0f, 0x00, 0x00, 0x00, 0x1e, 0x78, 0x78, 0x3c, 0x7c, 0x3c, 0xc0, 0x0f,
    0x1e, 0x3e, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0x78, 0x78, 0x3c, 0x7c, 0x3c,
    0xc0, 0x07, 0x1e, 0x3e, 0x0f, 0x00, 0x00, 0x00, 0x1e, 0x78, 0x78, 0x3c,
    0x7c, 0x7c, 0xc0, 0x0f, 0x1e, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x78,
    0x78, 0x3c, 0xfc, 0x7c, 0x80, 0x7f, 0x1e, 0x7c, 0x00, 0x00, 0x00, 0x00,
    0x1e, 0xf8, 0x78, 0x7c, 0xf8, 0xff, 0x00, 0xff, 0x1f, 0xf8, 0xff, 0x00,
    0x00, 0x00, 0x1e, 0xf8, 0x78, 0x7c, 0xf0, 0xff, 0x07, 0xfe, 0x1f, 0xf8,
    0xff, 0x00, 0x00, 0x00, 0x1e, 0xf8, 0x78, 0x7c, 0xf0, 0xff, 0x07, 0xf8,
    0x1f, 0xf0, 0xff, 0x01, 0x00, 0x00, 0x1e, 0xf8, 0x78, 0x7c, 0xc0, 0xef,
    0x07, 0xe0, 0x1f, 0xc0, 0xff, 0x01, 0x00, 0x00, 0x1e, 0x70, 0x40, 0x78,
    0x00, 0xc7, 0x07, 0x00, 0x1e, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x1e, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07,
    0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc0, 0x0f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x60, 0x00, 0xc0, 0x0f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x78, 0x00, 0xc0, 0x8f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0xc0, 0x8f, 0x3f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0xe0, 0x9f, 0x7f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0xe0, 0xdf,
    0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x78, 0x00,
    0xe0, 0xdf, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x0c,
    0x78, 0x30, 0xf0, 0xff, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e,
    0x00, 0x0f, 0xf8, 0x70, 0xf0, 0xff, 0x7b, 0x00, 0x00, 0x1f, 0x00, 0xe0,
    0x0f, 0x1e, 0x80, 0x0f, 0xf8, 0x78, 0xf0, 0xfd, 0xf9, 0x00, 0xc0, 0x1f,
    0x00, 0xf8, 0x0f, 0x00, 0xe0, 0x1f, 0xf8, 0x7c, 0xf0, 0xfc, 0xf9, 0x00,
    0xf0, 0x1f, 0x00, 0xfe, 0x0f, 0x00, 0xf0, 0x07, 0xf8, 0x3e, 0xf8, 0xfc,
    0xf0, 0x01, 0xf8, 0x1f, 0x00, 0xff, 0x0f, 0x1e, 0xf0, 0x03, 0xf8, 0x3f,
    0xf8, 0xf8, 0xf0, 0x01, 0xfc, 0x1f, 0x80, 0x7f, 0x0f, 0x1e, 0xf8, 0x00,
    0xf8, 0x1f, 0x78, 0x18, 0xf0, 0x01, 0x7c, 0x1e, 0xc0, 0x0f, 0x0f, 0x1e,
    0x7c, 0x00, 0xf0, 0x0f, 0x78, 0x00, 0xf0, 0x01, 0x3e, 0x1e, 0xe0, 0x07,
    0x0f, 0x1e, 0x7c, 0x00, 0xf0, 0x07, 0x7c, 0x00, 0xe0, 0x01, 0x3e, 0x1e,
    0xe0, 0x03, 0x0f, 0x1e, 0x3e, 0x00, 0xf0, 0x0f, 0x7c, 0x00, 0xe0, 0x03,
    0x3e, 0x3e, 0xe0, 0x07, 0x0f, 0x1e, 0x1e, 0x00, 0xf0, 0x1f, 0x3c, 0x00,
    0xe0, 0x03, 0x7e, 0x3e, 0xc0, 0x3f, 0x0f, 0x1e, 0x3e, 0x00, 0xf0, 0x1f,
    0x3e, 0x00, 0xe0, 0x03, 0xfc, 0x7f, 0x80, 0xff, 0x0f, 0x1e, 0xfc, 0x00,
    0xf0, 0x3e, 0x3e, 0x00, 0xc0, 0x03, 0xf8, 0xff, 0x03, 0xff, 0x0f, 0x1e,
    0xfc, 0x07, 0xf0, 0x7c, 0x1e, 0x00, 0xc0, 0x03, 0xf8, 0xff, 0x03, 0xfc,
    0x0f, 0x1e, 0xf8, 0x1f, 0xf0, 0xf8, 0x1e, 0x00, 0xc0, 0x03, 0xe0, 0xf7,
    0x03, 0xf0, 0x0f, 0x1e, 0xe0, 0x3f, 0xf0, 0x78, 0x1c, 0x00, 0x80, 0x03,
    0x80, 0xe3, 0x03, 0x00, 0x0f, 0x1e, 0xc0, 0x3f, 0xf0, 0x30, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0e, 0x00, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0f, 0x00, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

static void ClientCommandMessage(Display *display,XWindows *windows,
  XEvent *event)
{
  /*
    If client window delete message, withdraw command widget.
  */
  if (event->xclient.message_type != windows->wm_protocols)
    return;
  if (*event->xclient.data.l != (int) windows->wm_delete_window)
    return;
  (void) XWithdrawWindow(display,windows->command.id,
    windows->command.screen);
}

static size_t ConfigureCommandWindow(XWindows *windows,XEvent *event,
  size_t state)
{
  /*
    Update widget configuration.
  */
  if (event->xconfigure.window != windows->command.id)
    return(state);
  if (event->xconfigure.send_event != 0)
    {
      windows->command.x=event->xconfigure.x;
      windows->command.y=event->xconfigure.y;
    }
  if ((event->xconfigure.width == (int) windows->command.width) &&
      (event->xconfigure.height == (int) windows->command.height))
    return(state);
  windows->command.width=(unsigned int)
    MagickMax(event->xconfigure.width,(int) windows->command.min_width);
  windows->command.height=(unsigned int)
    MagickMax(event->xconfigure.height,(int) windows->command.min_height);
  state|=UpdateConfigurationState;
  return(state);
}

static size_t ExposeCommandWindow(XWindows *windows,XEvent *event,
  size_t state)
{
  if (event->xexpose.window != windows->command.id)
    return(state);
  if (event->xexpose.count != 0)
    return(state);
  state|=RedrawWidgetState;
  return(state);
}

static void HandleCommandWindowEvent(const CommandWidgetEvent *tile,int *id,
  size_t *state)
{
  Display
    *const display = tile->display;

  XWindows
    *const windows = tile->windows;

  XEvent
    *const event = tile->event;

  const unsigned int
    number_selections = tile->number_selections;

  switch (event->type)
  {
    case Expose:
    {
      (*state)=ExposeCommandWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      CommandWidgetEvent
        motion;

      /*
        Return the ID of the highlighted menu entry.
      */
      motion.display=display;
      motion.windows=windows;
      motion.event=event;
      motion.number_selections=number_selections;
      (*id)=FollowCommandMotion(&motion,(*id));
      break;
    }
    case MapNotify:
    {
      windows->command.mapped=MagickTrue;
      break;
    }
    case UnmapNotify:
    {
      windows->command.mapped=MagickFalse;
      break;
    }
    default:
      break;
  }
}

static void HandleCommandEvent(const CommandWidgetEvent *tile,int *id,
  size_t *state)
{
  Display
    *const display = tile->display;

  XWindows
    *const windows = tile->windows;

  XEvent
    *const event = tile->event;

  const unsigned int
    number_selections = tile->number_selections;

  if (event != (XEvent *) NULL)
    switch (event->type)
    {
      case ButtonPress:
      {
        CommandWidgetEvent
          command_event;

        command_event.display=display;
        command_event.windows=windows;
        command_event.event=event;
        command_event.number_selections=number_selections;
        (*id)=PressCommandSelection(&command_event,(*id));
        break;
      }
      case ButtonRelease:
      {
        CommandWidgetEvent
          command_event;

        command_event.display=display;
        command_event.windows=windows;
        command_event.event=event;
        command_event.number_selections=number_selections;
        (*id)=ReleaseCommandSelection(&command_event,(*id));
        break;
      }
      case ClientMessage:
      {
        ClientCommandMessage(display,windows,event);
        break;
      }
      case ConfigureNotify:
      {
        (*state)=ConfigureCommandWindow(windows,event,(*state));
        break;
      }
      default:
      {
        HandleCommandWindowEvent(tile,id,state);
        break;
      }
    }
}

#define tile_width 112
#define tile_height 70

static void RedrawCommandButtons(Display *display,XWindows *windows,
  const unsigned int number_selections)
{
  int
    i;

  Pixmap
    tile_pixmap;

  /*
    Draw command buttons.
  */
  tile_pixmap=XCreatePixmapFromBitmapData(display,windows->command.id,
    (char *) tile_bits,tile_width,tile_height,1L,0L,1);
  if (tile_pixmap != (Pixmap) NULL)
    {
      (void) XCopyPlane(display,tile_pixmap,windows->command.id,
        windows->command.annotate_context,0,0,tile_width,tile_height,
        (int) ((windows->command.width-tile_width) >> 1),10,1L);
      (void) XFreePixmap(display,tile_pixmap);
    }
  for (i=0; i < (int) number_selections; i++)
  {
    XDrawBeveledButton(display,&windows->command,&selection_info[i]);
    if (i >= (int) windows->command.data)
      continue;
    toggle_info.raised=MagickFalse;
    toggle_info.y=selection_info[i].y+(int) (selection_info[i].height >> 1)-
      (int) (toggle_info.height >> 1);
    XDrawTriangleEast(display,&windows->command,&toggle_info);
  }
  XHighlightWidget(display,&windows->command,BorderOffset,BorderOffset);
}

typedef struct _CommandLayout
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  XFontStruct
    *font_info;

  unsigned int
    height,
    number_selections;
} CommandLayout;

static void InitializeCommandButtons(const CommandLayout *layout)
{
  Display
    *const display = layout->display;

  XWindows
    *const windows = layout->windows;

  const char
    *const *const selections = layout->selections;

  XFontStruct
    *const font_info = layout->font_info;

  const unsigned int
    height = layout->height,
    number_selections = layout->number_selections;

  int
    i,
    y;

  y=tile_height+20;
  for (i=0; i < (int) number_selections; i++)
  {
    XGetWidgetInfo(selections[i],&selection_info[i]);
    selection_info[i].center=MagickFalse;
    selection_info[i].bevel_width--;
    selection_info[i].height=(unsigned int) ((3*height) >> 1);
    selection_info[i].x=(QuantumMargin >> 1)+4;
    selection_info[i].width=(unsigned int) ((int) windows->command.width-
      (selection_info[i].x << 1));
    selection_info[i].y=y;
    y+=(int) selection_info[i].height+(int)
      (selection_info[i].bevel_width << 1)+6;
  }
  XGetWidgetInfo((char *) NULL,&toggle_info);
  toggle_info.bevel_width--;
  toggle_info.width=(unsigned int) (((5*height) >> 3)-
    (toggle_info.bevel_width << 1));
  toggle_info.height=toggle_info.width;
  toggle_info.x=selection_info[0].x+(int) selection_info[0].width-
    (int) toggle_info.width-(int) (QuantumMargin >> 1);
  if (windows->command.mapped)
    (void) XClearWindow(display,windows->command.id);
}

static void ReconfigureCommandWindow(Display *display,XWindows *windows)
{
  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  if (windows->command.id != (Window) NULL)
    {
      Status
        status;

      /*
        Reconfigure command window.
      */
      status=XStringListToTextProperty(&windows->command.name,1,
        &window_name);
      if (status != False)
        {
          XSetWMName(display,windows->command.id,&window_name);
          XSetWMIconName(display,windows->command.id,&window_name);
          (void) XFree((void *) window_name.value);
        }
      window_changes.width=(int) windows->command.width;
      window_changes.height=(int) windows->command.height;
      (void) XReconfigureWMWindow(display,windows->command.id,
        windows->command.screen,(unsigned int) (CWWidth | CWHeight),
        &window_changes);
    }
}

typedef struct _CommandSizing
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *selections;

  XFontStruct
    *font_info;

  unsigned int
    height;
} CommandSizing;

static unsigned int SizeCommandWindow(const CommandSizing *sizing)
{
  Display
    *const display = sizing->display;

  XWindows
    *const windows = sizing->windows;

  const char
    *const *const selections = sizing->selections;

  XFontStruct
    *const font_info = sizing->font_info;

  const unsigned int
    height = sizing->height;

  int
    i;

  unsigned int
    number_selections,
    width;

  windows->command.width=0;
  for (i=0; selections[i] != (char *) NULL; i++)
  {
    width=WidgetTextWidth(font_info,(char *) selections[i]);
    if (width > windows->command.width)
      windows->command.width=width;
  }
  number_selections=(unsigned int) i;
  windows->command.width+=(unsigned int) (3*QuantumMargin+10);
  if ((int) windows->command.width < (tile_width+QuantumMargin+10))
    windows->command.width=(unsigned  int) (tile_width+QuantumMargin+10);
  windows->command.height=(unsigned  int) (number_selections*
    (((3*height) >> 1)+10)+tile_height+20);
  windows->command.min_width=windows->command.width;
  windows->command.min_height=windows->command.height;
  XConstrainWindowPosition(display,&windows->command);
  return(number_selections);
}

MagickPrivate int XCommandWidget(Display *display,XWindows *windows,
  const char *const *selections,XEvent *event)
{
  int
    id;

  static unsigned int
    number_selections;

  unsigned int
    height;

  size_t
    state;

  XFontStruct
    *font_info;

  CommandWidgetEvent
    tile;

  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  font_info=windows->command.font_info;
  height=(unsigned int) (font_info->ascent+font_info->descent);
  id=(~0);
  state=DefaultState;
  if (event == (XEvent *) NULL)
    {
      CommandSizing
        sizing;

      /*
        Determine command window attributes.
      */
      assert(selections != (const char **) NULL);
      sizing.display=display;
      sizing.windows=windows;
      sizing.selections=selections;
      sizing.font_info=font_info;
      sizing.height=height;
      number_selections=SizeCommandWindow(&sizing);
      ReconfigureCommandWindow(display,windows);
      /*
        Allocate selection info memory.
      */
      if (selection_info != (XWidgetInfo *) NULL)
        selection_info=(XWidgetInfo *) RelinquishMagickMemory(selection_info);
      selection_info=(XWidgetInfo *) AcquireQuantumMemory(number_selections,
        sizeof(*selection_info));
      if (selection_info == (XWidgetInfo *) NULL)
        {
          ThrowXWindowFatalException(ResourceLimitFatalError,
            "MemoryAllocationFailed","...");
          return(id);
        }
      state|=(int) UpdateConfigurationState | (int) RedrawWidgetState;
    }
  /*
    Wait for next event.
  */
  tile.display=display;
  tile.windows=windows;
  tile.event=event;
  tile.number_selections=number_selections;
  HandleCommandEvent(&tile,&id,&state);
  if (state & UpdateConfigurationState)
    {
      CommandLayout
        layout;

      /*
        Initialize button information.
      */
      assert(selections != (const char **) NULL);
      layout.display=display;
      layout.windows=windows;
      layout.selections=selections;
      layout.font_info=font_info;
      layout.height=height;
      layout.number_selections=number_selections;
      InitializeCommandButtons(&layout);
    }
  if (state & RedrawWidgetState)
    RedrawCommandButtons(display,windows,number_selections);
  return(id);
}



#include "MagickCore/widget-file-private.h"

MagickPrivate void XFileBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
  char
    *directory,
    **filelist,
    home_directory[MagickPathExtent],
    primary_selection[MagickPathExtent],
    text[MagickPathExtent],
    working_path[MagickPathExtent];

  static char
    glob_pattern[MagickPathExtent] = "*",
    format[MagickPathExtent] = "miff";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  size_t
    files;

  ssize_t
    i;

  unsigned int
    anomaly,
    height,
    text_width,
    width;

  XFontStruct
    *font_info;

  const FileBrowserRunTargets
    run_targets = { &files, &i, &filelist, &mask };

  /*
    Read filelist from current directory.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  directory=getcwd(home_directory,MagickPathExtent);
  (void) directory;
  (void) CopyMagickString(working_path,home_directory,MagickPathExtent);
  filelist=ListFiles(working_path,glob_pattern,&files);
  if (filelist == (char **) NULL)
    {
      /*
        Directory read failed.
      */
      XNoticeWidget(display,windows,"Unable to read directory:",working_path);
      (void) XDialogWidget(display,windows,action,"Enter filename:",reply);
      return;
    }
  /*
    Determine File Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=WidestFileBrowserName(font_info,filelist,files);
  width=GetFileBrowserWidth(font_info,action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position File Browser widget.
  */
  {
    const FileBrowserLayout
      layout = { display, font_info, width, height, text_width };

    PositionFileBrowserWidget(windows,&layout);
  }
  MapFileBrowserWidget(display,windows,mask);
  /*
    Respond to X events.
  */
  anomaly=GetFileBrowserAnomaly(action);
  {
    const FileBrowserContext
      file_context = { display, windows, reply, glob_pattern, working_path,
        primary_selection, font_info, height, format, anomaly, home_directory,
        action, width, text };

    RunFileBrowser(&file_context,&run_targets);
  }
  WithdrawBrowserWidget(display,windows);
  FreeFileBrowserList(filelist,files);
  SetFileBrowserReply(reply,working_path);
}

#include "MagickCore/widget-font-private.h"

MagickPrivate void XFontBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
  char
    back_pattern[MagickPathExtent],
    **fontlist,
    **listhead,
    primary_selection[MagickPathExtent] = "",
    reset_pattern[MagickPathExtent],
    text[MagickPathExtent];

  int
    fonts;

  int
    i;

  static char
    glob_pattern[MagickPathExtent] = "*";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  unsigned int
    height,
    text_width,
    width;

  XFontStruct
    *font_info;

  const FontBrowserRunTargets
    run_targets = { &fonts, &i, &font_info, &fontlist, &listhead, &mask };

  /*
    Get font list and sort in ascending order.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(back_pattern,glob_pattern,MagickPathExtent);
  (void) CopyMagickString(reset_pattern,"*",MagickPathExtent);
  if (AcquireFontList(display,windows,glob_pattern,&run_targets) == MagickFalse)
    return;
  /*
    Determine Font Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=WidestFontBrowserName(font_info,fontlist,fonts);
  width=GetFontBrowserWidth(font_info,action);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position Font Browser widget.
  */
  windows->widget.width=width+MagickMin(text_width,MaxTextWidth)+(unsigned int)
    (6*QuantumMargin);
  windows->widget.min_width=width+MinTextWidth+(unsigned int) (4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    (((85*(int) height) >> 2)+((13*QuantumMargin) >> 1)+4);
  windows->widget.min_height=(unsigned int)
    (((27*(int) height) >> 1)+((13*QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  MapFontBrowserWidget(display,windows,mask);
  {
    const FontBrowserContext
      font_context = { display, windows,
        glob_pattern, back_pattern, reset_pattern, primary_selection, height,
        action, width, reply, text };

    if (RunFontBrowser(&font_context,&run_targets) == MagickFalse)
      return;
  }
  WithdrawBrowserWidget(display,windows);
  FreeFontBrowserList(listhead,fontlist);
}

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

static XWidgetInfo PageListBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const unsigned int visible_entries)
{
  if (event.xbutton.y < slider_info.y)
    slider_info.id-=(int) (visible_entries-1);
  else
    slider_info.id+=(int) (visible_entries-1);
  return(slider_info);
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

static int FindListBrowserCursor(XFontStruct *font_info,
  const XWidgetInfo reply_info,const int x)
{
  int
    i;

  for (i=1; i <= Extent(reply_info.marker); i++)
    if (XTextWidth(font_info,reply_info.marker,i) > x)
      break;
  return(i);
}

static XWidgetInfo HighlightListBrowserReply(const ListBrowserClick *click,
  char *primary_selection,XWidgetInfo reply_info)
{
  Display
    *const display = click->display;

  XWindowInfo
    *const window_info = click->window_info;

  const XEvent
    event = click->event;

  const Time
    click_time = click->click_time;

  if (event.xbutton.time > (click_time+(unsigned long) DoubleClick))
    reply_info.highlight=MagickFalse;
  else
    {
      /*
        Become the XA_PRIMARY selection owner.
      */
      (void) CopyMagickString(primary_selection,reply_info.text,
        MagickPathExtent);
      (void) XSetSelectionOwner(display,XA_PRIMARY,window_info->id,
        event.xbutton.time);
      reply_info.highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
        window_info->id ? MagickTrue : MagickFalse;
    }
  return(reply_info);
}

static size_t ConfirmListBrowserReply(Display *display,
  const XWidgetInfo reply_info,size_t state)
{
  if (*reply_info.text == '\0')
    (void) XBell(display,0);
  else
    state|=ExitState;
  return(state);
}

static void ReleaseListBrowserAction(const ListBrowserRelease *release,
  const XWidgetInfo reply_info,XWidgetInfo *action_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindowInfo
    *const window_info = release->window_info;

  const XEvent
    event = release->event;

  if ((*action_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == window_info->id) &&
          (MatteIsActive((*action_info),event.xbutton)))
        {
          (*state)=ConfirmListBrowserReply(display,reply_info,(*state));
        }
      (*action_info).raised=MagickTrue;
      XDrawBeveledButton(display,window_info,&(*action_info));
    }
}

static void ReleaseListBrowserCancel(const ListBrowserRelease *release,
  const XWidgetInfo reply_info,XWidgetInfo *cancel_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindowInfo
    *const window_info = release->window_info;

  const XEvent
    event = release->event;

  if ((*cancel_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == window_info->id) &&
          (MatteIsActive((*cancel_info),event.xbutton)))
        {
          *reply_info.text='\0';
          (*state)|=ExitState;
        }
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,window_info,&(*cancel_info));
    }
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

static XWidgetInfo ApplyListBrowserControlKey(XWidgetInfo reply_info,
  const KeySym key_symbol,const size_t state)
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
        *reply_info.text='\0';
        reply_info.cursor=reply_info.text;
        reply_info.marker=reply_info.text;
        reply_info.highlight=MagickFalse;
        break;
      }
      default:
        break;
    }
  return(reply_info);
}

static XWidgetInfo TrackListBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const unsigned int entries)
{
  slider_info.y=event.xmotion.y-(int)
    ((slider_info.height+slider_info.bevel_width) >> 1)+1;
  if (slider_info.y < slider_info.min_y)
    slider_info.y=slider_info.min_y;
  if (slider_info.y > slider_info.max_y)
    slider_info.y=slider_info.max_y;
  slider_info.id=0;
  if (slider_info.y != slider_info.min_y)
    slider_info.id=((int) entries*(slider_info.y-
      slider_info.min_y+1))/(slider_info.max_y-slider_info.min_y+1);
  return(slider_info);
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

MagickPrivate void XListBrowserWidget(Display *display,XWindows *windows,
  XWindowInfo *window_info,const char *const *list,const char *action,
  const char *query,char *reply)
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
  assert(list != (const char **) NULL);
  assert(action != (char *) NULL);
  assert(query != (char *) NULL);
  assert(reply != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (list == (const char **) NULL)
    {
      XNoticeWidget(display,windows,"No text to browse:",(char *) NULL);
      return;
    }
  entries=CountListBrowserEntries(list);
  /*
    Determine Font Browser widget attributes.
  */
  font_info=window_info->font_info;
  text_width=WidgetTextWidth(font_info,(char *) query);
  text_width=WidestListBrowserEntry(font_info,list,entries,text_width);
  width=GetListBrowserWidth(font_info,action);
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
  list_context.list=list;
  list_context.height=height;
  list_context.action=action;
  list_context.width=width;
  list_context.reply=reply;
  list_context.query=query;
  RunListBrowser(&list_context,&mask);
  WithdrawListBrowserWidget(display,windows,window_info);
}

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

  if ((submenu_info.active != 0) &&
      (event.xmotion.window == windows->command.id))
    {
      if (((*state) & InactiveWidgetState) == 0)
        {
          if (MatteIsActive(submenu_info,event.xmotion) == MagickFalse)
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
      if (submenu_info.active == 0)
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
      if (submenu_info.active == 0)
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
  if (submenu_info.active != 0)
    {
      submenu_info.active=MagickFalse;
      toggle_info.raised=MagickFalse;
      XDrawTriangleEast(display,&windows->command,&toggle_info);
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
  if (submenu_info.active != 0)
    {
      windows->widget.x=windows->command.x+(int) windows->command.width-
        QuantumMargin;
      toggle_info.raised=MagickTrue;
      XDrawTriangleEast(display,&windows->command,&toggle_info);
    }
  windows->widget.y=submenu_info.active == 0 ? y-(int) (title_height >> 2) : y;
  if (submenu_info.active != 0)
    windows->widget.y=windows->command.y+submenu_info.y;
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
  const char *title,const char *const *selections,char *item)
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
  assert(title != (char *) NULL);
  assert(selections != (const char **) NULL);
  assert(item != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",title);
  font_info=windows->widget.font_info;
  windows->widget.width=submenu_info.active == 0 ?
    WidgetTextWidth(font_info,(char *) title) : 0;
  for (id=0; selections[id] != (char *) NULL; id++)
  {
    width=WidgetTextWidth(font_info,(char *) selections[id]);
    if (width > windows->widget.width)
      windows->widget.width=width;
  }
  number_selections=(unsigned int) id;
  XGetWidgetInfo((char *) NULL,&menu_info);
  title_height=(unsigned int) (submenu_info.active == 0 ?
    (3*(font_info->descent+font_info->ascent) >> 1)+5 : 2);
  width=WidgetTextWidth(font_info,(char *) title);
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
  event_context.selections=selections;
  event_context.number_selections=number_selections;
  event_context.top_offset=top_offset;
  event_context.item=item;
  event_context.height=height;
  event_context.title=title;
  event_context.title_height=title_height;
  RespondToMenuEvents(&event_context,&targets);
  return(CloseMenuWidget(&event_context,&targets,cursor));
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
%   X P r e f e r e n c e s W i d g e t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPreferencesWidget() displays a Preferences widget with program preferences.
%  If the user presses the Apply button, the preferences are stored in a
%  configuration file in the users' home directory.
%
%  The format of the XPreferencesWidget method is:
%
%      MagickBooleanType XPreferencesWidget(Display *display,
%        XResourceInfo *resource_info,XWindows *windows)
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
*/
#define ApplyButtonText  "Apply"
#define CacheButtonText  "%lu mega-bytes of memory in the undo edit cache   "
#define CancelButtonText  "Cancel"
#define NumberPreferences  8

static size_t ReleasePreferencesButton(XWindows *windows,const XEvent event,
  const XWidgetInfo button_info,size_t state)
{
  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(button_info,event.xbutton)))
    state|=ExitState;
  return(state);
}

static void TogglePreferencesButton(Display *display,XWindows *windows,
  const XEvent event,XWidgetInfo *preferences_info)
{
  int
    i;

  for (i=0; i < NumberPreferences; i++)
    if (MatteIsActive(preferences_info[i],event.xbutton))
      {
        /*
          User pressed a Preferences button.
        */
        preferences_info[i].raised=preferences_info[i].raised ==
          MagickFalse ? MagickTrue : MagickFalse;
        XDrawBeveledButton(display,&windows->widget,&preferences_info[i]);
        break;
      }
}

static void DoublePreferencesUndoCache(XResourceInfo *resource_info)
{
  resource_info->undo_cache<<=1;
  if (resource_info->undo_cache > 256)
    resource_info->undo_cache=1;
}

static const char
  *Preferences[] =
  {
    "display image centered on a backdrop",
    "confirm on program exit",
    "confirm on image edits",
    "correct image for display gamma",
    "display warning messages",
    "apply Floyd/Steinberg error diffusion to image",
    "use a shared colormap for colormapped X visuals",
    "display images as an X server pixmap"
  };

static void MapPreferencesWidget(Display *display,XWindows *windows)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,"Preferences",MagickPathExtent);
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

static unsigned int GetPreferencesTextWidth(XFontStruct *font_info)
{
  int
    i;

  unsigned int
    text_width;

  text_width=WidgetTextWidth(font_info,CacheButtonText);
  for (i=0; i < NumberPreferences; i++)
    if (WidgetTextWidth(font_info,(char *) Preferences[i]) > text_width)
      text_width=WidgetTextWidth(font_info,(char *) Preferences[i]);
  return(text_width);
}

static unsigned int GetPreferencesButtonWidth(XFontStruct *font_info)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,ApplyButtonText);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  width+=(unsigned int) QuantumMargin;
  return(width);
}

static void PositionPreferencesWidth(XWindows *windows,XFontStruct *font_info,
  unsigned int width,unsigned int text_width)
{
  windows->widget.width=(unsigned int) (MagickMax((int) (width << 1),
    (int) text_width)+6*QuantumMargin);
  windows->widget.min_width=(width << 1)+(unsigned int) QuantumMargin;
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
}

static void PositionPreferencesHeight(XWindows *windows,
  XFontStruct *font_info,unsigned int height)
{
  windows->widget.height=(unsigned int) (7*(int) height+NumberPreferences*
    ((int) height+(QuantumMargin >> 1)));
  windows->widget.min_height=(unsigned int) (7*(int) height+NumberPreferences*
    ((int) height+(QuantumMargin >> 1)));
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
}

static void SavePreferences(XResourceInfo *resource_info,
  const XWidgetInfo *preferences_info)
{
  /*
    Save user preferences to the client configuration file.
  */
  resource_info->backdrop=preferences_info[0].raised == MagickFalse;
  resource_info->confirm_exit=preferences_info[1].raised == MagickFalse;
  resource_info->confirm_edit=preferences_info[2].raised == MagickFalse;
  resource_info->gamma_correct=preferences_info[3].raised == MagickFalse;
  resource_info->display_warnings=preferences_info[4].raised == MagickFalse;
  resource_info->quantize_info->dither_method=
    preferences_info[5].raised == MagickFalse ?
    RiemersmaDitherMethod : NoDitherMethod;
  resource_info->colormap=SharedColormap;
  if (preferences_info[6].raised)
    resource_info->colormap=PrivateColormap;
  resource_info->use_pixmap=preferences_info[7].raised == MagickFalse;
  XUserPreferences(resource_info);
}

typedef struct _PreferencesWidgets
{
  XWidgetInfo
    *cache_info;

  char
    *cache;
  XWidgetInfo
    *preferences_info;
  XWidgetInfo
    *cancel_info;

  XWidgetInfo
    *apply_info;

  unsigned int
    *width;

  size_t
    *state;
  XEvent
    *event;
} PreferencesWidgets;

typedef struct _PreferencesContext
{
  XResourceInfo
    *resource_info;

  XFontStruct
    *font_info;
  XWindows
    *windows;

  unsigned int
    height;
  Display
    *display;
} PreferencesContext;

static void InitializePreferencesCache(const PreferencesContext *context,
  const PreferencesWidgets *widgets,int y)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const cache_info = widgets->cache_info;

  char
    *const cache = widgets->cache;

  (void) FormatLocaleString(cache,MagickPathExtent,CacheButtonText,
    (unsigned long) resource_info->undo_cache);
  XGetWidgetInfo(cache,cache_info);
  (*cache_info).bevel_width--;
  (*cache_info).width=(unsigned int) QuantumMargin >> 1;
  (*cache_info).height=(unsigned int) QuantumMargin >> 1;
  (*cache_info).x=QuantumMargin << 1;
  (*cache_info).y=y;
}

static void SetPreferencesRaised(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  XWidgetInfo
    *const preferences_info = widgets->preferences_info;

  preferences_info[0].raised=resource_info->backdrop == MagickFalse;
  preferences_info[1].raised=resource_info->confirm_exit == MagickFalse;
  preferences_info[2].raised=resource_info->confirm_edit == MagickFalse;
  preferences_info[3].raised=resource_info->gamma_correct ==
    MagickFalse;
  preferences_info[4].raised=resource_info->display_warnings ==
    MagickFalse;
  preferences_info[5].raised=
    resource_info->quantize_info->dither_method == NoDitherMethod;
  preferences_info[6].raised=resource_info->colormap != SharedColormap;
  preferences_info[7].raised=resource_info->use_pixmap == MagickFalse;
}

static void UpdatePreferencesButtons(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  XFontStruct
    *const font_info = context->font_info;

  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const preferences_info = widgets->preferences_info;

  unsigned int
    *const width = widgets->width;

  size_t
    *const state = widgets->state;

  int
    i,
    y;

  if ((*state) & UpdateConfigurationState)
    {
      /*
        Initialize button information.
      */
      XGetWidgetInfo(CancelButtonText,cancel_info);
      (*cancel_info).width=(*width);
      (*cancel_info).height=(unsigned int) (3*height) >> 1;
      (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
        (QuantumMargin << 1);
      (*cancel_info).y=(int) windows->widget.height-(int) (*cancel_info).height-
        QuantumMargin;
      XGetWidgetInfo(ApplyButtonText,apply_info);
      (*apply_info).width=(*width);
      (*apply_info).height=(unsigned int) (3*height) >> 1;
      (*apply_info).x=QuantumMargin << 1;
      (*apply_info).y=(*cancel_info).y;
      y=(int) (height << 1);
      for (i=0; i < NumberPreferences; i++)
      {
        XGetWidgetInfo(Preferences[i],&preferences_info[i]);
        preferences_info[i].bevel_width--;
        preferences_info[i].width=(unsigned int) QuantumMargin >> 1;
        preferences_info[i].height=(unsigned int) QuantumMargin >> 1;
        preferences_info[i].x=QuantumMargin << 1;
        preferences_info[i].y=y;
        y+=(int) height+(QuantumMargin >> 1);
      }
      SetPreferencesRaised(context,widgets);
      InitializePreferencesCache(context,widgets,y);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawPreferencesWidget(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const preferences_info = widgets->preferences_info;

  XWidgetInfo
    *const cache_info = widgets->cache_info;

  size_t
    *const state = widgets->state;

  int
    i;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Preferences widget.
      */
      XDrawBeveledButton(display,&windows->widget,apply_info);
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      for (i=0; i < NumberPreferences; i++)
        XDrawBeveledButton(display,&windows->widget,&preferences_info[i]);
      XDrawTriangleEast(display,&windows->widget,cache_info);
      XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void PressPreferencesButton(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XResourceInfo
    *const resource_info = context->resource_info;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const preferences_info = widgets->preferences_info;

  XWidgetInfo
    *const cache_info = widgets->cache_info;

  unsigned int
    *const width = widgets->width;

  char
    *const cache = widgets->cache;

  int
    x,
    y;

  if (MatteIsActive((*apply_info),event.xbutton))
    {
      /*
        User pressed Apply button.
      */
      (*apply_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,apply_info);
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
  TogglePreferencesButton(display,windows,event,preferences_info);
  if (MatteIsActive((*cache_info),event.xbutton))
    {
      /*
        User pressed Cache button.
      */
      x=(*cache_info).x+(int) (*cache_info).width+
        (int) (*cache_info).bevel_width+(QuantumMargin >> 1);
      y=(*cache_info).y+(int) (((*cache_info).height-height) >> 1);
      (*width)=WidgetTextWidth(font_info,cache);
      (void) XClearArea(display,windows->widget.id,x,y,(*width),height,
        False);
      DoublePreferencesUndoCache(resource_info);
      (void) FormatLocaleString(cache,MagickPathExtent,CacheButtonText,
        (unsigned long) resource_info->undo_cache);
      (*cache_info).raised=MagickFalse;
      XDrawTriangleEast(display,&windows->widget,cache_info);
      return;
    }
}

static void ReleasePreferencesButtons(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  XWidgetInfo
    *const cache_info = widgets->cache_info;

  size_t
    *const state = widgets->state;

  if (windows->widget.mapped == MagickFalse)
    return;
  if ((*apply_info).raised == MagickFalse)
    {
      (*state)=ReleasePreferencesButton(windows,event,(*apply_info),(*state));
      (*apply_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,apply_info);
      (*apply_info).raised=MagickFalse;
    }
  if ((*cancel_info).raised == MagickFalse)
    {
      (*state)=ReleasePreferencesButton(windows,event,(*cancel_info),(*state));
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
    }
  if ((*cache_info).raised == MagickFalse)
    {
      (*cache_info).raised=MagickTrue;
      XDrawTriangleEast(display,&windows->widget,cache_info);
    }
}

static void PressPreferencesKey(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  size_t
    *const state = widgets->state;

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
      (*apply_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,apply_info);
      (*state)|=ExitState;
      return;
    }
}

static void MovePreferencesPointer(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const apply_info = widgets->apply_info;

  XWidgetInfo
    *const cancel_info = widgets->cancel_info;

  size_t
    *const state = widgets->state;

  XEvent
    *const event = widgets->event;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
  if ((*state) & InactiveWidgetState)
    return;
  if ((*apply_info).raised == MatteIsActive((*apply_info),(*event).xmotion))
    {
      /*
        Apply button status changed.
      */
      (*apply_info).raised=(*apply_info).raised == MagickFalse;
      XDrawBeveledButton(display,&windows->widget,apply_info);
      return;
    }
  if ((*cancel_info).raised == MatteIsActive((*cancel_info),(*event).xmotion))
    {
      /*
        Cancel button status changed.
      */
      (*cancel_info).raised=(*cancel_info).raised == MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
}

static void HandlePreferencesKeyEvent(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case KeyPress:
    {
      PressPreferencesKey(context,event,widgets);
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    case MotionNotify:
    {
      MovePreferencesPointer(context,widgets);
      break;
    }
    default:
      break;
  }
}

static void HandlePreferencesWindowEvent(const PreferencesContext *context,
  const XEvent event,const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

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
      HandlePreferencesKeyEvent(context,event,widgets);
      break;
    }
  }
}

static void RespondToPreferencesEvents(const PreferencesContext *context,
  const PreferencesWidgets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  XEvent
    *const event = widgets->event;

  do
  {
    UpdatePreferencesButtons(context,widgets);
    RedrawPreferencesWidget(context,widgets);
    /*
      Wait for next event.
    */
    (void) XIfEvent(display,event,XScreenEvent,(char *) windows);
    switch ((*event).type)
    {
      case ButtonPress:
      {
        PressPreferencesButton(context,(*event),widgets);
        break;
      }
      case ButtonRelease:
      {
        ReleasePreferencesButtons(context,(*event),widgets);
        break;
      }
      default:
      {
        HandlePreferencesWindowEvent(context,(*event),widgets);
        break;
      }
    }
  } while (((*state) & ExitState) == 0);
}

MagickPrivate MagickBooleanType XPreferencesWidget(Display *display,
  XResourceInfo *resource_info,XWindows *windows)
{
  char
    cache[MagickPathExtent];

  unsigned int
    height,
    text_width,
    width;

  size_t
    state;

  XEvent
    event;

  XFontStruct
    *font_info;

  XWidgetInfo
    apply_info,
    cache_info,
    cancel_info,
    preferences_info[NumberPreferences];

  const PreferencesWidgets
    widgets = { &cache_info, cache, preferences_info, &cancel_info,
      &apply_info, &width, &state, &event };

  PreferencesContext
    context;

  /*
    Determine Preferences widget attributes.
  */
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(display != (Display *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  assert(windows != (XWindows *) NULL);
  XCheckRefreshWindows(display,windows);
  font_info=windows->widget.font_info;
  text_width=GetPreferencesTextWidth(font_info);
  width=GetPreferencesButtonWidth(font_info);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position Preferences widget.
  */
  PositionPreferencesWidth(windows,font_info,width,text_width);
  PositionPreferencesHeight(windows,font_info,height);
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Preferences widget.
  */
  MapPreferencesWidget(display,windows);
  /*
    Respond to X events.
  */
  state=UpdateConfigurationState;
  XSetCursorState(display,windows,MagickTrue);
  context.resource_info=resource_info;
  context.font_info=font_info;
  context.windows=windows;
  context.height=height;
  context.display=display;
  RespondToPreferencesEvents(&context,&widgets);
  WithdrawBrowserWidget(display,windows);
  if (apply_info.raised)
    return(MagickFalse);
  SavePreferences(resource_info,preferences_info);
  return(MagickTrue);
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
%        const char *task,const MagickOffsetType offset,
%        const MagickSizeType span)
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
%    o offset: Specifies the offset position within the span which represents
%      how much progress has been made in completing a task.
%
%    o span: Specifies the span relative to completing a task.
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
  const char *task,const MagickOffsetType offset,const MagickSizeType span)
{
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

static XWidgetInfo PlaceTextViewExpose(XWidgetInfo expose_info,
  const XWidgetInfo slider_info)
{
  if (slider_info.y > expose_info.y)
    {
      expose_info.height=(unsigned int) (slider_info.y-expose_info.y);
      expose_info.y=slider_info.y-(int) expose_info.height-(int)
        slider_info.bevel_width-1;
    }
  else
    {
      expose_info.height=(unsigned int) (expose_info.y-slider_info.y);
      expose_info.y=slider_info.y+(int) slider_info.height+(int)
        slider_info.bevel_width+1;
    }
  return(expose_info);
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

static void InitializeTextViewScroll(const TextViewContext *context,
  const TextViewWidgets *widgets)
{
  XWindows
    *const windows = context->windows;

  const unsigned int
    lines = context->lines;

  const unsigned int
    height = context->height;

  XFontStruct
    *const font_info = context->font_info;

  XFontStruct
    *const text_info = context->text_info;

  XWidgetInfo
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  XWidgetInfo
    *const dismiss_info = widgets->dismiss_info;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_lines = widgets->visible_lines;

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
    *const north_info = widgets->north_info;

  XWidgetInfo
    *const south_info = widgets->south_info;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  unsigned int
    *const visible_lines = widgets->visible_lines;

  XWidgetInfo
    *const list_info = widgets->list_info;

  XWidgetInfo
    *const selection_info = widgets->selection_info;

  XWidgetInfo
    *const expose_info = widgets->expose_info;

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
