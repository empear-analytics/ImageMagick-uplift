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

  MagickCore private X11 window import methods: XClientWindow, XGetSubwindow and the
  XSelectWindow family, shared by xwindow-import.c and xwindow-import-image.c.
*/
#ifndef MAGICKCORE_XWINDOW_IMPORT_PRIVATE_H
#define MAGICKCORE_XWINDOW_IMPORT_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Method prototypes.
*/
static Window
  XSelectWindow(Display *,RectangleInfo *);

static inline Window XPropertyOrTargetWindow(Display *display,
  Window target_window,Atom state)
{
  Window
    client_window;

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
static inline Window XClientWindow(Display *display,Window target_window)
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
  return(XPropertyOrTargetWindow(display,target_window,state));
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
static inline Window XGetSubwindow(Display *display,Window window,int x,int y)
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
static inline GC XCreateInvertContext(Display *display,const Window root_window)
{
  GC
    annotate_context;

  XGCValues
    context_values;

  context_values.background=XBlackPixel(display,XDefaultScreen(display));
  context_values.foreground=XWhitePixel(display,XDefaultScreen(display));
  context_values.function=GXinvert;
  context_values.plane_mask=
    context_values.background ^ context_values.foreground;
  context_values.subwindow_mode=IncludeInferiors;
  annotate_context=XCreateGC(display,root_window,(size_t) (GCBackground |
    GCForeground | GCFunction | GCSubwindowMode),&context_values);
  return(annotate_context);
}

static inline MagickBooleanType XGrabTargetCursor(Display *display,
  const Window root_window,Cursor *target_cursor)
{
  Status
    status;

  {
    const XCursorColors
      cursor_colors = { (char * ) "white", (char * ) "black" };

    (*target_cursor)=XMakeCursor(display,root_window,XDefaultColormap(display,
      XDefaultScreen(display)),&cursor_colors);
  }
  status=XGrabPointer(display,root_window,MagickFalse,(unsigned int)
    (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask),GrabModeSync,
    GrabModeAsync,root_window,(*target_cursor),CurrentTime);
  if (status != GrabSuccess)
    return(MagickFalse);
  return(MagickTrue);
}

#define MinimumCropArea  (unsigned int) 9

static inline void XDrawCropRectangle(Display *display,const Window root_window,
  const GC annotate_context,const RectangleInfo *crop_info)
{
  if ((crop_info->width*crop_info->height) >= MinimumCropArea)
    (void) XDrawRectangle(display,root_window,annotate_context,
      (int) crop_info->x,(int) crop_info->y,(unsigned int) crop_info->width-1,
      (unsigned int) crop_info->height-1);
}

static inline void XCheckCropBoundary(RectangleInfo *crop_info,const int x_offset,
  const int y_offset)
{
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

static inline Window XSelectCropWindow(const Window root_window,
  RectangleInfo *crop_info,Window target_window)
{
  if ((crop_info->width*crop_info->height) < MinimumCropArea)
    {
      crop_info->width=0;
      crop_info->height=0;
    }
  if ((crop_info->width != 0) && (crop_info->height != 0))
    target_window=root_window;
  return(target_window);
}

typedef struct _XSelectionInput
{
  Display
    *display;

  Window
    root_window;

  RectangleInfo
    *crop_info;
} XSelectionInput;

typedef struct _XSelectionTargets
{
  Window
    *target_window;

  int
    *x_offset,
    *y_offset,
    *presses;

  XEvent
    *event;
} XSelectionTargets;

static inline void XHandleSelectionEvent(const XSelectionInput *input,
  const XSelectionTargets *targets)
{
  Display
    *const display = input->display;

  Window
    *const target_window = targets->target_window;

  const Window
    root_window = input->root_window;

  RectangleInfo
    *const crop_info = input->crop_info;

  int
    *const x_offset = targets->x_offset,
    *const y_offset = targets->y_offset,
    *const presses = targets->presses;

  XEvent
    *const event = targets->event;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      (*target_window)=XGetSubwindow(display,(*event).xbutton.subwindow,
        (*event).xbutton.x,(*event).xbutton.y);
      if ((*target_window) == (Window) NULL)
        (*target_window)=root_window;
      (*x_offset)=(*event).xbutton.x_root;
      (*y_offset)=(*event).xbutton.y_root;
      crop_info->x=(ssize_t) (*x_offset);
      crop_info->y=(ssize_t) (*y_offset);
      crop_info->width=0;
      crop_info->height=0;
      (*presses)++;
      break;
    }
    case ButtonRelease:
    {
      (*presses)--;
      break;
    }
    case MotionNotify:
    {
      /*
        Discard pending button motion events.
      */
      while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
      crop_info->x=(ssize_t) (*event).xmotion.x;
      crop_info->y=(ssize_t) (*event).xmotion.y;
      XCheckCropBoundary(crop_info,(*x_offset),(*y_offset));
    }
    default:
      break;
  }
}

static inline Window XSelectWindow(Display *display,RectangleInfo *crop_info)
{
  Cursor
    target_cursor;

  GC
    annotate_context;

  int
    presses,
    x_offset,
    y_offset;

  Window
    root_window,
    target_window;

  XEvent
    event;

  const XSelectionTargets
    targets = { &target_window,&x_offset,&y_offset,&presses,&event };

  /*
    Initialize graphic context.
  */
  assert(display != (Display *) NULL);
  assert(crop_info != (RectangleInfo *) NULL);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  root_window=XRootWindow(display,XDefaultScreen(display));
  annotate_context=XCreateInvertContext(display,root_window);
  if (annotate_context == (GC) NULL)
    return((Window) NULL);
  /*
    Grab the pointer using target cursor.
  */
  if (XGrabTargetCursor(display,root_window,&target_cursor) == MagickFalse)
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
    XDrawCropRectangle(display,root_window,annotate_context,crop_info);
    /*
      Allow another event.
    */
    (void) XAllowEvents(display,SyncPointer,CurrentTime);
    (void) XWindowEvent(display,root_window,ButtonPressMask |
      ButtonReleaseMask | ButtonMotionMask,&event);
    XDrawCropRectangle(display,root_window,annotate_context,crop_info);
    {
      const XSelectionInput
        input = { display, root_window, crop_info };

      XHandleSelectionEvent(&input,&targets);
    }
  } while ((target_window == (Window) NULL) || (presses > 0));
  (void) XUngrabServer(display);
  (void) XUngrabPointer(display,CurrentTime);
  (void) XFreeCursor(display,target_cursor);
  (void) XFreeGC(display,annotate_context);
  target_window=XSelectCropWindow(root_window,crop_info,target_window);
  if (event.xbutton.button == Button3)
    target_window=(Window) NULL;
  return(target_window);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
