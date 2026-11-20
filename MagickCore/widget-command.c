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
%                CCCC   OOO   M   M  M   M   AAA   N   N  DDDD                %
%               C      O   O  MM MM  MM MM  A   A  NN  N  D   D               %
%               C      O   O  M M M  M M M  AAAAA  N N N  D   D               %
%               C      O   O  M   M  M   M  A   A  N  NN  D   D               %
%                CCCC   OOO   M   M  M   M  A   A  N   N  DDDD                %
%                                                                             %
%                                                                             %
%                    MagickCore X11 Command Widget Methods                    %
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
    if (MatteIsActive((*SelectionInfoReference())[i],event->xbutton) == MagickFalse)
      continue;
    if (i >= (int) windows->command.data)
      {
        (*SelectionInfoReference())[i].raised=MagickFalse;
        XDrawBeveledButton(display,&windows->command,&(*SelectionInfoReference())[i]);
        break;
      }
    (*SubmenuInfoReference())=(*SelectionInfoReference())[i];
    (*SubmenuInfoReference()).active=MagickTrue;
    (*ToggleInfoReference()).y=(*SubmenuInfoReference()).y+(int) ((*SubmenuInfoReference()).height >> 1)-
      (int) ((*ToggleInfoReference()).height >> 1);
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
    if (MatteIsActive((*SelectionInfoReference())[i],event->xbutton) == MagickFalse)
      continue;
    id=i;
    if (id >= (int) windows->command.data)
      {
        (*SelectionInfoReference())[id].raised=MagickTrue;
        XDrawBeveledButton(display,&windows->command,&(*SelectionInfoReference())[id]);
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
        if ((*SelectionInfoReference())[i].raised ==
            MatteIsActive((*SelectionInfoReference())[i],event->xmotion))
          {
            /*
              Button status changed.
            */
            (*SelectionInfoReference())[i].raised=!(*SelectionInfoReference())[i].raised;
            XDrawBeveledButton(display,&windows->command,
              &(*SelectionInfoReference())[i]);
          }
        continue;
      }
    if (MatteIsActive((*SelectionInfoReference())[i],event->xmotion) == MagickFalse)
      continue;
    (*SubmenuInfoReference())=(*SelectionInfoReference())[i];
    (*SubmenuInfoReference()).active=MagickTrue;
    (*ToggleInfoReference()).raised=MagickTrue;
    (*ToggleInfoReference()).y=(*SubmenuInfoReference()).y+(int) ((*SubmenuInfoReference()).height >> 1)-
      (int) ((*ToggleInfoReference()).height >> 1);
    XDrawTriangleEast(display,&windows->command,&(*ToggleInfoReference()));
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
    (*ToggleInfoReference()).raised=MagickFalse;
    if (windows->command.data != 0)
      XDrawTriangleEast(display,&windows->command,&(*ToggleInfoReference()));
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

static void DrawCommandTile(Display *display,XWindows *windows)
{
  Pixmap
    tile_pixmap;

  tile_pixmap=XCreatePixmapFromBitmapData(display,windows->command.id,
    (char *) tile_bits,tile_width,tile_height,1L,0L,1);
  if (tile_pixmap != (Pixmap) NULL)
    {
      (void) XCopyPlane(display,tile_pixmap,windows->command.id,
        windows->command.annotate_context,0,0,tile_width,tile_height,
        (int) ((windows->command.width-tile_width) >> 1),10,1L);
      (void) XFreePixmap(display,tile_pixmap);
    }
}

static void RedrawCommandButtons(Display *display,XWindows *windows,
  const unsigned int number_selections)
{
  int
    i;

  /*
    Draw command buttons.
  */
  DrawCommandTile(display,windows);
  for (i=0; i < (int) number_selections; i++)
  {
    XDrawBeveledButton(display,&windows->command,&(*SelectionInfoReference())[i]);
    if (i >= (int) windows->command.data)
      continue;
    (*ToggleInfoReference()).raised=MagickFalse;
    (*ToggleInfoReference()).y=(*SelectionInfoReference())[i].y+(int) ((*SelectionInfoReference())[i].height >> 1)-
      (int) ((*ToggleInfoReference()).height >> 1);
    XDrawTriangleEast(display,&windows->command,&(*ToggleInfoReference()));
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
    XGetWidgetInfo(selections[i],&(*SelectionInfoReference())[i]);
    (*SelectionInfoReference())[i].center=MagickFalse;
    (*SelectionInfoReference())[i].bevel_width--;
    (*SelectionInfoReference())[i].height=(unsigned int) ((3*height) >> 1);
    (*SelectionInfoReference())[i].x=(QuantumMargin >> 1)+4;
    (*SelectionInfoReference())[i].width=(unsigned int) ((int) windows->command.width-
      ((*SelectionInfoReference())[i].x << 1));
    (*SelectionInfoReference())[i].y=y;
    y+=(int) (*SelectionInfoReference())[i].height+(int)
      ((*SelectionInfoReference())[i].bevel_width << 1)+6;
  }
  XGetWidgetInfo((char *) NULL,&(*ToggleInfoReference()));
  (*ToggleInfoReference()).bevel_width--;
  (*ToggleInfoReference()).width=(unsigned int) (((5*height) >> 3)-
    ((*ToggleInfoReference()).bevel_width << 1));
  (*ToggleInfoReference()).height=(*ToggleInfoReference()).width;
  (*ToggleInfoReference()).x=(*SelectionInfoReference())[0].x+(int) (*SelectionInfoReference())[0].width-
    (int) (*ToggleInfoReference()).width-(int) (QuantumMargin >> 1);
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
      if ((*SelectionInfoReference()) != (XWidgetInfo *) NULL)
        (*SelectionInfoReference())=(XWidgetInfo *) RelinquishMagickMemory((*SelectionInfoReference()));
      (*SelectionInfoReference())=(XWidgetInfo *) AcquireQuantumMemory(number_selections,
        sizeof(*(*SelectionInfoReference())));
      if ((*SelectionInfoReference()) == (XWidgetInfo *) NULL)
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
#endif
