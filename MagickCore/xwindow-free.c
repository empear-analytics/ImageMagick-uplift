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
%                         FFFFF  RRRR   EEEEE  EEEEE                          %
%                         F      R   R  E      E                              %
%                         FFF    RRRR   EEE    EEE                            %
%                         F      R R    E      E                              %
%                         F      R  R   EEEEE  EEEEE                          %
%                                                                             %
%                                                                             %
%                   MagickCore X11 Resource Release Methods                   %
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
#endif
