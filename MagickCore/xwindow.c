/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%              X   X  W   W  IIIII  N   N  DDDD    OOO   W   W                %
%               X X   W   W    I    NN  N  D   D  O   O  W   W                %
%                X    W   W    I    N N N  D   D  O   O  W   W                %
%               X X   W W W    I    N  NN  D   D  O   O  W W W                %
%              X   X   W W   IIIII  N   N  DDDD    OOO    W W                 %
%                                                                             %
%                                                                             %
%                       MagickCore X11 Utility Methods                        %
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

/*
  X defines.
*/

#define AccentuateModulate  ScaleCharToQuantum(80)

#define XLIB_ILLEGAL_ACCESS  1
#undef ForgetGravity
#undef NorthWestGravity
#undef NorthGravity
#undef NorthEastGravity
#undef WestGravity
#undef CenterGravity
#undef EastGravity
#undef SouthWestGravity
#undef SouthGravity
#undef SouthEastGravity
#undef StaticGravity

#undef index
#if defined(hpux9)
#define XFD_SET  int
#else
#define XFD_SET  fd_set
#endif

/*
  Enumeration declarations.
*/
typedef enum
{
#undef DoRed
  DoRed = 0x0001,
#undef DoGreen
  DoGreen = 0x0002,
#undef DoBlue
  DoBlue = 0x0004,
  DoMatte = 0x0008
} XColorFlags;


/*
  Method prototypes.
*/


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y X R e s o u r c e s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyXResources() destroys any X resources.
%
%  The format of the DestroyXResources method is:
%
%      void DestroyXResources()
%
%  A description of each parameter follows:
%
*/
#if defined(MAGICKCORE_HAVE_SHARED_MEMORY)
static void DetachXShmSegment(XShmSegmentInfo *segment_info)
{
  if (segment_info != (XShmSegmentInfo *) NULL)
    if (segment_info[0].shmid >= 0)
      {
        if (segment_info[0].shmaddr != NULL)
          (void) shmdt(segment_info[0].shmaddr);
        (void) shmctl(segment_info[0].shmid,IPC_RMID,0);
        segment_info[0].shmaddr=NULL;
        segment_info[0].shmid=(-1);
      }
}
#endif

static void FreeXWindowCursors(Display *display,XWindowInfo *window)
{
  if (window->cursor != (Cursor) NULL)
    {
      (void) XFreeCursor(display,window->cursor);
      window->cursor=(Cursor) NULL;
    }
  if (window->busy_cursor != (Cursor) NULL)
    {
      (void) XFreeCursor(display,window->busy_cursor);
      window->busy_cursor=(Cursor) NULL;
    }
  if (window->highlight_stipple != (Pixmap) NULL)
    {
      (void) XFreePixmap(display,window->highlight_stipple);
      window->highlight_stipple=(Pixmap) NULL;
    }
  if (window->shadow_stipple != (Pixmap) NULL)
    {
      (void) XFreePixmap(display,window->shadow_stipple);
      window->shadow_stipple=(Pixmap) NULL;
    }
}

static void DestroyXWindowNames(XWindowInfo *window)
{
  if (window->name != (char *) NULL)
    window->name=(char *) RelinquishMagickMemory(window->name);
  if (window->icon_name != (char *) NULL)
    window->icon_name=(char *) RelinquishMagickMemory(window->icon_name);
}

static void DestroyXWindowImagesAndId(Display *display,XWindowInfo *window)
{
  if (window->matte_image != (XImage *) NULL)
    {
      XDestroyImage(window->matte_image);
      window->matte_image=(XImage *) NULL;
    }
  if (window->ximage != (XImage *) NULL)
    {
      XDestroyImage(window->ximage);
      window->ximage=(XImage *) NULL;
    }
  if (window->pixmap != (Pixmap) NULL)
    {
      (void) XFreePixmap(display,window->pixmap);
      window->pixmap=(Pixmap) NULL;
    }
  if (window->id != (Window) NULL)
    {
      (void) XDestroyWindow(display,window->id);
      window->id=(Window) NULL;
    }
}

static void DestroyXWindowOwnedImage(Display *display,XWindowInfo *window)
{
  if (window->destroy != MagickFalse)
    {
      if (window->image != (Image *) NULL)
        {
          window->image=DestroyImage(window->image);
          window->image=NewImageList();
        }
      if (window->matte_pixmap != (Pixmap) NULL)
        {
          (void) XFreePixmap(display,window->matte_pixmap);
          window->matte_pixmap=(Pixmap) NULL;
        }
    }
}

static void DestroyXIconPixelInfo(XWindows *windows)
{
  if (windows->icon_pixel != (XPixelInfo *) NULL)
    {
      if (windows->icon_pixel->pixels != (unsigned long *) NULL)
        windows->icon_pixel->pixels=(unsigned long *)
          RelinquishMagickMemory(windows->icon_pixel->pixels);
      if (windows->icon_pixel->annotate_context != (GC) NULL)
        XFreeGC(windows->display,windows->icon_pixel->annotate_context);
      windows->icon_pixel=(XPixelInfo *)
        RelinquishMagickMemory(windows->icon_pixel);
    }
}

static void DestroyXPixelInfo(XWindows *windows)
{
  if (windows->pixel_info != (XPixelInfo *) NULL)
    {
      if (windows->pixel_info->pixels != (unsigned long *) NULL)
        windows->pixel_info->pixels=(unsigned long *)
          RelinquishMagickMemory(windows->pixel_info->pixels);
      if (windows->pixel_info->annotate_context != (GC) NULL)
        XFreeGC(windows->display,windows->pixel_info->annotate_context);
      if (windows->pixel_info->widget_context != (GC) NULL)
        XFreeGC(windows->display,windows->pixel_info->widget_context);
      if (windows->pixel_info->highlight_context != (GC) NULL)
        XFreeGC(windows->display,windows->pixel_info->highlight_context);
      windows->pixel_info=(XPixelInfo *)
        RelinquishMagickMemory(windows->pixel_info);
    }
}

static void DestroyXHints(XWindows *windows)
{
  if (windows->font_info != (XFontStruct *) NULL)
    {
      XFreeFont(windows->display,windows->font_info);
      windows->font_info=(XFontStruct *) NULL;
    }
  if (windows->class_hints != (XClassHint *) NULL)
    {
      if (windows->class_hints->res_name != (char *) NULL)
        windows->class_hints->res_name=DestroyString(
          windows->class_hints->res_name);
      if (windows->class_hints->res_class != (char *) NULL)
        windows->class_hints->res_class=DestroyString(
          windows->class_hints->res_class);
      XFree(windows->class_hints);
      windows->class_hints=(XClassHint *) NULL;
    }
  if (windows->manager_hints != (XWMHints *) NULL)
    {
      XFree(windows->manager_hints);
      windows->manager_hints=(XWMHints *) NULL;
    }
}

static void DestroyXColormapAndVisualInfo(XWindows *windows)
{
  if (windows->map_info != (XStandardColormap *) NULL)
    {
      XFree(windows->map_info);
      windows->map_info=(XStandardColormap *) NULL;
    }
  if (windows->icon_map != (XStandardColormap *) NULL)
    {
      XFree(windows->icon_map);
      windows->icon_map=(XStandardColormap *) NULL;
    }
  if (windows->visual_info != (XVisualInfo *) NULL)
    {
      XFree(windows->visual_info);
      windows->visual_info=(XVisualInfo *) NULL;
    }
  if (windows->icon_visual != (XVisualInfo *) NULL)
    {
      XFree(windows->icon_visual);
      windows->icon_visual=(XVisualInfo *) NULL;
    }
}

MagickExport void DestroyXResources(void)
{
  int
    i;

  unsigned int
    number_windows;

  XWindowInfo
    *magick_windows[MaxXWindows];

  XWindows
    *windows;

  DestroyXWidget();
  windows=XSetWindows((XWindows *) ~0);
  if ((windows == (XWindows *) NULL) || (windows->display == (Display *) NULL))
    return;
  number_windows=0;
  magick_windows[number_windows++]=(&windows->context);
  magick_windows[number_windows++]=(&windows->group_leader);
  magick_windows[number_windows++]=(&windows->backdrop);
  magick_windows[number_windows++]=(&windows->icon);
  magick_windows[number_windows++]=(&windows->image);
  magick_windows[number_windows++]=(&windows->info);
  magick_windows[number_windows++]=(&windows->magnify);
  magick_windows[number_windows++]=(&windows->pan);
  magick_windows[number_windows++]=(&windows->command);
  magick_windows[number_windows++]=(&windows->widget);
  magick_windows[number_windows++]=(&windows->popup);
  for (i=0; i < (int) number_windows; i++)
  {
    if (magick_windows[i]->mapped != MagickFalse)
      {
        (void) XWithdrawWindow(windows->display,magick_windows[i]->id,
          magick_windows[i]->screen);
        magick_windows[i]->mapped=MagickFalse;
      }
    DestroyXWindowNames(magick_windows[i]);
    FreeXWindowCursors(windows->display,magick_windows[i]);
    DestroyXWindowImagesAndId(windows->display,magick_windows[i]);
    DestroyXWindowOwnedImage(windows->display,magick_windows[i]);
    if (magick_windows[i]->segment_info != (void *) NULL)
      {
#if defined(MAGICKCORE_HAVE_SHARED_MEMORY)
        XShmSegmentInfo
          *segment_info;

        segment_info=(XShmSegmentInfo *) magick_windows[i]->segment_info;
        DetachXShmSegment(segment_info);
#endif
        magick_windows[i]->segment_info=(void *)
          RelinquishMagickMemory(magick_windows[i]->segment_info);
      }
  }
  windows->icon_resources=(XResourceInfo *)
    RelinquishMagickMemory(windows->icon_resources);
  DestroyXIconPixelInfo(windows);
  DestroyXPixelInfo(windows);
  DestroyXHints(windows);
  DestroyXColormapAndVisualInfo(windows);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X B e s t I c o n S i z e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XBestIconSize() returns the "best" icon size.  "Best" is defined as an icon
%  size that maintains the aspect ratio of the image.  If the window manager
%  has preferred icon sizes, one of the preferred sizes is used.
%
%  The format of the XBestIconSize method is:
%
%      void XBestIconSize(Display *display,XWindowInfo *window,Image *image)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o image: the image.
%
*/
static unsigned int XIconWidth(const XIconSize *icon_size,
  const double scale_factor,const unsigned int width)
{
  unsigned int
    icon_width;

  icon_width=(unsigned int) icon_size->min_width;
  while ((int) icon_width < icon_size->max_width)
  {
    if (icon_width >= (unsigned int) (scale_factor*width+0.5))
      break;
    icon_width+=(unsigned int) icon_size->width_inc;
  }
  return(icon_width);
}

static unsigned int XIconHeight(const XIconSize *icon_size,
  const double scale_factor,const unsigned int height)
{
  unsigned int
    icon_height;

  icon_height=(unsigned int) icon_size->min_height;
  while ((int) icon_height < icon_size->max_height)
  {
    if (icon_height >= (unsigned int) (scale_factor*height+0.5))
      break;
    icon_height+=(unsigned int) icon_size->height_inc;
  }
  return(icon_height);
}

static XIconSize *XPreferredIconSize(Display *display,
  const XWindowInfo *window)
{
  int
    number_sizes;

  Window
    root_window;

  XIconSize
    *icon_size,
    *size_list;

  icon_size=(XIconSize *) NULL;
  number_sizes=0;
  root_window=XRootWindow(display,window->screen);
  if (XGetIconSizes(display,root_window,&size_list,&number_sizes) != 0)
    if ((number_sizes > 0) && (size_list != (XIconSize *) NULL))
      icon_size=size_list;
  return(icon_size);
}

MagickPrivate void XBestIconSize(Display *display,XWindowInfo *window,
  Image *image)
{
  int
    i;

  double
    scale_factor;

  unsigned int
    height,
    icon_height,
    icon_width,
    width;

  XIconSize
    *icon_size;

  /*
    Determine if the window manager has specified preferred icon sizes.
  */
  assert(display != (Display *) NULL);
  assert(window != (XWindowInfo *) NULL);
  assert(image != (Image *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  window->width=MaxIconSize;
  window->height=MaxIconSize;
  icon_size=XPreferredIconSize(display,window);
  if (icon_size == (XIconSize *) NULL)
    {
      /*
        Window manager does not restrict icon size.
      */
      icon_size=XAllocIconSize();
      if (icon_size == (XIconSize *) NULL)
        {
          ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
            image->filename);
          return;
        }
      icon_size->min_width=1;
      icon_size->max_width=MaxIconSize;
      icon_size->min_height=1;
      icon_size->max_height=MaxIconSize;
      icon_size->width_inc=1;
      icon_size->height_inc=1;
    }
  /*
    Determine aspect ratio of image.
  */
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  i=0;
  if (window->crop_geometry)
    (void) XParseGeometry(window->crop_geometry,&i,&i,&width,&height);
  /*
    Look for an icon size that maintains the aspect ratio of image.
  */
  scale_factor=(double) icon_size->max_width/width;
  if (scale_factor > ((double) icon_size->max_height/height))
    scale_factor=(double) icon_size->max_height/height;
  icon_width=XIconWidth(icon_size,scale_factor,width);
  icon_height=XIconHeight(icon_size,scale_factor,height);
  (void) XFree((void *) icon_size);
  window->width=icon_width;
  window->height=icon_height;
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C o m p o n e n t T e r m i n u s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XComponentTerminus() destroys the module component.
%
%  The format of the XComponentTerminus method is:
%
%      XComponentTerminus(void)
%
*/
MagickPrivate void XComponentTerminus(void)
{
  DestroyXResources();
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X C o n s t r a i n W i n d o w P o s i t i o n                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XConstrainWindowPosition() assures a window is positioned within the X
%  server boundaries.
%
%  The format of the XConstrainWindowPosition method is:
%
%      void XConstrainWindowPosition(Display *display,XWindowInfo *window_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a XWindowInfo structure.
%
*/
MagickPrivate void XConstrainWindowPosition(Display *display,
  XWindowInfo *window_info)
{
  int
    limit;

  assert(display != (Display *) NULL);
  assert(window_info != (XWindowInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  limit=XDisplayWidth(display,window_info->screen)-(int) window_info->width;
  if (window_info->x < 0)
    window_info->x=0;
  else
    if (window_info->x > (int) limit)
      window_info->x=(int) limit;
  limit=XDisplayHeight(display,window_info->screen)-(int) window_info->height;
  if (window_info->y < 0)
    window_info->y=0;
  else
    if (window_info->y > limit)
      window_info->y=limit;
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D e s t r o y R e s o u r c e I n f o                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDestroyResourceInfo() frees memory associated with the XResourceInfo
%  structure.
%
%  The format of the XDestroyResourceInfo method is:
%
%      void XDestroyResourceInfo(XResourceInfo *resource_info)
%
%  A description of each parameter follows:
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
*/
MagickExport void XDestroyResourceInfo(XResourceInfo *resource_info)
{
  if (resource_info->image_geometry != (char *) NULL)
    resource_info->image_geometry=(char *)
      RelinquishMagickMemory(resource_info->image_geometry);
  if (resource_info->quantize_info != (QuantizeInfo *) NULL)
    resource_info->quantize_info=DestroyQuantizeInfo(
      resource_info->quantize_info);
  if (resource_info->client_name != (char *) NULL)
    resource_info->client_name=(char *)
      RelinquishMagickMemory(resource_info->client_name);
  if (resource_info->name != (char *) NULL)
    resource_info->name=DestroyString(resource_info->name);
  (void) memset(resource_info,0,sizeof(*resource_info));
  (void) XSetWindows((XWindows *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D e s t r o y W i n d o w C o l o r s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDestroyWindowColors() frees X11 color resources previously saved on a
%  window by XRetainWindowColors or programs like xsetroot.
%
%  The format of the XDestroyWindowColors method is:
%
%      void XDestroyWindowColors(Display *display,Window window)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a Window structure.
%
*/
static inline MagickBooleanType IsRootPixmapProperty(const Atom type,
  const int format,const unsigned long length,const unsigned long after)
{
  return(((type == XA_PIXMAP) && (format == 32) && (length == 1) &&
    (after == 0)) ? MagickTrue : MagickFalse);
}

MagickPrivate void XDestroyWindowColors(Display *display,Window window)
{
  Atom
    property,
    type;

  int
    format;

  Status
    status;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  /*
    If there are previous resources on the root window, destroy them.
  */
  assert(display != (Display *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  property=XInternAtom(display,"_XSETROOT_ID",MagickFalse);
  if (property == (Atom) NULL)
    {
      ThrowXWindowException(XServerError,"UnableToCreateProperty",
        "_XSETROOT_ID");
      return;
    }
  status=XGetWindowProperty(display,window,property,0L,1L,MagickTrue,
    (Atom) AnyPropertyType,&type,&format,&length,&after,&data);
  if (status != Success)
    return;
  if (IsRootPixmapProperty(type,format,length,after) != MagickFalse)
    {
      (void) XKillClient(display,(XID) (*((Pixmap *) data)));
      (void) XDeleteProperty(display,window,property);
    }
  if (type != None)
    (void) XFree((void *) data);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t R e s o u r c e C l a s s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetResourceClass() queries the X server for the specified resource name or
%  class.  If the resource name or class is not defined in the database, the
%  supplied default value is returned.
%
%  The format of the XGetResourceClass method is:
%
%      char *XGetResourceClass(XrmDatabase database,const char *client_name,
%        const char *keyword,char *resource_default)
%
%  A description of each parameter follows:
%
%    o database: Specifies a resource database; returned from
%      XrmGetStringDatabase.
%
%    o client_name:  Specifies the application name used to retrieve resource
%      info from the X server database.
%
%    o keyword: Specifies the keyword of the value being retrieved.
%
%    o resource_default: Specifies the default value to return if the query
%      fails to find the specified keyword/class.
%
*/
static int XUppercaseKeysym(int c)
{
  if ((c >= XK_a) && (c <= XK_z))
    c-=(XK_a-XK_A);
  else
    if ((c >= XK_agrave) && (c <= XK_odiaeresis))
      c-=(XK_agrave-XK_Agrave);
    else
      if ((c >= XK_oslash) && (c <= XK_thorn))
        c-=(XK_oslash-XK_Ooblique);
  return(c);
}

MagickExport char *XGetResourceClass(XrmDatabase database,
  const char *client_name,const char *keyword,char *resource_default)
{
  char
    resource_class[MagickPathExtent],
    resource_name[MagickPathExtent];

  static char
    *resource_type;

  Status
    status;

  XrmValue
    resource_value;

  if (database == (XrmDatabase) NULL)
    return(resource_default);
  *resource_name='\0';
  *resource_class='\0';
  if (keyword != (char *) NULL)
    {
      int
        c,
        k;

      /*
        Initialize resource keyword and class.
      */
      (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.%s",
        client_name,keyword);
      c=(int) (*client_name);
      c=XUppercaseKeysym(c);
      k=(int) (*keyword);
      k=XUppercaseKeysym(k);
      (void) FormatLocaleString(resource_class,MagickPathExtent,"%c%s.%c%s",c,
        client_name+1,k,keyword+1);
    }
  status=XrmGetResource(database,resource_name,resource_class,&resource_type,
    &resource_value);
  if (status == False)
    return(resource_default);
  return(resource_value.addr);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t R e s o u r c e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetResourceInfo() initializes the ResourceInfo structure.
%
%  The format of the XGetResourceInfo method is:
%
%      void XGetResourceInfo(const ImageInfo *image_info,XrmDatabase database,
%        const char *client_name,XResourceInfo *resource_info)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o database: Specifies a resource database; returned from
%      XrmGetStringDatabase.
%
%    o client_name:  Specifies the application name used to retrieve
%      resource info from the X server database.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
*/
static void GetResourceFontNames(XrmDatabase database,const char *client_name,
  XResourceInfo *resource_info)
{
  /*
    Font resources: font, fontList and font1 to font0.
  */
  resource_info->font=XGetResourceClass(database,client_name,"font",
    (char *) NULL);
  resource_info->font=XGetResourceClass(database,client_name,"fontList",
    resource_info->font);
  resource_info->font_name[0]=XGetResourceClass(database,client_name,"font1",
    (char *) "fixed");
  resource_info->font_name[1]=XGetResourceClass(database,client_name,"font2",
    (char *) "variable");
  resource_info->font_name[2]=XGetResourceClass(database,client_name,"font3",
    (char *) "5x8");
  resource_info->font_name[3]=XGetResourceClass(database,client_name,"font4",
    (char *) "6x10");
  resource_info->font_name[4]=XGetResourceClass(database,client_name,"font5",
    (char *) "7x13bold");
  resource_info->font_name[5]=XGetResourceClass(database,client_name,"font6",
    (char *) "8x13bold");
  resource_info->font_name[6]=XGetResourceClass(database,client_name,"font7",
    (char *) "9x15bold");
  resource_info->font_name[7]=XGetResourceClass(database,client_name,"font8",
    (char *) "10x20");
  resource_info->font_name[8]=XGetResourceClass(database,client_name,"font9",
    (char *) "12x24");
  resource_info->font_name[9]=XGetResourceClass(database,client_name,"font0",
    (char *) "fixed");
  resource_info->font_name[10]=XGetResourceClass(database,client_name,"font0",
    (char *) "fixed");
}

static void GetResourcePenColors(XrmDatabase database,const char *client_name,
  XResourceInfo *resource_info)
{
  /*
    Pen colour resources: pen1 to pen0.
  */
  resource_info->pen_colors[0]=XGetResourceClass(database,client_name,"pen1",
    (char *) "black");
  resource_info->pen_colors[1]=XGetResourceClass(database,client_name,"pen2",
    (char *) "blue");
  resource_info->pen_colors[2]=XGetResourceClass(database,client_name,"pen3",
    (char *) "cyan");
  resource_info->pen_colors[3]=XGetResourceClass(database,client_name,"pen4",
    (char *) "green");
  resource_info->pen_colors[4]=XGetResourceClass(database,client_name,"pen5",
    (char *) "gray");
  resource_info->pen_colors[5]=XGetResourceClass(database,client_name,"pen6",
    (char *) "red");
  resource_info->pen_colors[6]=XGetResourceClass(database,client_name,"pen7",
    (char *) "magenta");
  resource_info->pen_colors[7]=XGetResourceClass(database,client_name,"pen8",
    (char *) "yellow");
  resource_info->pen_colors[8]=XGetResourceClass(database,client_name,"pen9",
    (char *) "white");
  resource_info->pen_colors[9]=XGetResourceClass(database,client_name,"pen0",
    (char *) "gray");
  resource_info->pen_colors[10]=XGetResourceClass(database,client_name,"pen0",
    (char *) "gray");
}

static void GetResourceDisplayOptions(XrmDatabase database,
  const char *client_name,XResourceInfo *resource_info)
{
  char
    *resource_value;

  resource_value=XGetResourceClass(database,client_name,"backdrop",
    (char *) "False");
  resource_info->backdrop=IsStringTrue(resource_value);
  resource_info->background_color=XGetResourceInstance(database,client_name,
    "background",(char *) "#d6d6d6d6d6d6");
  resource_info->border_color=XGetResourceInstance(database,client_name,
    "borderColor",BorderColor);
  resource_value=XGetResourceClass(database,client_name,"borderWidth",
    (char *) "2");
  resource_info->border_width=(unsigned int) StringToUnsignedLong(
    resource_value);
  resource_value=XGetResourceClass(database,client_name,"colormap",
    (char *) "shared");
  resource_info->colormap=UndefinedColormap;
  if (LocaleCompare("private",resource_value) == 0)
    resource_info->colormap=PrivateColormap;
  if (LocaleCompare("shared",resource_value) == 0)
    resource_info->colormap=SharedColormap;
  if (resource_info->colormap == UndefinedColormap)
    ThrowXWindowException(OptionError,"UnrecognizedColormapType",
      resource_value);
  resource_value=XGetResourceClass(database,client_name,
    "colorRecovery",(char *) "False");
  resource_info->color_recovery=IsStringTrue(resource_value);
  resource_value=XGetResourceClass(database,client_name,"confirmExit",
    (char *) "False");
  resource_info->confirm_exit=IsStringTrue(resource_value);
  resource_value=XGetResourceClass(database,client_name,"confirmEdit",
    (char *) "False");
  resource_info->confirm_edit=IsStringTrue(resource_value);
  resource_value=XGetResourceClass(database,client_name,"delay",(char *) "1");
  resource_info->delay=(unsigned int) StringToUnsignedLong(resource_value);
  resource_info->display_gamma=XGetResourceClass(database,client_name,
    "displayGamma",(char *) "2.2");
  resource_value=XGetResourceClass(database,client_name,"displayWarnings",
    (char *) "True");
  resource_info->display_warnings=IsStringTrue(resource_value);
}

static void GetResourceLayoutInfo(XrmDatabase database,
  const char *client_name,XResourceInfo *resource_info)
{
  char
    *directory,
    *resource_value;

  resource_info->foreground_color=XGetResourceInstance(database,client_name,
    "foreground",ForegroundColor);
  resource_value=XGetResourceClass(database,client_name,"gammaCorrect",
    (char *) "False");
  resource_info->gamma_correct=IsStringTrue(resource_value);
  resource_info->image_geometry=ConstantString(XGetResourceClass(database,
    client_name,"geometry",(char *) NULL));
  resource_value=XGetResourceClass(database,client_name,"gravity",
    (char *) "Center");
  resource_info->gravity=(int) ParseCommandOption(MagickGravityOptions,
    MagickFalse,resource_value);
  directory=getcwd(resource_info->home_directory,MagickPathExtent);
  (void) directory;
  resource_info->icon_geometry=XGetResourceClass(database,client_name,
    "iconGeometry",(char *) NULL);
  resource_value=XGetResourceClass(database,client_name,"iconic",
    (char *) "False");
  resource_info->iconic=IsStringTrue(resource_value);
  resource_value=XGetResourceClass(database,client_name,"immutable",
    LocaleCompare(client_name,"PerlMagick") == 0 ? (char *) "True" :
    (char *) "False");
  resource_info->immutable=IsStringTrue(resource_value);
  resource_value=XGetResourceClass(database,client_name,"magnify",
    (char *) "3");
  resource_info->magnify=(unsigned int) StringToUnsignedLong(resource_value);
  resource_info->map_type=XGetResourceClass(database,client_name,"map",
    (char *) NULL);
  resource_info->matte_color=XGetResourceInstance(database,client_name,
    "mattecolor",(char *) NULL);
  resource_info->name=ConstantString(XGetResourceClass(database,client_name,
    "name",(char *) NULL));
}

MagickExport void XGetResourceInfo(const ImageInfo *image_info,
  XrmDatabase database,const char *client_name,XResourceInfo *resource_info)
{
  char
    *resource_value;

  /*
    Initialize resource info fields.
  */
  assert(resource_info != (XResourceInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  (void) memset(resource_info,0,sizeof(*resource_info));
  resource_info->resource_database=database;
  resource_info->image_info=(ImageInfo *) image_info;
  (void) SetImageInfoProgressMonitor(resource_info->image_info,
    XMagickProgressMonitor,(void *) NULL);
  resource_info->quantize_info=CloneQuantizeInfo((QuantizeInfo *) NULL);
  resource_info->close_server=MagickTrue;
  resource_info->client_name=AcquireString(client_name);
  GetResourceDisplayOptions(database,client_name,resource_info);
  GetResourceFontNames(database,client_name,resource_info);
  GetResourceLayoutInfo(database,client_name,resource_info);
  GetResourcePenColors(database,client_name,resource_info);
  resource_value=XGetResourceClass(database,client_name,"pause",(char *) "0");
  resource_info->pause=(unsigned int) StringToUnsignedLong(resource_value);
  resource_value=XGetResourceClass(database,client_name,"quantum",(char *) "1");
  resource_info->quantum=StringToLong(resource_value);
  resource_info->text_font=XGetResourceClass(database,client_name,(char *)
    "font",(char *) "fixed");
  resource_info->text_font=XGetResourceClass(database,client_name,
    "textFontList",resource_info->text_font);
  resource_info->title=XGetResourceClass(database,client_name,"title",
    (char *) NULL);
  resource_value=XGetResourceClass(database,client_name,"undoCache",
    (char *) "4096");
  resource_info->undo_cache=(unsigned int) StringToUnsignedLong(resource_value);
  resource_value=XGetResourceClass(database,client_name,"update",
    (char *) "False");
  resource_info->update=IsStringTrue(resource_value);
  resource_value=XGetResourceClass(database,client_name,"usePixmap",
    (char *) "True");
  resource_info->use_pixmap=IsStringTrue(resource_value);
  resource_value=XGetResourceClass(database,client_name,"sharedMemory",
    (char *) "True");
  resource_info->use_shared_memory=IsStringTrue(resource_value);
  resource_info->visual_type=XGetResourceClass(database,client_name,"visual",
    (char *) NULL);
  resource_info->window_group=XGetResourceClass(database,client_name,
    "windowGroup",(char *) NULL);
  resource_info->window_id=XGetResourceClass(database,client_name,"window",
    (char *) NULL);
  resource_info->write_filename=XGetResourceClass(database,client_name,
    "writeFilename",(char *) NULL);
  resource_info->debug=(GetLogEventMask() & X11Event) != 0 ? MagickTrue :
    MagickFalse;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t R e s o u r c e I n s t a n c e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetResourceInstance() queries the X server for the specified resource name.
%  If the resource name is not defined in the database, the supplied default
%  value is returned.
%
%  The format of the XGetResourceInstance method is:
%
%      char *XGetResourceInstance(XrmDatabase database,const char *client_name,
%        const char *keyword,const char *resource_default)
%
%  A description of each parameter follows:
%
%    o database: Specifies a resource database; returned from
%      XrmGetStringDatabase.
%
%    o client_name:  Specifies the application name used to retrieve
%      resource info from the X server database.
%
%    o keyword: Specifies the keyword of the value being retrieved.
%
%    o resource_default: Specifies the default value to return if the query
%      fails to find the specified keyword/class.
%
*/
MagickExport char *XGetResourceInstance(XrmDatabase database,
  const char *client_name,const char *keyword,const char *resource_default)
{
  char
    *resource_type,
    resource_name[MagickPathExtent];

  Status
    status;

  XrmValue
    resource_value;

  if (database == (XrmDatabase) NULL)
    return((char *) resource_default);
  *resource_name='\0';
  if (keyword != (char *) NULL)
    (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.%s",client_name,
      keyword);
  status=XrmGetResource(database,resource_name,"ImageMagick",&resource_type,
    &resource_value);
  if (status == False)
    return((char *) resource_default);
  return(resource_value.addr);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t S c r e e n D e n s i t y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetScreenDensity() returns the density of the X server screen in
%  dots-per-inch.
%
%  The format of the XGetScreenDensity method is:
%
%      char *XGetScreenDensity(Display *display)
%
%  A description of each parameter follows:
%
%    o density: XGetScreenDensity() returns the density of the X screen in
%      dots-per-inch.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
*/
MagickExport char *XGetScreenDensity(Display *display)
{
  char
    density[MagickPathExtent];

  double
    x_density,
    y_density;

  /*
    Set density as determined by screen size.
  */
  x_density=((((double) DisplayWidth(display,XDefaultScreen(display)))*25.4)/
    ((double) DisplayWidthMM(display,XDefaultScreen(display))));
  y_density=((((double) DisplayHeight(display,XDefaultScreen(display)))*25.4)/
    ((double) DisplayHeightMM(display,XDefaultScreen(display))));
  (void) FormatLocaleString(density,MagickPathExtent,"%gx%g",x_density,
    y_density);
  return(GetPageGeometry(density));
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X H i g h l i g h t E l l i p s e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XHighlightEllipse() puts a border on the X server around a region defined by
%  highlight_info.
%
%  The format of the XHighlightEllipse method is:
%
%      void XHighlightEllipse(Display *display,Window window,
%        GC annotate_context,const RectangleInfo *highlight_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a Window structure.
%
%    o annotate_context: Specifies a pointer to a GC structure.
%
%    o highlight_info: Specifies a pointer to a RectangleInfo structure.  It
%      contains the extents of any highlighting rectangle.
%
*/
MagickPrivate void XHighlightEllipse(Display *display,Window window,
  GC annotate_context,const RectangleInfo *highlight_info)
{
  assert(display != (Display *) NULL);
  assert(window != (Window) NULL);
  assert(annotate_context != (GC) NULL);
  assert(highlight_info != (RectangleInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if ((highlight_info->width < 4) || (highlight_info->height < 4))
    return;
  (void) XDrawArc(display,window,annotate_context,(int) highlight_info->x,
    (int) highlight_info->y,(unsigned int) highlight_info->width-1,
    (unsigned int) highlight_info->height-1,0,360*64);
  (void) XDrawArc(display,window,annotate_context,(int) highlight_info->x+1,
    (int) highlight_info->y+1,(unsigned int) highlight_info->width-3,
    (unsigned int) highlight_info->height-3,0,360*64);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X H i g h l i g h t L i n e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XHighlightLine() puts a border on the X server around a region defined by
%  highlight_info.
%
%  The format of the XHighlightLine method is:
%
%      void XHighlightLine(Display *display,Window window,GC annotate_context,
%        const XSegment *highlight_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a Window structure.
%
%    o annotate_context: Specifies a pointer to a GC structure.
%
%    o highlight_info: Specifies a pointer to a RectangleInfo structure.  It
%      contains the extents of any highlighting rectangle.
%
*/
MagickPrivate void XHighlightLine(Display *display,Window window,
  GC annotate_context,const XSegment *highlight_info)
{
  assert(display != (Display *) NULL);
  assert(window != (Window) NULL);
  assert(annotate_context != (GC) NULL);
  assert(highlight_info != (XSegment *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  (void) XDrawLine(display,window,annotate_context,highlight_info->x1,
    highlight_info->y1,highlight_info->x2,highlight_info->y2);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X H i g h l i g h t R e c t a n g l e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XHighlightRectangle() puts a border on the X server around a region defined
%  by highlight_info.
%
%  The format of the XHighlightRectangle method is:
%
%      void XHighlightRectangle(Display *display,Window window,
%        GC annotate_context,const RectangleInfo *highlight_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a Window structure.
%
%    o annotate_context: Specifies a pointer to a GC structure.
%
%    o highlight_info: Specifies a pointer to a RectangleInfo structure.  It
%      contains the extents of any highlighting rectangle.
%
*/
MagickPrivate void XHighlightRectangle(Display *display,Window window,
  GC annotate_context,const RectangleInfo *highlight_info)
{
  assert(display != (Display *) NULL);
  assert(window != (Window) NULL);
  assert(annotate_context != (GC) NULL);
  assert(highlight_info != (RectangleInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if ((highlight_info->width < 4) || (highlight_info->height < 4))
    return;
  (void) XDrawRectangle(display,window,annotate_context,(int) highlight_info->x,
    (int) highlight_info->y,(unsigned int) highlight_info->width-1,
    (unsigned int) highlight_info->height-1);
  (void) XDrawRectangle(display,window,annotate_context,(int) highlight_info->x+
    1,(int) highlight_info->y+1,(unsigned int) highlight_info->width-3,
    (unsigned int) highlight_info->height-3);
}




/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X M a g i c k P r o g r e s s M o n i t o r                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagickProgressMonitor() displays the progress a task is making in
%  completing a task.
%
%  The format of the XMagickProgressMonitor method is:
%
%      void XMagickProgressMonitor(const char *task,
%        const MagickOffsetType quantum,const MagickSizeType span,
%        void *client_data)
%
%  A description of each parameter follows:
%
%    o task: Identifies the task in progress.
%
%    o quantum: Specifies the quantum position within the span which represents
%      how much progress has been made in completing a task.
%
%    o span: Specifies the span relative to completing a task.
%
%    o client_data: Pointer to any client data.
%
*/

static const char *GetLocaleMonitorMessage(const char *text)
{
  char
    message[MagickPathExtent],
    tag[MagickPathExtent];

  const char
    *locale_message;

  char
    *p;

  (void) CopyMagickString(tag,text,MagickPathExtent);
  p=strrchr(tag,'/');
  if (p != (char *) NULL)
    *p='\0';
  (void) FormatLocaleString(message,MagickPathExtent,"Monitor/%s",tag);
  locale_message=GetLocaleMessage(message);
  if (locale_message == message)
    return(text);
  return(locale_message);
}

MagickPrivate MagickBooleanType XMagickProgressMonitor(const char *tag,
  const MagickOffsetType quantum,const MagickSizeType span,
  void *magick_unused(client_data))
{
  XWindows
    *windows;

  windows=XSetWindows((XWindows *) ~0);
  if (windows == (XWindows *) NULL)
    return(MagickTrue);
  if (windows->info.mapped != MagickFalse)
    XProgressMonitorWidget(windows->display,windows,
      GetLocaleMonitorMessage(tag),quantum,span);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X Q u e r y C o l o r D a t a b a s e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XQueryColorCompliance() looks up a RGB values for a color given in the target
%  string.
%
%  The format of the XQueryColorDatabase method is:
%
%      MagickBooleanType XQueryColorCompliance(const char *target,XColor *color)
%
%  A description of each parameter follows:
%
%    o target: Specifies the color to lookup in the X color database.
%
%    o color: A pointer to an PixelInfo structure.  The RGB value of the target
%      color is returned as this value.
%
*/
MagickPrivate MagickBooleanType XQueryColorCompliance(const char *target,
  XColor *color)
{
  Colormap
    colormap;

  static Display
    *display = (Display *) NULL;

  Status
    status;

  XColor
    xcolor;

  /*
    Initialize color return value.
  */
  assert(color != (XColor *) NULL);
  color->red=0;
  color->green=0;
  color->blue=0;
  color->flags=(char) (DoRed | DoGreen | DoBlue);
  if ((target == (char *) NULL) || (*target == '\0'))
    target="#ffffffffffff";
  /*
    Let the X server define the color for us.
  */
  if (display == (Display *) NULL)
    display=XOpenDisplay((char *) NULL);
  if (display == (Display *) NULL)
    {
      ThrowXWindowException(XServerError,"ColorIsNotKnownToServer",target);
      return(MagickFalse);
    }
  colormap=XDefaultColormap(display,XDefaultScreen(display));
  status=XParseColor(display,colormap,(char *) target,&xcolor);
  if (status == False)
    ThrowXWindowException(XServerError,"ColorIsNotKnownToServer",target)
  else
    {
      color->red=xcolor.red;
      color->green=xcolor.green;
      color->blue=xcolor.blue;
      color->flags=xcolor.flags;
    }
  return(status != False ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X Q u e r y P o s i t i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XQueryPosition() gets the pointer coordinates relative to a window.
%
%  The format of the XQueryPosition method is:
%
%      void XQueryPosition(Display *display,const Window window,int *x,int *y)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a Window.
%
%    o x: Return the x coordinate of the pointer relative to the origin of the
%      window.
%
%    o y: Return the y coordinate of the pointer relative to the origin of the
%      window.
%
*/
MagickPrivate void XQueryPosition(Display *display,const Window window,int *x,
  int *y)
{
  int
    x_root,
    y_root;

  unsigned int
    mask;

  Window
    root_window;

  assert(display != (Display *) NULL);
  assert(window != (Window) NULL);
  assert(x != (int *) NULL);
  assert(y != (int *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  (void) XQueryPointer(display,window,&root_window,&root_window,&x_root,&y_root,
    x,y,&mask);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X R e m o t e C o m m a n d                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XRemoteCommand() forces a remote display(1) to display the specified
%  image filename.
%
%  The format of the XRemoteCommand method is:
%
%      MagickBooleanType XRemoteCommand(Display *display,const char *window,
%        const char *filename)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o window: Specifies the name or id of an X window.
%
%    o filename: the name of the image filename to display.
%
*/
static Window XRemoteWindowByName(Display *display,const Window root_window,
  const char *window,Window remote_window)
{
  if (window != (char *) NULL)
    {
      /*
        Search window hierarchy and identify any clients by name or ID.
      */
      if (isdigit((int) ((unsigned char) *window)) != 0)
        remote_window=XWindowByID(display,root_window,(Window)
          strtol((char *) window,(char **) NULL,0));
      if (remote_window == (Window) NULL)
        remote_window=XWindowByName(display,root_window,window);
    }
  return(remote_window);
}

MagickExport MagickBooleanType XRemoteCommand(Display *display,
  const char *window,const char *filename)
{
  Atom
    remote_atom;

  Window
    remote_window,
    root_window;

  assert(filename != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  if (display == (Display *) NULL)
    display=XOpenDisplay((char *) NULL);
  if (display == (Display *) NULL)
    {
      ThrowXWindowException(XServerError,"UnableToOpenXServer",filename);
      return(MagickFalse);
    }
  remote_atom=XInternAtom(display,"IM_PROTOCOLS",MagickFalse);
  remote_window=(Window) NULL;
  root_window=XRootWindow(display,XDefaultScreen(display));
  remote_window=XRemoteWindowByName(display,root_window,window,remote_window);
  if (remote_window == (Window) NULL)
    remote_window=XWindowByProperty(display,root_window,remote_atom);
  if (remote_window == (Window) NULL)
    {
      ThrowXWindowException(XServerError,"UnableToConnectToRemoteDisplay",
        filename);
      return(MagickFalse);
    }
  /*
    Send remote command.
  */
  remote_atom=XInternAtom(display,"IM_REMOTE_COMMAND",MagickFalse);
  (void) XChangeProperty(display,remote_window,remote_atom,XA_STRING,8,
    PropModeReplace,(unsigned char *) filename,(int) strlen(filename));
  (void) XSync(display,MagickFalse);
  return(MagickTrue);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X R e t a i n W i n d o w C o l o r s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XRetainWindowColors() sets X11 color resources on a window.  This preserves
%  the colors associated with an image displayed on the window.
%
%  The format of the XRetainWindowColors method is:
%
%      void XRetainWindowColors(Display *display,const Window window)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindowInfo structure.
%
*/
MagickExport void XRetainWindowColors(Display *display,const Window window)
{
  Atom
    property;

  Pixmap
    pixmap;

  /*
    Put property on the window.
  */
  assert(display != (Display *) NULL);
  assert(window != (Window) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  property=XInternAtom(display,"_XSETROOT_ID",MagickFalse);
  if (property == (Atom) NULL)
    {
      ThrowXWindowException(XServerError,"UnableToCreateProperty",
        "_XSETROOT_ID");
      return;
    }
  pixmap=XCreatePixmap(display,window,1,1,1);
  if (pixmap == (Pixmap) NULL)
    {
      ThrowXWindowException(XServerError,"UnableToCreateBitmap","");
      return;
    }
  (void) XChangeProperty(display,window,property,XA_PIXMAP,32,PropModeReplace,
    (unsigned char *) &pixmap,1);
  (void) XSetCloseDownMode(display,RetainPermanent);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X S e t W i n d o w s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSetWindows() sets the X windows structure if the windows info is specified.
%  Otherwise the current windows structure is returned.
%
%  The format of the XSetWindows method is:
%
%      XWindows *XSetWindows(XWindows *windows_info)
%
%  A description of each parameter follows:
%
%    o windows_info: Initialize the Windows structure with this information.
%
*/
MagickPrivate XWindows *XSetWindows(XWindows *windows_info)
{
  static XWindows
    *windows = (XWindows *) NULL;

  if (windows_info != (XWindows *) ~0)
    {
      windows=(XWindows *) RelinquishMagickMemory(windows);
      windows=windows_info;
    }
  return(windows);
}
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X U s e r P r e f e r e n c e s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XUserPreferences() saves the preferences in a configuration file in the
%  users' home directory.
%
%  The format of the XUserPreferences method is:
%
%      void XUserPreferences(XResourceInfo *resource_info)
%
%  A description of each parameter follows:
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
*/
#if defined(X11_PREFERENCES_PATH)
static void PutXBooleanPreference(XrmDatabase *preferences_database,
  const char *specifier,const MagickBooleanType enabled)
{
  const char
    *value;

  value=enabled ? "True" : "False";
  XrmPutStringResource(preferences_database,specifier,(char *) value);
}
#endif

MagickPrivate void XUserPreferences(XResourceInfo *resource_info)
{
#if defined(X11_PREFERENCES_PATH)
  char
    cache[MagickPathExtent],
    filename[MagickPathExtent],
    specifier[MagickPathExtent];

  const char
    *client_name,
    *value;

  XrmDatabase
    preferences_database;

  /*
    Save user preferences to the client configuration file.
  */
  assert(resource_info != (XResourceInfo *) NULL);
  client_name=GetClientName();
  preferences_database=XrmGetStringDatabase("");
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.backdrop",client_name);
  PutXBooleanPreference(&preferences_database,specifier,
    resource_info->backdrop);
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.colormap",client_name);
  value=resource_info->colormap == SharedColormap ? "Shared" : "Private";
  XrmPutStringResource(&preferences_database,specifier,(char *) value);
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.confirmExit",
    client_name);
  PutXBooleanPreference(&preferences_database,specifier,
    resource_info->confirm_exit);
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.confirmEdit",
    client_name);
  PutXBooleanPreference(&preferences_database,specifier,
    resource_info->confirm_edit);
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.displayWarnings",
    client_name);
  PutXBooleanPreference(&preferences_database,specifier,
    resource_info->display_warnings);
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.dither",client_name);
  value=resource_info->quantize_info->dither_method != NoDitherMethod ?
    "True" : "False";
  XrmPutStringResource(&preferences_database,specifier,(char *) value);
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.gammaCorrect",
    client_name);
  PutXBooleanPreference(&preferences_database,specifier,
    resource_info->gamma_correct);
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.undoCache",client_name);
  (void) FormatLocaleString(cache,MagickPathExtent,"%.17g",(double)
    resource_info->undo_cache);
  XrmPutStringResource(&preferences_database,specifier,cache);
  (void) FormatLocaleString(specifier,MagickPathExtent,"%s.usePixmap",client_name);
  PutXBooleanPreference(&preferences_database,specifier,
    resource_info->use_pixmap);
  (void) FormatLocaleString(filename,MagickPathExtent,"%s%src",
    X11_PREFERENCES_PATH,client_name);
  ExpandFilename(filename);
  XrmPutFileDatabase(preferences_database,filename);
#endif
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X W a r n i n g                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XWarning() displays a warning reason in a Notice widget.
%
%  The format of the XWarning method is:
%
%      void XWarning(const unsigned int warning,const char *reason,
%        const char *description)
%
%  A description of each parameter follows:
%
%    o warning: Specifies the numeric warning category.
%
%    o reason: Specifies the reason to display before terminating the
%      program.
%
%    o description: Specifies any description to the reason.
%
*/
MagickPrivate void XWarning(const ExceptionType magick_unused(warning),
  const char *reason,const char *description)
{
  char
    text[MagickPathExtent];

  XWindows
    *windows;

  if (reason == (char *) NULL)
    return;
  (void) CopyMagickString(text,reason,MagickPathExtent);
  (void) ConcatenateMagickString(text,":",MagickPathExtent);
  windows=XSetWindows((XWindows *) ~0);
  if (windows != (XWindows *) NULL)
     XNoticeWidget(windows->display,windows,text,(char *) description);
}

#else

#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C o m p o n e n t G e n e s i s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XComponentGenesis() instantiates the X component.
%
%  The format of the XComponentGenesis method is:
%
%      MagickBooleanType XComponentGenesis(void)
%
*/
MagickPrivate MagickBooleanType XComponentGenesis(void)
{
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t I m p o r t I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetImportInfo() initializes the XImportInfo structure.
%
%  The format of the XGetImportInfo method is:
%
%      void XGetImportInfo(XImportInfo *ximage_info)
%
%  A description of each parameter follows:
%
%    o ximage_info: Specifies a pointer to an ImageInfo structure.
%
*/
MagickExport void XGetImportInfo(XImportInfo *ximage_info)
{
  assert(ximage_info != (XImportInfo *) NULL);
  ximage_info->frame=MagickFalse;
  ximage_info->borders=MagickFalse;
  ximage_info->screen=MagickFalse;
  ximage_info->descend=MagickTrue;
  ximage_info->silent=MagickFalse;
}
