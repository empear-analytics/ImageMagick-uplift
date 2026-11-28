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

  MagickCore private methods of display-image.c: the magnify handlers, from
  XUpdateMagnifyInfo to XMoveMagnifyRightDown; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_MAGNIFY_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_MAGNIFY_PRIVATE_H

static void XUpdateMagnifyInfo(Display *display,XWindows *windows,const int x,
  const int y)
{
  char
    text[MagickPathExtent];

  if (windows->info.mapped != MagickFalse)
    {
      if ((x < (windows->info.x+(int) windows->info.width)) &&
          (y < (windows->info.y+(int) windows->info.height)))
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
    }
  else
    if ((x > (windows->info.x+(int) windows->info.width)) ||
        (y > (windows->info.y+(int) windows->info.height)))
      (void) XMapWindow(display,windows->info.id);
  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+d%+d ",
        windows->magnify.x,windows->magnify.y);
      XInfoWidget(display,windows,text);
    }
}

static void XClampMagnifyPosition(XWindows *windows,int *x,int *y)
{
  if ((*x) < 0)
    (*x)=0;
  else
    if ((*x) >= (int) windows->image.width)
      (*x)=(int) windows->image.width-1;
  if ((*y) < 0)
    (*y)=0;
  else
   if ((*y) >= (int) windows->image.height)
     (*y)=(int) windows->image.height-1;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g n i f y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagnifyImage() magnifies portions of the image as indicated by the pointer.
%  The magnified portion is displayed in a separate window.
%
%  The format of the XMagnifyImage method is:
%
%      void XMagnifyImage(Display *display,XWindows *windows,XEvent *event,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XTrackMagnifyButton(Display *display,XWindows *windows,
  XEvent *event,ExceptionInfo *exception)
{
  int
    x,
    y;

  size_t
    state;

  (void) XCheckDefineCursor(display,windows->image.id,windows->magnify.cursor);
  state=DefaultState;
  x=event->xbutton.x;
  y=event->xbutton.y;
  windows->magnify.x=(int) windows->image.x+x;
  windows->magnify.y=(int) windows->image.y+y;
  do
  {
    /*
      Map and unmap Info widget as text cursor crosses its boundaries.
    */
    XUpdateMagnifyInfo(display,windows,x,y);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    switch (event->type)
    {
      case ButtonPress:
        break;
      case ButtonRelease:
      {
        /*
          User has finished magnifying image.
        */
        x=event->xbutton.x;
        y=event->xbutton.y;
        state|=ExitState;
        break;
      }
      case Expose:
        break;
      case MotionNotify:
      {
        x=event->xmotion.x;
        y=event->xmotion.y;
        break;
      }
      default:
        break;
    }
    /*
      Check boundary conditions.
    */
    XClampMagnifyPosition(windows,&x,&y);
  } while ((state & ExitState) == 0);
}

static void XMagnifyImage(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  /*
    Update magnified image until the mouse button is released.
  */
  XTrackMagnifyButton(display,windows,event,exception);
  /*
    Display magnified image.
  */
  XSetCursorState(display,windows,MagickFalse);
}
typedef struct _MagnifyDigitKey
{
  int
    key,
    base;
} MagnifyDigitKey;

static const MagnifyDigitKey
  MagnifyDigitKeys[] =
  {
    { XK_0, XK_0 },
    { XK_1, XK_0 },
    { XK_2, XK_0 },
    { XK_3, XK_0 },
    { XK_4, XK_0 },
    { XK_5, XK_0 },
    { XK_6, XK_0 },
    { XK_7, XK_0 },
    { XK_8, XK_0 },
    { XK_9, XK_0 },
    { XK_KP_0, XK_KP_0 },
    { XK_KP_1, XK_KP_0 },
    { XK_KP_2, XK_KP_0 },
    { XK_KP_3, XK_KP_0 },
    { XK_KP_4, XK_KP_0 },
    { XK_KP_5, XK_KP_0 },
    { XK_KP_6, XK_KP_0 },
    { XK_KP_7, XK_KP_0 },
    { XK_KP_8, XK_KP_0 },
    { XK_KP_9, XK_KP_0 }
  };

static void XSelectMagnifyDigit(XWindows *windows,const KeySym key_symbol)
{
  const int
    key = (int) key_symbol;

  size_t
    i;

  for (i=0; i < (sizeof(MagnifyDigitKeys)/sizeof(*MagnifyDigitKeys)); i++)
    if (MagnifyDigitKeys[i].key == key)
      {
        windows->magnify.data=(key_symbol-MagnifyDigitKeys[i].base);
        break;
      }
}

static void XMoveMagnifyRightDown(XWindows *windows,const unsigned int quantum,
  const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Right:
    case XK_KP_Right:
    {
      if (windows->magnify.x < (int) (windows->image.ximage->width-1))
        windows->magnify.x+=(int) quantum;
      break;
    }
    case XK_Down:
    case XK_KP_Down:
    {
      if (windows->magnify.y < (int) (windows->image.ximage->height-1))
        windows->magnify.y+=(int) quantum;
      break;
    }
    default:
    {
      XSelectMagnifyDigit(windows,key_symbol);
      break;
    }
  }
}

#endif
