/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               DDDD   IIIII  SSSSS  PPPP   L       AAA   Y   Y               %
%               D   D    I    SS     P   P  L      A   A   Y Y                %
%               D   D    I     SSS   PPPP   L      AAAAA    Y                 %
%               D   D    I       SS  P      L      A   A    Y                 %
%               DDDD   IIIII  SSSSS  P      LLLLL  A   A    Y                 %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                  MagickCore X Window Image Display Methods                  %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                                July 1992                                    %
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
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/composite.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/delegate.h"
#include "MagickCore/display.h"
#include "MagickCore/display-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/shear.h"
#include "MagickCore/segment.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/visual-effects.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

#include "MagickCore/display-image-annotate-private.h"

#include "MagickCore/display-image-crop-private.h"

#include "MagickCore/display-image-draw-private.h"

#include "MagickCore/display-image-apply-private.h"

#include "MagickCore/display-image-matte-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSaveImage() saves an image to a file.
%
%  The format of the XSaveImage method is:
%
%      MagickBooleanType XSaveImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPostscriptFormat(
  const ImageInfo *image_info)
{
  return((MagickBooleanType) ((LocaleCompare(image_info->magick,"EPS") == 0) ||
    (LocaleCompare(image_info->magick,"PDF") == 0) ||
    (LocaleCompare(image_info->magick,"PS") == 0) ||
    (LocaleCompare(image_info->magick,"PS2") == 0)));
}

static void XSelectPageGeometry(Display *display,XWindows *windows,
  ImageInfo *image_info)
{
  if (IsPostscriptFormat(image_info) != MagickFalse)
    {
      char
        geometry[MagickPathExtent];

      const char
        *const PageSizes[] =
        {
          "Letter",
          "Tabloid",
          "Ledger",
          "Legal",
          "Statement",
          "Executive",
          "A3",
          "A4",
          "A5",
          "B4",
          "B5",
          "Folio",
          "Quarto",
          "10x14",
          (char *) NULL
        };

      /*
        Request page geometry from user.
      */
      (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
      if (LocaleCompare(image_info->magick,"PDF") == 0)
        (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
      if (image_info->page != (char *) NULL)
        (void) CopyMagickString(geometry,image_info->page,MagickPathExtent);
      XListBrowserWidget(display,windows,&windows->widget,PageSizes,"Select",
        "Select page geometry:",geometry);
      if (*geometry != '\0')
        image_info->page=GetPageGeometry(geometry);
    }
}

static MagickBooleanType XRequestJPEGQuality(Display *display,
  XWindows *windows,Image *image,ImageInfo *image_info)
{
  if ((LocaleCompare(image_info->magick,"JPEG") == 0) ||
      (LocaleCompare(image_info->magick,"JPG") == 0))
    {
      char
        quality[MagickPathExtent];

      int
        status;

      /*
        Request JPEG quality from user.
      */
      (void) FormatLocaleString(quality,MagickPathExtent,"%.17g",(double)
        image->quality);
      status=XDialogWidget(display,windows,"Save","Enter JPEG quality:",
        quality);
      if (*quality == '\0')
        return(MagickFalse);
      image->quality=StringToUnsignedLong(quality);
      image_info->interlace=status != 0 ? NoInterlace : PlaneInterlace;
    }
  return(MagickTrue);
}

static MagickBooleanType XSeekOverwritePermission(Display *display,
  XWindows *windows,const char *filename)
{
  if (IsPathAccessible(filename) != MagickFalse)
    {
      int
        status;

      /*
        File exists-- seek user's permission before overwriting.
      */
      status=XConfirmWidget(display,windows,"Overwrite",filename);
      if (status <= 0)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static void XGetSaveFilename(const XResourceInfo *resource_info,
  const Image *image,char *filename,ExceptionInfo *exception)
{
  if (resource_info->write_filename != (char *) NULL)
    (void) CopyMagickString(filename,resource_info->write_filename,
      MagickPathExtent);
  else
    {
      char
        path[MagickPathExtent];

      int
        status;

      GetPathComponent(image->filename,HeadPath,path);
      GetPathComponent(image->filename,TailPath,filename);
      if (*path != '\0')
        {
          status=chdir(path);
          if (status == -1)
            (void) ThrowMagickException(exception,GetMagickModule(),
              FileOpenError,"UnableToOpenFile","%s",path);
        }
    }
}

static MagickBooleanType XSaveImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    geometry[MagickPathExtent];

  Image
    *save_image;

  ImageInfo
    *image_info;

  MagickStatusType
    status;

  /*
    Request file name from user.
  */
  XGetSaveFilename(resource_info,image,filename,exception);
  XFileBrowserWidget(display,windows,"Save",filename);
  if (*filename == '\0')
    return(MagickTrue);
  if (XSeekOverwritePermission(display,windows,filename) == MagickFalse)
    return(MagickTrue);
  image_info=CloneImageInfo(resource_info->image_info);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) SetImageInfo(image_info,1,exception);
  if (XRequestJPEGQuality(display,windows,image,image_info) == MagickFalse)
    return(MagickTrue);
  XSelectPageGeometry(display,windows,image_info);
  /*
    Apply image transforms.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  save_image=CloneImage(image,0,0,MagickTrue,exception);
  if (save_image == (Image *) NULL)
    return(MagickFalse);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(&save_image,windows->image.crop_geometry,geometry,
    exception);
  /*
    Write image.
  */
  (void) CopyMagickString(save_image->filename,filename,MagickPathExtent);
  status=WriteImage(image_info,save_image,exception);
  if (status != MagickFalse)
    image->taint=MagickFalse;
  save_image=DestroyImage(save_image);
  image_info=DestroyImageInfo(image_info);
  XSetCursorState(display,windows,MagickFalse);
  return((MagickBooleanType) (status != 0));
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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T i l e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTileImage() loads or deletes a selected tile from a visual image directory.
%  The load or delete command is chosen from a menu.
%
%  The format of the XTileImage method is:
%
%      Image *XTileImage(Display *display,XResourceInfo *resource_info,
%        XWindows *windows,Image *image,XEvent *event,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o tile_image:  XTileImage reads or deletes the tile image
%      and returns it.  A null image is returned if an error occurs.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsTileNameCharacter(const char *q)
{
  return((MagickBooleanType) ((*q != '\xff') && (*q != '\0')));
}

typedef struct _XTileFillInfo
{
  Image
    *image;

  CacheView
    *image_view;

  int
    x_offset,
    y_offset;

  unsigned int
    height,
    width;

  PixelInfo
    pixel;

  ExceptionInfo
    *exception;
} XTileFillInfo;

static void XFillMissingTile(const XTileFillInfo *fill)
{
  Image
    *const image = fill->image;

  CacheView
    *const image_view = fill->image_view;

  const int
    x_offset = fill->x_offset,
    y_offset = fill->y_offset;

  const unsigned int
    height = fill->height,
    width = fill->width;

  const PixelInfo
    pixel = fill->pixel;

  ExceptionInfo
    *const exception = fill->exception;

  int
    i,
    j;

  Quantum
    *s;

  for (i=0; i < (int) height; i++)
  {
    s=GetCacheViewAuthenticPixels(image_view,(ssize_t) x_offset,(ssize_t)
      y_offset+i,width,1,exception);
    if (s == (Quantum *) NULL)
      break;
    for (j=0; j < (int) width; j++)
    {
      SetPixelViaPixelInfo(image,&pixel,s);
      s+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

typedef struct _XTileInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    *image;

  ExceptionInfo
    *exception;

  int
    x,
    y;

  unsigned int
    height,
    width;

  char
    *filename;

  const ModeType
    *TileCommands;

  int
    id;
} XTileInput;

static void XTileUpdateImage(const XTileInput *input)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    *const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  const int
    x = input->x,
    y = input->y;

  const unsigned int
    height = input->height,
    width = input->width;

  char
    filename[MagickPathExtent],
    *p,
    *q;

  int
    tile;

  int
    x_offset,
    y_offset;

  PixelInfo
    pixel;

  /*
    Ensure all the images exist.
  */
  tile=0;
  GetPixelInfo(image,&pixel);
  for (p=image->directory; *p != '\0'; p++)
  {
    CacheView
      *image_view;

    q=p;
    while ((IsTileNameCharacter(q) != MagickFalse) &&
           ((size_t) (q-p+1) < sizeof(filename)))
      q++;
    (void) CopyMagickString(filename,p,(size_t) (q-p+1));
    p=q;
    if (IsPathAccessible(filename) != MagickFalse)
      {
        tile++;
        continue;
      }
    /*
      Overwrite tile with background color.
    */
    x_offset=((int) width*(tile % (((int) image->columns-x)/(int) width))+
      x);
    y_offset=((int) height*(tile/(((int) image->columns-x)/(int) width))+
      y);
    image_view=AcquireAuthenticCacheView(image,exception);
    (void) GetOneCacheViewVirtualPixelInfo(image_view,0,0,&pixel,exception);
    {
      const XTileFillInfo
        fill = { image, image_view, x_offset, y_offset, height, width,
          pixel, exception };

      XFillMissingTile(&fill);
    }
    image_view=DestroyCacheView(image_view);
    tile++;
  }
  windows->image.window_changes.width=(int) image->columns;
  windows->image.window_changes.height=(int) image->rows;
  XConfigureImageColormap(display,resource_info,windows,image,exception);
  (void) XConfigureImage(display,resource_info,windows,image,exception);
}

static Image *XTileLoadImage(const XTileInput *input)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  ExceptionInfo
    *const exception = input->exception;

  char
    *const filename = input->filename;

  Image
    *tile_image;

  /*
    Load tile image.
  */
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(resource_info->image_info->magick,"MIFF",
    MagickPathExtent);
  (void) CopyMagickString(resource_info->image_info->filename,filename,
    MagickPathExtent);
  tile_image=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  return(tile_image);
}

static void XScaleTileEvent(Image *image,XWindows *windows,XEvent *event)
{
  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  /*
    Tile image is relative to montage image configuration.
  */
  x=0;
  y=0;
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  event->xbutton.x+=windows->image.x;
  event->xbutton.x=(int) (scale_factor*event->xbutton.x+x+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  event->xbutton.y+=windows->image.y;
  event->xbutton.y=(int) (scale_factor*event->xbutton.y+y+0.5);
}

typedef struct _XTileLocationTargets
{
  unsigned int
    *width,
    *height;

  int
    *x,
    *y;

  char
    **p;
} XTileLocationTargets;

static MagickBooleanType XLocateTile(Display *display,Image *image,
  XEvent *event,const XTileLocationTargets *targets)
{
  int
    *const x = targets->x,
    *const y = targets->y;

  char
    **const p = targets->p;

  unsigned int
    *const width = targets->width,
    *const height = targets->height;

  int
    i,
    tile;

  /*
    Determine size and location of each tile in the visual image directory.
  */
  (*width)=(unsigned int) image->columns;
  (*height)=(unsigned int) image->rows;
  (*x)=0;
  (*y)=0;
  (void) XParseGeometry(image->montage,x,y,width,height);
  tile=((event->xbutton.y-(*y))/(int) (*height))*(((int) image->columns-(*x))/(int)
    (*width))+(event->xbutton.x-(*x))/(int) (*width);
  if (tile < 0)
    {
      /*
        Button press is outside any tile.
      */
      (void) XBell(display,0);
      return(MagickFalse);
    }
  /*
    Determine file name from the tile directory.
  */
  (*p)=image->directory;
  for (i=tile; (i != 0) && (*(*p) != '\0'); )
  {
    if (*(*p) == '\xff')
      i--;
    (*p)++;
  }
  if (*(*p) == '\0')
    {
      /*
        Button press is outside any tile.
      */
      (void) XBell(display,0);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XTileDeleteImage(const XTileInput *input)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  char
    *const filename = input->filename;

  int
    status;

  /*
    Delete tile image.
  */
  if (IsPathAccessible(filename) == MagickFalse)
    {
      XNoticeWidget(display,windows,"Image file does not exist:",filename);
      return;
    }
  status=XConfirmWidget(display,windows,"Really delete tile",filename);
  if (status <= 0)
    return;
  status=ShredFile(filename) == MagickFalse ? 0 : 1;
  status|=remove_utf8(filename);
  if (status != MagickFalse)
    {
      XNoticeWidget(display,windows,"Unable to delete image file:",
        filename);
      return;
    }
  XTileUpdateImage(input);
}

static Image *XTileCommand(const XTileInput *input)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const TileCommands = input->TileCommands;

  const int
    id = input->id;

  Image
    *tile_image;

  /*
    Perform command for the selected tile.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  tile_image=NewImageList();
  switch (TileCommands[id])
  {
    case TileLoadCommand:
    {
      tile_image=XTileLoadImage(input);
      break;
    }
    case TileNextCommand:
    {
      /*
        Display next image.
      */
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_next_image,CurrentTime);
      break;
    }
    case TileFormerCommand:
    {
      /*
        Display former image.
      */
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_former_image,CurrentTime);
      break;
    }
    case TileDeleteCommand:
    {
      XTileDeleteImage(input);
      break;
    }
    case TileUpdateCommand:
    {
      XTileUpdateImage(input);
      break;
    }
    default:
      break;
  }
  XSetCursorState(display,windows,MagickFalse);
  return(tile_image);
}

static Image *XTileImage(Display *display,XResourceInfo *resource_info,
  XWindows *windows,Image *image,XEvent *event,ExceptionInfo *exception)
{
  const char
    *const VerbMenu[] =
    {
      "Load",
      "Next",
      "Former",
      "Delete",
      "Update",
      (char *) NULL,
    };

  static const ModeType
    TileCommands[] =
    {
      TileLoadCommand,
      TileNextCommand,
      TileFormerCommand,
      TileDeleteCommand,
      TileUpdateCommand
    };

  char
    command[MagickPathExtent],
    filename[MagickPathExtent];

  int
    id,
    x,
    y;

  char
    *p,
    *q;

  unsigned int
    height,
    width;

  XTileInput
    input;

  const XTileLocationTargets
    location = { &width, &height, &x, &y, &p };

  XScaleTileEvent(image,windows,event);
  if (XLocateTile(display,image,event,&location) == MagickFalse)
    return((Image *) NULL);
  /*
    Select a command from the pop-up menu.
  */
  id=XMenuWidget(display,windows,"Tile Verb",VerbMenu,command);
  if (id < 0)
    return((Image *) NULL);
  q=p;
  while ((IsTileNameCharacter(q) != MagickFalse) &&
         ((size_t) (q-p+1) < sizeof(filename)))
    q++;
  (void) CopyMagickString(filename,p,(size_t) (q-p+1));
  input.display=display;
  input.resource_info=resource_info;
  input.windows=windows;
  input.image=image;
  input.exception=exception;
  input.x=x;
  input.y=y;
  input.height=height;
  input.width=width;
  input.filename=filename;
  input.TileCommands=TileCommands;
  input.id=id;
  return(XTileCommand(&input));
}

static void XClampImageOffset(XWindows *windows)
{
  if (windows->image.x < 0)
    windows->image.x=0;
  else
    if ((windows->image.x+(int) windows->image.width) > windows->image.ximage->width)
      windows->image.x=windows->image.ximage->width-(int) windows->image.width;
  if (windows->image.y < 0)
    windows->image.y=0;
  else
    if ((windows->image.y+(int) windows->image.height) > windows->image.ximage->height)
      windows->image.y=windows->image.ximage->height-(int)
        windows->image.height;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T r a n s l a t e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTranslateImage() translates the image within an Image window by one pixel
%  as specified by the key symbol.  If the image has a montage string the
%  translation is respect to the width and height contained within the string.
%
%  The format of the XTranslateImage method is:
%
%      void XTranslateImage(Display *display,XWindows *windows,
%        Image *image,const KeySym key_symbol)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o key_symbol: Specifies a KeySym which indicates which side of the image
%      to trim.
%
*/
static MagickBooleanType XOffsetImageWindowDown(XWindows *windows,
  const KeySym key_symbol,const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_Down:
    case XK_KP_Down:
    {
      windows->image.y+=(int) y_offset;
      break;
    }
    default:
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType XOffsetImageWindowUpRight(XWindows *windows,
  const KeySym key_symbol,const unsigned int x_offset,
  const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Next:
    case XK_Up:
    case XK_KP_Up:
    {
      windows->image.y-=(int) y_offset;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      windows->image.x+=(int) x_offset;
      break;
    }
    default:
    {
      return(XOffsetImageWindowDown(windows,key_symbol,y_offset));
    }
  }
  return(MagickTrue);
}

static MagickBooleanType XOffsetImageWindow(XWindows *windows,
  const KeySym key_symbol,const unsigned int x_offset,
  const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      windows->image.x=(int) windows->image.width/2;
      windows->image.y=(int) windows->image.height/2;
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      windows->image.x-=(int) x_offset;
      break;
    }
    default:
    {
      return(XOffsetImageWindowUpRight(windows,key_symbol,x_offset,y_offset));
    }
  }
  return(MagickTrue);
}

static void XTranslateImage(Display *display,XWindows *windows,
  Image *image,const KeySym key_symbol)
{
  char
    text[MagickPathExtent];

  int
    x,
    y;

  unsigned int
    x_offset,
    y_offset;

  /*
    User specified a pan position offset.
  */
  x_offset=windows->image.width;
  y_offset=windows->image.height;
  if (image->montage != (char *) NULL)
    (void) XParseGeometry(image->montage,&x,&y,&x_offset,&y_offset);
  if (XOffsetImageWindow(windows,key_symbol,x_offset,y_offset) == MagickFalse)
    return;
  /*
    Check boundary conditions.
  */
  XClampImageOffset(windows);
  /*
    Refresh Image window.
  */
  (void) FormatLocaleString(text,MagickPathExtent," %ux%u%+d%+d ",
    windows->image.width,windows->image.height,windows->image.x,
    windows->image.y);
  XInfoWidget(display,windows,text);
  XCheckRefreshWindows(display,windows);
  XDrawPanRectangle(display,windows);
  XRefreshWindow(display,&windows->image,(XEvent *) NULL);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T r i m I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTrimImage() trims the edges from the Image window.
%
%  The format of the XTrimImage method is:
%
%      MagickBooleanType XTrimImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XTrimLeftEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the left edge.
  */
  background=XGetPixel(windows->image.ximage,0,0);
  (*trim_info).width=(size_t) windows->image.ximage->width;
  for (x=0; x < windows->image.ximage->width; x++)
  {
    for (y=0; y < windows->image.ximage->height; y++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (y < windows->image.ximage->height)
      break;
  }
  (*trim_info).x=(ssize_t) x;
}

static void XTrimRightEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the right edge.
  */
  background=XGetPixel(windows->image.ximage,windows->image.ximage->width-1,0);
  for (x=windows->image.ximage->width-1; x != 0; x--)
  {
    for (y=0; y < windows->image.ximage->height; y++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (y < windows->image.ximage->height)
      break;
  }
  (*trim_info).width=(size_t) (x-(*trim_info).x+1);
}

static void XTrimTopEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the top edge.
  */
  background=XGetPixel(windows->image.ximage,0,0);
  (*trim_info).height=(size_t) windows->image.ximage->height;
  for (y=0; y < windows->image.ximage->height; y++)
  {
    for (x=0; x < windows->image.ximage->width; x++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (x < windows->image.ximage->width)
      break;
  }
  (*trim_info).y=(ssize_t) y;
}

static MagickBooleanType XTrimImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  RectangleInfo
    trim_info;

  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Trim edges from image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  /*
    Crop the left edge.
  */
  XTrimLeftEdge(windows,&trim_info);
  if (trim_info.x == (ssize_t) windows->image.ximage->width)
    {
      XSetCursorState(display,windows,MagickFalse);
      return(MagickFalse);
    }
  /*
    Crop the right edge.
  */
  XTrimRightEdge(windows,&trim_info);
  /*
    Crop the top edge.
  */
  XTrimTopEdge(windows,&trim_info);
  /*
    Crop the bottom edge.
  */
  background=XGetPixel(windows->image.ximage,0,windows->image.ximage->height-1);
  for (y=windows->image.ximage->height-1; y != 0; y--)
  {
    for (x=0; x < windows->image.ximage->width; x++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (x < windows->image.ximage->width)
      break;
  }
  trim_info.height=(size_t) (y-trim_info.y+1);
  if (((unsigned int) trim_info.width != windows->image.width) ||
      ((unsigned int) trim_info.height != windows->image.height))
    {
      /*
        Reconfigure Image window as defined by the trimming rectangle.
      */
      XSetCropGeometry(display,windows,&trim_info,image);
      windows->image.window_changes.width=(int) trim_info.width;
      windows->image.window_changes.height=(int) trim_info.height;
      (void) XConfigureImage(display,resource_info,windows,image,exception);
    }
  XSetCursorState(display,windows,MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X V i s u a l D i r e c t o r y I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XVisualDirectoryImage() creates a Visual Image Directory.
%
%  The format of the XVisualDirectoryImage method is:
%
%      Image *XVisualDirectoryImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _DirectoryInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  char
    *filename;

  ExceptionInfo
    *exception;
} DirectoryInput;

static Image *XMontageImageDirectory(const DirectoryInput *input,
  ImageInfo *read_info,Image *images)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *montage_image;

  MontageInfo
    *montage_info;

  char
    *const filename = input->filename;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  /*
    Create the Visual Image Directory.
  */
  montage_info=CloneMontageInfo(read_info,(MontageInfo *) NULL);
  montage_info->pointsize=10;
  if (resource_info->font != (char *) NULL)
    (void) CloneString(&montage_info->font,resource_info->font);
  (void) CopyMagickString(montage_info->filename,filename,MagickPathExtent);
  montage_image=MontageImageList(read_info,montage_info,GetFirstImageInList(
    images),exception);
  images=DestroyImageList(images);
  montage_info=DestroyMontageInfo(montage_info);
  read_info=DestroyImageInfo(read_info);
  XSetCursorState(display,windows,MagickFalse);
  if (montage_image == (Image *) NULL)
    return(montage_image);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_next_image,CurrentTime);
  return(montage_image);
}

typedef struct _DirectoryLoadInput
{
  Display
    *display;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  ImageInfo
    *read_info;

  char
    **filelist;

  int
    number_files;

  MagickBooleanType
    backdrop;

  XResourceInfo
    *background_resources;
} DirectoryLoadInput;

static void XLoadBackdropImage(const DirectoryLoadInput *load_input,
  Image *next_image)
{
  Display
    *const display = load_input->display;

  ExceptionInfo
    *const exception = load_input->exception;

  XWindows
    *const windows = load_input->windows;

  XResourceInfo
    *const background_resources = load_input->background_resources;

  (void) XDisplayBackgroundImage(display,background_resources,next_image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
}

static MagickBooleanType IsDirectoryLoadCancelled(Image *images,const int i,
  const int number_files)
{
  if (images->progress_monitor != (MagickProgressMonitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=SetImageProgress(images,LoadImageTag,(MagickOffsetType) i,
        (MagickSizeType) number_files);
      if (proceed == MagickFalse)
        return(MagickTrue);
    }
  return(MagickFalse);
}

static Image *XLoadDirectoryImages(const DirectoryLoadInput *load_input,
  Image *images)
{
  ExceptionInfo
    *const exception = load_input->exception;

  ImageInfo
    *const read_info = load_input->read_info;

  char
    **const filelist = load_input->filelist;

  const int
    number_files = load_input->number_files;

  const MagickBooleanType
    backdrop = load_input->backdrop;

  Image
    *next_image,
    *thumbnail_image;

  RectangleInfo
    geometry;

  int
    i;

  for (i=0; i < (int) number_files; i++)
  {
    (void) CopyMagickString(read_info->filename,filelist[i],MagickPathExtent);
    filelist[i]=DestroyString(filelist[i]);
    *read_info->magick='\0';
    next_image=ReadImage(read_info,exception);
    CatchException(exception);
    if (next_image == (Image *) NULL)
      continue;
    (void) DeleteImageProperty(next_image,"label");
    (void) SetImageProperty(next_image,"label",InterpretImageProperties(
      read_info,next_image,DefaultTileLabel,exception),exception);
    (void) ParseRegionGeometry(next_image,read_info->size,&geometry,
      exception);
    thumbnail_image=ThumbnailImage(next_image,geometry.width,
      geometry.height,exception);
    if (thumbnail_image != (Image *) NULL)
      {
        next_image=DestroyImage(next_image);
        next_image=thumbnail_image;
      }
    if (backdrop)
      XLoadBackdropImage(load_input,next_image);
    AppendImageToList(&images,next_image);
    if (IsDirectoryLoadCancelled(images,i,number_files) != MagickFalse)
      break;
  }
  return(images);
}

typedef struct _DirectoryListTargets
{
  char
    ***filelist;

  int
    *number_files;
} DirectoryListTargets;

static MagickBooleanType XExpandDirectoryFiles(char *filenames,
  const DirectoryListTargets *targets)
{
  char
    ***const filelist = targets->filelist;

  int
    *const number_files = targets->number_files;

  MagickStatusType
    status;

  /*
    Expand the filenames.
  */
  (*filelist)=(char **) AcquireMagickMemory(sizeof(**filelist));
  if ((*filelist) == (char **) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        filenames);
      return(MagickFalse);
    }
  (*number_files)=1;
  (*filelist)[0]=filenames;
  status=ExpandFilenames(number_files,filelist);
  if ((status == MagickFalse) || ((*number_files) == 0))
    {
      if ((*number_files) == 0)
        ThrowXWindowException(ImageError,"NoImagesWereFound",filenames)
      else
        ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
          filenames);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static Image *XVisualDirectoryImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,ExceptionInfo *exception)
{
#define TileImageTag  "Scale/Image"
#define XClientName  "montage"

  char
    **filelist;

  Image
    *images;

  ImageInfo
    *read_info;

  int
    number_files;

  MagickBooleanType
    backdrop;

  static char
    filename[MagickPathExtent] = "\0",
    filenames[MagickPathExtent] = "*";

  XResourceInfo
    background_resources;

  const DirectoryInput
    input = { display, resource_info, windows, filename, exception };

  const DirectoryListTargets
    list_targets = { &filelist, &number_files };

  /*
    Request file name from user.
  */
  XFileBrowserWidget(display,windows,"Directory",filenames);
  if (*filenames == '\0')
    return((Image *) NULL);
  if (XExpandDirectoryFiles(filenames,&list_targets) == MagickFalse)
    return((Image *) NULL);
  /*
    Set image background resources.
  */
  background_resources=(*resource_info);
  background_resources.window_id=AcquireString("");
  (void) FormatLocaleString(background_resources.window_id,MagickPathExtent,
    "0x%lx",windows->image.id);
  background_resources.backdrop=MagickTrue;
  /*
    Read each image and convert them to a tile.
  */
  backdrop=(MagickBooleanType) ((windows->visual_info->klass == TrueColor) ||
    (windows->visual_info->klass == DirectColor));
  read_info=CloneImageInfo(resource_info->image_info);
  (void) SetImageOption(read_info,"jpeg:size","120x120");
  (void) CloneString(&read_info->size,DefaultTileGeometry);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  images=NewImageList();
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  {
    const DirectoryLoadInput
      load_input = { display, windows, exception, read_info, filelist,
        number_files, backdrop, &background_resources };

    images=XLoadDirectoryImages(&load_input,images);
  }
  filelist=(char **) RelinquishMagickMemory(filelist);
  if (images == (Image *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      XSetCursorState(display,windows,MagickFalse);
      ThrowXWindowException(ImageError,"NoImagesWereLoaded",filenames);
      return((Image *) NULL);
    }
  /*
    Create the Visual Image Directory.
  */
  return(XMontageImageDirectory(&input,read_info,images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D i s p l a y B a c k g r o u n d I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDisplayBackgroundImage() displays an image in the background of a window.
%
%  The format of the XDisplayBackgroundImage method is:
%
%      MagickBooleanType XDisplayBackgroundImage(Display *display,
%        XResourceInfo *resource_info,Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XLookupBackgroundWindowId(char *window_id,XWindowInfo *window_info,
  const Window root_window,Display *display)
{
  if (LocaleCompare(window_id,"root") == 0)
    (*window_info).id=root_window;
  else
    {
      if (isdigit((int) ((unsigned char) *window_id)) != 0)
        (*window_info).id=XWindowByID(display,root_window,
          (Window) strtol((char *) window_id,(char **) NULL,0));
      if ((*window_info).id == (Window) NULL)
        (*window_info).id=XWindowByName(display,root_window,window_id);
    }
}

static MagickBooleanType XFindBackgroundWindow(Display *display,
  char *window_id,const Window root_window,XWindowInfo *window_info)
{
  XLookupBackgroundWindowId(window_id,window_info,root_window,display);
  if ((*window_info).id == (Window) NULL)
    {
      ThrowXWindowException(XServerError,"NoWindowWithSpecifiedIDExists",
        window_id);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XGetBackgroundVisualType(Display *display,
  const XWindowInfo *window_info,XWindowAttributes *window_attributes,
  char *visual_type)
{
  int
    status;

  (*window_attributes).width=XDisplayWidth(display,XDefaultScreen(display));
  (*window_attributes).height=XDisplayHeight(display,XDefaultScreen(display));
  (void) CopyMagickString(visual_type,"default",MagickPathExtent);
  status=XGetWindowAttributes(display,(*window_info).id,window_attributes);
  if (status != 0)
    (void) FormatLocaleString(visual_type,MagickPathExtent,"0x%lx",
      XVisualIDFromVisual((*window_attributes).visual));
}

static void XFitBackgroundWindow(const XWindowAttributes *window_attributes,
  XWindowInfo *window_info)
{
  char
    geometry[MagickPathExtent];

  RectangleInfo
    geometry_info;

  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>",
    (*window_attributes).width,(*window_attributes).height);
  geometry_info.width=(*window_info).width;
  geometry_info.height=(*window_info).height;
  geometry_info.x=(ssize_t) (*window_info).x;
  geometry_info.y=(ssize_t) (*window_info).y;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  (*window_info).width=(unsigned int) geometry_info.width;
  (*window_info).height=(unsigned int) geometry_info.height;
  (*window_info).x=(int) geometry_info.x;
  (*window_info).y=(int) geometry_info.y;
}

typedef struct _BackgroundTargets
{
  XStandardColormap
    **map_info;

  XPixelInfo
    *pixel;

  XResourceInfo
    *resources;

  XVisualInfo
    **visual_info;

  XWindowInfo
    *window_info;
} BackgroundTargets;

static void XInitializeBackgroundVisual(Display *display,const Image *image,
  char *visual_type,const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Allocate standard colormap.
  */
  (*map_info)=XAllocStandardColormap();
  if ((*map_info) == (XStandardColormap *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
      image->filename);
  (*map_info)->colormap=(Colormap) NULL;
  (*pixel).pixels=(unsigned long *) NULL;
  /*
    Initialize visual info.
  */
  (*resources).map_type=(char *) NULL;
  (*resources).visual_type=visual_type;
  (*visual_info)=XBestVisualInfo(display,(*map_info),resources);
  if ((*visual_info) == (XVisualInfo *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToGetVisual",
      (*resources).visual_type);
  /*
    Initialize window info.
  */
  (*window_info).ximage=(XImage *) NULL;
  (*window_info).matte_image=(XImage *) NULL;
  (*window_info).pixmap=(Pixmap) NULL;
  (*window_info).matte_pixmap=(Pixmap) NULL;
}

static void XInitializeBackgroundColormap(XResourceInfo *resources,
  Display *display,XVisualInfo **visual_info,Image *image,
  XStandardColormap **map_info,XPixelInfo *pixel,ExceptionInfo *exception)
{
  (*resources).colormap=SharedColormap;
  XMakeStandardColormap(display,(*visual_info),resources,image,(*map_info),
    pixel,exception);
}

static void XInitializeBackgroundContext(XPixelInfo *pixel,Display *display,
  XWindowInfo *window_info,Image *image)
{
  XGCValues
    context_values;

  context_values.background=(*pixel).foreground_color.pixel;
  context_values.foreground=(*pixel).background_color.pixel;
  (*pixel).annotate_context=XCreateGC(display,(*window_info).id,
    (size_t) (GCBackground | GCForeground),&context_values);
  if ((*pixel).annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      image->filename);
}

static void XInitializeBackgroundAttributes(XWindowInfo *window_info,
  Display *display,XVisualInfo **visual_info,XStandardColormap **map_info,
  XPixelInfo *pixel,XResourceInfo *resources)
{
  (*window_info).name=AcquireString("\0");
  (*window_info).icon_name=AcquireString("\0");
  XGetWindowInfo(display,(*visual_info),(*map_info),pixel,(XFontStruct *) NULL,
    resources,window_info);
}

static void XCreateBackgroundWindowInfo(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Initialize Standard Colormap.
  */
  XInitializeBackgroundColormap(resources,display,visual_info,image,map_info,
    pixel,exception);
  /*
    Graphic context superclass.
  */
  XInitializeBackgroundContext(pixel,display,window_info,image);
  /*
    Initialize Image window attributes.
  */
  XInitializeBackgroundAttributes(window_info,display,visual_info,map_info,
    pixel,resources);
}

static void XSizeBackgroundWindow(const Image *image,XWindowInfo *window_info)
{
  (*window_info).width=(unsigned int) image->columns;
  (*window_info).height=(unsigned int) image->rows;
  if ((image->columns != (*window_info).width) ||
      (image->rows != (*window_info).height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image->filename);
}

static void XMakeBackgroundXImage(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  int
    status;

  status=XMakeImage(display,resources,window_info,image,(*window_info).width,
    (*window_info).height,exception) == MagickFalse ? 0 : 1;
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image->filename);
  (*window_info).x=0;
  (*window_info).y=0;
}

typedef struct _BackgroundInput
{
  Display
    *display;

  Image
    *image;

  const XWindowAttributes
    *window_attributes;
} BackgroundInput;

static void XCenterBackdropImage(XResourceInfo *resources,
  XWindowInfo *window_info,const XWindowAttributes *window_attributes,
  int *width,int *height)
{
  if ((*resources).backdrop != MagickFalse)
    {
      /*
        Center image on window.
      */
      (*window_info).x=((*window_attributes).width/2)-
        ((*window_info).ximage->width/2);
      (*window_info).y=((*window_attributes).height/2)-
        ((*window_info).ximage->height/2);
      (*width)=(*window_attributes).width;
      (*height)=(*window_attributes).height;
    }
}

static void XAdjustBackgroundSize(const BackgroundInput *input,
  const BackgroundTargets *targets,int *width,int *height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  const XWindowAttributes
    *const window_attributes = input->window_attributes;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  (*width)=(int) (*window_info).width;
  (*height)=(int) (*window_info).height;
  XCenterBackdropImage(resources,window_info,window_attributes,width,height);
  if (((*resources).image_geometry != (char *) NULL) &&
      (*(*resources).image_geometry != '\0'))
    {
      char
        default_geometry[MagickPathExtent];

      int
        flags,
        gravity;

      XSizeHints
        *size_hints;

      /*
        User specified geometry.
      */
      size_hints=XAllocSizeHints();
      if (size_hints == (XSizeHints *) NULL)
        ThrowXWindowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",image->filename);
      size_hints->flags=0L;
      (void) FormatLocaleString(default_geometry,MagickPathExtent,"%dx%d",
        (*width),(*height));
      flags=XWMGeometry(display,(*visual_info)->screen,
        (*resources).image_geometry,default_geometry,
        (*window_info).border_width,size_hints,&(*window_info).x,
        &(*window_info).y,width,height,&gravity);
      if (flags & (XValue | YValue))
        {
          (*width)=(*window_attributes).width;
          (*height)=(*window_attributes).height;
        }
      (void) XFree((void *) size_hints);
    }
}

static void XCreateBackgroundPixmap(XWindowInfo *window_info,Display *display,
  const int width,const int height,Image *image)
{
  (*window_info).pixmap=XCreatePixmap(display,(*window_info).id,
    (unsigned int) width,(unsigned int) height,(*window_info).depth);
  if ((*window_info).pixmap == (Pixmap) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXPixmap",
      image->filename);
}

static void XShowBackgroundPixmap(const int width,XWindowInfo *window_info,
  const int height,Display *display,Image *image)
{
  size_t
    delay;

  if (((unsigned int) width > (*window_info).width) ||
      ((unsigned int) height > (*window_info).height))
    (void) XFillRectangle(display,(*window_info).pixmap,
      (*window_info).annotate_context,0,0,(unsigned int) width,
      (unsigned int) height);
  (void) XPutImage(display,(*window_info).pixmap,
    (*window_info).annotate_context,(*window_info).ximage,0,0,
    (*window_info).x,(*window_info).y,(unsigned int) (*window_info).width,
    (unsigned int) (*window_info).height);
  (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
    (*window_info).pixmap);
  (void) XClearWindow(display,(*window_info).id);
  delay=1000*image->delay/(size_t) MagickMax(image->ticks_per_second,1L);
  XDelay(display,delay == 0UL ? 10UL : delay);
  (void) XSync(display,MagickFalse);
}

static void XDisplayBackgroundPixmap(const BackgroundInput *input,
  const BackgroundTargets *targets,const int width,const int height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Create the X pixmap.
  */
  XCreateBackgroundPixmap(window_info,display,width,height,image);
  /*
    Display pixmap on the window.
  */
  XShowBackgroundPixmap(width,window_info,height,display,image);
}

MagickExport MagickBooleanType XDisplayBackgroundImage(Display *display,
  XResourceInfo *resource_info,Image *image,ExceptionInfo *exception)
{
  char
    visual_type[MagickPathExtent];

  int
    height,
    width;

  static XPixelInfo
    pixel;

  static XStandardColormap
    *map_info;

  static XVisualInfo
    *visual_info = (XVisualInfo *) NULL;

  static XWindowInfo
    window_info;

  Window
    root_window;

  XResourceInfo
    resources;

  BackgroundInput
    input;

  const BackgroundTargets
    targets = { &map_info, &pixel, &resources, &visual_info, &window_info };

  XWindowAttributes
    window_attributes;

  /*
    Determine target window.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  resources=(*resource_info);
  window_info.id=(Window) NULL;
  root_window=XRootWindow(display,XDefaultScreen(display));
  if (XFindBackgroundWindow(display,resources.window_id,root_window,
      &window_info) == MagickFalse)
    return(MagickFalse);
  /*
    Determine window visual id.
  */
  XGetBackgroundVisualType(display,&window_info,&window_attributes,visual_type);
  if (visual_info == (XVisualInfo *) NULL)
    XInitializeBackgroundVisual(display,image,visual_type,&targets);
  /*
    Free previous root colors.
  */
  if (window_info.id == root_window)
    (void) XDestroyWindowColors(display,root_window);
  XCreateBackgroundWindowInfo(display,image,exception,&targets);
  /*
    Create the X image.
  */
  XSizeBackgroundWindow(image,&window_info);
  XFitBackgroundWindow(&window_attributes,&window_info);
  XMakeBackgroundXImage(display,image,exception,&targets);
  if (resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",image->filename,(double) image->scene,
        (double) image->columns,(double) image->rows);
      if (image->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",image->magick);
    }
  /*
    Adjust image dimensions as specified by backdrop or geometry options.
  */
  input.display=display;
  input.image=image;
  input.window_attributes=(&window_attributes);
  XAdjustBackgroundSize(&input,&targets,&width,&height);
  XDisplayBackgroundPixmap(&input,&targets,width,height);
  return((MagickBooleanType) (window_info.id == root_window));
}

static void XPositionCommandAndWidget(Display *display,XWindows *windows,
  const XEvent event)
{
  XWindowChanges
    window_changes;

  if ((windows->command.geometry == (char *) NULL) &&
      (windows->command.mapped == MagickFalse))
    {
      windows->command.x=event.xconfigure.x-(int)
        windows->command.width-25;
      windows->command.y=event.xconfigure.y;
      XConstrainWindowPosition(display,&windows->command);
      window_changes.x=windows->command.x;
      window_changes.y=windows->command.y;
      (void) XReconfigureWMWindow(display,windows->command.id,
        windows->command.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
  if ((windows->widget.geometry == (char *) NULL) &&
      (windows->widget.mapped == MagickFalse))
    {
      windows->widget.x=event.xconfigure.x+
        event.xconfigure.width/10;
      windows->widget.y=event.xconfigure.y+
        event.xconfigure.height/10;
      XConstrainWindowPosition(display,&windows->widget);
      window_changes.x=windows->widget.x;
      window_changes.y=windows->widget.y;
      (void) XReconfigureWMWindow(display,windows->widget.id,
        windows->widget.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
}

static void XPositionMagnifyAndPan(Display *display,XWindows *windows,
  const XEvent event)
{
  XWindowChanges
    window_changes;

  if ((windows->magnify.geometry == (char *) NULL) &&
      (windows->magnify.mapped == MagickFalse))
    {
      windows->magnify.x=event.xconfigure.x+
        event.xconfigure.width+25;
      windows->magnify.y=event.xconfigure.y;
      XConstrainWindowPosition(display,&windows->magnify);
      window_changes.x=windows->magnify.x;
      window_changes.y=windows->magnify.y;
      (void) XReconfigureWMWindow(display,windows->magnify.id,
        windows->magnify.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
  if ((windows->pan.geometry == (char *) NULL) &&
      (windows->pan.mapped == MagickFalse))
    {
      windows->pan.x=event.xconfigure.x+(int)
        event.xconfigure.width+25;
      windows->pan.y=event.xconfigure.y+(int)
        windows->magnify.height+50;
      XConstrainWindowPosition(display,&windows->pan);
      window_changes.x=windows->pan.x;
      window_changes.y=windows->pan.y;
      (void) XReconfigureWMWindow(display,windows->pan.id,
        windows->pan.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
}

static void XUpdatePanWindow(Display *display,XWindows *windows,
  const XEvent event)
{
  if ((event.xconfigure.width < windows->image.ximage->width) ||
      (event.xconfigure.height < windows->image.ximage->height))
    {
      (void) XMapRaised(display,windows->pan.id);
      XDrawPanRectangle(display,windows);
    }
  else
    if (windows->pan.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->pan.id,
        windows->pan.screen);
}

typedef struct _DisplayEventContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  XPixelInfo
    *pixel;

  XStandardColormap
    *map_info;

  XWindowInfo
    **magick_windows;

  unsigned int
    number_windows;

  MagickStatusType
    context_mask;

  size_t
    *state;

  Window
    root_window;

  const char
    *const *CommandMenu;

  char
    *command;

  XVisualInfo
    *visual_info;

  const char
    *const *VirtualMenu;

  DisplayCommand
    *VirtualCommands;

  const char
    *const *ShortCutsMenu;

  DisplayCommand
    *ShortCutsCommands;

  Image
    **image;

  time_t
    update_time;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;

  const char *const
    **Menus;

  DisplayCommand
    **Commands;

  XFontStruct
    *font_info;

  char
    **argv;

  int
    argc;

  XClassHint
    *class_hints;

  XWMHints
    *manager_hints;

  XResourceInfo
    *icon_resources;
} DisplayEventContext;

static void XUpdateStaticImageWindow(XWindows *windows,const XEvent event,
  Display *display,XResourceInfo *resource_info,Image *display_image,
  ExceptionInfo *exception)
{
  if (windows->image.mapped != MagickFalse &&
      windows->image.stasis != MagickFalse)
    {
      /*
        Update image window configuration.
      */
      windows->image.window_changes.width=event.xconfigure.width;
      windows->image.window_changes.height=event.xconfigure.height;
      (void) XConfigureImage(display,resource_info,windows,
        display_image,exception);
    }
}

static void XConfigureImageWindow(const DisplayEventContext *context,
  const XEvent event,Image *display_image,const XPoint vid_info)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  /*
    Image window has a new configuration.
  */
  if (event.xconfigure.send_event != 0)
    {
      /*
        Position the transient windows relative of the Image window.
      */
      XPositionCommandAndWidget(display,windows,event);
      XPositionMagnifyAndPan(display,windows,event);
    }
  if ((event.xconfigure.width == (int) windows->image.width) &&
      (event.xconfigure.height == (int) windows->image.height))
    return;
  windows->image.width=(unsigned int) event.xconfigure.width;
  windows->image.height=(unsigned int) event.xconfigure.height;
  windows->image.x=0;
  windows->image.y=0;
  if (display_image->montage != (char *) NULL)
    {
      windows->image.x=vid_info.x;
      windows->image.y=vid_info.y;
    }
  XUpdateStaticImageWindow(windows,event,display,resource_info,display_image,
    exception);
  /*
    Update pan window configuration.
  */
  XUpdatePanWindow(display,windows,event);
}

static MagickStatusType XConfigureMagnifyWindow(
  const DisplayEventContext *context,const XEvent event,Image *display_image,
  MagickStatusType status)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  unsigned int
    magnify;

  XWindowChanges
    window_changes;

  /*
    Magnify window has a new configuration.
  */
  windows->magnify.width=(unsigned int) event.xconfigure.width;
  windows->magnify.height=(unsigned int) event.xconfigure.height;
  if (windows->magnify.mapped == MagickFalse)
    return(status);
  magnify=1;
  while ((int) magnify <= event.xconfigure.width)
    magnify<<=1;
  while ((int) magnify <= event.xconfigure.height)
    magnify<<=1;
  magnify>>=1;
  if (((int) magnify != event.xconfigure.width) ||
      ((int) magnify != event.xconfigure.height))
    {
      window_changes.width=(int) magnify;
      window_changes.height=(int) magnify;
      (void) XReconfigureWMWindow(display,windows->magnify.id,
        windows->magnify.screen,(unsigned int) (CWWidth | CWHeight),
        &window_changes);
      return(status);
    }
  if (windows->magnify.mapped != MagickFalse &&
      windows->magnify.stasis != MagickFalse)
    {
      status=XMakeImage(display,resource_info,&windows->magnify,
        display_image,windows->magnify.width,windows->magnify.height,
        exception);
      XMakeMagnifyImage(display,windows,exception);
    }
  return(status);
}

static void XConfigurePanWindow(XWindows *windows,const XEvent event)
{
  /*
    Pan icon window has a new configuration.
  */
  if (event.xconfigure.send_event != 0)
    {
      windows->pan.x=event.xconfigure.x;
      windows->pan.y=event.xconfigure.y;
    }
  windows->pan.width=(unsigned int) event.xconfigure.width;
  windows->pan.height=(unsigned int) event.xconfigure.height;
}

static void XUpdateWindowColormaps(const DisplayEventContext *context,
  XGCValues *context_values)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  XWindowInfo
    **const magick_windows = context->magick_windows;

  const unsigned int
    number_windows = context->number_windows;

  const MagickStatusType
    context_mask = context->context_mask;

  int
    i;

  /*
    Update graphic context and window colormap.
  */
  for (i=0; i < (int) number_windows; i++)
  {
    if (magick_windows[i]->id == windows->icon.id)
      continue;
    (*context_values).background=pixel->background_color.pixel;
    (*context_values).foreground=pixel->foreground_color.pixel;
    (void) XChangeGC(display,magick_windows[i]->annotate_context,
      context_mask,context_values);
    (void) XChangeGC(display,magick_windows[i]->widget_context,
      context_mask,context_values);
    (*context_values).background=pixel->foreground_color.pixel;
    (*context_values).foreground=pixel->background_color.pixel;
    (*context_values).plane_mask=(*context_values).background ^
      (*context_values).foreground;
    (void) XChangeGC(display,magick_windows[i]->highlight_context,
      (size_t) (context_mask | GCPlaneMask),
      context_values);
    magick_windows[i]->attributes.background_pixel=
      pixel->background_color.pixel;
    magick_windows[i]->attributes.border_pixel=
      pixel->border_color.pixel;
    magick_windows[i]->attributes.colormap=map_info->colormap;
    (void) XChangeWindowAttributes(display,magick_windows[i]->id,
      (unsigned long) magick_windows[i]->mask,
      &magick_windows[i]->attributes);
  }
  if (windows->pan.mapped != MagickFalse)
    {
      (void) XSetWindowBackgroundPixmap(display,windows->pan.id,
        windows->pan.pixmap);
      (void) XClearWindow(display,windows->pan.id);
      XDrawPanRectangle(display,windows);
    }
  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
}

static Image *XDropImage(const DisplayEventContext *context,const XEvent event,
  Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  const Window
    root_window = context->root_window;

  Atom
    selection,
    type;

  int
    format,
    status;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  /*
    Display image named by the Drag-and-Drop selection.
  */
  if ((*event.xclient.data.l != 2) && (*event.xclient.data.l != 128))
    return(nexus);
  selection=XInternAtom(display,"DndSelection",MagickFalse);
  status=XGetWindowProperty(display,root_window,selection,0L,(long)
    MagickPathExtent,MagickFalse,(Atom) AnyPropertyType,&type,&format,
    &length,&after,&data);
  if ((status != Success) || (length == 0))
    return(nexus);
  if (*event.xclient.data.l == 2)
    {
      /*
        Offix DND.
      */
      (void) CopyMagickString(resource_info->image_info->filename,
        (char *) data,MagickPathExtent);
    }
  else
    {
      /*
        XDND.
      */
      if (strncmp((char *) data, "file:", 5) != 0)
        {
          (void) XFree((void *) data);
          return(nexus);
        }
      (void) CopyMagickString(resource_info->image_info->filename,
        ((char *) data)+5,MagickPathExtent);
    }
  nexus=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if (nexus != (Image *) NULL)
    *state|=NextImageState | ExitState;
  (void) XFree((void *) data);
  return(nexus);
}

typedef struct _DisplayEventTargets
{
  Image
    **nexus;

  Image
    **display_image;

  XEvent
    *event;

  XPoint
    *vid_info;

  time_t
    *timer;

  XGCValues
    *context_values;

  MagickStatusType
    *status;

  WarningHandler
    *warning_handler;

  XWindows
    **windows;

  unsigned int
    *number_windows;
} DisplayEventTargets;

static void XSelectVirtualCommand(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const command = context->command;

  const char
    *const *const VirtualMenu = context->VirtualMenu;

  DisplayCommand
    *const VirtualCommands = context->VirtualCommands;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  int
    entry;

  /*
    Select a command from the Virtual menu.
  */
  entry=XMenuWidget(display,windows,"Commands",VirtualMenu,
    command);
  if (entry >= 0)
    (*nexus)=XMagickCommand(display,resource_info,windows,
      VirtualCommands[entry],display_image,exception);
}

static void XPressImageButton1(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const char
    *const *const CommandMenu = context->CommandMenu;

  if (resource_info->immutable)
    {
      XSelectVirtualCommand(context,targets);
      return;
    }
  /*
    Map/unmap Command widget.
  */
  if (windows->command.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->command.id,
      windows->command.screen);
  else
    {
      (void) XCommandWidget(display,windows,CommandMenu,
        (XEvent *) NULL);
      (void) XMapRaised(display,windows->command.id);
    }
}

static void XPressImageButton3(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  char
    *const command = context->command;

  const char
    *const *const ShortCutsMenu = context->ShortCutsMenu;

  DisplayCommand
    *const ShortCutsCommands = context->ShortCutsCommands;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  XPoint
    *const vid_info = targets->vid_info;

  int
    entry;

  if (resource_info->immutable)
    {
      XSelectVirtualCommand(context,targets);
      return;
    }
  if ((*display_image)->montage != (char *) NULL)
    {
      /*
        Open or delete a tile from a visual image directory.
      */
      (*nexus)=XTileImage(display,resource_info,windows,
        (*display_image),event,exception);
      if ((*nexus) != (Image *) NULL)
        *state|=MontageImageState | NextImageState | ExitState;
      (*vid_info).x=(short int) windows->image.x;
      (*vid_info).y=(short int) windows->image.y;
      return;
    }
  /*
    Select a command from the Short Cuts menu.
  */
  entry=XMenuWidget(display,windows,"Short Cuts",ShortCutsMenu,
    command);
  if (entry >= 0)
    (*nexus)=XMagickCommand(display,resource_info,windows,
      ShortCutsCommands[entry],display_image,exception);
}

static void XPressImageWindow(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  switch ((*event).xbutton.button)
  {
    case Button1:
    {
      XPressImageButton1(context,targets);
      break;
    }
    case Button2:
    {
      /*
        User pressed the image magnify button.
      */
      (void) XMagickCommand(display,resource_info,windows,ZoomCommand,
        display_image,exception);
      XMagnifyImage(display,windows,event,exception);
      break;
    }
    case Button3:
    {
      XPressImageButton3(context,targets);
      break;
    }
    case Button4:
    {
      /*
        Wheel up.
      */
      XTranslateImage(display,windows,*image,XK_Up);
      break;
    }
    case Button5:
    {
      /*
        Wheel down.
      */
      XTranslateImage(display,windows,*image,XK_Down);
      break;
    }
    default:
      break;
  }
}

static void XPressMagnifyWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const command = context->command;

  const char
    *const MagnifyMenu[] =
    {
      "2",
      "4",
      "5",
      "6",
      "7",
      "8",
      "9",
      "3",
      (char *) NULL,
    };

  int
    factor;

  const MagnifyWindowInput
    input = { display, windows, exception };

  static KeySym
    MagnifyCommands[] =
    {
      XK_2,
      XK_4,
      XK_5,
      XK_6,
      XK_7,
      XK_8,
      XK_9,
      XK_3
    };

  /*
    Select a magnify factor from the pop-up menu.
  */
  factor=XMenuWidget(display,windows,"Magnify",MagnifyMenu,command);
  if (factor >= 0)
    XMagnifyWindowCommand(&input,0,MagnifyCommands[factor]);
}

static void XPressPanWindow(const DisplayEventContext *context,XEvent *event)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  switch ((*event).xbutton.button)
  {
    case Button4:
    {
      /*
        Wheel up.
      */
      XTranslateImage(display,windows,*image,XK_Up);
      break;
    }
    case Button5:
    {
      /*
        Wheel down.
      */
      XTranslateImage(display,windows,*image,XK_Down);
      break;
    }
    default:
    {
      XPanImage(display,windows,event,exception);
      break;
    }
  }
}

static void XImageWindowKeyPress(const DisplayEventContext *context,
  const XEvent event,const KeySym key_symbol,const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  DisplayCommand
    display_command;

  const ImageWindowKeyInput
    input = { display, resource_info, windows, display_image, exception };

  display_command=XImageWindowCommand(&input,event.xkey.state,key_symbol);
  if (display_command != NullCommand)
    (*nexus)=XMagickCommand(display,resource_info,windows,
      display_command,display_image,exception);
}

static void XPanWindowKeyPress(const DisplayEventContext *context,
  const KeySym key_symbol)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  if ((key_symbol == XK_q) || (key_symbol == XK_Escape))
    (void) XWithdrawWindow(display,windows->pan.id,
      windows->pan.screen);
  else
    if ((key_symbol == XK_F1) || (key_symbol == XK_Help))
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Pan",ImagePanHelp);
      }
    else
      XTranslateImage(display,windows,*image,key_symbol);
}

static Image *XMapImageWindow(const DisplayEventContext *context,
  Image *display_image,Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XStandardColormap
    *const map_info = context->map_info;

  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
  if ((LocaleCompare(display_image->magick,"LOGO") == 0) &&
      (LocaleCompare(display_image->filename,"LOGO") == 0))
    nexus=XOpenImage(display,resource_info,windows,MagickFalse);
  if (((int) windows->image.width < windows->image.ximage->width) ||
      ((int) windows->image.height < windows->image.ximage->height))
    (void) XMapRaised(display,windows->pan.id);
  windows->image.mapped=MagickTrue;
  return(nexus);
}

static void XFocusImageWindow(Display *display,XWindows *windows)
{
  if (windows->backdrop.id != (Window) NULL)
    (void) XSetInputFocus(display,windows->image.id,RevertToParent,
      CurrentTime);
}

static void XUnmapIconWindow(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XStandardColormap
    *const map_info = context->map_info;

  XVisualInfo
    *const icon_visual = context->icon_visual;

  XStandardColormap
    *const icon_map = context->icon_map;

  XPixelInfo
    *const icon_pixel = context->icon_pixel;

  if (map_info->colormap == icon_map->colormap)
    XConfigureImageColormap(display,resource_info,windows,
      display_image,exception);
  (void) XFreeStandardColormap(display,icon_visual,icon_map,
    icon_pixel);
  windows->icon.mapped=MagickFalse;
}

static void XLogDisplayImage(XResourceInfo *resource_info,Image *display_image)
{
  if (resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",display_image->filename,
        (double) display_image->scene,(double) display_image->columns,
        (double) display_image->rows);
      if (display_image->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          display_image->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        display_image->magick);
    }
}

static time_t XGetUpdateTime(XResourceInfo *resource_info,Image *display_image)
{
  struct stat
    attributes;

  time_t
    update_time;

  update_time=0;
  if (resource_info->update != MagickFalse)
    {
      MagickBooleanType
        status;

      /*
        Determine when file data was last modified.
      */
      status=GetPathAttributes(display_image->filename,&attributes);
      if (status != MagickFalse)
        update_time=attributes.st_mtime;
    }
  return(update_time);
}

static void XNameImageWindow(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  size_t
    *const state = context->state;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,resource_info,
    &windows->image);
  windows->image.shape=MagickTrue;  /* non-rectangular shape hint */
  if (resource_info->use_shared_memory == MagickFalse)
    windows->image.shared_memory=MagickFalse;
  if ((resource_info->title != (char *) NULL) && !(*state & MontageImageState))
    {
      char
        *title;

      title=InterpretImageProperties(resource_info->image_info,display_image,
        resource_info->title,exception);
      (void) CloneString(&windows->image.name,title);
      (void) CloneString(&windows->image.icon_name,title);
      title=DestroyString(title);
    }
  else
    {
      char
        filename[MagickPathExtent],
        window_name[MagickPathExtent];

      /*
        Window name is the base of the filename.
      */
      GetPathComponent(display_image->magick_filename,TailPath,filename);
      if (display_image->scene == 0)
        (void) FormatLocaleString(window_name,MagickPathExtent,"%s: %s",
          MagickPackageName,filename);
      else
        (void) FormatLocaleString(window_name,MagickPathExtent,
          "%s: %s[scene: %.17g frames: %.17g]",MagickPackageName,filename,
          (double) display_image->scene,(double) GetImageListLength(
          display_image));
      (void) CloneString(&windows->image.name,window_name);
      (void) CloneString(&windows->image.icon_name,filename);
    }
}

static void XMakeBackdropWindow(XResourceInfo *resource_info,XWindows *windows,
  Display *display,XVisualInfo *visual_info,XWMHints *manager_hints,
  Window root_window,char **argv,int argc,XClassHint *class_hints)
{
  if ((resource_info->backdrop) || (windows->backdrop.id != (Window) NULL))
    {
      /*
        Initialize backdrop window.
      */
      windows->backdrop.x=0;
      windows->backdrop.y=0;
      (void) CloneString(&windows->backdrop.name,"Backdrop");
      windows->backdrop.flags=(size_t) (USSize | USPosition);
      windows->backdrop.width=(unsigned int)
        XDisplayWidth(display,visual_info->screen);
      windows->backdrop.height=(unsigned int)
        XDisplayHeight(display,visual_info->screen);
      windows->backdrop.border_width=0;
      windows->backdrop.immutable=MagickTrue;
      windows->backdrop.attributes.do_not_propagate_mask=ButtonPressMask |
        ButtonReleaseMask;
      windows->backdrop.attributes.event_mask=ButtonPressMask | KeyPressMask |
        StructureNotifyMask;
      manager_hints->flags=IconWindowHint | InputHint | StateHint;
      manager_hints->icon_window=windows->icon.id;
      manager_hints->input=MagickTrue;
      manager_hints->initial_state=resource_info->iconic ? IconicState :
        NormalState;
      XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
        &windows->backdrop);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (backdrop)",windows->backdrop.id);
      (void) XMapWindow(display,windows->backdrop.id);
      (void) XClearWindow(display,windows->backdrop.id);
      if (windows->image.id != (Window) NULL)
        {
          (void) XDestroyWindow(display,windows->image.id);
          windows->image.id=(Window) NULL;
        }
      /*
        Position image in the center the backdrop.
      */
      windows->image.flags|=USPosition;
      windows->image.x=(XDisplayWidth(display,visual_info->screen)/2)-
        ((int) windows->image.width/2);
      windows->image.y=(XDisplayHeight(display,visual_info->screen)/2)-
        ((int) windows->image.height/2);
    }
}

static void XInitializeBackdropWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->backdrop);
  XMakeBackdropWindow(resource_info,windows,display,visual_info,manager_hints,
    root_window,argv,argc,class_hints);
}

static void XFollowGroupLeader(XWindows *windows,XWMHints *manager_hints,
  Display *display,XResourceInfo *resource_info)
{
  if (windows->group_leader.id != (Window) NULL)
    {
      /*
        Follow the leader.
      */
      manager_hints->flags|=WindowGroupHint;
      manager_hints->window_group=windows->group_leader.id;
      (void) XSelectInput(display,windows->group_leader.id,StructureNotifyMask);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (group leader)",windows->group_leader.id);
    }
}

static void XMakeImageWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const Window
    root_window = context->root_window;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  manager_hints->flags=IconWindowHint | InputHint | StateHint;
  manager_hints->icon_window=windows->icon.id;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=resource_info->iconic ? IconicState :
    NormalState;
  XFollowGroupLeader(windows,manager_hints,display,resource_info);
  XMakeWindow(display,
    (Window) (resource_info->backdrop ? windows->backdrop.id : root_window),
    argv,argc,class_hints,manager_hints,&windows->image);
  (void) XChangeProperty(display,windows->image.id,windows->im_protocols,
    XA_STRING,8,PropModeReplace,(unsigned char *) NULL,0);
  if (windows->group_leader.id != (Window) NULL)
    (void) XSetTransientForHint(display,windows->image.id,
      windows->group_leader.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (image)",
      windows->image.id);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D i s p l a y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDisplayImage() displays an image via X11.  A new image is created and
%  returned if the user interactively transforms the displayed image.
%
%  The format of the XDisplayImage method is:
%
%      Image *XDisplayImage(Display *display,XResourceInfo *resource_info,
%        char **argv,int argc,Image **image,size_t *state,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o nexus:  Method XDisplayImage returns an image when the
%      user chooses 'Open Image' from the command menu or picks a tile
%      from the image directory.  Otherwise a null image is returned.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o argv: Specifies the application's argument list.
%
%    o argc: Specifies the number of arguments.
%
%    o image: Specifies an address to an address of an Image structure;
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsDynamicVisualClass(const int v)
{
  switch (v)
  {
    case GrayScale:
    case PseudoColor:
    case DirectColor:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static Image *XReloadModifiedImage(XResourceInfo *resource_info,
  const Image *display_image,size_t *state,ExceptionInfo *exception)
{
  Image
    *nexus;

  (void) FormatLocaleString(
    resource_info->image_info->filename,MagickPathExtent,
    "%s:%s",display_image->magick,
    display_image->filename);
  nexus=ReadImage(resource_info->image_info,exception);
  if (nexus != (Image *) NULL)
    *state|=NextImageState | ExitState;
  return(nexus);
}

#define MagnifySize  256  /* must be a power of 2 */
#define MagickMenus  10
#define MagickTitle  "Commands"

static void XHandleProtocolMessage(const DisplayEventContext *context,
  const XEvent event,XGCValues *context_values)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = context->state;

  const char
    *const *const CommandMenu = context->CommandMenu;

  if (*event.xclient.data.l == (long) windows->im_update_widget)
    {
      (void) CloneString(&windows->command.name,MagickTitle);
      windows->command.data=MagickMenus;
      (void) XCommandWidget(display,windows,CommandMenu,
        (XEvent *) NULL);
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_update_colormap)
    {
      XUpdateWindowColormaps(context,context_values);
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_former_image)
    {
      *state|=FormerImageState | ExitState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_next_image)
    {
      *state|=NextImageState | ExitState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_retain_colors)
    {
      *state|=RetainColorsState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_exit)
    {
      *state|=ExitState;
      return;
    }
}

static void XReloadModifiedDisplay(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  const time_t
    update_time = context->update_time;

  MagickBooleanType
    status;

  struct stat
    attributes;

  size_t
    delay;

  /*
    Determine if image file was modified.
  */
  status=GetPathAttributes(display_image->filename,&attributes);
  if ((status != MagickFalse) &&
      (update_time != attributes.st_mtime))
    {
      /*
        Redisplay image.
      */
      (*nexus)=XReloadModifiedImage(resource_info,display_image,
        state,exception);
    }
  delay=display_image->delay/(size_t) MagickMax(
    display_image->ticks_per_second,1L);
  (*timer)=GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1;
}

static void XUpdateImageTimer(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  size_t
    *const state = context->state;

  if ((*timer) < GetMagickTime())
    {
      if (resource_info->update == MagickFalse)
        *state|=NextImageState | ExitState;
      else
        {
          XReloadModifiedDisplay(context,display_image,nexus,timer);
        }
    }
}

static MagickBooleanType XCheckImageTimer(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  if ((display_image->delay != 0) || (resource_info->update != 0))
    {
      XUpdateImageTimer(context,display_image,nexus,timer);
      if (XEventsQueued(display,QueuedAfterFlush) == 0)
        {
          /*
            Do not block if delay > 0.
          */
          XDelay(display,SuspendTime << 2);
          return(MagickTrue);
        }
    }
  return(MagickFalse);
}

static MagickBooleanType XSelectCommandMenuEntry(
  const DisplayEventContext *context,const int id,
  DisplayCommand *display_command)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const CommandMenu = context->CommandMenu;

  char
    *const command = context->command;

  const char *const
    **const Menus = context->Menus;

  DisplayCommand
    **const Commands = context->Commands;

  int
    entry;

  if (id < MagickMenus)
    {
      /*
        Select a command from a pop-up menu.
      */
      entry=XMenuWidget(display,windows,CommandMenu[id],Menus[id],
        command);
      if (entry < 0)
        return(MagickTrue);
      (void) CopyMagickString(command,Menus[id][entry],MagickPathExtent);
      (*display_command)=Commands[id][entry];
    }
  return(MagickFalse);
}

static const char
  *const CommandMenu[] =
  {
    "File",
    "Edit",
    "View",
    "Transform",
    "Enhance",
    "Effects",
    "F/X",
    "Image Edit",
    "Miscellany",
    "Help",
    (char *) NULL
  },
  *const FileMenu[] =
  {
    "Open...",
    "Next",
    "Former",
    "Select...",
    "Save...",
    "Print...",
    "Delete...",
    "New...",
    "Visual Directory...",
    "Quit",
    (char *) NULL
  },
  *const EditMenu[] =
  {
    "Undo",
    "Redo",
    "Cut",
    "Copy",
    "Paste",
    (char *) NULL
  },
  *const ViewMenu[] =
  {
    "Half Size",
    "Original Size",
    "Double Size",
    "Resize...",
    "Apply",
    "Refresh",
    "Restore",
    (char *) NULL
  },
  *const TransformMenu[] =
  {
    "Crop",
    "Chop",
    "Flop",
    "Flip",
    "Rotate Right",
    "Rotate Left",
    "Rotate...",
    "Shear...",
    "Roll...",
    "Trim Edges",
    (char *) NULL
  },
  *const EnhanceMenu[] =
  {
    "Hue...",
    "Saturation...",
    "Brightness...",
    "Gamma...",
    "Spiff",
    "Dull",
    "Contrast Stretch...",
    "Sigmoidal Contrast...",
    "Normalize",
    "Equalize",
    "Negate",
    "Grayscale",
    "Map...",
    "Quantize...",
    (char *) NULL
  },
  *const EffectsMenu[] =
  {
    "Despeckle",
    "Emboss",
    "Reduce Noise",
    "Add Noise...",
    "Sharpen...",
    "Blur...",
    "Threshold...",
    "Edge Detect...",
    "Spread...",
    "Shade...",
    "Raise...",
    "Segment...",
    (char *) NULL
  },
  *const FXMenu[] =
  {
    "Solarize...",
    "Sepia Tone...",
    "Swirl...",
    "Implode...",
    "Vignette...",
    "Wave...",
    "Oil Paint...",
    "Charcoal Draw...",
    (char *) NULL
  },
  *const ImageEditMenu[] =
  {
    "Annotate...",
    "Draw...",
    "Color...",
    "Matte...",
    "Composite...",
    "Add Border...",
    "Add Frame...",
    "Comment...",
    "Launch...",
    "Region of Interest...",
    (char *) NULL
  },
  *const MiscellanyMenu[] =
  {
    "Image Info",
    "Zoom Image",
    "Show Preview...",
    "Show Histogram",
    "Show Matte",
    "Background...",
    "Slide Show...",
    "Preferences...",
    (char *) NULL
  },
  *const HelpMenu[] =
  {
    "Overview",
    "Browse Documentation",
    "About Display",
    (char *) NULL
  },
  *const ShortCutsMenu[] =
  {
    "Next",
    "Former",
    "Open...",
    "Save...",
    "Print...",
    "Undo",
    "Restore",
    "Image Info",
    "Quit",
    (char *) NULL
  },
  *const VirtualMenu[] =
  {
    "Image Info",
    "Print",
    "Next",
    "Quit",
    (char *) NULL
  };

static const char
  *const *Menus[MagickMenus] =
  {
    FileMenu,
    EditMenu,
    ViewMenu,
    TransformMenu,
    EnhanceMenu,
    EffectsMenu,
    FXMenu,
    ImageEditMenu,
    MiscellanyMenu,
    HelpMenu
  };

static DisplayCommand
  CommandMenus[] =
  {
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
  },
  FileCommands[] =
  {
    OpenCommand,
    NextCommand,
    FormerCommand,
    SelectCommand,
    SaveCommand,
    PrintCommand,
    DeleteCommand,
    NewCommand,
    VisualDirectoryCommand,
    QuitCommand
  },
  EditCommands[] =
  {
    UndoCommand,
    RedoCommand,
    CutCommand,
    CopyCommand,
    PasteCommand
  },
  ViewCommands[] =
  {
    HalfSizeCommand,
    OriginalSizeCommand,
    DoubleSizeCommand,
    ResizeCommand,
    ApplyCommand,
    RefreshCommand,
    RestoreCommand
  },
  TransformCommands[] =
  {
    CropCommand,
    ChopCommand,
    FlopCommand,
    FlipCommand,
    RotateRightCommand,
    RotateLeftCommand,
    RotateCommand,
    ShearCommand,
    RollCommand,
    TrimCommand
  },
  EnhanceCommands[] =
  {
    HueCommand,
    SaturationCommand,
    BrightnessCommand,
    GammaCommand,
    SpiffCommand,
    DullCommand,
    ContrastStretchCommand,
    SigmoidalContrastCommand,
    NormalizeCommand,
    EqualizeCommand,
    NegateCommand,
    GrayscaleCommand,
    MapCommand,
    QuantizeCommand
  },
  EffectsCommands[] =
  {
    DespeckleCommand,
    EmbossCommand,
    ReduceNoiseCommand,
    AddNoiseCommand,
    SharpenCommand,
    BlurCommand,
    ThresholdCommand,
    EdgeDetectCommand,
    SpreadCommand,
    ShadeCommand,
    RaiseCommand,
    SegmentCommand
  },
  FXCommands[] =
  {
    SolarizeCommand,
    SepiaToneCommand,
    SwirlCommand,
    ImplodeCommand,
    VignetteCommand,
    WaveCommand,
    OilPaintCommand,
    CharcoalDrawCommand
  },
  ImageEditCommands[] =
  {
    AnnotateCommand,
    DrawCommand,
    ColorCommand,
    MatteCommand,
    CompositeCommand,
    AddBorderCommand,
    AddFrameCommand,
    CommentCommand,
    LaunchCommand,
    RegionOfInterestCommand
  },
  MiscellanyCommands[] =
  {
    InfoCommand,
    ZoomCommand,
    ShowPreviewCommand,
    ShowHistogramCommand,
    ShowMatteCommand,
    BackgroundCommand,
    SlideShowCommand,
    PreferencesCommand
  },
  HelpCommands[] =
  {
    HelpCommand,
    BrowseDocumentationCommand,
    VersionCommand
  },
  ShortCutsCommands[] =
  {
    NextCommand,
    FormerCommand,
    OpenCommand,
    SaveCommand,
    PrintCommand,
    UndoCommand,
    RestoreCommand,
    InfoCommand,
    QuitCommand
  },
  VirtualCommands[] =
  {
    InfoCommand,
    PrintCommand,
    NextCommand,
    QuitCommand
  };

static DisplayCommand
  *Commands[MagickMenus] =
  {
    FileCommands,
    EditCommands,
    ViewCommands,
    TransformCommands,
    EnhanceCommands,
    EffectsCommands,
    FXCommands,
    ImageEditCommands,
    MiscellanyCommands,
    HelpCommands
  };

static time_t XGetDisplayTimer(const Image *display_image)
{
  size_t
    delay;

  delay=display_image->delay/(size_t)
    MagickMax(display_image->ticks_per_second,1L);
  return(GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1);
}

static void XHandleButtonPress(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  time_t
    *const timer = targets->timer;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Press: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if (((*event).xbutton.button == Button3) &&
      ((*event).xbutton.state & Mod1Mask))
    {
      /*
        Convert Alt-Button3 to Button2.
      */
      (*event).xbutton.button=Button2;
      (*event).xbutton.state&=(unsigned int) (~Mod1Mask);
    }
  if ((*event).xbutton.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,(*event).xbutton.window,RevertToParent,
        (*event).xbutton.time);
      return;
    }
  if ((*event).xbutton.window == windows->image.id)
    {
      XPressImageWindow(context,targets);
      return;
    }
  if ((*event).xbutton.window == windows->magnify.id)
    {
      XPressMagnifyWindow(context);
      return;
    }
  if ((*event).xbutton.window == windows->pan.id)
    {
      XPressPanWindow(context,event);
      return;
    }
  (*timer)=XGetDisplayTimer((*display_image));
}

static void XHandleClientMessage(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  XVisualInfo
    *const visual_info = context->visual_info;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  XGCValues
    *const context_values = targets->context_values;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Client Message: 0x%lx 0x%lx %d 0x%lx",(*event).xclient.window,
      (*event).xclient.message_type,(*event).xclient.format,(unsigned long)
      (*event).xclient.data.l[0]);
  if ((*event).xclient.message_type == windows->im_protocols)
    {
      XHandleProtocolMessage(context,(*event),context_values);
      return;
    }
  if ((*event).xclient.message_type == windows->dnd_protocols)
    {
      (*nexus)=XDropImage(context,(*event),(*nexus));
      return;
    }
  /*
    If client window delete message, exit.
  */
  if ((*event).xclient.message_type != windows->wm_protocols)
    return;
  if (*(*event).xclient.data.l != (long) windows->wm_delete_window)
    return;
  (void) XWithdrawWindow(display,(*event).xclient.window,
    visual_info->screen);
  if ((*event).xclient.window == windows->image.id)
    {
      *state|=ExitState;
      return;
    }
  if ((*event).xclient.window == windows->pan.id)
    {
      /*
        Restore original image size when pan window is deleted.
      */
      windows->image.window_changes.width=windows->image.ximage->width;
      windows->image.window_changes.height=windows->image.ximage->height;
      (void) XConfigureImage(display,resource_info,windows,
        (*display_image),exception);
    }
}

static void XHandleConfigureNotify(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  XPoint
    *const vid_info = targets->vid_info;

  MagickStatusType
    *const status = targets->status;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Configure Notify: 0x%lx %dx%d+%d+%d %d",(*event).xconfigure.window,
      (*event).xconfigure.width,(*event).xconfigure.height,(*event).xconfigure.x,
      (*event).xconfigure.y,(*event).xconfigure.send_event);
  if ((*event).xconfigure.window == windows->image.id)
    {
      XConfigureImageWindow(context,(*event),(*display_image),(*vid_info));
      return;
    }
  if ((*event).xconfigure.window == windows->magnify.id)
    {
      (*status)=XConfigureMagnifyWindow(context,(*event),(*display_image),
        (*status));
      return;
    }
  if (windows->magnify.mapped != MagickFalse &&
      ((*event).xconfigure.window == windows->pan.id))
    {
      XConfigurePanWindow(windows,(*event));
      return;
    }
  if ((*event).xconfigure.window == windows->icon.id)
    {
      /*
        Icon window has a new configuration.
      */
      windows->icon.width=(unsigned int) (*event).xconfigure.width;
      windows->icon.height=(unsigned int) (*event).xconfigure.height;
      return;
    }
}

static void XHandleExpose(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  time_t
    *const timer = targets->timer;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Expose: 0x%lx %dx%d+%d+%d",(*event).xexpose.window,
      (*event).xexpose.width,(*event).xexpose.height,(*event).xexpose.x,
      (*event).xexpose.y);
  /*
    Refresh windows that are now exposed.
  */
  if (((*event).xexpose.window == windows->image.id) &&
      windows->image.mapped != MagickFalse)
    {
      XRefreshWindow(display,&windows->image,event);
      (*timer)=XGetDisplayTimer((*display_image));
      return;
    }
  if (((*event).xexpose.window == windows->magnify.id) &&
      windows->magnify.mapped != MagickFalse)
    {
      XMakeMagnifyImage(display,windows,exception);
      return;
    }
  if ((*event).xexpose.window == windows->pan.id)
    {
      XDrawPanRectangle(display,windows);
      return;
    }
  if ((*event).xexpose.window == windows->icon.id)
    {
      XRefreshWindow(display,&windows->icon,event);
      return;
    }
}

static void XHandleKeyPress(const DisplayEventContext *context,
  const KeySym key_symbol,const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const command = context->command;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  time_t
    *const timer = targets->timer;

  const MagnifyWindowInput
    input = { display, windows, exception };

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key press: %d 0x%lx (%s)",(*event).xkey.state,(unsigned long)
      key_symbol,command);
  if ((*event).xkey.window == windows->image.id)
    {
      XImageWindowKeyPress(context,(*event),key_symbol,
        targets);
    }
  if ((*event).xkey.window == windows->magnify.id)
    XMagnifyWindowCommand(&input,(*event).xkey.state,key_symbol);
  if ((*event).xkey.window == windows->pan.id)
    {
      XPanWindowKeyPress(context,key_symbol);
    }
  (*timer)=XGetDisplayTimer((*display_image));
}

static void XRecordMappedWindow(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XVisualInfo
    *const icon_visual = context->icon_visual;

  XStandardColormap
    *const icon_map = context->icon_map;

  XPixelInfo
    *const icon_pixel = context->icon_pixel;

  XResourceInfo
    *const icon_resources = context->icon_resources;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  if ((*event).xmap.window == windows->info.id)
    {
      windows->info.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->icon.id)
    {
      MagickBooleanType
        taint;

      /*
        Create an icon image.
      */
      taint=(*display_image)->taint;
      XMakeStandardColormap(display,icon_visual,icon_resources,
        (*display_image),icon_map,icon_pixel,exception);
      (void) XMakeImage(display,icon_resources,&windows->icon,
        (*display_image),windows->icon.width,windows->icon.height,
        exception);
      (*display_image)->taint=taint;
      (void) XSetWindowBackgroundPixmap(display,windows->icon.id,
        windows->icon.pixmap);
      (void) XClearWindow(display,windows->icon.id);
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
      windows->icon.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->command.id)
    {
      windows->command.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->popup.id)
    {
      windows->popup.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->widget.id)
    {
      windows->widget.mapped=MagickTrue;
      return;
    }
}

static void XHandleMapNotify(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Map Notify: 0x%lx",
      (*event).xmap.window);
  if ((*event).xmap.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,(*event).xmap.window,RevertToParent,
        CurrentTime);
      windows->backdrop.mapped=MagickTrue;
      return;
    }
  if ((*event).xmap.window == windows->image.id)
    {
      (*nexus)=XMapImageWindow(context,(*display_image),(*nexus));
      return;
    }
  if ((*event).xmap.window == windows->magnify.id)
    {
      XMakeMagnifyImage(display,windows,exception);
      windows->magnify.mapped=MagickTrue;
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
      return;
    }
  if ((*event).xmap.window == windows->pan.id)
    {
      XMakePanImage(display,resource_info,windows,(*display_image),
        exception);
      windows->pan.mapped=MagickTrue;
      return;
    }
  XRecordMappedWindow(context,targets);
}

static void XRecordUnmappedWindow(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  if ((*event).xunmap.window == windows->info.id)
    {
      windows->info.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->icon.id)
    {
      XUnmapIconWindow(context,(*display_image));
      return;
    }
  if ((*event).xunmap.window == windows->command.id)
    {
      windows->command.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->popup.id)
    {
      XFocusImageWindow(display,windows);
      windows->popup.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->widget.id)
    {
      XFocusImageWindow(display,windows);
      windows->widget.mapped=MagickFalse;
      return;
    }
}

static void XHandleUnmapNotify(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XEvent
    *const event = targets->event;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Unmap Notify: 0x%lx",(*event).xunmap.window);
  if ((*event).xunmap.window == windows->backdrop.id)
    {
      windows->backdrop.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->image.id)
    {
      windows->image.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->magnify.id)
    {
      windows->magnify.mapped=MagickFalse;
      return;
    }
  if ((*event).xunmap.window == windows->pan.id)
    {
      windows->pan.mapped=MagickFalse;
      return;
    }
  XRecordUnmappedWindow(context,targets);
}

static void XHandlePropertyNotify(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  Image
    **const nexus = targets->nexus;

  XEvent
    *const event = targets->event;

  Atom
    type;

  int
    format,
    status;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Property Notify: 0x%lx 0x%lx %d",(*event).xproperty.window,
      (*event).xproperty.atom,(*event).xproperty.state);
  if ((*event).xproperty.atom != windows->im_remote_command)
    return;
  /*
    Display image named by the remote command protocol.
  */
  status=XGetWindowProperty(display,(*event).xproperty.window,
    (*event).xproperty.atom,0L,(long) MagickPathExtent,MagickFalse,(Atom)
    AnyPropertyType,&type,&format,&length,&after,&data);
  if ((status != Success) || (length == 0))
    return;
  if (LocaleCompare((char *) data,"-quit") == 0)
    {
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_exit,CurrentTime);
      (void) XFree((void *) data);
      return;
    }
  (void) CopyMagickString(resource_info->image_info->filename,
    (char *) data,MagickPathExtent);
  (void) XFree((void *) data);
  (*nexus)=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if ((*nexus) != (Image *) NULL)
    *state|=NextImageState | ExitState;
}

static void XMakeInfoWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,resource_info,
    &windows->info);
  (void) CloneString(&windows->info.name,"Info");
  (void) CloneString(&windows->info.icon_name,"Info");
  windows->info.border_width=1;
  windows->info.x=2;
  windows->info.y=2;
  windows->info.flags|=PPosition;
  windows->info.attributes.win_gravity=UnmapGravity;
  windows->info.attributes.event_mask=ButtonPressMask | ExposureMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,windows->image.id,argv,argc,class_hints,manager_hints,
    &windows->info);
  windows->info.highlight_stipple=XCreateBitmapFromData(display,
    windows->info.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->info.shadow_stipple=XCreateBitmapFromData(display,
    windows->info.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->info.id,windows->image.id);
  if (windows->image.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (info)",
      windows->info.id);
}

static void XMakeCommandWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  char
    resource_name[MagickPathExtent];

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->command);
  windows->command.data=MagickMenus;
  (void) XCommandWidget(display,windows,CommandMenu,(XEvent *) NULL);
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.command",
    resource_info->client_name);
  windows->command.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) CloneString(&windows->command.name,MagickTitle);
  windows->command.border_width=0;
  windows->command.flags|=PPosition;
  windows->command.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | LeaveWindowMask |
    OwnerGrabButtonMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->command);
  windows->command.highlight_stipple=XCreateBitmapFromData(display,
    windows->command.id,(char *) HighlightBitmap,HighlightWidth,
    HighlightHeight);
  windows->command.shadow_stipple=XCreateBitmapFromData(display,
    windows->command.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->command.id,windows->image.id);
  if (windows->command.mapped != MagickFalse)
    (void) XMapRaised(display,windows->command.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (command)",windows->command.id);
}

static void XMakeWidgetWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  char
    resource_name[MagickPathExtent];

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->widget);
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.widget",
    resource_info->client_name);
  windows->widget.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  windows->widget.border_width=0;
  windows->widget.flags|=PPosition;
  windows->widget.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | OwnerGrabButtonMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->widget);
  windows->widget.highlight_stipple=XCreateBitmapFromData(display,
    windows->widget.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->widget.shadow_stipple=XCreateBitmapFromData(display,
    windows->widget.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->widget.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (widget)",windows->widget.id);
}

static void XMakePopupWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->popup);
  windows->popup.border_width=0;
  windows->popup.flags|=PPosition;
  windows->popup.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->popup);
  windows->popup.highlight_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->popup.shadow_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->popup.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (pop up)",windows->popup.id);
}

static void XMakeMagnifyWindow(const DisplayEventContext *context,
  const Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  char
    resource_name[MagickPathExtent];

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->magnify);
  if (resource_info->use_shared_memory == MagickFalse)
    windows->magnify.shared_memory=MagickFalse;
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.magnify",
    resource_info->client_name);
  windows->magnify.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) FormatLocaleString(windows->magnify.name,MagickPathExtent,
    "Magnify %uX",resource_info->magnify);
  if (windows->magnify.cursor != (Cursor) NULL)
    (void) XFreeCursor(display,windows->magnify.cursor);
  windows->magnify.cursor=XMakeCursor(display,windows->image.id,
    map_info->colormap,resource_info->background_color,
    resource_info->foreground_color);
  if (windows->magnify.cursor == (Cursor) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateCursor",
      display_image->filename);
  windows->magnify.width=MagnifySize;
  windows->magnify.height=MagnifySize;
  windows->magnify.flags|=PPosition;
  windows->magnify.min_width=MagnifySize;
  windows->magnify.min_height=MagnifySize;
  windows->magnify.width_inc=MagnifySize;
  windows->magnify.height_inc=MagnifySize;
  windows->magnify.data=resource_info->magnify;
  windows->magnify.attributes.cursor=windows->magnify.cursor;
  windows->magnify.attributes.event_mask=ButtonPressMask | ButtonReleaseMask |
    ExposureMask | KeyPressMask | KeyReleaseMask | OwnerGrabButtonMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->magnify);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (magnify)",windows->magnify.id);
}

static void XMakePanWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  char
    resource_name[MagickPathExtent];

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->pan);
  (void) CloneString(&windows->pan.name,"Pan Icon");
  windows->pan.width=windows->icon.width;
  windows->pan.height=windows->icon.height;
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.pan",
    resource_info->client_name);
  windows->pan.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) XParseGeometry(windows->pan.geometry,&windows->pan.x,&windows->pan.y,
    &windows->pan.width,&windows->pan.height);
  windows->pan.flags|=PPosition;
  windows->pan.immutable=MagickTrue;
  windows->pan.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | ExposureMask | KeyPressMask | KeyReleaseMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->pan);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (pan)",
      windows->pan.id);
  (void) XSetTransientForHint(display,windows->pan.id,windows->image.id);
}

static MagickBooleanType XSelectCommandWidgetEvent(
  const DisplayEventContext *context,const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const command = context->command;

  Image
    **const nexus = targets->nexus,
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  DisplayCommand
    display_command;

  int
    id;

  if ((*event).xany.window == windows->command.id)
    {
      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,CommandMenu,event);
      if (id < 0)
        return(MagickTrue);
      (void) CopyMagickString(command,CommandMenu[id],MagickPathExtent);
      display_command=CommandMenus[id];
      if (XSelectCommandMenuEntry(context,id,&display_command) !=
          MagickFalse)
        return(MagickTrue);
      if (display_command != NullCommand)
        (*nexus)=XMagickCommand(display,resource_info,windows,display_command,
          display_image,exception);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static void XUpdateWindowStasis(XWindows *windows,const time_t timestamp)
{
  if ((windows->image.stasis == MagickFalse) ||
      (windows->magnify.stasis == MagickFalse))
    {
      if ((GetMagickTime()-timestamp) > 0)
        {
          windows->image.stasis=MagickTrue;
          windows->magnify.stasis=MagickTrue;
        }
    }
}

static inline MagickBooleanType IsSelectiveColormapCrossing(Display *display,
  const XStandardColormap *map_info,const XVisualInfo *visual_info,
  const int mode)
{
  return((MagickBooleanType) ((map_info->colormap !=
    XDefaultColormap(display,visual_info->screen)) && (mode != NotifyUngrab)));
}

static void XDispatchWindowStateEvent(const DisplayEventContext *event_context,
  const DisplayEventTargets *event_targets)
{
  XResourceInfo
    *const resource_info = event_context->resource_info;

  XEvent
    *const event = event_targets->event;

  switch ((*event).type)
  {
    case PropertyNotify:
    {
      XHandlePropertyNotify(event_context,event_targets);
      break;
    }
    case ReparentNotify:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Reparent Notify: 0x%lx=>0x%lx",(*event).xreparent.parent,
          (*event).xreparent.window);
      break;
    }
    case UnmapNotify:
    {
      XHandleUnmapNotify(event_context,event_targets);
      break;
    }
    default:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"Event type: %d",
          (*event).type);
      break;
    }
  }
}

static void XDispatchMapEvent(const DisplayEventContext *event_context,
  const DisplayEventTargets *event_targets)
{
  Display
    *const display = event_context->display;

  XStandardColormap
    *const map_info = event_context->map_info;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XEvent
    *const event = event_targets->event;

  switch ((*event).type)
  {
    case LeaveNotify:
    {
      /*
        Selectively uninstall colormap.
      */
      if (IsSelectiveColormapCrossing(display,map_info,visual_info,
            (*event).xcrossing.mode) != MagickFalse)
        XUninstallColormap(display,map_info->colormap);
      break;
    }
    case MapNotify:
    {
      XHandleMapNotify(event_context,event_targets);
      break;
    }
    case MappingNotify:
    {
      (void) XRefreshKeyboardMapping(&(*event).xmapping);
      break;
    }
    case NoExpose:
      break;
    default:
    {
      XDispatchWindowStateEvent(event_context,event_targets);
      break;
    }
  }
}

static void XDispatchExposeEvent(const DisplayEventContext *event_context,
  const DisplayEventTargets *event_targets)
{
  Display
    *const display = event_context->display;

  XResourceInfo
    *const resource_info = event_context->resource_info;

  XWindows
    *const windows = event_context->windows;

  XStandardColormap
    *const map_info = event_context->map_info;

  size_t
    *const state = event_context->state;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  XEvent
    *const event = event_targets->event;

  switch ((*event).type)
  {
    case DestroyNotify:
    {
      /*
        Group leader has exited.
      */
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Destroy Notify: 0x%lx",(*event).xdestroywindow.window);
      if ((*event).xdestroywindow.window == windows->group_leader.id)
        {
          *state|=ExitState;
          break;
        }
      break;
    }
    case EnterNotify:
    {
      /*
        Selectively install colormap.
      */
      if (IsSelectiveColormapCrossing(display,map_info,visual_info,
            (*event).xcrossing.mode) != MagickFalse)
        XInstallColormap(display,map_info->colormap);
      break;
    }
    case Expose:
    {
      XHandleExpose(event_context,event_targets);
      break;
    }
    default:
    {
      XDispatchMapEvent(event_context,event_targets);
      break;
    }
  }
}

static void XDispatchButtonEvent(const DisplayEventContext *event_context,
  const DisplayEventTargets *event_targets)
{
  XResourceInfo
    *const resource_info = event_context->resource_info;

  XEvent
    *const event = event_targets->event;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XHandleButtonPress(event_context,event_targets);
      break;
    }
    case ButtonRelease:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Button Release: 0x%lx %u +%d+%d",(*event).xbutton.window,
          (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
      break;
    }
    case ClientMessage:
    {
      XHandleClientMessage(event_context,event_targets);
      break;
    }
    case ConfigureNotify:
    {
      XHandleConfigureNotify(event_context,event_targets);
      break;
    }
    default:
    {
      XDispatchExposeEvent(event_context,event_targets);
      break;
    }
  }
}

static void XWithdrawDynamicWindows(Display *display,XWindows *windows)
{
  if (IsDynamicVisualClass(windows->visual_info->klass) != MagickFalse)
    {
      /*
        Withdraw pan and Magnify window.
      */
      if (windows->info.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      if (windows->magnify.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->magnify.id,
          windows->magnify.screen);
      if (windows->command.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->command.id,
          windows->command.screen);
    }
}

static void XFreeDisplayResources(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XPixelInfo
    *const icon_pixel = context->icon_pixel,
    *const pixel = context->pixel;

  XStandardColormap
    *const icon_map = context->icon_map,
    *const map_info = context->map_info;

  XVisualInfo
    *const icon_visual = context->icon_visual,
    *const visual_info = context->visual_info;

  size_t
    *const state = context->state;

  if (*state & ExitState)
    {
      /*
        Free Standard Colormap.
      */
      (void) XFreeStandardColormap(display,icon_visual,icon_map,icon_pixel);
      if (resource_info->map_type == (char *) NULL)
        (void) XFreeStandardColormap(display,visual_info,map_info,pixel);
      /*
        Free X resources.
      */
      if (resource_info->copy_image != (Image *) NULL)
        {
          resource_info->copy_image=DestroyImage(resource_info->copy_image);
          resource_info->copy_image=NewImageList();
        }
      DestroyXResources();
    }
}

static void XLogKeyRelease(const XResourceInfo *resource_info,
  const KeySym key_symbol,const char *command)
{
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key release: 0x%lx (%c)",(unsigned long) key_symbol,*command);
}

static WarningHandler XRestoreWorkingDirectory(XResourceInfo *resource_info,
  char *working_directory,ExceptionInfo *exception)
{
  WarningHandler
    warning_handler;

  int
    status;

  if (*working_directory == '\0')
    (void) CopyMagickString(working_directory,".",MagickPathExtent);
  status=chdir(working_directory);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",working_directory);
  warning_handler=resource_info->display_warnings ?
    SetErrorHandler(XWarning) : SetErrorHandler((ErrorHandler) NULL);
  warning_handler=resource_info->display_warnings ?
    SetWarningHandler(XWarning) : SetWarningHandler((WarningHandler) NULL);
  return(warning_handler);
}

static XWindows *XAllocateDisplayWindows(Display *display,
  XResourceInfo *resource_info,const Image *display_image,Image **image)
{
  XWindows
    *windows;

  resource_info->colors=display_image->colors;
  windows=XSetWindows(XInitializeWindows(display,resource_info));
  if (windows == (XWindows *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateWindow",
      (*image)->filename);
  return(windows);
}

static void XInitializeWindowIds(XWindows *windows,
  XWindowInfo **magick_windows,unsigned int *number_windows)
{
  int
    i;

  (*number_windows)=0;
  magick_windows[(*number_windows)++]=(&windows->icon);
  magick_windows[(*number_windows)++]=(&windows->backdrop);
  magick_windows[(*number_windows)++]=(&windows->image);
  magick_windows[(*number_windows)++]=(&windows->info);
  magick_windows[(*number_windows)++]=(&windows->command);
  magick_windows[(*number_windows)++]=(&windows->widget);
  magick_windows[(*number_windows)++]=(&windows->popup);
  magick_windows[(*number_windows)++]=(&windows->magnify);
  magick_windows[(*number_windows)++]=(&windows->pan);
  for (i=0; i < (int) (*number_windows); i++)
    magick_windows[i]->id=(Window) NULL;
}

typedef struct _DisplayExitInputs
{
  size_t
    *state;

  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;
} DisplayExitInputs;

static void XConfirmDisplayExit(const DisplayExitInputs *inputs,
  Image **display_image)
{
  size_t
    *const state = inputs->state;

  Display
    *const display = inputs->display;

  XResourceInfo
    *const resource_info = inputs->resource_info;

  XWindows
    *const windows = inputs->windows;

  ExceptionInfo
    *const exception = inputs->exception;

  if ((*state & ExitState) == 0)
    (void) XMagickCommand(display,resource_info,windows,FreeBuffersCommand,
      &(*display_image),exception);
  else
    if (resource_info->confirm_edit != MagickFalse)
      {
        /*
          Query user if image has changed.
        */
        if ((resource_info->immutable == MagickFalse) &&
            (*display_image)->taint != MagickFalse)
          {
            int
              status;

            status=XConfirmWidget(display,windows,"Your image changed.",
              "Do you want to save it");
            if (status == 0)
              *state&=(unsigned int) (~ExitState);
            else
              if (status > 0)
                (void) XMagickCommand(display,resource_info,windows,SaveCommand,
                  &(*display_image),exception);
          }
      }
}

static void XWithdrawBackdropWindow(XResourceInfo *resource_info,
  XWindows *windows,Display *display)
{
  if ((resource_info->backdrop == MagickFalse) && (windows->backdrop.mapped))
    {
      (void) XWithdrawWindow(display,windows->backdrop.id,
        windows->backdrop.screen);
      (void) XDestroyWindow(display,windows->backdrop.id);
      windows->backdrop.id=(Window) NULL;
      (void) XWithdrawWindow(display,windows->image.id,
        windows->image.screen);
      (void) XDestroyWindow(display,windows->image.id);
      windows->image.id=(Window) NULL;
    }
}

static void XCloseDisplayWindows(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = context->state;

  XWithdrawDynamicWindows(display,windows);
  if (windows->pan.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->pan.id,windows->pan.screen);
  XWithdrawBackdropWindow(resource_info,windows,display);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (((*state & FormerImageState) != 0) || ((*state & NextImageState) != 0))
    *state&=(unsigned int) (~ExitState);
  XFreeDisplayResources(context);
  (void) XSync(display,MagickFalse);
}

static void XChangeToHomeDirectory(XResourceInfo *resource_info,
  ExceptionInfo *exception)
{
  int
    status;

  if (*resource_info->home_directory == '\0')
    (void) CopyMagickString(resource_info->home_directory,".",MagickPathExtent);
  status=chdir(resource_info->home_directory);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",resource_info->home_directory);
}

static MagickStatusType XMakeDisplayXImages(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  unsigned int
    height,
    width;

  windows->image.x=0;
  windows->image.y=0;
  windows->magnify.shape=MagickFalse;
  width=(unsigned int) display_image->columns;
  height=(unsigned int) display_image->rows;
  if ((display_image->columns != width) || (display_image->rows != height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  status=XMakeImage(display,resource_info,&windows->image,display_image,
    width,height,exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  status=XMakeImage(display,resource_info,&windows->magnify,(Image *) NULL,
    windows->magnify.width,windows->magnify.height,exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  if (windows->magnify.mapped != MagickFalse)
    (void) XMapRaised(display,windows->magnify.id);
  if (windows->pan.mapped != MagickFalse)
    (void) XMapRaised(display,windows->pan.id);
  windows->image.window_changes.width=(int) display_image->columns;
  windows->image.window_changes.height=(int) display_image->rows;
  (void) XConfigureImage(display,resource_info,windows,display_image,exception);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  (void) XSync(display,MagickFalse);
  return(status);
}

static void XInitializePanWindow(const DisplayEventContext *context,
  XWindows *windows,Display *display)
{
  XMakePanWindow(context);
  if (windows->info.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if ((windows->image.mapped == MagickFalse) ||
      (windows->backdrop.id != (Window) NULL))
    (void) XMapWindow(display,windows->image.id);
}

static void XMakeDisplayWindows(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XVisualInfo
    *const visual_info = context->visual_info;

  char
    geometry[MagickPathExtent];

  RectangleInfo
    geometry_info;

  XNameImageWindow(context,display_image);
  if (resource_info->immutable)
    windows->image.immutable=MagickTrue;
  windows->image.use_pixmap=resource_info->use_pixmap;
  windows->image.geometry=resource_info->image_geometry;
  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>!",
    XDisplayWidth(display,visual_info->screen),
    XDisplayHeight(display,visual_info->screen));
  geometry_info.width=display_image->columns;
  geometry_info.height=display_image->rows;
  geometry_info.x=0;
  geometry_info.y=0;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  windows->image.width=(unsigned int) geometry_info.width;
  windows->image.height=(unsigned int) geometry_info.height;
  windows->image.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | OwnerGrabButtonMask |
    PropertyChangeMask | StructureNotifyMask | SubstructureNotifyMask;
  XInitializeBackdropWindow(context);
  XMakeImageWindow(context);
  /*
    Initialize Info widget.
  */
  XMakeInfoWindow(context);
  /*
    Initialize Command widget.
  */
  XMakeCommandWindow(context);
  /*
    Initialize Widget window.
  */
  XMakeWidgetWindow(context);
  /*
    Initialize popup window.
  */
  XMakePopupWindow(context);
  /*
    Initialize Magnify window and cursor.
  */
  XMakeMagnifyWindow(context,display_image);
  (void) XSetTransientForHint(display,windows->magnify.id,windows->image.id);
  /*
    Initialize panning window.
  */
  XInitializePanWindow(context,windows,display);
}

static void XInitializeIconContext(XPixelInfo *icon_pixel,Display *display,
  XGCValues *context_values,XWindows *windows,Image *display_image)
{
  if (icon_pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,icon_pixel->annotate_context);
  (*context_values).background=icon_pixel->background_color.pixel;
  (*context_values).foreground=icon_pixel->foreground_color.pixel;
  icon_pixel->annotate_context=XCreateGC(display,windows->icon.id,
    (size_t) (GCBackground | GCForeground),context_values);
  if (icon_pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  windows->icon.annotate_context=icon_pixel->annotate_context;
}

static void XMakeIconWindow(const DisplayEventContext *context,
  Image *display_image,XGCValues *context_values)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info,
    *const icon_resources = context->icon_resources;

  XWindows
    *const windows = context->windows;

  XVisualInfo
    *const icon_visual = context->icon_visual;

  XStandardColormap
    *const icon_map = context->icon_map;

  XPixelInfo
    *const icon_pixel = context->icon_pixel;

  const Window
    root_window = context->root_window;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  XGetWindowInfo(display,icon_visual,icon_map,icon_pixel,(XFontStruct *) NULL,
    icon_resources,&windows->icon);
  windows->icon.geometry=resource_info->icon_geometry;
  XBestIconSize(display,&windows->icon,display_image);
  windows->icon.attributes.colormap=XDefaultColormap(display,
    icon_visual->screen);
  windows->icon.attributes.event_mask=ExposureMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=IconicState;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->icon);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (icon)",
      windows->icon.id);
  /*
    Initialize graphic context for icon window.
  */
  XInitializeIconContext(icon_pixel,display,context_values,windows,
    display_image);
}

static void XMakeContextWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  windows->context.id=(Window) NULL;
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->context);
  (void) CloneString(&class_hints->res_name,resource_info->client_name);
  (void) CloneString(&class_hints->res_class,resource_info->client_name);
  class_hints->res_class[0]=(char) LocaleToUppercase((int)
    class_hints->res_class[0]);
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=WithdrawnState;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->context);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (context)",windows->context.id);
}

static void XCreateDisplayGCs(const DisplayEventContext *context,
  const Image *display_image,const MagickStatusType context_mask,
  XGCValues *context_values)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  if (pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,pixel->annotate_context);
  pixel->annotate_context=XCreateGC(display,windows->context.id,
    context_mask,context_values);
  if (pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  (*context_values).background=pixel->depth_color.pixel;
  if (pixel->widget_context != (GC) NULL)
    (void) XFreeGC(display,pixel->widget_context);
  pixel->widget_context=XCreateGC(display,windows->context.id,context_mask,
    context_values);
  if (pixel->widget_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  (*context_values).background=pixel->foreground_color.pixel;
  (*context_values).foreground=pixel->background_color.pixel;
  (*context_values).plane_mask=(*context_values).background ^
    (*context_values).foreground;
  if (pixel->highlight_context != (GC) NULL)
    (void) XFreeGC(display,pixel->highlight_context);
  pixel->highlight_context=XCreateGC(display,windows->context.id,
    (size_t) (context_mask | GCPlaneMask),context_values);
  if (pixel->highlight_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  (void) XDestroyWindow(display,windows->context.id);
}

static WarningHandler XInstallWarningHandlers(
  const XResourceInfo *resource_info,WarningHandler warning_handler)
{
  if (warning_handler == (WarningHandler) NULL)
    {
      warning_handler=resource_info->display_warnings ?
        SetErrorHandler(XWarning) : SetErrorHandler((ErrorHandler) NULL);
      warning_handler=resource_info->display_warnings ?
        SetWarningHandler(XWarning) : SetWarningHandler((WarningHandler) NULL);
    }
  return(warning_handler);
}

static void XLoadDisplayFont(Display *display,XResourceInfo *resource_info,
  XWindows *windows)
{
  if (windows->font_info != (XFontStruct *) NULL)
    (void) XFreeFont(display,windows->font_info);
  windows->font_info=XBestFont(display,resource_info,MagickFalse);
  if (windows->font_info == (XFontStruct *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToLoadFont",
      resource_info->font);
}

static MagickBooleanType XCheckMappedImageTimer(
  const DisplayEventContext *context,const DisplayEventTargets *targets)
{
  XWindows
    *const windows = context->windows;

  Image
    **const nexus = targets->nexus,
    **const display_image = targets->display_image;

  time_t
    *const timer = targets->timer;

  if (windows->image.mapped != MagickFalse)
    if (XCheckImageTimer(context,(*display_image),nexus,timer) !=
        MagickFalse)
      return(MagickTrue);
  return(MagickFalse);
}

static MagickBooleanType XNextDisplayEvent(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XEvent
    *const event = targets->event;

  time_t
    timestamp;

  if (XCheckMappedImageTimer(context,targets) != MagickFalse)
    return(MagickTrue);
  timestamp=GetMagickTime();
  (void) XNextEvent(display,event);
  XUpdateWindowStasis(windows,timestamp);
  if (XSelectCommandWidgetEvent(context,targets) !=
      MagickFalse)
    return(MagickTrue);
  return(MagickFalse);
}

static void XTraceDisplayImage(Image **image)
{
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
}

static void XReturnToHomeDirectory(char *working_directory,
  XResourceInfo *resource_info,ExceptionInfo *exception)
{
  char
    *directory;

  directory=getcwd(working_directory,MagickPathExtent);
  (void) directory;
  XChangeToHomeDirectory(resource_info,exception);
}

static void XRestoreDisplayState(const WarningHandler warning_handler,
  char *working_directory,XResourceInfo *resource_info,
  ExceptionInfo *exception)
{
  (void) SetErrorHandler(warning_handler);
  (void) SetWarningHandler(warning_handler);
  /*
    Change to home directory.
  */
  XReturnToHomeDirectory(working_directory,resource_info,exception);
}

typedef struct _DisplayContextInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  XWindowInfo
    **magick_windows;

  unsigned int
    number_windows;

  size_t
    *state;

  char
    *command;

  Image
    **image;

  char
    **argv;

  int
    argc;
} DisplayContextInput;

static void XInitializeDisplayColormap(const DisplayContextInput *input,
  DisplayEventContext *event_context)
{
  Window
    root_window;

  XClassHint
    *class_hints;

  XFontStruct
    *font_info;

  XPixelInfo
    *icon_pixel,
    *pixel;

  XResourceInfo
    *icon_resources;

  XStandardColormap
    *icon_map,
    *map_info;

  XVisualInfo
    *icon_visual,
    *visual_info;

  XWMHints
    *manager_hints;

  map_info=input->windows->map_info;
  icon_map=input->windows->icon_map;
  visual_info=input->windows->visual_info;
  icon_visual=input->windows->icon_visual;
  pixel=input->windows->pixel_info;
  icon_pixel=input->windows->icon_pixel;
  font_info=input->windows->font_info;
  icon_resources=input->windows->icon_resources;
  class_hints=input->windows->class_hints;
  manager_hints=input->windows->manager_hints;
  root_window=XRootWindow(input->display,visual_info->screen);
  (*event_context).display=input->display;
  (*event_context).resource_info=input->resource_info;
  (*event_context).windows=input->windows;
  (*event_context).exception=input->exception;
  (*event_context).pixel=pixel;
  (*event_context).map_info=map_info;
  (*event_context).magick_windows=input->magick_windows;
  (*event_context).number_windows=input->number_windows;
  (*event_context).state=input->state;
  (*event_context).root_window=root_window;
  (*event_context).CommandMenu=CommandMenu;
  (*event_context).command=input->command;
  (*event_context).visual_info=visual_info;
  (*event_context).VirtualMenu=VirtualMenu;
  (*event_context).VirtualCommands=VirtualCommands;
  (*event_context).ShortCutsMenu=ShortCutsMenu;
  (*event_context).ShortCutsCommands=ShortCutsCommands;
  (*event_context).image=input->image;
  (*event_context).icon_visual=icon_visual;
  (*event_context).icon_map=icon_map;
  (*event_context).icon_pixel=icon_pixel;
  (*event_context).Menus=Menus;
  (*event_context).Commands=Commands;
  (*event_context).font_info=font_info;
  (*event_context).argv=input->argv;
  (*event_context).argc=input->argc;
  (*event_context).class_hints=class_hints;
  (*event_context).manager_hints=manager_hints;
  (*event_context).icon_resources=icon_resources;
}

static void XFillDisplayEventContext(const DisplayContextInput *input,
  DisplayEventContext *event_context)
{
  /*
    Initialize font info.
  */
  XLoadDisplayFont(input->display,input->resource_info,input->windows);
  /*
    Initialize Standard Colormap.
  */
  XInitializeDisplayColormap(input,event_context);
}

typedef struct _DisplayWindowsInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  Image
    **image;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;
} DisplayWindowsInput;

static void XOpenDisplayWindows(const DisplayWindowsInput *input,
  const DisplayEventTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *const display_image = *targets->display_image;

  Image
    **const image = input->image;

  XPoint
    *const vid_info = targets->vid_info;

  WarningHandler
    *const warning_handler = targets->warning_handler;

  XWindows
    **const windows = targets->windows;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindowInfo
    **const magick_windows = input->magick_windows;

  char
    *const working_directory = input->working_directory;

  unsigned int
    *const number_windows = targets->number_windows;

  (*warning_handler)=(WarningHandler) NULL;
  (*windows)=XSetWindows((XWindows *) ~0);
  if ((*windows) != (XWindows *) NULL)
    {
      (*warning_handler)=XRestoreWorkingDirectory(resource_info,
        working_directory,exception);
    }
  else
    {
      /*
        Allocate windows structure.
      */
      (*windows)=XAllocateDisplayWindows(display,resource_info,display_image,
        image);
      /*
        Initialize window id's.
      */
      XInitializeWindowIds((*windows),magick_windows,number_windows);
      (*vid_info).x=0;
      (*vid_info).y=0;
    }
}

typedef struct _DisplaySetUpInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  size_t
    *state;
} DisplaySetUpInput;

static void XSetUpDisplay(const DisplaySetUpInput *input,
  DisplayEventContext *event_context,const DisplayEventTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *const display_image = *targets->display_image;

  Image
    **const nexus = targets->nexus;

  MagickStatusType
    *const status = targets->status;

  size_t
    *const state = input->state;

  time_t
    *const timer = targets->timer;

  WarningHandler
    *const warning_handler = targets->warning_handler;

  XFontStruct
    *const font_info = event_context->font_info;

  XGCValues
    *const context_values = targets->context_values;

  XPixelInfo
    *const pixel = event_context->pixel;

  XResourceInfo
    *const resource_info = input->resource_info;

  XStandardColormap
    *const map_info = event_context->map_info;

  XVisualInfo
    *const visual_info = event_context->visual_info;

  MagickStatusType
    context_mask;

  time_t
    update_time;

  (*nexus)=NewImageList();
  XLogDisplayImage(resource_info,display_image);
  XMakeStandardColormap(display,visual_info,resource_info,display_image,
    map_info,pixel,exception);
  display_image->taint=MagickFalse;
  /*
    Initialize graphic context.
  */
  XMakeContextWindow(event_context);
  (*context_values).background=pixel->background_color.pixel;
  (*context_values).font=font_info->fid;
  (*context_values).foreground=pixel->foreground_color.pixel;
  (*context_values).graphics_exposures=MagickFalse;
  context_mask=(MagickStatusType)
    (GCBackground | GCFont | GCForeground | GCGraphicsExposures);
  XCreateDisplayGCs(event_context,display_image,context_mask,
    context_values);
  /*
    Initialize icon window.
  */
  XMakeIconWindow(event_context,display_image,context_values);
  /*
    Initialize Image window.
  */
  XMakeDisplayWindows(event_context,display_image);
  /*
    Set our progress monitor and warning handlers.
  */
  (*warning_handler)=XInstallWarningHandlers(resource_info,(*warning_handler));
  /*
    Initialize Image and Magnify X images.
  */
  (*status)=XMakeDisplayXImages(event_context,display_image);
  /*
    Respond to events.
  */
  (*timer)=XGetDisplayTimer(display_image);
  update_time=XGetUpdateTime(resource_info,display_image);
  *state&=(unsigned int) (~FormerImageState);
  *state&=(unsigned int) (~MontageImageState);
  *state&=(unsigned int) (~NextImageState);
  (*event_context).context_mask=context_mask;
  (*event_context).update_time=update_time;
}

typedef struct _DisplayFinishInput
{
  size_t
    *state;

  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  char
    *working_directory;

  Image
    **image;
} DisplayFinishInput;

static Image *XFinishDisplayImage(const DisplayFinishInput *finish_input,
  const DisplayEventContext *event_context,const DisplayEventTargets *targets)
{
  Display
    *const display = finish_input->display;

  ExceptionInfo
    *const exception = finish_input->exception;

  Image
    **const display_image = targets->display_image;

  Image
    **const image = finish_input->image;

  Image
    **const nexus = targets->nexus;

  XResourceInfo
    *const resource_info = finish_input->resource_info;

  XWindows
    *const windows = *targets->windows;

  char
    *const working_directory = finish_input->working_directory;

  size_t
    *const state = finish_input->state;

  const WarningHandler
    warning_handler = *targets->warning_handler;

  {
    const DisplayExitInputs inputs = { state, display, resource_info, windows,
      exception };

    XConfirmDisplayExit(&inputs,display_image);
  }
  XCloseDisplayWindows(event_context);
  /*
    Restore our progress monitor and warning handlers.
  */
  XRestoreDisplayState(warning_handler,working_directory,resource_info,
    exception);
  *image=(*display_image);
  return((*nexus));
}

typedef struct _DisplayImageInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  ExceptionInfo
    *exception;

  Image
    **image;

  char
    *working_directory;

  XWindowInfo
    **magick_windows;

  size_t
    *state;

  char
    *command;

  char
    **argv;

  int
    argc;
} DisplayImageInput;

static void XInitializeDisplayImage(const DisplayImageInput *image_input,
  DisplayEventContext *event_context,const DisplayEventTargets *targets)
{
  Display
    *const display = image_input->display;

  ExceptionInfo
    *const exception = image_input->exception;

  Image
    **const image = image_input->image;

  XResourceInfo
    *const resource_info = image_input->resource_info;

  XWindowInfo
    **const magick_windows = image_input->magick_windows;

  XWindows
    **const windows = targets->windows;

  char
    *const command = image_input->command;

  char
    *const working_directory = image_input->working_directory;

  char
    **const argv = image_input->argv;

  const int
    argc = image_input->argc;

  size_t
    *const state = image_input->state;

  unsigned int
    *const number_windows = targets->number_windows;

  {
    const DisplayWindowsInput input = { display, resource_info, exception,
      image, working_directory, magick_windows };

    XOpenDisplayWindows(&input,targets);
  }
  {
    const DisplayContextInput input = { display, resource_info, (*windows),
      exception, magick_windows, (*number_windows), state, command, image, argv,
      argc };

    XFillDisplayEventContext(&input,event_context);
  }
  {
    const DisplaySetUpInput input = { display, resource_info, exception,
      state };

    XSetUpDisplay(&input,event_context,targets);
  }
}

static void XLookupKeyPress(const DisplayEventContext *context,
  const DisplayEventTargets *targets,char *command,const size_t extent)
{
  XEvent
    *const event = targets->event;

  int
    length;

  KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  length=XLookupString((XKeyEvent *) &event->xkey,command,(int)
    extent,&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  XHandleKeyPress(context,key_symbol,targets);
}

static void XLookupKeyRelease(const XResourceInfo *resource_info,
  XEvent *event,char *command,const size_t extent)
{
  KeySym
    key_symbol;

  /*
    Respond to a user key release.
  */
  (void) XLookupString((XKeyEvent *) &event->xkey,command,(int)
    extent,&key_symbol,(XComposeStatus *) NULL);
  XLogKeyRelease(resource_info,key_symbol,command);
}

MagickExport Image *XDisplayImage(Display *display,XResourceInfo *resource_info,
  char **argv,int argc,Image **image,size_t *state,ExceptionInfo *exception)
{
  char
    command[MagickPathExtent];

  Image
    *display_image,
    *nexus;

  MagickStatusType
    status;

  static char
    working_directory[MagickPathExtent];

  static XPoint
    vid_info;

  static XWindowInfo
    *magick_windows[MaxXWindows];

  static unsigned int
    number_windows;

  time_t
    timer;

  WarningHandler
    warning_handler;

  XEvent
    event;

  XGCValues
    context_values;

  XWindows
    *windows;

  DisplayEventContext
    event_context;

  const DisplayEventTargets
    event_targets = { &nexus, &display_image, &event, &vid_info, &timer,
      &context_values, &status, &warning_handler, &windows, &number_windows };

  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickCoreSignature);
  XTraceDisplayImage(image);
  display_image=(*image);
  {
    const DisplayImageInput input = { display, resource_info, exception, image,
      working_directory, magick_windows, state, command, argv, argc };

    XInitializeDisplayImage(&input,&event_context,&event_targets);
  }
  do
  {
    /*
      Handle a window event.
    */
    if (XNextDisplayEvent(&event_context,&event_targets) != MagickFalse)
      continue;
    switch (event.type)
    {
      case KeyPress:
        XLookupKeyPress(&event_context,&event_targets,command,
          sizeof(command));
        break;
      case KeyRelease:
        XLookupKeyRelease(resource_info,&event,command,sizeof(command));
        break;
      default:
      {
        XDispatchButtonEvent(&event_context,&event_targets);
        break;
      }
    }
  } while (!(*state & ExitState));
  {
    const DisplayFinishInput input = { state, display, resource_info,
      exception, working_directory, image };

    return(XFinishDisplayImage(&input,&event_context,&event_targets));
  }
}
#endif
