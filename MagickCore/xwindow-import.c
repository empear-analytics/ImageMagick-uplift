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
%                                                                             %
%                     MagickCore X Window Import Methods                      %
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
  Typedef declarations.
*/
typedef struct _WindowImageContext
{
  Display
    *display;

  ExceptionInfo
    *exception;
} WindowImageContext;

/*
  Forward declarations.
*/
static Window
  XSelectWindow(Display *,RectangleInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C l i e n t W i n d o w                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XClientWindow() finds a window, at or below the specified window, which has
%  a WM_STATE property.  If such a window is found, it is returned, otherwise
%  the argument window is returned.
%
%  The format of the XClientWindow function is:
%
%      client_window=XClientWindow(display,target_window)
%
%  A description of each parameter follows:
%
%    o client_window: XClientWindow returns a window, at or below the specified
%      window, which has a WM_STATE property otherwise the argument
%      target_window is returned.
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o target_window: Specifies the window to find a WM_STATE property.
%
*/
static Window XClientWindow(Display *display,Window target_window)
{
  Atom
    state,
    type;

  int
    format;

  Status
    status;

  unsigned char
    *data;

  unsigned long
    after,
    number_items;

  Window
    client_window;

  assert(display != (Display *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  state=XInternAtom(display,"WM_STATE",MagickTrue);
  if (state == (Atom) NULL)
    return(target_window);
  type=(Atom) NULL;
  status=XGetWindowProperty(display,target_window,state,0L,0L,MagickFalse,
    (Atom) AnyPropertyType,&type,&format,&number_items,&after,&data);
  if ((status == Success) && (type != (Atom) NULL))
    return(target_window);
  client_window=XWindowByProperty(display,target_window,state);
  if (client_window == (Window) NULL)
    return(target_window);
  return(client_window);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X G e t S u b w i n d o w                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetSubwindow() returns the subwindow of a window chosen the user with the
%  pointer and a button press.
%
%  The format of the XGetSubwindow method is:
%
%      Window XGetSubwindow(Display *display,Window window,int x,int y)
%
%  A description of each parameter follows:
%
%    o subwindow: XGetSubwindow() returns NULL if no subwindow is found
%      otherwise the subwindow is returned.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a Window.
%
%    o x: the x coordinate of the pointer relative to the origin of the
%      window.
%
%    o y: the y coordinate of the pointer relative to the origin of the
%      window.
%
*/
static Window XGetSubwindow(Display *display,Window window,int x,int y)
{
  int
    x_offset,
    y_offset;

  Status
    status;

  Window
    source_window,
    target_window;

  assert(display != (Display *) NULL);
  source_window=XRootWindow(display,XDefaultScreen(display));
  if (window == (Window) NULL)
    return(source_window);
  target_window=window;
  for ( ; ; )
  {
    status=XTranslateCoordinates(display,source_window,window,x,y,
      &x_offset,&y_offset,&target_window);
    if (status != True)
      break;
    if (target_window == (Window) NULL)
      break;
    source_window=window;
    window=target_window;
    x=x_offset;
    y=y_offset;
  }
  if (target_window == (Window) NULL)
    target_window=window;
  return(target_window);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X G e t W i n d o w C o l o r                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetWindowColor() returns the color of a pixel interactively chosen from the
%  X server.
%
%  The format of the XGetWindowColor method is:
%
%      MagickBooleanType XGetWindowColor(Display *display,XWindows *windows,
%        char *name,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o name: the name of the color if found in the X Color Database is
%      returned in this character string.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickPrivate MagickBooleanType XGetWindowColor(Display *display,
  XWindows *windows,char *name,ExceptionInfo *exception)
{
  int
    x,
    y;

  PixelInfo
    pixel;

  RectangleInfo
    crop_info;

  Status
    status;

  Window
    child,
    client_window,
    root_window,
    target_window;

  XColor
    color;

  XImage
    *ximage;

  XWindowAttributes
    window_attributes;

  /*
    Choose a pixel from the X server.
  */
  assert(display != (Display *) NULL);
  assert(name != (char *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",name);
  *name='\0';
  target_window=XSelectWindow(display,&crop_info);
  if (target_window == (Window) NULL)
    return(MagickFalse);
  root_window=XRootWindow(display,XDefaultScreen(display));
  client_window=target_window;
  if (target_window != root_window)
    {
      unsigned int
        d;

      /*
        Get client window.
      */
      status=XGetGeometry(display,target_window,&root_window,&x,&x,&d,&d,&d,&d);
      if (status != False)
        {
          client_window=XClientWindow(display,target_window);
          target_window=client_window;
        }
    }
  /*
    Verify window is viewable.
  */
  status=XGetWindowAttributes(display,target_window,&window_attributes);
  if ((status == False) || (window_attributes.map_state != IsViewable))
    return(MagickFalse);
  /*
    Get window X image.
  */
  (void) XTranslateCoordinates(display,root_window,target_window,
    (int) crop_info.x,(int) crop_info.y,&x,&y,&child);
  ximage=XGetImage(display,target_window,x,y,1,1,AllPlanes,ZPixmap);
  if (ximage == (XImage *) NULL)
    return(MagickFalse);
  color.pixel=XGetPixel(ximage,0,0);
  XDestroyImage(ximage);
  /*
    Match color against the color database.
  */
  (void) XQueryColor(display,window_attributes.colormap,&color);
  pixel.red=(double) ScaleShortToQuantum(color.red);
  pixel.green=(double) ScaleShortToQuantum(color.green);
  pixel.blue=(double) ScaleShortToQuantum(color.blue);
  pixel.alpha=(MagickRealType) OpaqueAlpha;
  (void) QueryColorname(windows->image.image,&pixel,X11Compliance,name,
    exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X G e t W i n d o w I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetWindowImage() reads an image from the target X window and returns it.
%  XGetWindowImage() optionally descends the window hierarchy and overlays the
%  target image with each child image in an optimized fashion.  Any child
%  window that have the same visual, colormap, and are contained by its parent
%  are exempted.
%
%  The format of the XGetWindowImage method is:
%
%      Image *XGetWindowImage(Display *display,const Window window,
%        const unsigned int borders,const unsigned int level,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies the window to obtain the image from.
%
%    o borders: Specifies whether borders pixels are to be saved with
%      the image.
%
%    o level: Specifies an unsigned integer representing the level of
%      decent in the window hierarchy.  This value must be zero or one on
%      the initial call to XGetWindowImage.  A value of zero returns after
%      one call.  A value of one causes the function to descend the window
%      hierarchy and overlay the target image with each subwindow image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _WindowInfo
{
  Window
    window,
    parent;

  Visual
    *visual;

  Colormap
    colormap;

  XSegment
    bounds;

  RectangleInfo
    crop_info;
} WindowInfo;

static inline MagickBooleanType IsSameVisualParent(
  const WindowInfo *window_info,const int id,const int i)
{
  return(((window_info[i].window == window_info[id].parent) &&
    (window_info[i].visual == window_info[id].visual) &&
    (window_info[i].colormap == window_info[id].colormap)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsOutsideParentBounds(
  const WindowInfo *window_info,const int id,const int i)
{
  return(((window_info[id].bounds.x1 < window_info[i].bounds.x1) ||
    (window_info[id].bounds.x2 > window_info[i].bounds.x2) ||
    (window_info[id].bounds.y1 < window_info[i].bounds.y1) ||
    (window_info[id].bounds.y2 > window_info[i].bounds.y2)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsInsideWindowBounds(
  const WindowInfo *window_info,const int id,const int j)
{
  return(((window_info[id].bounds.x1 >= window_info[j].bounds.x1) &&
    (window_info[id].bounds.x2 <= window_info[j].bounds.x2) &&
    (window_info[id].bounds.y1 >= window_info[j].bounds.y1) &&
    (window_info[id].bounds.y2 <= window_info[j].bounds.y2)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IntersectsTopLevelWindow(
  const WindowInfo *window_info,const int id)
{
  return(((window_info[id].bounds.x2 >= window_info[0].bounds.x1) &&
     (window_info[id].bounds.x1 <= window_info[0].bounds.x2) &&
     (window_info[id].bounds.y2 >= window_info[0].bounds.y1) &&
     (window_info[id].bounds.y1 <= window_info[0].bounds.y2)) ?
    MagickTrue : MagickFalse);
}

static void XInitializeDirectColors(const WindowInfo *window_info,
  const int id,const unsigned int number_colors,XColor *colors)
{
  int
    i;

  size_t
    blue,
    blue_bit,
    green,
    green_bit,
    red,
    red_bit;

  /*
    DirectColor or TrueColor visual.
  */
  red=0;
  green=0;
  blue=0;
  red_bit=window_info[id].visual->red_mask &
    (~(window_info[id].visual->red_mask)+1);
  green_bit=window_info[id].visual->green_mask &
    (~(window_info[id].visual->green_mask)+1);
  blue_bit=window_info[id].visual->blue_mask &
    (~(window_info[id].visual->blue_mask)+1);
  for (i=0; i < (int) number_colors; i++)
  {
    colors[i].pixel=(unsigned long) (red | green | blue);
    colors[i].pad='\0';
    red+=red_bit;
    if (red > window_info[id].visual->red_mask)
      red=0;
    green+=green_bit;
    if (green > window_info[id].visual->green_mask)
      green=0;
    blue+=blue_bit;
    if (blue > window_info[id].visual->blue_mask)
      blue=0;
  }
}

static void XImportPseudoClassRows(Image *composite_image,
  CacheView *composite_view,XImage *ximage,ExceptionInfo *exception)
{
  int
    x,
    y;

  Status
    status;

  Quantum
    index,
    *magick_restrict q;

  for (y=0; y < (int) composite_image->rows; y++)
  {
    q=QueueCacheViewAuthenticPixels(composite_view,0,(ssize_t) y,
      composite_image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) composite_image->columns; x++)
    {
      index=(Quantum) XGetPixel(ximage,x,y);
      SetPixelIndex(composite_image,index,q);
      SetPixelViaPixelInfo(composite_image,
        composite_image->colormap+(ssize_t) index,q);
      q+=(ptrdiff_t) GetPixelChannels(composite_image);
    }
    status=SyncCacheViewAuthenticPixels(composite_view,exception)
      == MagickFalse ? 0 : 1;
    if (status == MagickFalse)
      break;
  }
}

typedef struct _XDirectRowSource
{
  Image
    *composite_image;

  CacheView
    *composite_view;

  ExceptionInfo
    *exception;

  XImage
    *ximage;

  XColor
    *colors;

  size_t
    blue_mask,
    blue_shift,
    green_mask,
    green_shift,
    red_mask,
    red_shift;
} XDirectRowSource;

static void XImportDirectColorRows(const XDirectRowSource *source)
{
  Image
    *const composite_image = source->composite_image;

  CacheView
    *const composite_view = source->composite_view;

  ExceptionInfo
    *const exception = source->exception;

  XImage
    *const ximage = source->ximage;

  XColor
    *const colors = source->colors;

  const size_t
    blue_mask = source->blue_mask,
    blue_shift = source->blue_shift,
    green_mask = source->green_mask,
    green_shift = source->green_shift,
    red_mask = source->red_mask,
    red_shift = source->red_shift;

  int
    x,
    y;

  Quantum
    *magick_restrict q;

  size_t
    index,
    pixel;

  Status
    status;

  for (y=0; y < (int) composite_image->rows; y++)
  {
    q=QueueCacheViewAuthenticPixels(composite_view,0,(ssize_t) y,
      composite_image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) composite_image->columns; x++)
    {
      pixel=XGetPixel(ximage,x,y);
      index=(pixel >> red_shift) & red_mask;
      SetPixelRed(composite_image,
        ScaleShortToQuantum(colors[index].red),q);
      index=(pixel >> green_shift) & green_mask;
      SetPixelGreen(composite_image,
        ScaleShortToQuantum(colors[index].green),q);
      index=(pixel >> blue_shift) & blue_mask;
      SetPixelBlue(composite_image,
        ScaleShortToQuantum(colors[index].blue),q);
      q+=(ptrdiff_t) GetPixelChannels(composite_image);
    }
    status=SyncCacheViewAuthenticPixels(composite_view,exception)
      == MagickFalse ? 0 : 1;
    if (status == MagickFalse)
      break;
  }
}

static void XImportTrueColorPixels(const XDirectRowSource *source,
  const int y,Quantum *magick_restrict q)
{
  Image
    *const composite_image = source->composite_image;

  XImage
    *const ximage = source->ximage;

  const size_t
    blue_mask = source->blue_mask,
    blue_shift = source->blue_shift,
    green_mask = source->green_mask,
    green_shift = source->green_shift,
    red_mask = source->red_mask,
    red_shift = source->red_shift;

  int
    x;

  size_t
    color,
    pixel;

  for (x=0; x < (int) composite_image->columns; x++)
  {
    pixel=XGetPixel(ximage,x,y);
    color=(pixel >> red_shift) & red_mask;
    if (red_mask != 0)
      color=(65535UL*color)/red_mask;
    SetPixelRed(composite_image,ScaleShortToQuantum(
      (unsigned short) color),q);
    color=(pixel >> green_shift) & green_mask;
    if (green_mask != 0)
      color=(65535UL*color)/green_mask;
    SetPixelGreen(composite_image,ScaleShortToQuantum(
      (unsigned short) color),q);
    color=(pixel >> blue_shift) & blue_mask;
    if (blue_mask != 0)
      color=(65535UL*color)/blue_mask;
    SetPixelBlue(composite_image,ScaleShortToQuantum(
      (unsigned short) color),q);
    q+=(ptrdiff_t) GetPixelChannels(composite_image);
  }
}

static void XInitializeWindowColors(const WindowInfo *window_info,
  const int id,const unsigned int number_colors,XColor *colors)
{
  int
    i;

  if ((window_info[id].visual->klass != DirectColor) &&
      (window_info[id].visual->klass != TrueColor))
    for (i=0; i < (int) number_colors; i++)
    {
      colors[i].pixel=(size_t) i;
      colors[i].pad='\0';
    }
  else
    XInitializeDirectColors(window_info,id,number_colors,colors);
}

static void XImportTrueColorRows(const XDirectRowSource *source)
{
  Image
    *const composite_image = source->composite_image;

  CacheView
    *const composite_view = source->composite_view;

  ExceptionInfo
    *const exception = source->exception;

  int
    y;

  Quantum
    *magick_restrict q;

  Status
    status;

  for (y=0; y < (int) composite_image->rows; y++)
  {
    q=QueueCacheViewAuthenticPixels(composite_view,0,(ssize_t) y,
      composite_image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    XImportTrueColorPixels(source,y,q);
    status=SyncCacheViewAuthenticPixels(composite_view,exception)
      == MagickFalse ? 0 : 1;
    if (status == MagickFalse)
      break;
  }
}

typedef struct _ColormapInfo
{
  Colormap
    colormap;

  XColor
    *colors;

  struct _ColormapInfo
    *next;
} ColormapInfo;

static MagickBooleanType IsWindowImported(const WindowInfo *window_info,
  const int id)
{
  int
    j;

  MagickBooleanType
    import;

  /*
    Does target window intersect top level window?
  */
  import=IntersectsTopLevelWindow(window_info,id);
  /*
    Is target window contained by another window with the same colormap?
  */
  for (j=0; j < id; j++)
    if ((window_info[id].visual == window_info[j].visual) &&
        (window_info[id].colormap == window_info[j].colormap))
      {
        if (IsInsideWindowBounds(window_info,id,j) != MagickFalse)
          import=MagickFalse;
      }
  return(import);
}

static ColormapInfo *FindWindowColormap(ColormapInfo *colormap_info,
  const WindowInfo *window_info,const int id)
{
  ColormapInfo
    *p;

  for (p=colormap_info; p != (ColormapInfo *) NULL; p=p->next)
    if (p->colormap == window_info[id].colormap)
      break;
  return(p);
}

typedef struct _WindowColormapInput
{
  Display
    *display;

  const WindowInfo
    *window_info;

  int
    id;

  XImage
    *ximage;
} WindowColormapInput;

typedef struct _WindowColormapTargets
{
  unsigned int
    *number_colors;

  XColor
    **colors;

  ColormapInfo
    **colormap_info;
} WindowColormapTargets;

static MagickBooleanType XGetWindowColormap(const WindowColormapInput *input,
  const WindowColormapTargets *targets)
{
  Display
    *const display = input->display;

  const WindowInfo
    *const window_info = input->window_info;

  const int
    id = input->id;

  XImage
    *const ximage = input->ximage;

  unsigned int
    *const number_colors = targets->number_colors;

  XColor
    **const colors = targets->colors;

  ColormapInfo
    **const colormap_info = targets->colormap_info;

  if (window_info[id].colormap != (Colormap) NULL)
    {
      ColormapInfo
        *p;

      /*
        Search colormap list for window colormap.
      */
      (*number_colors)=(unsigned int) window_info[id].visual->map_entries;
      p=FindWindowColormap((*colormap_info),window_info,id);
      if (p == (ColormapInfo *) NULL)
        {
          /*
            Get the window colormap.
          */
          (*colors)=(XColor *) AcquireQuantumMemory((*number_colors),
            sizeof(*(*colors)));
          if ((*colors) == (XColor *) NULL)
            {
              XDestroyImage(ximage);
              return(MagickFalse);
            }
          XInitializeWindowColors(window_info,id,(*number_colors),(*colors));
          (void) XQueryColors(display,window_info[id].colormap,(*colors),
            (int) (*number_colors));
          /*
            Append colormap to colormap list.
          */
          p=(ColormapInfo *) AcquireMagickMemory(sizeof(*p));
          if (p == (ColormapInfo *) NULL)
            return(MagickFalse);
          p->colormap=window_info[id].colormap;
          p->colors=(*colors);
          p->next=(*colormap_info);
          (*colormap_info)=p;
        }
      (*colors)=p->colors;
    }
  return(MagickTrue);
}

static void XGetMaskShift(const unsigned long visual_mask,size_t *mask,
  size_t *shift)
{
  (*mask)=visual_mask;
  (*shift)=0;
  while (((*mask) != 0) && (((*mask) & 0x01) == 0))
  {
    (*mask)>>=1;
    (*shift)++;
  }
}

typedef struct _WindowImageSource
{
  Image
    *composite_image;

  CacheView
    *composite_view;

  ExceptionInfo
    *exception;

  XImage
    *ximage;

  XColor
    *colors;

  unsigned int
    number_colors;

  WindowInfo
    *window_info;

  int
    id;
} WindowImageSource;

static void XImportDirectClass(const WindowImageSource *source)
{
  Image
    *const composite_image = source->composite_image;

  CacheView
    *const composite_view = source->composite_view;

  ExceptionInfo
    *const exception = source->exception;

  XImage
    *const ximage = source->ximage;

  XColor
    *const colors = source->colors;

  const unsigned int
    number_colors = source->number_colors;

  WindowInfo
    *const window_info = source->window_info;

  const int
    id = source->id;

  size_t
    blue_mask,
    blue_shift,
    green_mask,
    green_shift,
    red_mask,
    red_shift;

  XDirectRowSource
    direct_source;

  /*
    Determine shift and mask for red, green, and blue.
  */
  XGetMaskShift(window_info[id].visual->red_mask,&red_mask,&red_shift);
  XGetMaskShift(window_info[id].visual->green_mask,&green_mask,
    &green_shift);
  XGetMaskShift(window_info[id].visual->blue_mask,&blue_mask,
    &blue_shift);
  /*
    Convert X image to DirectClass packets.
  */
  direct_source.composite_image=composite_image;
  direct_source.composite_view=composite_view;
  direct_source.exception=exception;
  direct_source.ximage=ximage;
  direct_source.colors=colors;
  direct_source.blue_mask=blue_mask;
  direct_source.blue_shift=blue_shift;
  direct_source.green_mask=green_mask;
  direct_source.green_shift=green_shift;
  direct_source.red_mask=red_mask;
  direct_source.red_shift=red_shift;
  if ((number_colors != 0) &&
      (window_info[id].visual->klass == DirectColor))
    XImportDirectColorRows(&direct_source);
  else
    XImportTrueColorRows(&direct_source);
}

static MagickBooleanType XImportWindowImage(const WindowImageSource *source)
{
  Image
    *composite_image = source->composite_image;

  CacheView
    *const composite_view = source->composite_view;

  ExceptionInfo
    *const exception = source->exception;

  XImage
    *const ximage = source->ximage;

  XColor
    *const colors = source->colors;

  const unsigned int
    number_colors = source->number_colors;

  int
    i;

  Status
    status;

  switch (composite_image->storage_class)
  {
    case DirectClass:
    default:
    {
      XImportDirectClass(source);
      break;
    }
    case PseudoClass:
    {
      /*
        Create colormap.
      */
      status=AcquireImageColormap(composite_image,number_colors,
        exception) == MagickFalse ? 0 : 1;
      if (status == MagickFalse)
        {
          XDestroyImage(ximage);
          composite_image=DestroyImage(composite_image);
          return(MagickFalse);
        }
      if (colors == (XColor *) NULL)
        break;
      for (i=0; i < (int) composite_image->colors; i++)
      {
        composite_image->colormap[colors[i].pixel].red=(double)
          ScaleShortToQuantum(colors[i].red);
        composite_image->colormap[colors[i].pixel].green=(double)
          ScaleShortToQuantum(colors[i].green);
        composite_image->colormap[colors[i].pixel].blue=(double)
          ScaleShortToQuantum(colors[i].blue);
      }
      /*
        Convert X image to PseudoClass packets.
      */
      XImportPseudoClassRows(composite_image,composite_view,ximage,
        exception);
      break;
    }
  }
  return(MagickTrue);
}

static Image *XGetWindowImage(const WindowImageContext *window_context,
  const Window window,const unsigned int borders,const unsigned int level)
{
  Display
    *const display = window_context->display;

  ExceptionInfo
    *const exception = window_context->exception;

  int
    display_height,
    display_width,
    id,
    x_offset,
    y_offset;

  RectangleInfo
    crop_info;

  int
    i;

  static ColormapInfo
    *colormap_info = (ColormapInfo *) NULL;

  static int
    max_windows = 0,
    number_windows = 0;

  static WindowInfo
    *window_info;

  Status
    status;

  Window
    child,
    root_window;

  XWindowAttributes
    window_attributes;

  /*
    Verify window is viewable.
  */
  assert(display != (Display *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  status=XGetWindowAttributes(display,window,&window_attributes);
  if ((status == False) || (window_attributes.map_state != IsViewable))
    return((Image *) NULL);
  /*
    Cropping rectangle is relative to root window.
  */
  root_window=XRootWindow(display,XDefaultScreen(display));
  (void) XTranslateCoordinates(display,window,root_window,0,0,&x_offset,
    &y_offset,&child);
  crop_info.x=(ssize_t) x_offset;
  crop_info.y=(ssize_t) y_offset;
  crop_info.width=(size_t) window_attributes.width;
  crop_info.height=(size_t) window_attributes.height;
  if (borders != MagickFalse)
    {
      /*
        Include border in image.
      */
      crop_info.x-=(ssize_t) window_attributes.border_width;
      crop_info.y-=(ssize_t) window_attributes.border_width;
      crop_info.width+=(size_t) (window_attributes.border_width << 1);
      crop_info.height+=(size_t) (window_attributes.border_width << 1);
    }
  /*
    Crop to root window.
  */
  if (crop_info.x < 0)
    {
      crop_info.width=(unsigned int) ((int) crop_info.width+crop_info.x);
      crop_info.x=0;
    }
  if (crop_info.y < 0)
    {
      crop_info.height=(unsigned int) ((int) crop_info.height+crop_info.y);
      crop_info.y=0;
    }
  display_width=XDisplayWidth(display,XDefaultScreen(display));
  if ((int) (crop_info.x+(int) crop_info.width) > display_width)
    crop_info.width=(size_t) (display_width-crop_info.x);
  display_height=XDisplayHeight(display,XDefaultScreen(display));
  if ((int) (crop_info.y+(int) crop_info.height) > display_height)
    crop_info.height=(size_t) (display_height-crop_info.y);
  /*
    Initialize window info attributes.
  */
  if (number_windows >= max_windows)
    {
      /*
        Allocate or resize window info buffer.
      */
      max_windows+=1024;
      if (window_info == (WindowInfo *) NULL)
        window_info=(WindowInfo *) AcquireQuantumMemory((size_t) max_windows,
          sizeof(*window_info));
      else
        window_info=(WindowInfo *) ResizeQuantumMemory(window_info,(size_t)
          max_windows,sizeof(*window_info));
    }
  if (window_info == (WindowInfo *) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed","...");
      return((Image *) NULL);
    }
  id=number_windows++;
  window_info[id].window=window;
  window_info[id].visual=window_attributes.visual;
  window_info[id].colormap=window_attributes.colormap;
  window_info[id].bounds.x1=(short) crop_info.x;
  window_info[id].bounds.y1=(short) crop_info.y;
  window_info[id].bounds.x2=(short) (crop_info.x+(int) crop_info.width-1);
  window_info[id].bounds.y2=(short) (crop_info.y+(int) crop_info.height-1);
  crop_info.x-=x_offset;
  crop_info.y-=y_offset;
  window_info[id].crop_info=crop_info;
  if (level != 0)
    {
      unsigned int
        number_children;

      Window
        *children;

      /*
        Descend the window hierarchy.
      */
      status=XQueryTree(display,window,&root_window,&window_info[id].parent,
        &children,&number_children);
      for (i=0; i < id; i++)
        if ((IsSameVisualParent(window_info,id,i) != MagickFalse) &&
            (IsOutsideParentBounds(window_info,id,i) != MagickFalse))
          {
            /*
              Eliminate windows not circumscribed by their parent.
            */
            number_windows--;
            break;
          }
      if ((status == True) && (number_children != 0))
        {
          const WindowImageContext
            child_context = { display, exception };

          for (i=0; i < (int) number_children; i++)
            (void) XGetWindowImage(&child_context,children[i],MagickFalse,
              level+1);
          (void) XFree((void *) children);
        }
    }
  if (level <= 1)
    {
      CacheView
        *composite_view;

      ColormapInfo
        *next;

      Image
        *composite_image,
        *image;

      MagickBooleanType
        import;

      unsigned int
        number_colors;

      XColor
        *colors;

      XImage
        *ximage;

      const WindowColormapTargets
        targets = { &number_colors,&colors,&colormap_info };

      /*
        Get X image for each window in the list.
      */
      image=NewImageList();
      for (id=0; id < number_windows; id++)
      {
        /*
          Does target window intersect top level window?
        */
        import=IsWindowImported(window_info,id);
        if (import == MagickFalse)
          continue;
        /*
          Get X image.
        */
        ximage=XGetImage(display,window_info[id].window,(int)
          window_info[id].crop_info.x,(int) window_info[id].crop_info.y,
          (unsigned int) window_info[id].crop_info.width,(unsigned int)
          window_info[id].crop_info.height,AllPlanes,ZPixmap);
        if (ximage == (XImage *) NULL)
          continue;
        /*
          Initialize window colormap.
        */
        number_colors=0;
        colors=(XColor *) NULL;
        {
          const WindowColormapInput
            input = { display, window_info, id, ximage };

          if (XGetWindowColormap(&input,&targets) == MagickFalse)
            return((Image *) NULL);
        }
        /*
          Allocate image structure.
        */
        composite_image=AcquireImage((ImageInfo *) NULL,exception);
        if (composite_image == (Image *) NULL)
          {
            XDestroyImage(ximage);
            return((Image *) NULL);
          }
        /*
          Convert X image to MIFF format.
        */
        if ((window_info[id].visual->klass != TrueColor) &&
            (window_info[id].visual->klass != DirectColor))
          composite_image->storage_class=PseudoClass;
        composite_image->columns=(size_t) ximage->width;
        composite_image->rows=(size_t) ximage->height;
        composite_view=AcquireAuthenticCacheView(composite_image,exception);
        {
          const WindowImageSource
            source = { composite_image, composite_view, exception, ximage,
              colors, number_colors, window_info, id };

          if (XImportWindowImage(&source) == MagickFalse)
            return((Image *) NULL);
        }
        composite_view=DestroyCacheView(composite_view);
        XDestroyImage(ximage);
        if (image == (Image *) NULL)
          {
            image=composite_image;
            continue;
          }
        /*
          Composite any children in back-to-front order.
        */
        (void) XTranslateCoordinates(display,window_info[id].window,window,0,0,
          &x_offset,&y_offset,&child);
        x_offset-=(int) crop_info.x;
        if (x_offset < 0)
          x_offset=0;
        y_offset-=(int) crop_info.y;
        if (y_offset < 0)
          y_offset=0;
        (void) CompositeImage(image,composite_image,CopyCompositeOp,MagickTrue,
          (ssize_t) x_offset,(ssize_t) y_offset,exception);
        composite_image=DestroyImage(composite_image);
      }
      /*
        Relinquish resources.
      */
      while (colormap_info != (ColormapInfo *) NULL)
      {
        next=colormap_info->next;
        colormap_info->colors=(XColor *) RelinquishMagickMemory(
          colormap_info->colors);
        colormap_info=(ColormapInfo *) RelinquishMagickMemory(colormap_info);
        colormap_info=next;
      }
      /*
        Relinquish resources and restore initial state.
      */
      window_info=(WindowInfo *) RelinquishMagickMemory(window_info);
      max_windows=0;
      number_windows=0;
      colormap_info=(ColormapInfo *) NULL;
      return(image);
    }
  return((Image *) NULL);
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
static inline MagickBooleanType IsWindowTreeTop(const Status status,
  const Window parent,const Window root)
{
  return(((status == False) || (parent == (Window) NULL) ||
    (parent == root)) ? MagickTrue : MagickFalse);
}

static void XFindFrameWindow(Display *display,Window *root,Window *target)
{
  Status
    status;

  unsigned int
    d;

  Window
    *children;

  for ( ; ; )
  {
    Window
      parent;

    /*
      Find window manager frame.
    */
    status=XQueryTree(display,*target,root,&parent,&children,&d);
    if ((status != False) && (children != (Window *) NULL))
      (void) XFree((char *) children);
    if (IsWindowTreeTop(status,parent,*root) != MagickFalse)
      break;
    *target=parent;
  }
}

static void XSetDescendForColormaps(Display *display,const Window target,
  XImportInfo *ximage_info)
{
  Colormap
    *colormaps;

  int
    number_colormaps,
    number_windows;

  Status
    status;

  Window
    *children;

  /*
    If WM_COLORMAP_WINDOWS property is set or multiple colormaps, descend.
  */
  number_windows=0;
  status=XGetWMColormapWindows(display,target,&children,&number_windows);
  if ((status == True) && (number_windows > 0))
    {
      ximage_info->descend=MagickTrue;
      (void) XFree ((char *) children);
    }
  colormaps=XListInstalledColormaps(display,target,&number_colormaps);
  if (number_colormaps > 0)
    {
      if (number_colormaps > 1)
        ximage_info->descend=MagickTrue;
      (void) XFree((char *) colormaps);
    }
}

static Window XSelectWindowByIDOrName(Display *display,const Window root,
  const ImageInfo *image_info,Window target)
{
  /*
    Select window by ID or name.
  */
  if (isdigit((int) ((unsigned char) *image_info->filename)) != 0)
    target=XWindowByID(display,root,(Window)
      strtol(image_info->filename,(char **) NULL,0));
  if (target == (Window) NULL)
    target=XWindowByName(display,root,image_info->filename);
  if (target == (Window) NULL)
    ThrowXWindowException(XServerError,"NoWindowWithSpecifiedIDExists",
      image_info->filename);
  return(target);
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

MagickExport Image *XImportImage(const ImageInfo *image_info,
  XImportInfo *ximage_info,ExceptionInfo *exception)
{
  Display
    *display;

  Image
    *image;

  int
    x;

  RectangleInfo
    crop_info;

  Status
    status;

  Window
    client,
    prior_target,
    root,
    target;

  WindowImageContext
    window_context;

  const XImportTargets
    targets = { &crop_info, &target };

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
  target=(Window) NULL;
  if (*image_info->filename != '\0')
    {
      if (LocaleCompare(image_info->filename,"root") == 0)
        target=root;
      else
        target=XSelectWindowByIDOrName(display,root,image_info,target);
    }
  /*
    If target window is not defined, interactively select one.
  */
  prior_target=target;
  if (target == (Window) NULL)
    target=XSelectWindow(display,&crop_info);
  if (target == (Window) NULL)
    ThrowXWindowException(XServerError,"UnableToReadXWindowImage",
      image_info->filename);
  client=target;   /* obsolete */
  if (target != root)
    {
      unsigned int
        d;

      status=XGetGeometry(display,target,&root,&x,&x,&d,&d,&d,&d);
      if (status != False)
        {
          XFindFrameWindow(display,&root,&target);
          /*
            Get client window.
          */
          client=XClientWindow(display,target);
          if (ximage_info->frame == MagickFalse)
            target=client;
          if ((ximage_info->frame == MagickFalse) &&
              (prior_target != MagickFalse))
            target=prior_target;
        }
    }
  if (XGetScreenCrop(display,ximage_info,root,&targets) == MagickFalse)
    {
      ThrowXWindowException(XServerError,"UnableToReadXWindowAttributes",
        image_info->filename);
      (void) XCloseDisplay(display);
      return((Image *) NULL);
    }
  XSetDescendForColormaps(display,target,ximage_info);
  /*
    Alert the user not to alter the screen.
  */
  if (ximage_info->silent == MagickFalse)
    (void) XBell(display,0);
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
  if (ximage_info->silent == MagickFalse)
    {
      /*
        Alert the user we're done.
      */
      (void) XBell(display,0);
      (void) XBell(display,0);
    }
  (void) XCloseDisplay(display);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X S e l e c t W i n d o w                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSelectWindow() allows a user to select a window using the mouse.  If the
%  mouse moves, a cropping rectangle is drawn and the extents of the rectangle
%  is returned in the crop_info structure.
%
%  The format of the XSelectWindow function is:
%
%      target_window=XSelectWindow(display,crop_info)
%
%  A description of each parameter follows:
%
%    o window: XSelectWindow returns the window id.
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o crop_info: Specifies a pointer to a RectangleInfo structure.  It
%      contains the extents of any cropping rectangle.
%
*/
static Window XSelectWindow(Display *display,RectangleInfo *crop_info)
{
#define MinimumCropArea  (unsigned int) 9

  Cursor
    target_cursor;

  GC
    annotate_context;

  int
    presses,
    x_offset,
    y_offset;

  Status
    status;

  Window
    root_window,
    target_window;

  XEvent
    event;

  XGCValues
    context_values;

  /*
    Initialize graphic context.
  */
  assert(display != (Display *) NULL);
  assert(crop_info != (RectangleInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  root_window=XRootWindow(display,XDefaultScreen(display));
  context_values.background=XBlackPixel(display,XDefaultScreen(display));
  context_values.foreground=XWhitePixel(display,XDefaultScreen(display));
  context_values.function=GXinvert;
  context_values.plane_mask=
    context_values.background ^ context_values.foreground;
  context_values.subwindow_mode=IncludeInferiors;
  annotate_context=XCreateGC(display,root_window,(size_t) (GCBackground |
    GCForeground | GCFunction | GCSubwindowMode),&context_values);
  if (annotate_context == (GC) NULL)
    return((Window) NULL);
  /*
    Grab the pointer using target cursor.
  */
  target_cursor=XMakeCursor(display,root_window,XDefaultColormap(display,
    XDefaultScreen(display)),(char * ) "white",(char * ) "black");
  status=XGrabPointer(display,root_window,MagickFalse,(unsigned int)
    (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask),GrabModeSync,
    GrabModeAsync,root_window,target_cursor,CurrentTime);
  if (status != GrabSuccess)
    {
      ThrowXWindowException(XServerError,"UnableToGrabMouse","");
      return((Window) NULL);
    }
  /*
    Select a window.
  */
  crop_info->width=0;
  crop_info->height=0;
  presses=0;
  target_window=(Window) NULL;
  x_offset=0;
  y_offset=0;
  (void) XGrabServer(display);
  do
  {
    if ((crop_info->width*crop_info->height) >= MinimumCropArea)
      (void) XDrawRectangle(display,root_window,annotate_context,
        (int) crop_info->x,(int) crop_info->y,(unsigned int) crop_info->width-1,
        (unsigned int) crop_info->height-1);
    /*
      Allow another event.
    */
    (void) XAllowEvents(display,SyncPointer,CurrentTime);
    (void) XWindowEvent(display,root_window,ButtonPressMask |
      ButtonReleaseMask | ButtonMotionMask,&event);
    if ((crop_info->width*crop_info->height) >= MinimumCropArea)
      (void) XDrawRectangle(display,root_window,annotate_context,
        (int) crop_info->x,(int) crop_info->y,(unsigned int) crop_info->width-1,
        (unsigned int) crop_info->height-1);
    switch (event.type)
    {
      case ButtonPress:
      {
        target_window=XGetSubwindow(display,event.xbutton.subwindow,
          event.xbutton.x,event.xbutton.y);
        if (target_window == (Window) NULL)
          target_window=root_window;
        x_offset=event.xbutton.x_root;
        y_offset=event.xbutton.y_root;
        crop_info->x=(ssize_t) x_offset;
        crop_info->y=(ssize_t) y_offset;
        crop_info->width=0;
        crop_info->height=0;
        presses++;
        break;
      }
      case ButtonRelease:
      {
        presses--;
        break;
      }
      case MotionNotify:
      {
        /*
          Discard pending button motion events.
        */
        while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
        crop_info->x=(ssize_t) event.xmotion.x;
        crop_info->y=(ssize_t) event.xmotion.y;
        /*
          Check boundary conditions.
        */
        if ((int) crop_info->x < x_offset)
          crop_info->width=(size_t) (x_offset-crop_info->x);
        else
          {
            crop_info->width=(size_t) (crop_info->x-x_offset);
            crop_info->x=(ssize_t) x_offset;
          }
        if ((int) crop_info->y < y_offset)
          crop_info->height=(size_t) (y_offset-crop_info->y);
        else
          {
            crop_info->height=(size_t) (crop_info->y-y_offset);
            crop_info->y=(ssize_t) y_offset;
          }
      }
      default:
        break;
    }
  } while ((target_window == (Window) NULL) || (presses > 0));
  (void) XUngrabServer(display);
  (void) XUngrabPointer(display,CurrentTime);
  (void) XFreeCursor(display,target_cursor);
  (void) XFreeGC(display,annotate_context);
  if ((crop_info->width*crop_info->height) < MinimumCropArea)
    {
      crop_info->width=0;
      crop_info->height=0;
    }
  if ((crop_info->width != 0) && (crop_info->height != 0))
    target_window=root_window;
  if (event.xbutton.button == Button3)
    target_window=(Window) NULL;
  return(target_window);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X W i n d o w B y I D                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XWindowByID() locates a child window with a given ID.  If not window with
%  the given name is found, 0 is returned.   Only the window specified and its
%  subwindows are searched.
%
%  The format of the XWindowByID function is:
%
%      child=XWindowByID(display,window,id)
%
%  A description of each parameter follows:
%
%    o child: XWindowByID returns the window with the specified
%      id.  If no windows are found, XWindowByID returns 0.
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o id: Specifies the id of the window to locate.
%
*/
MagickPrivate Window XWindowByID(Display *display,const Window root_window,
  const size_t id)
{
  RectangleInfo
    rectangle_info;

  int
    i;

  Status
    status;

  unsigned int
    number_children;

  Window
    child,
    *children,
    window;

  assert(display != (Display *) NULL);
  assert(root_window != (Window) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (id == 0)
    return(XSelectWindow(display,&rectangle_info));
  if (root_window == id)
    return(root_window);
  status=XQueryTree(display,root_window,&child,&child,&children,
    &number_children);
  if (status == False)
    return((Window) NULL);
  window=(Window) NULL;
  for (i=0; i < (int) number_children; i++)
  {
    /*
      Search each child and their children.
    */
    window=XWindowByID(display,children[i],id);
    if (window != (Window) NULL)
      break;
  }
  if (children != (Window *) NULL)
    (void) XFree((void *) children);
  return(window);
}

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
MagickPrivate Window XWindowByName(Display *display,const Window root_window,
  const char *name)
{
  int
    i;

  Status
    status;

  unsigned int
    number_children;

  Window
    *children,
    child,
    window;

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
