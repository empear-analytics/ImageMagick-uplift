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
  Define declarations.
*/
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
