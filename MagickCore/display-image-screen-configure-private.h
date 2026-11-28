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

  MagickCore private methods of display-image.c: the screen-event configure,
  expose, key-press and map handlers, from XScreenConfigure to
  XTrackScreenMagnify; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_SCREEN_CONFIGURE_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_SCREEN_CONFIGURE_PRIVATE_H

static void XScreenConfigure(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if (event->xconfigure.window == windows->magnify.id)
    {
      unsigned int
        magnify;

      /*
        Magnify window has a new configuration.
      */
      windows->magnify.width=(unsigned int) event->xconfigure.width;
      windows->magnify.height=(unsigned int) event->xconfigure.height;
      if (windows->magnify.mapped == MagickFalse)
        return;
      magnify=1;
      while ((int) magnify <= event->xconfigure.width)
        magnify<<=1;
      while ((int) magnify <= event->xconfigure.height)
        magnify<<=1;
      magnify>>=1;
      if (((int) magnify != event->xconfigure.width) ||
          ((int) magnify != event->xconfigure.height))
        {
          XWindowChanges
            window_changes;

          window_changes.width=(int) magnify;
          window_changes.height=(int) magnify;
          (void) XReconfigureWMWindow(display,windows->magnify.id,
            windows->magnify.screen,(unsigned int) (CWWidth | CWHeight),
            &window_changes);
          return;
        }
      XMakeMagnifyImage(display,windows,exception);
      return;
    }
}

static void XScreenExpose(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if (event->xexpose.window == windows->image.id)
    {
      XRefreshWindow(display,&windows->image,event);
      return;
    }
  if ((event->xexpose.window == windows->pan.id) &&
      (event->xexpose.count == 0))
    {
      XDrawPanRectangle(display,windows);
      return;
    }
  if ((event->xexpose.window == windows->magnify.id) &&
      (event->xexpose.count == 0))
    {
      XMakeMagnifyImage(display,windows,exception);
      return;
    }
}

static void XScreenKeyPress(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  const MagnifyWindowInput
    input = { display, windows, exception };

  if (event->xkey.window != windows->magnify.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &event->xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  XMagnifyWindowCommand(&input,event->xkey.state,key_symbol);
}

static void XScreenMap(Display *display,XWindows *windows,XEvent *event)
{
  if (event->xmap.window == windows->magnify.id)
    {
      windows->magnify.mapped=MagickTrue;
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      return;
    }
  if (event->xmap.window == windows->info.id)
    {
      windows->info.mapped=MagickTrue;
      return;
    }
}

static void XTrackScreenMagnify(XEvent *event,XWindows *windows,
  Display *display,ExceptionInfo *exception)
{
  int
    x,
    y;

  if ((event->xmotion.window == windows->image.id) &&
      (windows->magnify.mapped != MagickFalse))
    {
      /*
        Update magnified image.
      */
      x=event->xmotion.x;
      y=event->xmotion.y;
      XSetMagnifyPosition(windows,x,y);
      XMakeMagnifyImage(display,windows,exception);
    }
}

#endif
