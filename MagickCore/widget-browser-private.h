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

  MagickCore X11 widget browser and arrow methods.
*/
#ifndef MAGICKCORE_WIDGET_BROWSER_PRIVATE_H
#define MAGICKCORE_WIDGET_BROWSER_PRIVATE_H

#include "MagickCore/widget-core-private.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_X11_DELEGATE)

/*
  Define declarations.
*/
#define AreaIsActive(matte_info,position)  ( \
  ((position.y >= (matte_info.y-(int) matte_info.bevel_width)) &&  \
   (position.y < (matte_info.y+(int) matte_info.height+(int) matte_info.bevel_width))) \
   ? MagickTrue : MagickFalse)
#define MaxTextWidth  ((unsigned int) (255*XTextWidth(font_info,"_",1)))
#define MinTextWidth  ((unsigned int) (26*XTextWidth(font_info,"_",1)))

/*
  Accessor declarations.
*/
extern MagickPrivate char
  (*ReleaseKeyCommandReference(void))[MagickPathExtent];

extern MagickPrivate KeySym
  *ReleaseKeySymbolReference(void);

/*
  Typedef declarations.
*/
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

/*
  Constant declarations.
*/
static const int
  BorderOffset = 4,
  DoubleClick = 250;

/*
  Method prototypes.
*/
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

static XWidgetInfo
  TrackFontBrowserSlider(XWidgetInfo,const XEvent,const int);

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
static inline void XDrawTriangleEastLabel(Display *display,
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

static inline void XDrawTriangleEast(Display *display,const XWindowInfo *window_info,
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
static inline void XDrawTriangleNorth(Display *display,const XWindowInfo *window_info,
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
static inline void XDrawTriangleSouth(Display *display,const XWindowInfo *window_info,
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

static inline size_t ReleaseWidgetKey(XWindows *windows,const XEvent event,
  size_t state)
{
  /*
    Respond to a user key release.
  */
  if (event.xkey.window != windows->widget.id)
    return(state);
  (void) XLookupString((XKeyEvent *) &event.xkey,(*ReleaseKeyCommandReference()),
    (int) sizeof((*ReleaseKeyCommandReference())),&(*ReleaseKeySymbolReference()),(XComposeStatus *) NULL);
  if ((*ReleaseKeySymbolReference()) == XK_Control_L)
    state&=(unsigned int) (~ControlState);
  return(state);
}

static inline void WithdrawBrowserWidget(Display *display,XWindows *windows)
{
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
}

static inline void SelectFontBrowserEntry(const FontBrowserPress *press,
  char *glob_pattern,const FontBrowserTargets *targets)
{
  Display
    *const display = press->display;

  XWindows
    *const windows = press->windows;

  char
    **const fontlist = press->fontlist;

  const int
    fonts = press->fonts;

  const unsigned int
    height = press->height;

  const XEvent
    event = press->event;

  const XWidgetInfo
    slider_info = press->slider_info;

  XWidgetInfo
    *const reply_info = targets->reply_info;

  size_t
    *const state = targets->state;

  XWidgetInfo
    *const selection_info = targets->selection_info,
    *const list_info = targets->list_info;

  int
    id;

  /*
    User pressed list matte.
  */
  id=slider_info.id+(event.xbutton.y-((*list_info).y+(int)
    (height >> 1))+1)/(int) (*selection_info).height;
  if (id >= (int) fonts)
    return;
  (void) CopyMagickString((*reply_info).text,fontlist[id],MagickPathExtent);
  (*reply_info).highlight=MagickFalse;
  (*reply_info).marker=(*reply_info).text;
  (*reply_info).cursor=(*reply_info).text+Extent((*reply_info).text);
  XDrawMatteText(display,&windows->widget,reply_info);
  (*state)|=RedrawActionState;
  if (id == (*list_info).id)
    {
      (void) CopyMagickString(glob_pattern,(*reply_info).text,
        MagickPathExtent);
      (*state)|=UpdateListState;
    }
  (*selection_info).id=(~0);
  (*list_info).id=id;
  (*state)|=RedrawListState;
}

static inline void DrawFontBrowserNames(const FontBrowserNames *names,
  XWidgetInfo *selection_info,int *i)
{
  Display
    *const display = names->display;

  XWindows
    *const windows = names->windows;

  char
    **const fontlist = names->fontlist;

  const int
    fonts = names->fonts;

  const unsigned int
    visible_fonts = names->visible_fonts;

  const XWidgetInfo
    list_info = names->list_info,
    slider_info = names->slider_info;

  for ((*i)=0; (*i) < (int) visible_fonts; (*i)++)
  {
    (*selection_info).raised=(slider_info.id+(*i)) != list_info.id ?
      MagickTrue : MagickFalse;
    (*selection_info).text=(char *) NULL;
    if ((slider_info.id+(*i)) < fonts)
      (*selection_info).text=fontlist[slider_info.id+(*i)];
    XDrawWidgetText(display,&windows->widget,selection_info);
    (*selection_info).y+=(int) (*selection_info).height;
  }
}

static inline XWidgetInfo TrackFontBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const int fonts)
{
  slider_info.y=event.xmotion.y-(int)
    ((slider_info.height+slider_info.bevel_width) >> 1)+1;
  if (slider_info.y < slider_info.min_y)
    slider_info.y=slider_info.min_y;
  if (slider_info.y > slider_info.max_y)
    slider_info.y=slider_info.max_y;
  slider_info.id=0;
  if (slider_info.y != slider_info.min_y)
    slider_info.id=(fonts*(slider_info.y-slider_info.min_y+1))/
      (slider_info.max_y-slider_info.min_y+1);
  return(slider_info);
}

static inline void MoveFontBrowserSliderByPage(const KeySym key_symbol,
  const unsigned int visible_fonts,const int fonts,XWidgetInfo *slider_info)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_KP_Prior:
    {
      (*slider_info).id-=(int) visible_fonts;
      break;
    }
    case XK_Next:
    case XK_KP_Next:
    {
      (*slider_info).id+=(int) visible_fonts;
      break;
    }
    case XK_End:
    case XK_KP_End:
    {
      (*slider_info).id=fonts;
      break;
    }
  }
}

static inline XWidgetInfo MoveFontBrowserSlider(XWidgetInfo slider_info,
  const KeySym key_symbol,const unsigned int visible_fonts,const int fonts)
{
  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      slider_info.id=0;
      break;
    }
    case XK_Up:
    case XK_KP_Up:
    {
      slider_info.id--;
      break;
    }
    case XK_Down:
    case XK_KP_Down:
    {
      slider_info.id++;
      break;
    }
    default:
    {
      MoveFontBrowserSliderByPage(key_symbol,visible_fonts,fonts,
        &slider_info);
      break;
    }
  }
  return(slider_info);
}

static inline size_t ReleaseFontBrowserBack(const FontBrowserBack *back,
  size_t state)
{
  XWindows
    *const windows = back->windows;

  const XEvent
    event = back->event;

  const XWidgetInfo
    back_info = back->back_info;

  char
    *const glob_pattern = back->glob_pattern;

  const char
    *const back_pattern = back->back_pattern;

  if ((event.xbutton.window == windows->widget.id) &&
      (MatteIsActive(back_info,event.xbutton)))
    {
      (void) CopyMagickString(glob_pattern,back_pattern,
        MagickPathExtent);
      state|=UpdateListState;
    }
  return(state);
}

static inline void MatchFontBrowserReply(const FontBrowserMatch *match,
  XWidgetInfo *list_info,int *i)
{
  char
    **const fontlist = match->fontlist;

  const int
    fonts = match->fonts;

  char
    *const reply = match->reply;

  for ((*i)=0; (*i) < fonts; (*i)++)
    if (LocaleCompare(fontlist[(*i)],reply) >= 0)
      {
        (*list_info).id=LocaleCompare(fontlist[(*i)],reply) == 0 ? (*i) : ~0;
        break;
      }
}

static inline XWidgetInfo PageListBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const unsigned int visible_entries)
{
  if (event.xbutton.y < slider_info.y)
    slider_info.id-=(int) (visible_entries-1);
  else
    slider_info.id+=(int) (visible_entries-1);
  return(slider_info);
}

static inline int FindListBrowserCursor(XFontStruct *font_info,
  const XWidgetInfo reply_info,const int x)
{
  int
    i;

  for (i=1; i <= Extent(reply_info.marker); i++)
    if (XTextWidth(font_info,reply_info.marker,i) > x)
      break;
  return(i);
}

static inline XWidgetInfo HighlightListBrowserReply(const ListBrowserClick *click,
  char *primary_selection,XWidgetInfo reply_info)
{
  Display
    *const display = click->display;

  XWindowInfo
    *const window_info = click->window_info;

  const XEvent
    event = click->event;

  const Time
    click_time = click->click_time;

  if (event.xbutton.time > (click_time+(unsigned long) DoubleClick))
    reply_info.highlight=MagickFalse;
  else
    {
      /*
        Become the XA_PRIMARY selection owner.
      */
      (void) CopyMagickString(primary_selection,reply_info.text,
        MagickPathExtent);
      (void) XSetSelectionOwner(display,XA_PRIMARY,window_info->id,
        event.xbutton.time);
      reply_info.highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
        window_info->id ? MagickTrue : MagickFalse;
    }
  return(reply_info);
}

static inline size_t ConfirmListBrowserReply(Display *display,
  const XWidgetInfo reply_info,size_t state)
{
  if (*reply_info.text == '\0')
    (void) XBell(display,0);
  else
    state|=ExitState;
  return(state);
}

static inline void ReleaseListBrowserAction(const ListBrowserRelease *release,
  const XWidgetInfo reply_info,XWidgetInfo *action_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindowInfo
    *const window_info = release->window_info;

  const XEvent
    event = release->event;

  if ((*action_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == window_info->id) &&
          (MatteIsActive((*action_info),event.xbutton)))
        {
          (*state)=ConfirmListBrowserReply(display,reply_info,(*state));
        }
      (*action_info).raised=MagickTrue;
      XDrawBeveledButton(display,window_info,&(*action_info));
    }
}

static inline void ReleaseListBrowserCancel(const ListBrowserRelease *release,
  const XWidgetInfo reply_info,XWidgetInfo *cancel_info,size_t *state)
{
  Display
    *const display = release->display;

  XWindowInfo
    *const window_info = release->window_info;

  const XEvent
    event = release->event;

  if ((*cancel_info).raised == MagickFalse)
    {
      if ((event.xbutton.window == window_info->id) &&
          (MatteIsActive((*cancel_info),event.xbutton)))
        {
          *reply_info.text='\0';
          (*state)|=ExitState;
        }
      (*cancel_info).raised=MagickTrue;
      XDrawBeveledButton(display,window_info,&(*cancel_info));
    }
}

static inline XWidgetInfo ApplyListBrowserControlKey(XWidgetInfo reply_info,
  const KeySym key_symbol,const size_t state)
{
  if (state & ControlState)
    switch ((int) key_symbol)
    {
      case XK_u:
      case XK_U:
      {
        /*
          Erase the entire line of text.
        */
        *reply_info.text='\0';
        reply_info.cursor=reply_info.text;
        reply_info.marker=reply_info.text;
        reply_info.highlight=MagickFalse;
        break;
      }
      default:
        break;
    }
  return(reply_info);
}

static inline XWidgetInfo TrackListBrowserSlider(XWidgetInfo slider_info,
  const XEvent event,const unsigned int entries)
{
  slider_info.y=event.xmotion.y-(int)
    ((slider_info.height+slider_info.bevel_width) >> 1)+1;
  if (slider_info.y < slider_info.min_y)
    slider_info.y=slider_info.min_y;
  if (slider_info.y > slider_info.max_y)
    slider_info.y=slider_info.max_y;
  slider_info.id=0;
  if (slider_info.y != slider_info.min_y)
    slider_info.id=((int) entries*(slider_info.y-
      slider_info.min_y+1))/(slider_info.max_y-slider_info.min_y+1);
  return(slider_info);
}

static inline XWidgetInfo PlaceTextViewExpose(XWidgetInfo expose_info,
  const XWidgetInfo slider_info)
{
  if (slider_info.y > expose_info.y)
    {
      expose_info.height=(unsigned int) (slider_info.y-expose_info.y);
      expose_info.y=slider_info.y-(int) expose_info.height-(int)
        slider_info.bevel_width-1;
    }
  else
    {
      expose_info.height=(unsigned int) (expose_info.y-slider_info.y);
      expose_info.y=slider_info.y+(int) slider_info.height+(int)
        slider_info.bevel_width+1;
    }
  return(expose_info);
}

#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
