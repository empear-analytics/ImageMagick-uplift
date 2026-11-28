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

  MagickCore private methods of display-image.c: the screen-event predicate
  and the button and client-message handlers, from XPredicate to
  XScreenClientMessage; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_SCREEN_BUTTON_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_SCREEN_BUTTON_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int XPredicate(Display *magick_unused(display),XEvent *event,char *data)
{
  XWindows
    *windows;

  windows=(XWindows *) data;
  if ((event->type == ClientMessage) &&
      (event->xclient.window == windows->image.id))
    return(MagickFalse);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static void XSetMagnifyPosition(XWindows *windows,int x,int y)
{
  if (x < 0)
    x=0;
  else
    if (x >= (int) windows->image.width)
      x=(int) (windows->image.width-1);
  windows->magnify.x=(int) windows->image.x+x;
  if (y < 0)
    y=0;
  else
   if (y >= (int) windows->image.height)
     y=(int) (windows->image.height-1);
  windows->magnify.y=windows->image.y+y;
}

static void XMagnifyAtButton(Display *display,XWindows *windows,
  XEvent *event,ExceptionInfo *exception)
{
  int
    x,
    y;

  /*
    Update magnified image.
  */
  x=event->xbutton.x;
  y=event->xbutton.y;
  XSetMagnifyPosition(windows,x,y);
  if (windows->magnify.mapped == MagickFalse)
    (void) XMapRaised(display,windows->magnify.id);
  XMakeMagnifyImage(display,windows,exception);
  if (event->type == ButtonRelease)
    (void) XWithdrawWindow(display,windows->info.id,
      windows->info.screen);
}

static void XScreenButton(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if ((event->xbutton.button == Button3) &&
      (event->xbutton.state & Mod1Mask))
    {
      /*
        Convert Alt-Button3 to Button2.
      */
      event->xbutton.button=Button2;
      event->xbutton.state&=(unsigned int) (~Mod1Mask);
    }
  if (event->xbutton.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,event->xbutton.window,RevertToParent,
        event->xbutton.time);
      return;
    }
  if (event->xbutton.window == windows->pan.id)
    {
      XPanImage(display,windows,event,exception);
      return;
    }
  if ((event->xbutton.window == windows->image.id) &&
      (event->xbutton.button == Button2))
    {
      XMagnifyAtButton(display,windows,event,exception);
      return;
    }
}

static void XScreenClientMessage(Display *display,XWindows *windows,
  XEvent *event)
{
  /*
    If client window delete message, exit.
  */
  if (event->xclient.message_type != windows->wm_protocols)
    return;
  if (*event->xclient.data.l != (long) windows->wm_delete_window)
    return;
  if (event->xclient.window == windows->magnify.id)
    {
      (void) XWithdrawWindow(display,windows->magnify.id,
        windows->magnify.screen);
      return;
    }
}

#endif
