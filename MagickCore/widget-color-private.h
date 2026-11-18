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

  MagickCore private methods of widget.c: the triangle and bevel drawing
  helpers and the colour browser, the run between DestroyXWidget and
  XColorBrowserWidget; included by that file only.
*/
#ifndef MAGICKCORE_WIDGET_COLOR_PRIVATE_H
#define MAGICKCORE_WIDGET_COLOR_PRIVATE_H

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w T r i a n g l e E a s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawTriangleEast() draws a triangle with a highlighted left bevel and a
%  shadowed right and lower bevel.  The highlighted and shadowed bevels create
%  a 3-D effect.
%
%  The format of the XDrawTriangleEast function is:
%
%      XDrawTriangleEast(display,window_info,triangle_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o triangle_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the triangle.
%
*/
static void XDrawTriangleEastLabel(Display *display,
  const XWindowInfo *window_info,const XWidgetInfo *triangle_info)
{
  int
    x1,
    y1;

  XFontStruct
    *font_info;

  font_info=window_info->font_info;
  XSetTextColor(display,window_info,MagickTrue);
  x1=triangle_info->x+(int) triangle_info->width+(int)
    triangle_info->bevel_width+(QuantumMargin >> 1);
  y1=triangle_info->y+(((int) triangle_info->height-(int)
    (font_info->ascent+font_info->descent)) >> 1)+(int) font_info->ascent;
  (void) XDrawString(display,window_info->id,window_info->widget_context,x1,y1,
    triangle_info->text,Extent(triangle_info->text));
}

static void XDrawTriangleEast(Display *display,const XWindowInfo *window_info,
  const XWidgetInfo *triangle_info)
{
  int
    x1,
    x2,
    x3,
    y1,
    y2,
    y3;

  unsigned int
    bevel_width;

  XPoint
    points[4];

  /*
    Draw triangle matte.
  */
  x1=triangle_info->x;
  y1=triangle_info->y;
  x2=triangle_info->x+(int) triangle_info->width;
  y2=triangle_info->y+(int) (triangle_info->height >> 1);
  x3=triangle_info->x;
  y3=triangle_info->y+(int) triangle_info->height;
  bevel_width=triangle_info->bevel_width;
  points[0].x=x1;
  points[0].y=y1;
  points[1].x=x2;
  points[1].y=y2;
  points[2].x=x3;
  points[2].y=y3;
  XSetMatteColor(display,window_info,triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,3,Complex,CoordModeOrigin);
  /*
    Draw bottom bevel.
  */
  points[0].x=x2;
  points[0].y=y2;
  points[1].x=x3;
  points[1].y=y3;
  points[2].x=x3-(int) bevel_width;
  points[2].y=y3+(int) bevel_width;
  points[3].x=x2+(int) bevel_width;
  points[3].y=y2;
  XSetBevelColor(display,window_info,!triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  /*
    Draw Left bevel.
  */
  points[0].x=x3;
  points[0].y=y3;
  points[1].x=x1;
  points[1].y=y1;
  points[2].x=x1-(int) bevel_width+1;
  points[2].y=y1-(int) bevel_width;
  points[3].x=x3-(int) bevel_width+1;
  points[3].y=y3+(int) bevel_width;
  XSetBevelColor(display,window_info,triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  /*
    Draw top bevel.
  */
  points[0].x=x1;
  points[0].y=y1;
  points[1].x=x2;
  points[1].y=y2;
  points[2].x=x2+(int) bevel_width;
  points[2].y=y2;
  points[3].x=x1-(int) bevel_width;
  points[3].y=y1-(int) bevel_width;
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  (void) XSetFillStyle(display,window_info->widget_context,FillSolid);
  if (triangle_info->text == (char *) NULL)
    return;
  /*
    Write label to right of triangle.
  */
  XDrawTriangleEastLabel(display,window_info,triangle_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w T r i a n g l e N o r t h                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawTriangleNorth() draws a triangle with a highlighted left bevel and a
%  shadowed right and lower bevel.  The highlighted and shadowed bevels create
%  a 3-D effect.
%
%  The format of the XDrawTriangleNorth function is:
%
%      XDrawTriangleNorth(display,window_info,triangle_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o triangle_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the triangle.
%
*/
static void XDrawTriangleNorth(Display *display,const XWindowInfo *window_info,
  const XWidgetInfo *triangle_info)
{
  int
    x1,
    x2,
    x3,
    y1,
    y2,
    y3;

  unsigned int
    bevel_width;

  XPoint
    points[4];

  /*
    Draw triangle matte.
  */
  x1=triangle_info->x;
  y1=triangle_info->y+(int) triangle_info->height;
  x2=triangle_info->x+(int) (triangle_info->width >> 1);
  y2=triangle_info->y;
  x3=triangle_info->x+(int) triangle_info->width;
  y3=triangle_info->y+(int) triangle_info->height;
  bevel_width=triangle_info->bevel_width;
  points[0].x=x1;
  points[0].y=y1;
  points[1].x=x2;
  points[1].y=y2;
  points[2].x=x3;
  points[2].y=y3;
  XSetMatteColor(display,window_info,triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,3,Complex,CoordModeOrigin);
  /*
    Draw left bevel.
  */
  points[0].x=x1;
  points[0].y=y1;
  points[1].x=x2;
  points[1].y=y2;
  points[2].x=x2;
  points[2].y=y2-(int) bevel_width-2;
  points[3].x=x1-(int) bevel_width-1;
  points[3].y=y1+(int) bevel_width;
  XSetBevelColor(display,window_info,triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  /*
    Draw right bevel.
  */
  points[0].x=x2;
  points[0].y=y2;
  points[1].x=x3;
  points[1].y=y3;
  points[2].x=x3+(int) bevel_width;
  points[2].y=y3+(int) bevel_width;
  points[3].x=x2;
  points[3].y=y2-(int) bevel_width;
  XSetBevelColor(display,window_info,!triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  /*
    Draw lower bevel.
  */
  points[0].x=x3;
  points[0].y=y3;
  points[1].x=x1;
  points[1].y=y1;
  points[2].x=x1-(int) bevel_width;
  points[2].y=y1+(int) bevel_width;
  points[3].x=x3+(int) bevel_width;
  points[3].y=y3+(int) bevel_width;
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  (void) XSetFillStyle(display,window_info->widget_context,FillSolid);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w T r i a n g l e S o u t h                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawTriangleSouth() draws a border with a highlighted left and right bevel
%  and a shadowed lower bevel.  The highlighted and shadowed bevels create a
%  3-D effect.
%
%  The format of the XDrawTriangleSouth function is:
%
%      XDrawTriangleSouth(display,window_info,triangle_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o triangle_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the triangle.
%
*/
static void XDrawTriangleSouth(Display *display,const XWindowInfo *window_info,
  const XWidgetInfo *triangle_info)
{
  int
    x1,
    x2,
    x3,
    y1,
    y2,
    y3;

  unsigned int
    bevel_width;

  XPoint
    points[4];

  /*
    Draw triangle matte.
  */
  x1=triangle_info->x;
  y1=triangle_info->y;
  x2=triangle_info->x+(int) (triangle_info->width >> 1);
  y2=triangle_info->y+(int) triangle_info->height;
  x3=triangle_info->x+(int) triangle_info->width;
  y3=triangle_info->y;
  bevel_width=triangle_info->bevel_width;
  points[0].x=x1;
  points[0].y=y1;
  points[1].x=x2;
  points[1].y=y2;
  points[2].x=x3;
  points[2].y=y3;
  XSetMatteColor(display,window_info,triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,3,Complex,CoordModeOrigin);
  /*
    Draw top bevel.
  */
  points[0].x=x3;
  points[0].y=y3;
  points[1].x=x1;
  points[1].y=y1;
  points[2].x=x1-(int) bevel_width;
  points[2].y=y1-(int) bevel_width;
  points[3].x=x3+(int) bevel_width;
  points[3].y=y3-(int) bevel_width;
  XSetBevelColor(display,window_info,triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  /*
    Draw right bevel.
  */
  points[0].x=x2;
  points[0].y=y2;
  points[1].x=x3+1;
  points[1].y=y3-(int) bevel_width;
  points[2].x=x3+(int) bevel_width;
  points[2].y=y3-(int) bevel_width;
  points[3].x=x2;
  points[3].y=y2+(int) bevel_width;
  XSetBevelColor(display,window_info,!triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  /*
    Draw left bevel.
  */
  points[0].x=x1;
  points[0].y=y1;
  points[1].x=x2;
  points[1].y=y2;
  points[2].x=x2;
  points[2].y=y2+(int) bevel_width;
  points[3].x=x1-(int) bevel_width;
  points[3].y=y1-(int) bevel_width;
  XSetBevelColor(display,window_info,triangle_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,4,Complex,CoordModeOrigin);
  (void) XSetFillStyle(display,window_info->widget_context,FillSolid);
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
%  XScreenEvent() returns MagickTrue if the any event on the X server queue is
%  associated with the widget window.
%
%  The format of the XScreenEvent function is:
%
%      int XScreenEvent(Display *display,XEvent *event,char *data)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o event: Specifies a pointer to a X11 XEvent structure.
%
%    o data: Specifies a pointer to a XWindows structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif




typedef struct _ListBrowserClick
{
  Display
    *display;

  XWindowInfo
    *window_info;

  XEvent
    event;

  Time
    click_time;
} ListBrowserClick;

typedef struct _ListBrowserRelease
{
  Display
    *display;

  XWindowInfo
    *window_info;

  XEvent
    event;
} ListBrowserRelease;

typedef struct _FontBrowserTargets
{
  XWidgetInfo
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *selection_info,
    *list_info;
} FontBrowserTargets;

typedef struct _FontBrowserPress
{
  Display
    *display;

  XWindows
    *windows;

  char
    **fontlist;

  int
    fonts;

  unsigned int
    height;

  XEvent
    event;

  XWidgetInfo
    slider_info;
} FontBrowserPress;

typedef struct _FontBrowserNames
{
  Display
    *display;

  XWindows
    *windows;

  char
    **fontlist;

  int
    fonts;

  unsigned int
    visible_fonts;

  XWidgetInfo
    list_info,
    slider_info;
} FontBrowserNames;

typedef struct _FontBrowserMatch
{
  char
    **fontlist;

  int
    fonts;

  char
    *reply;
} FontBrowserMatch;

typedef struct _FontBrowserBack
{
  XWindows
    *windows;

  XEvent
    event;

  XWidgetInfo
    back_info;

  char
    *glob_pattern;

  const char
    *back_pattern;
} FontBrowserBack;

static XWidgetInfo
  ApplyListBrowserControlKey(XWidgetInfo,const KeySym,const size_t);

static size_t
  ConfirmListBrowserReply(Display *,const XWidgetInfo,size_t);

static void
  ReleaseListBrowserAction(const ListBrowserRelease *,const XWidgetInfo,
    XWidgetInfo *,size_t *);

static void
  ReleaseListBrowserCancel(const ListBrowserRelease *,const XWidgetInfo,
    XWidgetInfo *,size_t *);

static XWidgetInfo
  HighlightListBrowserReply(const ListBrowserClick *,char *,XWidgetInfo);

static XWidgetInfo
  PlaceTextViewExpose(XWidgetInfo,const XWidgetInfo);

static void
  SelectFontBrowserEntry(const FontBrowserPress *,char *,
    const FontBrowserTargets *);

static int
  FindListBrowserCursor(XFontStruct *,const XWidgetInfo,const int);

static XWidgetInfo
  PageListBrowserSlider(XWidgetInfo,const XEvent,const unsigned int);

static void
  MatchFontBrowserReply(const FontBrowserMatch *,XWidgetInfo *,int *);

static void
  DrawFontBrowserNames(const FontBrowserNames *,XWidgetInfo *,int *);

static size_t
  ReleaseFontBrowserBack(const FontBrowserBack *,size_t);

static void
  MoveFontBrowserSliderByPage(const KeySym,const unsigned int,const int,
    XWidgetInfo *);

static XWidgetInfo
  MoveFontBrowserSlider(XWidgetInfo,const KeySym,const unsigned int,const int);

static void CopyColorBrowserMode(const XWidgetInfo mode_info,
  const XWidgetInfo reply_info)
{
  if (mode_info.text != (char *) NULL)
    (void) CopyMagickString(reply_info.text,mode_info.text,
      MagickPathExtent);
}

static XWidgetInfo TrackColorBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const size_t colors)
{
  slider_info.y=event.xmotion.y-(int)
    ((slider_info.height+slider_info.bevel_width) >> 1)+1;
  if (slider_info.y < slider_info.min_y)
    slider_info.y=slider_info.min_y;
  if (slider_info.y > slider_info.max_y)
    slider_info.y=slider_info.max_y;
  slider_info.id=0;
  if (slider_info.y != slider_info.min_y)
    slider_info.id=(int) (((int) colors*(slider_info.y-
      slider_info.min_y+1))/(slider_info.max_y-slider_info.min_y+1));
  return(slider_info);
}

static size_t ReadColorBrowserPattern(const XWidgetInfo reply_info,
  char *glob_pattern,size_t state)
{
  /*
    Read new color or glob pattern.
  */
  if (*reply_info.text == '\0')
    return(state);
  (void) CopyMagickString(glob_pattern,reply_info.text,MagickPathExtent);
  state|=UpdateListState;
  return(state);
}

typedef struct _ColorBrowserGrab
{
  Display
    *display;

  XWindows
    *windows;

  XEvent
    event;

  XWidgetInfo
    grab_info;

  ExceptionInfo
    *exception;
} ColorBrowserGrab;

static void ReleaseColorBrowserGrab(const ColorBrowserGrab *grab,
  XWidgetInfo *reply_info,size_t *state)
{
  Display
    *const display = grab->display;

  XWindows
    *const windows = grab->windows;

  const XEvent
    event = grab->event;

  const XWidgetInfo
    grab_info = grab->grab_info;

  ExceptionInfo
    *const exception = grab->exception;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(grab_info,event.xbutton)))
    {
      /*
        Select a fill color from the X server.
      */
      (void) XGetWindowColor(display,windows,(*reply_info).text,
        exception);
      (*reply_info).marker=(*reply_info).text;
      (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
      XDrawMatteText(display,&windows->widget,&(*reply_info));
      (*state)|=RedrawActionState;
    }
}

typedef struct _ColorBrowserUpdate
{
  Display
    *display;

  XWindows
    *windows;

  char
    *reply,
    *glob_pattern;

  const char
    *reset_pattern;

  ExceptionInfo
    *exception;
} ColorBrowserUpdate;

typedef struct _ColorBrowserUpdateTargets
{
  XWidgetInfo
    *action_info;

  char
    ***colorlist;

  size_t
    *colors;
} ColorBrowserUpdateTargets;

static MagickBooleanType UpdateColorBrowserList(
  const ColorBrowserUpdate *update,const ColorBrowserUpdateTargets *targets)
{
  Display
    *const display = update->display;

  XWindows
    *const windows = update->windows;

  char
    *const reply = update->reply,
    *const glob_pattern = update->glob_pattern;

  const char
    *const reset_pattern = update->reset_pattern;

  ExceptionInfo
    *const exception = update->exception;

  XWidgetInfo
    *const action_info = targets->action_info;

  char
    ***const colorlist = targets->colorlist;

  size_t
    *const colors = targets->colors;

  char
    **checklist;

  int
    i;

  size_t
    number_colors;

  Status
    status;

  XColor
    color;

  status=XParseColor(display,windows->widget.map_info->colormap,
    glob_pattern,&color);
  if ((status != False) || (strchr(glob_pattern,'-') != (char *) NULL))
    {
      /*
        Reply is a single color name-- exit.
      */
      (void) CopyMagickString(reply,glob_pattern,MagickPathExtent);
      (void) CopyMagickString(glob_pattern,reset_pattern,MagickPathExtent);
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      return(MagickFalse);
    }
  /*
    Update color list.
  */
  checklist=GetColorList(glob_pattern,&number_colors,exception);
  if (number_colors == 0)
    {
      (void) CopyMagickString(glob_pattern,reset_pattern,MagickPathExtent);
      (void) XBell(display,0);
    }
  else
    {
      for (i=0; i < (int) (*colors); i++)
        (*colorlist)[i]=DestroyString((*colorlist)[i]);
      if ((*colorlist) != (char **) NULL)
        (*colorlist)=(char **) RelinquishMagickMemory((*colorlist));
      (*colorlist)=checklist;
      (*colors)=number_colors;
    }
  return(MagickTrue);
}

typedef struct _ColorBrowserTargets
{
  XWidgetInfo
    *reply_info;

  size_t
    *state;

  XWidgetInfo
    *grab_info,
    *reset_info,
    *action_info,
    *cancel_info,
    *slider_info;

  size_t
    *colors;

  XWidgetInfo
    *scroll_info;

  unsigned int
    *visible_colors;

  XWidgetInfo
    *north_info;

  size_t
    *delay;

  XWidgetInfo
    *south_info,
    *mode_info;

  char
    ***colorlist;

  XWidgetInfo
    *list_info,
    *expose_info,
    *text_info,
    *selection_info;

  MagickStatusType
    *mask;
} ColorBrowserTargets;

typedef struct _ColorBrowserContext
{
  Display
    *display;

  XWindows
    *windows;

  char
    *glob_pattern,
    *reset_pattern;

  ExceptionInfo
    *exception;

  XFontStruct
    *font_info;

  char
    *primary_selection;

  unsigned int
    height;

  const char
    *action;

  unsigned int
    width;

  char
    *reply,
    *text;
} ColorBrowserContext;

static void ClientColorBrowserMessage(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  /*
    If client window delete message, exit.
  */
  if (event.xclient.message_type != windows->wm_protocols)
    return;
  if (*event.xclient.data.l == (int) windows->wm_take_focus)
    {
      (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
        (Time) event.xclient.data.l[1]);
      return;
    }
  if (*event.xclient.data.l != (int) windows->wm_delete_window)
    return;
  if (event.xclient.window == windows->widget.id)
    {
      *(*reply_info).text='\0';
      (*state)|=ExitState;
      return;
    }
}

static size_t ReleaseWidgetKey(XWindows *windows,const XEvent event,
  size_t state)
{
  static char
    command[MagickPathExtent];

  static KeySym
    key_symbol;

  /*
    Respond to a user key release.
  */
  if (event.xkey.window != windows->widget.id)
    return(state);
  (void) XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  if (key_symbol == XK_Control_L)
    state&=(unsigned int) (~ControlState);
  return(state);
}

static void SelectColorBrowserReply(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  Status
    status;

  Atom
    type;

  int
    format;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  /*
    Obtain response from primary selection.
  */
  if (event.xselection.property == (Atom) None)
    return;
  status=XGetWindowProperty(display,event.xselection.requestor,
    event.xselection.property,0L,2047L,MagickTrue,XA_STRING,&type,
    &format,&length,&after,&data);
  if (IsUnusableSelection(status,type,format,length) != MagickFalse)
    return;
  if ((Extent((*reply_info).text)+(int) length) >= (MagickPathExtent-1))
    (void) XBell(display,0);
  else
    {
      /*
        Insert primary selection in reply text.
      */
      *(data+length)='\0';
      XEditText(display,reply_info,(KeySym) XK_Insert,(char *) data,
        (*state));
      XDrawMatteText(display,&windows->widget,reply_info);
      (*state)|=JumpListState;
      (*state)|=RedrawActionState;
    }
  (void) XFree((void *) data);
}

static void ToggleColorBrowserButtons(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  if ((*grab_info).raised == MatteIsActive((*grab_info),event.xmotion))
    {
      /*
        Grab button status changed.
      */
      (*grab_info).raised=!(*grab_info).raised;
      XDrawBeveledButton(display,&windows->widget,grab_info);
      return;
    }
  if ((*reset_info).raised == MatteIsActive((*reset_info),event.xmotion))
    {
      /*
        Reset button status changed.
      */
      (*reset_info).raised=!(*reset_info).raised;
      XDrawBeveledButton(display,&windows->widget,reset_info);
      return;
    }
  if ((*action_info).raised == MatteIsActive((*action_info),event.xmotion))
    {
      /*
        Action button status changed.
      */
      (*action_info).raised=(*action_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      return;
    }
  if ((*cancel_info).raised == MatteIsActive((*cancel_info),event.xmotion))
    {
      /*
        Cancel button status changed.
      */
      (*cancel_info).raised=(*cancel_info).raised == MagickFalse ?
        MagickTrue : MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
}

static void MoveColorBrowserButtons(const ColorBrowserContext *context,
  XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  /*
    Discard pending button motion events.
  */
  while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
  if ((*slider_info).active)
    {
      /*
        Move slider matte.
      */
      (*slider_info)=TrackColorBrowserSlider((*slider_info),event,(*colors));
      (*state)|=RedrawListState;
      return;
    }
  if ((*state) & InactiveWidgetState)
    return;
  ToggleColorBrowserButtons(context,event,widgets);
}

static void PressColorBrowserKey(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XColor
    color;

  Status
    status;

  static char
    command[MagickPathExtent];

  static int
    length;

  static KeySym
    key_symbol;

  /*
    Respond to a user key press.
  */
  if (event.xkey.window != windows->widget.id)
    return;
  length=XLookupString((XKeyEvent *) &event.xkey,command,
    (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (AreaIsActive((*scroll_info),event.xkey))
    {
      /*
        Move slider.
      */
      (*slider_info)=MoveFontBrowserSlider((*slider_info),key_symbol,
        (*visible_colors),(int) (*colors));
      (*state)|=RedrawListState;
      return;
    }
  if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
    {
      (*state)=ReadColorBrowserPattern((*reply_info),glob_pattern,(*state));
      return;
    }
  if (key_symbol == XK_Control_L)
    {
      (*state)|=ControlState;
      return;
    }
  (*reply_info)=ApplyListBrowserControlKey((*reply_info),key_symbol,(*state));
  XEditText(display,reply_info,key_symbol,command,(*state));
  XDrawMatteText(display,&windows->widget,reply_info);
  (*state)|=JumpListState;
  status=XParseColor(display,windows->widget.map_info->colormap,
    (*reply_info).text,&color);
  if (status != False)
    (*state)|=RedrawActionState;
}

static void ReleaseColorBrowserButtons(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const reset_pattern = context->reset_pattern;

  ExceptionInfo
    *const exception = context->exception;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  ListBrowserRelease
    release;

  if ((*grab_info).raised == MagickFalse)
    {
      ColorBrowserGrab
        grab;

      grab.display=display;
      grab.windows=windows;
      grab.event=event;
      grab.grab_info=(*grab_info);
      grab.exception=exception;
      ReleaseColorBrowserGrab(&grab,reply_info,state);
      (*grab_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,grab_info);
    }
  if ((*reset_info).raised == MagickFalse)
    {
      FontBrowserBack
        back;

      back.windows=windows;
      back.event=event;
      back.back_info=(*reset_info);
      back.glob_pattern=glob_pattern;
      back.back_pattern=reset_pattern;
      (*state)=ReleaseFontBrowserBack(&back,(*state));
      (*reset_info).raised=MagickTrue;
      XDrawBeveledButton(display,&windows->widget,reset_info);
    }
  release.display=display;
  release.window_info=(&windows->widget);
  release.event=event;
  ReleaseListBrowserAction(&release,(*reply_info),action_info,state);
  release.display=display;
  release.window_info=(&windows->widget);
  release.event=event;
  ReleaseListBrowserCancel(&release,(*reply_info),cancel_info,state);
  if (MatteIsActive((*reply_info),event.xbutton) == MagickFalse)
    return;
}

static void ReleaseColorBrowserScroll(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const slider_info = widgets->slider_info,
    *const north_info = widgets->north_info;

  size_t
    *const delay = widgets->delay;

  XWidgetInfo
    *const south_info = widgets->south_info;

  if (windows->widget.mapped == MagickFalse)
    return;
  if ((*north_info).raised == MagickFalse)
    {
      /*
        User released up button.
      */
      (*delay)=SuspendTime << 2;
      (*north_info).raised=MagickTrue;
      XDrawTriangleNorth(display,&windows->widget,north_info);
    }
  if ((*south_info).raised == MagickFalse)
    {
      /*
        User released down button.
      */
      (*delay)=SuspendTime << 2;
      (*south_info).raised=MagickTrue;
      XDrawTriangleSouth(display,&windows->widget,south_info);
    }
  if ((*slider_info).active)
    {
      /*
        Stop tracking slider.
      */
      (*slider_info).active=MagickFalse;
      return;
    }
  ReleaseColorBrowserButtons(context,event,widgets);
}

static void PressColorBrowserReply(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  int
    x;

  int
    i;

  if (MatteIsActive((*action_info),event.xbutton))
    {
      /*
        User pressed action button.
      */
      (*action_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,action_info);
      return;
    }
  if (MatteIsActive((*cancel_info),event.xbutton))
    {
      /*
        User pressed Cancel button.
      */
      (*cancel_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      return;
    }
  if (MatteIsActive((*reply_info),event.xbutton) == MagickFalse)
    return;
  if (event.xbutton.button != Button2)
    {
      static Time
        click_time;

      ListBrowserClick
        click;

      /*
        Move text cursor to position of button press.
      */
      x=event.xbutton.x-(*reply_info).x-(int) (QuantumMargin >> 2);
      i=FindListBrowserCursor(font_info,(*reply_info),x);
      (*reply_info).cursor=(*reply_info).marker+i-1;
      click.display=display;
      click.window_info=(&windows->widget);
      click.event=event;
      click.click_time=click_time;
      (*reply_info)=HighlightListBrowserReply(&click,primary_selection,
        (*reply_info));
      XDrawMatteText(display,&windows->widget,reply_info);
      click_time=event.xbutton.time;
      return;
    }
  /*
    Request primary selection.
  */
  (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
    windows->widget.id,event.xbutton.time);
}

static void PressColorBrowserButtons(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const mode_info = widgets->mode_info;

  if (MatteIsActive((*grab_info),event.xbutton))
    {
      /*
        User pressed Grab button.
      */
      (*grab_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,grab_info);
      return;
    }
  if (MatteIsActive((*reset_info),event.xbutton))
    {
      /*
        User pressed Reset button.
      */
      (*reset_info).raised=MagickFalse;
      XDrawBeveledButton(display,&windows->widget,reset_info);
      return;
    }
  if (MatteIsActive((*mode_info),event.xbutton))
    {
      /*
        User pressed mode button.
      */
      CopyColorBrowserMode((*mode_info),(*reply_info));
      (void) CopyMagickString(primary_selection,(*reply_info).text,
        MagickPathExtent);
      (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
        event.xbutton.time);
      (*reply_info).highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
        windows->widget.id ? MagickTrue : MagickFalse;
      (*reply_info).marker=(*reply_info).text;
      (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
      XDrawMatteText(display,&windows->widget,reply_info);
      return;
    }
  PressColorBrowserReply(context,event,widgets);
}

static void SelectColorBrowserEntry(const ColorBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  char
    ***const colorlist = widgets->colorlist;

  FontBrowserPress
    press;

  press.display=display;
  press.windows=windows;
  press.fontlist=(*colorlist);
  press.fonts=(*colors);
  press.height=height;
  press.event=event;
  press.slider_info=(*slider_info);
  SelectFontBrowserEntry(&press,glob_pattern,targets);
}

static void PressColorBrowserScroll(const ColorBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const ColorBrowserTargets *widgets)
{
  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info;

  if (MatteIsActive((*slider_info),event.xbutton))
    {
      /*
        Track slider.
      */
      (*slider_info).active=MagickTrue;
      return;
    }
  if ((MatteIsActive((*north_info),event.xbutton)) && ((*slider_info).id > 0))
    {
      /*
        Move slider up.
      */
      (*north_info).raised=MagickFalse;
      (*slider_info).id--;
      (*state)|=RedrawListState;
      return;
    }
  if ((MatteIsActive((*south_info),event.xbutton)) &&
      ((*slider_info).id < (int) (*colors)))
    {
      /*
        Move slider down.
      */
      (*south_info).raised=MagickFalse;
      (*slider_info).id++;
      (*state)|=RedrawListState;
      return;
    }
  if (MatteIsActive((*scroll_info),event.xbutton))
    {
      /*
        Move slider.
      */
      (*slider_info)=PageListBrowserSlider((*slider_info),event,
        (*visible_colors));
      (*state)|=RedrawListState;
      return;
    }
  if (MatteIsActive((*list_info),event.xbutton))
    {
      SelectColorBrowserEntry(context,event,targets,
        widgets);
      return;
    }
  PressColorBrowserButtons(context,event,widgets);
}

#define CancelButtonText  "Cancel"
#define ColornameText  "Name:"
#define ColorPatternText  "Pattern:"
#define GrabButtonText  "Grab"
#define ResetButtonText  "Reset"

static void InitializeColorBrowserButtons(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  const char
    *const action = context->action;

  const unsigned int
    width = context->width;

  XWidgetInfo
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info;

  /*
    Initialize button information.
  */
  XGetWidgetInfo(CancelButtonText,cancel_info);
  (*cancel_info).width=width;
  (*cancel_info).height=(unsigned int) ((3*height) >> 1);
  (*cancel_info).x=(int) windows->widget.width-(int) (*cancel_info).width-
    QuantumMargin-2;
  (*cancel_info).y=(int) windows->widget.height-(int) (*cancel_info).height-
    QuantumMargin;
  XGetWidgetInfo(action,action_info);
  (*action_info).width=width;
  (*action_info).height=(unsigned int) ((3*height) >> 1);
  (*action_info).x=(int) windows->widget.width-(int) (*action_info).width-
    (int) (*cancel_info).width-2*QuantumMargin-2;
  (*action_info).y=(*cancel_info).y;
  XGetWidgetInfo(GrabButtonText,grab_info);
  (*grab_info).width=width;
  (*grab_info).height=(unsigned int) ((3*height) >> 1);
  (*grab_info).x=QuantumMargin;
  (*grab_info).y=((5*QuantumMargin) >> 1)+(int) height;
  XGetWidgetInfo(ResetButtonText,reset_info);
  (*reset_info).width=width;
  (*reset_info).height=(unsigned int) ((3*height) >> 1);
  (*reset_info).x=QuantumMargin;
  (*reset_info).y=(*grab_info).y+(int) (*grab_info).height+QuantumMargin;
}

static void InitializeColorBrowserReply(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height,
    width = context->width;

  char
    *const reply = context->reply;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const action_info = widgets->action_info,
    *const mode_info = widgets->mode_info;

  /*
    Initialize reply information.
  */
  XGetWidgetInfo(reply,reply_info);
  (*reply_info).raised=MagickFalse;
  (*reply_info).bevel_width--;
  (*reply_info).width=windows->widget.width-width-(unsigned int)
    ((6*QuantumMargin) >> 1);
  (*reply_info).height=height << 1;
  (*reply_info).x=(int) width+(QuantumMargin << 1);
  (*reply_info).y=(*action_info).y-(int) (*reply_info).height-QuantumMargin;
  /*
    Initialize mode information.
  */
  XGetWidgetInfo((char *) NULL,mode_info);
  (*mode_info).active=MagickTrue;
  (*mode_info).bevel_width=0;
  (*mode_info).width=(unsigned int) ((*action_info).x-
    (int) (QuantumMargin << 1));
  (*mode_info).height=(*action_info).height;
  (*mode_info).x=QuantumMargin;
  (*mode_info).y=(*action_info).y;
}

static void InitializeColorBrowserScroll(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const grab_info = widgets->grab_info,
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const expose_info = widgets->expose_info;

  int
    id;

  /*
    Initialize scroll information.
  */
  XGetWidgetInfo((char *) NULL,scroll_info);
  (*scroll_info).bevel_width--;
  (*scroll_info).width=height;
  (*scroll_info).height=(unsigned int) ((*reply_info).y-(*grab_info).y-
    (QuantumMargin >> 1));
  (*scroll_info).x=(*reply_info).x+(int) ((*reply_info).width-
    (*scroll_info).width);
  (*scroll_info).y=(*grab_info).y-(int) (*reply_info).bevel_width;
  (*scroll_info).raised=MagickFalse;
  (*scroll_info).trough=MagickTrue;
  (*north_info)=(*scroll_info);
  (*north_info).raised=MagickTrue;
  (*north_info).width-=((*north_info).bevel_width << 1);
  (*north_info).height=(*north_info).width-1;
  (*north_info).x+=(int) (*north_info).bevel_width;
  (*north_info).y+=(int) (*north_info).bevel_width;
  (*south_info)=(*north_info);
  (*south_info).y=(*scroll_info).y+(int) (*scroll_info).height-(int)
    (*scroll_info).bevel_width-(int) (*south_info).height;
  id=(*slider_info).id;
  (*slider_info)=(*north_info);
  (*slider_info).id=id;
  (*slider_info).width-=2;
  (*slider_info).min_y=(*north_info).y+(int) (*north_info).height+(int)
    (*north_info).bevel_width+(int) (*slider_info).bevel_width+2;
  (*slider_info).height=(unsigned int) ((int) (*scroll_info).height-
    (((*slider_info).min_y-(*scroll_info).y+1) << 1)+4);
  (*visible_colors)=(unsigned int) ((*scroll_info).height*
    MagickSafeReciprocal((double) height+(height >> 3)));
  if ((*colors) > (*visible_colors))
    (*slider_info).height=(unsigned int) (((*visible_colors)*
      (*slider_info).height)/(*colors));
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-
    (int) (*slider_info).bevel_width-2;
  (*slider_info).x=(*scroll_info).x+(int) (*slider_info).bevel_width+1;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info)=(*scroll_info);
  (*expose_info).y=(*slider_info).y;
}

static void InitializeColorBrowserList(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XWindows
    *const windows = context->windows;

  XFontStruct
    *const font_info = context->font_info;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info,
    *const list_info = widgets->list_info;

  /*
    Initialize list information.
  */
  XGetWidgetInfo((char *) NULL,list_info);
  (*list_info).raised=MagickFalse;
  (*list_info).bevel_width--;
  (*list_info).width=(unsigned int)
    ((*scroll_info).x-(*reply_info).x-(int) (QuantumMargin >> 1));
  (*list_info).height=(*scroll_info).height;
  (*list_info).x=(*reply_info).x;
  (*list_info).y=(*scroll_info).y;
  if (windows->widget.mapped == MagickFalse)
    (*state)|=JumpListState;
}

static void InitializeColorBrowserText(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  char
    *const text = context->text;

  XWidgetInfo
    *const reply_info = widgets->reply_info,
    *const list_info = widgets->list_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  /*
    Initialize text information.
  */
  *text='\0';
  XGetWidgetInfo(text,text_info);
  (*text_info).center=MagickFalse;
  (*text_info).width=(*reply_info).width;
  (*text_info).height=height;
  (*text_info).x=(*list_info).x-(int) (QuantumMargin >> 1);
  (*text_info).y=QuantumMargin;
  /*
    Initialize selection information.
  */
  XGetWidgetInfo((char *) NULL,selection_info);
  (*selection_info).center=MagickFalse;
  (*selection_info).width=(*list_info).width;
  (*selection_info).height=(unsigned int) ((9*height) >> 3);
  (*selection_info).x=(*list_info).x;
}

static void UpdateColorBrowserLayout(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  size_t
    *const state = widgets->state;

  if ((*state) & UpdateConfigurationState)
    {
      InitializeColorBrowserButtons(context,widgets);
      InitializeColorBrowserReply(context,widgets);
      InitializeColorBrowserScroll(context,widgets);
      InitializeColorBrowserList(context,widgets);
      InitializeColorBrowserText(context,widgets);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
}

static void RedrawColorBrowserWidget(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    height = context->height;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const grab_info = widgets->grab_info,
    *const reset_info = widgets->reset_info,
    *const action_info = widgets->action_info,
    *const cancel_info = widgets->cancel_info,
    *const slider_info = widgets->slider_info,
    *const scroll_info = widgets->scroll_info,
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  int
    x,
    y;

  if ((*state) & RedrawWidgetState)
    {
      /*
        Redraw Color Browser window.
      */
      x=QuantumMargin;
      y=(*text_info).y+(int) (((*text_info).height-height) >> 1)+
        font_info->ascent;
      (void) XDrawString(display,windows->widget.id,
        windows->widget.annotate_context,x,y,ColorPatternText,
        Extent(ColorPatternText));
      (void) CopyMagickString((*text_info).text,glob_pattern,MagickPathExtent);
      XDrawWidgetText(display,&windows->widget,text_info);
      XDrawBeveledButton(display,&windows->widget,grab_info);
      XDrawBeveledButton(display,&windows->widget,reset_info);
      XDrawBeveledMatte(display,&windows->widget,list_info);
      XDrawBeveledMatte(display,&windows->widget,scroll_info);
      XDrawTriangleNorth(display,&windows->widget,north_info);
      XDrawBeveledButton(display,&windows->widget,slider_info);
      XDrawTriangleSouth(display,&windows->widget,south_info);
      x=QuantumMargin;
      y=(*reply_info).y+(int) (((*reply_info).height-height) >> 1)+
        font_info->ascent;
      (void) XDrawString(display,windows->widget.id,
        windows->widget.annotate_context,x,y,ColornameText,
        Extent(ColornameText));
      XDrawBeveledMatte(display,&windows->widget,reply_info);
      XDrawMatteText(display,&windows->widget,reply_info);
      XDrawBeveledButton(display,&windows->widget,action_info);
      XDrawBeveledButton(display,&windows->widget,cancel_info);
      XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
      (*selection_info).id=(~0);
      (*state)|=RedrawActionState;
      (*state)|=RedrawListState;
      (*state)&=(unsigned int) (~RedrawWidgetState);
    }
}

static void JumpColorBrowserList(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  char
    *const reply = context->reply;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  char
    ***const colorlist = widgets->colorlist;

  XWidgetInfo
    *const list_info = widgets->list_info,
    *const selection_info = widgets->selection_info;

  int
    i;

  if ((*state) & JumpListState)
    {
      FontBrowserMatch
        match;

      /*
        Jump scroll to match user color.
      */
      (*list_info).id=(~0);
      match.fontlist=(*colorlist);
      match.fonts=(int) (*colors);
      match.reply=reply;
      MatchFontBrowserReply(&match,list_info,&i);
      if ((i < (*slider_info).id) ||
          (i >= (int) ((*slider_info).id+(int) (*visible_colors))))
        (*slider_info).id=i-(int) ((*visible_colors) >> 1);
      (*selection_info).id=(~0);
      (*state)|=RedrawListState;
      (*state)&=(unsigned int) (~JumpListState);
    }
}

static void RedrawColorBrowserList(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    height = context->height;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info;

  char
    ***const colorlist = widgets->colorlist;

  XWidgetInfo
    *const list_info = widgets->list_info,
    *const expose_info = widgets->expose_info,
    *const selection_info = widgets->selection_info;

  int
    i;

  if ((*state) & RedrawListState)
    {
      /*
        Determine slider id and position.
      */
      if ((*slider_info).id >= (int) ((*colors)-(*visible_colors)))
        (*slider_info).id=(int) ((*colors)-(*visible_colors));
      if (((*slider_info).id < 0) || ((*colors) <= (*visible_colors)))
        (*slider_info).id=0;
      (*slider_info).y=(*slider_info).min_y;
      if ((*colors) != 0)
        (*slider_info).y+=(int) (*slider_info).id*((*slider_info).max_y-
          (*slider_info).min_y+1)/(int) (*colors);
      if ((*slider_info).id != (*selection_info).id)
        {
          FontBrowserNames
            names;

          /*
            Redraw scroll bar and file names.
          */
          (*selection_info).id=(*slider_info).id;
          (*selection_info).y=(*list_info).y+(int) (height >> 3)+2;
          names.display=display;
          names.windows=windows;
          names.fontlist=(*colorlist);
          names.fonts=(int) (*colors);
          names.visible_fonts=(*visible_colors);
          names.list_info=(*list_info);
          names.slider_info=(*slider_info);
          DrawFontBrowserNames(&names,selection_info,&i);
          /*
            Update slider.
          */
          (*expose_info)=PlaceTextViewExpose((*expose_info),(*slider_info));
          XDrawTriangleNorth(display,&windows->widget,north_info);
          XDrawMatte(display,&windows->widget,expose_info);
          XDrawBeveledButton(display,&windows->widget,slider_info);
          XDrawTriangleSouth(display,&windows->widget,south_info);
          (*expose_info).y=(*slider_info).y;
        }
      (*state)&=(unsigned int) (~RedrawListState);
    }
}

static void RedrawColorBrowserAction(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const mode_info = widgets->mode_info;

  XColor
    color;

  if ((*state) & RedrawActionState)
    {
      static char
        colorname[MagickPathExtent];

      /*
        Display the selected color in a drawing area.
      */
      color=windows->widget.pixel_info->matte_color;
      (void) XParseColor(display,windows->widget.map_info->colormap,
        (*reply_info).text,&windows->widget.pixel_info->matte_color);
      XBestPixel(display,windows->widget.map_info->colormap,(XColor *) NULL,
        (unsigned int) windows->widget.visual_info->colormap_size,
        &windows->widget.pixel_info->matte_color);
      (*mode_info).text=colorname;
      (void) FormatLocaleString((*mode_info).text,MagickPathExtent,
        "#%02x%02x%02x",windows->widget.pixel_info->matte_color.red,
        windows->widget.pixel_info->matte_color.green,
        windows->widget.pixel_info->matte_color.blue);
      XDrawBeveledButton(display,&windows->widget,mode_info);
      windows->widget.pixel_info->matte_color=color;
      (*state)&=(unsigned int) (~RedrawActionState);
    }
}

static MagickBooleanType ReloadColorList(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern,
    *const reset_pattern = context->reset_pattern;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const reply = context->reply;

  XWidgetInfo
    *const action_info = widgets->action_info;

  size_t
    *const colors = widgets->colors;

  char
    ***const colorlist = widgets->colorlist;

  const ColorBrowserUpdateTargets
    update_targets = { action_info, colorlist, colors };

  ColorBrowserUpdate
    update;

  update.display=display;
  update.windows=windows;
  update.reply=reply;
  update.glob_pattern=glob_pattern;
  update.reset_pattern=reset_pattern;
  update.exception=exception;
  if (UpdateColorBrowserList(&update,&update_targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void UpdateColorBrowserSlider(const ColorBrowserContext *context,
  const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const glob_pattern = context->glob_pattern;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const scroll_info = widgets->scroll_info;

  unsigned int
    *const visible_colors = widgets->visible_colors;

  XWidgetInfo
    *const north_info = widgets->north_info,
    *const south_info = widgets->south_info,
    *const list_info = widgets->list_info,
    *const expose_info = widgets->expose_info,
    *const text_info = widgets->text_info,
    *const selection_info = widgets->selection_info;

  /*
    Sort color list in ascending order.
  */
  (*slider_info).height=(unsigned int) ((int) (*scroll_info).height-
    (((*slider_info).min_y-(*scroll_info).y+1) << 1)+1);
  if ((*colors) > (*visible_colors))
    (*slider_info).height=(unsigned int) (((*visible_colors)*
      (*slider_info).height)/(*colors));
  (*slider_info).max_y=(*south_info).y-(int) (*south_info).bevel_width-
    (int) (*slider_info).bevel_width-2;
  (*slider_info).id=0;
  (*slider_info).y=(*slider_info).min_y;
  (*expose_info).y=(*slider_info).y;
  (*selection_info).id=(~0);
  (*list_info).id=(~0);
  (*state)|=RedrawListState;
  /*
    Redraw color name & reply.
  */
  *(*reply_info).text='\0';
  (*reply_info).cursor=(*reply_info).text;
  (void) CopyMagickString((*text_info).text,glob_pattern,MagickPathExtent);
  XDrawWidgetText(display,&windows->widget,text_info);
  XDrawMatteText(display,&windows->widget,reply_info);
  XDrawBeveledMatte(display,&windows->widget,scroll_info);
  XDrawTriangleNorth(display,&windows->widget,north_info);
  XDrawBeveledButton(display,&windows->widget,slider_info);
  XDrawTriangleSouth(display,&windows->widget,south_info);
  XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
  (*state)&=(unsigned int) (~UpdateListState);
}

static MagickBooleanType WaitColorEvent(const ColorBrowserContext *context,
  XEvent *event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  XWidgetInfo
    *const slider_info = widgets->slider_info;

  size_t
    *const colors = widgets->colors;

  XWidgetInfo
    *const north_info = widgets->north_info;

  size_t
    *const delay = widgets->delay;

  XWidgetInfo
    *const south_info = widgets->south_info;

  if ((*north_info).raised && (*south_info).raised)
    (void) XIfEvent(display,event,XScreenEvent,(char *) windows);
  else
    {
      /*
        Brief delay before advancing scroll bar.
      */
      XDelay(display,(*delay));
      (*delay)=SuspendTime;
      (void) XCheckIfEvent(display,event,XScreenEvent,(char *) windows);
      if (((*north_info).raised == MagickFalse) && ((*slider_info).id > 0))
        {
          /*
            Move slider up.
          */
          (*slider_info).id--;
          (*state)|=RedrawListState;
        }
      if (((*south_info).raised == MagickFalse) &&
          ((*slider_info).id < (int) (*colors)))
        {
          /*
            Move slider down.
          */
          (*slider_info).id++;
          (*state)|=RedrawListState;
        }
      if ((*event).type != ButtonRelease)
        return(MagickTrue);
    }
  return(MagickFalse);
}

static void HandleColorSelectionEvent(const ColorBrowserContext *context,
  XEvent event,const ColorBrowserTargets *widgets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  char
    *const primary_selection = context->primary_selection;

  XWidgetInfo
    *const reply_info = widgets->reply_info;

  switch (event.type)
  {
    case SelectionClear:
    {
      (*reply_info).highlight=MagickFalse;
      XDrawMatteText(display,&windows->widget,reply_info);
      break;
    }
    case SelectionNotify:
    {
      SelectColorBrowserReply(context,event,widgets);
      break;
    }
    case SelectionRequest:
    {
      XSelectionEvent
        notify;

      XSelectionRequestEvent
        *request;

      if ((*reply_info).highlight == MagickFalse)
        break;
      /*
        Set primary selection.
      */
      request=(&(event.xselectionrequest));
      (void) XChangeProperty(request->display,request->requestor,
        request->property,request->target,8,PropModeReplace,
        (unsigned char *) primary_selection,Extent(primary_selection));
      notify.type=SelectionNotify;
      notify.send_event=MagickTrue;
      notify.display=request->display;
      notify.requestor=request->requestor;
      notify.selection=request->selection;
      notify.target=request->target;
      notify.time=request->time;
      if (request->property == None)
        notify.property=request->target;
      else
        notify.property=request->property;
      (void) XSendEvent(request->display,request->requestor,False,
        NoEventMask,(XEvent *) &notify);
    }
    default:
      break;
  }
}

static void HandleColorKeyEvent(const ColorBrowserContext *context,
  const XEvent event,const ColorBrowserTargets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  MagickStatusType
    *const mask = widgets->mask;

  switch (event.type)
  {
    case KeyPress:
    {
      PressColorBrowserKey(context,event,widgets);
      break;
    }
    case KeyRelease:
    {
      (*state)=ReleaseWidgetKey(windows,event,(*state));
      break;
    }
    case LeaveNotify:
    {
      (*state)=LeaveWidgetWindow(windows,event,(*state));
      break;
    }
    case MapNotify:
    {
      (*mask)&=(unsigned int) (~CWX);
      (*mask)&=(unsigned int) (~CWY);
      break;
    }
    case MotionNotify:
    {
      MoveColorBrowserButtons(context,event,widgets);
      break;
    }
    default:
    {
      HandleColorSelectionEvent(context,event,widgets);
      break;
    }
  }
}

static void HandleColorEvent(const ColorBrowserContext *context,
  const XEvent event,const FontBrowserTargets *targets,
  const ColorBrowserTargets *widgets)
{
  XWindows
    *const windows = context->windows;

  size_t
    *const state = widgets->state;

  switch (event.type)
  {
    case ButtonPress:
    {
      PressColorBrowserScroll(context,event,targets,widgets);
      break;
    }
    case ButtonRelease:
    {
      ReleaseColorBrowserScroll(context,event,widgets);
      break;
    }
    case ClientMessage:
    {
      ClientColorBrowserMessage(context,event,widgets);
      break;
    }
    case ConfigureNotify:
    {
      (*state)=ConfigureWidgetWindow(windows,event,(*state));
      break;
    }
    case EnterNotify:
    {
      (*state)=EnterWidgetWindow(windows,event,(*state));
      break;
    }
    case Expose:
    {
      (*state)=ExposeWidgetWindow(windows,event,(*state));
      break;
    }
    default:
    {
      HandleColorKeyEvent(context,event,widgets);
      break;
    }
  }
}

static void RespondToColorBrowserEvents(const ColorBrowserContext *context,
  const FontBrowserTargets *targets,const ColorBrowserTargets *widgets)
{
  size_t
    *const state = widgets->state;

  XEvent
    event;

  do
  {
    UpdateColorBrowserLayout(context,widgets);
    RedrawColorBrowserWidget(context,widgets);
    if ((*state) & UpdateListState)
      {
        if (ReloadColorList(context,widgets) == MagickFalse)
          break;
        UpdateColorBrowserSlider(context,widgets);
      }
    JumpColorBrowserList(context,widgets);
    RedrawColorBrowserList(context,widgets);
    RedrawColorBrowserAction(context,widgets);
    /*
      Wait for next event.
    */
    if (WaitColorEvent(context,&event,widgets) != MagickFalse)
      continue;
    HandleColorEvent(context,event,targets,widgets);
  } while (((*state) & ExitState) == 0);
}

static void CheckColorBrowserReply(Display *display,XWindows *windows,
  char *reply)
{
  Status
    status;

  XColor
    color;

  if ((*reply == '\0') || (strchr(reply,'-') != (char *) NULL))
    return;
  status=XParseColor(display,windows->widget.map_info->colormap,reply,&color);
  if (status != False)
    return;
  XNoticeWidget(display,windows,"Color is unknown to X server:",reply);
  (void) CopyMagickString(reply,"gray",MagickPathExtent);
}

static void FreeColorBrowserList(char **colorlist,size_t colors,
  ExceptionInfo *exception)
{
  int
    i;

  /*
    Free color list.
  */
  for (i=0; i < (int) colors; i++)
    colorlist[i]=DestroyString(colorlist[i]);
  if (colorlist != (char **) NULL)
    colorlist=(char **) RelinquishMagickMemory(colorlist);
  exception=DestroyExceptionInfo(exception);
}

static void MapColorBrowserWidget(Display *display,XWindows *windows,
  MagickStatusType mask)
{
  Status
    status;

  XTextProperty
    window_name;

  XWindowChanges
    window_changes;

  (void) CopyMagickString(windows->widget.name,"Browse and Select a Color",
    MagickPathExtent);
  status=XStringListToTextProperty(&windows->widget.name,1,&window_name);
  if (status != False)
    {
      XSetWMName(display,windows->widget.id,&window_name);
      XSetWMIconName(display,windows->widget.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  window_changes.width=(int) windows->widget.width;
  window_changes.height=(int) windows->widget.height;
  window_changes.x=windows->widget.x;
  window_changes.y=windows->widget.y;
  (void) XReconfigureWMWindow(display,windows->widget.id,windows->widget.screen,
    mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
}

static unsigned int WidestColorBrowserName(XFontStruct *font_info,
  char **colorlist,size_t colors)
{
  int
    i;

  unsigned int
    text_width;

  text_width=0;
  for (i=0; i < (int) colors; i++)
    if (WidgetTextWidth(font_info,colorlist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,colorlist[i]);
  return(text_width);
}

static void RunColorBrowser(const ColorBrowserContext *context,
  char ***colorlist,size_t *colors,MagickStatusType *mask)
{
  unsigned int
    visible_colors;

  size_t
    delay,
    state;

  XWidgetInfo
    action_info,
    cancel_info,
    expose_info,
    grab_info,
    list_info,
    mode_info,
    north_info,
    reply_info,
    reset_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info;

  const FontBrowserTargets
    targets = { &reply_info, &state, &selection_info, &list_info };

  const ColorBrowserTargets
    color_widgets = { &reply_info, &state,
      &grab_info, &reset_info, &action_info, &cancel_info, &slider_info,
      colors, &scroll_info, &visible_colors, &north_info, &delay, &south_info,
      &mode_info, colorlist, &list_info, &expose_info, &text_info,
      &selection_info, mask };

  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&mode_info);
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_colors=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  RespondToColorBrowserEvents(context,&targets,&color_widgets);
}

typedef struct _ColorBrowserLayout
{
  Display
    *display;

  XFontStruct
    *font_info;

  unsigned int
    width,
    height,
    text_width;
} ColorBrowserLayout;

static void PositionColorBrowserWidget(XWindows *windows,
  const ColorBrowserLayout *layout)
{
  Display
    *const display = layout->display;

  XFontStruct
    *const font_info = layout->font_info;

  const unsigned int
    width = layout->width,
    height = layout->height,
    text_width = layout->text_width;

  /*
    Position Color Browser widget.
  */
  windows->widget.width=width+MagickMin(text_width,MaxTextWidth)+
    6*(unsigned int) QuantumMargin;
  windows->widget.min_width=width+MinTextWidth+4*(unsigned int) QuantumMargin;
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    ((81*height) >> 2)+((13*(unsigned int) QuantumMargin) >> 1)+4;
  windows->widget.min_height=(unsigned int)
    (((23*height) >> 1)+((13*(unsigned int) QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
}

static void WithdrawBrowserWidget(Display *display,XWindows *windows)
{
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
}

static unsigned int GetColorBrowserWidth(XFontStruct *font_info,
  const char *action)
{
  unsigned int
    width;

  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,ResetButtonText) > width)
    width=WidgetTextWidth(font_info,ResetButtonText);
  if (WidgetTextWidth(font_info,GrabButtonText) > width)
    width=WidgetTextWidth(font_info,GrabButtonText);
  width+=(unsigned int) QuantumMargin;
  if (WidgetTextWidth(font_info,ColorPatternText) > width)
    width=WidgetTextWidth(font_info,ColorPatternText);
  if (WidgetTextWidth(font_info,ColornameText) > width)
    width=WidgetTextWidth(font_info,ColornameText);
  return(width);
}

#endif
