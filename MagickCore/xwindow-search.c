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
%                  SSSSS  EEEEE   AAA   RRRR    CCCC  H   H                   %
%                  SS     E      A   A  R   R  C      H   H                   %
%                   SSS   EEE    AAAAA  RRRR   C      HHHHH                   %
%                     SS  E      A   A  R R    C      H   H                   %
%                  SSSSS  EEEEE  A   A  R  R    CCCC  H   H                   %
%                                                                             %
%                                                                             %
%                    MagickCore X11 Window Search Methods                     %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X W i n d o w B y N a m e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XWindowByName() locates a window with a given name on a display.  If no
%  window with the given name is found, 0 is returned. If more than one window
%  has the given name, the first one is returned.  Only root and its children
%  are searched.
%
%  The format of the XWindowByName function is:
%
%      window=XWindowByName(display,root_window,name)
%
%  A description of each parameter follows:
%
%    o window: XWindowByName returns the window id.
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o root_window: Specifies the id of the root window.
%
%    o name: Specifies the name of the window to locate.
%
*/
static Window XSearchChildrenByName(Display *display,Window *children,
  const unsigned int number_children,const char *name)
{
  int
    i;

  Window
    window;

  window=(Window) NULL;
  for (i=0; i < (int) number_children; i++)
  {
    /*
      Search each child and their children.
    */
    window=XWindowByName(display,children[i],name);
    if (window != (Window) NULL)
      break;
  }
  if (children != (Window *) NULL)
    (void) XFree((void *) children);
  return(window);
}

MagickPrivate Window XWindowByName(Display *display,const Window root_window,
  const char *name)
{
  Status
    status;

  unsigned int
    number_children;

  Window
    *children,
    child;

  XTextProperty
    window_name;

  assert(display != (Display *) NULL);
  assert(root_window != (Window) NULL);
  assert(name != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",name);
  if ((XGetWMName(display,root_window,&window_name) != 0) &&
      (LocaleCompare((char *) window_name.value,name) == 0))
    return(root_window);
  status=XQueryTree(display,root_window,&child,&child,&children,
    &number_children);
  if (status == False)
    return((Window) NULL);
  return(XSearchChildrenByName(display,children,number_children,name));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X W i n d o w B y P r o p e r y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XWindowByProperty() locates a child window with a given property. If not
%  window with the given name is found, 0 is returned.  If more than one window
%  has the given property, the first one is returned.  Only the window
%  specified and its subwindows are searched.
%
%  The format of the XWindowByProperty function is:
%
%      child=XWindowByProperty(display,window,property)
%
%  A description of each parameter follows:
%
%    o child: XWindowByProperty returns the window id with the specified
%      property.  If no windows are found, XWindowByProperty returns 0.
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o property: Specifies the property of the window to locate.
%
*/
static Window XFindPropertyChild(Display *display,Window *children,
  const unsigned int number_children,const Atom property)
{
  Atom
    type;

  int
    format;

  Status
    status;

  unsigned char
    *data;

  unsigned int
    i;

  unsigned long
    after,
    number_items;

  Window
    child;

  type=(Atom) NULL;
  child=(Window) NULL;
  for (i=0; (i < number_children) && (child == (Window) NULL); i++)
  {
    status=XGetWindowProperty(display,children[i],property,0L,0L,MagickFalse,
      (Atom) AnyPropertyType,&type,&format,&number_items,&after,&data);
    if (data != NULL)
      (void) XFree((void *) data);
    if ((status == Success) && (type != (Atom) NULL))
      child=children[i];
  }
  return(child);
}

MagickPrivate Window XWindowByProperty(Display *display,const Window window,
  const Atom property)
{
  Status
    status;

  unsigned int
    i,
    number_children;

  Window
    child,
    *children,
    parent,
    root;

  assert(display != (Display *) NULL);
  assert(window != (Window) NULL);
  assert(property != (Atom) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  status=XQueryTree(display,window,&root,&parent,&children,&number_children);
  if (status == False)
    return((Window) NULL);
  child=XFindPropertyChild(display,children,number_children,property);
  for (i=0; (i < number_children) && (child == (Window) NULL); i++)
    child=XWindowByProperty(display,children[i],property);
  if (children != (Window *) NULL)
    (void) XFree((void *) children);
  return(child);
}
#endif
