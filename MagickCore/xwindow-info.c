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
%                         IIIII  N   N  FFFFF   OOO                           %
%                           I    NN  N  F      O   O                          %
%                           I    N N N  FFF    O   O                          %
%                           I    N  NN  F      O   O                          %
%                         IIIII  N   N  F       OOO                           %
%                                                                             %
%                                                                             %
%                  MagickCore X11 Window Information Methods                  %
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
#include "MagickCore/xwindow-image-private.h"
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X F r e e R e s o u r c e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XFreeResources() frees X11 resources.
%
%  The format of the XFreeResources method is:
%
%      void XFreeResources(Display *display,XVisualInfo *visual_info,
%        XStandardColormap *map_info,XPixelInfo *pixel,XFontStruct *font_info,
%        XResourceInfo *resource_info,XWindowInfo *window_info)
%        resource_info,window_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o visual_info: Specifies a pointer to a X11 XVisualInfo structure;
%      returned from XGetVisualInfo.
%
%    o map_info: If map_type is specified, this structure is initialized
%      with info from the Standard Colormap.
%
%    o pixel: Specifies a pointer to a XPixelInfo structure.
%
%    o font_info: Specifies a pointer to a XFontStruct structure.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
*/
static void XFreeWindowContexts(Display *display,const XVisualInfo *visual_info,
  XWindowInfo *window_info)
{
  if (window_info->id != (Window) NULL)
    {
      /*
        Free destroy window and free cursors.
      */
      if (window_info->id != XRootWindow(display,visual_info->screen))
        (void) XDestroyWindow(display,window_info->id);
      if (window_info->annotate_context != (GC) NULL)
        (void) XFreeGC(display,window_info->annotate_context);
      if (window_info->highlight_context != (GC) NULL)
        (void) XFreeGC(display,window_info->highlight_context);
      if (window_info->widget_context != (GC) NULL)
        (void) XFreeGC(display,window_info->widget_context);
      if (window_info->cursor != (Cursor) NULL)
        (void) XFreeCursor(display,window_info->cursor);
      window_info->cursor=(Cursor) NULL;
      if (window_info->busy_cursor != (Cursor) NULL)
        (void) XFreeCursor(display,window_info->busy_cursor);
      window_info->busy_cursor=(Cursor) NULL;
    }
}

static void XFreeWindowInfo(Display *display,const XVisualInfo *visual_info,
  XWindowInfo *window_info)
{
  if (window_info != (XWindowInfo *) NULL)
    {
      /*
        Free X image.
      */
      if (window_info->ximage != (XImage *) NULL)
        XDestroyImage(window_info->ximage);
      XFreeWindowContexts(display,visual_info,window_info);
    }
}

MagickPrivate void XFreeResources(Display *display,XVisualInfo *visual_info,
  XStandardColormap *map_info,XPixelInfo *pixel,XFontStruct *font_info,
  XResourceInfo *resource_info,XWindowInfo *window_info)
{
  assert(display != (Display *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  XFreeWindowInfo(display,visual_info,window_info);
  /*
    Free font.
  */
  if (font_info != (XFontStruct *) NULL)
    {
      (void) XFreeFont(display,font_info);
      font_info=(XFontStruct *) NULL;
    }
  if (map_info != (XStandardColormap *) NULL)
    {
      /*
        Free X Standard Colormap.
      */
      if (resource_info->map_type == (char *) NULL)
        (void) XFreeStandardColormap(display,visual_info,map_info,pixel);
      (void) XFree((void *) map_info);
    }
  /*
    Free X visual info.
  */
  if (visual_info != (XVisualInfo *) NULL)
    (void) XFree((void *) visual_info);
  if (resource_info->close_server != MagickFalse)
    (void) XCloseDisplay(display);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t W i n d o w I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetWindowInfo() initializes the XWindowInfo structure.
%
%  The format of the XGetWindowInfo method is:
%
%      void XGetWindowInfo(Display *display,XVisualInfo *visual_info,
%        XStandardColormap *map_info,XPixelInfo *pixel,XFontStruct *font_info,
%        XResourceInfo *resource_info,XWindowInfo *window)
%        resource_info,window)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o visual_info: Specifies a pointer to a X11 XVisualInfo structure;
%      returned from XGetVisualInfo.
%
%    o map_info: If map_type is specified, this structure is initialized
%      with info from the Standard Colormap.
%
%    o pixel: Specifies a pointer to a XPixelInfo structure.
%
%    o font_info: Specifies a pointer to a XFontStruct structure.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
*/
static void XInitializeWindowGeometry(Display *display,const XPixelInfo *pixel,
  const XResourceInfo *resource_info,XWindowInfo *window)
{
  window->cursor=XCreateFontCursor(display,XC_left_ptr);
  window->busy_cursor=XCreateFontCursor(display,XC_watch);
  window->geometry=(char *) NULL;
  window->icon_geometry=(char *) NULL;
  if (resource_info->icon_geometry != (char *) NULL)
    (void) CloneString(&window->icon_geometry,resource_info->icon_geometry);
  window->crop_geometry=(char *) NULL;
  window->flags=(size_t) PSize;
  window->width=1;
  window->height=1;
  window->min_width=1;
  window->min_height=1;
  window->width_inc=1;
  window->height_inc=1;
  window->border_width=resource_info->border_width;
  window->annotate_context=pixel->annotate_context;
  window->highlight_context=pixel->highlight_context;
  window->widget_context=pixel->widget_context;
  window->shadow_stipple=(Pixmap) NULL;
  window->highlight_stipple=(Pixmap) NULL;
  window->use_pixmap=MagickTrue;
  window->immutable=MagickFalse;
  window->shape=MagickFalse;
  window->data=0;
}

static void XInitializeWindowOnce(Display *display,
  const XVisualInfo *visual_info,XWindowInfo *window)
{
  window->id=(Window) NULL;
  if (window->name == (char *) NULL)
    window->name=AcquireString("");
  if (window->icon_name == (char *) NULL)
    window->icon_name=AcquireString("");
  window->x=XDisplayWidth(display,visual_info->screen) >> 1;
  window->y=XDisplayWidth(display,visual_info->screen) >> 1;
  window->ximage=(XImage *) NULL;
  window->matte_image=(XImage *) NULL;
  window->pixmap=(Pixmap) NULL;
  window->matte_pixmap=(Pixmap) NULL;
  window->mapped=MagickFalse;
  window->stasis=MagickFalse;
  window->shared_memory=MagickTrue;
  window->segment_info=(void *) NULL;
}

static void XFreeWindowCursorsAndStipples(Display *display,
  const XWindowInfo *window)
{
  if (window->cursor != (Cursor) NULL)
    (void) XFreeCursor(display,window->cursor);
  if (window->busy_cursor != (Cursor) NULL)
    (void) XFreeCursor(display,window->busy_cursor);
  if (window->highlight_stipple != (Pixmap) NULL)
    (void) XFreePixmap(display,window->highlight_stipple);
  if (window->shadow_stipple != (Pixmap) NULL)
    (void) XFreePixmap(display,window->shadow_stipple);
}

static void XResetWindowInfo(Display *display,XWindowInfo *window)
{
  XFreeWindowCursorsAndStipples(display,window);
  if (window->name == (char *) NULL)
    window->name=AcquireString("");
  if (window->icon_name == (char *) NULL)
    window->icon_name=AcquireString("");
}

static void XInitializeWindowAttributes(const XPixelInfo *pixel,
  const XStandardColormap *map_info,XWindowInfo *window)
{
  window->mask=(int) (CWBackingStore | CWBackPixel | CWBackPixmap |
    CWBitGravity | CWBorderPixel | CWColormap | CWCursor | CWDontPropagate |
    CWEventMask | CWOverrideRedirect | CWSaveUnder | CWWinGravity);
  window->attributes.background_pixel=pixel->background_color.pixel;
  window->attributes.background_pixmap=(Pixmap) NULL;
  window->attributes.bit_gravity=ForgetGravity;
  window->attributes.backing_store=WhenMapped;
  window->attributes.save_under=MagickTrue;
  window->attributes.border_pixel=pixel->border_color.pixel;
  window->attributes.colormap=map_info->colormap;
  window->attributes.cursor=window->cursor;
  window->attributes.do_not_propagate_mask=NoEventMask;
  window->attributes.event_mask=NoEventMask;
  window->attributes.override_redirect=MagickFalse;
  window->attributes.win_gravity=NorthWestGravity;
  window->orphan=MagickFalse;
}

MagickPrivate void XGetWindowInfo(Display *display,
  const XWindowInfoSource *source,XResourceInfo *resource_info,
  XWindowInfo *window)
{
  XVisualInfo
    *const visual_info = source->visual_info;

  XStandardColormap
    *const map_info = source->map_info;

  XPixelInfo
    *const pixel = source->pixel;

  XFontStruct
    *const font_info = source->font_info;

  /*
    Initialize window info.
  */
  assert(display != (Display *) NULL);
  assert(visual_info != (XVisualInfo *) NULL);
  assert(map_info != (XStandardColormap *) NULL);
  assert(pixel != (XPixelInfo *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  assert(window != (XWindowInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (window->id != (Window) NULL)
    XResetWindowInfo(display,window);
  else
    {
      /*
        Initialize these attributes just once.
      */
      XInitializeWindowOnce(display,visual_info,window);
#if defined(MAGICKCORE_HAVE_SHARED_MEMORY)
      {
        XShmSegmentInfo
          *segment_info;

        if (window->segment_info == (void *) NULL)
          window->segment_info=AcquireCriticalMemory(2*sizeof(*segment_info));
        segment_info=(XShmSegmentInfo *) window->segment_info;
        segment_info[0].shmid=(-1);
        segment_info[0].shmaddr=(char *) NULL;
        segment_info[1].shmid=(-1);
        segment_info[1].shmaddr=(char *) NULL;
      }
#endif
    }
  /*
    Initialize these attributes every time function is called.
  */
  window->screen=visual_info->screen;
  window->root=XRootWindow(display,visual_info->screen);
  window->visual=visual_info->visual;
  window->storage_class=(unsigned int) visual_info->klass;
  window->depth=(unsigned int) visual_info->depth;
  window->visual_info=visual_info;
  window->map_info=map_info;
  window->pixel_info=pixel;
  window->font_info=font_info;
  XInitializeWindowGeometry(display,pixel,resource_info,window);
  XInitializeWindowAttributes(pixel,map_info,window);
}
#endif
