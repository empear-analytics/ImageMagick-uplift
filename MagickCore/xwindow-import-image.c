/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               X   X  W   W  IIIII  N   N  DDDD    OOO   W   W               %
%                X X   W   W    I    NN  N  D   D  O   O  W   W               %
%                 X    W W W    I    N N N  D   D  O   O  W W W               %
%                X X   WW WW    I    N  NN  D   D  O   O  WW WW               %
%               X   X  W   W  IIIII  N   N  DDDD    OOO   W   W               %
%                                                                             %
%                  IIIII  M   M  PPPP    OOO   RRRR   TTTTT                   %
%                    I    MM MM  P   P  O   O  R   R    T                     %
%                    I    M M M  PPPP   O   O  RRRR     T                     %
%                    I    M   M  P      O   O  R R      T                     %
%                  IIIII  M   M  P       OOO   R  R     T                     %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                 MagickCore X11 Window Image Import Methods                  %
%                                                                             %
%                               Software Design                               %
%                                    Cristy                                   %
%                                  July 1992                                  %
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
#include "MagickCore/animate.h"
#include "MagickCore/artifact.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colormap.h"
#include "MagickCore/composite.h"
#include "MagickCore/constitute.h"
#include "MagickCore/display.h"
#include "MagickCore/distort.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/identify.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale_.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/memory-private.h"
#include "MagickCore/monitor.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resource_.h"
#include "MagickCore/resize.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/token.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"
#include "MagickCore/version.h"
#if defined(__BEOS__)
#include <OS.h>
#endif
#if defined(MAGICKCORE_X11_DELEGATE)
#include <X11/Xproto.h>
#include <X11/Xlocale.h>
#if defined(MAGICK_HAVE_POLL)
# include <sys/poll.h>
#endif
#if defined(MAGICKCORE_HAVE_SHARED_MEMORY)
#if defined(MAGICKCORE_HAVE_MACHINE_PARAM_H)
# include <machine/param.h>
#endif
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif
#if defined(MAGICKCORE_HAVE_SHAPE)
#include <X11/extensions/shape.h>
#endif
#include "MagickCore/xwindow-import-private.h"
#include "MagickCore/xwindow-import-image-window-private.h"


static void XAlertImportStart(Display *display,const XImportInfo *ximage_info)
{
  /*
    Alert the user not to alter the screen.
  */
  if (ximage_info->silent == MagickFalse)
    (void) XBell(display,0);
}

static void XAlertImportDone(Display *display,const XImportInfo *ximage_info)
{
  if (ximage_info->silent == MagickFalse)
    {
      /*
        Alert the user we're done.
      */
      (void) XBell(display,0);
      (void) XBell(display,0);
    }
}

static void XSelectImportWindow(Display *display,const ImageInfo *image_info,
  RectangleInfo *crop_info,Window *target)
{
  if ((*target) == (Window) NULL)
    (*target)=XSelectWindow(display,crop_info);
  if ((*target) == (Window) NULL)
    ThrowXWindowException(XServerError,"UnableToReadXWindowImage",
      image_info->filename);
}

static Image *XCropWindowImage(Image *image,const RectangleInfo crop_info,
  ExceptionInfo *exception)
{
  if ((crop_info.width != 0) && (crop_info.height != 0))
    {
      Image
        *crop_image;

      /*
        Crop image as defined by the cropping rectangle.
      */
      crop_image=CropImage(image,&crop_info,exception);
      if (crop_image != (Image *) NULL)
        {
          image=DestroyImage(image);
          image=crop_image;
        }
    }
  return(image);
}

static void XNameWindowImage(Display *display,const Window target,
  const ImageInfo *image_info,Image *image)
{
  Status
    status;

  XTextProperty
    window_name;

  status=XGetWMName(display,target,&window_name);
  if (status == True)
    {
      if (*image_info->filename == '\0')
        (void) CopyMagickString(image->filename,(char *) window_name.value,
          (size_t) MagickMin(window_name.nitems+1,MagickPathExtent));
      (void) XFree((void *) window_name.value);
    }
}

typedef struct _XImportTargets
{
  RectangleInfo
    *crop_info;

  Window
    *root,
    *target;
} XImportTargets;

static MagickBooleanType XGetScreenCrop(Display *display,
  const XImportInfo *ximage_info,const Window root,
  const XImportTargets *targets)
{
  RectangleInfo
    *const crop_info = targets->crop_info;

  Status
    status;

  Window
    *const target = targets->target;

  int
    x;

  if (ximage_info->screen != MagickFalse)
    {
      int
        y;

      Window
        child;

      XWindowAttributes
        window_attributes;

      /*
        Obtain window image directly from screen.
      */
      status=XGetWindowAttributes(display,*target,&window_attributes);
      if (status == False)
        return(MagickFalse);
      (void) XTranslateCoordinates(display,*target,root,0,0,&x,&y,&child);
      (*crop_info).x=(ssize_t) x;
      (*crop_info).y=(ssize_t) y;
      (*crop_info).width=(size_t) window_attributes.width;
      (*crop_info).height=(size_t) window_attributes.height;
      if (ximage_info->borders != 0)
        {
          /*
            Include border in image.
          */
          (*crop_info).x-=window_attributes.border_width;
          (*crop_info).y-=window_attributes.border_width;
          (*crop_info).width+=(size_t) (window_attributes.border_width << 1);
          (*crop_info).height+=(size_t) (window_attributes.border_width << 1);
        }
      *target=root;
    }
  return(MagickTrue);
}

static void XResolveImportTarget(Display *display,
  const XImportInfo *ximage_info,const Window prior_target,
  const XImportTargets *targets)
{
  int
    x;

  Status
    status;

  Window
    client,
    *const root = targets->root,
    *const target = targets->target;

  client=(*target);   /* obsolete */
  if ((*target) != (*root))
    {
      unsigned int
        d;

      status=XGetGeometry(display,(*target),root,&x,&x,&d,&d,&d,&d);
      if (status != False)
        {
          XFindFrameWindow(display,root,target);
          /*
            Get client window.
          */
          client=XClientWindow(display,(*target));
          if (ximage_info->frame == MagickFalse)
            (*target)=client;
          if ((ximage_info->frame == MagickFalse) &&
              (prior_target != MagickFalse))
            (*target)=prior_target;
        }
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X I m p o r t I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XImportImage() reads an image from an X window.
%
%  The format of the XImportImage method is:
%
%      Image *XImportImage(const ImageInfo *image_info,XImportInfo *ximage_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o ximage_info: Specifies a pointer to an XImportInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

MagickExport Image *XImportImage(const ImageInfo *image_info,
  XImportInfo *ximage_info,ExceptionInfo *exception)
{
  Display
    *display;

  Image
    *image;

  RectangleInfo
    crop_info;

  Window
    prior_target,
    root,
    target;

  WindowImageContext
    window_context;

  const XImportTargets
    targets = { &crop_info, &root, &target };

  /*
    Open X server connection.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(ximage_info != (XImportInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  display=XOpenDisplay(image_info->server_name);
  if (display == (Display *) NULL)
    {
      ThrowXWindowException(XServerError,"UnableToOpenXServer",
        XDisplayName(image_info->server_name));
      return((Image *) NULL);
    }
  /*
    Set our forgiving exception handler.
  */
  (void) XSetErrorHandler(XError);
  /*
    Select target window.
  */
  crop_info.x=0;
  crop_info.y=0;
  crop_info.width=0;
  crop_info.height=0;
  root=XRootWindow(display,XDefaultScreen(display));
  target=XSelectNamedWindow(display,root,image_info);
  /*
    If target window is not defined, interactively select one.
  */
  prior_target=target;
  XSelectImportWindow(display,image_info,&crop_info,&target);
  XResolveImportTarget(display,ximage_info,prior_target,&targets);
  if (XGetScreenCrop(display,ximage_info,root,&targets) == MagickFalse)
    {
      ThrowXWindowException(XServerError,"UnableToReadXWindowAttributes",
        image_info->filename);
      (void) XCloseDisplay(display);
      return((Image *) NULL);
    }
  XSetDescendForColormaps(display,target,ximage_info);
  XAlertImportStart(display,ximage_info);
  /*
    Get image by window id.
  */
  (void) XGrabServer(display);
  window_context.display=display;
  window_context.exception=exception;
  image=XGetWindowImage(&window_context,target,ximage_info->borders,
    ximage_info->descend ? 1U : 0U);
  (void) XUngrabServer(display);
  if (image != (Image *) NULL)
    {
      (void) CopyMagickString(image->filename,image_info->filename,
        MagickPathExtent);
      image=XCropWindowImage(image,crop_info,exception);
      XNameWindowImage(display,target,image_info,image);
    }
  XAlertImportDone(display,ximage_info);
  (void) XCloseDisplay(display);
  return(image);
}

#else

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X I m p o r t I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XImportImage() reads an image from an X window.
%
%  The format of the XImportImage method is:
%
%      Image *XImportImage(const ImageInfo *image_info,XImportInfo *ximage_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info..
%
%    o ximage_info: Specifies a pointer to an XImportInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *XImportImage(const ImageInfo *image_info,
  XImportInfo *ximage_info,ExceptionInfo *exception)
{
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(ximage_info != (XImportInfo *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  (void) ximage_info;
  (void) exception;
  return((Image *) NULL);
}
#endif
