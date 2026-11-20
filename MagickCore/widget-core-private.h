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

  MagickCore X11 widget core methods.
*/
#ifndef MAGICKCORE_WIDGET_CORE_PRIVATE_H
#define MAGICKCORE_WIDGET_CORE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(MAGICKCORE_X11_DELEGATE)

/*
  Define declarations.
*/
#define Extent(s)  ((int) strlen(s))
#define MatteIsActive(matte_info,position)  ( \
  ((position.x >= (matte_info.x-(int) matte_info.bevel_width)) && \
   (position.y >= (matte_info.y-(int) matte_info.bevel_width)) &&  \
   (position.x < (matte_info.x+(int) matte_info.width+(int) matte_info.bevel_width)) &&  \
   (position.y < (matte_info.y+(int) matte_info.height+(int) matte_info.bevel_width))) \
   ? MagickTrue : MagickFalse)
#define QuantumMargin   MagickMax(font_info->max_bounds.width,12)
#define WidgetTextWidth(font_info,text)  \
  ((unsigned int) XTextWidth(font_info,text,Extent(text)))

/*
  Typedef declarations.
*/
typedef enum
{
  ControlState = 0x0001,
  InactiveWidgetState = 0x0004,
  JumpListState = 0x0008,
  RedrawActionState = 0x0010,
  RedrawListState = 0x0020,
  RedrawWidgetState = 0x0040,
  UpdateListState = 0x0100
} WidgetState;

typedef struct _XWidgetInfo
{
  char
    *cursor,
    *text,
    *marker;

  int
    id;

  unsigned int
    bevel_width,
    width,
    height;

  int
    x,
    y,
    min_y,
    max_y;

  MagickStatusType
    raised,
    active,
    center,
    trough,
    highlight;
} XWidgetInfo;

typedef struct _DialogRelease
{
  Display
    *display;

  XWindows
    *windows;

  XEvent
    event;
} DialogRelease;

/*
  Accessor declarations.
*/
extern MagickPrivate const int
  *BorderOffsetReference(void),
  *DoubleClickReference(void);

/*
  Method prototypes.
*/
static void
  XDrawMatte(Display *,const XWindowInfo *,const XWidgetInfo *),
  XSetBevelColor(Display *,const XWindowInfo *,const MagickStatusType),
  XSetMatteColor(Display *,const XWindowInfo *,const MagickStatusType),
  XSetTextColor(Display *,const XWindowInfo *,const MagickStatusType);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w B e v e l                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawBevel() "sets off" an area with a highlighted upper and left bevel and
%  a shadowed lower and right bevel.  The highlighted and shadowed bevels
%  create a 3-D effect.
%
%  The format of the XDrawBevel function is:
%
%      XDrawBevel(display,window_info,bevel_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o bevel_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the bevel.
%
*/
static inline void XDrawBevel(Display *display,const XWindowInfo *window_info,
  const XWidgetInfo *bevel_info)
{
  int
    x1,
    x2,
    y1,
    y2;

  unsigned int
    bevel_width;

  XPoint
    points[6];

  /*
    Draw upper and left beveled border.
  */
  x1=bevel_info->x;
  y1=bevel_info->y+(int) bevel_info->height;
  x2=bevel_info->x+(int) bevel_info->width;
  y2=bevel_info->y;
  bevel_width=bevel_info->bevel_width;
  points[0].x=x1;
  points[0].y=y1;
  points[1].x=x1;
  points[1].y=y2;
  points[2].x=x2;
  points[2].y=y2;
  points[3].x=x2+(int) bevel_width;
  points[3].y=y2-(int) bevel_width;
  points[4].x=x1-(int) bevel_width;
  points[4].y=y2-(int) bevel_width;
  points[5].x=x1-(int) bevel_width;
  points[5].y=y1+(int) bevel_width;
  XSetBevelColor(display,window_info,bevel_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,6,Complex,CoordModeOrigin);
  /*
    Draw lower and right beveled border.
  */
  points[0].x=x1;
  points[0].y=y1;
  points[1].x=x2;
  points[1].y=y1;
  points[2].x=x2;
  points[2].y=y2;
  points[3].x=x2+(int) bevel_width;
  points[3].y=y2-(int) bevel_width;
  points[4].x=x2+(int) bevel_width;
  points[4].y=y1+(int) bevel_width;
  points[5].x=x1-(int) bevel_width;
  points[5].y=y1+(int) bevel_width;
  XSetBevelColor(display,window_info,!bevel_info->raised);
  (void) XFillPolygon(display,window_info->id,window_info->widget_context,
    points,6,Complex,CoordModeOrigin);
  (void) XSetFillStyle(display,window_info->widget_context,FillSolid);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w B e v e l e d B u t t o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawBeveledButton() draws a button with a highlighted upper and left bevel
%  and a shadowed lower and right bevel.  The highlighted and shadowed bevels
%  create a 3-D effect.
%
%  The format of the XDrawBeveledButton function is:
%
%      XDrawBeveledButton(display,window_info,button_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o button_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the button.
%
*/

static inline void XDrawBeveledButton(Display *display,const XWindowInfo *window_info,
  const XWidgetInfo *button_info)
{
  int
    x,
    y;

  unsigned int
    width;

  XFontStruct
    *font_info;

  XRectangle
    crop_info;

  /*
    Draw matte.
  */
  XDrawBevel(display,window_info,button_info);
  XSetMatteColor(display,window_info,button_info->raised);
  (void) XFillRectangle(display,window_info->id,window_info->widget_context,
    button_info->x,button_info->y,button_info->width,button_info->height);
  x=button_info->x-(int) button_info->bevel_width-1;
  y=button_info->y-(int) button_info->bevel_width-1;
  (void) XSetForeground(display,window_info->widget_context,
    window_info->pixel_info->trough_color.pixel);
  if (button_info->raised || (window_info->depth == 1))
    (void) XDrawRectangle(display,window_info->id,window_info->widget_context,
      x,y,button_info->width+(button_info->bevel_width << 1)+1,
      button_info->height+(button_info->bevel_width << 1)+1);
  if (button_info->text == (char *) NULL)
    return;
  /*
    Set cropping region.
  */
  crop_info.width=(unsigned short) button_info->width;
  crop_info.height=(unsigned short) button_info->height;
  crop_info.x=button_info->x;
  crop_info.y=button_info->y;
  /*
    Draw text.
  */
  font_info=window_info->font_info;
  width=WidgetTextWidth(font_info,button_info->text);
  x=button_info->x+(int) (QuantumMargin >> 1);
  if (button_info->center)
    x=button_info->x+(int) (button_info->width >> 1)-(int) (width >> 1);
  y=button_info->y+(int) (((int) button_info->height-(int)
    (font_info->ascent+font_info->descent)) >> 1)+font_info->ascent;
  if ((int) button_info->width == (QuantumMargin >> 1))
    {
      /*
        Option button-- write label to right of button.
      */
      XSetTextColor(display,window_info,MagickTrue);
      x=button_info->x+(int) button_info->width+(int) button_info->bevel_width+
        (QuantumMargin >> 1);
      (void) XDrawString(display,window_info->id,window_info->widget_context,
        x,y,button_info->text,Extent(button_info->text));
      return;
    }
  (void) XSetClipRectangles(display,window_info->widget_context,0,0,&crop_info,
    1,Unsorted);
  XSetTextColor(display,window_info,button_info->raised);
  (void) XDrawString(display,window_info->id,window_info->widget_context,x,y,
    button_info->text,Extent(button_info->text));
  (void) XSetClipMask(display,window_info->widget_context,None);
  if (button_info->raised == MagickFalse)
    XDelay(display,SuspendTime << 2);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w B e v e l e d M a t t e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawBeveledMatte() draws a matte with a shadowed upper and left bevel and
%  a highlighted lower and right bevel.  The highlighted and shadowed bevels
%  create a 3-D effect.
%
%  The format of the XDrawBeveledMatte function is:
%
%      XDrawBeveledMatte(display,window_info,matte_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o matte_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the matte.
%
*/
static inline void XDrawBeveledMatte(Display *display,const XWindowInfo *window_info,
  const XWidgetInfo *matte_info)
{
  /*
    Draw matte.
  */
  XDrawBevel(display,window_info,matte_info);
  XDrawMatte(display,window_info,matte_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w M a t t e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawMatte() fills a rectangular area with the matte color.
%
%  The format of the XDrawMatte function is:
%
%      XDrawMatte(display,window_info,matte_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o matte_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the matte.
%
*/
static inline void XDrawMatte(Display *display,const XWindowInfo *window_info,
  const XWidgetInfo *matte_info)
{
  /*
    Draw matte.
  */
  if ((matte_info->trough == MagickFalse) || (window_info->depth == 1))
    (void) XFillRectangle(display,window_info->id,
      window_info->highlight_context,matte_info->x,matte_info->y,
      matte_info->width,matte_info->height);
  else
    {
      (void) XSetForeground(display,window_info->widget_context,
        window_info->pixel_info->trough_color.pixel);
      (void) XFillRectangle(display,window_info->id,window_info->widget_context,
        matte_info->x,matte_info->y,matte_info->width,matte_info->height);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w M a t t e T e x t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawMatteText() draws a matte with text.  If the text exceeds the extents
%  of the text, a portion of the text relative to the cursor is displayed.
%
%  The format of the XDrawMatteText function is:
%
%      XDrawMatteText(display,window_info,text_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o text_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the text.
%
*/
static inline void XScrollTextMarker(XFontStruct *font_info,XWidgetInfo *text_info,
  const unsigned int width)
{
  const char
    *text;

  int
    i,
    n;

  text=text_info->text;
  for (i=0; i < Extent(text); i++)
  {
    n=XTextWidth(font_info,(char *) text+i,(int)
      (text_info->cursor-text-i));
    if (n <= (int) width)
      break;
  }
  text_info->marker=(char *) text+i;
}

static inline void XSetVisibleTextMarker(XFontStruct *font_info,
  XWidgetInfo *text_info,const unsigned int width)
{
  const char
    *text;

  if (text_info->cursor < text_info->marker)
    text_info->marker=text_info->cursor;
  else
    {
      text=text_info->marker;
      if (XTextWidth(font_info,(char *) text,(int) (text_info->cursor-text)) >
          (int) width)
        XScrollTextMarker(font_info,text_info,width);
    }
}

static inline void XDrawMatteText(Display *display,const XWindowInfo *window_info,
  XWidgetInfo *text_info)
{
  int
    x,
    y;

  unsigned int
    height,
    width;

  XFontStruct
    *font_info;

  XRectangle
    crop_info;

  /*
    Clear the text area.
  */
  XSetMatteColor(display,window_info,MagickFalse);
  (void) XFillRectangle(display,window_info->id,window_info->widget_context,
    text_info->x,text_info->y,text_info->width,text_info->height);
  if (text_info->text == (char *) NULL)
    return;
  XSetTextColor(display,window_info,text_info->highlight);
  font_info=window_info->font_info;
  x=text_info->x+(int) (QuantumMargin >> 2);
  y=text_info->y+font_info->ascent+(int) (text_info->height >> 2);
  width=text_info->width-(unsigned int) (QuantumMargin >> 1);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  if (*text_info->text == '\0')
    {
      /*
        No text-- just draw cursor.
      */
      (void) XDrawLine(display,window_info->id,window_info->annotate_context,
        x,y+3,x,y-(int) height+3);
      return;
    }
  /*
    Set cropping region.
  */
  crop_info.width=(unsigned short) text_info->width;
  crop_info.height=(unsigned short) text_info->height;
  crop_info.x=text_info->x;
  crop_info.y=text_info->y;
  /*
    Determine beginning of the visible text.
  */
  XSetVisibleTextMarker(font_info,text_info,width);
  /*
    Draw text and cursor.
  */
  if (text_info->highlight == MagickFalse)
    {
      (void) XSetClipRectangles(display,window_info->widget_context,0,0,
        &crop_info,1,Unsorted);
      (void) XDrawString(display,window_info->id,window_info->widget_context,
        x,y,text_info->marker,Extent(text_info->marker));
      (void) XSetClipMask(display,window_info->widget_context,None);
    }
  else
    {
      (void) XSetClipRectangles(display,window_info->annotate_context,0,0,
        &crop_info,1,Unsorted);
      width=WidgetTextWidth(font_info,text_info->marker);
      (void) XFillRectangle(display,window_info->id,
        window_info->annotate_context,x,y-font_info->ascent,width,height);
      (void) XSetClipMask(display,window_info->annotate_context,None);
      (void) XSetClipRectangles(display,window_info->highlight_context,0,0,
        &crop_info,1,Unsorted);
      (void) XDrawString(display,window_info->id,
        window_info->highlight_context,x,y,text_info->marker,
        Extent(text_info->marker));
      (void) XSetClipMask(display,window_info->highlight_context,None);
    }
  x+=XTextWidth(font_info,text_info->marker,(int)
    (text_info->cursor-text_info->marker));
  (void) XDrawLine(display,window_info->id,window_info->annotate_context,x,y+3,
    x,y-(int) height+3);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w W i d g e t T e x t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawWidgetText() first clears the widget and draws a text string justified
%  left (or center) in the x-direction and centered within the y-direction.
%
%  The format of the XDrawWidgetText function is:
%
%      XDrawWidgetText(display,window_info,text_info)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a XWindowText structure.
%
%    o text_info: Specifies a pointer to XWidgetInfo structure.
%
*/
static inline void XDrawWidgetText(Display *display,const XWindowInfo *window_info,
  XWidgetInfo *text_info)
{
  GC
    widget_context;

  int
    x,
    y;

  unsigned int
    height,
    width;

  XFontStruct
    *font_info;

  XRectangle
    crop_info;

  /*
    Clear the text area.
  */
  widget_context=window_info->annotate_context;
  if (text_info->raised)
    (void) XClearArea(display,window_info->id,text_info->x,text_info->y,
      text_info->width,text_info->height,MagickFalse);
  else
    {
      (void) XFillRectangle(display,window_info->id,widget_context,text_info->x,
        text_info->y,text_info->width,text_info->height);
      widget_context=window_info->highlight_context;
    }
  if (text_info->text == (char *) NULL)
    return;
  if (*text_info->text == '\0')
    return;
  /*
    Set cropping region.
  */
  font_info=window_info->font_info;
  crop_info.width=(unsigned short) text_info->width;
  crop_info.height=(unsigned short) text_info->height;
  crop_info.x=text_info->x;
  crop_info.y=text_info->y;
  /*
    Draw text.
  */
  width=WidgetTextWidth(font_info,text_info->text);
  x=text_info->x+(int) (QuantumMargin >> 1);
  if (text_info->center)
    x=text_info->x+(int) (text_info->width >> 1)-(int) (width >> 1);
  if (text_info->raised)
    if (width > (text_info->width-(unsigned int) QuantumMargin))
      x+=(int) (text_info->width-(unsigned int) QuantumMargin-width);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  y=text_info->y+(int) ((text_info->height-height) >> 1)+font_info->ascent;
  (void) XSetClipRectangles(display,widget_context,0,0,&crop_info,1,Unsorted);
  (void) XDrawString(display,window_info->id,widget_context,x,y,text_info->text,
    Extent(text_info->text));
  (void) XSetClipMask(display,widget_context,None);
  if (x < text_info->x)
    (void) XDrawLine(display,window_info->id,window_info->annotate_context,
      text_info->x,text_info->y,text_info->x,text_info->y+(int)
      text_info->height-1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X E d i t T e x t                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XEditText() edits a text string as indicated by the key symbol.
%
%  The format of the XEditText function is:
%
%      XEditText(display,text_info,key_symbol,text,state)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o text_info: Specifies a pointer to a XWidgetInfo structure.  It
%      contains the extents of the text.
%
%    o key_symbol:  A X11 KeySym that indicates what editing function to
%      perform to the text.
%
%    o text: A character string to insert into the text.
%
%    o state:  An size_t that indicates whether the key symbol is a
%      control character or not.
%
*/
static inline void XInsertWidgetText(XWidgetInfo *text_info,char *text)
{
  char
    *p,
    *q;

  int
    i;

  q=text_info->text+Extent(text_info->text)+strlen(text);
  for (i=0; i <= Extent(text_info->cursor); i++)
  {
    if ((q-Extent(text)) > text_info->text)
      *q=(*(q-Extent(text)));
    q--;
  }
  p=text;
  for (i=0; i < Extent(text); i++)
    *text_info->cursor++=(*p++);
}

static inline void XEraseHighlightedText(XWidgetInfo *text_info)
{
  if (text_info->highlight)
    {
      /*
        Erase the entire line of text.
      */
      *text_info->text='\0';
      text_info->cursor=text_info->text;
      text_info->marker=text_info->text;
      text_info->highlight=MagickFalse;
    }
}

static inline void XEditTextInsertKey(Display *display,XWidgetInfo *text_info,
  char *text,const size_t state)
{
  if (state & ControlState)
    return;
  if (*text == '\0')
    return;
  if ((Extent(text_info->text)+1) >= (int) MagickPathExtent)
    (void) XBell(display,0);
  else
    {
      XEraseHighlightedText(text_info);
      /*
        Insert a string into the text.
      */
      XInsertWidgetText(text_info,text);
    }
}

static inline void XEditTextCursorLeft(XWidgetInfo *text_info)
{
  /*
    Move cursor one position left.
  */
  if (text_info->cursor == text_info->text)
    return;
  text_info->cursor--;
}

static inline void XEditTextCursorRight(XWidgetInfo *text_info)
{
  /*
    Move cursor one position right.
  */
  if (text_info->cursor == (text_info->text+Extent(text_info->text)))
    return;
  text_info->cursor++;
}

static inline void XEditTextBackspace(XWidgetInfo *text_info)
{
  if (text_info->highlight)
    {
      /*
        Erase the entire line of text.
      */
      *text_info->text='\0';
      text_info->cursor=text_info->text;
      text_info->marker=text_info->text;
      text_info->highlight=MagickFalse;
    }
  /*
    Erase one character.
  */
  if (text_info->cursor != text_info->text)
    {
      text_info->cursor--;
      (void) memmove(text_info->cursor,text_info->cursor+1,
        strlen(text_info->cursor+1)+1);
      text_info->highlight=MagickFalse;
      return;
    }
  XEditTextCursorLeft(text_info);
}

static inline void XEditText(Display *display,XWidgetInfo *text_info,
  const KeySym key_symbol,char *text,const size_t state)
{
  switch ((int) key_symbol)
  {
    case XK_BackSpace:
    case XK_Delete:
    {
      XEditTextBackspace(text_info);
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      XEditTextCursorLeft(text_info);
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      XEditTextCursorRight(text_info);
      break;
    }
    default:
    {
      XEditTextInsertKey(display,text_info,text,state);
      break;
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X G e t W i d g e t I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XGetWidgetInfo() initializes the XWidgetInfo structure.
%
%  The format of the XGetWidgetInfo function is:
%
%      XGetWidgetInfo(text,widget_info)
%
%  A description of each parameter follows:
%
%    o text: A string of characters associated with the widget.
%
%    o widget_info: Specifies a pointer to a X11 XWidgetInfo structure.
%
*/
static inline void XGetWidgetInfo(const char *text,XWidgetInfo *widget_info)
{
  /*
    Initialize widget info.
  */
  widget_info->id=(~0);
  widget_info->bevel_width=3;
  widget_info->width=1;
  widget_info->height=1;
  widget_info->x=0;
  widget_info->y=0;
  widget_info->min_y=0;
  widget_info->max_y=0;
  widget_info->raised=MagickTrue;
  widget_info->active=MagickFalse;
  widget_info->center=MagickTrue;
  widget_info->trough=MagickFalse;
  widget_info->highlight=MagickFalse;
  widget_info->text=(char *) text;
  widget_info->cursor=(char *) text;
  if (text != (char *) NULL)
    widget_info->cursor+=Extent(text);
  widget_info->marker=(char *) text;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X H i g h l i g h t W i d g e t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XHighlightWidget() draws a highlighted border around a window.
%
%  The format of the XHighlightWidget function is:
%
%      XHighlightWidget(display,window_info,x,y)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o x: Specifies an integer representing the rectangle offset in the
%      x-direction.
%
%    o y: Specifies an integer representing the rectangle offset in the
%      y-direction.
%
*/
static inline void XHighlightWidget(Display *display,const XWindowInfo *window_info,
  const int x,const int y)
{
  /*
    Draw the widget highlighting rectangle.
  */
  XSetBevelColor(display,window_info,MagickTrue);
  (void) XDrawRectangle(display,window_info->id,window_info->widget_context,x,y,
    (unsigned int) ((int) window_info->width-(x << 1)),(unsigned int)
    ((int) window_info->height-(y << 1)));
  (void) XDrawRectangle(display,window_info->id,window_info->widget_context,
    x-1,y-1,(unsigned int) ((int) window_info->width-(x << 1)+1),(unsigned int)
    ((int) window_info->height-(y << 1)+1));
  XSetBevelColor(display,window_info,MagickFalse);
  (void) XDrawRectangle(display,window_info->id,window_info->widget_context,
    x-1,y-1,(unsigned int) ((int) window_info->width-(x << 1)),(unsigned int)
    ((int) window_info->height-(y << 1)));
  (void) XSetFillStyle(display,window_info->widget_context,FillSolid);
}

static inline void XScreenExpose(Display *display,XWindows *windows,XEvent *event)
{
  if (event->xexpose.window == windows->image.id)
    {
      XRefreshWindow(display,&windows->image,event);
      return;
    }
  if (event->xexpose.window == windows->magnify.id)
    if ((event->xexpose.count == 0) && (windows->magnify.mapped))
      {
        ExceptionInfo
          *exception;

        exception=AcquireExceptionInfo();
        XMakeMagnifyImage(display,windows,exception);
        exception=DestroyExceptionInfo(exception);
        return;
      }
  if ((event->xexpose.window == windows->command.id) &&
      (event->xexpose.count == 0))
    {
      (void) XCommandWidget(display,windows,(const char *const *) NULL,
        event);
      return;
    }
}

static inline void XScreenTrackMapping(const XEvent *event,MagickBooleanType *mapped)
{
  if (event->type == MapNotify)
    (*mapped)=MagickTrue;
  if (event->type == UnmapNotify)
    (*mapped)=MagickFalse;
}

static inline void XScreenConvertAltButton(XEvent *event)
{
  if ((event->xbutton.button == Button3) &&
      (event->xbutton.state & Mod1Mask))
    {
      /*
        Convert Alt-Button3 to Button2.
      */
      event->xbutton.button=Button2;
      event->xbutton.state&=(unsigned int) (~Mod1Mask);
    }
}

static inline int XScreenInputEvent(const XEvent *event)
{
  switch (event->type)
  {
    case ButtonRelease:
    case KeyPress:
    case KeyRelease:
    case MotionNotify:
    case SelectionNotify:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static inline int XScreenEvent(Display *display,XEvent *event,char *data)
{
  XWindows
    *windows;

  windows=(XWindows *) data;
  if (event->xany.window == windows->popup.id)
    {
      XScreenTrackMapping(event,&windows->popup.mapped);
      return(MagickTrue);
    }
  if (event->xany.window == windows->widget.id)
    {
      XScreenTrackMapping(event,&windows->widget.mapped);
      return(MagickTrue);
    }
  switch (event->type)
  {
    case ButtonPress:
    {
      XScreenConvertAltButton(event);
      return(MagickTrue);
    }
    case Expose:
    {
      XScreenExpose(display,windows,event);
      break;
    }
    case FocusOut:
    {
      /*
        Set input focus for backdrop window.
      */
      if (event->xfocus.window == windows->image.id)
        (void) XSetInputFocus(display,windows->image.id,RevertToNone,
          CurrentTime);
      return(MagickTrue);
    }
    default:
      return(XScreenInputEvent(event));
  }
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S e t B e v e l C o l o r                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSetBevelColor() sets the graphic context for drawing a beveled border.
%
%  The format of the XSetBevelColor function is:
%
%      XSetBevelColor(display,window_info,raised)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o raised: A value other than zero indicates the color show be a
%      "highlight" color, otherwise the "shadow" color is set.
%
*/
static inline void XSetBevelColor(Display *display,const XWindowInfo *window_info,
  const MagickStatusType raised)
{
  if (window_info->depth == 1)
    {
      Pixmap
        stipple;

      /*
        Monochrome window.
      */
      (void) XSetBackground(display,window_info->widget_context,
        XBlackPixel(display,window_info->screen));
      (void) XSetForeground(display,window_info->widget_context,
        XWhitePixel(display,window_info->screen));
      (void) XSetFillStyle(display,window_info->widget_context,
        FillOpaqueStippled);
      stipple=window_info->highlight_stipple;
      if (raised == MagickFalse)
        stipple=window_info->shadow_stipple;
      (void) XSetStipple(display,window_info->widget_context,stipple);
    }
  else
    if (raised)
      (void) XSetForeground(display,window_info->widget_context,
        window_info->pixel_info->highlight_color.pixel);
    else
      (void) XSetForeground(display,window_info->widget_context,
        window_info->pixel_info->shadow_color.pixel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S e t M a t t e C o l o r                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSetMatteColor() sets the graphic context for drawing the matte.
%
%  The format of the XSetMatteColor function is:
%
%      XSetMatteColor(display,window_info,raised)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o raised: A value other than zero indicates the matte is active.
%
*/
static inline void XSetMatteColor(Display *display,const XWindowInfo *window_info,
  const MagickStatusType raised)
{
  if (window_info->depth == 1)
    {
      /*
        Monochrome window.
      */
      if (raised)
        (void) XSetForeground(display,window_info->widget_context,
          XWhitePixel(display,window_info->screen));
      else
        (void) XSetForeground(display,window_info->widget_context,
          XBlackPixel(display,window_info->screen));
    }
  else
    if (raised)
      (void) XSetForeground(display,window_info->widget_context,
        window_info->pixel_info->matte_color.pixel);
    else
      (void) XSetForeground(display,window_info->widget_context,
        window_info->pixel_info->depth_color.pixel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S e t T e x t C o l o r                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSetTextColor() sets the graphic context for drawing text on a matte.
%
%  The format of the XSetTextColor function is:
%
%      XSetTextColor(display,window_info,raised)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o window_info: Specifies a pointer to a X11 XWindowInfo structure.
%
%    o raised: A value other than zero indicates the color show be a
%      "highlight" color, otherwise the "shadow" color is set.
%
*/
static inline void XSetTextColor(Display *display,const XWindowInfo *window_info,
  const MagickStatusType raised)
{
  ssize_t
    foreground,
    matte;

  if (window_info->depth == 1)
    {
      /*
        Monochrome window.
      */
      if (raised)
        (void) XSetForeground(display,window_info->widget_context,
          XBlackPixel(display,window_info->screen));
      else
        (void) XSetForeground(display,window_info->widget_context,
          XWhitePixel(display,window_info->screen));
      return;
    }
  foreground=(ssize_t) XPixelIntensity(
    &window_info->pixel_info->foreground_color);
  matte=(ssize_t) XPixelIntensity(&window_info->pixel_info->matte_color);
  if (MagickAbsoluteValue((int) (foreground-matte)) > (65535L >> 3))
    (void) XSetForeground(display,window_info->widget_context,
      window_info->pixel_info->foreground_color.pixel);
  else
    (void) XSetForeground(display,window_info->widget_context,
      window_info->pixel_info->background_color.pixel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X C o l o r B r o w s e r W i d g e t                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XColorBrowserWidget() displays a Color Browser widget with a color query
%  to the user.  The user keys a reply and presses the Action or Cancel button
%  to exit.  The typed text is returned as the reply function parameter.
%
%  The format of the XColorBrowserWidget method is:
%
%      void XColorBrowserWidget(Display *display,XWindows *windows,
%        const char *action,char *reply)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o window: Specifies a pointer to a XWindows structure.
%
%    o action: Specifies a pointer to the action of this widget.
%
%    o reply: the response from the user is returned in this parameter.
%
*/
static inline MagickBooleanType IsUnusableSelection(const Status status,const Atom type,
  const int format,const unsigned long length)
{
  return(((status != Success) || (type != XA_STRING) || (format == 32) ||
    (length == 0)) ? MagickTrue : MagickFalse);
}

static inline size_t ConfigureWidgetWindow(XWindows *windows,const XEvent event,
  size_t state)
{
  /*
    Update widget configuration.
  */
  if (event.xconfigure.window != windows->widget.id)
    return(state);
  if ((event.xconfigure.width == (int) windows->widget.width) &&
      (event.xconfigure.height == (int) windows->widget.height))
    return(state);
  windows->widget.width=(unsigned int)
    MagickMax(event.xconfigure.width,(int) windows->widget.min_width);
  windows->widget.height=(unsigned int)
    MagickMax(event.xconfigure.height,(int) windows->widget.min_height);
  state|=UpdateConfigurationState;
  return(state);
}

static inline size_t EnterWidgetWindow(XWindows *windows,const XEvent event,
  size_t state)
{
  if (event.xcrossing.window != windows->widget.id)
    return(state);
  state&=(unsigned int) (~InactiveWidgetState);
  return(state);
}

static inline size_t ExposeWidgetWindow(XWindows *windows,const XEvent event,
  size_t state)
{
  if (event.xexpose.window != windows->widget.id)
    return(state);
  if (event.xexpose.count != 0)
    return(state);
  state|=RedrawWidgetState;
  return(state);
}

static inline size_t LeaveWidgetWindow(XWindows *windows,const XEvent event,
  size_t state)
{
  if (event.xcrossing.window != windows->widget.id)
    return(state);
  state|=InactiveWidgetState;
  return(state);
}

static inline size_t ClientWidgetMessage(Display *display,XWindows *windows,
  const XEvent event,size_t state)
{
  /*
    If client window delete message, exit.
  */
  if (event.xclient.message_type != windows->wm_protocols)
    return(state);
  if (*event.xclient.data.l == (int) windows->wm_take_focus)
    {
      (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
        (Time) event.xclient.data.l[1]);
      return(state);
    }
  if (*event.xclient.data.l != (int) windows->wm_delete_window)
    return(state);
  if (event.xclient.window == windows->widget.id)
    {
      state|=ExitState;
      return(state);
    }
  return(state);
}

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

#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
