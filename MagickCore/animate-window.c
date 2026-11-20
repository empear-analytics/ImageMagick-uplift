/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                AAA   N   N  IIIII  M   M   AAA   TTTTT  EEEEE               %
%               A   A  NN  N    I    MM MM  A   A    T    E                   %
%               AAAAA  N N N    I    M M M  AAAAA    T    EEE                 %
%               A   A  N  NN    I    M   M  A   A    T    E                   %
%               A   A  N   N  IIIII  M   M  A   A    T    EEEEE               %
%                                                                             %
%                  W   W  IIIII  N   N  DDDD    OOO   W   W                   %
%                  W   W    I    NN  N  D   D  O   O  W   W                   %
%                  W W W    I    N N N  D   D  O   O  W W W                   %
%                  WW WW    I    N  NN  D   D  O   O  WW WW                   %
%                  W   W  IIIII  N   N  DDDD    OOO   W   W                   %
%                                                                             %
%                                                                             %
%                    MagickCore X Window Animation Methods                    %
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
#include "MagickCore/animate.h"
#include "MagickCore/animate-private.h"
#include "MagickCore/attribute.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/color-private.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/colorspace-private.h"
#include "MagickCore/constitute.h"
#include "MagickCore/delegate.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image-private.h"
#include "MagickCore/layer.h"
#include "MagickCore/list.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/log.h"
#include "MagickCore/image.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/resource_.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

#include "MagickCore/animate-window-menu-private.h"
#include "MagickCore/animate-window-command-private.h"


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X A n i m a t e B a c k g r o u n d I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XAnimateBackgroundImage() animates an image sequence in the background of
%  a window.
%
%  The format of the XAnimateBackgroundImage method is:
%
%      void XAnimateBackgroundImage(Display *display,
%        XResourceInfo *resource_info,Image *images,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o images: the image list.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport void XAnimateBackgroundImage(Display *display,
  XResourceInfo *resource_info,Image *images,ExceptionInfo *exception)
{
  char
    visual_type[MagickPathExtent];

  Image
    **image_list;

  size_t
    number_scenes;

  static XPixelInfo
    pixel;

  static XStandardColormap
    *map_info;

  static XVisualInfo
    *visual_info = (XVisualInfo *) NULL;

  static XWindowInfo
    window_info;

  unsigned int
    height,
    width;

  Window
    root_window;

  XResourceInfo
    resources;

  XWindowAttributes
    window_attributes;

  AnimateBackgroundSetup
    setup;

  const AnimateBackgroundTargets
    targets = { &window_info, &resources, &window_attributes, &width,
      &height, &pixel };

  const AnimateBackgroundEarlyTargets
    early_targets = { &root_window, &resources, &window_info, &pixel,
      &map_info, &visual_info };

  /*
    Determine target window.
  */
  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  XFindBackgroundWindow(display,resource_info,&early_targets);
  if (window_info.id == (Window) NULL)
    {
      ThrowXWindowException(XServerError,"NoWindowWithSpecifiedIDExists",
        resources.window_id);
      return;
    }
  /*
    Determine window visual id.
  */
  XGetBackgroundVisualType(display,&targets,visual_type);
  XCreateBackgroundVisual(display,images->filename,visual_type,
    &early_targets);
  /*
    Free previous root colors.
  */
  images=XPrepareBackgroundImages(display,images,exception,&early_targets);
  /*
    Sort images by increasing scene number.
  */
  number_scenes=GetImageListLength(images);
  image_list=XSortAnimateScenes(images,number_scenes,exception);
  setup.display=display;
  setup.resource_info=resource_info;
  setup.exception=exception;
  setup.images=images;
  setup.image_list=image_list;
  setup.number_scenes=number_scenes;
  setup.visual_info=visual_info;
  setup.map_info=map_info;
  /*
    Initialize Standard Colormap.
  */
  XMakeBackgroundColormap(&setup,&targets);
  /*
    Graphic context superclass.
  */
  XInitializeBackgroundWindow(&setup,&targets);
  /*
    Create the X image.
  */
  XCreateBackgroundXImage(&setup,&targets);
  /*
    Adjust image dimensions as specified by backdrop or geometry options.
  */
  XAdjustBackgroundSize(&setup,&targets);
  /*
    Create the X pixmap.
  */
  XCreateBackgroundPixmaps(&setup,&targets);
  XCreateBackgroundScenes(&setup,&targets);
  window_info.pixel_info=(&pixel);
  /*
    Display pixmap on the window.
  */
  XRunBackgroundAnimation(&setup,&targets);
  XReleaseBackgroundImages(display,images,image_list);
}

#include "MagickCore/animate-window-scene-private.h"

#include "MagickCore/animate-window-events-private.h"

#include "MagickCore/animate-window-session-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X A n i m a t e I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XAnimateImages() displays an image via X11.
%
%  The format of the XAnimateImages method is:
%
%      Image *XAnimateImages(Display *display,XResourceInfo *resource_info,
%        char **argv,const int argc,Image *images,ExceptionInfo *exception)
%
%  A description of each parameter follows:
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
%    o images: the image list.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport Image *XAnimateImages(Display *display,
  XResourceInfo *resource_info,char **argv,const int argc,Image *images,
  ExceptionInfo *exception)
{
  static char
    working_directory[MagickPathExtent];

  static size_t
    number_windows;

  static XWindowInfo
    *magick_windows[MaXWindows];

  WarningHandler
    warning_handler;

  XWindows
    *windows;

  AnimateWindowInputs
    window_inputs;

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  warning_handler=(WarningHandler) NULL;
  windows=XSetWindows((XWindows *) ~0);
  if (windows != (XWindows *) NULL)
    {
      warning_handler=XEnterWorkingDirectory(resource_info,working_directory,
        exception);
    }
  else
    {
      windows=XCreateAnimateWindows(display,resource_info,images);
      /*
        Initialize window id's.
      */
      XInitializeMagickWindows(windows,magick_windows,&number_windows);
    }
  XLoadAnimateFont(display,resource_info,windows);
  window_inputs.display=display;
  window_inputs.resource_info=resource_info;
  window_inputs.argv=argv;
  window_inputs.argc=argc;
  window_inputs.windows=windows;
  window_inputs.images=images;
  window_inputs.exception=exception;
  window_inputs.warning_handler=warning_handler;
  window_inputs.working_directory=working_directory;
  window_inputs.magick_windows=magick_windows;
  window_inputs.number_windows=number_windows;
  return(XBindAnimateWindows(&window_inputs));
}

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
%    o status: Method XSaveImage return True if the image is
%      written.  False is returned is there is a memory shortage or if the
%      image fails to write.
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
*/
static MagickBooleanType IsPostscriptFormat(const ImageInfo *image_info)
{
  return(((LocaleCompare(image_info->magick,"EPS") == 0) ||
    (LocaleCompare(image_info->magick,"PDF") == 0) ||
    (LocaleCompare(image_info->magick,"PS") == 0) ||
    (LocaleCompare(image_info->magick,"PS2") == 0)) ? MagickTrue : MagickFalse);
}

static void XChangeToImageDirectory(const Image *image,char *filename,
  ExceptionInfo *exception)
{
  char
    path[MagickPathExtent];

  int
    status;

  GetPathComponent(image->filename,HeadPath,path);
  GetPathComponent(image->filename,TailPath,filename);
  if (*path == '\0')
    (void) CopyMagickString(path,".",MagickPathExtent);
  status=chdir(path);
  if (status == -1)
    (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
      "UnableToOpenFile","%s",path);
}

static void XRequestPageGeometry(Display *display,XWindows *windows,
  ImageInfo *image_info)
{
  char
    geometry[MagickPathExtent];

  /*
    Request page geometry from user.
  */
  (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
  if (LocaleCompare(image_info->magick,"PDF") == 0)
    (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
  if (image_info->page != (char *) NULL)
    (void) CopyMagickString(geometry,image_info->page,MagickPathExtent);
  {
    const XListBrowserRequest
      browse_request = { PageSizes, "Select", "Select page geometry:",
        geometry };

    XListBrowserWidget(display,windows,&windows->widget,&browse_request);
  }
  if (*geometry != '\0')
    image_info->page=GetPageGeometry(geometry);
}

static MagickBooleanType IsJPEGFormat(const ImageInfo *image_info)
{
  return(((LocaleCompare(image_info->magick,"JPEG") == 0) ||
    (LocaleCompare(image_info->magick,"JPG") == 0)) ? MagickTrue :
    MagickFalse);
}

static void XSetJPEGQuality(Image *image,ImageInfo *image_info,
  const char *quality,const int status)
{
  image->quality=StringToUnsignedLong(quality);
  image_info->interlace=status != MagickFalse ?  NoInterlace :
    PlaneInterlace;
}

static MagickStatusType XWriteSavedImages(ImageInfo *image_info,
  Image *image,char *filename,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  image=GetFirstImageInList(image);
  status=WriteImages(image_info,image,filename,exception);
  if (status != MagickFalse)
    image->taint=MagickFalse;
  return(status);
}

static MagickBooleanType XFinishSaveImage(Display *display,
  XWindows *windows,ImageInfo *image_info,const MagickStatusType status)
{
  image_info=DestroyImageInfo(image_info);
  XSetCursorState(display,windows,MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

typedef struct _AnimateSaveInputs
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

  char
    *filename;
} AnimateSaveInputs;

static MagickBooleanType XSaveImageFile(const AnimateSaveInputs *save)
{
  Display
    *const display = save->display;

  XResourceInfo
    *const resource_info = save->resource_info;

  XWindows
    *const windows = save->windows;

  Image
    *const image = save->image;

  ExceptionInfo
    *const exception = save->exception;

  char
    *const filename = save->filename;

  ImageInfo
    *image_info;

  MagickStatusType
    status;

  image_info=CloneImageInfo(resource_info->image_info);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) SetImageInfo(image_info,1,exception);
  if (IsJPEGFormat(image_info) != MagickFalse)
    {
      char
        quality[MagickPathExtent];

      int
        status;

      /*
        Request JPEG quality from user.
      */
      (void) FormatLocaleString(quality,MagickPathExtent,"%.17g",(double)
        image_info->quality);
      {
        const XDialogRequest
          dialog_request = { "Save", "Enter JPEG quality:", quality };

        status=XDialogWidget(display,windows,&dialog_request);
      }
      if (*quality == '\0')
        return(MagickTrue);
      XSetJPEGQuality(image,image_info,quality,status);
    }
  if (IsPostscriptFormat(image_info) != MagickFalse)
    XRequestPageGeometry(display,windows,image_info);
  /*
    Write image.
  */
  status=XWriteSavedImages(image_info,image,filename,exception);
  return(XFinishSaveImage(display,windows,image_info,status));
}

static MagickBooleanType XSaveImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  AnimateSaveInputs
    save;

  char
    filename[MagickPathExtent];

  /*
    Request file name from user.
  */
  if (resource_info->write_filename != (char *) NULL)
    (void) CopyMagickString(filename,resource_info->write_filename,
      MagickPathExtent);
  else
    XChangeToImageDirectory(image,filename,exception);
  XFileBrowserWidget(display,windows,"Save",filename);
  if (*filename == '\0')
    return(MagickTrue);
  if (IsPathAccessible(filename) != MagickFalse)
    {
      int
        status;

      /*
        File exists-- seek user's permission before overwriting.
      */
      status=XConfirmWidget(display,windows,"Overwrite",filename);
      if (status == 0)
        return(MagickTrue);
    }
  save.display=display;
  save.resource_info=resource_info;
  save.windows=windows;
  save.image=image;
  save.exception=exception;
  save.filename=filename;
  return(XSaveImageFile(&save));
}
#endif
