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

  MagickCore private methods of display-image.c: the screen-event dispatch and
  XSetCropGeometry, from XScreenMotion to XSetCropGeometry; included by that
  file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_SCREEN_EVENT_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_SCREEN_EVENT_PRIVATE_H

static void XScreenMotion(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
  XTrackScreenMagnify(event,windows,display,exception);
}

static void XScreenUnmap(Display *magick_unused(display),XWindows *windows,
  XEvent *event)
{
  magick_unreferenced(display);
  if (event->xunmap.window == windows->magnify.id)
    {
      windows->magnify.mapped=MagickFalse;
      return;
    }
  if (event->xunmap.window == windows->info.id)
    {
      windows->info.mapped=MagickFalse;
      return;
    }
}

static void XScreenOtherEvent(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  switch (event->type)
  {
    case Expose:
    {
      XScreenExpose(display,windows,event,exception);
      break;
    }
    case KeyPress:
    {
      XScreenKeyPress(display,windows,event,exception);
      break;
    }
    case MapNotify:
    {
      XScreenMap(display,windows,event);
      break;
    }
    case MotionNotify:
    {
      XScreenMotion(display,windows,event,exception);
      break;
    }
    case UnmapNotify:
    {
      XScreenUnmap(display,windows,event);
      break;
    }
    default:
      break;
  }
}

static void XDispatchScreenEvent(XEvent *event,Display *display,
  XWindows *windows,ExceptionInfo *exception)
{
  switch (event->type)
  {
    case ButtonPress:
    case ButtonRelease:
    {
      XScreenButton(display,windows,event,exception);
      break;
    }
    case ClientMessage:
    {
      XScreenClientMessage(display,windows,event);
      break;
    }
    case ConfigureNotify:
    {
      XScreenConfigure(display,windows,event,exception);
      break;
    }
    default:
    {
      XScreenOtherEvent(display,windows,event,exception);
      break;
    }
  }
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
%  XScreenEvent() handles global events associated with the Pan and Magnify
%  windows.
%
%  The format of the XScreenEvent function is:
%
%      void XScreenEvent(Display *display,XWindows *windows,XEvent *event,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o event: Specifies a pointer to a X11 XEvent structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XScreenEvent(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  (void) XIfEvent(display,event,XPredicate,(char *) windows);
  if (event->xany.window == windows->command.id)
    return;
  XDispatchScreenEvent(event,display,windows,exception);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S e t C r o p G e o m e t r y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSetCropGeometry() accepts a cropping geometry relative to the Image window
%  and translates it to a cropping geometry relative to the image.
%
%  The format of the XSetCropGeometry method is:
%
%      void XSetCropGeometry(Display *display,XWindows *windows,
%        RectangleInfo *crop_info,Image *image)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o crop_info:  A pointer to a RectangleInfo that defines a region of the
%      Image window to crop.
%
%    o image: the image.
%
*/
static void XSetCropGeometry(Display *display,XWindows *windows,
  RectangleInfo *crop_info,Image *image)
{
  char
    text[MagickPathExtent];

  int
    x,
    y;

  double
    scale_factor;

  unsigned int
    height,
    width;

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display info on cropping rectangle.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %.17gx%.17g%+.20g%+.20g",
        (double) crop_info->width,(double) crop_info->height,(double)
        crop_info->x,(double) crop_info->y);
      XInfoWidget(display,windows,text);
    }
  /*
    Cropping geometry is relative to any previous crop geometry.
  */
  x=0;
  y=0;
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  else
    windows->image.crop_geometry=AcquireString((char *) NULL);
  /*
    Define the crop geometry string from the cropping rectangle.
  */
  scale_factor=(double) width/windows->image.ximage->width;
  if (crop_info->x > 0)
    x+=(int) (scale_factor*crop_info->x+0.5);
  width=(unsigned int) (scale_factor*crop_info->width+0.5);
  if (width == 0)
    width=1;
  scale_factor=(double) height/windows->image.ximage->height;
  if (crop_info->y > 0)
    y+=(int) (scale_factor*crop_info->y+0.5);
  height=(unsigned int) (scale_factor*crop_info->height+0.5);
  if (height == 0)
    height=1;
  (void) FormatLocaleString(windows->image.crop_geometry,MagickPathExtent,
    "%ux%u%+d%+d",width,height,x,y);
}

#endif
